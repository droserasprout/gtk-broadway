# Brotway

A fork of GTK that fills in the missing pieces of the **Broadway** backend - GTK4's HTML5 renderer that serves an app to a web browser over a WebSocket. Broadway was [deprecated](https://www.phoronix.com/news/GTK-X11-Now-Deprecated) alongside X11 in GTK 4.18; this fork keeps it usable through the GTK4 lifecycle as a thin layer on stock GTK, with no intent to upstream.

## Run

```sh
apt install ./gtk4-brotway_*.deb       # Debian/Ubuntu (.deb from Releases)
makepkg -si                            # Arch (build from packaging/arch)
gtk4-brotway-run gtk4-demo             # changeme  ->  http://localhost:8085
```

Any GTK4 binary works - `gtk4-demo`, `gtk4-widget-factory`, `gnome-calculator`, or your own app.

## Documentation

Everything lives at **<https://droserasprout.github.io/gtk-brotway/>**:

- [Quickstart](https://droserasprout.github.io/gtk-brotway/quickstart.html) and [Installation](https://droserasprout.github.io/gtk-brotway/guide/installation.html) - install the `gtk4-brotway` Debian package and run an app over Broadway
- [Features](https://droserasprout.github.io/gtk-brotway/features/input.html) - clipboard, touch, pinch-zoom, reconnect, dynamic cursor, HiDPI, plus the [backend comparison](https://droserasprout.github.io/gtk-brotway/features/comparison.html) matrix
- [Security model](https://droserasprout.github.io/gtk-brotway/guide/security.html) and [Configuration](https://droserasprout.github.io/gtk-brotway/guide/config.html) - deploying behind TLS
- [Internals](https://droserasprout.github.io/gtk-brotway/internals/architecture.html) and [Contributing](https://droserasprout.github.io/gtk-brotway/build/contributing.html) - architecture, the wire protocol, building from source

Prebuilt packages are on the [Releases](https://github.com/droserasprout/gtk-brotway/releases) page. Current base: GTK 4.22.4 on `ubuntu:26.04`, built for amd64 + arm64 - see [Requirements](https://droserasprout.github.io/gtk-brotway/guide/requirements.html).

## License

Same as GTK: GNU Lesser General Public License, version 2 or later (`LGPL-2.0-or-later`). The patches inherit the license of the files they modify; see [`COPYING`](COPYING).
