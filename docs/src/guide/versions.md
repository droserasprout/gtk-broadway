# Supported versions

One GTK base: a pinned upstream GTK stable tag plus the fork patches.

| GTK    | Ubuntu base    | SONAME                 | Fork branch | Patch |
| ------ | -------------- | ---------------------- | ----------- | ----- |
| 4.22.4 | `ubuntu:26.04` | `libgtk-4.so.1.2200.4` | [`4.22.4-brotway`](https://github.com/droserasprout/gtk-brotway/tree/4.22.4-brotway) | [diff](https://github.com/droserasprout/gtk-brotway/compare/4.22.4...4.22.4-brotway) |

**Supported architectures:** `amd64`, `arm64`.

## Matching your environment

The base GTK must match the `.deb` - the `.deb` overlays the SONAME-versioned `.so` in place, so package base and system GTK must agree. Run on `ubuntu:26.04` (GTK 4.22.x). See [Installation](installation.md).

## Older GTK

The fork targets a single base, GTK 4.22.4. **Other GTK versions are not supported.** Older bases (4.14 and down to 4.6 on `ubuntu:22.04`, 4.8 on `debian:12`) predate or diverge from the Broadway renderer changes the fork patches against, so backports are high-conflict for little gain. The last 4.14 build was v2.1.
