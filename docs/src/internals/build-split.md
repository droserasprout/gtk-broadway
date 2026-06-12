# broadwayd vs libgtk

Every fork change is one of two kinds, and the distinction drives both how you iterate on it and how a release ships it. Each [feature implementation](../features/comparison.md) page tags its changes with this split.

## The two kinds

**broadwayd-only** - a change to `broadway.js`, `client.html`, or the daemon C (`broadwayd.c`, `broadway-server.c`, `broadway-output.c`). To iterate: rebuild and restart the daemon, then reload the browser. `client.html` / `broadway.js` are embedded into the daemon (generated into `clienthtml.h` / `broadwayjs.h` at build time), so even a pure JS change needs the daemon rebuilt - but `ninja` does that incrementally, and the page is served `no-store`, so a plain reload picks it up. No app restart.

**libgtk** - a change to the GDK Broadway backend, the GSK Broadway renderer, or a GTK widget. To iterate: rebuild `libgtk-4.so` and restart the **app**.

## Why it matters for iteration

| Change in... | Rebuild | Restart | Browser |
|---|---|---|---|
| `broadway.js` / `client.html` / daemon C | daemon (`ninja`, incremental) | daemon | plain reload |
| GDK backend / GSK renderer / GTK widget | `libgtk-4.so` | the app | plain reload |

So a client-side tweak is a fast loop (restart daemon, reload tab), while a widget fix is the slow loop (rebuild the library, restart the app).

## Why it matters for shipping

The `gtk4-brotway` `.deb`, built on the single GTK 4.22.4 base (`4.22.4-brotway`), ships **both** the patched `libgtk-4.so` and the `gtk4-broadwayd` binary (which carries the embedded client), so a single release always covers both kinds of change. See [Installation](../guide/installation.md) and [CI & packaging](../build/ci.md).
