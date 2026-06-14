# Base Brotway image: a stock Ubuntu GTK runtime with the brotway Broadway .deb
# installed into a private prefix. App-agnostic - FROM this for any GTK4 Broadway
# binary; it carries no app, no Python, no bindings, just the patched GTK +
# gtk4-broadwayd.
#
# The .deb installs the broadway-only fork lib + daemon under /usr/lib/gtk4-brotway
# WITHOUT replacing the system GTK (so it's desktop-safe). Inside this container
# we want every GTK4 app to use the fork, so LD_LIBRARY_PATH opts them all in -
# the launcher's per-process trick made global. broadwayd lives in the prefix;
# start it via `gtk4-brotway-run` or by full path.
#
# Built multi-arch by .github/workflows/image.yml, which downloads both per-arch
# .debs from the matching GitHub Release into ./debs before this build.
ARG UBUNTU_VERSION=26.04
FROM ubuntu:${UBUNTU_VERSION}
ARG DEBIAN_FRONTEND=noninteractive

# Broadway is the only backend here, so make it the default; point the loader at
# the fork prefix so every app picks up the patched libgtk-4; silence the missing
# at-spi a11y-bus warnings (no session a11y bus under Broadway).
ENV GDK_BACKEND=broadway \
    LD_LIBRARY_PATH=/usr/lib/gtk4-brotway \
    NO_AT_BRIDGE=1

# Both per-arch .debs land in ./debs; pick the one for the target arch. Under
# buildx emulation dpkg reports the emulated target arch, so this resolves per
# platform in a single multi-arch build.
COPY debs/ /tmp/debs/
RUN set -eux; \
    arch="$(dpkg --print-architecture)"; \
    deb="$(ls /tmp/debs/gtk4-brotway_*_${arch}.deb)"; \
    apt-get update; \
    # apt pulls the deb's Depends (libgtk-4-1, libgtk-4-bin) for the rest of the
    # GTK runtime; the fork lib sits beside them in its prefix, no hold needed.
    # librsvg2-common + adwaita-icon-theme aren't deb deps under --no-install-
    # recommends, but GTK's symbolic icons are SVG - without them apps render
    # iconless. Generic to any GTK4 app, so they belong in the base.
    apt-get install -y --no-install-recommends \
      "${deb}" librsvg2-common adwaita-icon-theme; \
    rm -rf /tmp/debs /var/lib/apt/lists/*; \
    apt-get clean
