# Touch implementation

The user-facing summary is in [Touch interface](../features/input.md). This page covers the individual fixes, each tagged **broadwayd-only** (`broadway.js` / `client.html` / daemon C) or **libgtk** (GDK / GTK widgets); see [broadwayd vs libgtk](../build/from-source.md#iterating-broadwayd-vs-libgtk).

## Source touch as a touchscreen

GTK gates all touch text UI on the event device being `GDK_SOURCE_TOUCHSCREEN`. Stock `gdkeventsource.c` built the `GdkTouchEvent` with `core_pointer` (a `GDK_SOURCE_MOUSE` device), so the gate never opened. The fix emits touch events from Broadway's existing-but-unused `touchscreen` device. Most other fixes build on this. *(libgtk.)*

## Reliable taps

Touch listeners on `document` are passive by default in Firefox, so `ev.preventDefault()` did nothing and a real tap fired `touchstart` then `touchcancel` (never `touchend`), eating ~half of all taps. Registering them `{passive: false}` fixes it, and `touchcancel` is now routed to `onTouchEnd`. Android Chrome offsets `touch.identifier` by a global counter, so the client remaps ids before the wire and sends the mapped id on *all* wire events (begin/move/end/cancel) - previously end/cancel sent the raw id, leaving GTK a begin-without-end and a stuck grab. *(broadwayd-only.)*

## Gestures survive repaints

A scroll/drag died after ~0.5s: GTK detaches the target node from the DOM on the first surface repaint, and once detached it stops bubbling to `document`. The fix: on `touchstart`, also attach move/end/cancel to `ev.target` (survives detachment), cache the surface id per touch, and dedupe via a per-event flag so the document and target paths don't double-send. *(broadwayd-only.)*

## On-screen keyboard

The OSK lagged one gesture behind because `SET_SHOW_KEYBOARD` was only applied on the next touch. The fix factors focus/blur into `applyKeyboard()` and calls it from the `SET_SHOW_KEYBOARD` handler on actual state change. *(broadwayd-only.)* A residual flicker remains when the selection bubble churns the entry's focus.

## Text input: IME and non-Latin

Non-Latin text (Cyrillic, CJK), gesture-typed words, autocorrect, and dead keys never reached the widget: GBoard reports those as a keyCode-229 `keydown` the client drops, with the real character arriving only as a `beforeinput`/composition event on the hidden OSK input. The fix forwards `insertText` / `insertReplacementText` via `commitTextToGtk`, and commits composition on `compositionend` (Broadway has no preedit); dedup guards stop a Latin char inserting twice. OSK backspace/delete arrive the same way and are forwarded as 0xFF08 / 0xFFFF key pairs. On Android Chrome the hidden input is primed with an 80-space buffer, refilled after each commit, since GBoard stops emitting deletes on a visually empty field. *(broadwayd-only.)*

## No spurious hover

The touch path emitted `ENTER`/`LEAVE` crossings on the mouse device, so a tap read as mouse hover, popping `:hover` styling and tooltips. The fix drops the hover crossings from the touch path (it keeps `surfaceWithMouse` for grabs, and `TOUCH` events still route the tap). *(broadwayd-only.)*

## Popovers, menus, and the selection bubble

These turn on the popup/toplevel distinction, carried by the [`is_popup` wire flag](protocol.md#changed-stock-struct-is_popup-on-new_surface).

**Tap-outside dismiss.** `check_autohide` looks up the grab on the event's device (now the touchscreen), but the popover grab is on the logical pointer. The fix falls back to `device->associated`; a fallback can only find a grab, never hold one, so it can't strand a grab. *(libgtk, `gdk/gdksurface.c`.)*

**Bubble dismiss vs Cut/Copy/Paste.** The action fired instead of the bubble dismissing first: the daemon raised+focused the tapped surface on every touch-begin, so tapping the bubble (a popup) moved keyboard focus to it and the toplevel's focus-out handler hid the bubble before the button's release ran. The fix: on `TOUCH`, only raise+focus genuine toplevels (`!is_popup`), skip popups. *(daemon.)*

**Tap-through past popups.** Touch bypassed the daemon's pointer-grab routing, so a tap landed on the surface beneath a live popup grab. Touch now follows the pointer grab, routed per sequence (a hash records the client chosen at BEGIN so UPDATE/END can't split across clients), and the raise/focus block gained the same no-grab guard as `BUTTON_PRESS`. *(daemon.)*

**Reopen SIGSEGV.** Reopening the selection bubble crashed: `_gtk_gesture_update_point` inserted a point before assigning `data->event`, so a re-entrant check dereferenced NULL. The fix assigns `data->event` before the insert. *(libgtk, `gtk/gtkgesture.c`.)*

**Copy missing after Select-All.** A caret bubble (Select All / Paste) persisted after Select-All gave it a selection. The fix rebuilds the bubble after a Select-All whose bubble is visible, gated on visibility so keyboard Ctrl+A is unaffected. *(libgtk, `gtktext.c` + `gtktextview.c`.)*

## Dropdowns and the menu-tap freeze

**Wrong row.** `GtkDropDown` always selected the first item: `row_activated` read `popup_selection.get_selected()`, which only hover updates, and touch has no hover. The fix sets `popup_selection` to the activate signal's `position` first. A generic upstream touch bug, not Broadway-specific. *(libgtk, `gtk/gtkdropdown.c`.)*

**Menu-tap freeze.** Activating a popup item that destroys the popup left the logical pointer's focus stale, so later taps landed on dead focus. The fix routes a pointer re-assertion through the browser, covered in [Input region & pointer](input-region.md).
