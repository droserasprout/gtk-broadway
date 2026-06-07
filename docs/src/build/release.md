# Release process

A release is one **immutable GitHub Release** holding every base x arch `.deb`. It is cut by
pushing a `vN` tag to the `ci` branch, which triggers `release.yml`.

## Tagging a release

Each base is built from a **pinned per-version tag** `<gtk>-N`, which must be created first, then
the orchestrator `vN` tag triggers the build. For `v123`:

```sh
git tag 4.14.5-123 <commit-on-4.14.5-fork> && git push origin 4.14.5-123
git tag 4.22.2-123 <commit-on-4.22.2-fork> && git push origin 4.22.2-123
git tag v123 ci                            && git push origin v123
```

Pinning each base to its own tag means the release records exactly which fork commit each `.deb`
was built from. A missing `<gtk>-N` tag fails checkout in `build.yml`, so the release won't
publish a partial set.

## What `release.yml` does

1. **setup** - derive `rev` (`123` from `v123`).
2. **build** - a matrix over the bases (`4.14.5` on `ubuntu:24.04`, `4.22.2` on `ubuntu:26.04`,
   each with its [demos flag](from-source.md#version-specific-flag)) calls the reusable
   [`build.yml`](ci.md), building the pinned `<gtk>-N` tag for both arches. That's **4 artifacts**:
   2 bases x 2 arches.
3. **release** - download all `deb-*` artifacts, version-stamp each to
   `gtk4-broadway-fork_<gtk>-<rev>_<arch>.deb`, create the `vN` release if needed, and upload all
   four (`--clobber`).

The result is what [Installation](../user/installation.md) downloads, e.g.
`gtk4-broadway-fork_4.22.2-123_amd64.deb`.

## Version scheme

The revision `N` is a single rolling number **shared across both bases** for a given release - it
is independent of the GTK version. So `v123` produces `4.14.5-123` and `4.22.2-123` together: same
feature set, two GTK bases.
