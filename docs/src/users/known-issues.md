# Known issues

PRIMARY selection and middle-click paste are WONTFIX. Desktop browsers expose no JavaScript API
for the X11-style PRIMARY selection, so there's no way to bridge it between host and guest.

Two touch problems remain. The first tap can leak into a pinch or drag: when a second finger lands
to start a pinch or pan, the first finger's tap still registers, and I haven't found a fix that
avoids adding input latency. The emoji widget is also slow and ugly over Broadway.

Copy on insecure (`http`) origins may silently fail outside a user gesture, since
`navigator.clipboard` is gated to secure contexts. Serve over `https://` or `http://localhost`.

A continuously-repainting surface such as a spinner or a transfer-progress animation makes Broadway
round-trip after every paint, which can flood the socket. This is pre-existing upstream behaviour
that busy apps amplify. Fixing it means throttling the frame clock or stopping the animation, which
is out of scope for the fork.

## Tested configurations

- **Desktop:** Firefox 151.0.2, Chromium 148.0.7778.178
- **Android:** Firefox Beta 152.0b4, Google Chrome 146.0.7680.119
- **Transport:** HTTP on `localhost`, and HTTPS behind Traefik in Docker Swarm

## Not tested

- iOS
- Mixed devices (e.g. laptops with a touchscreen)
