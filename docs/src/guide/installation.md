# Installation

The fork is distributed three ways:

- **Debian package** `gtk4-brotway` - built by CI, published to [GitHub Releases](https://github.com/droserasprout/gtk-brotway/releases). It works on any Ubuntu/Debian host and *transparently* overlays the stock GTK runtime (every Broadway app uses the fork, no opt-in).
- **Base Docker image** `ghcr.io/droserasprout/gtk-brotway` - the `.deb` baked over a stock Ubuntu GTK runtime, multi-arch (amd64, arm64). App-agnostic: `FROM` it for any GTK4 Broadway binary. See [Docker](#docker).
- **Arch PKGBUILD** (`packaging/arch/`) - built by hand with `makepkg`. A *conflict-free* private-prefix overlay you opt into per launch via the `gtk4-brotway-run` wrapper.

Both only swap the Broadway pieces and leave the rest of your GTK (GIR, `gtk-4-common`, themes, ...) untouched - so the package and your system must agree on the GTK base version (4.22.x). See [Supported versions](versions.md).

## What gets overlaid

- the patched `libgtk-4.so` (the SONAME-versioned object, e.g. `libgtk-4.so.1.2200.4`),
- the `libgtk-4.so.1` SONAME symlink, re-pointed at the patched `.so`,
- the `gtk4-broadwayd` daemon binary,
- the `gtk4-brotway-debugmenu` binary (net-new; the daemon spawns the [debug menu](../internals/debug-menu.md) by name).

## Docker

The simplest path is the prebuilt **base image** - the `.deb` already baked over a stock Ubuntu GTK runtime, published multi-arch (amd64, arm64) to GHCR per release:

```dockerfile
FROM ghcr.io/droserasprout/gtk-brotway:v3.0.0   # or :latest
# ... add your GTK4 app on top; it runs on the patched Broadway backend
```

It carries no app and no language bindings - just the patched GTK, `gtk4-broadwayd`, and the generic bits any GTK4 app needs to render (the SVG icon loader + Adwaita icons), with `GDK_BACKEND=broadway` set as the default. It is built by the `image.yml` workflow on the `ci` branch from the same release `.deb`s.

To bake the fork into your own base instead, install the arch-matching `.deb` over a stock GTK base and `apt-mark hold` the runtime - the same [Ubuntu/Debian](#ubuntu--debian) steps inside a `RUN`. See [Security model](security.md#in-a-container) for the Dockerfile snippet and the operator checklist.

## Ubuntu / Debian

Pick the asset matching your architecture (the base is GTK 4.22.4 on `ubuntu:26.04`):

```sh
rel=v3.0.0       # the release to install - see the Releases page for the latest tag
gtk_ver=4.22.4   # on an ubuntu:26.04 base
arch="$(dpkg --print-architecture)"
wget -O gtk.deb "https://github.com/droserasprout/gtk-brotway/releases/download/${rel}/gtk4-brotway_${gtk_ver}-${rel#v}_${arch}.deb"
apt-get install -y ./gtk.deb
apt-mark hold libgtk-4-1 libgtk-4-bin
```

The asset name encodes base and architecture: `gtk4-brotway_<gtk>-<rev>_<arch>.deb` (e.g. `gtk4-brotway_4.22.4-3.0.0_amd64.deb`).

> To always grab the newest release without hardcoding the tag, resolve it from the GitHub API first:
>
> ```sh
> rel="$(curl -fsSL https://api.github.com/repos/droserasprout/gtk-brotway/releases/latest \
>   | grep -oP '"tag_name":\s*"\K[^"]+')"
> ```

### Hold the stock packages

The `apt-mark hold libgtk-4-1 libgtk-4-bin` matters: it stops a later `apt upgrade` from re-installing the stock GTK files over the patched ones. Skip the hold and a routine system upgrade silently reverts the fork, so Broadway loses every patched feature until you re-install the `.deb`.

> To undo the overlay: `apt-mark unhold libgtk-4-1 libgtk-4-bin` then `apt-get install --reinstall libgtk-4-1 libgtk-4-bin`.

## Arch / CachyOS

No prebuilt package - build it locally with the PKGBUILD on the `ci` branch. Your system `gtk4` must be the same upstream series (currently `1:4.22.4`).

```sh
git clone -b ci https://github.com/droserasprout/gtk-brotway
cd gtk-brotway/packaging/arch
makepkg -si
```

Unlike the `.deb`, this does **not** replace your system GTK. The patched lib and `gtk4-broadwayd` install into a private prefix (`/usr/lib/gtk4-brotway/`), so install/removal stay clean and `pacman -Syu` never fights it. Opt into the fork per launch:

```sh
gtk4-brotway-run gtk4-widget-factory
# -> http://localhost:8085  (triple-Shift = debug menu)

BROTWAY_PORT=9000 BROTWAY_DISPLAY=:7 gtk4-brotway-run gnome-calculator
```

The wrapper points `LD_LIBRARY_PATH` at the prefix, starts the fork's `broadwayd`, and tears it down on exit. To uninstall: `pacman -R gtk4-brotway`.
