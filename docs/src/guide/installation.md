# Installation

Three ways to get the fork - all overlay the same Broadway pieces and leave the rest of your GTK (GIR, `gtk-4-common`, themes) untouched, so it must match the [4.22.x base](requirements.md):

- **Debian package** `gtk4-brotway` - published to [GitHub Releases](https://github.com/droserasprout/gtk-brotway/releases). Transparently overlays system GTK (every Broadway app uses the fork). Headless/container only - **not for desktops** ([warning below](#ubuntu--debian)).
- **Base Docker image** `ghcr.io/droserasprout/gtk-brotway` - the `.deb` baked over Ubuntu, multi-arch. `FROM` it for any GTK4 Broadway binary ([Docker](#docker)).
- **Arch PKGBUILD** - a conflict-free private-prefix overlay, opt-in per launch ([Arch / CachyOS](#arch--cachyos)).

## What gets overlaid

- Patched `libgtk-4.so` (SONAME-versioned, e.g. `libgtk-4.so.1.2200.4`) and its `libgtk-4.so.1` symlink
- `gtk4-broadwayd` daemon
- `gtk4-brotway-debugmenu` binary (daemon spawns the [debug menu](../internals/debug-menu.md) by name)

## Docker

Use the prebuilt **base image** - the `.deb` baked over stock Ubuntu GTK, with the SVG icon loader, Adwaita icons, and `GDK_BACKEND=broadway` default:

```dockerfile
FROM ghcr.io/droserasprout/gtk-brotway:v3.0.0   # or :latest
# ... add your GTK4 app on top; it runs on the patched Broadway backend
```

To bake the fork into your own base instead, run the [Ubuntu / Debian](#ubuntu--debian) steps in a `RUN`. See [Security model](security.md#in-a-container) for the full Dockerfile and operator checklist.

## Ubuntu / Debian

> **Headless only, do not install on desktop!** This `.deb` is compiled Broadway-only (no X11/Wayland backend) and overlays the system `libgtk-4` in place. To run the fork on a desktop without touching system GTK, use a private-prefix install instead - the [Arch PKGBUILD](#arch--cachyos) shows the pattern (or [build from source](../build/from-source.md) and point `LD_LIBRARY_PATH` at it).

Pick the asset for your architecture (base is GTK 4.22.4 on `ubuntu:26.04`):

```sh
rel=v3.0.0       # the release to install - see the Releases page for the latest tag
arch="$(dpkg --print-architecture)"
wget -O gtk.deb "https://github.com/droserasprout/gtk-brotway/releases/download/${rel}/gtk4-brotway_4.22.4-${rel#v}_${arch}.deb"
apt-get install -y ./gtk.deb
apt-mark hold libgtk-4-1 libgtk-4-bin   # keep a later apt upgrade from reverting the fork to stock
```

Asset names are `gtk4-brotway_<gtk>-<rev>_<arch>.deb` (e.g. `gtk4-brotway_4.22.4-3.0.0_amd64.deb`).

> Latest tag without hardcoding: `rel="$(curl -fsSL https://api.github.com/repos/droserasprout/gtk-brotway/releases/latest | grep -oP '"tag_name":\s*"\K[^"]+')"`. Undo the overlay: `apt-mark unhold libgtk-4-1 libgtk-4-bin && apt-get install --reinstall libgtk-4-1 libgtk-4-bin`.

## Arch / CachyOS

No prebuilt package - build it with the PKGBUILD on the `ci` branch. Your system `gtk4` must be the same series (currently `1:4.22.4`).

```sh
git clone -b ci https://github.com/droserasprout/gtk-brotway
cd gtk-brotway/packaging/arch
makepkg -si
```

Unlike the `.deb`, this does **not** replace system GTK: the patched lib and `gtk4-broadwayd` go in a private prefix (`/usr/lib/gtk4-brotway/`), so install/removal stay clean and `pacman` never fights it.

```sh
gtk4-brotway-run gtk4-widget-factory
# -> http://localhost:8085  (triple-Shift = debug menu)
```

The launcher points `LD_LIBRARY_PATH` at the prefix, starts the fork's `broadwayd`, and tears it down on exit. It ships in the `.deb` too (where it skips `LD_LIBRARY_PATH`, since the fork is the system GTK) and takes `--auto` / `--open` / `--display` / `--port` - see [Running](running.md#one-command).
