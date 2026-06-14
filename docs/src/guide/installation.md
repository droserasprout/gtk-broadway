# Installation

Three ways to get the fork - all install into a private prefix and leave the rest of your GTK (GIR, `gtk-4-common`, themes, the system `libgtk-4`) untouched, so the fork must match the [4.22.x base](requirements.md):

- **Debian package** `gtk4-brotway` - published to [GitHub Releases](https://github.com/droserasprout/gtk-brotway/releases). Private-prefix install, desktop-safe; opt in per launch via `gtk4-brotway-run` ([Ubuntu / Debian](#ubuntu--debian)).
- **Base Docker image** `ghcr.io/droserasprout/gtk-brotway` - the `.deb` baked over Ubuntu, multi-arch, with `LD_LIBRARY_PATH` pointed at the prefix so every app uses the fork. `FROM` it for any GTK4 Broadway binary ([Docker](#docker)).
- **Arch PKGBUILD** - the same private-prefix layout, opt-in per launch ([Arch / CachyOS](#arch--cachyos)).

## What gets installed

Everything lands under `/usr/lib/gtk4-brotway/` (the daemon) and `/usr/bin` (the tools), never over the system GTK:

- Patched broadway-only `libgtk-4.so` (SONAME-versioned, e.g. `libgtk-4.so.1.2200.4`) and its `libgtk-4.so.1` symlink, in the prefix
- `gtk4-broadwayd` daemon, in the prefix
- `gtk4-brotway-run` launcher and `gtk4-brotway-debugmenu` binary in `/usr/bin` (daemon spawns the [debug menu](../internals/debug-menu.md) by name)

## Docker

Use the prebuilt **base image** - the `.deb` baked over stock Ubuntu GTK, with the SVG icon loader, Adwaita icons, and `GDK_BACKEND=broadway` default:

```dockerfile
FROM ghcr.io/droserasprout/gtk-brotway:v3.0.0   # or :latest
# ... add your GTK4 app on top; it runs on the patched Broadway backend
```

To bake the fork into your own base instead, run the [Ubuntu / Debian](#ubuntu--debian) steps in a `RUN`. See [Security model](security.md#in-a-container) for the full Dockerfile and operator checklist.

## Ubuntu / Debian

The fork is broadway-only (no X11/Wayland backend), but it installs into a private prefix and never replaces the system `libgtk-4`, so it's safe on a desktop: your other GTK4 apps keep their normal backends, and only what you launch through `gtk4-brotway-run` uses the fork. Your system `libgtk-4` must be the same [4.22.x series](requirements.md).

Pick the asset for your architecture (base is GTK 4.22.4 on `ubuntu:26.04`):

```sh
rel=v3.0.0       # the release to install - see the Releases page for the latest tag
arch="$(dpkg --print-architecture)"
wget -O gtk.deb "https://github.com/droserasprout/gtk-brotway/releases/download/${rel}/gtk4-brotway_4.22.4-${rel#v}_${arch}.deb"
apt-get install -y ./gtk.deb
gtk4-brotway-run gtk4-widget-factory   # -> http://localhost:8085
```

Asset names are `gtk4-brotway_<gtk>-<rev>_<arch>.deb` (e.g. `gtk4-brotway_4.22.4-3.0.0_amd64.deb`).

> Latest tag without hardcoding: `rel="$(curl -fsSL https://api.github.com/repos/droserasprout/gtk-brotway/releases/latest | grep -oP '"tag_name":\s*"\K[^"]+')"`. Uninstall: `apt-get remove gtk4-brotway` - nothing else is touched.

## Arch / CachyOS

No prebuilt package - build it with the PKGBUILD on the `ci` branch. Your system `gtk4` must be the same series (currently `1:4.22.4`).

```sh
git clone -b ci https://github.com/droserasprout/gtk-brotway
cd gtk-brotway/packaging/arch
makepkg -si
```

Same layout as the `.deb`: the patched lib and `gtk4-broadwayd` go in a private prefix (`/usr/lib/gtk4-brotway/`), so install/removal stay clean and `pacman` never fights it.

```sh
gtk4-brotway-run gtk4-widget-factory
# -> http://localhost:8085  (triple-Shift = debug menu)
```

The launcher points `LD_LIBRARY_PATH` at the prefix, starts the fork's `broadwayd`, and tears it down on exit. It takes `--auto` / `--open` / `--address` / `--display` / `--port` - see [Running](running.md#one-command).
