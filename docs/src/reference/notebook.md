# Notebook tabs

On Broadway the `GtkNotebook` tab bar is turned into a single **pixel-scrolled** strip: drag it on
touch and the tabs follow the finger pixel-for-pixel without changing the page; a plain tap still
selects a tab. This fixes the long-standing "tab bar not scrollable on touch" gap.

**File:** `gtk/gtknotebook.c`. **Deploy:** libgtk. **Verified:** on the local browser-free harness.

## One unified model

Earlier attempts did discrete tab-stepping, then a smooth-pan-with-snap that switched between
GtkNotebook's *windowed* layout (settled) and a show-all layout (dragging) - the mode switch plus
snap plus expand-to-fill made tabs jump and change width. The fix is one model,
`gtk_notebook_tab_pixel_scroll`, gated to: Broadway, scrollable, TOP/BOTTOM tabs, LTR. When true,
the stock tab *windowing* and *scroll arrows* are bypassed in **every** state - nothing to jump
between. Off Broadway it is false and stock behaviour is unchanged.

- **Show-all + pixel offset + clip.** `calculate_shown_tabs` lays **all** tabs in one continuous
  row at natural width (no stretch); `calculate_tabs_allocation` shifts the row by
  `anchor -= touch_pan_px`. The `tabs_widget` gizmo is `GTK_OVERFLOW_HIDDEN` permanently so off-edge
  tabs clip. `touch_pan_px` is the **persistent** scroll offset, clamped to `[0, touch_pan_max]`
  each allocation so it self-corrects as tabs are added/removed.
- **Edge fades, not arrows.** With arrows suppressed, `snapshot_tabs` fades the tab strip itself to
  transparent at whichever edge has more tabs, via `gtk_snapshot_push_mask(GSK_MASK_MODE_ALPHA)`
  with a 4-stop horizontal alpha gradient (48px fade). A **mask** (not a colour overlay) is used so
  it reveals the real background and reads as "fade to bg" on any theme - important under the dark
  theme, where a black overlay was invisible and left a bright sliver. Broadway has no `GskMaskNode`
  renderer but [falls back to a cairo texture](scaling.md).
- **No snapping.** `tab_scroll_end` just clears the scrolling flag; the strip stays where the finger
  left it and the next drag resumes from `touch_pan_px`.
- **Tap vs pan (deferred selection).** GtkNotebook selects on *press*, so a drag-from-a-tab would
  switch pages before any motion. For touchscreen events, `gesture_pressed` only **records** the
  tab; the select commits on **release** in `gesture_released` - unless the touch-only
  `GtkGestureDrag` claimed the sequence as a horizontal pan (which cancels the click gesture and
  drops the deferred tap). Commit threshold: `TAB_SCROLL_THRESHOLD` = 8px.
- **Mouse unchanged.** The drag gesture is touch-only; desktop reorder and click are stock. Wheel
  over the strip pans (horizontal) or switches the page and scrolls it into view (vertical).

> App-side note (Nicotine+): tab *reordering* is disabled under Broadway
> (`iconnotebook.py set_tab_reorderable`) so the native reorder-drag doesn't compete with the pan,
> and the CSS zeroes the notebook header's horizontal padding so tabs reach the true edge.
