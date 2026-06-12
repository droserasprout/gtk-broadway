# Re-forking a new GTK release

The fork is a **thin layer**: the patches touch only the Broadway backend, never app-facing GTK API, and stay close to upstream so each GTK4 point release can be re-forked with a minimal diff. This page is how to carry the fork onto a new upstream GTK tag.

## The boundary: GTK4 only

Broadway (and X11) are deprecated since GTK 4.17.4 and removed entirely in GTK5, so the whole stack is anchored to the GTK 4.x lifetime. **Do not port the backend patches to GTK5.** Keep maintaining the 4.x fork; it lives only as long as GTK4 itself.

## Branch model

The fork repo holds, per [Supported versions](../guide/versions.md):

- **`4.22.4-brotway`** - upstream GTK at that tag plus the fork commits. This is what releases build from.
- **Per-feature branches** (e.g. `connection-management`) - where each feature was developed before merging into the fork tip.
- **`ci`** - an orphan branch with the release orchestration (`.github/workflows/`), the `README`/`CHANGELOG`, and this book (`docs/`). No GTK source tree.

## Re-forking onto a new point release

To move the base from, say, `4.22.4` to a new `4.22.x` (e.g. `4.22.6`):

1. **Fetch the upstream tag** into the fork repo (upstream remains the `gnome` remote).
2. **Rebase the fork commits** from the current `4.22.4-brotway` tip onto the new tag. Because the patch set is confined to `gdk/broadway/`, `gsk/broadway/`, and a handful of widget files, conflicts are usually small and localized.
3. **Resolve API drift** (below) where upstream moved a signature or header.
4. **Build Broadway-only** ([from source](from-source.md)) and smoke-test in a browser.
5. **Tag the new tip** (e.g. `4.22.6-brotway`) and cut a [release](release.md). The SONAME is read from the build output, so a point-release bump needs no manual version edit.

A new *minor* base (e.g. standing up 4.24) is the same, but expect more API drift and a new Ubuntu base image to match its apt GTK.

## API drift to watch on a base bump

Upstream occasionally moves a signature or header between point releases. The first things to check when rebasing onto a newer base:

- **Frame end:** which `gdk_draw_context_end_frame*` signature the renderer calls (4.22 uses `gdk_draw_context_end_frame_full(..., NULL)`).
- **Node accessors:** whether render-node getters live in per-type headers (`gsktextnode.h`) or the umbrella `gskrendernode.h`.
- **Color state:** `GdkColorState` exists from GTK 4.16+.

## Keeping the diff minimal

- Touch only the Broadway backend and the minimum widget code; never app-facing GTK API. That keeps the rebase tractable and lets stock GTK supply everything else.
- Append new wire enum values at the end so numbers never shift across re-forks; see [Wire protocol](../internals/protocol.md).
