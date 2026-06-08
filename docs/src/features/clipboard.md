# Clipboard

Stock Broadway has no clipboard. The fork bridges `GdkClipboard` <-> `gtk4-broadwayd` <-> the
browser's `navigator.clipboard`, in both directions, with no permission prompt.

## What works

- **Copy / cut from the app** to the browser clipboard - Ctrl+C/X, right-click Copy, and custom
  Copy actions; works in text views too.
- **Paste into the app** from the browser - Ctrl+V, right-click Paste, and paste from the
  [on-screen keyboard](touch.md), with no browser permission prompt.
- **Multi-client safe** - with several browsers connected, a paste reply reaches the tab that
  asked, and a paste never hangs if a tab closes mid-request.
- Text up to 16 MiB.

## Limitations

- **PRIMARY selection / middle-click paste** are not supported - browsers expose no JS API for it.
- **Text only.** No image or rich-clipboard support; the read path rejects non-text.
- `navigator.clipboard` needs a [secure context](../guide/running.md#secure-context-note): over
  plain `http://`, copy may silently fail outside a user gesture. Serve over `https://` or
  `http://localhost`.

> The push-vs-request/reply wire paths, the hidden textarea, the length clamps, and touch-keyboard
> paste are in [Clipboard implementation](../internals/clipboard.md).
