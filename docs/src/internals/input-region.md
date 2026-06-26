# Input region & pointer

Broadway has no compositor to manage pointer focus and input regions. Two ops paper over the touch-interaction problems that fall out of that.

## `SET_INPUT_REGION` (op 19): per-surface input shape

Some parts of a surface shouldn't eat pointer events: text handles overlay content the user is trying to tap, and a popover's transparent shadow margin should pass clicks through. `BROADWAY_OP_SET_INPUT_REGION` carries the surface's input region so the browser only takes pointer events inside it.

Broadway can't represent an arbitrary shape, but the cases that matter are rectangular, so the op carries a **mode plus a bounding box** (`mode`, `BroadwayRect rect`):

- **mode 0 - whole**: the whole surface div takes pointer events (the default).
- **mode 1 - empty**: click-through everywhere. The surface div gets `pointer-events: none` so taps land on content behind - used for text handles.
- **mode 2 - rect**: only the given rect is interactive. The surface div goes `pointer-events: none` and a transparent *hit div* overlays the rect; a click on it resolves to this surface, while everything outside (e.g. a popover's shadow margin) falls through. The rect's x/y are signed - a toplevel's resize border sits just outside the surface, at negative coordinates.

The empty case (mode 1) doubles as a signal for [pointer recovery](#reassert_pointer-op-20-the-menu-tap-freeze): a surface with an empty input region (a text handle) doesn't count as interactive navigation, so it doesn't block a focus re-assert.

## `REASSERT_POINTER` (op 20): the menu-tap freeze

Tapping a menu/popup item that destroys the popup on activation (say, a history-dropdown item that fills a field and pops down) froze all subsequent touch.

The cause is stale pointer focus: GTK's per-sequence pointer focus ends up on the popup surface, which the activation destroys. GDK clears `surface_under_pointer` to NULL but never re-points it, and Broadway delivers no leave/enter crossing on destroy (the touch path deliberately [removed crossings](touch.md#no-spurious-hover) to kill `:hover`/tooltips). So every later tap lands on dead focus and is dropped.

The fix is browser-routed because the app's GTK needs the live serial/timestamp a real crossing carries - a daemon-built crossing is a no-op. So the daemon decides *when*, and the browser sends an `ENTER` via the normal input path through `BROADWAY_OP_REASSERT_POINTER` and its `broadway.js` handler.

It fires only when a popup actually closes (`is_popup`), skips if another interactive popup is still visible (so multi-level menus aren't interrupted), and defers ~50ms so the app processes the `ENTER` after its own popdown, then re-homes to the toplevel.

## The companion libgtk fix

`REASSERT_POINTER` handles popups that close on their own. The dropdown-item path also needed a `check_autohide` touch guard in `gdk/gdksurface.c`: tapping a dropdown item hit `check_autohide`, which thought the tap was *outside* and dismissed the popup, because `has_pointer` is only set from pointer crossings the touch path never sends. The guard skips the `!has_pointer` nulling for `GDK_TOUCH_BEGIN`, so the item activates instead.

## Grab stack

A popup chain (menu → submenu → popover) needs nested pointer grabs. The pointer grab was a single slot, so opening a submenu clobbered the parent's grab: closing the submenu then left the parent grabless, and it got dismissed (the whole chain collapsed, or the child stranded).

The grab is now a **stack**, kept in sync on both ends. The daemon (`broadway-server.c`) holds it as a list with the head = innermost; the old `pointer_grab_*` scalars cache the top so the routing read-sites are unchanged. A `GRAB_POINTER` op pushes, `UNGRAB_POINTER` pops back to the parent, and a surface that vanishes (hide/destroy) drops its level plus everything stacked above it. `broadway.js` mirrors the same stack. On reconnect the daemon replays the whole stack outermost-first.

Two follow-on fixes make keyboard menu navigation behave:

- **Pointer-focus crossing.** For an owner-events grab (menus/popovers), the browser only teleports pointer focus into the grab surface when the cursor is actually over it - on both grab and ungrab. A synthetic grab crossing into a surface the pointer isn't on lands at a stale coordinate and races GTK's keyboard-focus highlight, flickering or clearing the selected menu item. Confining and implicit grabs still teleport, since the pointer really is confined there.
- **`cascade-popdown`.** `GtkPopoverMenu` sets cascade-popdown (activating an item tears the whole chain down). Closing a submenu with the Left arrow would cascade up and also close the parent, so the popover-menu Left-arrow path suppresses cascade across the submenu popdown - it closes only the submenu and refocuses the parent item.

A window move (CSD titlebar drag) nests its own explicit grab over the press's implicit grab. On release the implicit grab must be dropped even though it's no longer the stack top, or it strands and swallows the next click.

## Always on top

`restack_layers` keeps **keep-above** surfaces above every normal one, preserving their order (popups follow via `transient_for`). A surface is keep-above if its `keep_above` flag is set - via the `SET_KEEP_ABOVE` op behind `gdk_broadway_surface_set_keep_above()` - or if it belongs to the spawned debug-menu client (auto-pinned, no binary change). It generalizes the old debug-menu-only pin into a per-surface layer.

A keep-above window also gets a **grab carve-out**: while another surface holds a pointer grab, events on a keep-above surface stay routed to its own client instead of being confined to the grab client, and a click on it moves keyboard focus there. So an always-on-top window (the debug menu) stays draggable, clickable, and typable while a menu is open. Broadway's pointer grab is one global daemon grab - unlike Wayland's per-client popup grab - so without the carve-out the grab would eat all input.
