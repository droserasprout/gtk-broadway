# syntax=docker/dockerfile:1
# Base Brotway image: stock Ubuntu GTK runtime + the brotway Broadway .deb in a
# private prefix. App-agnostic - FROM this for any GTK4 Broadway binary; no app,
# Python, or bindings, just the patched GTK + gtk4-broadwayd.
#
# The .deb installs the fork lib + daemon under /usr/lib/gtk4-brotway without
# touching the system GTK (desktop-safe). LD_LIBRARY_PATH opts every app in the
# container into the fork; broadwayd lives in the prefix (run via gtk4-brotway-run
# or full path). Built multi-arch by .github/workflows/image.yml, which drops both
# per-arch .debs into ./debs first.
ARG UBUNTU_VERSION=26.04
FROM ubuntu:${UBUNTU_VERSION}
ARG DEBIAN_FRONTEND=noninteractive
# Set to 1 (the `-nogl` variant) to drop the stock GTK + GL/Mesa/LLVM chain after
# install (~200 MB, mostly libllvm + mesa-gallium). Broadway never uses the GL
# renderer and the fork lib lives in /usr/lib/gtk4-brotway. Empty = full image.
ARG NOGL=

# Broadway is the only backend; point the loader at the fork prefix; silence the
# missing at-spi a11y-bus warnings (no session a11y bus under Broadway).
ENV GDK_BACKEND=broadway \
    LD_LIBRARY_PATH=/usr/lib/gtk4-brotway \
    NO_AT_BRIDGE=1

# Both per-arch .debs land in ./debs; pick the target arch. Under buildx emulation
# dpkg reports the emulated arch, so this resolves per platform. Bind-mount the debs
# (build-only) so they never land in a layer.
COPY packaging/debian/shim /tmp/shim.control
RUN --mount=type=bind,source=debs,target=/tmp/debs \
    set -eux; \
    arch="$(dpkg --print-architecture)"; \
    deb="$(ls /tmp/debs/gtk4-brotway_*_${arch}.deb)"; \
    apt-get update; \
    # apt pulls the deb's Depends (libgtk-4-1/-bin) for the rest of the GTK runtime.
    # librsvg2-common + adwaita-icon-theme aren't deb deps but GTK's symbolic icons
    # are SVG - without them apps render iconless. Generic, so they go in the base.
    apt-get install -y --no-install-recommends \
      "${deb}" librsvg2-common adwaita-icon-theme; \
    # -nogl: purge the unused stock GTK + GL/Mesa/LLVM chain in the SAME layer as
    # the install (a later RUN would only whiteout the files). First install a shim
    # that Provides libgtk-4-1/-bin (versioned, no files) so the dpkg graph stays
    # consistent and the base stays apt-expandable (a downstream app can install
    # gir1.2-gtk-4.0/python3-gi without pulling the GL chain back). Then dpkg
    # --force-depends (not apt) so on-paper dependents aren't cascaded out; filter
    # to installed pkgs so a missing one no-ops.
    if [ -n "${NOGL}" ]; then \
      gtkver="$(dpkg-query -W -f='${Version}' libgtk-4-1)"; \
      mkdir -p /tmp/shim/DEBIAN; \
      sed "s|\${ARCH}|${arch}|g; s|\${GTK_DEB_VERSION}|${gtkver}|g" \
        /tmp/shim.control > /tmp/shim/DEBIAN/control; \
      dpkg-deb --build /tmp/shim /tmp/gtk4-brotway-shim.deb; \
      dpkg -i /tmp/gtk4-brotway-shim.deb; \
      apt-mark hold gtk4-brotway-shim; \
      pkgs="libgtk-4-1 libgtk-4-bin libgstreamer-gl1.0-0 libgl1 libglx0 libegl1 \
            libgbm1 libgl1-mesa-dri libegl-mesa0 libglx-mesa0 mesa-libgallium libllvm21"; \
      inst="$(for p in $pkgs; do dpkg-query -W -f='${db:Status-Status} ${Package}\n' "$p" 2>/dev/null | awk '$1=="installed"{print $2}'; done)"; \
      [ -n "$inst" ] && dpkg --purge --force-depends $inst; \
    fi; \
    rm -rf /tmp/shim /tmp/shim.control /tmp/gtk4-brotway-shim.deb /var/lib/apt/lists/*; \
    apt-get clean
