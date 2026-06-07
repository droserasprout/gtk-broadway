# Clipboard

Stock Broadway has no clipboard. The fork bridges `GdkClipboard` <-> `gtk4-broadwayd` <-> the
browser's `navigator.clipboard`, both ways.

**Deploy:** spans `libgtk` (the new `gdkclipboard-broadway.c`, `gdkbroadway-server.c`) and
broadwayd (`broadwayd.c`, `broadway.js`) - full image rebuild and app restart.

## Two directions, two mechanisms

A browser accepts clipboard writes freely but hands out its contents only on request, so the two
paths differ.

**Copy / cut (guest -> host)** is push-based:

1. GTK claims the clipboard (Ctrl+C/X, right-click Copy, a custom Copy action).
2. The app sends `BROADWAY_OP_SET_CLIPBOARD` (op 17) with the text.
3. `broadway.js` calls `navigator.clipboard.writeText()`.

**Paste (host -> guest)** is request/reply:

1. The app sends `BROADWAY_OP_REQUEST_CLIPBOARD` (op 18) with a serial.
2. `broadway.js` reads the browser clipboard and replies with `BROADWAY_EVENT_CLIPBOARD_CONTENTS`
   (event 15).
3. The daemon routes the reply back to the one client that asked, by request serial.

The new file `gdkclipboard-broadway.c` implements the GDK side of both paths.

## Details

- **Per-client request table.** The daemon tracks outstanding paste requests by id, so with
  several browsers connected a reply reaches the requester and not another tab.
- **5-second read timeout** so a closed tab can't hang a pending paste.
- **Hidden textarea.** `client.html` carries an offscreen textarea; `broadway.js` captures a native
  paste into it, which hands over clipboard text without a permission prompt. (The offscreen
  clipboard/OSK helpers live on `body`, outside the zoomed `#zoomRoot`.)
- **`GtkTextView` fallback.** Some selections don't yield a plain string directly; the bridge
  serializes them to `text/plain`.
- **Length clamps.** Both the daemon `SET_CLIPBOARD` path (`broadwayd.c`) and the client
  `CLIPBOARD` reply (`gdkbroadway-server.c`) clamp the wire `len` to the framed message size, then
  to `BROADWAY_CLIPBOARD_MAX_SIZE` (16 MiB). `ensure_recv_capacity` doubles overflow-safe with a
  fatal guard on absurd frame sizes, rather than wrapping a `guint32` to 0.

## Touch keyboard paste

A paste from the on-screen keyboard never triggers GTK's own clipboard request. It is captured on
the hidden input and committed into the focused widget as Unicode key events (`commitTextToGtk`).
Control characters map to named GDK keysyms (`\n` -> Return `0xFF0D`, `\t` -> Tab `0xFF09`) so a
multi-line snippet produces real line breaks. See [Touch interface](touch.md) for the IME path.

## Limitations

- **PRIMARY selection / middle-click paste** is not bridged - browsers expose no JS API for it
  (WONTFIX).
- **Insecure origins.** `navigator.clipboard` needs a secure context; over plain `http://` copy may
  silently fail outside a user gesture. Serve over `https://` or `http://localhost`.
