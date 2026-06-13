# Pinch to zoom

<!-- SCREENCAST (pending) - uncomment after recording. See devnotes/2026-06-10-docs-screencasts.md
  Record: make record SCENARIO=tools/local/scenarios/zoom.json OUT=docs/src/images/zoom.mp4

<video src="../images/zoom.mp4" poster="../images/zoom.png"
  autoplay loop muted playsinline style="max-width:100%;border-radius:6px">
  <img src="../images/zoom.png" alt="Two-finger pinch re-rendering the UI at a new scale"
    style="max-width:100%;border-radius:6px">
</video>
-->

Two-finger pinch zooms the whole UI (0.25x-5x) with a real re-layout and re-render, not a stretched bitmap. Mobile browsers refuse to page-zoom a `user-scalable=no` page, so the fork drives it manually.

## What works

- **Two-finger pinch** zooms the whole UI from 0.25x to 5x, re-rendering crisply at the new scale.
- **Floating** - the magnified view follows the fingers during the pinch and sharpens once they lift.
- **Per-client** - zoom is local to each browser; desktop keeps native Ctrl+scroll page-zoom.
- **Persists** across a page refresh - the zoom is remembered per origin.

## Limitations

- When a second finger lands to start a pinch, the first finger's tap can still register; the pinch state machine suppresses the worst case (see [implementation](../internals/zoom.md#no-tap-leak-on-pinch)) but not all of it. See [Known issues](../guide/known-issues.md).

> Reflow, live preview, coordinate remap, and tap-leak suppression: [Pinch zoom implementation](../internals/zoom.md).
