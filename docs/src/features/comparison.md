# Backend comparison

Where the fork sits next to stock Broadway and the native Wayland backend, columns ordered stock -> fork -> native. The **Brotway** column is `4.22.4-brotway`; "Broadway" is stock upstream. The two Broadway columns match across most windowing and platform-integration rows - the fork doesn't touch them, and Broadway has no window manager, desktop session, or GPU, so most never apply.

Legend: **🟢** full support, **🟡** partial/workaround/caveats, **🔴** not supported, **⚪** not applicable.

| Feature | Broadway | **Brotway** | Wayland |
|---------|:---:|:---:|:---:|
| Remote access | 🟢 [^remote] | ✨ | 🔴 |
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
| On-screen keyboard sync | 🔴 | 🟢 [^osk] | 🟢 |
| IME / non-Latin / preedit | 🔴 | 🟡 [^ime] | 🟢 |
| Smooth (pixel-precise) scrolling | 🔴 | 🟢 [^smooth] | 🟢 |
| Touchpad / scroll-source detection | 🔴 | 🟡 [^scroll] | 🟢 |
| Named mouse cursors (resize / text / pointer) | 🔴 | 🟢 [^cursor] | 🟢 |
| Tablet / stylus input (pressure, tilt, tool) | 🔴 | 🔴 [^stylus] | 🟢 |
| Keyboard layout groups | 🔴 | 🔴 | 🟢 |
| Inhibit system shortcuts (grab all keys) | 🔴 | 🔴 | 🟢 |
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
| Browser tab title, favicon | 🔴 | 🟢 [^tabtitle] | ⚪ |
| Client-side decorations | 🟢 | 🟢 | 🟢 |
| Server-side decorations | ⚪ | ⚪ [^deco] | 🟢 |
| Multiple top-level windows | 🟢 | 🟢 | 🟢 |
| Window transparency / RGBA | 🟢 | 🟢 | 🟢 |
| **Window management** | | | |
| Interactive move / resize | 🟡 | 🟢 [^moveresize] | 🟢 |
| Maximize | 🟡 | 🟢 [^maximize] | 🟢 |
| Modal dialogs (dim + block parent) | 🔴 | 🟢 [^modal] | 🟢 |
| WM stacking & workspace hints (keep above/below, lower, sticky) | 🔴 | 🟡 [^keepabove] | 🔴 |
| Server window menu (`show_window_menu`) | 🔴 | 🔴 | 🟡 |
| Tiled-edge constraints | 🔴 | 🔴 | 🟢 |
| Startup notification / window handle export (xdg-activation) | 🔴 | 🔴 | 🟢 |
| **UI surfaces** | | | |
| Tooltips | 🟢 | 🟢 [^tooltip] | 🟢 |
| Popovers / autohide popups | 🟢 | 🟢 [^popup] | 🟢 |
| **Platform integration** | | | |
| `gtk_show_uri` / open external URIs | 🔴 | 🟢 [^uri] | 🟢 |
| Desktop dark mode / color scheme | 🔴 | 🟢 [^colorscheme] | 🟢 [^settings] |
| System tray / status icon | 🔴 | 🔴 | 🟡 [^tray] |
| Accessibility bridge | 🔴 | 🔴 | 🟢 [^a11y] |
| Desktop accent color | 🔴 | 🔴 | 🟢 [^settings] |
| Desktop / installed fonts | 🟡 [^fonts] | 🟡 [^fonts] | 🟢 [^settings] |

[^remote]: A `gtk4-broadwayd` daemon serves the running app over HTTP/WebSocket; any browser on any OS connects, no client install. Native backends draw to a local display only.

