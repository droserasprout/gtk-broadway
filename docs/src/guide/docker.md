# Docker

The base image is the quickest way to run a GTK4 app over Broadway in a container: stock Ubuntu GTK with the fork `.deb` overlaid into its private prefix, plus the SVG icon loader, Adwaita icons, and `GDK_BACKEND=broadway` / `LD_LIBRARY_PATH=/usr/lib/gtk4-brotway` pre-set. It carries no app - `FROM` it and add your own. Multi-arch (amd64 + arm64).

## Prebuilt base image

```dockerfile
FROM ghcr.io/droserasprout/gtk-brotway:v3.1.3   # or :latest
# ... add your GTK4 app on top; it runs on the patched Broadway backend
```

| Tag | Slim ([`-nogl`](#nogl-variant)) | Tracks |
|---|---|---|
| `:vX.Y.Z` | `:vX.Y.Z-nogl` | a pinned release |
| `:3` | `:3-nogl` | the latest 3.x (not v4+) |
| `:latest` | `:nogl` | the newest release |

## `:nogl` variant

`:vX.Y.Z-nogl` / `:nogl` is the same image minus the stock GTK + its GL/Mesa/LLVM chain (`libllvm`, `mesa-libgallium`, libGL/EGL/GBM), purged in the install layer - ~200 MB smaller. Broadway renders via cairo, so none of it runs.

```dockerfile
FROM ghcr.io/droserasprout/gtk-brotway:v3.1.3-nogl
```

It is opt-in, not the default, because the purge (`dpkg --force-depends`) leaves a deliberately broken dpkg state: a downstream `apt-get install -f` can drag the whole chain back in, and any app that creates its own GL context (`GtkGLArea`, offscreen GL) loses the llvmpipe software fallback and fails to realize. Use `-nogl` for apps known not to touch GL; use the full tag as a general base.

## Build your own base

To bake the fork into an arbitrary base instead of `FROM`-ing the prebuilt one, install the arch-matching `.deb` and point `LD_LIBRARY_PATH` at the prefix. The base image's GTK must match the `.deb` base ([4.22.4 on `ubuntu:26.04`](requirements.md)):

```dockerfile
ARG GTK_VER=4.22.4
ARG REL=v3.1.3
RUN arch="$(dpkg --print-architecture)" \
 && wget -O /tmp/gtk.deb "https://github.com/droserasprout/gtk-brotway/releases/download/${REL}/gtk4-brotway_${GTK_VER}-${REL#v}_${arch}.deb" \
 && apt-get install -y /tmp/gtk.deb \
 && rm /tmp/gtk.deb
ENV LD_LIBRARY_PATH=/usr/lib/gtk4-brotway
```

## Running the container

Two processes run, both loading the fork via `LD_LIBRARY_PATH`: `gtk4-broadwayd :N` owns the display and serves the page on `8080 + N`, and the app renders into it with `GDK_BACKEND=broadway BROTWAY_DISPLAY=:N`. The launcher wires both up:

```sh
gtk4-brotway-run gtk4-widget-factory   # -> http://localhost:8085
```

Process layout, ports, and the TLS/WebSocket proxy needed to expose a session: [Security model](security.md#in-a-container). Launcher flags and by-hand setup: [Running](running.md#the-launcher).
