# Deploying behind TLS

[Installation](installation.md) and [Running broadwayd](running.md) cover a single local session. This page is the operator view: serving the fork to real clients over the network, with a container and a TLS terminator.

## Why TLS is not optional

The [clipboard bridge](../features/clipboard.md) needs a [secure context](running.md#secure-context-note) (`https://` or `http://localhost`), so any deployment beyond localhost has to terminate TLS in front of the daemon. The reference deployment runs behind Traefik in Docker Swarm for exactly this reason.

## Access control - Broadway has none

Broadway ships **no authentication**. The daemon serves the app to anyone who can open `8080 + N`, and there is no login, token, or per-user check anywhere in the stack. Worse, the [single-display arbitration](../internals/connection.md#single-display-arbitration-newest-fresh-open-wins) means the **newest** fresh page load takes ownership of the session - so a stranger who reaches the port doesn't just view the app, they take over the live session from whoever is using it.

Treat the daemon port as fully trusted and never expose it directly. Put access control in front of it at the same TLS terminator:

- **HTTP basic auth** (Traefik `basicauth` middleware, nginx `auth_basic`, Caddy `basicauth`), or a forward-auth / SSO middleware for anything multi-user.
- **Network isolation** - bind the daemon to loopback or an internal Docker network so only the proxy can reach it; never publish `8080 + N` on a public interface.
- Remember auth gates *reaching* the app, not *who controls it*: there are no per-client sessions, so two authenticated users still contend for the one display.

## In a container

The reference deployment installs the arch-matching `.deb` inside a Docker image over apt's GTK and holds the runtime packages - the same three steps as [Installation](installation.md), plus `dpkg --print-architecture` to select between the amd64 and arm64 assets at build time:

```dockerfile
ARG GTK_VER=4.22.4
ARG REL=v3.0.0
RUN arch="$(dpkg --print-architecture)" \
 && wget -O /tmp/gtk.deb "https://github.com/droserasprout/gtk-brotway/releases/download/${REL}/gtk4-brotway_${GTK_VER}-${REL#v}_${arch}.deb" \
 && apt-get install -y /tmp/gtk.deb \
 && apt-mark hold libgtk-4-1 libgtk-4-bin \
 && rm /tmp/gtk.deb
```

The base image's GTK must match the `.deb` base ([4.22.4 on `ubuntu:26.04`](versions.md)); the overlay replaces the SONAME-versioned `.so` in place.

## Process layout

Inside the container two processes run, as in [Running broadwayd](running.md):

- `gtk4-broadwayd :N` owns the display and serves the browser page on `8080 + N`.
- the app, started with `GDK_BACKEND=broadway BROADWAY_DISPLAY=:N`, renders into it.

The TLS terminator (Traefik, nginx, Caddy, ...) proxies `https://your-host/` to the daemon's `8080 + N`, and must **forward WebSocket upgrades** - all display ops and input run over the same socket.

## Recipes

### Compose stack behind Traefik

The reference deployment, reduced to the generic case: the app container joins Traefik's network and publishes no ports, so the daemon (`:5` -> `8085`) is reachable only by the proxy. Traefik forwards WebSocket upgrades by default - one router carries the page and the socket, no header middleware needed.

```yaml
services:
  app:
    image: your-gtk4-app-image    # runs the two processes from Process layout
    networks: [proxy]
    # no ports: - the daemon stays internal
    labels:                       # Swarm: put these under deploy.labels
      - traefik.enable=true
      - traefik.http.routers.app.rule=Host(`app.example.com`)
      - traefik.http.routers.app.entrypoints=websecure
      - traefik.http.routers.app.tls.certresolver=letsencrypt
      - traefik.http.services.app.loadbalancer.server.port=8085
      - traefik.http.routers.app.middlewares=app-auth
      # htpasswd -nB user, with $ doubled to $$ for compose
      - traefik.http.middlewares.app-auth.basicauth.users=user:$$2y$$05$$...
    restart: unless-stopped

networks:
  proxy:
    external: true                # the network Traefik watches
```

### systemd units on bare metal

The same two processes as a daemon unit and an app unit bound to it. `BindsTo=` stops the app when the daemon goes away; a plain browser reload picks the session back up after a restart. Add `--address 127.0.0.1` to the daemon if the TLS proxy runs on the same host, so `8085` never listens publicly.

```ini
# /etc/systemd/system/broadwayd.service
[Unit]
Description=GTK Broadway display :5

[Service]
User=broadway
ExecStart=/usr/bin/gtk4-broadwayd :5
Restart=on-failure

[Install]
WantedBy=multi-user.target

# /etc/systemd/system/app.service
[Unit]
Description=GTK app on Broadway display :5
BindsTo=broadwayd.service
After=broadwayd.service

[Service]
User=broadway
Environment=GDK_BACKEND=broadway BROADWAY_DISPLAY=:5
ExecStart=/usr/bin/your-gtk4-app
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

## Checklist

- [ ] `.deb` base matches the image's GTK base, arch selected via `dpkg --print-architecture`.
- [ ] `apt-mark hold libgtk-4-1 libgtk-4-bin` so an image rebuild's `apt upgrade` can't revert it.
- [ ] TLS terminated in front of the daemon; WebSocket upgrade forwarded.
- [ ] Daemon port not published publicly; auth (basic/SSO) enforced at the proxy - Broadway has none.
- [ ] Reachable over `https://` (or `http://localhost` for local testing) so the clipboard works.

> For the daemon and app environment variables referenced here, see [Configuration reference](config.md).
