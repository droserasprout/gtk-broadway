# Wire protocol

All wire constants live in `gdk/broadway/broadway-protocol.h`, shared verbatim by the daemon (C),
the GDK backend (C), and `broadway.js` (the values are mirrored in the JS header comment). Because
the daemon and the browser client are built from one tree, the fork **appends** every new enum
value at the end so existing wire numbers never shift.

This chapter covers the fork's additions; stock entries appear for context.

## Display ops - daemon to browser (`BROADWAY_OP_*`)

Stock ops are `0`-`16` (`GRAB_POINTER` ... `ROUNDTRIP`). The fork appends:

| Op | # | Direction | Purpose |
|----|---|-----------|---------|
| `BROADWAY_OP_SET_CLIPBOARD`     | 17 | daemon -> browser | push app-copied text to the browser clipboard ([Clipboard](../features/clipboard.md)) |
| `BROADWAY_OP_REQUEST_CLIPBOARD` | 18 | daemon -> browser | ask the browser for its clipboard (reply comes back as the `CLIPBOARD_CONTENTS` event) |
| `BROADWAY_OP_SET_INPUT_REGION`  | 19 | daemon -> browser | mark a surface's input region empty (click-through) ([Input region](input-region.md)) |
| `BROADWAY_OP_REASSERT_POINTER`  | 20 | daemon -> browser | tell the browser to re-send pointer-focus crossings ([Input region](input-region.md)) |
| `BROADWAY_OP_OPEN_URI`          | 21 | daemon -> browser | open a URI in a new browser tab ([Opening links](../features/open-uri.md)) |
| `BROADWAY_OP_SESSION`           | 22 | daemon -> browser | per-daemon session token for reconnect ([Connection management](../features/connection.md)) |
| `BROADWAY_OP_PONG`              | 23 | daemon -> browser | heartbeat reply ([Connection management](../features/connection.md)) |
| `BROADWAY_OP_DEBUG_FLASH`       | 24 | daemon -> browser | toggle the paint-flash profiler overlay ([Debug menu](debug-menu.md)) |

## Input events - browser to app (`BROADWAY_EVENT_*`)

Stock events are `0`-`14` (`ENTER` ... `ROUNDTRIP_NOTIFY`); `TOUCH` (5) already existed but was
[never sourced as a touchscreen](../features/touch.md). The fork appends:

| Event | # | Purpose |
|-------|---|---------|
| `BROADWAY_EVENT_CLIPBOARD_CONTENTS` | 15 | browser's reply to `REQUEST_CLIPBOARD`, routed to the one client that asked ([Clipboard](../features/clipboard.md)) |
| `BROADWAY_EVENT_PING`               | 16 | heartbeat from the browser; the app answers with `PONG` ([Connection management](../features/connection.md)) |
| `BROADWAY_EVENT_MENU`               | 17 | Triple-Shift; the daemon intercepts it to spawn the debug menu, so it never reaches the app ([Debug menu](debug-menu.md)) |

## Requests - app to daemon (`BROADWAY_REQUEST_*`)

The fork appends `BROADWAY_REQUEST_SET_CLIPBOARD`, `BROADWAY_REQUEST_REQUEST_CLIPBOARD`,
`BROADWAY_REQUEST_SET_INPUT_REGION`, and `BROADWAY_REQUEST_OPEN_URI`, with matching structs
(`BroadwayRequestSetClipboard`, `BroadwayRequestOpenUri`, `BroadwayRequestSetInputRegion`).

Variable-length requests use the `len + bytes` framing (`guint32 len; char text[1];`), the same
shape as `SET_NODES`. The daemon clamps `len` to the framed request size before reading.

## Changed stock struct: `is_popup` on `NEW_SURFACE`

`BroadwayRequestNewSurface` gains a field:

```c
typedef struct {
  BroadwayRequestBase base;
  gint32 x, y;
  guint32 width, height;
  guint32 is_popup; /* TRUE for menus/popovers, FALSE for toplevels (incl. dialogs) */
} BroadwayRequestNewSurface;
```

`is_popup` is set on the GDK client from `surface->parent != NULL` and stored on the daemon's
`BroadwaySurface`. It replaced an earlier `transient_for`-based heuristic that misclassified
transient **dialogs** as popups. It now gates two touch behaviours: which surfaces get raised and
focused on a tap, and which surfaces count for pointer-recovery (`any_popup_visible`). See
[Touch interface](../features/touch.md).

## Size limits

```c
#define BROADWAY_CLIPBOARD_MAX_SIZE (16 * 1024 * 1024)
```

Bounds the allocation a (possibly malicious or buggy) browser-supplied length can drive on the
daemon, and the resulting reply size on the client. The clipboard and open-URI paths both clamp
against the framed message size first, then this ceiling.
