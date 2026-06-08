# Notebook tabs

On Broadway the `GtkNotebook` tab bar becomes a single **pixel-scrolled** strip, closing the
long-standing "tab bar not scrollable on touch" gap.

*TODO: add screenshot*

## What works

- **Drag the tab strip to scroll it** on touch - the tabs follow the finger pixel-for-pixel without
  changing the page. A plain tap still selects a tab.
- **Position persists** - the strip stays where you left it; nothing snaps back.
- **Tab widths stay natural** - no stretch-to-fill, no jump as you drag.
- **Edge fades** hint at more tabs off-screen, instead of scroll arrows.
- **Wheel over the strip** pans horizontally; a vertical wheel switches the page and scrolls the new
  tab into view.
- **Mouse is untouched** - desktop reorder and click stay stock.

> App-side note: an app can disable tab *reordering* under Broadway (via `set_tab_reorderable`) so
> the native reorder-drag doesn't compete with the pan, and zero the notebook header's horizontal
> padding so tabs reach the true edge.

> The unified pixel-scroll model, the show-all-plus-offset layout, the CSS-undershoot fades, and
> tap-vs-pan detection are in [Notebook implementation](../internals/notebook.md).
