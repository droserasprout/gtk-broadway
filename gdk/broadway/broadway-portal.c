#include "config.h"

#include "broadway-portal.h"

#include <gio/gio.h>

/* Minimal org.freedesktop.portal.Settings implementation (prototype). We own
 * org.freedesktop.portal.Desktop on the session bus and answer Read/ReadOne/
 * ReadAll for org.freedesktop.appearance color-scheme, plus emit SettingChanged
 * when the browser's prefers-color-scheme changes. libadwaita's AdwSettings
 * reads exactly this, so a containerized Adw app tracks the browser live.
 *
 * Only color-scheme is served; other namespaces return the standard NotFound. */

#define PORTAL_BUS_NAME   "org.freedesktop.portal.Desktop"
#define PORTAL_OBJECT     "/org/freedesktop/portal/desktop"
#define SETTINGS_IFACE    "org.freedesktop.portal.Settings"
#define APPEARANCE_NS     "org.freedesktop.appearance"
#define COLOR_SCHEME_KEY  "color-scheme"

static const char introspection_xml[] =
  "<node>"
  "  <interface name='org.freedesktop.portal.Settings'>"
  "    <method name='ReadAll'>"
  "      <arg type='as' name='namespaces' direction='in'/>"
  "      <arg type='a{sa{sv}}' name='value' direction='out'/>"
  "    </method>"
  "    <method name='Read'>"
  "      <arg type='s' name='namespace' direction='in'/>"
  "      <arg type='s' name='key' direction='in'/>"
  "      <arg type='v' name='value' direction='out'/>"
  "    </method>"
  "    <method name='ReadOne'>"
  "      <arg type='s' name='namespace' direction='in'/>"
  "      <arg type='s' name='key' direction='in'/>"
  "      <arg type='v' name='value' direction='out'/>"
  "    </method>"
  "    <signal name='SettingChanged'>"
  "      <arg type='s' name='namespace'/>"
  "      <arg type='s' name='key'/>"
  "      <arg type='v' name='value'/>"
  "    </signal>"
  "    <property name='version' type='u' access='read'/>"
  "  </interface>"
  "</node>";

static GDBusConnection    *portal_conn;
static GDBusNodeInfo      *portal_node;
static guint               portal_owner_id;
static guint32             current_scheme;  /* 0 no-pref, 1 dark, 2 light */
static gboolean            scheme_forced;   /* BROTWAY_COLOR_SCHEME pins it, ignore browser */

static gboolean
is_color_scheme (const char *ns, const char *key)
{
  return g_strcmp0 (ns, APPEARANCE_NS) == 0 &&
         g_strcmp0 (key, COLOR_SCHEME_KEY) == 0;
}

