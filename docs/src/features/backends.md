# Backend comparison

Where our Broadway fork sits next to the other GTK4 backends, feature by feature. The fork column
is `4.22.2-fork` (a branch, 28 commits on top of the stock `4.22.2` tag). "Broadway (stock)" is
upstream unpatched Broadway, the baseline the fork improves on.

Legend: **🟢** full support, **🟡** partial/workaround/caveats, **🔴** not supported.

| Feature | Wayland | X11 | Win32 | macOS | Broadway (stock) | Broadway (fork) |
|---------|:---:|:---:|:---:|:---:|:---:|:---:|
| Text clipboard (host read/write) | 🟢 | 🟢 | 🟢 | 🟢 | 🔴 | 🟢 [^clip] |
| Rich clipboard / images / mimetypes | 🟢 | 🟢 | 🟢 | 🟢 | 🔴 | 🔴 [^rich] |
| PRIMARY selection (middle-click) | 🟢 | 🟢 | 🔴 | 🔴 | 🔴 | 🔴 [^primary] |
| Drag-and-drop within app | 🟢 | 🟢 | 🟢 | 🟢 | 🟡 | 🟡 |
| Drag-and-drop cross-process | 🟢 | 🟢 | 🟢 | 🟢 | 🔴 | 🔴 [^dnd] |
| Touch input / touchscreen events | 🟢 | 🟢 | 🟢 | 🟢 | 🔴 [^touchstock] | 🟢 |
| Touch text-selection UI (handles + bubble) | 🟢 | 🟢 | 🟢 | 🟢 | 🔴 | 🟢 |
| Multi-touch gesture (pinch-zoom UI) | 🟢 | 🟢 | 🟡 | 🟢 | 🔴 | 🟢 [^zoom] |
| On-screen keyboard sync | 🟢 | 🟢 | 🟢 | 🟢 | 🔴 | **🟡** [^osk] |
| IME / non-Latin / preedit | 🟢 | 🟢 | 🟢 | 🟢 | 🔴 | **🟡** [^ime] |
| OpenGL rendering | 🟢 | 🟢 | 🟢 | 🔴 | 🔴 | 🔴 [^render] |
| Vulkan rendering | 🟢 | 🟡 | 🟢 | 🟢 | 🔴 | 🔴 [^render] |
| Cairo / software rendering | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 |
| HiDPI integer scaling | 🟢 | 🟢 | 🟢 | 🟢 | 🟡 | 🟢 [^hidpi] |
| Fractional scaling | 🟢 [^frac] | 🔴 | 🔴 | 🟢 [^frac] | 🔴 | 🔴 |
| Client-side decorations | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 |
| Server-side decorations | 🟢 | 🔴 | 🟢 | 🟢 | n/a | n/a [^deco] |
| Multiple monitors | 🟢 | 🟢 | 🟢 | 🟢 | 🟡 | 🟡 |
| Window transparency / RGBA | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 |
| Tooltips | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 [^tooltip] |
| Popovers / autohide popups | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 [^popup] |
| `gtk_show_uri` / open external URIs | 🟢 | 🟢 | 🟢 | 🟢 | 🔴 | 🟢 [^uri] |
| System tray / status icon | 🔴 | 🔴 | 🔴 | 🔴 | 🔴 | 🔴 |
| Multiple top-level windows | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 | 🟢 |

## What the fork adds over stock Broadway

The bolded cells above are where the fork diverges from upstream Broadway. In short: clipboard,
real touch (events, text-selection UI, pinch-zoom), OSK and IME bridging, HiDPI reflow, and opening
external links. Cross-process DnD, rich/image clipboard, PRIMARY selection, and GPU rendering stay
unsupported, same as stock.

## Beyond the fork: other backend differences

These are platform / windowing-protocol capabilities that differ across backends but that the fork
does **not** change. The Broadway column is therefore the same for stock and fork. They are listed
so the picture is honest about where Broadway sits even after our patches: it is a browser-tunneled
backend with no window manager, no desktop session, and no GPU, so most of these never apply.

