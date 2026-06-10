# Connection implementation

The user-facing summary is in [Connection management](../features/connection.md). This page covers the four mechanisms. They use the `SESSION`, `PING`, `PONG`, and `SUSPEND`/`RESUME` wire messages; see [Wire protocol](protocol.md).

## Session token (`SESSION` op)

On every connect the daemon sends a random per-daemon token via `BROADWAY_OP_SESSION` (22).

If the token is unchanged it's the same live daemon, so the client resumes in place. A changed token means the daemon restarted; that, or an explicit `DISCONNECTED`, means the session is gone, so the client shows the disconnected icon until refresh.

## Heartbeat (`PING` / `PONG`)

A half-open socket (Wi-Fi off on a foreground tab, or a cellular handover) never fires the browser's `onclose`, so a drop can go unnoticed. To catch it, the browser sends `BROADWAY_EVENT_PING` (16) and the app answers with `BROADWAY_OP_PONG` (23); a missing reply reveals the dead socket. The timers freeze on screen-off, so they burn no CPU while the device sleeps. The client also times its own PING/PONG round-trip and reports the last RTT, which the [debug menu](debug-menu.md#performance-section) shows as Latency.

## Single-display arbitration ("newest fresh-open wins")

Broadway is a single-display server, so two browsers must not fight over it. A fresh page load mints a new client id ([`?cid=`](../guide/config.md#url-parameters)) and takes ownership. A reconnect resumes only if it is still the owner; otherwise it is rejected on its own connection (shown disconnected) and never races with the live client.

`offline` / `online` / `visibilitychange` events drive the overlay and the recovery attempt.

## Hidden-tab rendering freeze (`SUSPEND` / `RESUME`)

*New in v3*

A backgrounded tab still receives every frame the app paints, wasting bandwidth and CPU on pixels nobody sees. On `visibilitychange` the browser sends `BROADWAY_EVENT_SUSPEND` (18) when hidden and `BROADWAY_EVENT_RESUME` (19) when visible (both payload-less, like `MENU`). Unlike `MENU`/`PING`, the daemon does not intercept them - it broadcasts them to all GTK clients.

GTK calls `gdk_surface_freeze_updates()` on every surface, so the frame clock skips its paint phase: the GSK Broadway renderer never runs while hidden. Crucially the renderer's diff baseline (`last_root`) stays put, so the first paint after `RESUME` is an incremental delta, not a full resync - the socket never dropped. A per-surface flag keeps the extra freeze/thaw balanced across surfaces created or destroyed while suspended, and a surface born while hidden is frozen on construction. The whole thing is idempotent, so a duplicate event from a reconnecting client is a no-op.

The app keeps running while suspended - only rendering pauses. Input typed or clicked into a hidden tab is processed and paints on resume. The browser re-asserts its visibility on every fresh socket (`ws.onopen`), since the app<->daemon link outlives a browser reconnect and GTK keeps whatever freeze state the prior socket left it in.
