# Pinch zoom implementation

The user-facing summary is in [Pinch to zoom](../features/zoom.md). This page covers the mechanism. It is entirely [broadwayd-only](build-split.md) (`broadway.js` / `client.html`).

## Reflow, not the native scale

Broadway's native `scale` only does integer HiDPI crispness (`round(devicePixelRatio)`); it sharpens rather than resizes. Desktop Ctrl+zoom stays crisp because the browser changes both `devicePixelRatio` and `innerWidth/Height`. `sendScreenSizeChanged` forwards those, so GTK re-lays-out smaller and renders at a higher scale. The fork replicates that for touch.

## Mechanism

Everything hangs off a per-client `zoomFactor` Z, clamped to `ZOOM_MIN 0.25` .. `ZOOM_MAX 5.0`:

1. `sendScreenSizeChanged` reports `w = round(innerWidth/Z)`, `h = round(innerHeight/Z)`, `s = max(1, round(devicePixelRatio*Z))` -> GTK reflows and renders crisp.
2. A `#zoomRoot` wrapper (in `client.html`) holds all toplevel surfaces (`APPEND_ROOT` appends there, not to `document.body`; the offscreen clipboard/OSK helpers stay on `body`, unscaled) and is magnified by `transform: scale(Z)` (`transform-origin: 0 0`).
3. Steady state is self-consistent: visual width `= (innerWidth/Z) * Z = innerWidth`, so it fills the viewport with no pan or letterbox.

Coordinate remap is a single divide-by-Z at `getPositionsFromEvent` (`absX = ev.pageX / Z`), covering mouse, wheel, and touch. Grab/ungrab and `REASSERT_POINTER` already use logical coords.

## Live preview, then crisp reflow

During the pinch only the wrapper transform updates live (so it goes briefly soft); the reported size and scale, which is what drives the GTK reflow, commits once on touch-end (`endPinch`). `applyPinchPreview` makes the live transform `translate(t) scale(Z)` pin the layout point under the pinch-start midpoint to the current midpoint, so the magnified view follows the fingers instead of anchoring top-left. `endPinch` resets to a plain `scale(Z)` as the reflow fills the viewport.

## No tap-leak on pinch

When the second finger lands, mobile Firefox fires `touchcancel` on the first finger. The client routed `touchcancel -> onTouchEnd`, which forwarded a type-2 end, so GTK read begin->end as a tap and activated the widget under the fingers. The fix: in `onTouchEnd`, a `touchcancel` now forwards touch type 3 == `GDK_TOUCH_CANCEL` instead of an end, while a genuine `touchend` still sends type 2. The pinch state machine (`activeTouches`, `beginPinch`, `suppressTouchForward` latched until all fingers lift) backs this up.

## Desktop no-op, persisted zoom

At `Z=1` every path is byte-identical (`/1`, `*1`, empty transform), so desktop keeps the browser's native Ctrl+scroll zoom. The committed zoom is saved per origin in `localStorage` (`broadwayZoom`) and restored in `start()` before the first `sendScreenSizeChanged()`, so a reload lays out at the saved zoom from the first frame. See [Configuration reference](../guide/config.md#localstorage-keys).
