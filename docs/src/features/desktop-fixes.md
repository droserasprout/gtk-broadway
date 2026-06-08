# Desktop & rendering fixes

Beyond the headline features, the fork fixes a set of rendering-correctness and desktop-interaction
problems stock Broadway has, mostly around client-side decorations and the browser host. These need
no touch device; they improve the plain desktop-browser session too.

## Rendering correctness

- **No white flash** on load or zoom-out - unpainted areas follow the browser's light/dark theme
  instead of flashing white.
- **Popups land on their anchor**, not offset by their shadow. Stock Broadway placed menus/popovers
  off by their shadow margin.
- **A popover's shadow passes clicks through** instead of swallowing them - the
  [input region](../internals/input-region.md) carries the popover's real shape, so the transparent
  shadow margin is click-through.
- **Seam-free borders** - uniform widget borders render without the 1px seam or corner sliver stock
  leaves between a border and its background.
- **Crisp icons** under scale transforms and HiDPI - see [Scaling & HiDPI](scaling.md).

## Window placement

- **New windows open centered** instead of at the top-left, and re-center on zoom so they can't be
  stranded off-screen. The maximized main window keeps its position.

## Desktop interaction

- **Horizontal two-finger swipe scrolls** instead of triggering the browser's back/forward
  navigation.
- **Drags survive the cursor leaving the widget** (e.g. a scrollbar drag continues when the pointer
  moves off the scrollbar).
- **No crash when starting a drag from a text selection.**
- **Clicking empty space in a list clears the selection.**

> Several of these (anchor placement, click-through shadow) share machinery with the touch fixes;
> the shadow shape is the same [input-region](../internals/input-region.md) op. Per-fix history is in
> the [Changelog](../changelog.md).
