# GTK Broadway fork

A fork of GTK **4.22.2** adding missing features to the **Broadway** backend - GTK's HTML5 renderer that serves an app to web browser over a WebSocket.

- [Bidirectional clipboard](#clipboard)
- [Touch interface](#touch-interface)
- [Pinch to zoom](#pinch-to-zoom)
- [Open links in browser](#opening-links)
- [Draggable notebook tabs](#notebook-tabs)
- [Bugfixes](#bugfixes)

## Installation

A Debian package `gtk4-broadway-fork` built by CI on `ubuntu:26.04` and published to the rolling `4.22.2-latest` GitHub Release. It Depends/Replaces `libgtk-4-1` and `libgtk-4-bin`, overlaying only the patched `libgtk-4.so` and `gtk4-broadwayd` keeping the rest of GTK in place:

```sh
wget -O gtk.deb "https://github.com/droserasprout/gtk-broadway/releases/download/4.22.2-latest/libgtk4-broadway-fork_$(dpkg --print-architecture).deb"
apt-get install -y ./gtk.deb
apt-mark hold libgtk-4-1 libgtk-4-bin
```

Or build from source (broadway-only):

```sh
meson setup _build -Dbroadway-backend=true
ninja -C _build
```

## Features

### Clipboard

- `GdkClipboard` <-> `gtk4-broadwayd` <-> browser `navigator.clipboard` bridge (`gdk/broadway/gdkclipboard-broadway.c`).
- Copy guest -> host (push): GTK claim -> `SET_CLIPBOARD` -> browser `writeText`. Ctrl+C/X, right-click copy, custom Copy actions.
- Paste host -> guest (request/reply): `REQUEST_CLIPBOARD` -> browser -> `CLIPBOARD_CONTENTS`, routed back to the asking client. Ctrl+V, right-click paste.
- New wire ops `SET_CLIPBOARD` / `REQUEST_CLIPBOARD` / `CLIPBOARD_CONTENTS` (`broadway-protocol.h`, `broadway-output.c`).
- Per-client request-id table routes each reply to the asking client only, cleaned up on disconnect (`broadwayd.c`).
- Growable client receive buffer for variable-size replies (`gdkbroadway-server.c`).
- GtkTextView selection copy via `text/plain` serialization (for providers with no plain string).
- Hidden-textarea trick captures native paste with no permission popup (`broadway.js`, `client.html`).
- Read-timeout + remote-reclaim so paste never hangs if the tab closed.

## Touch interface

- Touch emitted from the touchscreen device, unlocking GTK's touch text UI (selection handles + Cut/Copy/Paste bubble) which gates on `GDK_SOURCE_TOUCHSCREEN` (`gdkeventsource.c`).
- Non-passive listeners + `touchcancel` handling - `preventDefault()` works, so the browser stops cancelling jittery taps (`broadway.js`; `touch-action: none` in `client.html`).
- On-screen keyboard show/hide on Android, with no one-gesture lag.
- Touch/snippet paste forwarded to GTK as Unicode key events (`commitTextToGtk`).
- Non-Latin/IME text forwarding (Cyrillic, CJK, gesture-typed, autocorrect) via `beforeinput`/composition events.
- Autohide popovers dismiss on outside tap - logical-pointer grab fallback in `check_autohide` (`gdk/gdksurface.c`).
- Touch gestures survive surface repaints - listeners re-attach to the touched node after GTK detaches it; per-event dedupe.
- No spurious `:hover`/tooltips on tap - mouse hover crossings dropped from the touch path.
- Empty input regions honored as click-through (`SET_INPUT_REGION`), so cursor and selection handles aren't treated as interactive popups.

### Pinch to zoom

- Two-finger pinch zooms the whole UI (0.25x-5x), like desktop page-zoom: the client shrinks the reported screen size and bumps the render scale, so GTK reflows and re-renders crisply (not a stretched bitmap); a CSS `transform: scale()` on the wrapper fills the viewport (`broadway.js`, `client.html`).
- Smooth while pinching, crisp on release - the wrapper scales live; the reflow commits once the fingers lift.
- Floats with the fingers - magnification anchors to the pinch midpoint and pans with it, instead of zooming from the top-left.
- Per-client - zoom state is local to each browser; desktop keeps native Ctrl+scroll page-zoom.
- Survives a page refresh - the zoom is saved per origin in `localStorage` and restored before the first frame, so a reload keeps it instead of snapping to 1x.
- `GDK_TOUCH_CANCEL` plumbing - a browser `touchcancel`, and the in-flight finger at pinch start, forward as touch type 3 (`gdkeventsource.c`) so an aborted touch closes cleanly instead of landing as a tap (doesn't fully stop the first-tap leak - see Known issues).

### Opening links

Clicking a link did nothing - the headless container has no system URI handler, so `Gio.AppInfo.launch_default_for_uri` and the `webbrowser` fallback both fail. Only the browser viewing the WebUI can open a tab, so the URI is routed to it over the Broadway protocol (mirroring `SET_CLIPBOARD`).

- New op `BROADWAY_OP_OPEN_URI` carries the URI client -> daemon -> browser, where `broadway.js` calls `window.open(uri, "_blank", "noopener")` (`broadway-protocol.h`, `gdkbroadway-server.c`, `broadwayd.c`, `broadway-output.c`, `broadway.js`).
- Reached via a GTK choke point: `gtk_show_uri_full` short-circuits on Broadway into a new public `gdk_broadway_display_show_uri()` - Broadway has no introspection namespace for a direct call (`gtk/deprecated/gtkshow.c`, `gdkdisplay-broadway.c`). Covers `gtk_show_uri`, `GtkUriLauncher.launch`, and GtkLabel/GtkLinkButton auto-links.

### Notebook tabs

- Tab strip pixel-scrolls on touch - drag the tab bar to scroll it pixel-for-pixel without changing the page. The position persists (no snapping), tab widths stay natural, and edge fades (not arrows) hint at more tabs. A plain tap still selects a tab (deferred to release so a drag can preempt it). Touch- and Broadway-only; desktop mouse/reorder is unchanged (`gtk/gtknotebook.c`).
- Wheel over the tab strip - horizontal scroll pans it without changing the page; vertical scroll switches the page and pans the new tab into view. Broadway top/bottom strips only; stock scroll-to-switch is unchanged elsewhere (`gtk/gtknotebook.c`).

### Bugfixes

#### Touch

- Selection-bubble Cut/Copy/Paste work - on touch only genuine toplevels are raised+focused (never popups), so the bubble isn't torn down before its action fires (`broadway-server.c`).
- Menu-item/GtkDropDown freeze fixed - `check_autohide` touch guard (a tap inside a popup activates instead of dismissing it) + browser-routed `REASSERT_POINTER` focus recovery.
- GtkDropDown selects the tapped row, not always the first - `row_activated` honors the activate signal's `position` (`gtk/gtkdropdown.c`).
- Reopen-bubble SIGSEGV fixed - a gesture point with a not-yet-set event is no longer dereferenced (`gtk/gtkgesture.c`).
- Copy button restored after Select-All - the selection bubble is rebuilt with the now-enabled actions (`gtk/gtktext.c`, `gtk/gtktextview.c`).

#### Desktop

- Horizontal two-finger swipe scrolls instead of navigating - the browser's back/forward gesture is suppressed (non-passive `wheel` listener that `preventDefault()`s) and forwarded to GTK as a left/right scroll, so a sideways swipe pans the widget instead of leaving the app. Vertical scroll unchanged (`broadway.js`, `gdk/broadway/gdkeventsource.c`).
- No white background on load/zoom-out - unpainted areas (load, disconnect, zoomed-out margins) fill with a theme-ish background following the browser's light/dark preference instead of white (`client.html`).
- New windows open centered (all clients) - a new toplevel centers on the monitor instead of the (0,0) top-left default. First map only (re-present keeps the user's position), never for the maximized main window (`gdk/broadway/gdksurface-broadway.c`); open windows re-center on screen-size change (e.g. a zoom) so they can't be stranded off-screen (`gdk/broadway/gdkdisplay-broadway.c`).
- Drags survive the cursor leaving the widget - while a button is held, enter/leave crossings are suppressed so the pointer stays confined to the grabbed surface (like an X11/GDK implicit grab), fixing a scrollbar drag interrupted when the cursor wanders off. Explicit grabs (menus/popovers) unaffected (`broadway.js`).

## Status

Broadway backend was [deprecated](https://www.phoronix.com/news/GTK-X11-Now-Deprecated) by GTK maintainers in 2025 (alongside X11) due to lack of maintenance, so I don't expect patches in this repo to ever land upstream. My goal is to easily patch existing applications, mostly built on stock Ubuntu docker images:

- `ubuntu:24.04` ships `4.14.5` - done ✅
- `ubuntu:26.04` ships `4.22.2` - in progress (this branch)

Upcoming GTK 5 likely won't be supported by this fork.

## Known issues

- WONTFIX: PRIMARY selection, middle-click paste. Desktop browsers don't expose JS API for it.
- Touch: Pinch and drag gestures leak first tap. No solution found yet without introducing delays.
- Touch: Emoji widget is ugly and slow.

## Tested configurations

- Desktop: Firefox 151.0.2, Chromium 148.0.7778.178
- Android: Firefox Beta 152.0b4, Google Chrome 146.0.7680.119
- HTTP localhost and HTTPS behind Traefik in Docker Swarm

## Not tested

- iOS
- Mixed devices (laptops with touchscreen)
