# Security model

Broadway was built for a trusted local display, not the open network. The fork keeps that posture:

- **No authentication.** The daemon serves the app to anyone who can open `8080 + N`. There is no login, token, or per-user check anywhere in the stack.
- **No transport encryption.** The daemon speaks plain HTTP and WebSocket; nothing is encrypted until you put TLS in front of it.
- **Last connected browser wins.** The [single-display arbitration](../internals/connection.md#single-display-arbitration-newest-fresh-open-wins) hands the live session to the **newest** fresh page load. A stranger who reaches the port doesn't just view the app, they take it over from whoever is using it.

Treat the daemon port as fully trusted and never expose it directly. To put a session on the network anyway: terminate TLS, add auth, and keep the port private.

## Why TLS is not optional

The [clipboard bridge](../features/clipboard.md) needs a [secure context](running.md#secure-context-note) (`https://` or `http://localhost`), so any deployment beyond localhost has to terminate TLS in front of the daemon. The reference deployment runs behind Traefik in Docker Swarm.

## Adding access control

Broadway has none of its own (see the model above), so put it at the same TLS terminator:

- **HTTP basic auth** (Traefik `basicauth` middleware, nginx `auth_basic`, Caddy `basicauth`), or a forward-auth / SSO middleware for anything multi-user.
- **Network isolation** - bind the daemon to loopback or an internal Docker network so only the proxy can reach it; never publish `8080 + N` on a public interface.
- Remember auth gates *reaching* the app, not *who controls it*: there are no per-client sessions, so two authenticated users still contend for the one display.

## In a container

The reference deployment installs the arch-matching `.deb` inside a Docker image and points `LD_LIBRARY_PATH` at the fork prefix so every app in the container uses it - the same steps as [Installation](installation.md), plus `dpkg --print-architecture` to select between the amd64 and arm64 assets at build time:

```dockerfile
ARG GTK_VER=4.22.4
ARG REL=v3.1.1
RUN arch="$(dpkg --print-architecture)" \
 && wget -O /tmp/gtk.deb "https://github.com/droserasprout/gtk-brotway/releases/download/${REL}/gtk4-brotway_${GTK_VER}-${REL#v}_${arch}.deb" \
 && apt-get install -y /tmp/gtk.deb \
 && rm /tmp/gtk.deb
ENV LD_LIBRARY_PATH=/usr/lib/gtk4-brotway
```

The base image's GTK must match the `.deb` base ([4.22.4 on `ubuntu:26.04`](requirements.md)); the fork lib loads the system GTK's schemas and loaders. (The prebuilt base image already does all this.)

## Process layout

Inside the container two processes run, as in [Running](running.md#by-hand). Both load the fork via `LD_LIBRARY_PATH=/usr/lib/gtk4-brotway` (the base image sets it):

- `/usr/lib/gtk4-brotway/gtk4-broadwayd :N` owns the display and serves the browser page on `8080 + N`.
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
Environment=LD_LIBRARY_PATH=/usr/lib/gtk4-brotway
ExecStart=/usr/lib/gtk4-brotway/gtk4-broadwayd :5
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
Environment=LD_LIBRARY_PATH=/usr/lib/gtk4-brotway GDK_BACKEND=broadway BROADWAY_DISPLAY=:5
ExecStart=/usr/bin/your-gtk4-app
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

## Checklist

- [ ] `.deb` base matches the image's GTK base, arch selected via `dpkg --print-architecture`.
- [ ] `LD_LIBRARY_PATH=/usr/lib/gtk4-brotway` set so apps load the fork (the base image already sets it).
- [ ] TLS terminated in front of the daemon; WebSocket upgrade forwarded.
- [ ] Daemon port not published publicly; auth (basic/SSO) enforced at the proxy - Broadway has none.
- [ ] Reachable over `https://` (or `http://localhost` for local testing) so the clipboard works.

> For the daemon and app environment variables referenced here, see [Configuration reference](config.md).
