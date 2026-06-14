# Touch interface

<img src="../images/touch-kb.png" alt="Touch text editing, taps, and gestures on Broadway"
    style="max-width:300px;float:right;border-radius:6px">

Stock Broadway treats a touchscreen as a mouse, so GTK's touch text-editing UI never appears and many taps get dropped. The fork sources real touchscreen events and makes touch first-class input.

## What works

- **Touch text editing** - selection handles and the Cut/Copy/Paste bubble.
- **On-screen keyboard** shows and hides on Android.
- **Touch text input** including non-Latin and IME.
- **Tap to dissmiss** popover, menu, or selection bubble.
- **Gestures survive repaints** - a scroll or drag keeps working after the UI re-renders
- **No spurious hover** styling or tooltips on a tap.

## Touch bugfixes

- The selection bubble's Cut/Copy/Paste fire, instead of the bubble being dismissed first.
- Menus and dropdowns no longer freeze on tap.
- Dropdowns select the row you tapped, not always the first.
- No crash when reopening the selection bubble.
- Copy reappears in the bubble after Select-All.
- Taps on Android Chrome no longer leave a stuck touch grab - begin and end now send the same remapped touch id.
- Backspace and Delete work from the Android OSK (GBoard reports them only as `beforeinput`).
- A tap can't activate a widget beneath an open popup - touch follows the pointer grab now.
- The bubble dismisses when a tap collapses the selection, and a handle drag can't empty it (keeps one character, like GtkText).
- Tapping an already-selected row in a multi-select list collapses the selection to it on release.

## Limitations

- The first tap can leak into a pinch or pan when a second finger lands; no fix yet without adding input latency. See [pinch zoom implementation](../internals/zoom.md#no-tap-leak-on-pinch) for the related zoom case.

> Per-fix mechanics - device sourcing, passive-listener/`touchcancel` handling, OSK/IME plumbing, popover/menu/dropdown fixes: [Touch implementation](../internals/touch.md).