[^tabtitle]: The fork sets the browser tab's title and favicon from the app's title and icon. See [Tab title & favicon](display.md#tab-title-and-favicon).

[^moveresize]: Stock mis-places a "centered" dialog hard against the left edge (x=0) and can snap or grow it when resizing near an edge after a move; the fork centers and stabilizes the geometry (#61).

[^maximize]: Double-click-titlebar maximize toggles cleanly on the fork but is buggy on stock. Minimize is a no-op (no shell to minimize into) and GTK-level fullscreen is unimplemented (browser F11 covers it).

[^modal]: Stock tracks `modal_hint` but never enforces it - the parent isn't dimmed and hover leaks through. The fork adds a client-side scrim that dims and blocks the parent, matching libadwaita's backdrop.

[^keepabove]: The fork adds keep-above (always-on-top) as a per-surface flag via `gdk_broadway_surface_set_keep_above()` / the `SET_KEEP_ABOVE` op; it's set-only (no re-lower) and there's no below/lower/sticky. Wayland exposes no client stacking control at all. See [Always-on-top](../internals/input-region.md#always-on-top).

[^clip]: Stock upstream Broadway ships no `GdkClipboard`. The fork adds one, bridging the browser clipboard in both directions. See [Clipboard](input.md#clipboard).

[^rich]: Text-only by design; the read path rejects non-text. No image clipboard support.

[^primary]: Browsers expose no JS API for the PRIMARY selection.

[^dnd]: Broadway's DnD backend is still a stub. Within-app drag works only as far as stock does; the fork does not touch real DnD.

[^touchstock]: Stock Broadway delivers touch as mouse events, so GTK's touch text UI (gated on a real touchscreen device) never fires. The fork creates one. See [Touch interface](input.md).

[^zoom]: Whole-UI zoom 0.25x-5x, persisted per-origin, verified on mobile Firefox. See [Pinch to zoom](input.md#pinch-to-zoom).

[^stylus]: A stylus works as plain pointer/touch input; pressure, tilt, and tool identity are not bridged.

[^osk]: Summoned by focusing a hidden input on GTK's keyboard hint (`SET_SHOW_KEYBOARD`); an 80-space buffer keeps GBoard emitting backspace.

[^cursor]: Stock Broadway always shows the default arrow. The fork forwards GTK's per-surface cursor to the browser's CSS `cursor` (resize edges, text, links, ...). See [Dynamic cursor](input.md#dynamic-cursor).

[^ime]: Non-Latin / CJK / gesture-typed text commits via composition events; single backspace/delete is bridged. Word-level autocorrect/replacement isn't - with no mirror of GTK's buffer/caret, the old word can't be removed, so a correction appends instead of replacing.

[^smooth]: Stock forwards only discrete wheel steps. The fork sends pixel-precise deltas, so scrolling is smooth - minus kinetic fling, which is dropped (it animates many frames the remote framebuffer renders choppily; the browser already drives the deltas).

[^scroll]: The wheel-vs-surface unit is inferred from the browser's `deltaMode`, not the real device - browser-dependent (Chromium blurs wheel vs touchpad) - and every scroll rides the core pointer, so there's no `GDK_SOURCE_TOUCHPAD` for an app to branch on.

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

[^popup]: The fork fixes autohide dismiss-on-tap, `GtkDropDown` correct-item selection, the menu-tap freeze, and nested-submenu navigation (a [grab stack](../internals/input-region.md#grab-stack) so a submenu nests over its parent instead of clobbering it).

[^uri]: The fork routes external URIs to the browser's `window.open`, hooked through `gtk_show_uri`. See [Opening links](input.md#opening-links).

[^tray]: GTK4 has no status-icon API (`GtkStatusIcon` was removed). An app can still expose a D-Bus `StatusNotifierItem` via an appindicator library, shown by KDE natively or GNOME with the AppIndicator extension - backend-independent. A browser-tunneled Broadway session has no shell to host one.

[^a11y]: AT-SPI over D-Bus on the Linux backends, AccessKit on Windows/macOS. Broadway has no a11y bridge.

[^settings]: Dark-mode / accent / font settings come from the xdg settings portal on Wayland, XSETTINGS on X11, and AppKit (`NSAppearance`) on macOS. Stock Broadway has no desktop session and bridges none of them; the fork bridges color scheme only, over the browser's appearance portal.

[^colorscheme]: broadwayd serves the browser's `prefers-color-scheme` over an `org.freedesktop.appearance` portal, so plain GTK (`gtk-interface-color-scheme`) and libadwaita apps follow the browser's dark/light. `BROTWAY_COLOR_SCHEME` overrides it per process. See [Display & rendering](display.md).

[^fonts]: Installed fonts render through fontconfig, so text uses the host's fonts on a `run-host` or shared-config session; neither Broadway backend bridges the desktop's configured UI font, hinting, or antialiasing over a settings portal.
