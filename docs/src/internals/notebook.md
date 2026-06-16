# Notebook implementation

The user-facing summary is in [Notebook tabs](../features/input.md#notebook-tabs). This is a [libgtk](../build/from-source.md#iterating-broadwayd-vs-libgtk) change (`gtk/gtknotebook.c`), gated to Broadway.

## One unified model

`gtk_notebook_tab_pixel_scroll` (gated to Broadway, scrollable, TOP/BOTTOM tabs, LTR) replaces the stock tab *windowing* and *scroll arrows* in **every** state, so there's nothing for tabs to jump between. Off Broadway it's false and stock behaviour is unchanged.

## Layout: show-all plus offset plus clip

`calculate_shown_tabs` lays **all** tabs in one continuous row at natural width with no stretch, and `calculate_tabs_allocation` shifts the row by `anchor -= touch_pan_px`. The `tabs_widget` gizmo is permanently `GTK_OVERFLOW_HIDDEN` so off-edge tabs clip. `touch_pan_px` is the persistent scroll offset, clamped to `[0, touch_pan_max]` each allocation, so it self-corrects as tabs are added or removed.

## Edge fades, not arrows

With arrows suppressed, `snapshot_tabs` fades the strip into the header background at whichever edge has more tabs. The fades are themed CSS `undershoot` nodes (native `GSK_LINEAR_GRADIENT_NODE`s, gradient from the header `$dark_fill` to transparent) with no rasterized texture - theme-correct and zero texture traffic, unlike the alpha-mask approach Broadway would [rasterize](../features/display.md#scaling-and-hidpi). See [Performance](performance.md#limitations).

## Nothing snaps

`tab_scroll_end` just clears the scrolling flag; the strip stays where the finger left it, and the next drag resumes from `touch_pan_px`.

## Telling a tap from a pan

GtkNotebook selects on *press*, so a drag-from-a-tab would switch pages before any motion. For touchscreen events `gesture_pressed` only **records** the tab, and the select commits on **release**, unless the touch-only `GtkGestureDrag` claimed the sequence as a horizontal pan first (threshold `TAB_SCROLL_THRESHOLD` = 8px), which cancels the click and drops the deferred tap. The deferral only engages when the strip can actually pan (`touch_pan_max > 0`); otherwise touch falls through to the stock press path, so reorder/detach and focus handoff keep working. The deferred tab is held as its page pointer and re-validated on release.

The drag gesture is touch-only, so desktop reorder and click stay stock. Wheel over the strip pans horizontally, or switches the page and scrolls it into view on vertical; a strip with nothing to pan propagates the wheel instead of consuming it.
