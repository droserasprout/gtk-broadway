# GTK Broadway fork

A fork of GTK that adds the missing pieces to the **Broadway** backend - GTK's HTML5 renderer
that serves an app to a web browser over a WebSocket.

Broadway was [deprecated](https://www.phoronix.com/news/GTK-X11-Now-Deprecated) in 4.18 (alongside
X11) for lack of maintenance. This fork keeps it usable through the GTK4 lifecycle as a thin layer
on stock GTK, with no intent to upstream. Stock Broadway is mouse-and-keyboard only, has no
clipboard, no touch, and drops the session on any network blip.

## What the fork adds

- **[Clipboard](reference/clipboard.md)** - copy/paste between app and browser, no permission
  prompt, multi-client safe.
- **[Touch interface](reference/touch.md)** - touch text editing (selection handles, the
  Cut/Copy/Paste bubble), reliable taps, on-screen keyboard, IME and non-Latin input.
- **[Connection management](reference/connection.md)** - in-place reconnect with a session token
  and a PING/PONG heartbeat; survives screen-off and network handovers.
- **[Scaling & HiDPI](reference/scaling.md)** - crisp icons under scale transforms.
- **[Pinch to zoom](reference/zoom.md)** - two-finger UI zoom that re-renders crisply.
- **[Notebook tabs](reference/notebook.md)** - drag/wheel scrolling of the tab strip on touch.
- **[Opening links](reference/open-uri.md)** - clicked links open in the viewing browser.

## How it ships

The patched GTK is built Broadway-only and published as a Debian package, `gtk4-broadway-fork`,
that overlays stock `libgtk-4-1` / `libgtk-4-bin`: it replaces only `libgtk-4.so` and the
`gtk4-broadwayd` daemon, leaving the rest of GTK in place ([Installation](user/installation.md)).

Two GTK bases are maintained in parallel (**4.14.5** and **4.22.2**), each built for **amd64** and
**arm64** ([Supported versions](user/versions.md)).

## Layout

- **User Guide** - which version to pick, installing the `.deb`, serving an app with `broadwayd`.
- **Feature Reference** - [architecture](reference/architecture.md),
  [wire protocol](reference/protocol.md), [changed-files map](reference/files.md), and a chapter
  per subsystem.
- **Building & Releasing** - the Meson config, the CI `.deb` build, and the release flow.

> Both bases carry the same feature set (the 4.22 work is ported to 4.14). Chapters are
> version-agnostic; base differences (e.g. a Meson flag rename) are called out inline.
