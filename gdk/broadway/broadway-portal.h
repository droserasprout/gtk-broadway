#pragma once

#include <glib.h>

/* Brotway prototype: a minimal org.freedesktop.portal.Settings service so a
 * containerized libadwaita app follows the browser's prefers-color-scheme. The
 * daemon owns org.freedesktop.portal.Desktop on the session bus and reports
 * org.freedesktop.appearance color-scheme (0 no-preference, 1 dark, 2 light). */

/* Start the portal on the session bus. Call once from the daemon's main; safe to
 * call when no session bus is available (it just never acquires the name). */
void broadway_portal_start (void);

/* Update the reported color-scheme (0/1/2) and emit SettingChanged. Called when
 * the browser reports its prefers-color-scheme over the Broadway protocol.
 * A no-op when BROTWAY_COLOR_SCHEME pins the scheme. */
void broadway_portal_set_color_scheme (guint32 scheme);
