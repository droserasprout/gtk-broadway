# Glossary

Terms that recur across the book, for readers coming from outside GTK.

**Broadway** - GTK's HTML5 backend: renders a GTK app into a web browser over a WebSocket instead of to a local display. See the [Introduction](introduction.md#broadway).

**`gtk4-broadwayd`** - the Broadway daemon. Owns the virtual display, serves the browser page, and multiplexes between the app and connected browsers.

**GDK** - GTK's windowing and input layer. The Broadway *GDK backend* (`gdk/broadway/`) is the app-side half that talks to the daemon.

**GSK** - GTK's scene-graph / rendering layer. The Broadway *GSK renderer* (`gsk/broadway/`) turns render nodes into Broadway's node stream.

**Render node** - GTK's unit of drawing: a tree of typed nodes (text, texture, color, gradient, ...) describing a frame, rather than raw pixels.

**Node stream** - the serialization of those render nodes the daemon sends to the browser, which reconstructs them in the DOM. Nodes Broadway can't express fall back to a cairo-rasterized texture.

**libgtk** - the patched `libgtk-4.so`, the other half of what the `.deb` ships (alongside the daemon). A change to it needs the **app** restarted, not just the daemon. See [broadwayd vs libgtk](build/from-source.md#iterating-broadwayd-vs-libgtk).

**SONAME** - the versioned shared-object name (e.g. `libgtk-4.so.1.2200.4`). The fork ships its own copy in the prefix but loads the system GTK's schemas and loaders, so the package base and the system GTK must be the same 4.22 series. See [Requirements](guide/requirements.md).

**Secure context** - a browser security state (`https://` or `http://localhost`) that gates `navigator.clipboard`. Over plain remote `http://`, the [clipboard](features/input.md#clipboard) can fail silently.

**Input region** - the part of a surface that takes pointer events. The fork sends a per-surface region so a popover's shadow margin passes clicks through and text handles don't block taps. See [Input region & pointer](internals/input-region.md).

**Content dedup** - the fork's texture/node reuse: identical content reuses an already-uploaded texture id instead of re-encoding and re-sending it. See [Performance](internals/performance.md).

**Re-fork** - carrying the fork commits onto a new upstream GTK 4.x point release with a minimal diff.
