# Performance

Broadway sends rasterized content to the browser as PNG uploads, so the dominant cost is uploading
the same pixels twice. The [paint-flash profiler](debug-menu.md) made this visible: scrolling or
hovering a list flashed **red** (real uploads) almost everywhere, because identical content was
re-rasterized and re-uploaded each frame. Two tiers of reuse plus a native edge-fade cut that
traffic.

## Tier 1: content node reuse

*New in v2*

`gskbroadwayrenderer.c` already reused nodes by `GskRenderNode` *pointer* identity across frames,
emitting `BROADWAY_NODE_REUSE`. The problem is that widgets like `GtkTreeView` rebuild all their cell
nodes every snapshot, so pointer identity always missed and every cell re-rasterized. The fork adds a
second key: a content hash (FNV-1a over glyphs/colour/font/bounds/offset) compared against last
frame's. A re-snapshotted but otherwise identical node then reuses last frame's id without
re-rasterizing.

## Tier 2: cross-time texture dedup

*New in v2*

When tier 1 misses because the node genuinely moved (a scroll, say), the texture is re-rasterized into
a new `GdkTexture`. Textures were deduped only by `GdkTexture` *object* identity, so the fresh object
got a fresh PNG upload even when the pixels were identical.

`gdk_broadway_display_ensure_texture` now keeps a content cache on `GdkBroadwayDisplay`, keyed on
`(width, height, format, FNV-1a-64 of the raw downloaded pixels)`, reusing the already-uploaded
broadway texture id instead of re-uploading.

Pixels are hashed via `gdk_texture_download` (which normalizes to `GDK_MEMORY_DEFAULT`) rather than by
encoding a PNG just to compute the key. Entries are refcounted: an id is released (and dropped from the
browser) only once no live `GdkTexture` references it, and LRU eviction skips live entries. A live
texture's id is never pulled out from under it, since an earlier non-refcounted cut crashed the browser
exactly that way. The cache is LRU-capped at 512 entries, and textures larger than 512x512 px skip it
because the download and hash cost is not worth it and they rarely repeat. The fast path, an object
already uploaded, returns its id with no re-hash, so steady-state cost is unchanged.

Net effect: first paint of new content still uploads (red); re-scrolling or re-hovering seen content
reuses (magenta/green) and the Traffic counter climbs far more slowly.

## Native notebook edge-fade

*New in v2*

The [notebook](../features/notebook.md) tab-scroll edge-fade used `gtk_snapshot_push_mask`, which
Broadway rasterizes (there is no `GskMaskNode` renderer) into a texture re-uploaded every scroll frame.
It is now drawn as themed CSS `undershoot` nodes: native `GSK_LINEAR_GRADIENT_NODE`s fading into the
header `$dark_fill`. That costs zero texture traffic and stays theme-correct.

## What still rasterizes

Genuine per-frame animation, where each frame really is different pixels, still uploads. The scroll
**overshoot** shadow and the `GtkSwitch` knob mid-toggle both use `radial-gradient`, which falls back
to a cairo texture; the dedup catches only their settled states. Native radial-gradient support in the
Broadway renderer would remove the rest, but that is out of scope here.

# Transport and resources

The reuse tiers above cut *texture* traffic. A second pass trims the rest of the per-frame cost:
redundant wire ops, input latency, syscalls, and unbounded buffers.

## Wire: drop empty frames

*New in v2*

When a surface's frame diff produces no ops, `broadway-output.c` drops the whole `SET_NODES` instead
of sending an 11-byte no-op (and its forced flush); the serial is rolled back so the counter stays
dense.

A companion change that also deduped repeated **show-keyboard** / **input-region** ops was tried and
reverted. broadwayd keeps that state across a page reload but never resets it on client disconnect, so
the redundant re-sends were inadvertently re-syncing a reconnecting client. Suppressing them left a
stale OSK or empty-input-region state on the new client that popped the keyboard on first tap and
swallowed taps on mobile. Dedup here is only safe once disconnect resets the flags, or once the reconnect
sync re-asserts both states rather than just the active one.

## Input latency: pointer-move coalescing

*New in v2*

A high-Hz mouse or trackpad fires many `mousemove` events per displayed frame, but GTK only needs the
latest position. `broadway.js` now buffers moves and sends one per animation frame, so a motion flood
can't fill the websocket and delay a following click or key ([head-of-line
blocking](https://en.wikipedia.org/wiki/Head-of-line_blocking)). Any discrete event flushes the
pending move first, preserving order.

## CPU: fewer syscalls and allocations

*New in v2*

Three hot-path trims. In `broadway-output.c` the WebSocket header and payload now go out in one
`g_output_stream_writev_all` instead of two `write_all`s, with no payload copy. In `broadway.js`,
`sendInput` packs fields straight into the buffer, dropping the per-event `concat()`/`forEach()` closure
on the move/wheel hot path. And in `broadway-server.c`, since only texture nodes carry a client texture
id, the remap moved out of the node-data copy loop so the common case skips a per-word comparison.

## Memory bounding

*New in v2*

Two caps. A frame uploading a large texture can grow the `broadway-output.c` output buffer to many MB,
and `g_string_set_size(.., 0)` keeps that capacity for the connection's life; after an oversized flush
the buffer is now freed and restarted small. Separately, the per-source-texture recolor cache in
`gskbroadwayrenderer.c` (each entry a full decoded copy) is bounded to 16 entries, LRU, so recoloring
one texture many ways (symbolic icons across states and themes) can't grow it without limit.

## A note on browser differences

One experiment released the JS `Image` used to drive `decode()` once it settled, to reclaim the decoded
bitmap. It rendered fine in Chrome but produced thin-line artifacts in Firefox, which discards the
decoded data once no `Image` references it and re-decodes lazily on paint, so it was dropped. The
rendered `<img>` nodes load from the texture URL, so anything that lets the browser evict decoded data
out from under a pending paint is unsafe to assume across browsers.
