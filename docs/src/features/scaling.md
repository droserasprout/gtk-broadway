# Scaling & HiDPI

Icons (and anything under a scale or rotate transform) rendered blurry on HiDPI displays and under the fork's [pinch-zoom](zoom.md). The Broadway GSK renderer applied those transforms as a client-side matrix on an already-rasterized texture, so the bitmap got scaled up instead of being re-rasterized at the target resolution.

## Fix

Two changes. First, scale/rotate transforms now go through the cairo fallback at the display resolution rather than being applied as a matrix to a lower-resolution texture, so the result comes out sharp. Second, Broadway reuses cached textures across frames, so on a scale change we drop the cache; the cached textures then re-rasterize at the new scale.

Icons are no longer blurry under scale transforms or on HiDPI, and the re-render after a [pinch](zoom.md) settles crisp.
