# Touch implementation

The user-facing summary is in [Touch interface](../features/touch.md). This page covers the individual fixes. Each is tagged **broadwayd-only** (`broadway.js` / `client.html` / daemon C) or **libgtk** (GDK / GTK widgets); see [broadwayd vs libgtk](../build/from-source.md#iterating-broadwayd-vs-libgtk).

## Source touch as a touchscreen

GTK gates all touch text UI - selection handles, the Cut/Copy/Paste bubble - on the event device being `GDK_SOURCE_TOUCHSCREEN` (`gtktext.c`, `gtktextview.c`, `gtklabel`). Stock `gdkeventsource.c` built the `GdkTouchEvent` with `core_pointer`, a `GDK_SOURCE_MOUSE` device, so the gate never opened. The fix emits touch events from Broadway's existing-but-unused `touchscreen` device. *(libgtk.)* Most other fixes build on this.

## Reliable taps

Touch listeners on `document` are passive by default in Firefox, so `ev.preventDefault()` did nothing: a real tap fired `touchstart`, a small move, then `touchcancel` (never `touchend`), eating ~half of all taps. Registering them with `{passive: false}` fixes it (`touch-action: none` alone didn't). `touchcancel` was also unhandled, stranding `firstTouchDownId` and the implicit grab; the fix routes it to `onTouchEnd`. `touch-action: none` is set on `html, body` in `client.html` as a declarative complement. *(broadwayd-only.)*

Android Chrome offsets `touch.identifier` by a global counter, so the client remaps ids before the wire; `touchend`/`touchcancel` sent the raw browser id while begin/move sent the mapped one, so every tap there left GTK a begin-without-end sequence - a stuck implicit grab. All wire events now send the mapped id. *(broadwayd-only.)*

## Gestures survive repaints

A scroll/drag died after ~0.5s: events kept firing to the original target node, but GTK detaches it from the DOM on the first surface repaint, and once detached it stops bubbling to `document`. The fix: on `touchstart`, also attach move/end/cancel to `ev.target` (survives detachment), cache the surface id per touch (`touchSurfaceIds`), and dedupe via a per-event `broadwayHandled` flag so the document and target paths don't double-send. *(broadwayd-only.)*

## On-screen keyboard

The OSK lagged one gesture behind because `SET_SHOW_KEYBOARD` was only applied on the next touch. The fix factors focus/blur into `applyKeyboard()` and calls it from the `SET_SHOW_KEYBOARD` handler on actual state change. *(broadwayd-only.)* A residual flicker remains when the selection bubble/handles churn the entry's focus.

## Text input: IME and non-Latin

Non-Latin text (Cyrillic, CJK), gesture-typed words, autocorrect replacements, and dead keys never reached the widget, while paste and Latin typing worked. GBoard reports those as a keyCode-229 `keydown` (the IME "processing" code) which the client drops; the real character arrives only as a `beforeinput`/composition event on the hidden OSK input. The fix: in that handler, also forward `insertText` / `insertReplacementText` via `commitTextToGtk(ev.data)`, and commit composition on `compositionend` (Broadway has no preedit). Dedup guards (`lastKeyPressTime`, `lastCompositionEndTime`, `imeComposing`) stop a Latin char inserting twice. *(broadwayd-only.)*

OSK backspace and delete also arrive only as keyCode-229 keydowns; the same `beforeinput` handler forwards `deleteContentBackward` / `deleteContentForward` as 0xFF08 / 0xFFFF key pairs (an earlier `input`-listener replay path was dead code). On Android Chrome the hidden input is primed with an 80-space buffer - GBoard stops emitting deletes on a visually empty field - and the buffer is refilled after each commit instead of being left empty. *(broadwayd-only.)*

## No spurious hover

The touch path emitted `ENTER`/`LEAVE` crossings on the mouse device, so a tap read as mouse hover, popping `:hover` styling and tooltips (`GtkTooltip` only suppresses tooltips for touchscreen events). The fix drops the hover crossings from the touch path; it keeps `surfaceWithMouse` for grabs, and `TOUCH` events still route the tap. *(broadwayd-only.)*

## Popovers, menus, and the selection bubble

These turn on the popup/toplevel distinction, carried by the [`is_popup` wire flag](protocol.md#changed-stock-struct-is_popup-on-new_surface).

**Tap-outside dismiss.** `check_autohide` looks up the grab on the event's device (now the touchscreen), but the popover grab is on the logical pointer. The fix falls back to `device->associated`; a fallback can only find a grab, never hold one, so it can't strand a grab and freeze input. *(libgtk, `gdk/gdksurface.c`.)*

**Bubble dismiss vs Cut/Copy/Paste.** The action fired instead of the bubble dismissing first: the daemon raised and focused the tapped surface on every touch-begin (mirroring button-press), so tapping the bubble (a popup) moved keyboard focus to it, the toplevel's focused `GtkText` emitted focus-out, and its handler hid the bubble before the button's release could run. The fix: on `TOUCH`, only raise+focus genuine toplevels (`!is_popup`), skip popups. (Wayland xdg_popups don't take keyboard focus on click either.) *(daemon.)*

**Tap-through past popups.** Touch events bypassed the daemon's pointer-grab routing, so with a popup grab live a tap landed on the surface beneath it. Touch now follows the pointer grab, routed per sequence - a hash records the client chosen at BEGIN, so UPDATE/END can't be split across clients when the grab changes mid-touch - and the raise/focus block above gained the same no-grab guard as `BUTTON_PRESS`. *(daemon.)*

**Reopen SIGSEGV.** `_gtk_gesture_update_point` inserted a point into `priv->points` (NULL `event`) before assigning `data->event`, and a re-entrant gesture check dereferenced the not-yet-set event. The fix assigns `data->event` before the hash insert, covering all ~8 deref sites at once. *(libgtk, `gtk/gtkgesture.c`.)*

**Copy missing after Select-All.** The bubble is built once, so a caret bubble (Select All / Paste) persisted after Select-All gave it a selection. The fix rebuilds the bubble after a Select-All whose bubble is visible, gated on visibility so keyboard Ctrl+A is unaffected. *(libgtk, `gtktext.c` + `gtktextview.c`.)*

## Dropdowns and the menu-tap freeze

**Wrong row.** GtkDropDown always selected the first item: `row_activated` ignored the `position` from the activate signal and read `popup_selection.get_selected()`, which only select-on-hover updates, and touch has no hover crossing, so it stayed at 0. The fix sets `popup_selection` to `position` before the filter reset. This is a generic upstream GtkDropDown-on-touch bug, not Broadway-specific. *(libgtk, `gtk/gtkdropdown.c`.)*

**Menu-tap freeze.** Activating a popup item that destroys the popup left the logical pointer's focus stale, so later taps landed on dead focus. The fix routes a pointer re-assertion through the browser, covered in [Input region & pointer](input-region.md).
