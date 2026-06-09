# Touch interface

Stock Broadway treats a touchscreen as a mouse, so GTK's touch text-editing UI never appears and many taps get dropped. The fork sources real touchscreen events and makes touch a first-class input. Most of it was root-caused live on Android (Firefox over USB, HTTPS).

## What works

- **Touch text editing** - selection handles and the Cut/Copy/Paste bubble that GTK normally hides on Broadway.
- **Reliable taps** - the browser no longer cancels jittery or quick taps.
- **On-screen keyboard** shows and hides on Android, with no one-gesture lag.
- **Text input from touch**, including non-Latin and IME (Cyrillic, CJK, gesture-typing, autocorrect replacements, dead keys).
- **Tap outside** a popover, menu, or selection bubble to dismiss it.
- **Gestures survive repaints** - a scroll or drag keeps working after the UI re-renders.
- **No spurious hover** styling or tooltips on a tap.
- Cursor and selection handles don't block taps behind them (see [Input region](../internals/input-region.md)).

## Touch bugfixes

- The selection bubble's Cut/Copy/Paste fire, instead of the bubble being dismissed first.
- Menus and dropdowns no longer freeze on tap.
- Dropdowns select the row you tapped, not always the first.
- No crash when reopening the selection bubble.
- Copy reappears in the bubble after Select-All.

## Known touch gaps

- The first tap can leak into a pinch or pan when a second finger lands; no fix yet without adding input latency. See [pinch zoom implementation](../internals/zoom.md#no-tap-leak-on-pinch) for the related zoom case.
- The emoji widget is slow and ugly over Broadway.
- Autocorrect *deletions* are deliberately not bridged (to avoid double-deletes).

> The per-fix mechanics - sourcing the touchscreen device, passive-listener and `touchcancel` handling, OSK/IME plumbing, the popover/menu/dropdown fixes - are in [Touch implementation](../internals/touch.md).
