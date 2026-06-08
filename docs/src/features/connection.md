# Connection management

*New in v2*

Any WebSocket interruption drops the session in stock Broadway: screen-off, a Wi-Fi toggle, a
4G<->Wi-Fi handover. The only recovery is a full page reload, which loses UI state. The fork
reconnects the browser client in place and keeps the last frame dimmed under a spinner until the
daemon's resync repaints.

*TODO: add screenshot*

## Session token (`SESSION` op)

On every connect the daemon sends a random per-daemon token via `BROADWAY_OP_SESSION` (22).

If the token is unchanged it's the same live daemon, so the client resumes in place. A changed
token means the daemon restarted; that, or an explicit `DISCONNECTED`, means the session is gone, so
the client shows the disconnected icon until refresh.

## Heartbeat (`PING` / `PONG`)

A half-open socket (Wi-Fi off on a foreground tab, or a cellular handover) never fires the
browser's `onclose`, so a drop can go unnoticed. To catch it, the browser sends
`BROADWAY_EVENT_PING` (16) and the app answers with `BROADWAY_OP_PONG` (23); a missing reply reveals
the dead socket. The timers freeze on screen-off, so they burn no CPU while the device sleeps.

## Single-display arbitration ("newest fresh-open wins")

Broadway is a single-display server, so two browsers must not fight over it. A fresh page load mints
a new client id (`?cid=`) and takes ownership. A reconnect resumes only if it is still the owner;
otherwise it is rejected on its own connection (shown disconnected) and never races with the live
client.

`offline` / `online` / `visibilitychange` events drive the overlay and the recovery attempt.
