# Dynamic cursor

<!-- SCREENCAST (pending) - uncomment after recording. See devnotes/2026-06-10-docs-screencasts.md
  Record: make record SCENARIO=tools/local/scenarios/cursor.json OUT=docs/src/images/cursor.mp4

<video src="../images/cursor.mp4" poster="../images/cursor.png"
  autoplay loop muted playsinline style="max-width:100%;border-radius:6px">
  <img src="../images/cursor.png" alt="Pointer changing to resize, I-beam, and hand cursors"
    style="max-width:100%;border-radius:6px">
</video>
-->

*New in v3*

Stock Broadway always showed the browser's default arrow. The fork forwards GTK's per-surface cursor to the browser, so the pointer matches what's under it - most visibly the resize arrows at a window's edges.

## What works

- **Resize edges** show the matching resize cursor (`w/e/s/n-resize` on the sides, `se/sw/ne/nw-resize` on the corners) - the `<->` cue that a window can be dragged to resize.
- **Text fields** show the I-beam (`text`).
- **Links** show the hand (`pointer`).
- Anything else GTK asks for by name (grab/grabbing, wait, crosshair, not-allowed, ...) maps straight through.

It applies to every surface - toplevels, dialogs, popups, menus.

## Limitations

This mirrors GTK's cursor *intent* onto the browser's CSS `cursor`. So:

- Only **named** cursors map. A custom image/texture cursor (rare) falls back to the default arrow.
- The glyph, theme, and hotspot are the **browser's** - it renders its own OS cursor for the requested keyword.
- Touch never sends a cursor; it's gated to the mouse pointer.

> The `BROADWAY_OP_SET_CURSOR` op, name-to-CSS mapping, and daemon dedup: [Dynamic cursor implementation](../internals/cursor.md).
