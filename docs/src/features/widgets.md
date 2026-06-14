# Widgets

Per-widget behavior the fork changes to enable touch interaction (stock Broadway delivers no real touchscreen, so GTK's touch paths never fire - see [Touch interface](touch.md)). Each patch is gated to the Broadway backend.

## Notebook tabs

On Broadway the `GtkNotebook` tab bar becomes a single **pixel-scrolled** strip, closing the long-standing "tab bar not scrollable on touch" gap.

<video src="../images/notebook-tab-scroll.mp4" poster="../images/notebook-tab-scroll.png"
  autoplay loop muted playsinline style="max-width:100%;border-radius:6px">
  <img src="../images/notebook-tab-scroll.png" alt="Touch-swiping the overflowing tab strip"
    style="max-width:100%;border-radius:6px">
</video>

*Touch-swiping the overflowing tab strip: the tabs pixel-scroll to reveal the ones off-screen, fading at the edges, with no scroll arrows. (Amber dots trace the swipe.)*

- **Drag the tab strip to scroll it** on touch - the tabs follow the finger pixel-for-pixel without changing the page. A plain tap still selects a tab.
- **Position persists** - the strip stays where you left it; nothing snaps back.
- **Tab widths stay natural** - no stretch-to-fill, no jump as you drag.
- **Edge fades** hint at more tabs off-screen, instead of scroll arrows.
- **Wheel over the strip** pans horizontally; a vertical wheel switches the page and scrolls the new tab into view.
- **Touch reorder and detach still work** when the strip doesn't overflow.
- **Mouse is untouched** - desktop reorder and click stay stock.

> App-side note: an app can disable tab *reordering* under Broadway (via `set_tab_reorderable`) so the native reorder-drag doesn't compete with the pan, and zero the notebook header's horizontal padding so tabs reach the true edge.

> Pixel-scroll model, show-all-plus-offset layout, CSS-undershoot fades, and tap-vs-pan detection: [Notebook implementation](../internals/notebook.md).

## Labels

A selectable read-only `GtkLabel` normally has no touch selection UI. The fork gives it the same touch affordances as a text entry: **selection handles** and a **Copy / Select-all bubble**.

- Long-press to start a selection, then drag the handles to adjust it - a drag always keeps at least one character selected.
- The bubble offers **Copy** and **Select all**; tapping outside, or collapsing the selection to a caret, dismisses it.
- Selection and handles tear down cleanly when the label loses the PRIMARY selection or is re-rooted, so nothing is left floating.

The shared selection-bubble and handle machinery is covered in [Touch interface](touch.md).

## Drop-downs

A tap on an open `GtkDropDown` list selects the **row you tapped** - stock Broadway's pointer-emulated touch landed every tap on the first item instead.

## Tree views

The deprecated `GtkTreeView` (still used by some apps) gets its touch multi-selection aligned with the mouse:

- Pressing a row of a multi-row selection **keeps** the whole selection instead of collapsing to that row.
- Tapping **empty space** clears the selection.
- Tapping one row of a multi-selection **collapses to it on release** - stock left the tap a no-op and the cursor stale.
