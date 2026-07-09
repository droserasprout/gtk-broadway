# Security model

Broadway was built for a trusted local display, not the open network. The fork keeps that posture:

- **No authentication.** The daemon serves the app to anyone who can open `8080 + N`. No login, token, or per-user check anywhere in the stack.
- **No transport encryption.** Plain HTTP and WebSocket; nothing is encrypted until you put TLS in front of it.
- **Last connected browser wins.** [Single-display arbitration](../internals/connection.md#single-display-arbitration-newest-fresh-open-wins) hands the live session to the newest fresh page load, so a stranger who reaches the port takes it over from whoever is using it.

Treat the daemon port as fully trusted and never expose it directly. To put a session on the network: terminate TLS, add auth, and keep the port private.

## Why TLS is not optional

The [clipboard bridge](../features/input.md#clipboard) needs a [secure context](running.md#secure-context-note), so any deployment beyond localhost has to terminate TLS in front of the daemon.

## Adding access control

Broadway has none of its own, so put it at the TLS terminator:

- **HTTP basic auth** (Traefik `basicauth`, nginx `auth_basic`, Caddy `basicauth`), or forward-auth / SSO for multi-user.
- **Network isolation** - bind the daemon to loopback or an internal Docker network so only the proxy reaches it, keeping `8080 + N` off public interfaces.
- Auth gates *reaching* the app, not *who controls it*: there are no per-client sessions, so two authenticated users still contend for the one display.

## In a container

Build so every app loads the fork via `LD_LIBRARY_PATH=/usr/lib/gtk4-brotway` - `FROM` the prebuilt base or bake the `.deb` in ([Docker](docker.md)). The base image's GTK must match the `.deb` base ([4.22.4 on `ubuntu:26.04`](requirements.md)).

## Process layout

Inside the container two processes run, as in [Running](running.md#by-hand), both loading the fork via `LD_LIBRARY_PATH=/usr/lib/gtk4-brotway`:

- `/usr/lib/gtk4-brotway/gtk4-broadwayd :N` owns the display and serves the page on `8080 + N`.
- the app, started with `GDK_BACKEND=broadway BROTWAY_DISPLAY=:N`, renders into it.

The TLS terminator proxies `https://your-host/` to the daemon's `8080 + N`, and must **forward WebSocket upgrades** - all display ops and input run over the same socket.

## Recipes

### Compose stack behind Traefik

The app container joins Traefik's network and publishes no ports, so the daemon (`:5` -> `8085`) is reachable only by the proxy. Traefik forwards WebSocket upgrades by default.

```yaml
services:
  app:
    image: your-gtk4-app-image    # changeme; runs the two processes from Process layout
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

The same two processes as a daemon unit and an app unit bound to it. `BindsTo=` stops the app when the daemon goes away. Add `--address 127.0.0.1` to the daemon if the TLS proxy runs on the same host, so `8085` never listens publicly.

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
Environment=LD_LIBRARY_PATH=/usr/lib/gtk4-brotway GDK_BACKEND=broadway BROTWAY_DISPLAY=:5
# changeme
ExecStart=/usr/bin/gtk4-demo
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

## Checklist

- [ ] `.deb` base matches the image's GTK base, arch selected via `dpkg --print-architecture`.
- [ ] `LD_LIBRARY_PATH=/usr/lib/gtk4-brotway` set so apps load the fork.
- [ ] TLS terminated in front of the daemon; WebSocket upgrade forwarded.
- [ ] Daemon port not published publicly; auth enforced at the proxy.
- [ ] Reachable over `https://` (or `http://localhost`) so the clipboard works.

> Daemon and app environment variables: [Configuration reference](config.md).
