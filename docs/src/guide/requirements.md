# Requirements

The fork patches the **host** GTK and serves the app as HTML5 over Broadway, so requirements split between the host (where GTK runs) and the client (the browser).

## Host

| What | Requirement |
| ---- | ----------- |
| OS | Linux |
| Distro | Debian (`.deb`), Arch (PKGBUILD), or Docker base image - see [Installation](installation.md) |
| GTK | 4.22.x series, matching the system GTK |
| Architecture | amd64, arm64 |

Current fork base:

| GTK    | Ubuntu base    | SONAME                 | Fork branch | Patch |
| ------ | -------------- | ---------------------- | ----------- | ----- |
| 4.22.4 | `ubuntu:26.04` | `libgtk-4.so.1.2200.4` | [`4.22.4-brotway`](https://github.com/droserasprout/gtk-brotway/tree/4.22.4-brotway) | [diff](https://github.com/droserasprout/gtk-brotway/compare/4.22.4...4.22.4-brotway) |

## Client

Any device with a modern browser, no install - the UI is HTML5 over Broadway, so client OS and architecture don't matter (desktop and mobile, touch included).

### Tested browsers

- Linux desktop:
  - Firefox 151.0.2
  - Chromium 149.0.7827.53-1.1
  - Google Chrome 149.0.7827.114-1
- Android:
  - Firefox Beta 152.0b4
  - Google Chrome 146.0.7680.119
- Headless
  - Playwright w/ Chrome driver

### Not tested

- Windows browsers
- macOS browsers
- iOS (Safari, Orion)
- Mixed devices (e.g. laptops with a touchscreen)
- Mobile linux (postmarketOS)
