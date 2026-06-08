# Touch interface

Stock Broadway treats a touchscreen as a mouse. The touch-event plumbing exists, but nothing is ever
sourced as a real touchscreen, so GTK's touch text-editing UI stays hidden and many taps get dropped.
Most of the fixes below were root-caused live on Android (Firefox over USB, HTTPS). Each is tagged
**broadwayd-only** (`broadway.js` / `client.html` / daemon C) or **libgtk** (GDK / GTK widgets).

## Source touch as a touchscreen

GTK gates all touch text UI, the selection handles and the Cut/Copy/Paste bubble, on the event device
being `GDK_SOURCE_TOUCHSCREEN` (`gtktext.c`, `gtktextview.c`, `gtklabel`). Stock `gdkeventsource.c`
built the `GdkTouchEvent` with `core_pointer`, a `GDK_SOURCE_MOUSE` device, so the gate never opened.
The fix emits touch events from Broadway's existing-but-unused `touchscreen` device. *(libgtk.)* The
cursor and handles now appear, and most other fixes build on this.

*TODO: add screenshot*

## Reliable taps

Touch listeners on `document` are passive by default in Firefox, so `ev.preventDefault()` did nothing.
A real tap fired `touchstart`, then a small move, then `touchcancel` (never `touchend`), eating roughly
half of all taps. Registering touch listeners with `{passive: false}` fixes it (`touch-action: none`
alone didn't stop it). *(broadwayd-only.)*

`touchcancel` was also unhandled, stranding `firstTouchDownId` and the implicit grab. The fix routes
`touchcancel` to `onTouchEnd`. *(broadwayd-only.)* As a declarative complement, `touch-action: none`
is set on `html, body` in `client.html`.

## Gestures survive repaints

A scroll/drag died after ~0.5s. Touch events kept firing, but to the original target node, which GTK
detaches from the DOM on the first surface repaint. Once detached, it stops bubbling events to
`document`. The fix: on `touchstart`, also attach move/end/cancel to `ev.target` (which survives
detachment), cache the surface id per touch (`touchSurfaceIds`), and dedupe with a per-event
`broadwayHandled` flag so the document and target paths don't double-send. *(broadwayd-only.)*

## On-screen keyboard

The OSK lagged one gesture behind because `SET_SHOW_KEYBOARD` was only applied on the next touch. The
fix factors focus/blur into `applyKeyboard()` and calls it from the `SET_SHOW_KEYBOARD` handler on
actual state change. *(broadwayd-only.)* A residual flicker remains when the selection bubble/handles
churn the entry's focus.

## Text input: IME and non-Latin

Typing non-Latin text (Cyrillic, CJK), gesture-typed words, autocorrect replacements, and dead keys
never reached the widget, while paste and Latin typing worked. GBoard reports those as a keyCode-229
`keydown` (the IME "processing" code) which the client drops; the real character arrives only as a
`beforeinput`/composition event on the hidden OSK input. The fix: in that handler, also forward
`insertText` / `insertReplacementText` via `commitTextToGtk(ev.data)`, and commit IME composition on
`compositionend` (Broadway has no preedit). Dedup guards (`lastKeyPressTime`, `lastCompositionEndTime`,
`imeComposing`) stop a Latin char being inserted twice. *(broadwayd-only.)*

> Autocorrect *deletions* (`deleteContentBackward`) are deliberately not bridged, to avoid
> double-deletes against the physical-backspace `keydown(8)` path.

## No spurious hover

The touch path emitted `ENTER`/`LEAVE` crossings on the mouse device, so a tap read as mouse hover,
popping `:hover` styling and tooltips (`GtkTooltip` only suppresses tooltips for touchscreen events).
The fix drops the hover crossings from the touch path (it keeps `surfaceWithMouse` for grabs, and
`TOUCH` events still route the tap). *(broadwayd-only.)*

## Popovers, menus, and the selection bubble

These turn on the popup/toplevel distinction, carried by the
[`is_popup` wire flag](../internals/protocol.md#changed-stock-struct-is_popup-on-new_surface).

Tap-outside should dismiss a popover. `check_autohide` looks up the grab on the event's device (now
the touchscreen), but the popover grab is on the logical pointer. The fix falls back to
`device->associated`. A fallback can only find a grab, never hold one, so it can't strand a grab and
freeze input. *(libgtk, `gdk/gdksurface.c`.)*

The selection-bubble Cut/Copy/Paste used to fire instead of the bubble being dismissed first. The
daemon raised and focused the tapped surface on every touch-begin (mirroring button-press), so tapping
the bubble (a popup) moved keyboard focus to it, the toplevel's focused `GtkText` emitted focus-out,
and its handler hid the bubble before the button's release could run its action. The fix: on `TOUCH`,
only raise+focus genuine toplevels (`!is_popup`), and skip popups. (Wayland xdg_popups don't take
keyboard focus on click either.) *(daemon.)*

Reopening the bubble could SIGSEGV. `_gtk_gesture_update_point` inserted a point into `priv->points`
(NULL `event`) before assigning `data->event`, and a re-entrant gesture check dereferenced the
not-yet-set event. The fix assigns `data->event` before the hash insert, covering all ~8 deref sites
at once. *(libgtk, `gtk/gtkgesture.c`.)*

Copy went missing after Select-All. The bubble is built once, so a caret bubble (Select All / Paste)
persisted after Select-All gave it a selection. The fix rebuilds the bubble after a Select-All whose
bubble is visible, gated on visibility so keyboard Ctrl+A is unaffected. *(libgtk, `gtktext.c` +
`gtktextview.c`.)*

## Dropdowns and the menu-tap freeze

GtkDropDown always selected the first item. `row_activated` ignored the `position` from the activate
signal and read `popup_selection.get_selected()`, which is only updated by select-on-hover, and touch
has no hover crossing, so it stayed at 0. The fix sets `popup_selection` to `position` before the
filter reset. This is a generic upstream GtkDropDown-on-touch bug, not Broadway-specific. *(libgtk,
`gtk/gtkdropdown.c`.)*

A menu-item tap could freeze all touch. Activating a popup item that destroys the popup left the
logical pointer's focus stale, so later taps landed on dead focus. The fix routes a pointer
re-assertion through the browser, covered in [Input region & pointer](../internals/input-region.md).
