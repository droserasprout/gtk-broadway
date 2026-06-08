# Architecture

Broadway has three parts. The fork changes all three without altering the shape.

```
   GTK app (libgtk, GDK broadway backend)
        |  local socket  (BroadwayRequest / BroadwayReply)
   gtk4-broadwayd  (daemon: owns the display, multiplexes clients)
        |  WebSocket  (display ops out, input events in)
   browser  (client.html + broadway.js)
```

The app (`libgtk`) renders its UI into GSK render nodes. The Broadway GDK backend
(`gdk/broadway/`) serializes them and talks to the daemon over a local socket using
`BroadwayRequest` / `BroadwayReply`. The Broadway GSK renderer (`gsk/broadway/`) turns render
nodes into Broadway's node stream, falling back to cairo rasterization for node types Broadway
can't express natively.

`gtk4-broadwayd` owns the logical display. It accepts the app on one side and one or more
browsers on the other, and multiplexes between them.

The browser runs `broadway.js`, served in `client.html` (both are embedded in the daemon). It
applies incoming display ops to the DOM and sends input events back. Most of the fork's touch,
clipboard, zoom, and reconnect logic lives here.

## Three message vocabularies

The three enums in [`broadway-protocol.h`](protocol.md) stay distinct:

| Direction | Type | Examples |
|---|---|---|
| app -> daemon | `BroadwayRequest` (`BROADWAY_REQUEST_*`) | `SET_NODES`, `SET_CLIPBOARD`, `OPEN_URI` |
| daemon -> browser | display op (`BROADWAY_OP_*`) | `SET_NODES`, `SET_CLIPBOARD`, `SESSION`, `PONG` |
| browser -> daemon -> app | input event (`BROADWAY_EVENT_*`) | `TOUCH`, `PING`, `CLIPBOARD_CONTENTS` |

One feature usually touches more than one of these. The clipboard, for instance, adds an app
request, a daemon-to-browser op, and a browser-to-app event. [Wire protocol](protocol.md) lists
every op the fork added.

## Wire framing

- Daemon -> browser: little-endian `int32`.
- Browser -> daemon input: big-endian `int32`, `[cmd, lastSerial, ts, ...args]`.
- Variable-length payloads (clipboard text, URIs): `guint32 len` + `len` bytes.

For those variable-length payloads the daemon clamps `len` to the framed message size before
reading (the `SET_NODES` pattern), capped at `BROADWAY_CLIPBOARD_MAX_SIZE` (16 MiB).

## broadwayd-only vs libgtk

This split drives deployment. Changes to `broadway.js`, `client.html`, or the daemon C
(`broadwayd.c`, `broadway-server.c`, `broadway-output.c`) are broadwayd-only: rebuild and
restart the daemon, then hard-reload the browser. Changes to the GDK backend, the GSK renderer,
or a GTK widget are libgtk: rebuild the library and restart the app.

The `.deb` ships both the patched `libgtk-4.so` and `gtk4-broadwayd`, so a release covers both.
