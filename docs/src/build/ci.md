# CI & packaging

CI lives on the orphan **`ci`** branch. `.github/workflows/build.yml` is a reusable workflow that compiles the base across both arches and assembles the `.deb` as a workflow artifact, with no publishing. `.github/workflows/release.yml` is the orchestrator: it builds the base and collects the artifacts into one GitHub Release (see [Release process](release.md)). `.github/workflows/image.yml` then bakes those `.deb`s into the [base Docker image](#base-docker-image) on GHCR.

## The reusable build (`build.yml`)

`workflow_call` takes four inputs: `ref` (the fork branch/tag/SHA to build), `gtk` (e.g. `4.22.4`, used in the `.deb` name and caches), `container` (the Ubuntu image), and `demos` (the [`-Dbuild-demos=false`](from-source.md) flag).

Each arch builds natively inside the matching Ubuntu container, which keeps the binary ABI-compatible with that Ubuntu's apt GTK:

| arch  | runner            | multiarch triplet     |
|-------|-------------------|-----------------------|
| amd64 | `ubuntu-24.04`    | `x86_64-linux-gnu`    |
| arm64 | `ubuntu-24.04-arm`| `aarch64-linux-gnu`   |

`ccache` is restored per `(gtk, arch)`, so unchanged `.c` files relink without recompiling. The build step runs the [Broadway-only Meson config](from-source.md).

## Assembling the `.deb`

The package is built by hand from the Meson output, with no `dh`/`debhelper` involved.

The SONAME comes straight from the build output via `find _build/gtk -name 'libgtk-4.so.1.*.*'`, so a GTK point-release bump needs no edit. The script lays the fork lib + `libgtk-4.so.1` symlink + `gtk4-broadwayd` under the private prefix `usr/lib/gtk4-brotway`, and puts `gtk4-brotway-run` + `gtk4-brotway-debugmenu` in `usr/bin`. The `control` file sets `Package: gtk4-brotway` and lists `libgtk-4-1, libgtk-4-bin` under `Depends` only (no `Replaces`/`Conflicts`) - the fork sits beside the stock runtime in its prefix, so nothing is overlaid and the package is desktop-safe. `postinst` is just `ldconfig`. Everything is built with `dpkg-deb --root-owner-group --build` and uploaded as artifact `deb-<gtk>-<arch>`.

Version stamping: a `vX.Y.Z` tag gives revision `X.Y.Z`; everything else gets `0+r<run_number>` for rolling builds. The artifact keeps the stable unversioned name `gtk4-brotway_<gtk>_<arch>.deb`, and the release job derives the version-stamped name from it.

## Base Docker image

`image.yml` publishes the app-agnostic base image `ghcr.io/<owner>/gtk-brotway` - a stock Ubuntu GTK runtime with the fork `.deb` installed into its prefix and `LD_LIBRARY_PATH` pointed at it, so every app in the container uses the fork (`packaging/docker/Dockerfile`). It's the same private-prefix install as a host install, with the launcher's per-process `LD_LIBRARY_PATH` made global for the layer; `FROM` it for any GTK4 Broadway binary.

It triggers on a `vN` tag (and `workflow_dispatch` with a `tag` input for backfills). The inputs are the release's per-arch `.deb`s, so on a tag push it races `release.yml` and retries `gh release download` until both assets land. The build is multi-arch via `buildx` + QEMU; the Dockerfile picks the deb for the emulated target arch with `dpkg --print-architecture`. Tags pushed: the `vN` tag plus `latest` (the latter only on a real tag push, never a manual backfill).

## Per-branch validation

Each fork branch can call `build.yml` from a small `ci.yml` stub for a single-target validation build, so a feature branch gets checked before it's merged or tagged.
