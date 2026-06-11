# Connection management

<!-- SCREENCAST (pending) - uncomment after recording. See devnotes/2026-06-10-docs-screencasts.md
  Record: make record SCENARIO=tools/local/scenarios/connection.json OUT=docs/src/images/connection.mp4

<video src="../images/connection.mp4" poster="../images/connection.png"
  autoplay loop muted playsinline style="max-width:100%;border-radius:6px">
  <img src="../images/connection.png" alt="In-place reconnect: dimmed last frame under a spinner, then resync"
    style="max-width:100%;border-radius:6px">
</video>
-->

*New in v2*

Any WebSocket interruption drops the session in stock Broadway: screen-off, a Wi-Fi toggle, a 4G<->Wi-Fi handover. The only recovery is a full page reload, which loses UI state. The fork reconnects the browser client in place.

## What works

- **In-place reconnect** after screen-off or a network change - the session resumes without a reload.
- The last frame stays dimmed under a spinner until the daemon's resync repaints.
- A **heartbeat** catches half-open sockets that never fire the browser's `onclose`.
- **Single-display arbitration** - two browsers don't fight over the single Broadway display; the newest fresh page load wins.
- Timers freeze on screen-off, so they burn no CPU while the device sleeps.
- **Rendering pauses on a hidden tab** - backgrounding or switching away from the tab streams zero frames; the socket stays open, so switching back repaints just the delta, with no reconnect or resync.

> The session token, the PING/PONG heartbeat, the ownership arbitration, and the hidden-tab rendering freeze are detailed in [Connection implementation](../internals/connection.md).
