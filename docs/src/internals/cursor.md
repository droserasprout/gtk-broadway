# Dynamic cursor implementation

*New in v3*

The user-facing summary is in [Dynamic cursor](../features/input.md#dynamic-cursor). Stock Broadway's cursor path was a no-op: `gdk_broadway_device_set_surface_cursor` was empty and `gdkcursor-broadway.c` does nothing, so the cursor GTK chose never reached the browser. The fork wires that backend hook to forward the cursor *name* to the browser, where it becomes the surface element's CSS `cursor`.

## Why names map directly

GTK4 cursor names are CSS-aligned (`text`, `pointer`, `ew-resize`, ...), so it's near 1:1 - no glyph upload, no cursor theme. The backend resolves a `GdkCursor` to a name by walking the `gdk_cursor_get_fallback` chain to the first named cursor; a nameless (texture-only) cursor falls back to `default`.

## The op

`BROADWAY_OP_SET_CURSOR` (26) mirrors `SET_INPUT_REGION` (it carries a surface id) plus the `len + bytes` string framing of `OPEN_URI`. The path:

```text
gdk_broadway_device_set_surface_cursor (gdkdevice-broadway.c)  // resolve name, dedup
  -> _gdk_broadway_server_surface_set_cursor (gdkbroadway-server.c)
  -> BROADWAY_REQUEST_SET_CURSOR (broadwayd.c, len clamped to the framed request)
  -> broadway_server_surface_set_cursor -> broadway_output_set_cursor  (op + serial + id + len + bytes)
  -> broadway.js  case BROADWAY_OP_SET_CURSOR
  -> surface.div.style.cursor = name   // children inherit
```

New enum values are appended at the end so existing wire numbers don't shift; see [Wire protocol](protocol.md).

## Dedup is mandatory

`set_surface_cursor` fires on **every** motion event, not only on change (GDK has no change-gate). So the backend caches the last resolved name on `GdkBroadwaySurface.cursor_name` and only hits the wire when it actually changes; otherwise every mouse move would emit a wire op. Only the logical pointer (`GDK_SOURCE_MOUSE`) sends a cursor; touch is ignored.

## Browser side

`broadway.js` validates the received name against an allowlist of CSS cursor keywords; an unknown name drops to `default`. The name is set on the surface's container div, which child nodes inherit.

Spanning both libgtk and broadwayd, this needs a full image rebuild and an app restart, not a broadwayd-only refresh.
