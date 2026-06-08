# Notebook implementation

The user-facing summary is in [Notebook tabs](../features/notebook.md). This is a
[libgtk](build-split.md) change (`gtk/gtknotebook.c`), gated to Broadway.

## One unified model

Earlier attempts did discrete tab-stepping, then a smooth-pan-with-snap that switched between
GtkNotebook's *windowed* layout (settled) and a show-all layout (dragging). The mode switch, the
snap, and the expand-to-fill together made tabs jump and change width. The fix collapses all of
that into one model: `gtk_notebook_tab_pixel_scroll`, gated to Broadway, scrollable, TOP/BOTTOM
tabs, LTR. When it is true, the stock tab *windowing* and *scroll arrows* are bypassed in **every**
state, so there is nothing left to jump between. Off Broadway it is false and stock behaviour is
unchanged.

## Layout: show-all plus offset plus clip

`calculate_shown_tabs` lays **all** tabs in one continuous row at natural width with no stretch, and
`calculate_tabs_allocation` shifts the row by `anchor -= touch_pan_px`. The `tabs_widget` gizmo is
`GTK_OVERFLOW_HIDDEN` permanently so off-edge tabs clip. `touch_pan_px` is the **persistent** scroll
offset, clamped to `[0, touch_pan_max]` each allocation, so it self-corrects as tabs are added or
removed.

## Edge fades, not arrows

With arrows suppressed, `snapshot_tabs` fades the tab strip into the header background at whichever
edge has more tabs. The fades are themed CSS `undershoot` nodes, one per edge, parented under the
`tabs` gizmo, whose gradient runs from the header `$dark_fill` to transparent. They are native
`GSK_LINEAR_GRADIENT_NODE`s with no rasterized texture.

The first version used a `gtk_snapshot_push_mask(GSK_MASK_MODE_ALPHA)` alpha mask, but `GskMaskNode`
has no Broadway renderer and [fell back to a cairo texture](../features/scaling.md) re-uploaded every
scroll frame. The CSS-undershoot version stays theme-correct (it fades to the real header colour) and
adds zero texture traffic; see [Performance](performance.md#native-notebook-edge-fade).

## Nothing snaps

`tab_scroll_end` just clears the scrolling flag, the strip stays where the finger left it, and the
next drag resumes from `touch_pan_px`.

## Telling a tap from a pan

GtkNotebook selects on *press*, so a drag-from-a-tab would switch pages before any motion. For
touchscreen events `gesture_pressed` only **records** the tab, and the select commits on **release**
in `gesture_released`, unless the touch-only `GtkGestureDrag` claimed the sequence as a horizontal
pan first, which cancels the click gesture and drops the deferred tap. Commit threshold:
`TAB_SCROLL_THRESHOLD` = 8px.

The drag gesture is touch-only, so desktop reorder and click stay stock. Wheel over the strip pans
horizontally, or switches the page and scrolls it into view on vertical.
