# Changelog

All notable changes to the GTK Broadway fork. Format based on [Keep a Changelog](https://keepachangelog.com/).

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
- Desktop: horizontal two-finger swipe scrolls instead of browser back/forward.
- Desktop: drags survive the cursor leaving the widget.

[v1]: https://github.com/droserasprout/gtk-broadway/releases/tag/v1
