# Running broadwayd

Broadway serves a GTK app to the browser in two pieces. The `gtk4-broadwayd` daemon owns
the display and the WebSocket. The app itself runs with the Broadway GDK backend, so it
connects to that daemon instead of an X11/Wayland display.

## Start the daemon

```sh
gtk4-broadwayd :5
```

`:5` is the Broadway display number. The daemon listens for the browser on HTTP port
`8080 + N` (so `:5` maps to `http://localhost:8085`) and for app clients on the matching local
Broadway socket. Open the served page in a browser:

```
http://localhost:8085
```

## Start the app against it

Point the app at the same display via the GDK backend:

```sh
GDK_BACKEND=broadway BROADWAY_DISPLAY=:5 your-gtk4-app
```

The app renders into the daemon, which streams render nodes to every connected browser tab. The
browser sends input (pointer, touch, keyboard) back over the same socket.

## The browser client

The page the daemon serves is `client.html` + `broadway.js`, both embedded in the daemon binary
(generated into `broadwayjs.h` / `clienthtml.h` at build time). All the fork's browser-side logic
lives in `broadway.js`: touch delivery, the clipboard bridge, pinch-zoom, reconnect, the
[debug menu](../internals/debug-menu.md), and the paint-flash overlay. Both assets are served
`Cache-Control: no-store`, so a plain reload always picks up a rebuilt and restarted
`gtk4-broadwayd`. No hard-refresh needed.

> A change confined to `broadway.js` / `client.html` only needs a **broadwayd restart** then a
> normal browser reload. A change in `libgtk` (a widget or GDK fix) needs the library rebuilt and
> the **app** restarted. Each feature chapter notes which kind of change it is.

## Secure context note

The clipboard bridge uses `navigator.clipboard`, which browsers only expose in a secure
context: `https://` or `http://localhost`. Over plain `http://` to a remote host, copy may
silently fail outside a user gesture. The reference deployment runs behind TLS (Traefik in Docker
Swarm) for exactly this reason.