/* { "org.freedesktop.appearance": { "color-scheme": <uNN> } } */
static GVariant *
build_read_all (void)
{
  GVariantBuilder ns, keys;

  g_variant_builder_init (&keys, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add (&keys, "{sv}", COLOR_SCHEME_KEY,
                         g_variant_new_uint32 (current_scheme));

  g_variant_builder_init (&ns, G_VARIANT_TYPE ("a{sa{sv}}"));
  g_variant_builder_add (&ns, "{sa{sv}}", APPEARANCE_NS, &keys);

  return g_variant_new ("(a{sa{sv}})", &ns);
}

static void
handle_method (GDBusConnection       *conn,
               const char            *sender,
               const char            *object,
               const char            *iface,
               const char            *method,
               GVariant              *params,
               GDBusMethodInvocation *inv,
               gpointer               user_data)
{
  if (g_strcmp0 (method, "ReadAll") == 0)
    {
      g_dbus_method_invocation_return_value (inv, build_read_all ());
      return;
    }

  if (g_strcmp0 (method, "Read") == 0 || g_strcmp0 (method, "ReadOne") == 0)
    {
      const char *ns, *key;

      g_variant_get (params, "(&s&s)", &ns, &key);
      if (!is_color_scheme (ns, key))
        {
          g_dbus_method_invocation_return_error (inv, G_DBUS_ERROR,
                                                 G_DBUS_ERROR_UNKNOWN_PROPERTY,
                                                 "Unknown setting %s %s", ns, key);
          return;
        }

      /* ReadOne (Settings v2) returns the value singly wrapped; the legacy Read
       * double-wraps it (a historical quirk libadwaita still unwraps). */
      if (g_strcmp0 (method, "ReadOne") == 0)
        g_dbus_method_invocation_return_value (
          inv, g_variant_new ("(v)", g_variant_new_uint32 (current_scheme)));
      else
        g_dbus_method_invocation_return_value (
          inv, g_variant_new ("(v)", g_variant_new_variant (
                                       g_variant_new_uint32 (current_scheme))));
      return;
    }

  g_dbus_method_invocation_return_error (inv, G_DBUS_ERROR,
                                         G_DBUS_ERROR_UNKNOWN_METHOD,
                                         "Unknown method %s", method);
}

static GVariant *
handle_get_property (GDBusConnection *conn,
                     const char      *sender,
                     const char      *object,
                     const char      *iface,
                     const char      *property,
                     GError         **error,
                     gpointer         user_data)
{
  if (g_strcmp0 (property, "version") == 0)
    return g_variant_new_uint32 (2);  /* advertise ReadOne */

  g_set_error (error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
               "Unknown property %s", property);
  return NULL;
}

static const GDBusInterfaceVTable vtable = {
  handle_method, handle_get_property, NULL, { 0 }
};

static void
on_bus_acquired (GDBusConnection *conn, const char *name, gpointer user_data)
{
  GError *error = NULL;

  /* Register the object now; it only becomes reachable once we own the name. */
  g_dbus_connection_register_object (conn, PORTAL_OBJECT,
                                     portal_node->interfaces[0],
                                     &vtable, NULL, NULL, &error);
  if (error)
    {
      g_warning ("broadway portal: register failed: %s", error->message);
      g_error_free (error);
    }
}

static void
on_name_acquired (GDBusConnection *conn, const char *name, gpointer user_data)
{
  /* Only now are we the portal: serve reads and emit SettingChanged. */
  portal_conn = conn;
}

static void
on_name_lost (GDBusConnection *conn, const char *name, gpointer user_data)
{
  /* A real portal already owns the name (desktop session), or no bus. Stand
   * down - we don't fight the host portal. */
  portal_conn = NULL;
  g_message ("broadway portal: %s unavailable; not serving color-scheme", name);
}

void
broadway_portal_start (void)
{
  const char *forced;

  if (portal_owner_id != 0)
    return;

  /* BROTWAY_COLOR_SCHEME pins the scheme for every portal reader (incl.
   * libadwaita) and ignores the browser; auto/unset follows the browser. */
  forced = g_getenv ("BROTWAY_COLOR_SCHEME");
  if (forced != NULL && *forced != '\0' && g_ascii_strcasecmp (forced, "auto") != 0)
    {
      if (g_ascii_strcasecmp (forced, "dark") == 0)
        { current_scheme = 1; scheme_forced = TRUE; }
      else if (g_ascii_strcasecmp (forced, "light") == 0)
        { current_scheme = 2; scheme_forced = TRUE; }
      else
        g_warning ("BROTWAY_COLOR_SCHEME: unknown value '%s' (want auto/light/dark)",
                   forced);
    }

  portal_node = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  if (portal_node == NULL)
    return;  /* static XML is valid; guards a future edit from a NULL interfaces[0] deref */

  portal_owner_id = g_bus_own_name (G_BUS_TYPE_SESSION, PORTAL_BUS_NAME,
                                    G_BUS_NAME_OWNER_FLAGS_NONE,
                                    on_bus_acquired, on_name_acquired, on_name_lost,
                                    NULL, NULL);
}

void
broadway_portal_set_color_scheme (guint32 scheme)
{
  if (scheme_forced)
    return;
  if (scheme > 2)
    scheme = 0;
  if (scheme == current_scheme)
    return;

  current_scheme = scheme;

  if (portal_conn != NULL)
    g_dbus_connection_emit_signal (
      portal_conn, NULL, PORTAL_OBJECT, SETTINGS_IFACE, "SettingChanged",
      g_variant_new ("(ssv)", APPEARANCE_NS, COLOR_SCHEME_KEY,
                     g_variant_new_uint32 (current_scheme)),
      NULL);
}
