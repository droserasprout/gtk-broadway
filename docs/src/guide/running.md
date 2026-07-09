# Running

A GTK4 app reaches the browser in two pieces: the `gtk4-broadwayd` daemon owns the display and the WebSocket, and the app runs on the Broadway GDK backend, connecting to that daemon instead of an X11/Wayland display. `gtk4-brotway-run` ties both together.

## The launcher

`gtk4-brotway-run` starts `broadwayd`, runs the app against it, and tears the daemon down on exit. It points `LD_LIBRARY_PATH` at the fork prefix, so the app uses the fork without touching the system GTK.

```sh
gtk4-brotway-run gtk4-widget-factory
# -> http://localhost:8085  (triple-Shift = debug menu)
```

It runs any GTK4 binary out of the box, even ones built against a full GTK: those reference `gdk_x11_*` / `gdk_wayland_*` symbols a broadway-only build omits, and the fork's `libgtk-4` carries no-op stubs of that whole ABI so they resolve at load - no preload, no rebuild.

Useful flags:

- `--auto` - pick the first free display/port pair, so several apps can run at once
- `--open` - open the WebUI in a browser (`$BROWSER`, else `xdg-open`)
- `--address A` - broadwayd bind address, e.g. `0.0.0.0` to serve a mapped container port (env `BROTWAY_ADDRESS`)
- `--display :N` / `--port P` - pin them explicitly (env `BROTWAY_DISPLAY` / `BROTWAY_PORT`)

## By hand

What the launcher does, in two steps. The daemon lives in the fork prefix (not on `PATH`), and the app must load the fork lib via `LD_LIBRARY_PATH`:

```sh
export LD_LIBRARY_PATH=/usr/lib/gtk4-brotway   # so both load the fork lib

# daemon: display :5 -> http://localhost:8085 (HTTP port is 8080 + N)
/usr/lib/gtk4-brotway/gtk4-broadwayd :5 &

# app: point it at that display
GDK_BACKEND=broadway BROTWAY_DISPLAY=:5 gtk4-demo   # changeme
```

Open `http://localhost:8085`. The app renders into the daemon, which streams render nodes to every connected browser tab; the browser sends input (pointer, touch, keyboard) back over the same socket.

## The browser client

The page is `client.html` + `broadway.js`, both embedded in the daemon binary. All the fork's browser-side logic lives in `broadway.js`: touch delivery, the clipboard bridge, pinch-zoom, reconnect, the [debug menu](../internals/debug-menu.md), and the paint-flash overlay. Both assets are served `Cache-Control: no-store`, so a plain reload always picks up a rebuilt, restarted `gtk4-broadwayd`.

> Which changes need only a broadwayd restart and which need the library rebuilt and the app restarted: [broadwayd vs libgtk](../build/from-source.md#iterating-broadwayd-vs-libgtk).

## Secure context note

The clipboard bridge uses `navigator.clipboard`, which browsers only expose in a secure context: `https://` or `http://localhost`. Over plain `http://` to a remote host, copy may silently fail outside a user gesture. The reference deployment runs behind TLS (Traefik in Docker Swarm).
