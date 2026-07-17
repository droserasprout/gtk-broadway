# Open URI implementation

The user-facing summary is in [Opening links](../features/input.md#opening-links). The only thing that can open a tab is the browser viewing the WebUI, so the URI is routed over the Broadway protocol to it.

## The op

`BROADWAY_OP_OPEN_URI` (21) mirrors `SET_CLIPBOARD`, using the same `len + bytes` framing. The path:

```text
_gdk_broadway_server_open_uri (gdkbroadway-server.c)
  -> BROADWAY_REQUEST_OPEN_URI (broadwayd.c, len clamped to the framed request)
  -> broadway_server_open_uri -> broadway_output_open_uri  (op + serial + len + bytes)
  -> broadway.js  case BROADWAY_OP_OPEN_URI
  -> window.open(uri, "_blank", "noopener")
```

New enum values are appended at the end so existing wire numbers don't shift; see [Wire protocol](protocol.md).

## Reaching it from the app

Broadway has no introspection namespace - the `Gdk` GIR is built from public headers only, so Python can't call a new Broadway GDK function directly. Instead, a new public `gdk_broadway_display_show_uri()` is reached through an introspectable choke point: `gtk_show_uri_full` (`gtk/deprecated/gtkshow.c`) short-circuits when `GDK_IS_BROADWAY_DISPLAY`, sending the op and returning success. That one spot covers `gtk_show_uri`, `GtkUriLauncher.launch` (non-portal branch), and `GtkLabel`/`GtkLinkButton` auto-link activation.
