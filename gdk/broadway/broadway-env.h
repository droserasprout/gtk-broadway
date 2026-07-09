#pragma once

#include <glib.h>

/* Brotway: promote any legacy BROADWAY_* env var to its BROTWAY_* name once at
 * startup (warning per var), so the rest of the code reads only BROTWAY_*. */
void broadway_migrate_legacy_env (void);

/* Brotway: parse a boolean env var. Recognizes 1/0, true/false, yes/no, on/off
 * (case-insensitive); unset, empty, or unrecognized returns fallback. */
gboolean broadway_env_flag (const char *name,
                            gboolean    fallback);
