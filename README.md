# GTK Broadway fork

A fork of GTK adding missing features to the **Broadway** backend - GTK's HTML5 renderer that serves an app to a web browser over a WebSocket.

Broadway was [deprecated](https://www.phoronix.com/news/GTK-X11-Now-Deprecated) by GTK maintainers in 4.18 release (alongside X11) due to lack of maintenance. The goal of this fork is to keep it in shape during GTK4 lifecycle as a thin layer without pushing patches to upstream.

- [Supported versions](#supported-versions)
- [Installation](#installation)
- [Build from source](#build-from-source)
- [Features](#features)
  - [Bidirectional clipboard](#clipboard)
  - [Touch interface](#touch-interface)
  - [Pinch to zoom](#pinch-to-zoom)
  - [Open links in browser](#opening-links)
  - [Draggable notebook tabs](#notebook-tabs)
  - [Bugfixes](#bugfixes)
- [Known issues](#known-issues)
- [Tested configurations](#tested-configurations)

## Supported versions

| GTK    | Ubuntu base    | SONAME                 | Main branch    | Patch |
| ------ | -------------- | ---------------------- | -------------- | ----- |
| 4.14.5 | `ubuntu:24.04` | `libgtk-4.so.1.1400.5` | [`4.14.5-fork`](https://github.com/droserasprout/gtk-brotway/tree/4.14.5-fork) | [diff](https://github.com/droserasprout/gtk-brotway/compare/4.14.5...4.14.5-fork) |
| 4.22.2 | `ubuntu:26.04` | `libgtk-4.so.1.2200.2` | [`4.22.2-fork`](https://github.com/droserasprout/gtk-brotway/tree/4.22.2-fork) | [diff](https://github.com/droserasprout/gtk-brotway/compare/4.22.2...4.22.2-fork) |

Supported architectures: `amd64`, `arm64`.

## Installation

A Debian package `gtk4-brotway` is built by CI and published to [GitHub Releases](https://github.com/droserasprout/gtk-brotway/releases). It Depends/Replaces `libgtk-4-1` and `libgtk-4-bin`, overlaying only the patched `libgtk-4.so` and `gtk4-broadwayd` keeping the rest of GTK in place.

Pick the asset matching your Ubuntu base and architecture:

```sh
rel=v1           # the release to install
gtk_ver=4.14.5   # 4.22.2 on ubuntu:26.04
arch="$(dpkg --print-architecture)"
wget -O gtk.deb "https://github.com/droserasprout/gtk-brotway/releases/download/${rel}/gtk4-brotway_${gtk_ver}-${rel#v}_${arch}.deb"
apt-get install -y ./gtk.deb
apt-mark hold libgtk-4-1 libgtk-4-bin
```

## Build from source

This command builds only the Broadway backend for overlaying:

```sh
meson setup _build \
  -Dbroadway-backend=true \
  -Dx11-backend=false -Dwayland-backend=false -Dwin32-backend=false -Dmacos-backend=false \
  -Dvulkan=disabled -Dintrospection=disabled -Ddocumentation=false -Dman-pages=false \
  -Dbuild-testsuite=false -Dbuild-tests=false -Dbuild-examples=false -Dbuild-demos=false \
  -Dmedia-gstreamer=disabled -Dprint-cups=disabled \
  -Dbuildtype=release
ninja -C _build
```

On 4.14 use `-Ddemos=false` instead of `-Dbuild-demos=false` (the option was renamed in 4.22).

## Features

### Clipboard

- Copy from the app to the browser clipboard - Ctrl+C/X, right-click copy, custom Copy actions; works in text views too.
- Paste from the browser into the app - Ctrl+V, right-click paste, with no browser permission prompt.
- Multi-client safe, and paste never hangs if a tab closes.

### Touch interface

- Full touch text editing - selection handles and the Cut/Copy/Paste bubble that GTK normally hides on Broadway.
- Reliable taps - the browser no longer cancels jittery or quick taps.
- On-screen keyboard shows and hides on Android, with no lag.
- Text input from touch, including non-Latin and IME (Cyrillic, CJK, gesture-typing, autocorrect).
- Tap outside a popover or menu to dismiss it.
- Gestures keep working after the UI repaints.
- No spurious hover styling or tooltips on tap.
- The cursor and selection handles don't block taps behind them.

### Pinch to zoom

- Two-finger pinch zooms the whole UI (0.25x-5x) and re-renders crisply, like desktop page-zoom (not a stretched bitmap).
- Smooth and floating - magnification follows the fingers and sharpens once they lift.
- Per-client - zoom is local to each browser; desktop keeps native Ctrl+scroll page-zoom.
- Survives a page refresh - the zoom is remembered per browser.

### Opening links

Broadway has no system URI handler, so clicking a link normally does nothing. Links are instead routed to the browser viewing the app, which opens them in a new tab. Covers the standard GTK link paths (`gtk_show_uri`, `GtkUriLauncher`, and `GtkLabel`/`GtkLinkButton` links).

### Notebook tabs

- Drag the tab strip to scroll it on touch - pixel-for-pixel, position persists, tab widths stay natural, and edge fades hint at more tabs. A tap still selects a tab.
- Wheel over the tab strip scrolls it - horizontal pans, vertical switches the page and scrolls the new tab into view.

### Bugfixes

#### Touch

- Selection bubble's Cut/Copy/Paste fire instead of the bubble being dismissed first.
- Menus and dropdowns no longer freeze on tap.
- Dropdowns select the row you tapped, not always the first.
- Fixed a crash when reopening the selection bubble.
- Copy reappears in the bubble after Select-All.

#### Desktop

- Horizontal two-finger swipe scrolls instead of triggering the browser's back/forward navigation.
- No white flash on load or zoom-out - unpainted areas follow the browser's light/dark theme.
- New windows open centered instead of at the top-left, and re-center on zoom so they can't be stranded off-screen (the maximized main window keeps its position).
- Drags keep working when the cursor leaves the widget (e.g. scrollbar drags).

## Known issues

- WONTFIX: PRIMARY selection, middle-click paste. Desktop browsers don't expose JS API for it.
- Touch: Pinch and drag gestures leak first tap. No solution found yet without introducing delays.
- Touch: Emoji widget is ugly and slow.

## Tested configurations

- Desktop: Firefox 151.0.2, Chromium 148.0.7778.178
- Android: Firefox Beta 152.0b4, Google Chrome 146.0.7680.119
- HTTP localhost and HTTPS behind Traefik in Docker Swarm

## Not tested

- iOS
- Mixed devices (laptops with touchscreen)
