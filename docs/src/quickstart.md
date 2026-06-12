# Quickstart

From nothing to a GTK4 app in your browser, with the fork's clipboard and touch support. Three steps: install the `.deb`, start the daemon, point an app at it.

## 1. Install the patched GTK

Grab the `.deb` matching your architecture (the [GTK base](guide/versions.md) is 4.22.4 on `ubuntu:26.04`), install it, and hold the stock packages:

```sh
rel=v3.0.0       # the release to install - see the Releases page for the latest tag
gtk_ver=4.22.4   # on an ubuntu:26.04 base
arch="$(dpkg --print-architecture)"
wget -O gtk.deb "https://github.com/droserasprout/gtk-brotway/releases/download/${rel}/gtk4-brotway_${gtk_ver}-${rel#v}_${arch}.deb"
apt-get install -y ./gtk.deb
apt-mark hold libgtk-4-1 libgtk-4-bin
```

The `apt-mark hold` keeps a later `apt upgrade` from reverting the fork. See [Installation](guide/installation.md) for what the package overlays and the [hold rationale](guide/installation.md#hold-the-stock-packages).

## 2. Start the daemon

```sh
gtk4-broadwayd :5
```

`:5` is the display number; the browser page is served on `http://localhost:8085` (port `8080 + N`).

## 3. Run an app against it

```sh
GDK_BACKEND=broadway BROADWAY_DISPLAY=:5 your-gtk4-app
```

Open `http://localhost:8085` in a browser. Copy/paste, touch text editing, and pinch-zoom now work. The fork is app-agnostic - any GTK4 binary works - but for a complete, real-world deployment (the app this fork was built to serve, packaged with the `.deb` and a TLS terminator) see [`nicotineplus-proper`](https://github.com/droserasprout/nicotineplus-proper), which runs Nicotine+ as a browser WebUI on this stack.

See [Running broadwayd](guide/running.md) for how the pieces fit, and [Security model](guide/security.md) for a real (non-localhost) deployment, which the [clipboard's secure-context requirement](features/clipboard.md#limitations) needs.

## Next steps

- [Features](features/comparison.md) - what the fork adds over stock Broadway, feature by feature.
- [Configuration reference](guide/config.md) - every env var, port, and client-side setting.
- [Known issues](guide/known-issues.md) - what doesn't work, and tested browser/OS combinations.
