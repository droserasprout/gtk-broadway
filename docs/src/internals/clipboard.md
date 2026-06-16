# Clipboard implementation

The user-facing summary is in [Clipboard](../features/input.md#clipboard). This page covers the wire paths and the daemon/client mechanics. The ops are `BROADWAY_OP_SET_CLIPBOARD` / `REQUEST_CLIPBOARD` and the `CLIPBOARD_CONTENTS` event; see [Wire protocol](protocol.md).

## Two paths

A browser accepts clipboard writes freely but hands out its contents only on request, so the two directions differ.

Copy and cut (guest -> host) is push-based:

1. GTK claims the clipboard (Ctrl+C/X, right-click Copy, a custom Copy action).
2. The app sends `BROADWAY_OP_SET_CLIPBOARD` with the text.
3. `broadway.js` calls `navigator.clipboard.writeText()`.

Paste (host -> guest) is request/reply:

1. The app sends `BROADWAY_OP_REQUEST_CLIPBOARD` with a serial.
2. `broadway.js` reads the browser clipboard and replies with `BROADWAY_EVENT_CLIPBOARD_CONTENTS`.
3. The daemon routes the reply back to the one client that asked, by request serial.

The new file `gdkclipboard-broadway.c` implements the GDK side of both paths.

## Routing and timeouts

The daemon keeps a per-client request table, tracking outstanding paste requests by id, so with several browsers connected a reply reaches the requester rather than another tab. A 5-second read timeout keeps a closed tab from hanging a pending paste.

Paste relies on a hidden textarea: `client.html` carries an offscreen textarea, and `broadway.js` captures a native paste into it, handing over clipboard text without a permission prompt. Some selections don't yield a plain string directly, so the bridge falls back to serializing them to `text/plain` (the `GtkTextView` case).

## Length clamps

Both paths clamp the wire `len` to the framed message size, then to `BROADWAY_CLIPBOARD_MAX_SIZE` (16 MiB). See [Size limits](protocol.md#size-limits).

## Touch keyboard paste

A paste from the on-screen keyboard never triggers GTK's own clipboard request. It is captured on the hidden input and committed into the focused widget as Unicode key events (`commitTextToGtk`). Control characters map to named GDK keysyms (`\n` -> Return `0xFF0D`, `\t` -> Tab `0xFF09`) so a multi-line snippet produces real line breaks. See [Touch implementation](touch.md) for the IME path.
