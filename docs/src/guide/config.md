# Configuration reference

Every knob the fork reads, in one place. Most are stock Broadway; the fork adds the debug-menu FD and the client-side `localStorage` / URL settings.

## Daemon

`gtk4-broadwayd :N` takes a display number `N`. From it:

| Thing | Value |
|-------|-------|
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

The app re-encodes every changed texture to PNG per frame, so the libpng
settings trade encode CPU (frame latency) against frame size. That one axis is a
preset, not raw knobs:

| `BROADWAY_PNG` | libpng settings | Use |
|----------------|-----------------|-----|
| `fast` (default) | filter `sub`, strategy `rle`, level 3 | localhost / LAN - bandwidth is free, CPU/latency is the cost |
| `compact` | adaptive filter, default strategy, level 7 | remote / metered links - frame size dominates |

The [debug menu](../internals/debug-menu.md) (Triple-Shift) has a **PNG encoding**
selector that switches the preset live, overriding the env seed - use it with the
Traffic / Pushes stats to compare on real content.

## Debug menu

| Variable | Set by | Purpose |
|----------|--------|---------|
| `BROADWAY_DEBUGMENU_FD` | the daemon, on the spawned `gtk4-brotway-debugmenu` child | one end of a control socketpair carrying the `stats ...` line and command replies ([Debug menu](../internals/debug-menu.md)) |

The debug menu is summoned by **Triple-Shift** in the browser, not by configuration.

## localStorage keys

Set per origin by `broadway.js`:

| Key | Purpose |
|-----|---------|
| `broadwayZoom` | the committed [pinch-zoom](../features/zoom.md) factor, restored before the first frame on reload |

## URL parameters

| Parameter | Purpose |
|-----------|---------|
| `?cid=` | client id minted on a fresh page load; drives [single-display arbitration](../internals/connection.md#single-display-arbitration-newest-fresh-open-wins) so the newest load wins ownership |

## Build-time flags

Not runtime config, but the two knobs that vary per base, covered in [Build from source](../build/from-source.md): `-Dbroadway-backend=true` with every other backend off, and the demos flag (`-Dbuild-demos=false` on 4.22, `-Ddemos=false` on 4.14).
