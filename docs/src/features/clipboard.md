# Clipboard

<!-- SCREENCAST (pending) - uncomment after recording. See devnotes/2026-06-10-docs-screencasts.md
  Record: make record SCENARIO=tools/local/scenarios/clipboard.json OUT=docs/src/images/clipboard.mp4

<video src="../images/clipboard.mp4" poster="../images/clipboard.png"
  autoplay loop muted playsinline style="max-width:100%;border-radius:6px">
  <img src="../images/clipboard.png" alt="Copy from the app and paste back, bridged to the browser clipboard"
    style="max-width:100%;border-radius:6px">
</video>
-->

Brotway bridges `GdkClipboard` <-> `gtk4-broadwayd` <-> the browser's `navigator.clipboard`, in both directions. No permission prompt shown in browser.

## What works

- **Copy / cut from the app** to the browser - Ctrl+C/X, right-click Copy, and custom Copy actions.
- **Paste into the app** from the browser - Ctrl+V, right-click Paste, and paste from the [on-screen keyboard](touch.md).
- **Multi-client safe** - with several browsers connected, a paste reply reaches the tab that asked, and a paste never hangs if a tab closes mid-request.
- Text up to 16 MiB.

## Limitations

- **PRIMARY selection / middle-click paste** are not supported - browsers expose no JS API for it.
- **Text only.** No image or rich-clipboard support; the read path rejects non-text.
- `navigator.clipboard` needs a [secure context](../guide/running.md#secure-context-note): over plain `http://`, copy may silently fail outside a user gesture. Serve over `https://` or `http://localhost`.

> Wire paths and internals: [Clipboard implementation](../internals/clipboard.md).
