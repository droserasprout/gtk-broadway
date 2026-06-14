# packaging

Distro packaging for the brotway GTK fork.

| Dir | What | Used by |
|-----|------|---------|
| `debian/` | `.deb` control template (`control.in`, filled by `envsubst`) + `postinst` | `.github/workflows/build.yml` (sparse-checked-out into the build, assembles `gtk4-brotway_*.deb`) |
| `arch/` | `PKGBUILD` for a local Arch/CachyOS build | manual (`makepkg -si`), **not** wired into CI |
| `docker/` | `Dockerfile` for the app-agnostic base image (stock Ubuntu GTK runtime + the `.deb` installed into its private prefix, `LD_LIBRARY_PATH` pre-set) | `.github/workflows/image.yml` (downloads the release `.debs`, pushes `ghcr.io/<owner>/gtk-brotway`) |

Both paths install the fork conflict-free into the private prefix `/usr/lib/gtk4-brotway`
(never replacing the system GTK). The Debian path is the shipped artifact; the Arch
path builds from source for local dev - see `arch/PKGBUILD`.
