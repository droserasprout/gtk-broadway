# Backend comparison

Where the fork sits next to stock Broadway and the native Wayland backend, columns ordered stock -> fork -> native. The **Brotway** column is `4.22.4-brotway`; "Broadway" is stock upstream. The two Broadway columns match across most windowing and platform-integration rows - the fork doesn't touch them, and Broadway has no window manager, desktop session, or GPU, so most never apply.

Legend: **🟢** full support, **🟡** partial/workaround/caveats, **🔴** not supported, **⚪** not applicable.

| Feature | Broadway | **Brotway** | Wayland |
|---------|:---:|:---:|:---:|
| **Clipboard & drag-and-drop** | | | |
| Text clipboard (host read/write) | 🔴 | 🟢 [^clip] | 🟢 |
| Rich clipboard / images / mimetypes | 🔴 | 🔴 [^rich] | 🟢 |
| PRIMARY selection (middle-click) | 🔴 | 🔴 [^primary] | 🟢 |
| Drag-and-drop within app | 🟡 | 🟡 | 🟢 |
| Drag-and-drop cross-process | 🔴 | 🔴 [^dnd] | 🟢 |
| **Touch & input** | | | |
| Touch input / touchscreen events | 🔴 [^touchstock] | 🟢 | 🟢 |
| Touch text-selection UI (handles + bubble) | 🔴 | 🟢 | 🟢 |
| Multi-touch gesture (pinch-zoom UI) | 🔴 | 🟢 [^zoom] | 🟢 |
| Tablet / stylus input (pressure, tilt, tool) | 🔴 | 🔴 [^stylus] | 🟢 |
| On-screen keyboard sync | 🔴 | 🟡 [^osk] | 🟢 |
| IME / non-Latin / preedit | 🔴 | 🟡 [^ime] | 🟢 |
| Smooth / touchpad scroll-source detection | 🟡 | 🟡 [^scroll] | 🟢 |
| Keyboard layout groups | 🔴 | 🔴 | 🟢 |
| Inhibit system shortcuts (grab all keys) | 🔴 | 🔴 | 🟢 |
| Named mouse cursors (resize / text / pointer) | 🔴 | 🟢 [^cursor] | 🟢 |
| **Rendering** | | | |
| OpenGL rendering | 🔴 | 🔴 [^render] | 🟢 |
| Vulkan rendering | 🔴 | 🔴 [^render] | 🟢 |
| Cairo / software rendering | 🟢 | 🟢 | 🟢 |
| DMA-BUF texture import | 🔴 | 🔴 | 🟢 [^dmabuf] |
| Graphics offload / video subsurfaces | 🔴 | 🔴 | 🟢 [^offload] |
| HDR / wide-gamut color (`GdkColorState`) | 🔴 | 🔴 | 🟢 |
| Presentation-time / vsync feedback | 🔴 | 🔴 | 🟢 |
| **Scaling & monitors** | | | |
| HiDPI integer scaling | 🟡 | 🟢 [^hidpi] | 🟢 |
| Fractional scaling | 🔴 | 🔴 [^forkfrac] | 🟢 [^frac] |
| Multiple monitors | 🟡 | 🟡 | 🟢 |
| **Session & connection** | | | |
| Session reconnect after network drop / sleep | 🔴 | 🟢 [^reconn] | ⚪ |
| Pause rendering while not visible | 🔴 | 🟢 [^suspend] | 🟢 |
| **Windowing** | | | |
| Client-side decorations | 🟢 | 🟢 | 🟢 |
| Server-side decorations | ⚪ | ⚪ [^deco] | 🟢 |
| Multiple top-level windows | 🟢 | 🟢 | 🟢 |
| Window transparency / RGBA | 🟢 | 🟢 | 🟢 |
| WM stacking & workspace hints (keep above/below, lower, sticky) | 🔴 | 🔴 | 🔴 |
| Startup notification / window handle export (xdg-activation) | 🔴 | 🔴 | 🟢 |
| Server window menu (`show_window_menu`) | 🔴 | 🔴 | 🟡 |
| Tiled-edge constraints | 🔴 | 🔴 | 🟢 |
| **UI surfaces** | | | |
| Tooltips | 🟢 | 🟢 [^tooltip] | 🟢 |
| Popovers / autohide popups | 🟢 | 🟢 [^popup] | 🟢 |
| **Platform integration** | | | |
| `gtk_show_uri` / open external URIs | 🔴 | 🟢 [^uri] | 🟢 |
| System tray / status icon | 🔴 | 🔴 | 🟡 [^tray] |
| Accessibility bridge | 🔴 | 🔴 | 🟢 [^a11y] |
| Desktop settings (dark mode / accent / fonts) | 🔴 | 🔴 | 🟢 [^settings] |

