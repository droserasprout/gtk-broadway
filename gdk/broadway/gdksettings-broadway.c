/* GDK - The GIMP Drawing Kit
 *
 * Read the browser's prefers-color-scheme from broadwayd's Settings portal and
 * expose it as GtkSettings:gtk-interface-color-scheme, so plain GTK apps follow
 * the browser theme too (libadwaita reads the same portal on its own). broadwayd
 * owns org.freedesktop.portal.Desktop and serves org.freedesktop.appearance
 * color-scheme; here we are the client. Only color-scheme is served.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 */

#include "config.h"

#include "gdkprivate-broadway.h"
#include "gdkdisplay-broadway.h"

#include "gdk/gdkprivate.h"
#include "gdk/gdkdisplayprivate.h"

#include <gio/gio.h>
#include <string.h>

#define PORTAL_BUS_NAME         "org.freedesktop.portal.Desktop"
#define PORTAL_OBJECT_PATH      "/org/freedesktop/portal/desktop"
#define PORTAL_SETTINGS_IFACE   "org.freedesktop.portal.Settings"
#define APPEARANCE_NS           "org.freedesktop.appearance"
#define COLOR_SCHEME_KEY        "color-scheme"
#define COLOR_SCHEME_SETTING    "gtk-interface-color-scheme"

/* Portal appearance color-scheme (0 no-pref, 1 dark, 2 light) maps onto the
 * GtkInterfaceColorScheme enum by +1 (0 UNSUPPORTED, 1 DEFAULT, 2 DARK, 3 LIGHT),
 * exactly like the wayland backend. */
static void
apply_color_scheme (GdkDisplay *display,
                    GVariant   *value)
{
  GdkBroadwayDisplay *self = GDK_BROADWAY_DISPLAY (display);

  self->color_scheme = (int) (g_variant_get_uint32 (value) + 1);
}

static void
settings_portal_changed (GDBusProxy *proxy,
                         const char *sender_name,
                         const char *signal_name,
                         GVariant   *parameters,
                         gpointer    user_data)
{
  GdkDisplay *display = user_data;
  const char *namespace;
  const char *name;
  GVariant *value;

  if (strcmp (signal_name, "SettingChanged") != 0)
    return;

  g_variant_get (parameters, "(&s&sv)", &namespace, &name, &value);

  if (g_str_equal (namespace, APPEARANCE_NS) && g_str_equal (name, COLOR_SCHEME_KEY))
    {
      apply_color_scheme (display, value);
      gdk_display_setting_changed (display, COLOR_SCHEME_SETTING);
    }

  g_variant_unref (value);
}

void
_gdk_broadway_display_init_settings (GdkDisplay *display)
{
  GdkBroadwayDisplay *self = GDK_BROADWAY_DISPLAY (display);
  GDBusProxy *proxy;
  GVariant *ret;
  GVariant *value = NULL;
  GError *error = NULL;
  const char *forced;

  self->color_scheme = 0; /* GTK_INTERFACE_COLOR_SCHEME_UNSUPPORTED */

  /* BROTWAY_COLOR_SCHEME forces the scheme for this process and ignores the
   * browser: dark/light pin it; auto (or unset) follows the portal as usual. */
  forced = g_getenv ("BROTWAY_COLOR_SCHEME");
  if (forced != NULL && *forced != '\0' && g_ascii_strcasecmp (forced, "auto") != 0)
    {
      if (g_ascii_strcasecmp (forced, "dark") == 0)
        {
          self->color_scheme = 2;  /* GTK_INTERFACE_COLOR_SCHEME_DARK */
          self->color_scheme_forced = TRUE;
        }
      else if (g_ascii_strcasecmp (forced, "light") == 0)
        {
          self->color_scheme = 3;  /* GTK_INTERFACE_COLOR_SCHEME_LIGHT */
          self->color_scheme_forced = TRUE;
        }
      else
        g_warning ("BROTWAY_COLOR_SCHEME: unknown value '%s' (want auto/light/dark)",
                   forced);

      if (self->color_scheme_forced)
        return;  /* pinned: no portal proxy, browser preference ignored */
      /* unknown value: fall through and follow the portal */
    }

  if (gdk_display_get_debug_flags (display) &
      (GDK_DEBUG_DEFAULT_SETTINGS | GDK_DEBUG_NO_PORTALS))
    return;

  /* No should_use_portal() gate: broadwayd always serves this when present, and
   * the proxy quietly follows if it appears later. If there is no session bus or
   * portal at all, the ReadOne below fails and we fall back to defaults. */
  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         PORTAL_BUS_NAME,
                                         PORTAL_OBJECT_PATH,
                                         PORTAL_SETTINGS_IFACE,
                                         NULL,
                                         &error);
  if (proxy == NULL)
    {
      g_debug ("broadway: no Settings portal (%s); not following color-scheme",
               error->message);
      g_clear_error (&error);
      return;
    }

  /* Prime the current value. A failure here (portal not up yet, or a portal that
   * lacks color-scheme) is fine: keep the proxy subscribed so a later
   * SettingChanged still lands. */
  ret = g_dbus_proxy_call_sync (proxy, "ReadOne",
                                g_variant_new ("(ss)", APPEARANCE_NS, COLOR_SCHEME_KEY),
                                G_DBUS_CALL_FLAGS_NONE, G_MAXINT, NULL, &error);
  if (ret != NULL)
    {
      g_variant_get (ret, "(v)", &value);
      apply_color_scheme (display, value);
      g_variant_unref (value);
      g_variant_unref (ret);
    }
  else
    {
      g_debug ("broadway: initial color-scheme read failed (%s); waiting for changes",
               error->message);
      g_clear_error (&error);
    }

  g_signal_connect (proxy, "g-signal",
                    G_CALLBACK (settings_portal_changed), display);

  self->settings_portal = proxy;
}

void
_gdk_broadway_display_finalize_settings (GdkDisplay *display)
{
  GdkBroadwayDisplay *self = GDK_BROADWAY_DISPLAY (display);

  g_clear_object (&self->settings_portal);
}

gboolean
_gdk_broadway_display_get_setting (GdkDisplay *display,
                                   const char *name,
                                   GValue     *value)
{
  GdkBroadwayDisplay *self = GDK_BROADWAY_DISPLAY (display);

  if (self->settings_portal == NULL && !self->color_scheme_forced)
    return FALSE;

  if (!g_str_equal (name, COLOR_SCHEME_SETTING))
    return FALSE;

  g_value_set_enum (value, self->color_scheme);
  return TRUE;
}
