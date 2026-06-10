# Opening links

<!-- SCREENCAST (pending) - uncomment after recording. See devnotes/2026-06-10-docs-screencasts.md
  Record: make record SCENARIO=tools/local/scenarios/open-uri.json OUT=docs/src/images/open-uri.mp4

<video src="../images/open-uri.mp4" poster="../images/open-uri.png"
  autoplay loop muted playsinline style="max-width:100%;border-radius:6px">
  <img src="../images/open-uri.png" alt="Clicking a link routes the URI to the browser and opens a new tab"
    style="max-width:100%;border-radius:6px">
</video>
-->

Clicking a link in a headless Broadway container logged *"No known URI provider available"* and did nothing - there is no system URI handler. The fork routes the URI over the Broadway protocol to the browser viewing the WebUI, which opens it in a new tab.

## What works

- **Clicked links open in a new browser tab** - the one viewing the app.
- Covers the standard GTK link paths: the deprecated `gtk_show_uri`, the modern `GtkUriLauncher.launch`, and `GtkLabel` / `GtkLinkButton` auto-link activation.

## App-side integration

A URI-opening helper can add a `GDK_BACKEND == "broadway"` branch that calls `Gtk.show_uri(None, uri, 0)` before the GIO path, and falls back gracefully on unpatched GTK.

## Caveats

- It runs inside the WebSocket message handler rather than a user gesture, so a popup blocker may catch it.
- `file://` URIs get routed too, but browsers block `window.open("file://...")` from an http(s) origin. That's no worse than the prior failure.

> The `BROADWAY_OP_OPEN_URI` op, the wire path, and the introspection workaround that lets Python reach a Broadway-only GDK function are in [Open URI implementation](../internals/open-uri.md).
