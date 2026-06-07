# Supported versions

Two GTK bases are maintained in parallel. Each tracks an upstream GTK stable branch and adds
the same fork feature set on top, differing in version-specific build configuration and the
backend details noted below.

| GTK    | Ubuntu base    | SONAME                 | Fork branch    | Diff vs stock |
| ------ | -------------- | ---------------------- | -------------- | ------------- |
| 4.14.5 | `ubuntu:24.04` | `libgtk-4.so.1.1400.5` | [`4.14.5-fork`](https://github.com/droserasprout/gtk-broadway/tree/4.14.5-fork) | [compare](https://github.com/droserasprout/gtk-broadway/compare/4.14.5...4.14.5-fork) |
| 4.22.2 | `ubuntu:26.04` | `libgtk-4.so.1.2200.2` | [`4.22.2-fork`](https://github.com/droserasprout/gtk-broadway/tree/4.22.2-fork) | [compare](https://github.com/droserasprout/gtk-broadway/compare/4.22.2...4.22.2-fork) |

**Supported architectures:** `amd64`, `arm64`.

## Picking a base

- The base must match the GTK already installed in your target environment, because the `.deb`
  overlays the existing `libgtk-4.so` (it replaces the SONAME-versioned `.so` in place). Install
  the **4.14.5** build on an `ubuntu:24.04`-derived system and the **4.22.2** build on
  `ubuntu:26.04`.
- The SONAME (`libgtk-4.so.1.1400.5` / `libgtk-4.so.1.2200.2`) is the file the package ships and
  re-points `libgtk-4.so.1` at. It is read from the actual build output during packaging, so a
  GTK point-release bump needs no manual edits.

## 4.14 vs 4.22 in the Broadway backend

Both tips carry the same features; what differs is the GTK they patch:

- **A few API shims.** `gdk_draw_context_end_frame()` (4.14) vs
  `gdk_draw_context_end_frame_full(..., NULL)` (4.22); 4.22 splits node accessors into per-type
  headers (`gsktextnode.h`) and has `GdkColorState` (GTK 4.16+), where 4.14 declares them in
  `gskrendernode.h`. Texture downloads normalize to the same default format on both, so the
  [content dedup](../internals/performance.md) is identical.
- **TreeView hover churn.** 4.22 invalidates a `GtkTreeView` granularly on prelight; 4.14
  re-snapshots the whole list on pointer motion. Under Broadway, 4.14 re-sends more node commands
  while hovering a list - the node/texture reuse serves them from cache (no extra uploads), but
  the wire churn is higher on 4.14.
- **Build base.** 4.14 on `ubuntu:24.04`, 4.22 on `ubuntu:26.04`; both ship Cairo 1.18 /
  FreeType 2.13, new enough for server-side color-emoji rasterization.

## Branch layout in the fork repo

- `4.14.5-fork`, `4.22.2-fork` - the two maintained tips; each is upstream GTK + the fork commits.
- `ci` - an orphan branch holding the release orchestration (`.github/workflows/`), the
  `README`/`CHANGELOG`, and **this book** (`docs/`). It contains no GTK source tree.
- Per-feature branches (e.g. `4.22-connection-management`) are the working branches each feature
  was developed on before being merged into a `*-fork` tip.
