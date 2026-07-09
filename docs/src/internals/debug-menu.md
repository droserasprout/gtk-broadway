# Debug menu

*New in v2*

A **Triple-Shift** (press Shift three times quickly) summons a server-side debug overlay for the Broadway session: live stats, a paint-flash profiler, and session actions. It's a native GTK4 window composited into the same display the main app uses.

<img src="../images/debug-menu.png" alt="Broadway debug menu" width="230"
  style="float:right;width:230px;margin:0 0 1rem 1.5rem">

> The menu binary (`gtk4-brotway-debugmenu`) ships in the `.deb` since v2.1; `broadwayd` spawns it by name via `PATH`.

## Performance section

Session id, Traffic, Framerate, Latency, and Textures (count + summed PNG bytes, with per-second upload/release rates). See [Performance](performance.md#performance-section-metrics) for what each means and how to read them under load.

## Smoothness section

*New in v3*

Frame-pacing under load, since average FPS hides stutter: **Frame** (p95 / max gap over a rolling window) and **Write** (time blocked in the socket `writev`, plus bytes-per-frame). See [Performance](performance.md#smoothness-section-metrics).

## Screen section

*New in v3*

Pins the browser's logical screen **Width × Height × Scale** (integer scale only), overriding the live window size and `devicePixelRatio`; **Reset** unpins. The daemon sends `BROADWAY_OP_DEBUG_SET_SCREEN` and `broadway.js` drives its normal resize path with the pinned values, so the app re-lays-out at the forced size. Useful for reproducing a device's geometry/HiDPI from a desktop browser.

## Actions

- **Reconnect** closes the browser's socket so its auto-reconnect resumes in place, token unchanged.
- **Drop session** rolls the session token first, so the reconnecting client hard-resets.
- **Open test URL** exercises the [open-uri](../features/input.md#opening-links) path.
- **Test gallery** opens the [widget gallery](#widget-gallery).
- **Paint flashing** toggles the profiler.
- **Debug logging** is a placeholder, no-op for now.
- **PNG encoding** switches the per-frame PNG preset (Fast/Compact) live; see [PNG encoding](../guide/config.md#png-encoding). The daemon relays the choice to the app as `BROADWAY_EVENT_SET_PNG`, since the app encodes, not the daemon.

## Widget gallery

*New in v3*

**Test gallery** opens a second top-level window with one clean, empty widget per fork feature, giving any Broadway session - including a real Android device - a stable, data-free surface to exercise and screencast.

| Section | Widgets | Exercises |
|---|---|---|
| Text & clipboard | two `GtkEntry`, selectable `GtkLabel`, `GtkTextView` | [clipboard](../features/input.md#clipboard), [touch selection](../features/input.md), OSK/IME |
| Notebook tabs | scrollable `GtkNotebook`, overflowing | [tab pixel-scroll](../features/input.md#notebook-tabs) |
| Menus & popups | `GtkDropDown`, popover, "Open dialog" | [right-row tap, menu-tap freeze](../features/input.md), autohide, centering |
| Scrolling list | `GtkListBox`, 25 rows in a scroller | gesture-survives-repaint, hover prelight, [node/texture reuse](performance.md) |
| Animation & links | `GtkSpinner`, `GtkSwitch`, `GtkLinkButton` | continuous-repaint traffic, [open-uri](../features/input.md#opening-links) |

The menu and its windows share one main loop that quits only when the **last** window closes, so you can close the stats overlay and keep just the gallery up for a clean recording. `broadwayd` pins every debug-menu toplevel above the app's windows, most-recently-mapped on top.

## Paint-flash profiler

With Paint flashing on, every changed node gets a translucent overlay coloured by upload cost (green = reused, magenta = cached texture, red = uploaded this frame); see [Reading the paint-flash overlay](performance.md#reading-the-paint-flash-overlay).

The daemon sends `BROADWAY_OP_DEBUG_FLASH` to toggle it; the client tags each node as it's applied. Overlays are pooled and drawn in one read-then-write pass per frame, so heavy scrolling doesn't thrash layout.

## How it is wired

Triple-Shift in `broadway.js` sends `BROADWAY_EVENT_MENU`. `broadwayd` intercepts it and spawns `gtk4-brotway-debugmenu` with `GDK_BACKEND`/`BROTWAY_DISPLAY` pointed at itself, so the window renders into the same display, pinned always-on-top. Since `EVENT_MENU` comes from the untrusted browser, a failed spawn backs off 10s, and the menu is matched by client id (only a client that connected after the summon can own it) so another app's surface can't get pinned on top.

The daemon hands the child one end of a control socketpair via `BROTWAY_DEBUGMENU_FD`. Over it the daemon pushes a `stats` line every ~500 ms and reads back commands (`reconnect`, `drop-session`, `open-uri`, `paint-flash 0|1`, `screen W H S`, `png-preset N`). The titlebar, Escape, and a second Triple-Shift all dismiss it.
