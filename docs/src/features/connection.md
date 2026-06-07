# Connection management

*New in v2*

Stock Broadway drops the session on any WebSocket interruption - screen-off, Wi-Fi toggle,
4G<->Wi-Fi handover - and the only recovery is a full page reload, which loses UI state. The fork
reconnects the browser client in place, keeping the last frame dimmed under a spinner until the
daemon's resync repaints.

*TODO: add screenshot*

## Session token (`SESSION` op)

On every connect the daemon sends a random per-daemon token via `BROADWAY_OP_SESSION` (22).

- **Unchanged token** -> same live daemon: resume in place.
- **Changed token** (daemon restarted) or an explicit `DISCONNECTED` -> session gone: show the
  disconnected icon until refresh.

## Heartbeat (`PING` / `PONG`)

A half-open socket - Wi-Fi off on a foreground tab, or a cellular handover - never fires the
browser's `onclose`, so a drop can go unnoticed. The browser sends `BROADWAY_EVENT_PING` (16) and
the app answers with `BROADWAY_OP_PONG` (23); a missing reply reveals the dead socket. The timers
freeze on screen-off, so they use no CPU while the device sleeps.

## Single-display arbitration ("newest fresh-open wins")

Broadway is a single-display server, so two browsers must not fight over it.

- A fresh page load mints a new client id (`?cid=`) and takes ownership.
- A reconnect resumes only if it is still the owner; otherwise it is rejected on its own connection
  (shown disconnected), so it never races with the live client.

`offline` / `online` / `visibilitychange` events drive the overlay and the recovery attempt.
