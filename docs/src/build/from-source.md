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

A **broadwayd-only** change (`broadway.js`, `client.html`, or daemon C) is the fast loop: restart the daemon, reload the tab. The client files are embedded into the daemon (generated into `clienthtml.h` / `broadwayjs.h`), so even a pure JS change needs the daemon rebuilt - but `ninja` does that incrementally and the page is served `no-store`, so a plain reload picks it up. A **libgtk** change (GDK backend, GSK renderer, or a GTK widget) is the slow loop: rebuild `libgtk-4.so` and restart the app.

The `gtk4-brotway` `.deb` ships both the patched `libgtk-4.so` and `gtk4-broadwayd`, so a single release always covers both kinds of change.

## Build dependencies

The Broadway-only build needs the GTK build toolchain and these `-dev` libraries (the CI list, for Ubuntu):

```
build-essential meson ninja-build pkg-config gettext python3 git ca-certificates
libglib2.0-dev libglib2.0-dev-bin libgraphene-1.0-dev libcairo2-dev libpango1.0-dev
libgdk-pixbuf-2.0-dev libepoxy-dev libxkbcommon-dev libfribidi-dev libharfbuzz-dev
libjpeg-dev libpng-dev libtiff-dev libdrm-dev
```
