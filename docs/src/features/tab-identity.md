# Tab title & favicon

*New in v3.1*

Stock Broadway leaves the browser tab generic - a fixed title and no icon. The fork forwards the app's window title and icon to the page, so the tab identifies the running app.

## What works

- **Tab title** follows the GTK window title of the topmost real toplevel, so dialogs, menus, and popovers don't hijack it.
- **Favicon** follows the app's themed window icon.
- Both **persist across a page reload** - they're restored immediately on load, so a refresh doesn't flash the default before the session reconnects.
- Before any app connects, the tab shows a **`brotway`** default.

## Limitations

- Only the **topmost toplevel** sets the identity; a transient (dialog/popup) on top doesn't change it.
- An app with no window icon leaves the favicon empty - no fallback icon is invented.

> The title and icon ride `BROADWAY_OP_SET_TITLE` / `BROADWAY_OP_SET_ICON`, stored per-surface and replayed on reconnect. The client keeps the last pair in `sessionStorage` (the icon as a `data:` URL) so a reload restores it synchronously, before the WebSocket reconnects.
