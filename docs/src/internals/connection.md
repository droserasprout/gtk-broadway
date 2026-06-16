# Connection implementation

The user-facing summary is in [Connection management](../features/connection.md). This page covers the four mechanisms, which use the `SESSION`, `PING`, `PONG`, and `SUSPEND`/`RESUME` wire messages; see [Wire protocol](protocol.md).

## Session token (`SESSION` op)

On every connect the daemon sends a random per-daemon token via `BROADWAY_OP_SESSION`. An unchanged token means the same live daemon, so the client resumes in place; a changed token (or an explicit `DISCONNECTED`) means the session is gone, so the client shows the disconnected icon until refresh.

On resume the client resets its display and input state (surfaces, pointer grab, button state, pending move, touch tracking), then the daemon resyncs surfaces, textures, the live grab, and each surface's remembered cursor (cursor re-sends are deduped app-side, so the resync must replay them).

## Heartbeat (`PING` / `PONG`)

A half-open socket (Wi-Fi off on a foreground tab, a cellular handover) never fires the browser's `onclose`, so a drop can go unnoticed. The browser sends `BROADWAY_EVENT_PING` and the app answers with `BROADWAY_OP_PONG`; a missing reply reveals the dead socket. Liveness is only enforced once a socket has delivered a complete message: a fresh socket gets 60s for its first, then the normal 7s timeout. The daemon flushes each resync texture as its own ws frame, so the liveness signal keeps flowing through a slow resync. Timers freeze on screen-off, burning no CPU while the device sleeps. The client also reports its last PING/PONG RTT, shown as Latency in the [debug menu](debug-menu.md#performance-section).

## Single-display arbitration ("newest fresh-open wins")

Broadway is a single-display server, so two browsers must not fight over it. A fresh page load mints a new client id ([`?cid=`](../guide/config.md#url-parameters)) and takes ownership. A reconnect resumes only if it's still the owner; otherwise it's rejected on its own connection (shown disconnected) and never races the live client. `offline` / `online` / `visibilitychange` drive the overlay and recovery attempt.

## Hidden-tab rendering freeze (`SUSPEND` / `RESUME`)

*New in v3*

A backgrounded tab still receives every frame the app paints, wasting bandwidth and CPU. On `visibilitychange` the browser sends `BROADWAY_EVENT_SUSPEND` when hidden and `BROADWAY_EVENT_RESUME` when visible; unlike `MENU`/`PING`, the daemon broadcasts them to all GTK clients rather than intercepting.

GTK calls `gdk_surface_freeze_updates()` on every surface, so the frame clock skips its paint phase. Crucially the renderer's diff baseline (`last_root`) stays put, so the first paint after `RESUME` is an incremental delta, not a full resync - the socket never dropped. The whole thing is idempotent and per-surface balanced, so events from a reconnecting client or surfaces created while hidden are handled safely.

The app keeps running while suspended - only rendering pauses; input into a hidden tab is processed and paints on resume. The browser re-asserts visibility on every fresh socket, since the app<->daemon link outlives a browser reconnect.
