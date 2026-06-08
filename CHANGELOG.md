# Changelog

All notable changes to the GTK Broadway fork. Format based on [Keep a Changelog](https://keepachangelog.com/).

## [Unreleased]

### Added

- **Connection management** - auto-reconnect after screen-off or network change, sessions token, ping-pong heartbeat, single-display arbitration.
- **Label touch selection** - read-only labels get selection handles and a Copy/Select-all bubble like entries; tap outside or use the mouse to dismiss.
- **Debug menu** - Triple-Shift opens a server-side overlay with live performance stats and a paint-flash profiler that visualizes re-rendered areas.

### Fixed

- Icons stay crisp under scale transforms and HiDPI.
- Fixed crash when starting a drag from a text selection.
- Touch: long pressing multiple selected rows doesn't clear selection.
- Desktop: clicking empty space in a list clears the selection.
- Rendering no longer freezes under heavy scrolling; a stale node/texture reference degrades gracefully.

### Performance

- Reuse re-rendered-but-identical content (text scrolled into view, re-hovered rows, repeated icons).
- Drop empty `SET_NODES` no-op frames from the wire.
- Coalesce pointer-move events to one per frame, so a motion flood can't delay a following click or keypress.
- Lower per-frame CPU: header and payload go out in a single socket write, and input events are packed without per-event allocation.
- Bound memory: the output buffer is released after an oversized frame, and the per-texture recolor cache is LRU-capped.

## [v1] - 2026-06-05

### Added

- **Clipboard** - bidirectional copy/paste between app and browser, no permission prompt, multi-client safe.
- **Touch editing** - selection handles and the Cut/Copy/Paste bubble, reliable taps, tap-outside to dismiss popovers.
- **On-screen keyboard** - shows/hides on Android, with IME and non-Latin input (Cyrillic, CJK, gesture-typing, autocorrect).
- **Pinch to zoom** - two-finger UI zoom (0.25x-5x), crisp re-render, per-client, survives refresh.
- **Open links** - clicked links open in a new browser tab (`gtk_show_uri`, `GtkUriLauncher`, `GtkLabel`/`GtkLinkButton`).
- **Notebook tabs** - drag or wheel to scroll the tab strip on touch, with edge fades and persisted position.

### Fixed

- No white flash on load or zoom-out with dark browser theme.
- New windows open centered and re-center on zoom.
- Touch: bubble's Cut/Copy/Paste fire instead of dismissing it.
- Touch: menus and dropdowns no longer freeze on tap.
- Touch: dropdowns select the tapped row, not the first.
- Touch: no crash when reopening the selection bubble.
- Touch: Copy reappears after Select-All.
- Touch: pressing a selected row keeps the whole multi-selection, not just that row.
- Clicking empty space in a list clears the selection.
- Desktop: horizontal two-finger swipe scrolls instead of browser back/forward.
- Desktop: drags survive the cursor leaving the widget.

[v1]: https://github.com/droserasprout/gtk-broadway/releases/tag/v1
