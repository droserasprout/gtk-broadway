# Adding a wire op

An end-to-end recipe for extending the Broadway wire protocol, using the open-URI feature as the spine. The feature's own story is in [Open URI implementation](../internals/open-uri.md).

## 1. Pick the direction(s)

Broadway speaks [three message vocabularies](../internals/architecture.md#three-message-vocabularies): `BroadwayRequest` (app -> daemon), `BROADWAY_OP_*` (daemon -> browser), `BROADWAY_EVENT_*` (browser -> daemon -> app). Open URI is the minimal case: one request, one display op, no event back.

## 2. Append the enum values

All wire constants live in `gdk/broadway/broadway-protocol.h`. Append at the **end** of each enum - never renumber, or every deployed daemon/browser pair breaks ([Wire protocol](../internals/protocol.md)):

```c
  BROADWAY_OP_OPEN_URI = 21,        /* BroadwayOpType */
  BROADWAY_REQUEST_OPEN_URI,        /* BroadwayRequestType */
```

A variable-length payload uses the `len + bytes` framing (the `SET_NODES` / `SET_CLIPBOARD` shape) and a member in the `BroadwayRequest` union:

```c
typedef struct {
  BroadwayRequestBase base;
  guint32 len;
  char uri[1];
} BroadwayRequestOpenUri;
```

## 3. App side: the send function (libgtk)

`gdkbroadway-server.c` marshals the request onto the local socket (declare it in `gdkbroadway-server.h`):

```c
void
_gdk_broadway_server_open_uri (GdkBroadwayServer *server,
                               const char        *uri)
```

It clamps `len` to `BROADWAY_CLIPBOARD_MAX_SIZE`, sizes the message as `G_STRUCT_OFFSET (BroadwayRequestOpenUri, uri) + len`, pads to 4 bytes (so the daemon's framing stays aligned), and sends with `gdk_broadway_server_send_message_with_size (..., BROADWAY_REQUEST_OPEN_URI, -1)`.

## 4. Daemon dispatch

`broadwayd.c` routes requests in the `client_handle_request` switch. Reject a request framed smaller than its fixed header, then clamp the wire `len` against the framed size before reading - never trust the client-supplied length:

```c
case BROADWAY_REQUEST_OPEN_URI:
  if (request->base.size >= G_STRUCT_OFFSET (BroadwayRequestOpenUri, uri))
    {
      gsize max = request->base.size -
                  G_STRUCT_OFFSET (BroadwayRequestOpenUri, uri);
      guint32 len = request->open_uri.len > max
                    ? (guint32) max : request->open_uri.len;
      broadway_server_open_uri (server, request->open_uri.uri, len);
    }
```

## 5. Daemon to browser

`broadway_server_open_uri` (`broadway-server.c`) calls `broadway_output_open_uri` when a browser is attached, then flushes. The writer in `broadway-output.c` emits the op header (op byte + serial) and the payload:

```c
  write_header (output, BROADWAY_OP_OPEN_URI);
  append_uint32 (output, (guint32) len);
  g_string_append_len (output->buf, uri, len);
```

## 6. Browser side

Mirror the constant at the top of `gdk/broadway/broadway.js` and add a case to the op switch in `handleCommands`:

```js
case BROADWAY_OP_OPEN_URI:
    var _uri = new TextDecoder("utf-8").decode(cmd.get_data());
    window.open(_uri, "_blank", "noopener");
    break;
```

`cmd.get_data()` reads the `len + bytes` framing. Sanity-check before building: `node --check gdk/broadway/broadway.js`.

## 7. If the app needs to call it

Broadway-only GDK API ships no GIR, so Python apps can't call your new function directly. Route it through an **introspectable GTK choke point**. For open URI, the public wrapper `gdk_broadway_display_show_uri` is reached via a short-circuit in `gtk_show_uri_full` (`gtk/deprecated/gtkshow.c`):

```c
  if (GDK_IS_BROADWAY_DISPLAY (display))
    {
      gdk_broadway_display_show_uri (GDK_BROADWAY_DISPLAY (display), uri);
```

That one spot covers `gtk_show_uri`, `GtkUriLauncher`, and auto-link activation - see [Open URI implementation](../internals/open-uri.md).

## 8. Iterating

The change spans both halves of the [broadwayd vs libgtk split](from-source.md#iterating-broadwayd-vs-libgtk):

- Daemon C + `broadway.js` (steps 4-6): incremental `ninja`, restart `gtk4-broadwayd`, reload the tab.
- libgtk (steps 3, 7): rebuild `libgtk-4.so` and restart the app.

## 9. Document it

Add the new op/request/event rows to [Wire protocol](../internals/protocol.md) - CI checks that page's tables against `broadway-protocol.h`, so a new enum value without a matching row fails the docs build.

## Landing it

Develop on a `<topic>` branch off `4.22.4-brotway`. Branch naming, style, and the PR flow are in [Contributing](contributing.md).
