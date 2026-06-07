# Pinch to zoom

Two-finger pinch zooms the whole UI (0.25x-5x) with a real re-layout and re-render, the same as
desktop Firefox's Ctrl+scroll page-zoom. Mobile browsers refuse to page-zoom a `user-scalable=no`
page, so the fork drives it manually.

*TODO: add screenshot*

## Reflow, not the native scale

Broadway's native `scale` is integer HiDPI crispness only (`round(devicePixelRatio)`) - it sharpens
rather than resizes. Desktop Ctrl+zoom is crisp because the browser changes both `devicePixelRatio`
and `innerWidth/Height`, which `sendScreenSizeChanged` forwards, so GTK re-lays-out smaller and
renders at a higher scale. The fork replicates that for touch.

## Mechanism

A per-client `zoomFactor` Z (clamped `ZOOM_MIN 0.25` .. `ZOOM_MAX 5.0`):

1. `sendScreenSizeChanged` reports `w = round(innerWidth/Z)`, `h = round(innerHeight/Z)`,
   `s = max(1, round(devicePixelRatio*Z))` -> GTK reflows and renders crisp.
2. A `#zoomRoot` wrapper (in `client.html`) holds all toplevel surfaces (`APPEND_ROOT` appends
   there, not to `document.body`; the offscreen clipboard/OSK helpers stay on `body`, unscaled) and
   is magnified by `transform: scale(Z)` (`transform-origin: 0 0`).
3. Steady state is self-consistent: visual width `= (innerWidth/Z) * Z = innerWidth`, so it fills
   the viewport with no pan or letterbox.

Coordinate remap is a single divide-by-Z at `getPositionsFromEvent` (`absX = ev.pageX / Z`),
covering mouse, wheel, and touch. Grab/ungrab and `REASSERT_POINTER` already use logical coords.

## Live preview, then crisp reflow

During the pinch only the wrapper transform updates live (briefly soft); the reported size/scale -
the GTK reflow - commits once on touch-end (`endPinch`). `applyPinchPreview` makes the live
transform `translate(t) scale(Z)` pin the layout point under the pinch-start midpoint to the current
midpoint, so the magnified view follows the fingers instead of anchoring top-left. `endPinch` resets
to a plain `scale(Z)` as the reflow fills the viewport.

## No tap-leak on pinch

When the second finger lands, mobile Firefox fires `touchcancel` on the first finger, and the client
routed `touchcancel -> onTouchEnd`, which forwarded a type-2 end - GTK read begin->end as a tap that
activated the widget under the fingers. Fix: in `onTouchEnd`, a `touchcancel` forwards touch type 3
== `GDK_TOUCH_CANCEL` instead of an end; a genuine `touchend` still sends type 2. The pinch state
machine (`activeTouches`, `beginPinch`, `suppressTouchForward` latched until all fingers lift) backs
this up.

## Desktop no-op, persisted zoom

At `Z=1` every path is byte-identical (`/1`, `*1`, empty transform), so desktop keeps the browser's
native Ctrl+scroll zoom. The committed zoom is saved per origin in `localStorage` (`broadwayZoom`)
and restored in `start()` before the first `sendScreenSizeChanged()`, so a reload lays out at the
saved zoom from the first frame.
