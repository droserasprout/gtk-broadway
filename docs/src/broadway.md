# What is Broadway?

Broadway is GTK's HTML5 backend: instead of drawing to a local display (Wayland, X11, Win32,
macOS), it renders the app into a web browser over a WebSocket. A `gtk4-broadwayd` daemon owns a
virtual display and serves a page; any browser that connects to it sees and drives the running GTK
app. No app code changes - the same binary picks Broadway through `GDK_BACKEND=broadway`.

That makes it the simplest way to put a native GTK app on a screen it was never built for: a phone
browser, a remote machine, a kiosk, a tab next to everything else. This fork exists because the
stock backend gets you the picture but not much else - see [why this fork](#why-fork).

## A short history

Broadway started as Alexander Larsson's "[GTK+ 3.0 HTML5
backend](https://blogs.gnome.org/alexl/2010/11/26/gtk-3-0-html5-backend/)" prototype in late 2010
and [merged for GTK 3.2](https://blogs.gnome.org/alexl/2011/03/15/gtk-html-backend-update/) in 2011.
The first version simply streamed image frames of the window to the browser.

GTK4 reworked it around the new render-node pipeline
([Larsson, 2019](https://blogs.gnome.org/alexl/2019/03/29/broadway-adventures-in-gtk4/)): the GSK
Broadway renderer turns the app's render nodes into a compact node stream the browser reconstructs in
the DOM, rasterizing with cairo only for nodes Broadway can't express. That node-stream design is
what the fork builds on; see [Architecture](internals/architecture.md).

Through all of this Broadway stayed an experimental, lightly-maintained corner of GTK.

## Deprecation

In February 2025, GTK [deprecated both the X11 and Broadway
backends](https://www.phoronix.com/news/GTK-X11-Now-Deprecated) (in the 4.17.4 development release,
then stable **4.18**), to be **removed entirely in GTK5**. Broadway was cut for never advancing past
its experimental stage; GTK's display effort now centers on Wayland.

There is no GTK5 Broadway to move to, so this stack is anchored to the GTK 4.x lifetime: maintained
against 4.x, deliberately **not** ported to GTK5. How the fork is carried onto each new 4.x point
release, and why it stops at GTK4, is in [Re-forking a new GTK release](build/reforking.md).

## Why fork {#why-fork}

Stock Broadway was always a tech demo, not a real backend - it renders the app and handles mouse and
keyboard, and stops there. It's missing the obvious things (clipboard, touch, session recovery),
which is exactly why nobody shipped on it. But the core idea is genuinely good: zero-install access
to a native GTK app from any browser, including a phone, with no per-app porting and no app code
changes. Finished, that's a real way to run a desktop app anywhere there's a tab.

Getting there is also just fun. The patches live entirely in one self-contained backend - render
nodes to DOM, a WebSocket wire protocol, touch and clipboard bridged into the browser - so the work
is small, visible, and end-to-end: change one file, refresh the page, see it. Because it's
deprecated upstream there's no moving target and no review queue to satisfy; the only bar is making
it actually work.

And it's cheap to keep alive. The fork is a thin layer on stock GTK4, touching only the Broadway
backend and never app-facing API, so each 4.x point release re-forks with a minimal diff. That's the
whole bet: a small, bounded patch set turns an abandoned experiment into something you'd actually
use, for as long as GTK 4.x lives. What it adds, feature by feature, is in the
[Backend comparison](features/comparison.md); the design constraints are in the
[Introduction](introduction.md#goals).
