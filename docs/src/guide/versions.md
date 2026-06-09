# Supported versions

Two GTK bases are maintained in parallel. Each tracks an upstream GTK stable branch and carries
the same fork features on top. They differ only in version-specific build configuration and the
backend details noted below.

| GTK    | Ubuntu base    | SONAME                 | Fork branch    | Diff vs stock |
| ------ | -------------- | ---------------------- | -------------- | ------------- |
| 4.14.5 | `ubuntu:24.04` | `libgtk-4.so.1.1400.5` | [`4.14.5-fork`](https://github.com/droserasprout/gtk-broadway/tree/4.14.5-fork) | [compare](https://github.com/droserasprout/gtk-broadway/compare/4.14.5...4.14.5-fork) |
| 4.22.2 | `ubuntu:26.04` | `libgtk-4.so.1.2200.2` | [`4.22.2-fork`](https://github.com/droserasprout/gtk-broadway/tree/4.22.2-fork) | [compare](https://github.com/droserasprout/gtk-broadway/compare/4.22.2...4.22.2-fork) |

**Supported architectures:** `amd64`, `arm64`.

## Release versioning

Not semver. Each GitHub release is tagged `vA[.B]` - a fork revision counter, where `A`
increments per release and the optional `.B` is a packaging/point release (e.g. `v2.1`). A
release publishes per-base debs named `X.Y.Z-A[.B]`: `X.Y.Z` is the upstream GTK base
(`4.14.5`, `4.22.2`) and `A[.B]` the fork revision. So `gtk4-broadway-fork_4.22.2-2.1` is the
GTK 4.22.2 base at fork revision 2.1.

## Picking a base

Pick the base that matches the GTK already installed in your target environment. The `.deb`
overlays the existing `libgtk-4.so`, replacing the SONAME-versioned `.so` in place, so the
versions have to line up. Install the **4.14.5** build on an `ubuntu:24.04`-derived system and the
**4.22.2** build on `ubuntu:26.04`.

The SONAME (`libgtk-4.so.1.1400.5` / `libgtk-4.so.1.2200.2`) is the file the package ships and
re-points `libgtk-4.so.1` at. Packaging reads it from the actual build output, so a GTK
point-release bump needs no manual edits.

## 4.14 vs 4.22 in the Broadway backend

Both tips carry the same features. What differs is the GTK they patch.

A few API shims diverge. 4.14 calls `gdk_draw_context_end_frame()`, 4.22 calls
`gdk_draw_context_end_frame_full(..., NULL)`. 4.22 splits node accessors into per-type headers
(`gsktextnode.h`) and has `GdkColorState` (GTK 4.16+), where 4.14 declares them in
`gskrendernode.h`. Texture downloads normalize to the same default format on both, so the
[content dedup](../internals/performance.md) is identical.

TreeView hover behaves differently. 4.22 invalidates a `GtkTreeView` granularly on prelight,
while 4.14 re-snapshots the whole list on pointer motion. Under Broadway, 4.14 re-sends more node
commands while you hover a list. Node/texture reuse still serves them from cache without extra
uploads, but the wire churn is higher on 4.14.

The build bases differ too: 4.14 on `ubuntu:24.04`, 4.22 on `ubuntu:26.04`. Both ship Cairo 1.18
and FreeType 2.13, new enough for server-side color-emoji rasterization.

## Branch layout in the fork repo

- `4.14.5-fork`, `4.22.2-fork`: the two maintained tips, each upstream GTK plus the fork commits.
- `ci`: an orphan branch holding the release orchestration (`.github/workflows/`), the
  `README`/`CHANGELOG`, and this book (`docs/`). It contains no GTK source tree.
- Per-feature branches (e.g. `4.22-connection-management`): the working branches each feature was
  developed on before being merged into a `*-fork` tip.
