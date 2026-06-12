# Release process

A release is one immutable GitHub Release holding the `.deb` for each arch. You cut it by pushing a `vX.Y.Z` tag to the `ci` branch, which triggers `release.yml`.

## Before tagging

Run the [Release QA](qa.md) pass against the candidate build.

Label and milestone every merged PR going into the release: the change-type label (`bug`/`enhancement`/...) and the target `vX.Y.Z` milestone. The milestone is the manifest of what shipped; the CHANGELOG `vX.Y.Z` section (release-notes source) is cross-checked against it.

## Tagging a release

The build runs from a pinned per-version tag `4.22.4-<X.Y.Z>`. Create that first, then push the orchestrator `vX.Y.Z` tag to trigger the build. For `v3.0.0`:

```sh
git tag 4.22.4-3.0.0 <commit-on-4.22.4-brotway> && git push origin 4.22.4-3.0.0
git tag v3.0.0 ci                               && git push origin v3.0.0
```

Pinning to a code tag means the release records exactly which fork commit each `.deb` was built from. A missing `4.22.4-<X.Y.Z>` tag fails checkout in `build.yml`, so the release won't publish a partial set.

## What `release.yml` does

1. **setup**: derive `rev` (`3.0.0` from `v3.0.0`).
2. **build**: a matrix over the arches (`4.22.4` on `ubuntu:26.04`) calls the reusable [`build.yml`](ci.md), building the pinned `4.22.4-<X.Y.Z>` tag for both arches. That's 2 artifacts: 1 base x 2 arches.
3. **release**: download all `deb-*` artifacts, version-stamp each to `gtk4-brotway_<gtk>-<rev>_<arch>.deb`, create the `vX.Y.Z` release if needed, and upload both (`--clobber`).

The result is what [Installation](../guide/installation.md) downloads, e.g. `gtk4-brotway_4.22.4-3.0.0_amd64.deb`.

## Version scheme

Release tags use a three-part `vX.Y.Z` form. It's the semver *format* with loose discipline and no compatibility contract - this is a solo fork, so the numbers are a rough changelog ordering, not an API promise. So `v3.0.0` produces `gtk4-brotway_4.22.4-3.0.0` from the pinned `4.22.4-3.0.0` code tag.
