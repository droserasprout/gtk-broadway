# Performance

Broadway rasterizes content and sends it to the browser as PNG uploads, so the dominant cost is uploading the same pixels twice. A widget like `GtkTreeView` rebuilds all its cell nodes every snapshot, so naive reuse misses and every cell re-uploads each frame.

A live, server-side profiler makes this visible: the paint-flash overlay colours every changed node by upload cost, and the [debug menu](debug-menu.md) shows per-window traffic and frame-pacing stats. The fork's reuse tiers, transport trims, and native edge-fade are tuned against these readings.

## Profiling and diagnosis

The profiler lives in the [debug menu](debug-menu.md); this section covers reading it. Press **Shift three times quickly** (triple-Shift) to summon the overlay, then toggle **Paint flashing**.

### Performance-section metrics

- **Traffic** - total bytes sent. Flat on a static screen, climbing slowly during reuse-friendly interaction.
- **Framerate** - displayed FPS. Hides stutter; use Smoothness for that.
- **Latency** - real browser-to-daemon RTT, timed off the [heartbeat](../features/connection.md) PING/PONG.
- **Textures** - count and summed PNG bytes the browser holds (the warm working set of the [content texture cache](#tier-2-lru-texture-content-cache)), with two rates:
  - **up/s** - uploads per second, i.e. the content-cache *miss* rate (hits never cross the wire). Sustained `up/s` on a static screen means thrashing.
  - **release/s** - texture ids dropped per second as their last referencing `GdkTexture` dies.

### Smoothness-section metrics

- **Frame** - p95 / max gap between real display pushes over a rolling 64-frame window. Gaps over 250 ms count as idle and drop out. The overlay repaints at ~2/s (its own noise floor), so read Frame under load.
- **Write** - time blocked in the socket `writev` (~0 on localhost; spikes mean backpressure) plus bytes-per-frame.

### Reading the paint-flash overlay

Each changed node gets a translucent overlay coloured by cost:

- **green** - node and texture both reused; no work.
- **magenta** - node re-sent but its texture was cached; cheap.
- **red** - texture uploaded this frame; real bandwidth.

### Good vs bad readings

- **Static screen** - flat Traffic, `up/s` ~0, no flashes. Anything else means the cache is missing on idle content.
- **Healthy scroll** - a brief `up/s` spike (first paint of newly exposed content, flashing red), then it settles as scroll-back hits the cache (green/magenta).
- **Cache thrashing** - sustained `up/s` or red flashes on a screen that isn't changing.
- **Stutter** - sustained Frame gaps under load even when Traffic is modest.

## Rendering reuse

*New in v2*

### Tier 1: content-hash node reuse

`gskbroadwayrenderer.c` reused nodes by `GskRenderNode` *pointer* identity (`BROADWAY_NODE_REUSE`), but rebuilt cell nodes always miss that key. The fork adds a content hash (FNV-1a over glyphs, colour, font, bounds, offset) compared against last frame's, so a re-snapshotted but identical node reuses last frame's id without re-rasterizing. Hashes are tagged with the node type so equal bytes from different node kinds can't alias. The pointer path also re-encodes when a node's enclosing-clip offset changed, since the baked parent-local geometry would otherwise land displaced after a relayout.

### Tier 2: LRU texture content cache

When tier 1 misses because a node genuinely moved (a scroll), the texture re-rasterizes into a new `GdkTexture` and dedup by object identity misses too. `gdk_broadway_display_ensure_texture` adds a content cache keyed on `(width, height, native format, color state, two 64-bit hashes of the downloaded pixels)`, reusing the uploaded broadway id. Pixels are hashed via `gdk_texture_download`, not by encoding a PNG; the key keeps the native format and color state since the wire PNG encodes those. A second independent hash keeps a single 64-bit collision from aliasing unrelated images. Entries are refcounted - an id is dropped only when no live `GdkTexture` references it, and eviction skips live entries. The cache is LRU-capped at 4096; textures over 512x512 px skip it (download+hash not worth it); the fast path (an object already uploaded) returns its id with no re-hash. The 4096 cap covers a dense treeview's ~1-2k-texture working set.

## Transport and CPU

The reuse tiers cut *texture* traffic. A second pass trims the rest: redundant wire ops, input latency, syscalls, and unbounded buffers.

### PNG encode: cost-vs-size preset

*New in v3*

Every cache miss re-encodes a texture to PNG on the frame path, so libpng's settings are a real CPU/latency-vs-size knob. `gdk_save_png_full` exposes zlib level, filter, and strategy, driven by a preset: **fast** (adaptive filter, level 3) for localhost/LAN, **compact** (level 7) for remote/metered links. Only the level differs; both keep adaptive filtering, which (vs a single Sub filter) keeps anti-aliased text and gradients small. Seeded from `BROTWAY_PNG`, switched live from the debug menu. See [PNG encoding](../guide/config.md#png-encoding).

### Wire: drop empty frames

*New in v2*

When a surface's frame diff produces no ops, `broadway-output.c` drops the whole `SET_NODES` instead of sending an 11-byte no-op and its forced flush; the serial is rolled back so the counter stays dense. (Repeated show-keyboard / input-region ops are *not* deduped this way - broadwayd never resets that state on disconnect, so the re-sends double as a reconnect resync.)

### Input latency: pointer-move coalescing

*New in v2*

A high-Hz mouse fires many `mousemove` per frame, but GTK only needs the latest. `broadway.js` buffers moves and sends one per animation frame, so a motion flood can't fill the websocket and delay a following click or key ([head-of-line blocking](https://en.wikipedia.org/wiki/Head-of-line_blocking)). Any discrete event flushes the pending move first, preserving order.

### CPU: fewer syscalls and allocations

*New in v2*

The WebSocket header and payload go out in one `writev` instead of two writes; `broadway.js` `sendInput` packs fields straight into the buffer instead of per-event closures; and the texture-id remap moved out of the node-data copy loop so the common case skips a per-word comparison.

### Memory bounding

*New in v2*

Two caps. After an oversized texture flush, the `broadway-output.c` output buffer is freed and restarted small instead of holding its peak capacity for the connection's life. And the per-source-texture recolor cache in `gskbroadwayrenderer.c` is bounded to 16 entries LRU, so recoloring one texture many ways (symbolic icons across states/themes) can't grow without limit.

## Limitations

**Per-frame animation still rasterizes.** Where each frame is genuinely different pixels, uploads are unavoidable. The scroll overshoot shadow and the `GtkSwitch` knob mid-toggle use `radial-gradient`, which falls back to a cairo texture; dedup catches only their settled states.

**Notebook edge-fade (solved natively).** The [notebook](../features/input.md#notebook-tabs) tab-scroll edge-fade once used `gtk_snapshot_push_mask`, which Broadway rasterized into a texture re-uploaded every scroll frame. It's now drawn as themed CSS `undershoot` nodes (native `GSK_LINEAR_GRADIENT_NODE`s), so zero texture traffic and theme-correct.
