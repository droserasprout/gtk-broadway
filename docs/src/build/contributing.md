# Contributing

This page is for people hacking on the **fork itself** - the patched GTK library in [`droserasprout/gtk-brotway`](https://github.com/droserasprout/gtk-brotway). If you just want to run an app over Broadway, you don't need any of this; see [Installation](../guide/installation.md) and [Running broadwayd](../guide/running.md) instead.

The tree is large, but you only touch a small corner of it. This page covers that corner, the branch conventions, and how a change reaches a published `.deb`.

> Not sure where a change belongs? Jump to [Where the code lives](#where-the-code-lives).

## Scope and philosophy

Four ground rules shape every patch:

- **Broadway only.** The Broadway backend plus the bits of GDK/GSK/GTK it needs - never the X11, Wayland, Win32, or macOS backends.
- **GTK 4.x only - never GTK5.** GTK5 removes Broadway (and X11) entirely, so the fork is anchored to the GTK 4.x lifetime; please do not open GTK5 ports. Background: [Re-forking a new GTK release](reforking.md).
- **Stay close to upstream.** Small, surgical diffs rebase and review easier; prefer the minimal change that solves the problem.
- **We do not upstream.** Patches live here for the life of GTK4, not in GNOME's GTK.
- **App-agnostic docs.** The fork serves any GTK4 binary. Docs and examples never name a specific downstream app or deployment - keep them generic (`your-gtk4-app`).

## Supported base

One GTK base: `4.22.4-brotway` (4.22.4 on the Ubuntu 26.04 base). Table, SONAME, and the patch diff: [Requirements](../guide/requirements.md).

## Branch naming

Feature and fix work lives on its own topic branch, branched off `4.22.4-brotway` and named for a short topic:

```
<topic>      # e.g. touch-multiple-selection
```

The integration branches (do not develop directly on these) are:

- `4.22.4-brotway` - the fork tip that topic branches merge into. This is what releases build from.
- `ci` - an orphan branch holding the reusable build/release workflows and this book. See [CI & packaging](ci.md).

### Patch families

Existing topic branches cluster into a few families. When you fix something, see if it belongs next to one of these:

- **Clipboard** - bi-directional text clipboard (copy/paste/cut + GtkTextView serialization).
- **Touch / Android** - `touch-multiple-selection`, `label-copy-bubble`, `treeview-touch-selection`, plus touch scroll, pinch-zoom, and IME keyboard work.
- **Renderer perf** - `broadway-perf` (texture dedup, node reuse, native edge-fade).
- **Debug menu** - `debug-menu`, the server-side debug menu and its control channel.
- **Connection / display** - `connection-management` (auto-reconnect, session liveness, single-display arbitration).
- **Geometry / scaling** - `scaling-issues`, `shadow-geometry`, window geometry fixes.
- **Crash / correctness** - `dnd-crash`, `fix/broadway-reuse-desync`.

If your change is a new topic, branch from `4.22.4-brotway` and name it `<your-topic>`.

## Dev workflow: worktrees

Each feature gets its own **git worktree**, so you can build and test branches in parallel without thrashing one checkout:

```sh
git worktree add ../gtk-wt/<topic> -b <topic> 4.22.4-brotway
```

Work in that directory, build there, and remove the worktree when the branch is merged (`git worktree remove`).

## Building and testing

The local build is Broadway-only with the exact same Meson config as CI; the full command and the build dependencies all live in [Building from source](from-source.md). The first build is heavy; `ccache` makes later rebuilds quick.

### JavaScript sanity check

The browser-side client is plain JavaScript (`gdk/broadway/broadway.js`). You don't need a full GTK build to check a JS edit - just run:

```sh
node --check gdk/broadway/broadway.js
```

### Build / restart iteration

Whether a change needs only a `gtk4-broadwayd` restart or the full library rebuilt and the app restarted: see [broadwayd vs libgtk](from-source.md#iterating-broadwayd-vs-libgtk). Build outputs are listed in [What you get](from-source.md#what-you-get).

## Code style

- **Match the surrounding code.** This is upstream GTK's style; the repo ships an `.editorconfig`, a `.clang-format`, and a `.flake8`. Follow them.
- **Short, human comments.** Explain the *why* in a line or two where the Broadway behaviour is non-obvious. Skip comments that just restate the code.
- **Minimal patches.** Stay as close to upstream as you can; the less you diverge, the easier the next [re-fork](reforking.md) onto a new 4.x point release.
- **Hyphens, not em or en dashes**, in comments and commit messages.
- **Conventional, brief commit subjects**, e.g. `broadway: ...`, `treeview: ...`, `gtklabel: ...`. Look at `git log --oneline` for the house style.

## Submitting a change (PR flow)

1. Branch from `4.22.4-brotway` using the `<topic>` naming above (ideally in a worktree).
2. Make the smallest change that does the job. Keep commits focused; keep subjects short.
3. Build the Broadway-only target locally, and `node --check` any `broadway.js` edit.
4. Open a PR against `4.22.4-brotway`. The pattern is: topic branch -> PR -> merge into `4.22.4-brotway`. A fork branch can run a validation build of itself via its `ci.yml` stub before merge (see [CI & packaging](ci.md#per-branch-validation)).

Every merged PR is labelled and milestoned so the release manifest stays accurate: a change-type label (`bug` / `enhancement` / ...) and the target `vX.Y.Z` milestone. This is what the [Release process](release.md#before-tagging) cross-checks against the CHANGELOG when cutting a release.

When in doubt about scope or which branch to target, open an issue or draft PR and ask - a short question up front saves a rebase later.

## Where the code lives

The Broadway backend is split across two directories:

- **`gdk/broadway/`** - the backend itself: the display server / daemon (`broadwayd.c`, `broadway-server.c`), the wire protocol (`broadway-protocol.h`, `broadway-output.c`), the GDK objects (`gdkdisplay-broadway.c`, `gdksurface-broadway.c`, `gdkclipboard-broadway.c`, `gdkdevice-broadway.c`, `gdkdnd-broadway.c`, ...), and the browser client (`broadway.js`, `client.html`).
- **`gsk/broadway/`** - the Broadway scene-graph renderer (`gskbroadwayrenderer.c`).

Most touch, clipboard, and connection work lands in `gdk/broadway/`; renderer perf lands in `gsk/broadway/`; the browser-side half of anything lives in `gdk/broadway/broadway.js`. The [Architecture](../internals/architecture.md) chapter maps the rest of the tree.
