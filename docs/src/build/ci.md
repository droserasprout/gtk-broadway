# CI & packaging

CI lives on the orphan **`ci`** branch as two workflows. `.github/workflows/build.yml` is a reusable workflow that compiles one base across both arches and assembles the `.deb` as a workflow artifact, with no publishing. `.github/workflows/release.yml` is the orchestrator: it builds every base and collects all the artifacts into one GitHub Release (see [Release process](release.md)).

## The reusable build (`build.yml`)

`workflow_call` takes four inputs: `ref` (the fork branch/tag/SHA to build), `gtk` (e.g. `4.14.5`, used in the `.deb` name and caches), `container` (the Ubuntu image), and `demos` (the per-version flag, [`-Ddemos=false` vs `-Dbuild-demos=false`](from-source.md#version-specific-flag)).

Each arch builds natively inside the matching Ubuntu container, which keeps the binary ABI-compatible with that Ubuntu's apt GTK:

| arch  | runner            | multiarch triplet     |
|-------|-------------------|-----------------------|
| amd64 | `ubuntu-24.04`    | `x86_64-linux-gnu`    |
| arm64 | `ubuntu-24.04-arm`| `aarch64-linux-gnu`   |

`ccache` is restored per `(gtk, arch)`, so unchanged `.c` files relink without recompiling. The build step runs the [Broadway-only Meson config](from-source.md).

## Assembling the `.deb`

The package is built by hand from the Meson output, with no `dh`/`debhelper` involved:

The SONAME comes straight from the build output via `find _build/gtk -name 'libgtk-4.so.1.*.*'`, so a GTK point-release bump needs no edit. The script lays out `usr/lib/<multiarch>/<soname>`, re-points the `libgtk-4.so.1` symlink at it, and adds `usr/bin/gtk4-broadwayd`. The `control` file sets `Package: gtk4-broadway-fork` and lists `libgtk-4-1, libgtk-4-bin` under both `Depends` and `Replaces`, so the package overlays the stock runtime files and leaves the rest of GTK alone. `postinst` is just `ldconfig`. Everything is built with `dpkg-deb --root-owner-group --build` and uploaded as artifact `deb-<gtk>-<arch>`.

Version stamping: a `vN` tag gives revision `N`; everything else gets `0+r<run_number>` for rolling builds. The artifact keeps the stable unversioned per-base name `gtk4-broadway-fork_<gtk>_<arch>.deb`, and the release job derives the version-stamped name from it.

## Per-branch validation

Each fork branch can call `build.yml` from a small `ci.yml` stub for a single-target validation build, so a feature branch gets checked before it's merged or tagged.
