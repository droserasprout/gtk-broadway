# Quickstart

From nothing to a GTK4 app in your browser, with the fork's clipboard and touch support. Three
steps: install the `.deb`, start the daemon, point an app at it.

## 1. Install the patched GTK

Pick the asset matching your Ubuntu base ([4.14.5 on `ubuntu:24.04`, 4.22.2 on
`ubuntu:26.04`](guide/versions.md)) and architecture:

```sh
rel=v1           # the release to install
gtk_ver=4.14.5   # use 4.22.2 on an ubuntu:26.04 base
arch="$(dpkg --print-architecture)"
wget -O gtk.deb "https://github.com/droserasprout/gtk-broadway/releases/download/${rel}/gtk4-broadway-fork_${gtk_ver}-${rel#v}_${arch}.deb"
apt-get install -y ./gtk.deb
apt-mark hold libgtk-4-1 libgtk-4-bin
```

The `apt-mark hold` matters - without it a later `apt upgrade` reverts the fork. Full detail in
[Installation](guide/installation.md).

## 2. Start the daemon

```sh
gtk4-broadwayd :5
```

`:5` is the display number; the browser page is served on `http://localhost:8085` (port `8080 + N`).

## 3. Run an app against it

```sh
GDK_BACKEND=broadway BROADWAY_DISPLAY=:5 your-gtk4-app
```

Open `http://localhost:8085` in a browser. Copy/paste, touch text editing, and pinch-zoom now work.
See [Running broadwayd](guide/running.md) for how the pieces fit, and
[Deploying behind TLS](guide/deployment.md) for a real (non-localhost) deployment, which the
[clipboard's secure-context requirement](features/clipboard.md#limitations) needs.

## Next steps

- [Features](features/comparison.md) - what the fork adds over stock Broadway, feature by feature.
- [Configuration reference](guide/config.md) - every env var, port, and client-side setting.
- [Known issues](guide/known-issues.md) - what doesn't work, and tested browser/OS combinations.
