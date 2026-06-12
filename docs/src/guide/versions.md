# Supported versions

One GTK base, tracking an upstream GTK stable tag plus the fork features.

| GTK    | Ubuntu base    | SONAME                 | Fork branch | Patch |
| ------ | -------------- | ---------------------- | ----------- | ----- |
| 4.22.4 | `ubuntu:26.04` | `libgtk-4.so.1.2200.4` | [`4.22.4-brotway`](https://github.com/droserasprout/gtk-brotway/tree/4.22.4-brotway) | [diff](https://github.com/droserasprout/gtk-brotway/compare/4.22.4...4.22.4-brotway) |

**Supported architectures:** `amd64`, `arm64`.

## Matching your environment

The base GTK must match the `.deb` - the `.deb` overlays the SONAME-versioned `.so` in place, so package base and system GTK must agree. Run on `ubuntu:26.04` (GTK 4.22.x). See [Installation](installation.md).

## Older GTK

GTK **older than 4.14 is not supported and won't be**. Pre-4.14 (4.6 on `ubuntu:22.04`, 4.8 on `debian:12`) predates the Broadway renderer changes the fork patches against, so backports are high-conflict for little gain. Development happens on the single supported base.
