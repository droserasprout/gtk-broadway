# Window management

Broadway is a remote-display protocol, not a window manager. The **browser** composites (each surface is a `div`, stacked by `z-index`); **`broadwayd`** does a thin WM emulation over the wire (stacking, grabs, focus, transients); and **GTK** does client-side decorations and popup positioning. So the "WM" here is a minimal single-seat, single-canvas layer - deliberately not a compositor like Mutter, and it needn't be.

## What `broadwayd` manages

- **Stacking** - a flat surface list, mirrored to browser `z-index`, plus a binary always-on-top layer: [`restack_layers`](input-region.md#always-on-top) keeps `keep_above` surfaces on top.
- **Grabs** - a single global pointer grab, held as a nesting [stack](input-region.md#grab-stack) for popup chains; keyboard goes to one `focused_surface_id`. The keep-above [carve-out](input-region.md#always-on-top) is the one exception to "all input follows the grab".
- **Focus** - one keyboard focus; focus-on-click and focus-on-map (modal popups).
- **Window ops** - CSD move/resize (client-driven via an emulation surface), raise/lower, `transient_for` (children stack above parents), `modal_hint` (dim + block the parent).

## vs Mutter

| | Mutter | Brotway |
|---|---|---|
| Role | Wayland compositor + WM | wire protocol; browser composites, daemon = minimal WM, GTK = CSD |
| Stacking | layered (`below < normal < above < …`) + per-window above/below | flat list + binary keep-above |
| Always-on-top | `_NET_WM_STATE_ABOVE`, user + app toggleable | per-surface `keep_above` (API); set-only, no re-lower |
| Input grabs | per-client Wayland popup grabs | one global daemon grab, now a nesting stack; carve-out works around it |
| Focus | focus stack + policies + steal-prevention | single `focused_surface_id`; click / map / partial keyboard-follows-grab |
| Move / resize | WM-driven: constraints, snap, tiling | CSD, client-driven; raise + drag; no snap/tile |
| Max / min / fullscreen | full WM state | maximize opt-in, minimize hidden, F11 fullscreen |
| Transients / modality | transient-for, modal grabs | `transient_for` stacking + `modal_hint` dim/block |
| Workspaces / multi-monitor | yes | no - one canvas = the browser viewport |
| Compositing / effects | GL compositor | browser `z-index`; zoom is a CSS transform |
| Multi-seat | yes, per-client isolation | single seat; all clients share one grab/focus |

## Non-goals

The right column is not a roadmap. Broadway (and X11) are deprecated and removed in GTK5, so the stack is anchored to GTK 4.x and stays a minimal emulation. There is no combinatorial complexity to manage - single seat, single screen, binary layer - so the WM state lives as a few small helpers (`surface_is_above`, `restack_layers`, the grab-stack push/pop, `pointerInSurface`), not subsystems. The structure that matters is the daemon (state + routing) / browser (compositing) / GTK (CSD) split.
