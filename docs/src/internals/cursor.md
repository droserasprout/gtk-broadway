# Dynamic cursor implementation

The user-facing summary is in [Dynamic cursor](../features/cursor.md). Stock Broadway's cursor path was a no-op: `gdk_broadway_device_set_surface_cursor` was empty and `gdkcursor-broadway.c` does nothing, so the cursor GTK chose never reached the browser. The fork wires that backend hook to forward the cursor *name* to the browser, where it becomes the surface element's CSS `cursor`.

## Why names map directly

GTK4 cursor names are CSS-aligned (`gdk_cursor_get_name` returns `text`, `pointer`, `ew-resize`, `nwse-resize`, ...), so it's near 1:1 - no glyph upload, no cursor theme. GtkWindow's own resize logic asks for `n/s/e/w/ne/nw/se/sw-resize`; entries ask for `text`; links ask for `pointer`. The backend resolves a `GdkCursor` to a name by walking the `gdk_cursor_get_fallback` chain to the first named cursor; a nameless (texture-only) cursor falls back to `default`.

## The op

`BROADWAY_OP_SET_CURSOR` (26) mirrors `SET_INPUT_REGION` (it carries a surface id) plus the `len + bytes` string framing of `OPEN_URI`. The path:

```
gdk_broadway_device_set_surface_cursor (gdkdevice-broadway.c)  // resolve name, dedup
  -> _gdk_broadway_server_surface_set_cursor (gdkbroadway-server.c)
  -> BROADWAY_REQUEST_SET_CURSOR (broadwayd.c, len clamped to the framed request)
  -> broadway_server_surface_set_cursor -> broadway_output_set_cursor  (op + serial + id + len + bytes)
  -> broadway.js  case BROADWAY_OP_SET_CURSOR
  -> surface.div.style.cursor = name   // children inherit
```

New enum values are appended at the end so existing wire numbers don't shift; see [Wire protocol](protocol.md).

## Dedup is mandatory

`set_surface_cursor` fires on **every** motion event, not only on change: `gtk_window_capture_motion` -> `gtk_window_maybe_update_cursor` -> `gdk_surface_set_device_cursor` -> `update_cursor` -> the backend hook, with no change-gate in GDK. So the backend caches the last resolved name on `GdkBroadwaySurface.cursor_name` and only hits the wire when it actually changes. Without that, every mouse move would emit a wire op.

Only the logical pointer (`GDK_SOURCE_MOUSE`) sends a cursor; touch is ignored.

## Browser side

`broadway.js` validates the received name against an allowlist of CSS cursor keywords (`CSS_CURSOR_NAMES`); an unknown name drops to `default`, so the browser never silently keeps a stale cursor on a keyword it rejects. The name is set on the surface's container div, which the rendered child nodes inherit.

## Deploy

Spans libgtk (`gdkdevice-broadway.c`, `gdksurface-broadway.c`) **and** broadwayd (protocol/server/output/`broadway.js`), so it needs a full image rebuild and an app restart, not a broadwayd-only refresh.
