# CI & packaging

CI lives on the orphan **`ci`** branch as two workflows:

- `.github/workflows/build.yml` - a **reusable** workflow that compiles one base across both
  arches and assembles the `.deb` as a workflow artifact. No publishing.
- `.github/workflows/release.yml` - the **orchestrator** that builds every base and collects all
  artifacts into one GitHub Release (see [Release process](release.md)).

## The reusable build (`build.yml`)

`workflow_call` inputs: `ref` (the fork branch/tag/SHA to build), `gtk` (e.g. `4.14.5`, used in the
`.deb` name and caches), `container` (the Ubuntu image), and `demos` (the per-version flag,
[`-Ddemos=false` vs `-Dbuild-demos=false`](from-source.md#version-specific-flag)).

Each arch builds **natively** inside the matching Ubuntu container, so the binary stays
ABI-compatible with that Ubuntu's apt GTK:

| arch  | runner            | multiarch triplet     |
|-------|-------------------|-----------------------|
| amd64 | `ubuntu-24.04`    | `x86_64-linux-gnu`    |
| arm64 | `ubuntu-24.04-arm`| `aarch64-linux-gnu`   |

`ccache` is restored per `(gtk, arch)` so unchanged `.c` files relink without recompiling. The
build step runs the [Broadway-only Meson config](from-source.md).

## Assembling the `.deb`

The package is built by hand from the Meson output (no `dh`/`debhelper`):

- **SONAME is read from the build output** - `find _build/gtk -name 'libgtk-4.so.1.*.*'` - so a GTK
  point-release bump needs no edit.
- Lays out `usr/lib/<multiarch>/<soname>`, the `libgtk-4.so.1` symlink re-pointed at it, and
  `usr/bin/gtk4-broadwayd`.
- `control`: `Package: gtk4-broadway-fork`, `Depends` **and** `Replaces` `libgtk-4-1, libgtk-4-bin`
  (so it overlays the stock runtime files and leaves the rest of GTK alone).
- `postinst`: just `ldconfig`.
- Built with `dpkg-deb --root-owner-group --build` and uploaded as artifact
  `deb-<gtk>-<arch>`.

Version stamping: on a `vN` tag the revision is `N`; otherwise `0+r<run_number>` for rolling
builds. The artifact uses the stable unversioned per-base name
`gtk4-broadway-fork_<gtk>_<arch>.deb`; the release job derives the version-stamped name.

## Per-branch validation

Each fork branch can call `build.yml` from a small `ci.yml` stub for a single-target validation
build, so a feature branch is checked before it's merged or tagged.
