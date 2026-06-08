# Deploying behind TLS

[Installation](installation.md) and [Running broadwayd](running.md) cover a single local session.
This page is the operator view: serving the fork to real clients over the network, which means a
container and a TLS terminator.

## Why TLS is not optional

The [clipboard bridge](../features/clipboard.md) uses `navigator.clipboard`, which browsers expose
only in a **secure context**: `https://` or `http://localhost`. Over plain `http://` to a remote
host, copy may silently fail outside a user gesture. So any deployment that isn't `localhost` has to
terminate TLS in front of the daemon. The reference deployment runs behind Traefik in Docker Swarm
for exactly this reason.

## In a container

The reference deployment installs the arch-matching `.deb` inside a Docker image over apt's GTK and
holds the runtime packages - the same three steps as [Installation](installation.md), plus
`dpkg --print-architecture` to select between the amd64 and arm64 assets at build time:

```dockerfile
ARG GTK_VER=4.22.2
ARG REL=v1
RUN arch="$(dpkg --print-architecture)" \
 && wget -O /tmp/gtk.deb "https://github.com/droserasprout/gtk-broadway/releases/download/${REL}/gtk4-broadway-fork_${GTK_VER}-${REL#v}_${arch}.deb" \
 && apt-get install -y /tmp/gtk.deb \
 && apt-mark hold libgtk-4-1 libgtk-4-bin \
 && rm /tmp/gtk.deb
```

The base image's GTK must match the `.deb` base ([4.22.2 on `ubuntu:26.04`, 4.14.5 on
`ubuntu:24.04`](versions.md)); the overlay replaces the SONAME-versioned `.so` in place.

## Process layout

Inside the container two processes run, as in [Running broadwayd](running.md):

- `gtk4-broadwayd :N` owns the display and serves the browser page on `8080 + N`.
- the app, started with `GDK_BACKEND=broadway BROADWAY_DISPLAY=:N`, renders into it.

The TLS terminator (Traefik, nginx, Caddy, ...) proxies `https://your-host/` to the daemon's
`8080 + N`, and must **forward WebSocket upgrades** - all display ops and input run over the same
socket.

## Checklist

- [ ] `.deb` base matches the image's GTK base, arch selected via `dpkg --print-architecture`.
- [ ] `apt-mark hold libgtk-4-1 libgtk-4-bin` so an image rebuild's `apt upgrade` can't revert it.
- [ ] TLS terminated in front of the daemon; WebSocket upgrade forwarded.
- [ ] Reachable over `https://` (or `http://localhost` for local testing) so the clipboard works.

> For the daemon and app environment variables referenced here, see
> [Configuration reference](config.md).
