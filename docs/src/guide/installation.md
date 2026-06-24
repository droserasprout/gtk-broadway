# Installation

Three ways to get the fork. All install into a private prefix (`/usr/lib/gtk4-brotway`), leave the system GTK untouched, and so are desktop-safe; the fork only needs to match the [4.22.x base](requirements.md):

- **Debian package** `gtk4-brotway` - from [GitHub Releases](https://github.com/droserasprout/gtk-brotway/releases) ([Ubuntu / Debian](#ubuntu--debian))
- **Base Docker image** `ghcr.io/droserasprout/gtk-brotway` - the `.deb` baked over Ubuntu, multi-arch ([Docker](#docker))
- **Arch package** - prebuilt x86_64 `.pkg.tar.zst`, or build from the PKGBUILD ([Arch / CachyOS](#arch--cachyos))

## What gets installed

- `/usr/lib/gtk4-brotway/libgtk-4.so.1*` - patched broadway-only GTK lib (+ `libgtk-4.so.1` symlink)
- `/usr/lib/gtk4-brotway/gtk4-broadwayd` - the Broadway daemon
- `/usr/bin/gtk4-brotway-run` - the launcher
- `/usr/bin/gtk4-brotway-debugmenu` - the [debug menu](../internals/debug-menu.md), spawned by the daemon by name

## Docker

Use the prebuilt base image - the `.deb` over stock Ubuntu GTK, with the SVG icon loader, Adwaita icons, `GDK_BACKEND=broadway`, and `LD_LIBRARY_PATH` pre-set:

```dockerfile
FROM ghcr.io/droserasprout/gtk-brotway:v3.1.1   # or :latest
# ... add your GTK4 app on top; it runs on the patched Broadway backend
```

To bake the fork into your own base, run the [Ubuntu / Debian](#ubuntu--debian) steps in a `RUN`. See [Security model](security.md#in-a-container) for the full Dockerfile and operator checklist.

## Ubuntu / Debian

Pick the asset for your architecture (base is GTK 4.22.4 on `ubuntu:26.04`):

```sh
rel=v3.1.1       # the release to install - see the Releases page for the latest tag
arch="$(dpkg --print-architecture)"
wget -O gtk.deb "https://github.com/droserasprout/gtk-brotway/releases/download/${rel}/gtk4-brotway_4.22.4-${rel#v}_${arch}.deb"
apt-get install -y ./gtk.deb
```

Asset names are `gtk4-brotway_<gtk>-<rev>_<arch>.deb` (e.g. `gtk4-brotway_4.22.4-3.1.1_amd64.deb`). Uninstall with `apt-get remove gtk4-brotway`.

> Latest tag without hardcoding: `rel="$(curl -fsSL https://api.github.com/repos/droserasprout/gtk-brotway/releases/latest | grep -oP '"tag_name":\s*"\K[^"]+')"`.

## Arch / CachyOS

Prebuilt x86_64 package - download the `.pkg.tar.zst` from the Release and install it. It's frozen to the `gtk4` it was built against, so rebuild from source after an Arch `gtk4` soname bump:

```sh
rel=v3.1.2       # the release to install - see the Releases page for the latest tag
wget "https://github.com/droserasprout/gtk-brotway/releases/download/${rel}/gtk4-brotway-${rel#v}-1-x86_64.pkg.tar.zst"
sudo pacman -U ./gtk4-brotway-*.pkg.tar.zst
```

Or build from source (any arch; tracks your live `gtk4`, which must be the same 4.22.x series):

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

Flags (`--auto`, `--open`, `--address`, `--display`, `--port`) and the by-hand setup: [Running](running.md#the-launcher).
