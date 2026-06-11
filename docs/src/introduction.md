# Brotway, a GTK Broadway fork

This is a fork of GTK that fills in the missing pieces of the Broadway backend ([What is Broadway?](broadway.md)).

Broadway was deprecated by the GTK team in 4.18 (alongside X11) for lack of maintenance. This fork keeps it usable through the GTK4 lifecycle as a thin layer on stock GTK, with no intent to upstream.

## Project Goals {#goals}

- **Thin layer on GTK4.** The patches touch only the Broadway backend, never app-facing GTK API, and stay close to upstream so each GTK4 point release can be re-forked with a minimal diff. The fork lives only as long as GTK4 itself.
- **Accurate rendering.** What the browser shows should match what the app draws: correct scaling, no white flashes, no stretched bitmaps, crisp output under HiDPI and zoom.
- **Full touch and mobile support.** Treat touch and mobile as primary targets, not something bolted on after the fact. Touch text editing, the on-screen keyboard, IME, and pinch-zoom should work as well as mouse and keyboard.
- **Match the GNOME/Mutter/Adwaita experience.** The same interactions should behave the way they do under a real compositor, so a Broadway session works like a normal GTK desktop rather than a degraded remote view.
- **Performance and stability.** Stay stable: no crashes, no wedged input, no dropped sessions. Reconnect in place and survive screen-off, network handovers, and surface churn.

## What the fork adds

- **[Clipboard](features/clipboard.md):** copy/paste between app and browser, no permission prompt, multi-client safe.
- **[Touch interface](features/touch.md):** touch text editing (selection handles, the Cut/Copy/Paste bubble), reliable taps, on-screen keyboard, IME and non-Latin input.
- **[Connection management](features/connection.md):** in-place reconnect with a session token and a PING/PONG heartbeat; survives screen-off and network handovers.
- **[Scaling & HiDPI](features/scaling.md):** crisp icons under scale transforms.
- **[Pinch to zoom](features/zoom.md):** two-finger UI zoom that re-renders crisply.
- **[Notebook tabs](features/notebook.md):** drag/wheel scrolling of the tab strip on touch.
- **[Opening links](features/open-uri.md):** clicked links open in the viewing browser.
- **[Dynamic cursor](features/cursor.md):** the browser pointer follows GTK's cursor shape (resize edges, text, links).
- **[Desktop & rendering fixes](features/desktop-fixes.md):** no white flash, centered windows, popups on their anchor, click-through shadows, seam-free borders.

## How it ships

The patched GTK is built Broadway-only and published as a Debian package, `gtk4-brotway`, that overlays stock `libgtk-4-1` / `libgtk-4-bin`. It replaces only `libgtk-4.so` and the `gtk4-broadwayd` daemon, leaving the rest of GTK in place ([Installation](guide/installation.md)).

Multiple GTK bases are maintained in parallel, each built for amd64 and arm64 ([Supported versions](guide/versions.md)).

> Chapters are version-agnostic; base differences (e.g. a Meson flag rename) are called out inline.
