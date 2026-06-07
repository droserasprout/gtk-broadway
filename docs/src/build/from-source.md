# Build from source

The fork is built **Broadway-only** - every other backend and most optional subsystems are turned
off, so the build produces just the patched `libgtk-4.so` plus `gtk4-broadwayd`.

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

## Version-specific flag

The demos flag was renamed between the two bases:

- **4.22.2:** `-Dbuild-demos=false` (as above)
- **4.14.5:** `-Ddemos=false`

This is the only build-config difference between the bases; CI passes it in as an input (see
[CI & packaging](ci.md)).

## What you get

- `_build/gtk/libgtk-4.so.1.<minor>.<micro>` - the patched shared object (e.g.
  `libgtk-4.so.1.2200.2`).
- `_build/gdk/broadway/gtk4-broadwayd` - the daemon.

`ninja -C _build` also regenerates `broadwayjs.h` / `clienthtml.h` from `broadway.js` /
`client.html`, so a client-side change is picked up by an incremental rebuild + a daemon restart.
See [Running broadwayd](../users/running.md) for which changes need only the daemon vs. the full
library.

## Build dependencies

The Broadway-only build needs the GTK build toolchain and these `-dev` libraries (the CI list, for
Ubuntu):

```
build-essential meson ninja-build pkg-config gettext python3 git ca-certificates
libglib2.0-dev libglib2.0-dev-bin libgraphene-1.0-dev libcairo2-dev libpango1.0-dev
libgdk-pixbuf-2.0-dev libepoxy-dev libxkbcommon-dev libfribidi-dev libharfbuzz-dev
libjpeg-dev libpng-dev libtiff-dev libdrm-dev
```