## What the fork adds over stock Broadway

Text clipboard, real touch (events, text-selection UI, pinch-zoom), OSK and IME bridging, named mouse cursors, HiDPI reflow, opening external links, and session management (in-place reconnect, hidden-tab pause). Plus rendering-correctness fixes around client-side decorations: popups land on their anchor, a popover's shadow passes clicks through (via the [input region](../internals/input-region.md)), and uniform borders render without the 1px seam. Cross-process DnD, rich/image clipboard, PRIMARY selection, and GPU rendering stay unsupported, same as stock.

[^clip]: Stock upstream Broadway ships no `GdkClipboard`. The fork adds one, bridging the browser clipboard in both directions. See [Clipboard](input.md#clipboard).

[^rich]: Text-only by design; the read path rejects non-text. No image clipboard support.

[^primary]: Browsers expose no JS API for the PRIMARY selection.

[^dnd]: Broadway's DnD backend is still a stub. Within-app drag works only as far as stock does; the fork does not touch real DnD.

[^touchstock]: Stock Broadway delivers touch as mouse events, so GTK's touch text UI (gated on a real touchscreen device) never fires. The fork creates one. See [Touch interface](input.md).

[^zoom]: Whole-UI zoom 0.25x-5x, persisted per-origin, verified on mobile Firefox. See [Pinch to zoom](input.md#pinch-to-zoom).

[^stylus]: A stylus works as plain pointer/touch input; pressure, tilt, and tool identity are not bridged.

[^osk]: Show/hide mostly synced; residual flicker on mixed selection-bubble focus state.

[^cursor]: Stock Broadway always shows the default arrow. The fork forwards GTK's per-surface cursor to the browser's CSS `cursor` (resize edges, text, links, ...). See [Dynamic cursor](input.md#dynamic-cursor).

[^ime]: Non-Latin / CJK / gesture-typed text is committed via composition events. Autocorrect deletions are not bridged.

[^scroll]: Broadway forwards wheel scroll, but has no touchpad/source distinction or true smooth scroll; macOS reports everything as surface (smooth) scroll.

[^render]: Broadway has no GPU context; it renders through `gskbroadwayrenderer` with a Cairo fallback. The fork doesn't change this. The other backends reach GL/Vulkan/Metal natively.

[^dmabuf]: Linux-only by construction; imported via `zwp_linux_dmabuf` on Wayland. No other backend builds a dmabuf texture.

[^offload]: Wayland only; lets video/textures bypass the compositor. No other backend implements subsurface offload.

[^hidpi]: The fork drives genuine reflow plus crisp scale on zoom. Stock only does integer `devicePixelRatio` sharpening, no resize. See [Scaling & HiDPI](display.md#scaling-and-hidpi).

[^frac]: Via `fractional-scale-v1` on Wayland and the native backing scale on macOS.

[^forkfrac]: The fork's pinch-zoom reaches arbitrary 0.25x-5x, but not via a fractional surface scale: the GDK scale factor stays integer. Different mechanism, same visual result. See [Pinch to zoom](input.md#pinch-to-zoom).

[^reconn]: Session token + PING/PONG heartbeat; reconnects in place after screen-off or a network handover, with single-display arbitration between browsers. Local backends have no network link to lose. See [Connection management](connection.md).

[^suspend]: The browser's Page Visibility drives `SUSPEND`/`RESUME`: a hidden tab streams zero frames, resume repaints a delta without reconnect. Wayland and macOS throttle natively; X11/Win32 only suspend a *minimized* window, not an occluded one.

[^deco]: Broadway runs inside a browser tab; the browser window chromes it, so SSD/CSD is not meaningful in the usual sense.

[^tooltip]: The fork suppresses spurious `:hover` / tooltips on touch taps.

[^popup]: The fork fixes autohide dismiss-on-tap, `GtkDropDown` correct-item selection, and the menu-tap freeze.

[^uri]: The fork routes external URIs to the browser's `window.open`, hooked through `gtk_show_uri`. See [Opening links](input.md#opening-links).

[^tray]: GTK4 has no status-icon API (`GtkStatusIcon` was removed). An app can still expose a D-Bus `StatusNotifierItem` via an appindicator library, shown by KDE natively or GNOME with the AppIndicator extension - backend-independent. A browser-tunneled Broadway session has no shell to host one.

[^a11y]: AT-SPI over D-Bus on the Linux backends, AccessKit on Windows/macOS. Broadway has no a11y bridge.

[^settings]: Dark-mode / accent / font settings come from the xdg settings portal on Wayland, XSETTINGS on X11, and AppKit (`NSAppearance`) on macOS. Broadway has no desktop session; theming is whatever CSS the app ships.
