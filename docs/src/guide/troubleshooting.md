# Troubleshooting

Symptom-first. [Known issues](known-issues.md) lists what is by-design unsupported; this page is for things that should work but don't, almost always a deployment or install mismatch.

## Is the fork actually loaded?

Most "feature missing" reports are stock GTK running, not the fork. Confirm in order:

```sh
dpkg -l gtk4-brotway        # the package is installed
apt-mark showhold | grep libgtk-4 # libgtk-4-1 and libgtk-4-bin are held
```

The functional test: touch text selection (handles + Cut/Copy/Paste bubble) and clipboard copy/paste only exist in the fork. If they are absent, stock GTK is loaded - see "features vanished" below.

## Diagnostic table

| Symptom | Likely cause | Fix |
|---|---|---|
| Blank page, no connection | daemon not running, or browser hitting the wrong port | start `gtk4-broadwayd :N`; the page is on `8080 + N` ([Running](running.md)) |
| Blank page behind a proxy | proxy not forwarding the WebSocket upgrade | forward `Upgrade`/`Connection` headers; all ops run over the WS ([Deployment](deployment.md)) |
| Page loads, app never appears | app not started, or wrong target | run it with `GDK_BACKEND=broadway BROADWAY_DISPLAY=:N` ([Config](config.md)) |
| Page loads, but shows "disconnected" | another fresh tab took the single display | newest fresh load wins; reload the tab you want to own it ([arbitration](../internals/connection.md#single-display-arbitration-newest-fresh-open-wins)) |
| App fails to start / can't load `libgtk-4.so` | `.deb` base doesn't match the system GTK (SONAME mismatch) | install the base matching your Ubuntu ([Supported versions](versions.md)); see below |
| Touch UI / OSK / clipboard missing | stock GTK loaded, not the fork | confirm the package + hold above |
| Features worked, then vanished | `apt upgrade` reverted to stock GTK | re-install the `.deb`, then `apt-mark hold libgtk-4-1 libgtk-4-bin` ([Installation](installation.md)) |
| Copy/paste silently does nothing | insecure context over remote `http://` | serve `https://` or `http://localhost` ([secure context](running.md#secure-context-note)) |
| Middle-click paste doesn't work | PRIMARY selection is unsupported | by design - no JS API for it ([Known issues](known-issues.md)) |
| Links don't open / "No known URI provider available" | popup blocker caught `window.open` | allow popups for the origin ([Opening links](../features/open-uri.md#caveats)) |
| Socket floods, high CPU | a spinner/progress animation repaints every frame | pre-existing upstream behaviour; stop the animation ([Known issues](known-issues.md)) |

## Checking the GTK base mismatch

The `.deb` overlays the SONAME-versioned `libgtk-4.so` in place, so its base must match the GTK apt installed. Compare them:

```sh
dpkg -s libgtk-4-1 | grep ^Version       # the apt GTK version (e.g. 4.22.x on ubuntu:26.04)
ls -l /usr/lib/*/libgtk-4.so.1            # the SONAME the symlink points at
```

A `4.14.5` deb on a `4.22.x` system (or vice versa) leaves a dangling SONAME and the app won't load. Install the base that matches: [4.14.5 on `ubuntu:24.04`, 4.22.2 on `ubuntu:26.04`](versions.md).

## Iterating on a change that didn't take effect

A client-side change (`broadway.js` / `client.html`) needs the **daemon** rebuilt and restarted, then a plain reload. A `libgtk` change needs the library rebuilt and the **app** restarted. Reloading the browser alone never picks up a `libgtk` change. See [broadwayd vs libgtk](../internals/build-split.md).
