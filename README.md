# GTK Broadway fork

A fork of GTK **4.22.2** that adds bidirectional text clipboard and
Android/touch support to the **Broadway** backend (GTK rendered in a web browser
via `gtk4-broadwayd`). Upstream GTK docs: <https://gitlab.gnome.org/GNOME/gtk>.

## Features / Bugfixes

### Bidirectional text clipboard

- **GdkClipboard ⇄ gtk4-broadwayd ⇄ browser `navigator.clipboard` bridge** — a new
  clipboard backend, `gdk/broadway/gdkclipboard-broadway.c` (`GdkBroadwayClipboard`).
- **Copy guest → host** — push model: GTK claim → `SET_CLIPBOARD` → browser
  `writeText` (Ctrl+C / Ctrl+X, right-click-menu copy, custom "Copy" actions).
- **Paste host → guest** — request/reply: `REQUEST_CLIPBOARD` → browser →
  `CLIPBOARD_CONTENTS` event, routed back to the one client that asked
  (Ctrl+V, right-click paste).
- **New wire protocol** — ops/events `SET_CLIPBOARD`, `REQUEST_CLIPBOARD`,
  `CLIPBOARD_CONTENTS` (`broadway-protocol.h`, `broadway-output.c`).
- **Per-client request-id table** in the daemon — routes a reply to the
  requesting client only, with cleanup on disconnect (`broadwayd.c`).
- **Growable receive buffer** on the client for variable-size clipboard replies
  (`gdkbroadway-server.c`).
