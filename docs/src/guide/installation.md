# Installation

The fork ships as a Debian package, `gtk4-brotway`, built by CI and published to [GitHub Releases](https://github.com/droserasprout/gtk-brotway/releases).

## What the package does

The package `Depends` on and `Replaces` the stock runtime packages `libgtk-4-1` and `libgtk-4-bin`. It overlays only:

- the patched `libgtk-4.so` (the SONAME-versioned shared object, e.g. `libgtk-4.so.1.2200.2`),
- the `libgtk-4.so.1` SONAME symlink, re-pointed at the patched `.so`,
- the `gtk4-broadwayd` daemon binary,
- the `gtk4-brotway-debugmenu` binary (net-new file; the daemon spawns the [debug menu](../internals/debug-menu.md) by name).

Everything else from apt's GTK (the GIR, `gtk-4-common`, themes, ...) is left untouched. That is why the package and your system must agree on the GTK base version. See [Supported versions](versions.md).

## Install

Pick the asset matching your Ubuntu base and architecture:

```sh
rel=v2.1         # the release to install - see the Releases page for the latest tag
gtk_ver=4.14.5   # use 4.22.2 on an ubuntu:26.04 base
arch="$(dpkg --print-architecture)"
wget -O gtk.deb "https://github.com/droserasprout/gtk-brotway/releases/download/${rel}/gtk4-brotway_${gtk_ver}-${rel#v}_${arch}.deb"
apt-get install -y ./gtk.deb
apt-mark hold libgtk-4-1 libgtk-4-bin
```

The asset name encodes base and architecture: `gtk4-brotway_<gtk>-<rev>_<arch>.deb` (e.g. `gtk4-brotway_4.22.2-2.1_amd64.deb`).

> To always grab the newest release without hardcoding the tag, resolve it from the GitHub API first:
>
> ```sh
> rel="$(curl -fsSL https://api.github.com/repos/droserasprout/gtk-brotway/releases/latest \
>   | grep -oP '"tag_name":\s*"\K[^"]+')"
> ```

## Hold the stock packages

`apt-mark hold libgtk-4-1 libgtk-4-bin` matters: it stops a later `apt upgrade` from re-installing the stock GTK files over the patched ones. Skip the hold and a routine system upgrade silently reverts the fork, so Broadway loses clipboard/touch until you re-install the `.deb`.

## In a container

Same steps inside a Docker image; see [Deploying behind TLS](deployment.md#in-a-container) for the Dockerfile snippet and the full operator checklist.

> To undo the overlay: `apt-mark unhold libgtk-4-1 libgtk-4-bin` then `apt-get install --reinstall libgtk-4-1 libgtk-4-bin`.
