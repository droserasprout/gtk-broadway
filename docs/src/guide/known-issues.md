# Known issues

Bugs are tracked on the [issue tracker](https://github.com/droserasprout/gtk-brotway/issues). The items below are known limitations not currently planned for a fix.

## WONTFIX

- **PRIMARY selection and middle-click paste.** Desktop browsers expose no JavaScript API for the X11-style PRIMARY selection, so there's no way to bridge it between host and guest.

- **Clipboard copy on insecure origins.** Copy may silently fail outside a user gesture, since `navigator.clipboard` is gated to secure contexts. Serve over `https://` or `http://localhost`.

- **Popovers on an oversized unmaximized window.** When the window is larger than the browser viewport (HiDPI or browser zoom), its tracked position can desync from where the browser draws it, so menus open off-anchor or off-screen. Maximize the window to correct it.

## No fix yet

- **Tap leak into pinch and pan.** The first finger's tap can still register when a second finger lands to start a gesture. No solution found yet without adding input latency; the pinch state machine suppresses the worst case, now on Android Chrome too ([details](../internals/zoom.md#no-tap-leak-on-pinch)).

- **Emoji widget.** Slow and ugly over Broadway.

## Tested configurations

- **Desktop:** Firefox 151.0.2, Chromium 148.0.7778.178
- **Android:** Firefox Beta 152.0b4, Google Chrome 146.0.7680.119
- **Transport:** HTTP on `localhost`, and HTTPS behind Traefik in Docker Swarm

## Not tested

- iOS
- Mixed devices (e.g. laptops with a touchscreen)
