# Supported versions

Two GTK bases are maintained in parallel: **4.14.5** and **4.22.2**. Each tracks an upstream GTK stable branch and carries the **same fork features**, with the same support. They differ only in version-specific build configuration.

**Supported architectures:** `amd64`, `arm64`.

## Which to choose? 4.22.2

Prefer **4.22.2**. It performs slightly better for the Broadway/WebUI goal because of how it invalidates a `GtkTreeView`. 4.14 re-snapshots the whole list on hover or relayout, producing more render-node churn over the WebSocket; 4.22 invalidates granularly and re-sends far fewer node commands. Node/texture reuse still serves the extra commands from cache without re-uploads (see [Rendering & performance](../internals/performance.md)), so the difference is wire churn, not pixels. Feature-wise the two bases are identical.

## Install matrix

| GTK    | Ubuntu base    |
| ------ | -------------- |
| 4.14.5 | `ubuntu:24.04` |
| 4.22.2 | `ubuntu:26.04` |

Pick the build that matches the GTK already in your target environment. See [Installation](installation.md) for steps.
