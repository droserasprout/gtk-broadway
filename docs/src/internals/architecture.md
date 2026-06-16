# Architecture

Broadway consists of three parts.

```text
   GTK app (libgtk, GDK broadway backend)
          |
          |  local socket  (BroadwayRequest / BroadwayReply)
          |
   gtk4-broadwayd  (daemon: owns the display, multiplexes clients)
          |
          |  WebSocket  (display ops out, input events in)
          |
   browser  (client.html + broadway.js)
```

The app (`libgtk`) renders its UI into GSK render nodes. The Broadway GDK backend (`gdk/broadway/`) talks to the daemon over a local socket via `BroadwayRequest` / `BroadwayReply`; the Broadway GSK renderer (`gsk/broadway/`) turns render nodes into Broadway's node stream, falling back to cairo for node types Broadway can't express.

`gtk4-broadwayd` owns the logical display, multiplexing between the app on one side and one or more browsers on the other.

The browser runs `broadway.js`, served in `client.html` (both embedded in the daemon). It applies display ops to the DOM and sends input events back. Most of the fork's touch, clipboard, zoom, and reconnect logic lives here.

## Three message vocabularies

The three enums in [`broadway-protocol.h`](protocol.md) stay distinct:

| Direction | Type | Examples |
|---|---|---|
| app -> daemon | `BroadwayRequest` (`BROADWAY_REQUEST_*`) | `SET_NODES`, `SET_CLIPBOARD`, `OPEN_URI` |
| daemon -> browser | display op (`BROADWAY_OP_*`) | `SET_NODES`, `SET_CLIPBOARD`, `SESSION`, `PONG` |
| browser -> daemon -> app | input event (`BROADWAY_EVENT_*`) | `TOUCH`, `PING`, `CLIPBOARD_CONTENTS` |

A feature usually touches more than one: the clipboard adds an app request, a daemon-to-browser op, and a browser-to-app event. [Wire protocol](protocol.md) lists every op the fork added.

## Wire framing

- Daemon -> browser: little-endian `int32`.
- Browser -> daemon input: big-endian `int32`, `[cmd, lastSerial, ts, ...args]`.
- Variable-length payloads (clipboard text, URIs): `guint32 len` + `len` bytes.

For variable-length payloads the daemon clamps `len` to the framed message size before reading (the `SET_NODES` pattern), capped at `BROADWAY_CLIPBOARD_MAX_SIZE` (16 MiB).

Every fork change lives on one side of a split - client/daemon vs the library - which drives both iteration and shipping; see [broadwayd vs libgtk](../build/from-source.md#iterating-broadwayd-vs-libgtk).
