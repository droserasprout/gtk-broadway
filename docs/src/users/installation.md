# Installation

The fork ships as a Debian package, `gtk4-broadway-fork`, built by CI and published to
[GitHub Releases](https://github.com/droserasprout/gtk-broadway/releases).

## What the package does

The package `Depends` on and `Replaces` the stock runtime packages `libgtk-4-1` and
`libgtk-4-bin`. It overlays only:

- the patched `libgtk-4.so` (the SONAME-versioned shared object, e.g. `libgtk-4.so.1.2200.2`),
- the `libgtk-4.so.1` SONAME symlink, re-pointed at the patched `.so`,
- the `gtk4-broadwayd` daemon binary.

Everything else from apt's GTK (the GIR, `gtk-4-common`, themes, ...) is left untouched. This is
why the package and your system must agree on the GTK base version - see
[Supported versions](versions.md).

## Install

Pick the asset matching your Ubuntu base and architecture:

```sh
rel=v1           # the release to install
gtk_ver=4.14.5   # use 4.22.2 on an ubuntu:26.04 base
arch="$(dpkg --print-architecture)"
wget -O gtk.deb "https://github.com/droserasprout/gtk-broadway/releases/download/${rel}/gtk4-broadway-fork_${gtk_ver}-${rel#v}_${arch}.deb"
apt-get install -y ./gtk.deb
apt-mark hold libgtk-4-1 libgtk-4-bin
```

The asset name encodes base and architecture:
`gtk4-broadway-fork_<gtk>-<rev>_<arch>.deb` (e.g. `gtk4-broadway-fork_4.22.2-1_amd64.deb`).

## Hold the stock packages

`apt-mark hold libgtk-4-1 libgtk-4-bin` is important: it stops a later `apt upgrade` from
re-installing the stock GTK files over the patched ones. Without the hold, a routine system
upgrade silently reverts the fork and Broadway loses clipboard/touch until you re-install the
`.deb`.

## In a container

The reference deployment installs the arch-matching `.deb` inside a Docker image over apt's GTK
and holds the runtime packages - the same three steps as above, plus `dpkg --print-architecture`
to select between the amd64 and arm64 assets at build time.

> If you ever need to undo the overlay: `apt-mark unhold libgtk-4-1 libgtk-4-bin` then
> `apt-get install --reinstall libgtk-4-1 libgtk-4-bin`.
