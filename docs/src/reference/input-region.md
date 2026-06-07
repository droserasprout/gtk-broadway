# Input region & pointer

Two related ops fix touch interaction problems that come from Broadway having no real compositor to
manage pointer focus and input regions.

## `SET_INPUT_REGION` (op 19) - click-through surfaces

Some surfaces should not eat pointer events - notably the text cursor and selection handles, which
overlay content the user is trying to tap. `BROADWAY_OP_SET_INPUT_REGION` (and the matching
`BROADWAY_REQUEST_SET_INPUT_REGION`, struct `BroadwayRequestSetInputRegion { ... guint32 is_empty; }`)
marks a surface's input region **empty**, so the browser treats it as click-through and taps land
on the content behind it.

The empty-region flag also doubles as a signal for [pointer recovery](#reassert_pointer-op-20---the-menu-tap-freeze):
a surface with an empty input region (a text handle) does **not** count as "interactive navigation
in progress", so it doesn't block a focus re-assert.

## `REASSERT_POINTER` (op 20) - the menu-tap freeze

Tapping a menu/popup item that **destroys the popup on activation** (e.g. a history-dropdown item
that fills a field and pops down) froze all subsequent touch until a container restart.

**Root cause.** The emulated touch routes through the logical pointer; GTK's per-sequence pointer
focus ends up on the popup surface, which the activation then destroys. GDK clears
`surface_under_pointer` to NULL on destroy but never re-points it, and Broadway delivers **no
leave/enter crossing on destroy** (a real compositor would; and the touch path deliberately
[removed crossings](touch.md#no-spurious-hover) to kill `:hover`/tooltips). The logical pointer's
focus is left stale with nothing to reset it, so every later tap is delivered against dead focus and
dropped. It is **not** a grab - Broadway emits only a `GdkTouchEvent`, so the button-press
implicit-grab path never runs.

**Why the fix is browser-routed.** Injecting a single core-pointer `ENTER` crossing to the main
window recovers responsiveness instantly - but a daemon-built crossing is a no-op, because the app's
GTK needs the **live serial/timestamp** the browser carries on a real crossing. So the daemon
decides *when* and the **browser** sends the `ENTER` via the normal input path, through
`BROADWAY_OP_REASSERT_POINTER` (20) and its `broadway.js` handler.

**Gating.** Fire only when a popup actually closes (`is_popup`), skip if another interactive popup
is still visible (so multi-level menu navigation isn't interrupted - empty-input-region handles
don't count), defer ~50ms (so the app processes the socket `ENTER` after its own popdown/focus idle,
not before), and re-home to the toplevel. The deferred-enter timers are tracked on
`server->deferred_enters`, cancelled in `broadway_server_finalize`, and coalesced per-toplevel so a
hide+destroy can't double-fire.

## The companion libgtk fix

`REASSERT_POINTER` handles the freeze for popups that close on their own; the **dropdown item**
path also needed a `check_autohide` touch guard in `gdk/gdksurface.c`. Tapping a dropdown item hit
`check_autohide`, which - because `has_pointer` is only set from pointer crossings the touch path
never sends - thought the tap was *outside* the popup and dismissed it with a direct
`gdk_surface_hide()` (also bypassing `gtk_grab_remove`). The guard skips the `!has_pointer` nulling
for `GDK_TOUCH_BEGIN` (the finger landed on this surface, trust it), so the item activates instead.

> A libgtk `gtk_window_grab_notify` re-pick was tried and **reverted**: `GtkPopover` grabs with
> `gtk_grab_add` (GTK-level), not a GDK seat grab, so the GTK-level crossing never reached the
> GDK-level stale state. The browser-routed recovery is the one that works.
