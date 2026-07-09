#include "config.h"

#include "broadway-env.h"

#include <glib.h>
#include <string.h>

/* Legacy fork env vars were named BROADWAY_*; they're now BROTWAY_*. Promote any
 * BROADWAY_* still in the environment to its BROTWAY_* name once at startup -
 * warn, copy the value if the new name isn't already set, and drop the old one -
 * so every read site downstream deals only with BROTWAY_*. Called from the client
 * (display open) and the daemon (main). Self-contained (glib only) so linking it
 * into either target drags nothing else in. */
void
broadway_migrate_legacy_env (void)
{
  static gboolean done = FALSE;
  char **env, **e;

  if (done)
    return;
  done = TRUE;

  env = g_get_environ ();
  for (e = env; *e != NULL; e++)
    {
      char *eq, *newname;

      if (!g_str_has_prefix (*e, "BROADWAY_"))
        continue;
      eq = strchr (*e, '=');
      if (eq == NULL)
        continue;
      *eq = '\0';   /* split NAME\0VALUE in our private copy */

      newname = g_strconcat ("BROTWAY_", *e + strlen ("BROADWAY_"), NULL);
      if (g_getenv (newname) == NULL)
        g_setenv (newname, eq + 1, TRUE);
      g_warning ("%s is deprecated; use %s", *e, newname);
      g_unsetenv (*e);
      g_free (newname);
    }
  g_strfreev (env);
}

gboolean
broadway_env_flag (const char *name,
                   gboolean    fallback)
{
  const char *v = g_getenv (name);

  if (v == NULL || *v == '\0')
    return fallback;

  if (!g_ascii_strcasecmp (v, "1") || !g_ascii_strcasecmp (v, "true") ||
      !g_ascii_strcasecmp (v, "yes") || !g_ascii_strcasecmp (v, "on"))
    return TRUE;
  if (!g_ascii_strcasecmp (v, "0") || !g_ascii_strcasecmp (v, "false") ||
      !g_ascii_strcasecmp (v, "no") || !g_ascii_strcasecmp (v, "off"))
    return FALSE;

  return fallback;
}
