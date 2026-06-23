# Contributing

For people hacking on the **fork itself** - the patched GTK library in [`droserasprout/gtk-brotway`](https://github.com/droserasprout/gtk-brotway). To just run an app over Broadway, see [Installation](../guide/installation.md) and [Running](../guide/running.md) instead.

The tree is large, but you only touch a small corner. This page covers that corner, the branch conventions, and how a change reaches a published `.deb`.

## Scope and philosophy

- **Broadway only.** The Broadway backend plus the bits of GDK/GSK/GTK it needs.
- **GTK 4.x only.** GTK5 removes Broadway entirely, so the fork is anchored to the GTK 4.x lifetime.
- **Stay close to upstream.** Small, surgical diffs rebase and review easier.
- **We do not upstream.** Patches live here for the life of GTK4.
- **App-agnostic docs.** The fork serves any GTK4 binary; keep examples on upstream demo apps (`gtk4-demo`, `gtk4-widget-factory`, `gnome-calculator`).

## Supported base

One GTK base: `4.22.4-brotway` (4.22.4 on Ubuntu 26.04). Table, SONAME, and the patch diff: [Requirements](../guide/requirements.md).

## Branch naming

Feature and fix work lives on its own topic branch, branched off `4.22.4-brotway` and named for the topic (e.g. `touch-multiple-selection`). The integration branches (don't develop on these):

- `4.22.4-brotway` - the fork tip that topic branches merge into. Releases build from it.
- `ci` - an orphan branch holding the reusable build/release workflows and this book.

### Patch families

Existing topic branches cluster into a few families; when you fix something, see if it belongs next to one:

- **Clipboard** - bi-directional text clipboard.
- **Touch / Android** - touch selection, label/treeview selection, touch scroll, pinch-zoom, IME keyboard.
- **Renderer perf** - texture dedup, node reuse, native edge-fade.
- **Debug menu** - the server-side debug menu and its control channel.
- **Connection / display** - auto-reconnect, session liveness, single-display arbitration.
- **Geometry / scaling** - scaling, shadow geometry, window geometry.
- **Crash / correctness** - DnD crash, broadway-reuse desync.

## Dev workflow: worktrees

Each feature gets its own git worktree, so you can build and test branches in parallel:

```sh
git worktree add ../gtk-wt/<topic> -b <topic> 4.22.4-brotway
```

## Building and testing

The local build is Broadway-only with the same Meson config as CI - full command and dependencies in [Building from source](from-source.md). The browser-side client is plain JS; check an edit without a full build:

```sh
node --check gdk/broadway/broadway.js
```

Whether a change needs only a `gtk4-broadwayd` restart or a full library rebuild: [broadwayd vs libgtk](from-source.md#iterating-broadwayd-vs-libgtk).

## Code style

- **Match the surrounding code** - upstream GTK's style. The repo ships `.editorconfig`, `.clang-format`, `.flake8`.
- **Short, human comments** explaining the *why* where Broadway behaviour is non-obvious.
- **Minimal patches** - the less you diverge, the easier the next re-fork.
- **Hyphens, not em/en dashes**, in comments and commit messages.
- **Conventional, brief commit subjects** (`broadway: ...`, `treeview: ...`). See `git log --oneline`.

## Submitting a change (PR flow)

1. Branch from `4.22.4-brotway` using the `<topic>` naming (ideally in a worktree).
2. Make the smallest change that does the job; keep commits focused.
3. Build the Broadway-only target, and `node --check` any `broadway.js` edit.
4. Open a PR against `4.22.4-brotway`. A fork branch can run a validation build via its `ci.yml` stub before merge.

Label and milestone every merged PR (change-type label + target `vX.Y.Z` milestone) - the release process cross-checks these against the CHANGELOG.

## Where the code lives

The Broadway backend is split across two directories:

- **`gdk/broadway/`** - the backend: daemon (`broadwayd.c`, `broadway-server.c`), wire protocol (`broadway-protocol.h`, `broadway-output.c`), GDK objects (`gdk*-broadway.c`), and the browser client (`broadway.js`, `client.html`).
- **`gsk/broadway/`** - the Broadway scene-graph renderer (`gskbroadwayrenderer.c`).

Most touch, clipboard, and connection work lands in `gdk/broadway/`; renderer perf in `gsk/broadway/`; the browser-side half in `broadway.js`. The [Architecture](../internals/architecture.md) chapter maps the rest.
