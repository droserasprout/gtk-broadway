# Quickstart

From nothing to a GTK4 app in your browser. Two steps: install the fork, then launch an app with `gtk4-brotway-run`.

## Install the fork

On Debian/Ubuntu, grab the `.deb` for your architecture (the [GTK base](guide/requirements.md) is 4.22.4 on `ubuntu:26.04`) and install it:

```sh
rel=v3.1.0       # the release to install - see the Releases page for the latest tag
gtk_ver=4.22.4   # on an ubuntu:26.04 base
arch="$(dpkg --print-architecture)"
wget -O gtk.deb "https://github.com/droserasprout/gtk-brotway/releases/download/${rel}/gtk4-brotway_${gtk_ver}-${rel#v}_${arch}.deb"
apt-get install -y ./gtk.deb
```

It installs into a private prefix, safe on a desktop. On Arch build the PKGBUILD; for containers use the base image - see [Installation](guide/installation.md).

## Run an app against it

`gtk4-brotway-run` starts the daemon, runs the app, and tears the daemon down on exit:

```sh
gtk4-brotway-run your-gtk4-app
```

Open `http://localhost:8085` in a browser. Copy/paste, touch text editing, and pinch-zoom now work - the fork is app-agnostic, any GTK4 binary does. For a non-localhost deployment, add `--address 0.0.0.0` and put it behind a TLS terminator; see [Security model](guide/security.md).

See [Running](guide/running.md) for how the pieces fit. A non-localhost deployment also needs a [secure context](features/clipboard.md#limitations) for the clipboard.

## Next steps

- [Features](features/comparison.md) - what the fork adds over stock Broadway, feature by feature.
- [Configuration reference](guide/config.md) - every env var, port, and client-side setting.
- [Requirements](guide/requirements.md) - host/client support and tested browser/OS combinations.
- [Known issues](guide/known-issues.md) - what doesn't work.
