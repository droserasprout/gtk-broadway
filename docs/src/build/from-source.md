# Building from source

The fork is built Broadway-only. Every other backend and most optional subsystems are turned off, so the build produces just the patched `libgtk-4.so` plus `gtk4-broadwayd`.

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

## What you get

- `_build/gtk/libgtk-4.so.1.<minor>.<micro>`, the patched shared object (e.g. `libgtk-4.so.1.2200.4`).
- `_build/gdk/broadway/gtk4-broadwayd`, the daemon.

`ninja -C _build` also regenerates `broadwayjs.h` / `clienthtml.h` from `broadway.js` / `client.html`.

## Iterating: broadwayd vs libgtk

Every fork change is one of two kinds, which decides how you rebuild and reload it:

| Change in... | Rebuild | Restart | Browser |
|---|---|---|---|
| `broadway.js` / `client.html` / daemon C | daemon (`ninja`, incremental) | daemon | plain reload |
| GDK backend / GSK renderer / GTK widget | `libgtk-4.so` | the app | plain reload |

The **broadwayd-only** loop is fast: even a pure JS change needs the daemon rebuilt (the client files are embedded), but `ninja` is incremental and the page is served `no-store`, so a plain reload picks it up. A **libgtk** change is the slow loop: rebuild `libgtk-4.so` and restart the app. The `.deb` ships both, so one release covers both kinds.

## Build dependencies

The Broadway-only build needs the GTK build toolchain and these `-dev` libraries (the CI list, for Ubuntu):

```text
build-essential meson ninja-build pkg-config gettext python3 git ca-certificates
libglib2.0-dev libglib2.0-dev-bin libgraphene-1.0-dev libcairo2-dev libpango1.0-dev
libgdk-pixbuf-2.0-dev libepoxy-dev libxkbcommon-dev libfribidi-dev libharfbuzz-dev
libjpeg-dev libpng-dev libtiff-dev libdrm-dev
```

The release also publishes a base Docker image (and a slim `-nogl` variant) built from these artifacts: [Docker](../guide/docker.md).