| Capability | Wayland | X11 | Win32 | macOS | Broadway |
|------------|:---:|:---:|:---:|:---:|:---:|
| Keep-above / keep-below stacking | 🔴 | 🟢 | 🟢 | 🔴 | 🔴 |
| Sticky (on all workspaces) | 🔴 | 🟢 | 🔴 | 🔴 | 🔴 |
| Lower window below siblings | 🔴 | 🟢 | 🔴 | 🟢 | 🔴 |
| Inhibit system shortcuts (grab all keys) | 🟢 | 🟢 | 🟢 | 🔴 | 🔴 |
| Startup notification / activation token | 🟢 | 🟢 | 🔴 | 🔴 | 🔴 |
| Server window menu (`show_window_menu`) | 🟡 | 🟢 | 🟢 | 🔴 | 🔴 |
| Tiled-edge constraints | 🟢 | 🟢 | 🔴 | 🔴 | 🔴 |
| Window handle export (xdg-activation / dialogs) | 🟢 | 🟢 | 🔴 | 🔴 | 🔴 |
| Smooth / touchpad scroll-source detection | 🟢 | 🟢 | 🟢 | 🟡 | 🟡 [^scroll] |
| Keyboard layout groups | 🟢 | 🟡 | 🔴 | 🔴 | 🔴 |
| DMA-BUF texture import | 🟢 [^dmabuf] | 🔴 | 🔴 | 🔴 | 🔴 |
| Graphics offload / video subsurfaces | 🟢 [^offload] | 🔴 | 🔴 | 🔴 | 🔴 |
| HDR / wide-gamut color (`GdkColorState`) | 🟢 | 🔴 | 🔴 | 🟡 | 🔴 |
| Presentation-time / vsync feedback | 🟢 | 🟡 | 🟡 | 🟡 | 🔴 |
| Accessibility bridge | 🟢 [^a11y] | 🟢 [^a11y] | 🟢 [^a11y] | 🟢 [^a11y] | 🔴 |
| Desktop settings (dark mode / accent / fonts) | 🟢 [^settings] | 🟢 [^settings] | 🟡 | 🟢 [^settings] | 🔴 |

GL is reached through a different API per platform (EGL on Wayland, GLX or EGL on X11, WGL or EGL on
Win32, none on macOS which renders via Metal); see the OpenGL / Vulkan rows in the matrix above.

[^clip]: Stock upstream Broadway ships no `GdkClipboard` at all. The fork adds
    `gdkclipboard-broadway.c` with `SET_CLIPBOARD` / `REQUEST_CLIPBOARD` ops bridging
    `navigator.clipboard`, both directions. See [Clipboard](clipboard.md).

[^rich]: Text-only by design (`text/plain;charset=utf-8`); the read path rejects non-text. No
    image / `GdkTexture` / `GdkPixbuf` support.

[^primary]: No `GDK_SELECTION_PRIMARY` handling; browsers expose no JS API for the PRIMARY
    selection.

[^dnd]: `gdkdnd-broadway.c` is still a stub. Within-app drag works only as far as stock does; the
    fork does not touch real DnD.

[^touchstock]: Stock Broadway delivers touch as a `core_pointer` mouse. GTK's touch text UI gates
    on `GDK_SOURCE_TOUCHSCREEN`, which never fires. The fork creates a real touchscreen device. See
    [Touch interface](touch.md).

[^zoom]: Whole-UI zoom 0.25x-5x, JS-side `zoomFactor`, persisted per-origin in `localStorage`,
    verified on mobile Firefox. See [Pinch to zoom](zoom.md).

[^osk]: Show/hide mostly synced; residual flicker on mixed selection-bubble focus state.

[^ime]: Non-Latin / CJK / gesture-typed text committed via `commitTextToGtk` / `compositionend`.
    Autocorrect deletions are not bridged.

[^render]: Broadway has no GPU context. It renders through `gskbroadwayrenderer` (server-side node
    tree) with a Cairo fallback; the fork does not change this.

[^hidpi]: The fork drives genuine reflow plus crisp scale on zoom. Stock only does integer
    `devicePixelRatio` sharpening, no resize. See [Scaling & HiDPI](scaling.md).

[^frac]: Via `fractional-scale-v1` on Wayland and the native backing scale on macOS.

[^deco]: Broadway runs inside a browser tab; the browser window chromes it, so SSD/CSD is not
    meaningful in the usual sense.

[^tooltip]: The fork suppresses spurious `:hover` / tooltips on touch taps.

[^popup]: The fork fixes autohide dismiss-on-tap, `GtkDropDown` correct-item selection, and the
    menu-tap freeze (`REASSERT_POINTER`).

[^uri]: The fork adds `BROADWAY_OP_OPEN_URI` + `gdk_broadway_display_show_uri`, hooked through
    `gtk_show_uri_full` to `window.open(..., "_blank")`. See [Opening links](open-uri.md).

[^scroll]: Broadway forwards wheel scroll, but has no touchpad/source distinction or true smooth
    scroll; macOS reports everything as surface (smooth) scroll.

[^dmabuf]: Linux-only by construction (`gdkdmabuftexture.c`); imported via `zwp_linux_dmabuf` on
    Wayland. No other backend builds a dmabuf texture.

[^offload]: `gdksubsurface-wayland.c` only; lets video/textures bypass the compositor. No
    `gdksubsurface-*.c` exists for the other backends.

[^a11y]: AT-SPI over D-Bus on the Linux backends (`gtkatspicontext.c`), AccessKit on
    Windows/macOS (`gtkaccesskitcontext.c`, build-time `HAVE_ACCESSKIT`). Broadway has no a11y
    bridge.

[^settings]: Dark-mode / accent / font settings come from the xdg settings portal on Wayland,
    XSETTINGS on X11, and AppKit (`NSAppearance`) on macOS. Broadway has no desktop session to read
    from; theming is whatever CSS the app ships.
