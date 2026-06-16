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
