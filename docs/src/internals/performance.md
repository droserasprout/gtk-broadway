# Performance

Broadway sends rasterized content to the browser as PNG uploads, so the dominant cost is uploading
the same pixels twice. The [paint-flash profiler](debug-menu.md) made this visible: scrolling or
hovering a list flashed **red** (real uploads) almost everywhere, because identical content was
re-rasterized and re-uploaded each frame. Two tiers of reuse plus a native edge-fade cut that
traffic.

## Tier 1 - content node reuse

*New in v2*

`gskbroadwayrenderer.c` already reused nodes by `GskRenderNode` *pointer* identity across frames
(emitting `BROADWAY_NODE_REUSE`). But widgets like `GtkTreeView` rebuild **all** their cell nodes
every snapshot, so pointer identity always missed and every cell re-rasterized. The fork adds a
second key: a content hash (FNV-1a over glyphs/colour/font/bounds/offset) compared against last
frame's, so a re-snapshotted-but-identical node reuses last frame's id without re-rasterizing.

## Tier 2 - cross-time texture dedup

*New in v2*

When tier 1 misses (the node genuinely moved, e.g. scroll), the texture is re-rasterized into a new
`GdkTexture`. Textures were deduped only by `GdkTexture` *object* identity, so the fresh object got a
fresh PNG upload even when the pixels were identical.

`gdk_broadway_display_ensure_texture` now keeps a content cache on `GdkBroadwayDisplay`, keyed on
`(width, height, format, FNV-1a-64 of the raw downloaded pixels)`, reusing the already-uploaded
broadway texture id instead of re-uploading. Notes:

- Pixels are hashed via `gdk_texture_download` (which normalizes to `GDK_MEMORY_DEFAULT`), not by
  encoding a PNG just to compute the key.
- Entries are **refcounted**: an id is released (and dropped from the browser) only once no live
  `GdkTexture` references it - LRU eviction skips live entries. A live texture's id is never pulled
  out from under it (an earlier non-refcounted cut crashed the browser this way).
- LRU-capped (512 entries); textures larger than 512x512 px skip the cache (download + hash cost is
  not worth it and they rarely repeat).
- The fast path (an object already uploaded) returns its id with no re-hash, so steady-state cost is
  unchanged.

Net effect: first paint of new content still uploads (red); re-scrolling or re-hovering seen content
reuses (magenta/green) and the Traffic counter climbs far more slowly.

## Native notebook edge-fade

*New in v2*

The [notebook](../features/notebook.md) tab-scroll edge-fade used `gtk_snapshot_push_mask`, which Broadway
rasterizes (no `GskMaskNode` renderer) into a texture re-uploaded every scroll frame. It is now drawn
as themed CSS `undershoot` nodes - native `GSK_LINEAR_GRADIENT_NODE`s fading into the header
`$dark_fill` - so it costs zero texture traffic and stays theme-correct.

## What still rasterizes

Genuine per-frame animation, where each frame is honestly different pixels, still uploads: the
scroll **overshoot** shadow and the `GtkSwitch` knob mid-toggle both use `radial-gradient`, which
falls back to a cairo texture. The dedup catches their settled states. Native radial-gradient
support in the Broadway renderer would remove the rest, but is out of scope here.