- **GtkTextView selection copy** via `text/plain` serialization (for providers
  that don't yield a plain string).
- **Hidden-textarea trick** in JS to capture native paste without a permission
  popup (`broadway.js`, `client.html`).
- **Read-timeout + remote-reclaim** so a paste never hangs if the tab closed.

### Android / touch support

- **Touch delivered from the touchscreen device** — unlocks GTK's touch text UI
  (selection handles + Cut/Copy/Paste bubble), which gates on
  `GDK_SOURCE_TOUCHSCREEN` (`gdkeventsource.c`).
- **Non-passive touch listeners + `touchcancel` handling** — `preventDefault()`
  actually works, so jittery taps are no longer cancelled by the browser
  (`broadway.js`; `touch-action: none` in `client.html`).
- **On-screen keyboard show/hide** on Android, applied without a one-gesture lag.
- **Touch / snippet paste forwarded into GTK** as Unicode key events
  (`commitTextToGtk`).
- **Non-Latin / IME typed text forwarding** — Cyrillic / CJK / gesture-typed /
  autocorrect text, via `beforeinput` / composition events.
- **Autohide popovers dismiss on outside touch tap** — logical-pointer grab
  fallback in `check_autohide` (`gdk/gdksurface.c`).
- **Touch gestures survive surface repaints** — listeners re-attached to the
  touched node after GTK detaches the original DOM node; per-event dedupe.
- **No spurious `:hover` styling / tooltips on tap** — mouse hover crossings
  dropped from the touch path.
- **Empty input regions honored as click-through** (`SET_INPUT_REGION`), so text
  cursor/selection handles don't count as interactive popups.

### Touch interaction fixes

- **Selection-bubble Cut / Copy / Paste work** — on touch, only genuine
  toplevels are raised + focused (never popups), so the bubble isn't torn down
  before its action fires (`broadway-server.c`).
- **Menu-item / GtkDropDown freeze fixed** — `check_autohide` touch guard (a tap
  *inside* a popup activates instead of dismissing it) plus browser-routed
  `REASSERT_POINTER` pointer-focus recovery.
- **GtkDropDown selects the tapped row** (not always the first) — `row_activated`
  honors the `position` from the activate signal (`gtk/gtkdropdown.c`).
- **Reopen-bubble crash (SIGSEGV) fixed** — a gesture point with a not-yet-set
  event is no longer dereferenced (`gtk/gtkgesture.c`).
- **Copy button restored after Select-All** in editable fields — the selection
  bubble is rebuilt with the now-enabled actions (`gtk/gtktext.c`,
  `gtk/gtktextview.c`).
- **Tab strip (GtkNotebook) is pixel-scrolled on touch** — the tab bar is one
  continuous strip you drag to scroll pixel-for-pixel with the finger, without
  changing the page; the scroll position stays where you leave it (no snapping),
  tab widths stay natural, and edge fades (not arrows) hint at more tabs. A plain
  tap still selects a tab (selection is deferred to release so a drag can preempt
  it). Touch-only and Broadway-only, so desktop mouse/reorder is unchanged
  (`gtk/gtknotebook.c`).

### Touch pinch-zoom

- **Two-finger pinch zooms the whole UI** (0.25×–5×), mirroring desktop browser
  page-zoom: the client reports a logical screen size shrunk by the zoom factor
  and a render scale bumped by it, so GTK re-lays-out and re-renders **crisply**
  (a real reflow, not a stretched bitmap), while a CSS `transform: scale()` on
  the surface wrapper magnifies it to fill the viewport (`broadway.js`,
  `client.html`).
- **Smooth while pinching, crisp on release** — the wrapper scales live during
  the gesture; the crisp reflow is committed once the fingers lift.
- **Floats with the fingers** — during the gesture the magnification is anchored
  to the pinch midpoint and pans with it (Apple-like), instead of zooming from
  the top-left corner.
- **Per-client / per-device** — zoom state is local to each browser; desktop is
  untouched (it keeps using Firefox's native Ctrl+scroll page-zoom).
- **Survives a page refresh** — the committed zoom is persisted per origin in
  `localStorage` and restored before the first frame, so a reload (common on
  mobile) keeps the zoom instead of snapping back to 1×.
- **`GDK_TOUCH_CANCEL` plumbing** — a browser `touchcancel`, and the in-flight
  finger when a pinch begins, are forwarded as touch type 3 (mapped in
  `gdkeventsource.c`) so an aborted touch closes cleanly instead of completing as
  a tap. (This does not fully stop a pinch from leaking a tap to selectable text
  under the fingers — see Known issues.)

### Desktop

- **Horizontal two-finger swipe scrolls instead of navigating** — the browser's
  back/forward history gesture is suppressed (a non-passive standard `wheel`
  listener that `preventDefault()`s) and the swipe is forwarded to GTK as a
  left/right scroll, so a sideways swipe pans the widget under the pointer
  rather than leaving the app. Vertical wheel scrolling is unchanged
  (`broadway.js`, `gdk/broadway/gdkeventsource.c`).
- **Wheel over the notebook tab strip** — horizontal scroll pans the pixel-scroll
  strip without changing the page; vertical scroll switches the page and pans the
  strip so the now-active tab is fully in view. Pixel-scroll (Broadway top/bottom)
  strips only; off Broadway, stock scroll-to-switch is unchanged
  (`gtk/gtknotebook.c`).
- **No white background on load / zoom-out** — the page fills unpainted areas
  (page load, disconnect, the margins around a zoomed-out surface) with a
  theme-ish background following the browser's light/dark preference, instead of
  white (`client.html`).
- **New windows open centered** — a new toplevel is centered on the monitor
  instead of landing at the top-left (0,0) default. First map only (re-present
  keeps the user's position) and never for the maximized main window
  (`gdk/broadway/gdksurface-broadway.c`). Open windows are re-centered when the
  screen size changes (e.g. a zoom), so a centered window can't be stranded
  off-screen (`gdk/broadway/gdkdisplay-broadway.c`).
- **Drags survive the cursor leaving the widget** — while a mouse button is held
  (implicit pointer grab), surface-crossing enter/leave events are suppressed so
  the pointer stays confined to the grabbed surface, like an X11/GDK implicit
  grab. Fixes a scrollbar (or any) drag being interrupted when the cursor wanders
  off the widget. Explicit grabs (menus/popovers) are unaffected (`broadway.js`).

## Known issues

- WONTFIX: desktop: browsers expose no JS API for the X11/Wayland PRIMARY selection.
- WONTFIX: touch: pinch gesture leaks first tap. No solution found without introducing a single tap delay.
- Touch: emoji widget is ugly and slow

## Tested configurations

- Docker: `ubuntu:26.04`, GTK 4.22.2
- Desktop: Firefox 151.0.2, Chromium 148.0.7778.178
- Android: Firefox Beta 152.0b4, Google Chrome 146.0.7680.119
- HTTP localhost and HTTPS behind Traefik in Docker Swarm

## Not tested

- iOS
- Mixed devices (laptops with touchscreen)
