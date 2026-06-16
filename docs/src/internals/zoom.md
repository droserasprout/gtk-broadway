# Pinch zoom implementation

The user-facing summary is in [Pinch to zoom](../features/input.md#pinch-to-zoom). This page covers the mechanism. It is entirely [broadwayd-only](../build/from-source.md#iterating-broadwayd-vs-libgtk) (`broadway.js` / `client.html`).

## Reflow, not the native scale

Broadway's native `scale` only does integer HiDPI crispness; it sharpens rather than resizes. Desktop Ctrl+zoom stays crisp because the browser changes both `devicePixelRatio` and `innerWidth/Height`, and `sendScreenSizeChanged` forwards those so GTK re-lays-out smaller at a higher scale. The fork replicates that for touch.

## Mechanism

Everything hangs off a per-client `zoomFactor` Z, clamped to `0.25 .. 5.0`:

1. `sendScreenSizeChanged` reports `w = round(innerWidth/Z)`, `h = round(innerHeight/Z)`, `s = max(1, round(devicePixelRatio*Z))` -> GTK reflows and renders crisp.
2. A `#zoomRoot` wrapper holds all toplevel surfaces (the offscreen clipboard/OSK helpers stay on `body`, unscaled) and is magnified by `transform: scale(Z)`.
3. Steady state is self-consistent: visual width `= (innerWidth/Z) * Z = innerWidth`, filling the viewport with no pan or letterbox.

Coordinate remap is a single divide-by-Z at `getPositionsFromEvent`, covering mouse, wheel, and touch.

## Live preview, then crisp reflow

During the pinch only the wrapper transform updates live (so it goes briefly soft); the reported size and scale that drive the reflow commit once on touch-end. `applyPinchPreview` pins the layout point under the pinch-start midpoint to the current midpoint, so the magnified view follows the fingers; `endPinch` resets to a plain `scale(Z)` as the reflow fills the viewport.

## No tap-leak on pinch

When the second finger lands, mobile Firefox fires `touchcancel` on the first finger; the client used to forward that as a type-2 end, so GTK read begin->end as a tap and activated the widget. The fix forwards touch type 3 (`GDK_TOUCH_CANCEL`) for a `touchcancel`, type 2 only for a real `touchend`, backed by a pinch state machine (`suppressTouchForward` latched until all fingers lift). Mobile Chrome fires no `touchcancel`, so there `beginPinch` aborts the first finger's in-flight tap via `endInFlightGtkTouch`.

## Desktop no-op, persisted zoom

At `Z=1` every path is byte-identical (`/1`, `*1`, empty transform), so desktop keeps the browser's native Ctrl+scroll zoom. The committed zoom is saved per origin in `localStorage` (`broadwayZoom`) and restored before the first `sendScreenSizeChanged()`, so a reload lays out at the saved zoom from the first frame. See [Configuration reference](../guide/config.md#localstorage-keys).
