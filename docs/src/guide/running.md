# Running broadwayd

Broadway serves a GTK app to the browser in two pieces. The `gtk4-broadwayd` daemon owns the display and the WebSocket. The app itself runs with the Broadway GDK backend, so it connects to that daemon instead of an X11/Wayland display. [`gtk4-brotway-run`](#one-command) automates both; the manual steps below show what it does (and how to run the daemon from a [source build](../build/from-source.md)).

## Start the daemon

```sh
gtk4-broadwayd :5
```

> Packaged installs keep the daemon in the fork prefix (`/usr/lib/gtk4-brotway/gtk4-broadwayd`), not on `PATH` - use the [launcher](#one-command), which invokes it by full path.

`:5` is the Broadway display number. The daemon listens for the browser on HTTP port `8080 + N` (so `:5` maps to `http://localhost:8085`) and for app clients on the matching local Broadway socket. Open the served page in a browser:

```
http://localhost:8085
```

## Start the app against it

Point the app at the same display via the GDK backend:

```sh
GDK_BACKEND=broadway BROADWAY_DISPLAY=:5 your-gtk4-app
```

The app renders into the daemon, which streams render nodes to every connected browser tab. The browser sends input (pointer, touch, keyboard) back over the same socket.

## One command

`gtk4-brotway-run` does both steps in one shot: it starts `broadwayd`, runs the app against it, and tears the daemon down on exit. Both packages install the fork into the same private prefix (`/usr/lib/gtk4-brotway`); the launcher points `LD_LIBRARY_PATH` at it, so the app uses the fork without touching the system GTK.

```sh
gtk4-brotway-run gtk4-widget-factory
# -> http://localhost:8085  (triple-Shift = debug menu)
```

Useful flags:

- `--auto` - pick the first free display/port pair, so several apps can run at once
- `--open` - open the WebUI in a browser (`$BROWSER`, else `xdg-open`)
- `--address A` - broadwayd bind address, e.g. `0.0.0.0` to serve a mapped container port (env `BROTWAY_ADDRESS`)
- `--display :N` / `--port P` - pin them explicitly (env `BROTWAY_DISPLAY` / `BROTWAY_PORT`)

## The browser client

The page the daemon serves is `client.html` + `broadway.js`, both embedded in the daemon binary. All the fork's browser-side logic lives in `broadway.js`: touch delivery, the clipboard bridge, pinch-zoom, reconnect, the [debug menu](../internals/debug-menu.md), and the paint-flash overlay. Both assets are served `Cache-Control: no-store`, so a plain reload always picks up a rebuilt and restarted `gtk4-broadwayd`. No hard-refresh needed.

> Which changes need only a broadwayd restart and which need the library rebuilt and the app restarted: [broadwayd vs libgtk](../build/from-source.md#iterating-broadwayd-vs-libgtk).

## Secure context note

The clipboard bridge uses `navigator.clipboard`, which browsers only expose in a secure context: `https://` or `http://localhost`. Over plain `http://` to a remote host, copy may silently fail outside a user gesture. The reference deployment runs behind TLS (Traefik in Docker Swarm).
