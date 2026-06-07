# Known issues

- **WONTFIX - PRIMARY selection / middle-click paste.** Desktop browsers expose no JavaScript API
  for the X11-style PRIMARY selection, so it cannot be bridged host <-> guest.
- **Touch - first tap leaks into a pinch/drag gesture.** When a second finger lands to start a
  pinch or pan, the first finger's tap can still register. No solution found that doesn't
  introduce input latency.
- **Touch - the emoji widget is slow and ugly** over Broadway.
- **Copy on insecure (`http`) origins** may silently fail outside a user gesture, because
  `navigator.clipboard` is gated to secure contexts. Serve over `https://` or `http://localhost`.
- **Roundtrip storm (app-amplified).** Any continuously-repainting surface (a spinner, a
  transfer-progress animation) makes Broadway round-trip after every paint, which can flood the
  socket. This is pre-existing upstream behaviour, amplified by busy apps; fixing it means
  throttling the frame clock or stopping the animation. Out of scope for the fork.

## Tested configurations

- **Desktop:** Firefox 151.0.2, Chromium 148.0.7778.178
- **Android:** Firefox Beta 152.0b4, Google Chrome 146.0.7680.119
- **Transport:** HTTP on `localhost`, and HTTPS behind Traefik in Docker Swarm

## Not tested

- iOS
- Mixed devices (e.g. laptops with a touchscreen)
