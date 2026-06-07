# Scaling & HiDPI

Icons (and anything under a scale or rotate transform) rendered blurry on HiDPI displays and under
the fork's [pinch-zoom](zoom.md). The Broadway GSK renderer applied those transforms as a
client-side matrix on an already-rasterized texture, so the bitmap was scaled up rather than
re-rasterized at the target resolution.

## Fix

- **Rasterize at display resolution.** Scale/rotate transforms go through the cairo fallback at the
  **display resolution** rather than being applied as a matrix to a lower-resolution texture, so
  the result is sharp.
- **Invalidate the node-reuse cache on scale change.** Broadway reuses cached textures across
  frames; on a scale change the cache is dropped so cached textures re-rasterize crisply at the new
  scale instead of being stretched.

Icons are no longer blurry under scale transforms or on HiDPI, and the re-render after a
[pinch](zoom.md) settles crisp.
