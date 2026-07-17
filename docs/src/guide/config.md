# Configuration reference

Every knob the fork reads. The fork's own variables are all `BROTWAY_*`; the rest is `GDK_BACKEND` and the client-side `localStorage` / URL state.

## Daemon

`gtk4-broadwayd :N` takes a display number `N`:

| What | Value |
|------|-------|
| Browser HTTP port | `8080 + N` (so `:5` -> `http://localhost:8085`) |
| App socket | the local Broadway socket for display `:N` |

The served page (`client.html` + `broadway.js`) is embedded in the daemon binary and sent `Cache-Control: no-store`, so a plain browser reload always picks up a rebuilt, restarted daemon.

## Environment variables

Booleans accept `1`/`0`, `true`/`false`, `yes`/`no`, `on`/`off` (case-insensitive).

### Read by the app

The app process (not the daemon) reads these to target the daemon instead of X11/Wayland:

| Variable | Value | Purpose |
|----------|-------|---------|
| `GDK_BACKEND` | `broadway` | use the Broadway GDK backend |
| `BROTWAY_DISPLAY` | `:N` | connect to the daemon for display `N` |
| `BROTWAY_PNG` | `fast` / `compact` | PNG encoding preset ([PNG encoding](#png-encoding)) |
| `BROTWAY_NO_ANIMATIONS` | boolean | `1` force-disables GTK **and libadwaita** animations (`gtk-enable-animations=false`). Broadway pushes every animated frame over the socket, so turning them off cuts frame churn and bandwidth. |
| `BROTWAY_MAXIMIZE` | boolean | `1` maximizes the app's first toplevel when it doesn't set a window state itself, so the main window fills the browser viewport. Dialogs still float. |
| `BROTWAY_COLOR_SCHEME` | `auto` / `light` / `dark` | force the color scheme for this process; `light`/`dark` pin it and ignore the browser, `auto` (default) follows the browser's `prefers-color-scheme` ([Display & rendering](../features/display.md)) |

By default the app follows the browser's light/dark preference on its own; `BROTWAY_COLOR_SCHEME` overrides it per process ([Display & rendering](../features/display.md)).

### Read by the daemon

| Variable | Value | Purpose |
|----------|-------|---------|
| `BROTWAY_DEBUGMENU` | command | the debug-menu binary the daemon spawns (default `gtk4-brotway-debugmenu`) ([Debug menu](../internals/debug-menu.md)) |
| `BROTWAY_DEBUGMENU_FD` | set by the daemon | one end of a control socketpair carrying the `stats ...` line and command replies, passed to the spawned child - not user configuration |

The debug menu is summoned by **Triple-Shift** in the browser, not by configuration.

### Read by the launcher

`gtk4-brotway-run` takes these as defaults for its flags ([Running](running.md)):

| Variable | Flag | Purpose |
|----------|------|---------|
| `BROTWAY_DISPLAY` | `--display :N` | display number (default `:5`) |
| `BROTWAY_PORT` | `--port P` | WebUI port (default `8080 + N`) |
| `BROTWAY_ADDRESS` | `--address A` | `broadwayd` bind address, e.g. `0.0.0.0` to listen beyond localhost - read [Security model](security.md) first |

> The fork's variables were previously named `BROADWAY_*`. Any `BROADWAY_*` name is still promoted to its `BROTWAY_*` equivalent at startup with a one-time deprecation warning; switch to `BROTWAY_*`.

## PNG encoding

Every changed texture is re-encoded to PNG per frame, so the preset trades encode CPU (frame latency) against frame size:

| `BROTWAY_PNG` | libpng settings | Use |
|----------------|-----------------|-----|
| `fast` (default) | adaptive filter, level 3 | localhost / LAN - bandwidth is free, CPU/latency is the cost |
| `compact` | adaptive filter, level 7 | remote / metered links - frame size dominates |

The [debug menu](../internals/debug-menu.md) (Triple-Shift) has a **PNG encoding** selector that switches the preset live, overriding the env seed.

## localStorage keys

Set per origin by `broadway.js`:

| Key | Purpose |
|-----|---------|
| `broadwayZoom` | the committed [pinch-zoom](../features/input.md#pinch-to-zoom) factor, restored before the first frame on reload |

## URL parameters

| Parameter | Purpose |
|-----------|---------|
| `?cid=` | client id minted on a fresh page load; drives [single-display arbitration](../internals/connection.md#single-display-arbitration-newest-fresh-open-wins) so the newest load wins ownership |

## Build-time flags

Not runtime config, but the build knobs covered in [Building from source](../build/from-source.md): `-Dbroadway-backend=true` with every other backend off, and `-Dbuild-demos=false`.
