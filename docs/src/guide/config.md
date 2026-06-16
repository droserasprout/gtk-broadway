# Configuration reference

Every knob the fork reads. Most are stock Broadway; the fork adds the debug-menu FD and the client-side `localStorage` / URL settings.

## Daemon

`gtk4-broadwayd :N` takes a display number `N`:

| What | Value |
|------|-------|
| Browser HTTP port | `8080 + N` (so `:5` -> `http://localhost:8085`) |
| App socket | the local Broadway socket for display `:N` |

The served page (`client.html` + `broadway.js`) is embedded in the daemon binary and sent `Cache-Control: no-store`, so a plain browser reload always picks up a rebuilt, restarted daemon.

## App environment variables

The app process (not the daemon) reads these to target the daemon instead of X11/Wayland:

| Variable | Value | Purpose |
|----------|-------|---------|
| `GDK_BACKEND` | `broadway` | use the Broadway GDK backend |
| `BROADWAY_DISPLAY` | `:N` | connect to the daemon for display `N` |

## PNG encoding

Every changed texture is re-encoded to PNG per frame, so the preset trades encode CPU (frame latency) against frame size:

| `BROADWAY_PNG` | libpng settings | Use |
|----------------|-----------------|-----|
| `fast` (default) | adaptive filter, level 3 | localhost / LAN - bandwidth is free, CPU/latency is the cost |
| `compact` | adaptive filter, level 7 | remote / metered links - frame size dominates |

The [debug menu](../internals/debug-menu.md) (Triple-Shift) has a **PNG encoding** selector that switches the preset live, overriding the env seed.

## Debug menu

| Variable | Set by | Purpose |
|----------|--------|---------|
| `BROADWAY_DEBUGMENU_FD` | the daemon, on the spawned `gtk4-brotway-debugmenu` child | one end of a control socketpair carrying the `stats ...` line and command replies ([Debug menu](../internals/debug-menu.md)) |

The debug menu is summoned by **Triple-Shift** in the browser, not by configuration.

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
