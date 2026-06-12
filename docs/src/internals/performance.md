# Performance

Broadway rasterizes content and sends it to the browser as PNG uploads, so the dominant cost is uploading the same pixels twice. A widget like `GtkTreeView` rebuilds all its cell nodes every snapshot, so naive reuse misses and every cell re-rasterizes and re-uploads each frame.

A live, server-side profiler makes this visible: the [paint-flash overlay](#profiling-and-diagnosis) colours every changed node by upload cost, and the [debug menu](debug-menu.md) shows per-window traffic and frame-pacing stats. The fork's reuse tiers, transport trims, and a native edge-fade are all tuned against these readings.

## Profiling and diagnosis

The profiler lives in the [debug menu](debug-menu.md); see there for how it is wired into `broadwayd`. This section covers how to read it.

### Opening it

Press **Shift three times quickly** (triple-Shift) to summon the server-side debug overlay - a native GTK4 window composited into the same Broadway display the app uses. Its **Paint flashing** toggle turns on the paint-flash overlay.

### Performance-section metrics

- **Traffic** - total bytes sent. Should stay flat on a static screen and climb slowly during reuse-friendly interaction.
- **Framerate** - displayed FPS. Hides stutter; use the Smoothness section for that.
- **Latency** - real browser-to-daemon RTT, timed off the [heartbeat](../features/connection.md) PING/PONG.
- **Textures** - count and summed PNG bytes the browser holds (the warm working set of the [content texture cache](#tier-2-lru-texture-content-cache)), with two rates:
  - **up/s** - uploads per second, i.e. the content-cache *miss* rate (cache hits never cross the wire). Sustained `up/s` on a static screen means the cache is thrashing.
  - **release/s** - texture ids dropped per second as their last referencing `GdkTexture` dies.

### Smoothness-section metrics

- **Frame** - p95 / max gap between real display pushes over a rolling 64-frame window. Gaps over 250 ms are treated as idle and dropped, so the numbers reflect continuous rendering. The overlay repaints itself at ~2/s, its own noise floor, so read Frame under load, not at rest.
- **Write** - time blocked in the socket `writev` (~0 on localhost; spikes mean browser or network backpressure) plus bytes-per-frame.

### Reading the paint-flash overlay

Each changed node gets a translucent overlay coloured by cost:

- **green** - node and texture both reused; no work.
- **magenta** - node re-sent but its texture was cached; cheap.
- **red** - texture uploaded this frame; real bandwidth.

### Good vs bad readings

- **Static screen** - flat Traffic, `up/s` ~0, no flashes. Anything else means the cache is missing on idle content.
- **Healthy scroll** - a brief `up/s` spike (first paint of newly exposed content, flashing red), then it settles as scroll-back hits the cache (green/magenta) and Traffic levels off.
- **Cache thrashing** - sustained `up/s` or red flashes on a screen that is not changing. The working set is being evicted and re-uploaded.
- **Stutter** - sustained Frame gaps under load even when Traffic is modest.

## Rendering reuse

*New in v2*

### Tier 1: content-hash node reuse

`gskbroadwayrenderer.c` reused nodes by `GskRenderNode` *pointer* identity across frames (`BROADWAY_NODE_REUSE`), but rebuilt cell nodes always miss that key. The fork adds a content hash (FNV-1a over glyphs, colour, font, bounds, offset) compared against last frame's, so a re-snapshotted but otherwise identical node reuses last frame's id without re-rasterizing. Content hashes are tagged with the node type, so equal bytes from different node kinds can't alias. The pointer path stores the offset each node's geometry was encoded against (the nearest enclosing clip origin) and re-encodes when it changed - the baked parent-local geometry would otherwise land displaced after a relayout.

### Tier 2: LRU texture content cache

When tier 1 misses because a node genuinely moved (a scroll), the texture is re-rasterized into a new `GdkTexture`; dedup by `GdkTexture` object identity then misses too and re-uploads identical pixels. `gdk_broadway_display_ensure_texture` adds a content cache on `GdkBroadwayDisplay` keyed on `(width, height, native format, color state, two independent 64-bit hashes of the downloaded pixels)`, reusing the uploaded broadway id. Pixels are hashed via `gdk_texture_download` (normalized to `GDK_MEMORY_DEFAULT`), not by encoding a PNG - but the key keeps the native format and color state, since the wire PNG encodes those and identical downloads can still differ on the wire. The FNV-1a-64 walks the buffer a 64-bit word at a time (with a byte tail), ~8x fewer iterations than per-byte; a second independent hash computed in the same pass keeps a single 64-bit collision from aliasing unrelated images for the LRU's lifetime. Entries are refcounted: an id is released and dropped from the browser only when no live `GdkTexture` references it, and LRU eviction skips live entries (pulling a live id earlier crashed the browser). The cache is LRU-capped at 4096; textures over 512x512 px skip it (download+hash cost not worth it, they rarely repeat); the fast path - an object already uploaded - returns its id with no re-hash (it only relinks the entry to the LRU head, so a per-frame-redrawn texture stays hot instead of aging out), so steady-state cost is unchanged. The cap was raised from 512 after `up/s` showed it undersized: a dense treeview's working set runs ~1-2k textures, so at 512 anything scrolled off-screen was evicted and re-uploaded on the way back. At 4096 scroll-back is cache hits (cost is ~4 KB/texture of browser RAM).

## Transport and CPU

The reuse tiers cut *texture* traffic. A second pass trims the rest of the per-frame cost: redundant wire ops, input latency, syscalls, and unbounded buffers.

### PNG encode: cost-vs-size preset

*New in v3*

Every cache miss re-encodes a texture to PNG on the app's frame path, so libpng's settings are a real CPU/latency-vs-size knob. `gdk_save_png` never set a zlib level, filter, or strategy (libpng defaults: level 6, adaptive filter tries all 5 per row). `gdk_save_png_full` exposes the three knobs (`-1` keeps the default), driven by a preset in `gdkbroadway-server.c`: **fast** (adaptive filter, level 3) is the default for localhost/LAN where encode latency is the cost; **compact** (adaptive filter, level 7) suits remote/metered links. Only the zlib level differs between presets; both keep adaptive filtering. Seeded from `BROADWAY_PNG`, switched live from the debug menu (`png-preset` -> `BROADWAY_EVENT_SET_PNG`, since the app encodes, not the daemon).

An earlier fast preset used a single Sub filter + `Z_RLE` to minimize encode CPU, but measured against real treeview textures (capture the on-wire PNGs, re-encode the same pixels each way) it *grew* the output ~50% - median +51%, +120% over a 12-texture frame, up to +408% on a smooth gradient. `Sub`+`Z_RLE` compresses anti-aliased text and gradients far worse than adaptive filtering, which picks Up/Paeth on vertical coherence. That traded ~4x cheaper server encode for ~2.2x more bytes per frame, shifting cost to client transfer and image-decode - exactly what scrolling is bound by - so it hurt scroll smoothness while leaving averaged FPS unmoved. The **filter**, not the level, drove the bloat: level 3 with adaptive filtering lands within ~5-12% of the level-6 default while still encoding cheaper, so fast keeps the low level and drops the `Sub`+`Z_RLE`.

### Wire: drop empty frames

*New in v2*

When a surface's frame diff produces no ops, `broadway-output.c` drops the whole `SET_NODES` instead of sending an 11-byte no-op and its forced flush; the serial is rolled back so the counter stays dense.

A companion change that also deduped repeated **show-keyboard** / **input-region** ops was reverted: broadwayd keeps that state across a page reload but never resets it on client disconnect, so the redundant re-sends were inadvertently re-syncing a reconnecting client. Suppressing them left a stale OSK or empty-input-region state that popped the keyboard on first tap and swallowed taps on mobile. Dedup here is safe only once disconnect resets the flags or the reconnect sync re-asserts both states.

### Input latency: pointer-move coalescing

*New in v2*

A high-Hz mouse or trackpad fires many `mousemove` events per displayed frame, but GTK only needs the latest position. `broadway.js` buffers moves and sends one per animation frame, so a motion flood can't fill the websocket and delay a following click or key ([head-of-line blocking](https://en.wikipedia.org/wiki/Head-of-line_blocking)). Any discrete event flushes the pending move first, preserving order.

### CPU: fewer syscalls and allocations

*New in v2*

Three hot-path trims. In `broadway-output.c` the WebSocket header and payload go out in one `g_output_stream_writev_all` instead of two `write_all`s, with no payload copy. In `broadway.js`, `sendInput` packs fields straight into the buffer, dropping the per-event `concat()`/`forEach()` closure on the move/wheel hot path. In `broadway-server.c`, since only texture nodes carry a client texture id, the remap moved out of the node-data copy loop so the common case skips a per-word comparison.

### Memory bounding

*New in v2*

Two caps. A frame uploading a large texture can grow the `broadway-output.c` output buffer to many MB, and `g_string_set_size(.., 0)` keeps that capacity for the connection's life; after an oversized flush the buffer is now freed and restarted small. Separately, the per-source-texture recolor cache in `gskbroadwayrenderer.c` (each entry a full decoded copy) is bounded to 16 entries, LRU, so recoloring one texture many ways (symbolic icons across states and themes) can't grow without limit. The color-matrix caller hands its texture ref to the per-frame pin, leaving the cache entry as the only owner, so eviction actually frees the decoded copy (an extra ref used to keep every evicted entry alive).

## Limitations

**Per-frame animation still rasterizes.** Where each frame really is different pixels, uploads are unavoidable. The scroll **overshoot** shadow and the `GtkSwitch` knob mid-toggle both use `radial-gradient`, which falls back to a cairo texture; the dedup catches only their settled states. Native radial-gradient support in the Broadway renderer would remove the rest, but that is out of scope.

**Notebook edge-fade (solved natively).** The [notebook](../features/notebook.md) tab-scroll edge-fade used `gtk_snapshot_push_mask`, which Broadway rasterizes (no `GskMaskNode` renderer) into a texture re-uploaded every scroll frame. It is now drawn as themed CSS `undershoot` nodes: native `GSK_LINEAR_GRADIENT_NODE`s fading into the header `$dark_fill`. Zero texture traffic, theme-correct - the model for moving an effect off the rasterizer.

**Browser image-decode differences.** One experiment released the JS `Image` driving `decode()` once it settled, to reclaim the decoded bitmap. It rendered fine in Chrome but produced thin-line artifacts in Firefox, which discards decoded data once no `Image` references it and re-decodes lazily on paint, so it was dropped. The rendered `<img>` nodes load from the texture URL, so anything that lets the browser evict decoded data out from under a pending paint is unsafe to assume across browsers.
