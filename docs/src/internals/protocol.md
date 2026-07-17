# Wire protocol

All wire constants live in `gdk/broadway/broadway-protocol.h`, shared by the daemon (C), the GDK backend (C), and `broadway.js`. The fork **appends** every new enum value at the end, so existing wire numbers never shift.

Each table below lists the full enum in wire order; the fork's additions are the entries appended past the stock range (the ones linking to a feature page).

## Display ops

What the daemon tells the browser to do - create surfaces, push render nodes, move the cursor.

| Op | # | Purpose |
|----|---|---------|
| `BROADWAY_OP_GRAB_POINTER`      | 0  | grab the pointer to a surface |
| `BROADWAY_OP_UNGRAB_POINTER`    | 1  | release the pointer grab |
| `BROADWAY_OP_NEW_SURFACE`       | 2  | create a surface |
| `BROADWAY_OP_SHOW_SURFACE`      | 3  | map (show) a surface |
| `BROADWAY_OP_HIDE_SURFACE`      | 4  | unmap (hide) a surface |
| `BROADWAY_OP_RAISE_SURFACE`     | 5  | raise a surface to the top of the stack |
| `BROADWAY_OP_LOWER_SURFACE`     | 6  | lower a surface in the stack |
| `BROADWAY_OP_DESTROY_SURFACE`   | 7  | destroy a surface |
| `BROADWAY_OP_MOVE_RESIZE`       | 8  | move and/or resize a surface |
| `BROADWAY_OP_SET_TRANSIENT_FOR` | 9  | set a surface's transient parent |
| `BROADWAY_OP_DISCONNECTED`      | 10 | tell the browser the app side is gone |
| `BROADWAY_OP_SURFACE_UPDATE`    | 11 | batched surface geometry/visibility update |
| `BROADWAY_OP_SET_SHOW_KEYBOARD` | 12 | show/hide the on-screen keyboard |
| `BROADWAY_OP_UPLOAD_TEXTURE`    | 13 | upload an image, keyed by texture id |
| `BROADWAY_OP_RELEASE_TEXTURE`   | 14 | drop a previously uploaded texture |
| `BROADWAY_OP_SET_NODES`         | 15 | send the render-node tree to paint |
| `BROADWAY_OP_ROUNDTRIP`         | 16 | roundtrip marker; browser echoes `ROUNDTRIP_NOTIFY` |
| `BROADWAY_OP_SET_CLIPBOARD`     | 17 | push app-copied text to the browser clipboard ([Clipboard](../features/input.md#clipboard)) |
| `BROADWAY_OP_REQUEST_CLIPBOARD` | 18 | ask the browser for its clipboard (reply comes back as the `CLIPBOARD_CONTENTS` event) |
| `BROADWAY_OP_SET_INPUT_REGION`  | 19 | mark a surface's input region empty (click-through) ([Input region](input-region.md)) |
| `BROADWAY_OP_REASSERT_POINTER`  | 20 | tell the browser to re-send pointer-focus crossings ([Input region](input-region.md)) |
| `BROADWAY_OP_OPEN_URI`          | 21 | open a URI in a new browser tab ([Opening links](../features/input.md#opening-links)) |
| `BROADWAY_OP_SESSION`           | 22 | per-daemon session token for reconnect ([Connection management](../features/connection.md)) |
| `BROADWAY_OP_PONG`              | 23 | heartbeat reply ([Connection management](../features/connection.md)) |
| `BROADWAY_OP_DEBUG_FLASH`       | 24 | toggle the paint-flash profiler overlay ([Debug menu](debug-menu.md)) |
| `BROADWAY_OP_DEBUG_SET_SCREEN`  | 25 | pin logical screen size + integer scale, `0,0,0` unpins ([Debug menu](debug-menu.md)) |
| `BROADWAY_OP_SET_CURSOR`        | 26 | set a surface's CSS cursor by name ([Dynamic cursor](cursor.md)) |
| `BROADWAY_OP_SET_TITLE`         | 27 | set a surface's window title (mirrored to the browser tab title) ([Tab identity](../features/display.md#tab-title-and-favicon)) |
| `BROADWAY_OP_SET_ICON`          | 28 | set a surface's icon, PNG bytes; len 0 unsets (mirrored to the favicon) ([Tab identity](../features/display.md#tab-title-and-favicon)) |
| `BROADWAY_OP_SET_MODAL`         | 29 | mark a surface modal (1) or not (0); client dims and input-blocks behind the topmost modal ([Window placement](../features/display.md#window-placement)) |

## Input events

What the browser sends back - input and surface notifications. `TOUCH` (5) already existed but was [never sourced as a touchscreen](../features/input.md) until the fork.

| Event | # | Purpose |
|-------|---|---------|
| `BROADWAY_EVENT_ENTER`              | 0  | pointer entered a surface |
| `BROADWAY_EVENT_LEAVE`              | 1  | pointer left a surface |
| `BROADWAY_EVENT_POINTER_MOVE`       | 2  | pointer motion |
| `BROADWAY_EVENT_BUTTON_PRESS`       | 3  | mouse button pressed |
| `BROADWAY_EVENT_BUTTON_RELEASE`     | 4  | mouse button released |
| `BROADWAY_EVENT_TOUCH`              | 5  | touch down/move/up ([Touch interface](../features/input.md)) |
| `BROADWAY_EVENT_SCROLL`             | 6  | scroll / wheel |
| `BROADWAY_EVENT_KEY_PRESS`          | 7  | key pressed |
| `BROADWAY_EVENT_KEY_RELEASE`        | 8  | key released |
| `BROADWAY_EVENT_GRAB_NOTIFY`        | 9  | a grab was installed |
| `BROADWAY_EVENT_UNGRAB_NOTIFY`      | 10 | a grab was released |
| `BROADWAY_EVENT_CONFIGURE_NOTIFY`   | 11 | a surface's geometry changed |
| `BROADWAY_EVENT_SCREEN_SIZE_CHANGED`| 12 | the browser viewport resized |
| `BROADWAY_EVENT_FOCUS`              | 13 | keyboard focus moved |
| `BROADWAY_EVENT_ROUNDTRIP_NOTIFY`   | 14 | reply to a `ROUNDTRIP` op |
| `BROADWAY_EVENT_CLIPBOARD_CONTENTS` | 15 | browser's reply to `REQUEST_CLIPBOARD`, routed to the one client that asked ([Clipboard](../features/input.md#clipboard)) |
| `BROADWAY_EVENT_PING`               | 16 | heartbeat from the browser; the app answers with `PONG` ([Connection management](../features/connection.md)) |
| `BROADWAY_EVENT_MENU`               | 17 | Triple-Shift; the daemon intercepts it to spawn the debug menu, so it never reaches the app ([Debug menu](debug-menu.md)) |
| `BROADWAY_EVENT_SUSPEND`            | 18 | tab hidden; forwarded to GTK to freeze rendering ([Connection management](../features/connection.md)) |
| `BROADWAY_EVENT_RESUME`             | 19 | tab visible again; thaws rendering ([Connection management](../features/connection.md)) |
| `BROADWAY_EVENT_SET_PNG`            | 20 | daemon -> app, not from the browser: switch the PNG encoding preset live from the debug menu ([PNG encoding](../guide/config.md#png-encoding)) |
| `BROADWAY_EVENT_COLOR_SCHEME`       | 21 | browser's `prefers-color-scheme` (0 no-preference, 1 dark, 2 light), on connect and on change; the daemon intercepts it to serve the appearance portal, so it never reaches the app ([Display & rendering](../features/display.md)) |

## Requests

What the GDK backend asks the daemon to do over the local socket. These carry no explicit wire numbers - the enum is ordinal and the fork appends to the end.

| Request | Purpose |
|---------|---------|
| `BROADWAY_REQUEST_NEW_SURFACE`       | create a surface |
| `BROADWAY_REQUEST_FLUSH`             | flush pending output |
| `BROADWAY_REQUEST_SYNC`              | sync barrier |
| `BROADWAY_REQUEST_QUERY_MOUSE`       | query current pointer position |
| `BROADWAY_REQUEST_DESTROY_SURFACE`   | destroy a surface |
| `BROADWAY_REQUEST_SHOW_SURFACE`      | map a surface |
| `BROADWAY_REQUEST_HIDE_SURFACE`      | unmap a surface |
| `BROADWAY_REQUEST_SET_TRANSIENT_FOR` | set a transient parent |
| `BROADWAY_REQUEST_MOVE_RESIZE`       | move/resize a surface |
| `BROADWAY_REQUEST_GRAB_POINTER`      | grab the pointer |
| `BROADWAY_REQUEST_UNGRAB_POINTER`    | release the pointer grab |
| `BROADWAY_REQUEST_FOCUS_SURFACE`     | focus a surface |
| `BROADWAY_REQUEST_SET_SHOW_KEYBOARD` | toggle the on-screen keyboard |
| `BROADWAY_REQUEST_UPLOAD_TEXTURE`    | upload a texture |
| `BROADWAY_REQUEST_RELEASE_TEXTURE`   | drop a texture |
| `BROADWAY_REQUEST_SET_NODES`         | send the render-node tree |
| `BROADWAY_REQUEST_ROUNDTRIP`         | request a roundtrip |
| `BROADWAY_REQUEST_SET_MODAL_HINT`    | mark a surface modal |
| `BROADWAY_REQUEST_SET_CLIPBOARD`     | push text to the browser clipboard ([Clipboard](../features/input.md#clipboard)) |
| `BROADWAY_REQUEST_REQUEST_CLIPBOARD` | ask the browser for its clipboard ([Clipboard](../features/input.md#clipboard)) |
| `BROADWAY_REQUEST_SET_INPUT_REGION`  | mark a surface click-through ([Input region](input-region.md)) |
| `BROADWAY_REQUEST_OPEN_URI`          | open a URI in a new tab ([Opening links](../features/input.md#opening-links)) |
| `BROADWAY_REQUEST_SET_CURSOR`        | set a surface's CSS cursor ([Dynamic cursor](cursor.md)) |
| `BROADWAY_REQUEST_SET_TITLE`         | set a surface's window title ([Tab identity](../features/display.md#tab-title-and-favicon)) |
| `BROADWAY_REQUEST_SET_ICON`          | set a surface's icon ([Tab identity](../features/display.md#tab-title-and-favicon)) |
| `BROADWAY_REQUEST_SET_KEEP_ABOVE`    | pin a surface always-on-top ([Always-on-top](input-region.md#always-on-top)) |

The fork's requests carry matching structs (`BroadwayRequestSetClipboard`, `BroadwayRequestOpenUri`, `BroadwayRequestSetInputRegion`, `BroadwayRequestSetCursor`, `BroadwayRequestSetKeepAbove`).

Variable-length requests use `len + bytes` framing (`guint32 len; char text[1];`), the same shape as `SET_NODES`. The sender pads the request size to 4 bytes so the daemon reads aligned structs in place; the daemon rejects a request framed smaller than its fixed header, then clamps `len` to the framed size before reading.

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

The GDK client sets `is_popup` from `surface->parent != NULL`, and the daemon stores it on its `BroadwaySurface`. Two touch behaviours key off it: which surfaces get raised and focused on a tap, and which count for pointer-recovery. See [Touch interface](../features/input.md).

## Size limits

```c
#define BROADWAY_CLIPBOARD_MAX_SIZE (16 * 1024 * 1024)
```

This caps the allocation a browser-supplied length can drive on the daemon, and the reply size on the client. The clipboard and open-URI paths clamp against the framed message size first, then this ceiling. Browser event frames are bounds-checked the same way before any field read.
