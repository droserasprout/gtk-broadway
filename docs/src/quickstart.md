# Quickstart

Install the fork, then launch an app with `gtk4-brotway-run`.

## Install the fork

On Debian/Ubuntu, grab the `.deb` for your architecture and install it:

```sh
rel=v3.1.1       # the release to install - see the Releases page for the latest tag
gtk_ver=4.22.4   # on an ubuntu:26.04 base
arch="$(dpkg --print-architecture)"
wget -O gtk.deb "https://github.com/droserasprout/gtk-brotway/releases/download/${rel}/gtk4-brotway_${gtk_ver}-${rel#v}_${arch}.deb"
apt-get install -y ./gtk.deb
```

On Arch, build and install from the PKGBUILD (your `gtk4` must be the same 4.22.x series):

```sh
git clone -b ci https://github.com/droserasprout/gtk-brotway
cd gtk-brotway/packaging/arch && makepkg -si
```

It installs into a private prefix, safe on a desktop. For containers use the base image - see [Installation](guide/installation.md).

## Run an app against it

`gtk4-brotway-run` starts the daemon, runs the app, and tears the daemon down on exit:

```sh
gtk4-brotway-run gtk4-demo   # changeme
```

Open `http://localhost:8085` in a browser. Copy/paste, touch text editing, and pinch-zoom now work, for any GTK4 binary. For a non-localhost deployment, add `--address 0.0.0.0`, put it behind a TLS terminator, and serve it over a [secure context](features/input.md#clipboard-limitations) so the clipboard works; see [Security model](guide/security.md).

## Next steps

- [Features](features/comparison.md) - what the fork adds over stock Broadway.
- [Configuration reference](guide/config.md) - every env var, port, and client-side setting.
- [Requirements](guide/requirements.md) - host/client support and tested browser/OS combinations.
- [Known issues](guide/known-issues.md) - what doesn't work.
