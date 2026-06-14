# Installation

Three ways to get the fork. All install into a private prefix (`/usr/lib/gtk4-brotway`) and never touch the system GTK, so they're desktop-safe and the fork only needs to match the [4.22.x base](requirements.md):

- **Debian package** `gtk4-brotway` - from [GitHub Releases](https://github.com/droserasprout/gtk-brotway/releases) ([Ubuntu / Debian](#ubuntu--debian)).
- **Base Docker image** `ghcr.io/droserasprout/gtk-brotway` - the `.deb` baked over Ubuntu, multi-arch, with `LD_LIBRARY_PATH` pre-set so every app uses the fork ([Docker](#docker)).
- **Arch PKGBUILD** - the same layout, built by hand ([Arch / CachyOS](#arch--cachyos)).

## What gets installed

Under `/usr/lib/gtk4-brotway/`: the patched broadway-only `libgtk-4.so` (SONAME-versioned, e.g. `libgtk-4.so.1.2200.4`) with its `libgtk-4.so.1` symlink, and the `gtk4-broadwayd` daemon. In `/usr/bin`: the `gtk4-brotway-run` launcher and `gtk4-brotway-debugmenu` (the daemon spawns the [debug menu](../internals/debug-menu.md) by name). The fork lib loads the system GTK's schemas and loaders, which is why the base must match.

## Docker

Use the prebuilt **base image** - the `.deb` baked over stock Ubuntu GTK, with the SVG icon loader, Adwaita icons, and `GDK_BACKEND=broadway` default:

```dockerfile
FROM ghcr.io/droserasprout/gtk-brotway:v3.0.0   # or :latest
# ... add your GTK4 app on top; it runs on the patched Broadway backend
```

To bake the fork into your own base instead, run the [Ubuntu / Debian](#ubuntu--debian) steps in a `RUN`. See [Security model](security.md#in-a-container) for the full Dockerfile and operator checklist.

## Ubuntu / Debian

Pick the asset for your architecture (base is GTK 4.22.4 on `ubuntu:26.04`):

```sh
rel=v3.0.0       # the release to install - see the Releases page for the latest tag
arch="$(dpkg --print-architecture)"
wget -O gtk.deb "https://github.com/droserasprout/gtk-brotway/releases/download/${rel}/gtk4-brotway_4.22.4-${rel#v}_${arch}.deb"
apt-get install -y ./gtk.deb
```

Asset names are `gtk4-brotway_<gtk>-<rev>_<arch>.deb` (e.g. `gtk4-brotway_4.22.4-3.0.0_amd64.deb`). Uninstall with `apt-get remove gtk4-brotway`.

> Latest tag without hardcoding: `rel="$(curl -fsSL https://api.github.com/repos/droserasprout/gtk-brotway/releases/latest | grep -oP '"tag_name":\s*"\K[^"]+')"`.

## Arch / CachyOS

No prebuilt package - build it with the PKGBUILD on the `ci` branch. Your system `gtk4` must be the same series (currently `1:4.22.4`):

```sh
git clone -b ci https://github.com/droserasprout/gtk-brotway
cd gtk-brotway/packaging/arch
makepkg -si
```

## Running an app

Launch any GTK4 app through `gtk4-brotway-run`; it starts `broadwayd`, runs the app, and tears it down on exit:

```sh
gtk4-brotway-run gtk4-widget-factory   # -> http://localhost:8085  (triple-Shift = debug menu)
```

Flags (`--auto`, `--open`, `--address`, `--display`, `--port`) and the manual two-process setup: [Running](running.md#one-command).
