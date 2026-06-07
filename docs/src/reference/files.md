# Changed files

Almost all of the fork is confined to `gdk/broadway/` (the GDK backend + the daemon + the browser
client). A small number of GTK widget files are touched too, because some touch behaviours can
only be fixed above the backend.

## Broadway backend - `gdk/broadway/`

| File | Role | What the fork changes |
|------|------|------------------------|
| `broadway-protocol.h` | wire definitions | new ops/events/requests; `is_popup` on `NEW_SURFACE`; clipboard size cap |
| `broadway-output.c` / `.h` | daemon -> browser encoding | encode the new ops (incl. `DEBUG_FLASH`) |
| `broadway-server.c` / `.h` | daemon request handling | new ops, session/reconnect, touch raise/focus gating, pointer recovery, deferred-enter timers; debug-menu spawn + stats channel ([Debug menu](debug-menu.md)); `Cache-Control: no-store` on served assets |
| `broadwayd.c` | daemon main | clipboard request routing, open-URI, session tokens, length clamps |
| `broadway.js` | browser client (~1k LOC of additions) | touch delivery, IME/OSK, clipboard bridge, pinch-zoom, reconnect, session handling; debug-menu trigger + paint-flash overlay; stale-id display-op guards |
| `client.html` | browser page | `touch-action: none`, `#zoomRoot` wrapper, hidden paste textarea, dark page background |
| `gdkbroadway-server.c` / `.h` | GDK-side daemon connection | clipboard reads/writes, session state, reconnect detection, length clamps |
| `gdkclipboard-broadway.c` | **new file** | the `GdkClipboard` <-> daemon bridge ([Clipboard](clipboard.md)) |
| `gdkdisplay-broadway.c` / `.h` | display init | scale hooks, keyboard show/hide, `gdk_broadway_display_show_uri()`; cross-time texture content-dedup cache ([Performance](performance.md)) |
| `gdkeventsource.c` | event dispatch | source touch from the touchscreen device; `case 3 -> GDK_TOUCH_CANCEL` |
| `gdksurface-broadway.c` | surface ops | window centering, input region, un-`static` the drag-surface ctor |
| `gdkdnd-broadway.c` | drag-and-drop | give `GdkDrag` a real drag surface ([fixes a crash](#bugfixes-folded-in)) |
| `gdkprivate-broadway.h` | private decls | clipboard helpers, drag-surface ctor |
| `meson.build` | build | add `gdkclipboard-broadway.c` |

## GSK renderer - `gsk/broadway/`

| File | What the fork changes |
|------|------------------------|
| `gskbroadwayrenderer.c` | rasterize scale/rotate via cairo at display resolution; invalidate node cache on scale change ([Scaling & HiDPI](scaling.md)); content-based node reuse ([Performance](performance.md)) |

## Debug tool - `tools/`

| File | Role | What the fork changes |
|------|------|------------------------|
| `gtk-broadway-debugmenu.c` | **new file** | the native `gtk4-broadway-debugmenu` window the daemon spawns ([Debug menu](debug-menu.md)) |
| `meson.build` | build | add the `gtk4-broadway-debugmenu` binary |

## GTK widgets / GDK core - touch fixes that can't live in the backend

| File | Why |
|------|-----|
| `gdk/gdksurface.c` | `check_autohide` falls back to the logical pointer's grab so outside taps dismiss popovers; trust `GDK_TOUCH_BEGIN` inside an autohide popup |
| `gtk/gtkgesture.c` | set `data->event` before inserting the point - fixes a NULL-deref crash across all gesture deref sites |
| `gtk/gtkdropdown.c` | `row_activated` honors the activated `position` instead of the stale hover selection (touch has no hover) |
| `gtk/gtktext.c`, `gtk/gtktextview.c` | rebuild the selection bubble after Select-All so Copy reappears |
| `gtk/gtknotebook.c` | touch pixel-scroll / drag-to-pan the tab strip; native CSS-undershoot edge-fade ([Notebook tabs](notebook.md)) |
| `gtk/theme/Default/_common.scss` | the tab-strip `undershoot` edge-fade gradient ([Notebook tabs](notebook.md)) |
| `gtk/deprecated/gtkshow.c` | `gtk_show_uri_full` short-circuits to the Broadway URI op ([Opening links](open-uri.md)) |

> These are flagged in each feature chapter as **libgtk** changes - they need the library rebuilt
> and the app restarted, not just a `broadwayd` refresh.

## Bugfixes folded in

Two crash/correctness fixes are documented with their subsystem rather than as standalone
chapters:

- **Drag-and-drop SIGSEGV** - dragging a text selection (easy via touch) crashed because Broadway's
  `GdkDrag` had no drag surface. Fixed by reusing Broadway's existing move/resize drag-surface type
  (`gdksurface-broadway.c`, `gdkprivate-broadway.h`, `gdkdnd-broadway.c`). Reproduces on stock
  Broadway too; the fork's touch support just makes it trivial to trigger.
- **Selection-bubble reopen SIGSEGV** - covered under [Touch interface](touch.md).
