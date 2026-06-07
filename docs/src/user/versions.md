# Supported versions

Two GTK bases are maintained in parallel. Each tracks an upstream GTK stable branch and adds
the same fork feature set on top, differing only in version-specific build configuration.

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

## Branch layout in the fork repo

- `4.14.5-fork`, `4.22.2-fork` - the two maintained tips; each is upstream GTK + the fork commits.
- `ci` - an orphan branch holding the release orchestration (`.github/workflows/`), the
  `README`/`CHANGELOG`, and **this book** (`docs/`). It contains no GTK source tree.
- Per-feature branches (e.g. `4.22-connection-management`) are the working branches each feature
  was developed on before being merged into a `*-fork` tip.
