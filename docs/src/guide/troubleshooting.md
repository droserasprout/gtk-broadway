# Troubleshooting

Symptom-first, for things that should work but don't - usually a deployment or install mismatch. [Known issues](#known-issues) lists what is by-design unsupported.

## Is the fork actually loaded?

Most "feature missing" reports are stock GTK running, not the fork. Confirm:

```sh
dpkg -l gtk4-brotway        # the package is installed
ls /usr/lib/gtk4-brotway/   # the fork libgtk-4 + gtk4-broadwayd live in the prefix
```

Functional test: touch text selection and clipboard copy/paste only exist in the fork. If they're absent, stock GTK is loaded - see "features vanished" below.

## Diagnostic table

| Symptom | Likely cause | Fix |
|---|---|---|
| Blank page, no connection | daemon not running, or browser hitting the wrong port | run `gtk4-brotway-run gtk4-demo` (it starts the daemon); the page is on `8080 + N` ([Running](running.md)) |
| Blank page behind a proxy | proxy not forwarding the WebSocket upgrade | forward `Upgrade`/`Connection` headers; all ops run over the WS ([Security model](security.md)) |
| Page loads, app never appears | app not started, or wrong target | run it via `gtk4-brotway-run` (or by hand; see [Running](running.md#by-hand)) |
| Page loads, but shows "disconnected" | another fresh tab took the single display | newest fresh load wins; reload the tab you want to own it ([arbitration](../internals/connection.md#single-display-arbitration-newest-fresh-open-wins)) |
| App fails to start / can't load `libgtk-4.so` | fork base doesn't match the system GTK (SONAME mismatch) | run the 4.22.4 deb on an `ubuntu:26.04` base ([Requirements](requirements.md)); see below |
| `undefined symbol: gdk_x11_*` / `gdk_wayland_*` | running against an old fork `libgtk-4` (pre-stub) | update to a current fork build ([Running](running.md#the-launcher)) |
| Touch UI / OSK / clipboard missing | app launched directly, so it loaded stock GTK | start it via `gtk4-brotway-run` (or export `LD_LIBRARY_PATH=/usr/lib/gtk4-brotway`) |
| Copy/paste silently does nothing | insecure context over remote `http://` | serve `https://` or `http://localhost` ([secure context](running.md#secure-context-note)) |
| Middle-click paste doesn't work | PRIMARY selection is unsupported | by design ([Known issues](#known-issues)) |
| Links don't open / "No known URI provider available" | popup blocker caught `window.open` | allow popups for the origin ([Opening links](../features/input.md#opening-links)) |
| Socket floods, high CPU | a spinner/progress animation repaints every frame | pre-existing upstream behaviour; stop the animation ([Known issues](#known-issues)) |

## Checking the GTK base mismatch

The fork's `libgtk-4.so` loads the system GTK's schemas and loaders, so its base must match the GTK apt installed. Compare them:

```sh
dpkg -s libgtk-4-1 | grep ^Version                 # the apt GTK version (e.g. 4.22.x on ubuntu:26.04)
ls -l /usr/lib/gtk4-brotway/libgtk-4.so.1          # the SONAME the fork ships
```

A `4.22.4` fork won't load on a mismatched GTK series; run it on an `ubuntu:26.04` base ([Requirements](requirements.md)).

## Iterating on a change that didn't take effect

Reloading the browser never picks up a `libgtk` change. What needs a daemon restart vs a rebuild plus app restart is in [broadwayd vs libgtk](../build/from-source.md#iterating-broadwayd-vs-libgtk).

## Known issues

Bugs are tracked on the [issue tracker](https://github.com/droserasprout/gtk-brotway/issues). The items below are known limitations, not currently planned for a fix:

- **PRIMARY selection and middle-click paste.** Desktop browsers expose no JavaScript API for the X11-style PRIMARY selection, so there's no way to bridge it.
- **Clipboard copy on insecure origins.** Copy may silently fail outside a user gesture, since `navigator.clipboard` is gated to secure contexts. Serve over `https://` or `http://localhost`.
- **No fling on touchpad/wheel scrolling.** Scrolling is smooth (pixel-precise) but kinetic momentum is off: a server-side fling animates many frames that the remote framebuffer renders choppily, and the browser already drives the deltas. Touch-drag fling is unaffected.

Tested browsers live in [Requirements](requirements.md#tested-browsers).
