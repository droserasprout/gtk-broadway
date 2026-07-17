# Display & rendering

Rendering-correctness and presentation fixes stock Broadway lacks, mostly around client-side decorations and the browser tab. No touch device needed.

## Rendering correctness

- **No white flash** on load or zoom-out - unpainted areas follow the browser's light/dark theme.
- **The app follows the browser's light/dark preference** live: `broadwayd` serves the browser's `prefers-color-scheme` over an `org.freedesktop.appearance` portal, which both plain GTK (as `gtk-interface-color-scheme`) and libadwaita apps read. Set `BROTWAY_COLOR_SCHEME=light`/`dark` to pin it per process and ignore the browser (`auto` is the default).
- **Popups land on their anchor**, not offset by their shadow margin.
- **A popover's shadow passes clicks through** instead of swallowing them (via the [input region](../internals/input-region.md)).
- **Seam-free borders** - uniform widget borders render without the 1px seam or corner sliver stock leaves.
- **Glyphs stay in their row** under a scrolled list - icons and cell text no longer drift by the scroll offset.
- **Off-screen-anchored popovers open** instead of no-opping when their anchor falls outside the reported monitor.

## Scaling and HiDPI {#scaling-and-hidpi}

Icons (and anything under a scale or rotate transform) rendered blurry on HiDPI and under [pinch-zoom](input.md#pinch-to-zoom): the Broadway GSK renderer applied the transform as a matrix on an already-rasterized texture, scaling the bitmap up instead of re-rasterizing at the target resolution.

The fork routes scale/rotate transforms through the cairo fallback at display resolution, and drops cached textures on a scale change so they re-rasterize at the new scale. Icons stay crisp under scale transforms and HiDPI, and the re-render after a [pinch](input.md#pinch-to-zoom) settles sharp.

## Window placement

**New windows open centered** instead of top-left, and re-center on zoom so they can't be stranded off-screen.

- **Modal dialogs dim and block** the window behind them - the backdrop a compositor would draw.
- **No Minimize button** - Broadway has no window manager to iconify, so it's dropped from titlebars and the window menu.
- **Dialogs no longer snap** to the top-left (and grow) when you drag or resize them while another window is clicked.

## Tab title and favicon {#tab-title-and-favicon}

*New in v3.1*

Stock Broadway leaves the browser tab generic - a fixed title and no icon. The fork forwards the app's window title and icon to the page, so the tab identifies the running app.

- **Tab title** follows the topmost real toplevel's window title, so dialogs, menus, and popovers don't hijack it.
- **Favicon** follows the app's themed window icon.
- Both **persist across a page reload**, restored immediately so a refresh doesn't flash the default.
- Before any app connects, the tab shows a **`brotway`** default.

Limitations: only the **topmost toplevel** sets the identity (a transient on top doesn't change it), and an app with no window icon leaves the favicon empty.

> The title and icon ride `BROADWAY_OP_SET_TITLE` / `BROADWAY_OP_SET_ICON`, stored per-surface and replayed on reconnect. The client caches the last pair in `sessionStorage` (icon as a `data:` URL) so a reload restores it before the WebSocket reconnects.

> Anchor placement and click-through shadow share the [input-region](../internals/input-region.md) op with the touch fixes. Per-fix history: [Changelog](../changelog.md).
