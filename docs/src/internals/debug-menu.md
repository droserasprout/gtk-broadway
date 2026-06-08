# Debug menu

*New in v2*

A **Triple-Shift** (press Shift three times quickly) summons a server-side debug overlay for the
Broadway session: live stats, a paint-flash traffic profiler, and session actions. This is a native
GTK4 window composited into the same display main app use.

*TODO: add screenshot*

## Performance section

This section shows Session id, Traffic (total bytes), Framerate, Latency, and Textures. **Latency**
is the real browser-to-daemon round-trip: the client times its
[heartbeat](../features/connection.md) PING/PONG and reports the last RTT in the next PING payload
(`server->last_latency_ms`). **Textures** is the live texture buffer the browser holds (count plus
summed PNG bytes across `server->textures`), the footprint kept warm by the
[content texture cache](performance.md).

## Actions

- **Reconnect** closes the browser's socket so its auto-reconnect resumes in place, token unchanged.
- **Drop session** rolls the session token first, so the reconnecting client sees a new session and
  hard-resets.
- **Open test URL** exercises the [open-uri](../features/open-uri.md) path.
- **Paint flashing** toggles the profiler below.
- **Debug logging** is a placeholder, no-op for now.

## Paint-flash profiler

With Paint flashing on, every changed node gets a translucent overlay so you can *see* Broadway's
upload traffic, coloured by cost:

- **green**: node and texture reused (no work)
- **magenta**: node re-sent but its texture was cached (cheap)
- **red**: texture uploaded this frame (real bandwidth)

The daemon sends `BROADWAY_OP_DEBUG_FLASH` (24) to toggle it; the client tags each node as it is
applied. Overlays are pooled and drawn in one read-then-write pass with a single recycle timer per
frame. An earlier per-node implementation (one `getBoundingClientRect` + `appendChild` + `setTimeout`
each) thrashed layout and froze the page under heavy scrolling.

## How it is wired

Triple-Shift in `broadway.js` sends `BROADWAY_EVENT_MENU` (17). `broadwayd` intercepts it and
`spawn`s `gtk4-broadway-debugmenu` with `GDK_BACKEND`/`BROADWAY_DISPLAY` pointed at itself, so the
window renders into the same display and is pinned always-on-top via server-side stacking.

The daemon hands the child one end of a control socketpair through the `BROADWAY_DEBUGMENU_FD` env
var. Over it the daemon pushes a `stats <session> <bytes> <fps> <latency> <flash> <tex_count> <tex_bytes>`
line every ~500 ms and reads back newline-terminated commands.
