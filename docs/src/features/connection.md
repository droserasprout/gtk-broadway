# Connection management

*New in v2*

Any WebSocket interruption drops the session in stock Broadway: screen-off, a Wi-Fi toggle, a
4G<->Wi-Fi handover. The only recovery is a full page reload, which loses UI state. The fork
reconnects the browser client in place.

## What works

- **In-place reconnect** after screen-off or a network change - the session resumes without a reload.
- The last frame stays dimmed under a spinner until the daemon's resync repaints.
- A **heartbeat** catches half-open sockets that never fire the browser's `onclose`.
- **Single-display arbitration** - two browsers don't fight over the single Broadway display; the
  newest fresh page load wins.
- Timers freeze on screen-off, so they burn no CPU while the device sleeps.

> The session token, the PING/PONG heartbeat, and the ownership arbitration are detailed in
> [Connection implementation](../internals/connection.md).
