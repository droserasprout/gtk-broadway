# Supported versions

Two GTK bases are maintained in parallel, each tracking an upstream GTK stable tag and carrying the **same fork features**. They differ only in version-specific build configuration.

| GTK    | Ubuntu base    | SONAME                 | Fork branch | Patch |
| ------ | -------------- | ---------------------- | ----------- | ----- |
| 4.14.5 | `ubuntu:24.04` | `libgtk-4.so.1.1400.5` | [`4.14.5-fork`](https://github.com/droserasprout/gtk-brotway/tree/4.14.5-fork) | [diff](https://github.com/droserasprout/gtk-brotway/compare/4.14.5...4.14.5-fork) |
| 4.22.2 | `ubuntu:26.04` | `libgtk-4.so.1.2200.2` | [`4.22.2-fork`](https://github.com/droserasprout/gtk-brotway/tree/4.22.2-fork) | [diff](https://github.com/droserasprout/gtk-brotway/compare/4.22.2...4.22.2-fork) |

**Supported architectures:** `amd64`, `arm64`.

## Which base to pick

Match the GTK already in your target environment - the `.deb` overlays the SONAME-versioned `.so` in place, so package base and system GTK must agree. See [Installation](installation.md).

Given a free choice, prefer **4.22.2**: it invalidates a `GtkTreeView` granularly where 4.14 re-snapshots the whole list on hover or relayout, so it sends far fewer render-node commands over the WebSocket. Node/texture reuse serves the extra commands from cache either way ([Rendering & performance](../internals/performance.md)), so the difference is wire churn, not pixels.

## Older GTK

GTK **older than 4.14 is not supported and won't be**. Pre-4.14 (4.6 on `ubuntu:22.04`, 4.8 on `debian:12`) predates the Broadway renderer changes the fork patches against, so backports are high-conflict for little gain. Development happens on the latest supported base and is mirrored down to 4.14.
