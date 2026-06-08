# Opening links

Clicking a link in a headless Broadway container logged *"No known URI provider available"* and did
nothing. There is no system URI handler, so `Gio.AppInfo.launch_default_for_uri` and the
`webbrowser` fallback both fail. The only thing that can open a tab is the browser viewing the
WebUI, so we route the URI over the Broadway protocol to it.

## The op

`BROADWAY_OP_OPEN_URI` (21) mirrors `SET_CLIPBOARD`, using the same `len + bytes` framing. The path:

```
_gdk_broadway_server_open_uri (gdkbroadway-server.c)
  -> BROADWAY_REQUEST_OPEN_URI (broadwayd.c, len clamped to the framed request)
  -> broadway_server_open_uri -> broadway_output_open_uri  (op + serial + len + bytes)
  -> broadway.js  case BROADWAY_OP_OPEN_URI
  -> window.open(uri, "_blank", "noopener")
```

New enum values are appended at the end so existing wire numbers don't shift.

## Reaching it from the app

Broadway has no introspection namespace. The `Gdk` GIR is built from `gdk_public_headers` only;
X11 and Wayland get their own GIRs, Broadway gets none. That means Python can't call a new Broadway
GDK function directly. Instead, a new public `gdk_broadway_display_show_uri()` (`gdkdisplay-broadway.c`)
is reached through an introspectable GTK choke point. `gtk_show_uri_full`
(`gtk/deprecated/gtkshow.c`) short-circuits when `GDK_IS_BROADWAY_DISPLAY`, sending the op and
returning success immediately, skipping the app-launch-context path. That one spot covers the
deprecated `gtk_show_uri`, the modern `GtkUriLauncher.launch` (its non-portal branch calls
`gtk_show_uri_full`), and `GtkLabel`/`GtkLinkButton` auto-link activation.

App-side, a URI-opening helper can add a `GDK_BACKEND == "broadway"` branch that calls
`Gtk.show_uri(None, uri, 0)` before the GIO path, and falls back gracefully on unpatched GTK.

## Caveats

- It runs inside the WebSocket message handler rather than a user gesture, so a popup blocker may
  catch it.
- `file://` URIs get routed too, but browsers block `window.open("file://...")` from an http(s)
  origin. That's no worse than the prior failure.
