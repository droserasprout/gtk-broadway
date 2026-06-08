# Clipboard

Stock Broadway has no clipboard. The fork bridges `GdkClipboard` <-> `gtk4-broadwayd` <-> the
browser's `navigator.clipboard`, both ways.

## Two directions, two mechanisms

A browser accepts clipboard writes freely but hands out its contents only on request, so the two
paths differ.

Copy and cut (guest to host) is push-based:

1. GTK claims the clipboard (Ctrl+C/X, right-click Copy, a custom Copy action).
2. The app sends `BROADWAY_OP_SET_CLIPBOARD` (op 17) with the text.
3. `broadway.js` calls `navigator.clipboard.writeText()`.

Paste (host to guest) is request/reply:

1. The app sends `BROADWAY_OP_REQUEST_CLIPBOARD` (op 18) with a serial.
2. `broadway.js` reads the browser clipboard and replies with `BROADWAY_EVENT_CLIPBOARD_CONTENTS`
   (event 15).
3. The daemon routes the reply back to the one client that asked, by request serial.

The new file `gdkclipboard-broadway.c` implements the GDK side of both paths.

## Details

The daemon keeps a per-client request table, tracking outstanding paste requests by id. With several
browsers connected, a reply reaches the requester rather than another tab. A 5-second read timeout
keeps a closed tab from hanging a pending paste.

Paste relies on a hidden textarea. `client.html` carries an offscreen textarea, and `broadway.js`
captures a native paste into it, which hands over clipboard text without a permission prompt. (The
offscreen clipboard and OSK helpers live on `body`, outside the zoomed `#zoomRoot`.) Some selections
don't yield a plain string directly, so the bridge falls back to serializing them to `text/plain`,
the `GtkTextView` case.

Both paths clamp length. The daemon `SET_CLIPBOARD` path (`broadwayd.c`) and the client `CLIPBOARD`
reply (`gdkbroadway-server.c`) clamp the wire `len` to the framed message size, then to
`BROADWAY_CLIPBOARD_MAX_SIZE` (16 MiB). `ensure_recv_capacity` doubles overflow-safe with a fatal
guard on absurd frame sizes, rather than wrapping a `guint32` to 0.

## Touch keyboard paste

A paste from the on-screen keyboard never triggers GTK's own clipboard request. It is captured on
the hidden input and committed into the focused widget as Unicode key events (`commitTextToGtk`).
Control characters map to named GDK keysyms (`\n` -> Return `0xFF0D`, `\t` -> Tab `0xFF09`) so a
multi-line snippet produces real line breaks. See [Touch interface](touch.md) for the IME path.

## Limitations

The PRIMARY selection and middle-click paste aren't bridged, since browsers expose no JS API for it
(WONTFIX).

`navigator.clipboard` needs a secure context. Over plain `http://`, copy may silently fail outside a
user gesture, so serve over `https://` or `http://localhost`.
