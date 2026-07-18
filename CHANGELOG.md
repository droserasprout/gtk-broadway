# Changelog

All notable changes to this project will be documented in this file. The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## v3.2.0 - 2026-07-17

[release](https://github.com/droserasprout/gtk-brotway/releases/tag/v3.2.0) | [diff](https://github.com/droserasprout/gtk-brotway/compare/v3.1.3...v3.2.0)

### Added

- Apps follow the browser's `prefers-color-scheme` (light/dark) live - broadwayd serves an `org.freedesktop.appearance` portal that both plain GTK (via `gtk-interface-color-scheme`) and libadwaita apps read.
- `BROTWAY_COLOR_SCHEME=auto/light/dark` env var pins the color scheme per process and ignores the browser.
- `BROTWAY_NO_ANIMATIONS=1` env var force-disables GTK/libadwaita animations.
- `BROTWAY_MAXIMIZE=1` env var maximizes the app's first toplevel to fill the browser viewport (dialogs still float).

### Changed

- Fork env vars renamed `BROADWAY_*` -> `BROTWAY_*` (`BROTWAY_PNG`, `BROTWAY_DEBUGMENU`, `BROTWAY_DEBUGMENU_FD`, `BROTWAY_DISPLAY`). Legacy `BROADWAY_*` names still work but print a one-time deprecation warning.

### Fixed

- `-nogl` base image is now apt-expandable: a shim Provides `libgtk-4-1`/`libgtk-4-bin` so the dpkg graph stays consistent after the GL/Mesa/LLVM chain is force-purged. Downstream apps can install `gir1.2-gtk-4.0`/`python3-gi` without pulling GL back.
- Touch: tapping outside a popup to dismiss it no longer strands pointer focus on the closed popup; the last finger lifting now clears the mouse-focus trackers.

### Security

- Reject a WebSocket frame whose declared `payload_len` exceeds the received buffer, so the unmask loop can't run past the end.

## v3.1.3 - 2026-06-26

[release](https://github.com/droserasprout/gtk-brotway/releases/tag/v3.1.3) | [diff](https://github.com/droserasprout/gtk-brotway/compare/v3.1.2...v3.1.3)

### Added

- The triple-Shift debug menu shows Client session ID and resume token.
- Always-on-top: per-surface keep-above via `gdk_broadway_surface_set_keep_above()` / the `SET_KEEP_ABOVE` op, with a grab carve-out so a pinned window stays interactive while a menu grab is up. The debug menu now rides this generic flag.
- `-nogl` base image tag (`:v3.1.3-nogl` / `:nogl`): the full image minus the stock GTK + GL/Mesa/LLVM chain Broadway never uses (~200 MB smaller).

### Fixed

- The triple-Shift debug menu now stays interactive while a menu or popup grab is open.
- Nested menus: the Left arrow closes only the open submenu, keeping the parent menu open and focused, instead of dismissing the whole chain.
- Menu-item highlight no longer flickers or clears when a submenu opens or closes.
- A CSD window drag no longer swallows the first click made afterwards.
- Notebook: clicking an end action widget (e.g. a tab-list menu button) no longer also switches the active tab.

## v3.1.2 - 2026-06-23

[release](https://github.com/droserasprout/gtk-brotway/releases/tag/v3.1.2) | [diff](https://github.com/droserasprout/gtk-brotway/compare/v3.1.1...v3.1.2)

### Fixed

- Don't show no-op Minimize button in window menu and CSD.
- Dialog windows no longer snap top-left and grow when clicking the window below.
- Menus and popovers on a non-maximized window open at correct position.
- Modal dialogs now dim the window behind them and block its hover/clicks.
- Desktop: compose key, dead key, and IME sequences now reach the app.
- Desktop: smooth mouse-wheel and touchpad scrolling; use precise deltas instead of a fixed step per event.
- Blink/WebKit: Ctrl+scroll now zooms the app instead of pinch-zoom of the visual viewport.
- Notebook tab-strip edge fade no longer dissolves the header's bottom border.

## v3.1.1 - 2026-06-16

[release](https://github.com/droserasprout/gtk-brotway/releases/tag/v3.1.1) | [diff](https://github.com/droserasprout/gtk-brotway/compare/v3.1.0...v3.1.1)

### Fixed

- The fork's `libgtk-4` carries no-op stubs of the full gdk-x11/gdk-wayland ABI, so any app built against a full GTK loads against it.
- The disconnected overlay shows a severed-link glyph instead of a "prohibited" circle-slash.
- Overlay scrollbars no longer reappear on every content refresh while the pointer is stationary.
- `gtk4-brotway-run` validates arguments, waits for `broadwayd` socket readiness, and no longer mishandles `--auto` with a pinned port.

## v3.1.0 - 2026-06-15

[release](https://github.com/droserasprout/gtk-brotway/releases/tag/v3.1.0) | [diff](https://github.com/droserasprout/gtk-brotway/compare/v3.0.0...v3.1.0)

### Added

- `gtk4-brotway-run`: one-shot launcher (starts `broadwayd`, runs the app, tears it down on exit).
- Sync the GTK window title to the browser tab title and the app icon to the favicon.

### Changed

- The `.deb` now installs into a private prefix (`/usr/lib/gtk4-brotway`) instead of overlaying the system `libgtk-4`.
- The base Docker image sets `LD_LIBRARY_PATH=/usr/lib/gtk4-brotway` so every containerized app uses the fork transparently.

### Fixed

- A copy that can't be serialized to text no longer clears the host clipboard.
- Reconnect no longer replays stale frames from the dropped session before resyncing.
- Adwaita popup menus no longer render a stray 1px frame around the body.
- Notebook tab-strip overflow fade is subtler and sits flush to the edge (no un-faded sliver).
- Clicking a menu button to close its menu no longer leaves the button unclickable until the mouse moves.

### Security

- Validate Broadway input-frame length per event type before parsing; reject short/malformed frames.
- Only open `http`/`https`/`mailto` links the app requests; other URI schemes are ignored.

## v3.0.0 - 2026-06-12

Project renamed to Brotway, a GTK4 Broadway fork.

Documentation is available at [droserasprout.github.io/gtk-brotway](https://droserasprout.github.io/gtk-brotway/).

Release highlights: huge performance gains, the GTK cursor reaches the browser, taps no longer stick on Android Chrome, reconnect survives slow links and double drops, and stale display references no longer blank the UI.

### Added

- Dynamic cursor: forward the GTK cursor shape to the browser as a CSS cursor
- PNG encoding preset: Fast/Compact, switchable in the debug menu or via `BROADWAY_PNG`
- Pause rendering on a hidden browser tab; resume repaints a delta without reconnect
- Debug menu: frame-pacing metrics and texture upload/release rates
- Debug menu: pin fixed screen size and scale

### Changed

- Renamed package `gtk4-broadway-fork` -> `gtk4-brotway` (new package Provides/Replaces the old)
- Renamed binary `gtk4-broadway-debugmenu` -> `gtk4-brotway-debugmenu`

### Removed

- Dropped the GTK 4.14 / ubuntu:24.04 base; v3 is single-base on GTK 4.22.4. The last 4.14 build is v2.1.

### Fixed

- Touch: taps no longer stick on Android Chrome -  the pinch-begin cancel was never delivered due to the wrong sequence id
- Touch: sequences follow the pointer grab - no tap-through past an open popup, no raise/focus churn while a grab is live
- Touch: on-screen keyboard backspace and delete now reach GTK; GBoard's key buffer is refilled after each commit
- Reconnect: fixed reconnect loop after a network switch (backoff resets only on a confirmed session)
- Reconnect: a slow initial resync is no longer killed mid-stream and looped (60s first-message grace; resync textures flushed as separate frames)
- Reconnect: a drop during resume no longer duplicates every window; grab, button and touch state are reset and surface cursors replayed
- Rendering: stale node/surface/texture references in display ops warn and skip instead of throwing away the rest of the frame batch, and no longer leak the patched texture
- Rendering: node reuse re-encodes when the enclosing clip moved; destroying and recreating a surface id in one batch no longer drops the new surface
- Rendering: texture dedup can no longer alias two different images; colorized-texture cache eviction actually frees the decoded copies
- Rendering: icon and cell glyphs no longer drift a row under a scrolled list; borders no longer adopt geometry from an unrelated neighboring node
- Rendering: popovers anchored outside the browser monitor now open instead of no-opping with a `Gdk-CRITICAL`
- Widgets: the label copy bubble dismisses when the selection collapses; dragging a selection handle keeps at least one character selected
- Widgets: notebook touch tab reorder and detach work when the strip doesn't scroll, and a non-overflowing strip no longer eats horizontal scroll
- Widgets: tapping a row of a multi-selection collapses to it on release instead of doing nothing
- Debug menu: failed spawns back off, the menu window is matched by client (other apps' dialogs no longer get pinned on top), plus internal teardown and stats fixes

### Security

- Short websocket frames (PING) and undersized daemon-socket requests are length-checked before field reads
- Variable-length daemon-socket requests are padded to keep in-place struct reads aligned

### Performance

- Bump texture cache cap from 512 to 4096 (~16 MB)
- Default to a fast PNG preset (low zlib level, adaptive filter) over libpng's defaults
- Faster texture-dedup hash, keep redrawn textures hot in the cache, skip redundant browser image reloads

## v2.1 - 2026-06-09

### Fixed

- Include `gtk4-broadway-debugmenu` in deb packages

## v2 - 2026-06-08

### Added

- **Connection management** - auto-reconnect after screen-off or network change, session token, ping-pong heartbeat, single-display arbitration
- **Label touch selection** - read-only labels get selection handles and a Copy/Select-all bubble like entries; tap outside or use the mouse to dismiss
- **Debug menu** - Triple-Shift opens a server-side overlay with live performance stats and a paint-flash profiler that visualizes re-rendered areas

### Fixed

- Icons stay crisp under scale transforms and HiDPI
- No crash when starting a drag from a text selection
- Rendering no longer freezes under heavy scrolling or on a stale node/texture reference
- Touch: long-pressing a multi-row selection keeps it
- Desktop: clicking empty space in a list clears the selection
- Popovers and menus land on their anchor, not offset by their shadow
- A popover's shadow passes clicks through instead of swallowing them
- Widget borders render crisp: no 1px seam or corner sliver against the background

### Performance

- Reuse re-rendered-but-identical content (text scrolled into view, re-hovered rows, repeated icons)
- Drop empty `SET_NODES` no-op frames from the wire
- Coalesce pointer-move events to one per frame, so a motion flood can't delay a following click or keypress
- Lower per-frame CPU: header and payload go out in a single socket write, and input events are packed without per-event allocation
- Bound memory: the output buffer is released after an oversized frame, and the per-texture recolor cache is LRU-capped

## v1 - 2026-06-05

### Added

- **Clipboard** - bidirectional copy/paste between app and browser, no permission prompt, multi-client safe
- **Touch editing** - selection handles and the Cut/Copy/Paste bubble, reliable taps, tap-outside to dismiss popovers
- **On-screen keyboard** - shows/hides on Android, with IME and non-Latin input (Cyrillic, CJK, gesture-typing, autocorrect)
- **Pinch to zoom** - two-finger UI zoom (0.25x-5x), crisp re-render, per-client, survives refresh
- **Open links** - clicked links open in a new browser tab (`gtk_show_uri`, `GtkUriLauncher`, `GtkLabel`/`GtkLinkButton`)
- **Notebook tabs** - drag or wheel to scroll the tab strip on touch, with edge fades and persisted position

### Fixed

- No white flash on load or zoom-out with dark browser theme
- New windows open centered and re-center on zoom
- Touch: bubble's Cut/Copy/Paste fire instead of dismissing it
- Touch: menus and dropdowns no longer freeze on tap
- Touch: dropdowns select the tapped row, not the first
- Touch: no crash when reopening the selection bubble
- Touch: Copy reappears after Select-All
- Desktop: horizontal two-finger swipe scrolls instead of browser back/forward
- Desktop: drags survive the cursor leaving the widget

