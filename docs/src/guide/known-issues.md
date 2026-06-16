# Known issues

Bugs are tracked on the [issue tracker](https://github.com/droserasprout/gtk-brotway/issues). The items below are known limitations, not currently planned for a fix:

- **PRIMARY selection and middle-click paste.** Desktop browsers expose no JavaScript API for the X11-style PRIMARY selection, so there's no way to bridge it.
- **Clipboard copy on insecure origins.** Copy may silently fail outside a user gesture, since `navigator.clipboard` is gated to secure contexts. Serve over `https://` or `http://localhost`.

Tested browsers live in [Requirements](requirements.md#tested-browsers).
