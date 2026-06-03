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

## Known issues

- GTK: BROADWAY_OP_ROUNDTRIP storm (upstream)
- Touch: tab bar is not scrollable (probably Nicotine)
- Touch: emoji widget is ugly and slow

## Tested configurations

- Docker ubuntu:24.04, GTK 4.14.5
- Desktop: Firefox 151.0.2, Chromium 148.0.7778.178
- Android: Firefox Beta 152.0b4, Google Chrome 146.0.7680.119
- HTTP localhost and HTTPS behind Traefik in Docker Swarm

## Not tested

- iOS
- Mixed devices (laptops with touchscreen)
