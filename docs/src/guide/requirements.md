# Requirements

The fork patches the **host** GTK and serves the app as HTML5 over Broadway, so requirements split between the host (where GTK runs) and the client (the browser).

## Host

- **OS:** Linux only - Broadway serves over HTTP and the patched `libgtk-4` is a Linux `.so`.
- **Distros:** Debian-based (`.deb`), Arch/CachyOS (PKGBUILD), or any host via the Docker base image - see [Installation](installation.md). The `.deb` is Broadway-only (no X11/Wayland backend), so it's for headless/container hosts, **not desktops**; on a desktop use the conflict-free private-prefix build.
- **GTK:** the **4.22.x** series. The `.deb` overlays the SONAME-versioned `.so` in place (a lib built from 4.22.4), so the system GTK must be the same 4.22 series; the micro version may differ (ABI and schemas are stable within a bugfix series - the shipped image runs lib 4.22.4 on 4.22.2 common). Other versions are unsupported (older Broadway bases diverge too much to backport - last 4.14 build was v2.1).
- **Architecture:** `amd64` and `arm64` - native per-arch `.deb`s, multi-arch Docker manifest.

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
