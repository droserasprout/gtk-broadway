# Connection management

Stock Broadway drops the session on any WebSocket interruption - screen-off, Wi-Fi toggle,
4G<->Wi-Fi handover - and the only recovery is a full page reload, which loses UI state. The fork
makes the browser client reconnect in place, keeping the last frame dimmed under a spinner until the
daemon's resync repaints.

**Files:** `gdk/broadway/` - `broadway-protocol.h` (`OP_SESSION=22`, `OP_PONG=23`,
`EVENT_PING=16`), `broadway-output.c`/`.h`, `broadway-server.c`, `broadway.js`.
**Deploy:** broadwayd-side. **Status:** merged (PR #10).

## Session token (`SESSION` op)

On every connect the daemon sends a random per-daemon token via `BROADWAY_OP_SESSION` (22).

- **Unchanged token** -> same live daemon: resume in place.
- **Changed token** (daemon restarted) or an explicit `DISCONNECTED` -> session gone: show the
  disconnected icon until refresh.

## Heartbeat (`PING` / `PONG`)

A half-open socket - Wi-Fi off on a foreground tab, or a cellular handover - never fires the
browser's `onclose`, so a drop can go unnoticed. The browser sends `BROADWAY_EVENT_PING` (16) and
the app answers with `BROADWAY_OP_PONG` (23); a missing reply reveals the dead socket. The timers
freeze on screen-off, so they hold no CPU while the device sleeps.

## Single-display arbitration ("newest fresh-open wins")

Broadway is a single-display server, so two browsers must not fight over it.

- A fresh page load mints a new client id (`?cid=`) and takes ownership.
- A reconnect resumes only if it is still the owner; otherwise it is rejected on its own connection
  (shown disconnected), so it never races with the live client.

`offline` / `online` / `visibilitychange` events drive the overlay and the recovery attempt.

## Critical correctness fix (from review)

A rejected (superseded) reconnect must be fully torn down. `start()` returns a boolean and, on
rejection, frees both the output and input handlers. Without that, the rejected connection lingers
as a zombie: it crashes the daemon on a WebSocket ping (NULL deref) and injects stale input.
`invalidateSession()` closes the WebSocket, and the resume safety-timer is tracked in a single
variable so it can't leak.
