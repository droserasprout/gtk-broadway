# GTK Broadway fork

A fork of GTK **4.14.5** that adds bidirectional text clipboard and
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

## Known issues

- Touch: tab bar (gtknotebook) is not draggable
- Touch: emoji widget is ugly and slow
- Touch: a two-finger pinch over selectable text / list rows can leak a tap to
  the widget under the fingers — the press registers before the gesture is known
  to be a pinch. Deferring the press to prevent it added too much tap/selection
  latency (a pinch's first finger is indistinguishable from a tap until the
  second lands), so the leak is left in place.

## Tested configurations

- Docker: `ubuntu:24.04`, GTK 4.14.5
- Desktop: Firefox 151.0.2, Chromium 148.0.7778.178
- Android: Firefox Beta 152.0b4, Google Chrome 146.0.7680.119
- HTTP localhost and HTTPS behind Traefik in Docker Swarm

## Not tested

- iOS
- Mixed devices (laptops with touchscreen)
