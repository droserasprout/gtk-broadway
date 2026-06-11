# Brotway

A fork of GTK adding missing features to the **Broadway** backend - the HTML5 renderer that serves a GTK4 app to a web browser over a WebSocket.

Broadway was [deprecated](https://www.phoronix.com/news/GTK-X11-Now-Deprecated) by GTK maintainers in the 4.18 release (alongside X11) due to lack of maintenance. The goal of this fork is to **keep it in shape** during the GTK4 lifecycle as a thin layer on stock GTK, without pushing patches to upstream.

**Documentation:** <https://droserasprout.github.io/gtk-brotway/> - quickstart, feature guides, deployment, internals.

## Supported versions

| GTK    | Ubuntu base    | SONAME                 | Main branch    | Patch |
| ------ | -------------- | ---------------------- | -------------- | ----- |
| 4.14.5 | `ubuntu:24.04` | `libgtk-4.so.1.1400.5` | [`4.14.5-fork`](https://github.com/droserasprout/gtk-brotway/tree/4.14.5-fork) | [diff](https://github.com/droserasprout/gtk-brotway/compare/4.14.5...4.14.5-fork) |
| 4.22.2 | `ubuntu:26.04` | `libgtk-4.so.1.2200.2` | [`4.22.2-fork`](https://github.com/droserasprout/gtk-brotway/tree/4.22.2-fork) | [diff](https://github.com/droserasprout/gtk-brotway/compare/4.22.2...4.22.2-fork) |

Supported architectures: `amd64`, `arm64`. GTK older than 4.14 is not supported and won't be; see [Supported versions](https://droserasprout.github.io/gtk-brotway/guide/versions.html).

## Installation

A Debian package `gtk4-brotway` is built by CI and published to [GitHub Releases](https://github.com/droserasprout/gtk-brotway/releases). It Depends/Replaces `libgtk-4-1` and `libgtk-4-bin`, overlaying only the patched `libgtk-4.so`, `gtk4-broadwayd`, and the debug-menu binary, keeping the rest of GTK in place.

Pick the asset matching your Ubuntu base and architecture:

```sh
rel=v2.1         # the release to install - see the Releases page for the latest tag
gtk_ver=4.14.5   # 4.22.2 on ubuntu:26.04
arch="$(dpkg --print-architecture)"
wget -O gtk.deb "https://github.com/droserasprout/gtk-brotway/releases/download/${rel}/gtk4-brotway_${gtk_ver}-${rel#v}_${arch}.deb"
apt-get install -y ./gtk.deb
apt-mark hold libgtk-4-1 libgtk-4-bin
```

Then start the daemon and point an app at it:

```sh
gtk4-broadwayd :5 &
GDK_BACKEND=broadway BROADWAY_DISPLAY=:5 your-gtk4-app
# open http://localhost:8085
```

See the [Quickstart](https://droserasprout.github.io/gtk-brotway/quickstart.html) for the walkthrough and [Deploying behind TLS](https://droserasprout.github.io/gtk-brotway/guide/deployment.html) for a real deployment.

## Features

- **[Clipboard](https://droserasprout.github.io/gtk-brotway/features/clipboard.html)** - copy/paste between app and browser, no permission prompt, multi-client safe.
- **[Touch interface](https://droserasprout.github.io/gtk-brotway/features/touch.html)** - touch text editing (selection handles, the Cut/Copy/Paste bubble), reliable taps, on-screen keyboard, IME and non-Latin input.
- **[Pinch to zoom](https://droserasprout.github.io/gtk-brotway/features/zoom.html)** - two-finger UI zoom (0.25x-5x) that re-renders crisply; per client, survives refresh.
- **[Notebook tabs](https://droserasprout.github.io/gtk-brotway/features/notebook.html)** - drag/wheel scrolling of the tab strip on touch.
- **[Opening links](https://droserasprout.github.io/gtk-brotway/features/open-uri.html)** - clicked links open in the viewing browser.
- **[Dynamic cursor](https://droserasprout.github.io/gtk-brotway/features/cursor.html)** - the browser pointer follows GTK's cursor shape.
- **[Connection management](https://droserasprout.github.io/gtk-brotway/features/connection.html)** - in-place reconnect after screen-off or network change, heartbeat, single-display arbitration.
- **[Scaling & HiDPI](https://droserasprout.github.io/gtk-brotway/features/scaling.html)** - crisp icons under scale transforms.
- **[Desktop & rendering fixes](https://droserasprout.github.io/gtk-brotway/features/desktop-fixes.html)** - no white flash, centered windows, popups on their anchor, click-through shadows, seam-free borders.

The [backend comparison](https://droserasprout.github.io/gtk-brotway/features/comparison.html) matrix shows where the fork sits next to Wayland/X11/Win32/macOS, feature by feature.

## Building from source

The Broadway-only Meson config, the per-base flags, and the build dependencies are in [Building from source](https://droserasprout.github.io/gtk-brotway/build/from-source.html). For hacking on the fork itself, start with [Contributing](https://droserasprout.github.io/gtk-brotway/build/contributing.html).

## Known issues

PRIMARY selection / middle-click paste (no browser API for it), tap leak into pinch/pan gestures, slow emoji widget. Full list and tested browser/OS configurations: [Known issues](https://droserasprout.github.io/gtk-brotway/guide/known-issues.html).
