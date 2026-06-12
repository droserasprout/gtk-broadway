# Changelog

All notable changes to this project will be documented in this file. The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

Versioning is not semver: release tags are `vX[.Y]` (fork revision), package versions are `<gtk-base>-X[.Y]`, e.g. `4.22.2-2.1`. See [Supported versions](https://droserasprout.github.io/gtk-brotway/guide/versions.html).

## [Unreleased]

Project renamed to Brotway, a GTK4 Broadway fork.

### Added

- **Dynamic cursor** - forward the GTK cursor shape to the browser as a CSS cursor
- **PNG encoding preset** - Fast/Compact, switchable in the debug menu or via `BROADWAY_PNG`
- Pause rendering on a hidden browser tab; resume repaints a delta without reconnect
- Debug menu: frame-pacing metrics and texture upload/release rates
- Debug menu: pin fixed screen size and scale

### Changed

- Renamed package `gtk4-broadway-fork` -> `gtk4-brotway` (new package Provides/Replaces the old)
- Renamed binary `gtk4-broadway-debugmenu` -> `gtk4-brotway-debugmenu`

### Fixed

- Fixed reconnect loop after a network switch (backoff resets only on a confirmed session)
- Icon and cell glyphs no longer drift a row under a scrolled list (translate node emitted in absolute, not parent-local, coordinates)
- Popovers anchored outside the browser monitor now open instead of no-opping with a `Gdk-CRITICAL`
- Touch: releases were sent with the raw browser id while presses sent the mapped one - on Android Chrome every tap left a stuck sequence (begin without end)
- Touch: the cancel that aborts the first finger's tap at pinch-begin now actually reaches GTK on Android Chrome
- Touch: sequences follow the pointer grab - no more tap-through past an open popup, no raise/focus churn while a grab is live
- Android OSK: backspace and delete forwarded via `beforeinput`; GBoard's key buffer is refilled after each commit
- Reconnect: a live connection is no longer declared dead while the initial resync is still streaming (60s first-message grace; resync textures flushed as separate frames)
- Reconnect: a drop during resume no longer skips the client reset, which duplicated every window
- Reconnect: grab, button and touch state are reset with the session, and surface cursors are replayed
- Stale node/surface/texture references in display ops warn and skip instead of throwing away the rest of the frame batch
- A texture patched onto an already-deleted node no longer leaks for the session
- Borders no longer adopt size and corner radii from an unrelated neighboring node
- Destroying and recreating a surface id in one batch no longer drops the new surface
- Node reuse re-encodes when the enclosing clip moved, instead of replaying geometry against the stale offset
- Texture dedup keys include the native format and color state, with a second hash against collisions
- Colorized-texture cache eviction now actually frees the decoded copies
- Label: the copy bubble dismisses when the selection collapses; dragging a selection handle keeps at least one character selected
- Notebook: touch tab reorder and detach work again when the tab strip doesn't scroll; a non-overflowing strip no longer eats horizontal scroll
- Tree view: tapping a row of a multi-selection collapses to it on release instead of doing nothing
- Debug menu: failed spawns back off; the menu window is matched by client (other apps' dialogs no longer get pinned on top); traffic rate no longer wraps after a daemon restart

### Security

- Short websocket frames (PING) and undersized daemon-socket requests are length-checked before field reads
- Variable-length daemon-socket requests are padded to keep in-place struct reads aligned

### Performance

- Bump texture cache cap from 512 to 4096 (~16 MB)
- Default to a fast PNG preset (low zlib level, adaptive filter) over libpng's defaults
- Faster texture-dedup hash, keep redrawn textures hot in the cache, skip redundant browser image reloads

## [v2.1] - 2026-06-09

### Fixed

- Include `gtk4-broadway-debugmenu` in deb packages

## [v2] - 2026-06-08

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

## [v1] - 2026-06-05

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

[v2.1]: https://github.com/droserasprout/gtk-brotway/releases/tag/v2.1
[v2]: https://github.com/droserasprout/gtk-brotway/releases/tag/v2
[v1]: https://github.com/droserasprout/gtk-brotway/releases/tag/v1
