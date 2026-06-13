# packaging

Distro packaging for the brotway GTK fork.

| Dir | What | Used by |
|-----|------|---------|
| `debian/` | `.deb` control template (`control.in`, filled by `envsubst`) + `postinst` | `.github/workflows/build.yml` (sparse-checked-out into the build, assembles `gtk4-brotway_*.deb`) |
| `arch/` | `PKGBUILD` + `gtk4-brotway-run` launcher for a local Arch/CachyOS build | manual (`makepkg -si`), **not** wired into CI |
| `docker/` | `Dockerfile` for the app-agnostic base image (stock Ubuntu GTK runtime + the `.deb` overlaid & held) | `.github/workflows/image.yml` (downloads the release `.debs`, pushes `ghcr.io/<owner>/gtk-brotway`) |

The Debian path is the shipped artifact (overlays apt's `libgtk-4-1`). The Arch
path is a conflict-free private-prefix overlay for local dev - see `arch/PKGBUILD`.
