# Known issues

Bugs are tracked on the [issue tracker](https://github.com/droserasprout/gtk-broadway/issues). The items below are known limitations not currently planned for a fix.

## WONTFIX

- **PRIMARY selection and middle-click paste.** Desktop browsers expose no JavaScript API for the X11-style PRIMARY selection, so there's no way to bridge it between host and guest.

- **Clipboard copy on insecure origins.** Copy may silently fail outside a user gesture, since `navigator.clipboard` is gated to secure contexts. Serve over `https://` or `http://localhost`.

## Tested configurations

- **Desktop:** Firefox 151.0.2, Chromium 148.0.7778.178
- **Android:** Firefox Beta 152.0b4, Google Chrome 146.0.7680.119
- **Transport:** HTTP on `localhost`, and HTTPS behind Traefik in Docker Swarm

## Not tested

- iOS
- Mixed devices (e.g. laptops with a touchscreen)
