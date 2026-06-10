# Input region & pointer

Broadway has no real compositor to manage pointer focus and input regions. Two related ops paper over the touch-interaction problems that fall out of that.

## `SET_INPUT_REGION` (op 19): per-surface input shape

Some parts of a surface should not eat pointer events. The text cursor and selection handles overlay content the user is trying to tap; a popover's transparent shadow margin should pass clicks through to whatever is behind it. `BROADWAY_OP_SET_INPUT_REGION` (and the matching `BROADWAY_REQUEST_SET_INPUT_REGION`) carries the surface's input region so the browser only takes pointer events inside it.

Broadway can't represent an arbitrary shape, but the cases that matter are rectangular, so the op carries a **mode plus the region's bounding box** (`struct BroadwayRequestSetInputRegion { ... guint32 mode; BroadwayRect rect; }`):

- **mode 0 - whole**: no region; the whole surface div takes pointer events (the default).
- **mode 1 - empty**: click-through everywhere. The browser sets the surface div `pointer-events: none` so taps land on the content behind - used for text handles.
- **mode 2 - rect**: only the given rect is interactive. The surface div goes `pointer-events: none` (its rendered children inherit that) and one transparent *hit div* is overlaid on the rect; a click on it still resolves to this surface, while everything outside it - e.g. a popover's shadow margin - falls through. This is what lets a popover carry a shadow without the shadow becoming a dead click-zone around the menu. The rect's x/y are signed: a toplevel's resize border sits just *outside* the surface, at negative coordinates.

The empty case (mode 1) doubles as a signal for [pointer recovery](#reassert_pointer-op-20-the-menu-tap-freeze). A surface with an empty input region (a text handle) does not count as "interactive navigation in progress", so it doesn't block a focus re-assert.

## `REASSERT_POINTER` (op 20): the menu-tap freeze

Tapping a menu/popup item that destroys the popup on activation (say, a history-dropdown item that fills a field and pops down) froze all subsequent touch until a container restart.

The root cause is stale pointer focus. Emulated touch routes through the logical pointer, and GTK's per-sequence pointer focus ends up on the popup surface, which the activation then destroys. GDK clears `surface_under_pointer` to NULL on destroy but never re-points it, and Broadway delivers no leave/enter crossing on destroy. A real compositor would, but the touch path deliberately [removed crossings](touch.md#no-spurious-hover) to kill `:hover`/tooltips. That leaves the logical pointer's focus stale with nothing to reset it, so every later tap is delivered against dead focus and dropped. This is no grab: Broadway emits only a `GdkTouchEvent`, so the button-press implicit-grab path never runs.

The fix is browser-routed for a reason. Injecting a single core-pointer `ENTER` crossing to the main window recovers responsiveness instantly, but a daemon-built crossing is a no-op, because the app's GTK needs the live serial/timestamp the browser carries on a real crossing. So the daemon decides *when*, and the browser sends the `ENTER` via the normal input path, through `BROADWAY_OP_REASSERT_POINTER` (20) and its `broadway.js` handler.

Gating keeps it from firing in the wrong cases. It fires only when a popup actually closes (`is_popup`), and skips if another interactive popup is still visible, so multi-level menu navigation isn't interrupted (empty-input-region handles don't count). It defers ~50ms so the app processes the socket `ENTER` after its own popdown/focus idle rather than before, then re-homes to the toplevel. The deferred-enter timers live on `server->deferred_enters`, get cancelled in `broadway_server_finalize`, and are coalesced per-toplevel so a hide+destroy can't double-fire.

## The companion libgtk fix

`REASSERT_POINTER` handles the freeze for popups that close on their own. The dropdown-item path also needed a `check_autohide` touch guard in `gdk/gdksurface.c`. Tapping a dropdown item hit `check_autohide`, which thought the tap was *outside* the popup and dismissed it with a direct `gdk_surface_hide()` (also bypassing `gtk_grab_remove`). The reason: `has_pointer` is only set from pointer crossings, which the touch path never sends. The guard skips the `!has_pointer` nulling for `GDK_TOUCH_BEGIN` (the finger landed on this surface, so trust it), and the item activates instead.

> A libgtk `gtk_window_grab_notify` re-pick was tried and **reverted**: `GtkPopover` grabs with `gtk_grab_add` (GTK-level), not a GDK seat grab, so the GTK-level crossing never reached the GDK-level stale state. The browser-routed recovery is the one that works.
