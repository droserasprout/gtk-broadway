# Brotway, a GTK Broadway fork

<img src="images/calculator.png" alt="A native GTK app (GNOME Calculator) rendering in a browser tab over Broadway"
    style="float:right;width:46%;max-width:300px;margin:0.25rem 0 1rem 1.5rem;border-radius:6px">

Brotway is a fork of GTK that fills in the missing pieces of the Broadway backend, keeping it usable through the GTK4 lifecycle as a thin layer on stock GTK. It gives native GTK4 apps rendered in a browser tab a working clipboard, real touch, pinch-zoom, and in-place reconnect.

## What the fork adds

- **[Clipboard](features/clipboard.md):** copy/paste between app and browser, no permission prompt, multi-client safe.
- **[Touch interface](features/touch.md):** touch text editing (selection handles, the Cut/Copy/Paste bubble), on-screen keyboard, IME and non-Latin input.
- **[Connection management](features/connection.md):** in-place reconnect with a session token and a PING/PONG heartbeat; survives screen-off and network handovers.
- **[Scaling & HiDPI](features/scaling.md):** crisp icons under scale transforms.
- **[Pinch to zoom](features/zoom.md):** two-finger UI zoom that re-renders crisply.
- **[Notebook tabs](features/widgets.md#notebook-tabs):** drag/wheel scrolling of the tab strip on touch.
- **[Opening links](features/open-uri.md):** clicked links open in the viewing browser.
- **[Dynamic cursor](features/cursor.md):** the browser pointer follows GTK's cursor shape (resize edges, text, links).
- **[Desktop & rendering fixes](features/desktop-fixes.md):** no white flash, centered windows, popups on their anchor, click-through shadows, seam-free borders.

## What is Broadway? {#broadway}

Broadway is GTK's HTML5 backend: instead of drawing to a local display (Wayland, X11, Win32, macOS), it renders the app into a web browser over a WebSocket. A `gtk4-broadwayd` daemon owns a virtual display and serves a page; any browser that connects to it sees and drives the running GTK app. No app code changes - the same binary picks Broadway through `GDK_BACKEND=broadway`. That makes it a way to put a native GTK app on a screen it was never built for: a phone browser, a remote machine, a kiosk.

It started as Alexander Larsson's frame-streaming prototype ([2010](https://blogs.gnome.org/alexl/2010/11/26/gtk-3-0-html5-backend/), merged for GTK 3.2). GTK4 reworked it around the render-node pipeline ([Larsson, 2019](https://blogs.gnome.org/alexl/2019/03/29/broadway-adventures-in-gtk4/)): the GSK Broadway renderer turns the app's render nodes into a compact node stream the browser reconstructs in the DOM, rasterizing with cairo only what Broadway can't express. That node-stream design is what the fork builds on ([Architecture](internals/architecture.md)).

Broadway always stayed an experimental, lightly-maintained corner of GTK, and in February 2025 it was [deprecated alongside X11](https://www.phoronix.com/news/GTK-X11-Now-Deprecated) (in the 4.17.4 development release, then stable **4.18**), to be **removed entirely in GTK5**. There is no GTK5 Broadway to move to, so the fork is anchored to the GTK 4.x lifetime and deliberately not ported ([Re-forking a new GTK release](build/reforking.md)).

## Project goals {#goals}

- **Thin layer on GTK4.** The patches touch only the Broadway backend, never app-facing GTK API, and stay close to upstream so each GTK4 point release can be re-forked with a minimal diff. The fork lives only as long as GTK4 itself.
- **Accurate rendering.** What the browser shows should match what the app draws: correct scaling, no white flashes, no stretched bitmaps, crisp output under HiDPI and zoom.
- **Any browser, any device.** Treat touch and mobile as primary targets, not something bolted on after the fact. Touch text editing, the on-screen keyboard, IME, and pinch-zoom should work as well as mouse and keyboard.
- **Match the GNOME/Mutter/Adwaita experience.** The same interactions should behave the way they do under a real compositor, so a Broadway session works like a normal GTK desktop rather than a degraded remote view.
- **Performance and stability.** Stay stable: no crashes, no wedged input, no dropped sessions. Reconnect in place and survive screen-off, network handovers, and surface churn.

