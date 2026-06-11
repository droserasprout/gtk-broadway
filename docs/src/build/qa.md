# Release QA

The manual verification pass for a release candidate, run before pushing the `vN` tags ([Release process](release.md)). If the change set touched `libgtk` (not just `broadway.js`/`client.html`), run it on both bases - same patches, two GTK trees.

## Setup

Install the candidate `.deb` ([Installation](../guide/installation.md)) or use a local build ([Building from source](from-source.md)). Start the stack ([Running broadwayd](../guide/running.md)):

```sh
gtk4-broadwayd :5
GDK_BACKEND=broadway BROADWAY_DISPLAY=:5 your-gtk4-app
```

Open `http://localhost:8085`, then Triple-Shift > **Test gallery** ([debug menu](../internals/debug-menu.md#widget-gallery)) for a clean, data-free surface - the gallery's table maps each section to the feature it exercises. Keep the stats overlay up too; the connection and performance checks read it.

## Desktop browser pass

- [ ] **Clipboard, both directions** - Ctrl+C/V and right-click Copy/Paste between the gallery's text widgets and a host editor ([Clipboard](../features/clipboard.md)).
- [ ] **Links** - the gallery's link button opens in a new browser tab ([Opening links](../features/open-uri.md)).
- [ ] **Dynamic cursor** - resize arrows on the window edges, I-beam over text, hand over links ([Dynamic cursor](../features/cursor.md)).
- [ ] **Notebook wheel** - wheel over the tab strip pans it; a vertical wheel switches pages ([Notebook tabs](../features/notebook.md)).
- [ ] **Window placement** - the gallery, a new toplevel, opens centered, not top-left ([Desktop & rendering fixes](../features/desktop-fixes.md)).
- [ ] **No white flash** on page load ([Desktop & rendering fixes](../features/desktop-fixes.md)).

## Android pass

- [ ] **Touch selection** - handles plus the Cut/Copy/Paste bubble, and the bubble's buttons actually fire ([Touch interface](../features/touch.md)).
- [ ] **On-screen keyboard** - shows on entry focus and hides again, with no one-gesture lag.
- [ ] **IME / non-Latin input** - Cyrillic or CJK, autocorrect replacements.
- [ ] **Pinch zoom** - follows the fingers, sharpens once they lift, and survives a page reload ([Pinch to zoom](../features/zoom.md)).
- [ ] **Tab strip** - drag scrolls it pixel-for-pixel; a plain tap still selects a tab ([Notebook tabs](../features/notebook.md)).
- [ ] **Tap outside** - a tap outside a popover or menu dismisses it ([Touch interface](../features/touch.md)).

## Connection drills

All from [Connection management](../features/connection.md); watch the debug menu's Traffic counter during the last one.

- [ ] **Screen off/on** - the session resumes in place, no reload.
- [ ] **Network change** - toggle Wi-Fi: the last frame dims under a spinner, then the resync repaints.
- [ ] **Second tab** - a fresh page load takes ownership of the display; the newest wins.
- [ ] **Hidden tab** - backgrounding the tab streams zero frames (Traffic flat); switching back repaints just the delta, no reconnect.

## Performance sanity

Toggle **Paint flashing** in the debug menu; how to read it is in [Good vs bad readings](../internals/performance.md#good-vs-bad-readings).

- [ ] **Static screen** - no red flashes, `up/s` ~0, Traffic flat.
- [ ] **Scroll** - a brief red spike on newly exposed content, then green/magenta as scroll-back hits the cache.

## Record what you tested

Update **Tested configurations** in [Known issues](../guide/known-issues.md#tested-configurations) with the exact browser and OS versions this pass ran on. Those versions are snapshots of the QA pass - nothing else updates them.
