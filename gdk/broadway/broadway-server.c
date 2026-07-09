#include "config.h"

#include "broadway-server.h"

#include "broadway-output.h"

#include <glib.h>
#include <glib/gprintf.h>
#include "gdktypes.h"
#include "gdkdeviceprivate.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#elif defined (G_OS_WIN32)
#include <io.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#ifdef G_OS_UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif
#ifdef HAVE_GIO_UNIX
#include <gio/gunixsocketaddress.h>
#endif
#ifdef G_OS_WIN32
#include <windows.h>
#include <string.h>
#endif

typedef struct {
  int id;
  guint32 tag;
} BroadwayOutstandingRoundtrip;

typedef struct BroadwayInput BroadwayInput;
typedef struct BroadwaySurface BroadwaySurface;
struct _BroadwayServer {
  GObject parent_instance;

  char *address;
  int port;
  char *display;    /* ":N" passed to the debug-menu client we spawn */
  char *ssl_cert;
  char *ssl_key;
  GSocketService *service;
  BroadwayOutput *output;
  guint32 id_counter;
  guint32 saved_serial;
  guint64 last_seen_time;
  BroadwayInput *input;
  GList *input_messages;
  guint process_input_idle;

  GHashTable *surface_id_hash;
  GList *surfaces;
  BroadwaySurface *root;
  gint32 focused_surface_id; /* -1 => none */
  guint32 menu_owner; /* GTK client id of the spawned debug menu; all its
                       * toplevels (menu, gallery, dialogs) stay pinned above
                       * other surfaces, newest on top (0 => none) */
  gboolean expecting_menu_surface; /* tag the next toplevel as the debug menu */
  guint32 last_gtk_client_id;   /* newest daemon (unix socket) client id seen */
  guint32 menu_client_floor;    /* only clients newer than this at summon time
                                 * can be the spawned menu */
  int show_keyboard;

  /* Debug-menu stats. session_id identifies this daemon run; the byte/frame
   * totals are cumulative across browser reconnects (the live output's own
   * counters are added in on read, and folded in here when an output is freed). */
  guint32 session_id;
  guint64 session_bytes;
  guint32 session_frames;
  guint32 last_latency_ms;  /* client-measured ping round-trip, reported on EVENT_PING */
  gboolean paint_flash;     /* debug-menu: flash redrawn nodes pink in the browser */
  int pinned_w, pinned_h, pinned_scale; /* debug-menu: forced screen size/scale (0 = unpinned) */

  guint32 next_texture_id;
  GHashTable *textures;
  guint64 texture_uploads;  /* cumulative uploads = client content-cache misses */
  guint64 texture_releases; /* cumulative texture frees (evicted from the browser) */

  /* Random per-daemon id sent on connect; lets a client tell a live session
   * apart from a restarted daemon. */
  guint32 session_token;

  /* Single-display arbitration: owner_id holds the display, next_client_id
   * mints fresh ids. */
  guint32 owner_id;
  guint32 next_client_id;

  guint32 screen_scale;

  gint32 mouse_in_surface_id;
  int last_x, last_y; /* in root coords */
  guint32 last_state;
  gint32 real_mouse_in_surface_id; /* Not affected by grabs */

  /* Pointer grabs stack so a popup chain (menu -> submenu) nests: a submenu
   * pushes over its parent instead of clobbering it, and pops back on close.
   * Head = innermost. Push/pop are strictly LIFO (UNGRAB carries no surface id,
   * so both ends just drop the innermost) - fine for popup chains, the only
   * producers here. These scalars cache the top so read-sites stay plain;
   * owner_events/time are read off the head directly, so they're not cached. */
  GList *pointer_grabs;
  gint32 pointer_grab_surface_id; /* -1 => none (== top's surface) */
  gint32 pointer_grab_client_id; /* -1 => none */

  /* Active touch sequences: sequence id -> client the BEGIN was routed to,
   * so the rest of the sequence follows even across grab changes. */
  GHashTable *touch_sequences;

  /* Future data, from the currently queued events */
  int future_root_x;
  int future_root_y;
  guint32 future_state;
  int future_mouse_in_surface;

  GList *outstanding_roundtrips;

  GSList *deferred_enters; /* pending DeferredEnter timers (see below) */
};

struct _BroadwayServerClass
{
  GObjectClass parent_class;
};

typedef struct HttpRequest {
  BroadwayServer *server;
  GSocketConnection *socket_connection;
  GIOStream *connection;
  GDataInputStream *data;
  GString *request;
}  HttpRequest;

struct BroadwayInput {
  BroadwayServer *server;
  BroadwayOutput *output;
  GIOStream *connection;
  GByteArray *buffer;
  GSource *source;
  gboolean seen_time;
  gint64 time_base;
  gboolean active;
  guint32 client_id;   /* from the ?cid= query; 0 == fresh page load */
};

struct BroadwaySurface {
  guint32 owner;
  gint32 id;
  gint32 x;
  gint32 y;
  gint32 width;
  gint32 height;
  gboolean visible;
  gint32 transient_for;
  gboolean is_popup; /* menu/popover/bubble, as opposed to a toplevel or dialog */
  gboolean keep_above; /* always-on-top: pinned above normal surfaces, and exempt
                        * from grab confinement so it stays interactive */
  guint32 texture;
  gboolean modal_hint;
  gboolean input_region_is_empty;  /* mode == 1; kept for any_popup_visible() */
  int input_region_mode;           /* 0 = whole, 1 = empty, 2 = rect */
  BroadwayRect input_region_rect;  /* interactive area when mode == 2 */
  BroadwayNode *nodes;
  GHashTable *node_lookup;
  char *cursor_name;               /* last CSS cursor sent; replayed on resync */
  char *title;                     /* last window title sent; replayed on resync */
  GBytes *icon;                    /* last icon PNG sent; replayed on resync */
};

struct _BroadwayTexture {
  grefcount refcount;
  guint32 id;
  GBytes *bytes;
};

/* A pending pointer-recovery ENTER, scheduled when a popup closes (see
 * recover_pointer_focus). Tracked on server->deferred_enters so the timer can
 * be cancelled on finalize and coalesced per toplevel. */
typedef struct {
  BroadwayServer *server;
  gint32 parent_id;
  guint source_id;
} DeferredEnter;

static void broadway_server_resync_surfaces (BroadwayServer *server);
static void send_outstanding_roundtrips (BroadwayServer *server);

static void broadway_server_ref_texture (BroadwayServer   *server,
                                         guint32           id);

static GType broadway_server_get_type (void);

G_DEFINE_TYPE (BroadwayServer, broadway_server, G_TYPE_OBJECT)

static void
broadway_texture_free (BroadwayTexture *texture)
{
  g_bytes_unref (texture->bytes);
  g_free (texture);
}

static void
broadway_node_unref (BroadwayServer *server,
                     BroadwayNode *node)
{
  int i;

  if (g_ref_count_dec (&node->refcount))
    {
      for (i = 0; i < node->n_children; i++)
        broadway_node_unref (server, node->children[i]);

      if (node->texture_id)
        broadway_server_release_texture (server, node->texture_id);

      g_free (node);
    }
}

static BroadwayNode *
broadway_node_ref (BroadwayNode *node)
{
  g_ref_count_inc (&node->refcount);

  return node;
}

gboolean
broadway_node_equal (BroadwayNode     *a,
                     BroadwayNode     *b)
{
  int i;

  if (a->type != b->type)
    return FALSE;

  if (a->n_data != b->n_data)
    return FALSE;

  /* Don't check data for containers, that is just n_children, which
     we don't want to compare for a shallow equal */
  if (a->type != BROADWAY_NODE_CONTAINER)
    {
      for (i = 0; i < a->n_data; i++)
        if (a->data[i] != b->data[i])
          return FALSE;
    }

  return TRUE;
}

gboolean
broadway_node_deep_equal (BroadwayNode     *a,
                          BroadwayNode     *b)
{
  int i;

  if (a->hash != b->hash)
    return FALSE;

  if (!broadway_node_equal (a,b))
    return FALSE;

  if (a->n_children != b->n_children)
    return FALSE;

  for (i = 0; i < a->n_children; i++)
    if (!broadway_node_deep_equal (a->children[i], b->children[i]))
      return FALSE;

  return TRUE;
}


void
broadway_node_mark_deep_reused (BroadwayNode    *node,
                                gboolean         reused)
{
  node->reused = reused;
  for (int i = 0; i < node->n_children; i++)
    broadway_node_mark_deep_reused (node->children[i], reused);
}

void
broadway_node_mark_deep_consumed (BroadwayNode    *node,
                                  gboolean         consumed)
{
  node->consumed = consumed;
  for (int i = 0; i < node->n_children; i++)
    broadway_node_mark_deep_consumed (node->children[i], consumed);
}

void
broadway_node_add_to_lookup (BroadwayNode    *node,
                             GHashTable      *node_lookup)
{
  g_hash_table_insert (node_lookup, GINT_TO_POINTER(node->id), node);
  for (int i = 0; i < node->n_children; i++)
    broadway_node_add_to_lookup (node->children[i], node_lookup);
}

static void
broadway_server_init (BroadwayServer *server)
{
  BroadwaySurface *root;

  server->service = g_socket_service_new ();
  server->pointer_grab_surface_id = -1;
  server->menu_owner = 0;
  do
    server->session_id = g_random_int ();
  while (server->session_id == 0);
  server->saved_serial = 1;
  server->last_seen_time = 1;
  server->surface_id_hash = g_hash_table_new (NULL, NULL);
  server->id_counter = 0;
  /* Non-zero so the client can treat 0 as "no token seen yet". */
  do
    server->session_token = g_random_int ();
  while (server->session_token == 0);
  server->textures = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL,
                                            (GDestroyNotify)broadway_texture_free);
  server->touch_sequences = g_hash_table_new (NULL, NULL);

  root = g_new0 (BroadwaySurface, 1);
  root->id = server->id_counter++;
  root->width = 1024;
  root->height = 768;
  root->visible = TRUE;

  server->root = root;
  server->screen_scale = 1;

  g_hash_table_insert (server->surface_id_hash,
                       GINT_TO_POINTER (root->id),
                       root);
}

static void menu_control_teardown (void);

static void
broadway_server_finalize (GObject *object)
{
  BroadwayServer *server = BROADWAY_SERVER (object);
  GSList *l;

  /* Cancel any pending pointer-recovery timers so they can't fire on a freed
   * server. */
  for (l = server->deferred_enters; l != NULL; l = l->next)
    {
      DeferredEnter *de = l->data;
      g_source_remove (de->source_id);
      g_free (de);
    }
  g_slist_free (server->deferred_enters);

  g_list_free_full (server->pointer_grabs, g_free);

  /* The menu control sources and child watch hold the server raw. */
  menu_control_teardown ();

  g_free (server->address);
  g_free (server->display);
  g_free (server->ssl_cert);
  g_free (server->ssl_key);
  g_hash_table_destroy (server->textures);
  g_hash_table_destroy (server->touch_sequences);

  G_OBJECT_CLASS (broadway_server_parent_class)->finalize (object);
}

static void
broadway_server_class_init (BroadwayServerClass * class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = broadway_server_finalize;
}

static void
broadway_surface_free (BroadwayServer *server,
                       BroadwaySurface *surface)
{
  if (surface->nodes)
    broadway_node_unref (server, surface->nodes);
  g_hash_table_unref (surface->node_lookup);
  g_free (surface->cursor_name);
  g_free (surface->title);
  g_clear_pointer (&surface->icon, g_bytes_unref);
  g_free (surface);
}

static BroadwaySurface *
broadway_server_lookup_surface (BroadwayServer   *server,
                                guint32           id)
{
  return g_hash_table_lookup (server->surface_id_hash,
                              GINT_TO_POINTER (id));
}

static gboolean start (BroadwayInput *input);

static void
http_request_free (HttpRequest *request)
{
  g_object_unref (request->socket_connection);
  g_object_unref (request->connection);
  g_object_unref (request->data);
  g_string_free (request->request, TRUE);
  g_free (request);
}

static void
broadway_input_free (BroadwayInput *input)
{
  g_object_unref (input->connection);
  g_byte_array_free (input->buffer, FALSE);
  g_source_destroy (input->source);
  g_free (input);
}

static gboolean surface_is_above (BroadwayServer *server, BroadwaySurface *s);

static void
update_event_state (BroadwayServer *server,
                    BroadwayInputMsg *message)
{
  BroadwaySurface *surface;

  switch (message->base.type) {
  case BROADWAY_EVENT_ENTER:
    server->last_x = message->pointer.root_x;
    server->last_y = message->pointer.root_y;
    server->last_state = message->pointer.state;
    server->real_mouse_in_surface_id = message->pointer.mouse_surface_id;

    /* TODO: Unset when it dies */
    server->mouse_in_surface_id = message->pointer.event_surface_id;
    break;
  case BROADWAY_EVENT_LEAVE:
    server->last_x = message->pointer.root_x;
    server->last_y = message->pointer.root_y;
    server->last_state = message->pointer.state;
    server->real_mouse_in_surface_id = message->pointer.mouse_surface_id;

    server->mouse_in_surface_id = 0;
    break;
  case BROADWAY_EVENT_POINTER_MOVE:
    server->last_x = message->pointer.root_x;
    server->last_y = message->pointer.root_y;
    server->last_state = message->pointer.state;
    server->real_mouse_in_surface_id = message->pointer.mouse_surface_id;
    break;
  case BROADWAY_EVENT_BUTTON_PRESS:
  case BROADWAY_EVENT_BUTTON_RELEASE:
    if (message->base.type == BROADWAY_EVENT_BUTTON_PRESS &&
        server->focused_surface_id != message->pointer.mouse_surface_id)
      {
        if (server->pointer_grab_surface_id == -1)
          {
            broadway_server_surface_raise (server, message->pointer.mouse_surface_id);
            broadway_server_focus_surface (server, message->pointer.mouse_surface_id);
            broadway_server_flush (server);
          }
        else if (surface_is_above (server,
                   broadway_server_lookup_surface (server, message->pointer.mouse_surface_id)))
          {
            /* Keyboard half of the keep-above carve-out: a click on an always-on-top
             * window during a grab is delivered to it, so focus it too - else keys go
             * to the grabbed surface. No raise (already pinned; don't reorder under a grab). */
            broadway_server_focus_surface (server, message->pointer.mouse_surface_id);
            broadway_server_flush (server);
          }
      }

    server->last_x = message->pointer.root_x;
    server->last_y = message->pointer.root_y;
    server->last_state = message->pointer.state;
    server->real_mouse_in_surface_id = message->pointer.mouse_surface_id;
    break;
  case BROADWAY_EVENT_SCROLL:
    server->last_x = message->pointer.root_x;
    server->last_y = message->pointer.root_y;
    server->last_state = message->pointer.state;
    server->real_mouse_in_surface_id = message->pointer.mouse_surface_id;
    break;
  case BROADWAY_EVENT_TOUCH:
    /* Like BUTTON_PRESS above: while a pointer grab (popup) is live, a tap
     * must not raise/refocus past it and churn the grab down. */
    if (message->touch.touch_type == 0 && message->touch.is_emulated &&
        server->focused_surface_id != message->touch.event_surface_id &&
        server->pointer_grab_surface_id == -1)
      {
        BroadwaySurface *touched =
          broadway_server_lookup_surface (server, message->touch.event_surface_id);

        /* Tapping a popup (menu, popover, or the touch selection bubble) must
         * not move keyboard focus to it. Focusing the popup makes the
         * toplevel's focused widget emit focus-out, whose handler tears the
         * popup down (e.g. GtkText hides its selection bubble) before the tap
         * can activate the popup's content, so the bubble's Cut/Copy/Paste
         * silently do nothing. Real backends (Wayland xdg_popup) don't take
         * keyboard focus on click either. So only raise + focus genuine
         * toplevels (including dialogs) here, never popups. */
        if (touched && !touched->is_popup)
          {
            broadway_server_surface_raise (server, message->touch.event_surface_id);
            broadway_server_focus_surface (server, message->touch.event_surface_id);
            broadway_server_flush (server);
          }
      }

    if (message->touch.is_emulated)
      {
        /* touch and pointer structs have different layouts; the touch root
         * coords live in message->touch, not message->pointer (which would
         * alias sequence_id/is_emulated here). */
        server->last_x = message->touch.root_x;
        server->last_y = message->touch.root_y;
      }

    server->last_state = message->touch.state;
    break;
  case BROADWAY_EVENT_KEY_PRESS:
  case BROADWAY_EVENT_KEY_RELEASE:
    server->last_state = message->key.state;
    break;
  case BROADWAY_EVENT_GRAB_NOTIFY:
  case BROADWAY_EVENT_UNGRAB_NOTIFY:
    break;
  case BROADWAY_EVENT_CONFIGURE_NOTIFY:
    surface = broadway_server_lookup_surface (server, message->configure_notify.id);
    if (surface != NULL)
      {
        surface->x = message->configure_notify.x;
        surface->y = message->configure_notify.y;

	if (server->focused_surface_id != message->configure_notify.id &&
	    server->pointer_grab_surface_id == -1 && surface->modal_hint)
	{
	  broadway_server_surface_raise (server, message->configure_notify.id);
	  broadway_server_focus_surface (server, message->configure_notify.id);
	  broadway_server_flush (server);
	}
      }
    break;
  case BROADWAY_EVENT_ROUNDTRIP_NOTIFY:
    break;
  case BROADWAY_EVENT_SUSPEND:
  case BROADWAY_EVENT_RESUME:
    /* No server-side state; forwarded to GTK to freeze/thaw rendering. */
    break;
  case BROADWAY_EVENT_SCREEN_SIZE_CHANGED:
    server->root->width = message->screen_resize_notify.width;
    server->root->height = message->screen_resize_notify.height;
    server->screen_scale = message->screen_resize_notify.scale;
    break;

  default:
    g_printerr ("update_event_state - Unknown input command %c\n", message->base.type);
    break;
  }
}

gboolean
broadway_server_lookahead_event (BroadwayServer  *server,
                                 const char      *types)
{
  BroadwayInputMsg *message;
  GList *l;

  for (l = server->input_messages; l != NULL; l = l->next)
    {
      message = l->data;
      if (strchr (types, message->base.type) != NULL)
        return TRUE;
    }

  return FALSE;
}

static gboolean
is_pointer_event (BroadwayInputMsg *message)
{
  return
    message->base.type == BROADWAY_EVENT_ENTER ||
    message->base.type == BROADWAY_EVENT_LEAVE ||
    message->base.type == BROADWAY_EVENT_POINTER_MOVE ||
    message->base.type == BROADWAY_EVENT_BUTTON_PRESS ||
    message->base.type == BROADWAY_EVENT_BUTTON_RELEASE ||
    message->base.type == BROADWAY_EVENT_SCROLL ||
    message->base.type == BROADWAY_EVENT_GRAB_NOTIFY ||
    message->base.type == BROADWAY_EVENT_UNGRAB_NOTIFY;
}

static void
process_input_message (BroadwayServer *server,
                       BroadwayInputMsg *message)
{
  gint32 client;
  BroadwaySurface *surface;

  update_event_state (server, message);


  switch (message->base.type) {
  case BROADWAY_EVENT_ENTER:
  case BROADWAY_EVENT_LEAVE:
  case BROADWAY_EVENT_POINTER_MOVE:
  case BROADWAY_EVENT_BUTTON_PRESS:
  case BROADWAY_EVENT_BUTTON_RELEASE:
  case BROADWAY_EVENT_SCROLL:
  case BROADWAY_EVENT_GRAB_NOTIFY:
  case BROADWAY_EVENT_UNGRAB_NOTIFY:
    surface = broadway_server_lookup_surface (server, message->pointer.event_surface_id);
    break;
  case BROADWAY_EVENT_TOUCH:
    surface = broadway_server_lookup_surface (server, message->touch.event_surface_id);
    break;
  case BROADWAY_EVENT_CONFIGURE_NOTIFY:
    surface = broadway_server_lookup_surface (server, message->configure_notify.id);
    break;
  case BROADWAY_EVENT_ROUNDTRIP_NOTIFY:
    surface = broadway_server_lookup_surface (server, message->roundtrip_notify.id);
    break;
  case BROADWAY_EVENT_KEY_PRESS:
  case BROADWAY_EVENT_KEY_RELEASE:
    /* TODO: Send to keys focused clients only... */
  case BROADWAY_EVENT_FOCUS:
  case BROADWAY_EVENT_SCREEN_SIZE_CHANGED:
  case BROADWAY_EVENT_SUSPEND:
  case BROADWAY_EVENT_RESUME:
  default:
    surface = NULL;
    break;
  }

  if (surface)
    client = surface->owner;
  else
    client = -1;

  if (message->base.type == BROADWAY_EVENT_TOUCH)
    {
      /* Touch follows the pointer grab too, but routed per sequence: the
       * client picked at BEGIN keeps the rest of the sequence, so UPDATE/END
       * aren't split across clients when a grab starts or ends mid-touch. */
      gpointer key = GUINT_TO_POINTER (message->touch.sequence_id);
      gpointer val;

      if (message->touch.touch_type != 0 &&
          g_hash_table_lookup_extended (server->touch_sequences, key, NULL, &val))
        client = GPOINTER_TO_INT (val);
      else if (server->pointer_grab_surface_id != -1 &&
               !surface_is_above (server, surface)) /* carve-out, see below */
        client = server->pointer_grab_client_id;

      if (message->touch.touch_type == 0) /* begin */
        g_hash_table_replace (server->touch_sequences, key, GINT_TO_POINTER (client));
      else if (message->touch.touch_type == 2 || message->touch.touch_type == 3) /* end/cancel */
        g_hash_table_remove (server->touch_sequences, key);
    }
  else if (is_pointer_event (message) &&
      server->pointer_grab_surface_id != -1 &&
      !surface_is_above (server, surface))
    /* Grab carve-out: events on a keep-above surface (e.g. the debug menu) stay
     * with its own client so it keeps working - draggable - while a menu grab is
     * up, and the grab client never sees the press, so the menu doesn't dismiss. */
    client = server->pointer_grab_client_id;

  broadway_events_got_input (message, client);
}

static void
process_input_messages (BroadwayServer *server)
{
  BroadwayInputMsg *message;

  while (server->input_messages)
    {
      message = server->input_messages->data;
      server->input_messages =
        g_list_delete_link (server->input_messages,
                            server->input_messages);

      if (message->base.serial == 0)
        {
          /* This was sent before we got any requests, but we don't want the
             daemon serials to go backwards, so we fix it up to be the last used
             serial */
          message->base.serial = server->saved_serial - 1;
        }

      process_input_message (server, message);
      g_free (message);
    }
}

static void
fake_configure_notify (BroadwayServer *server,
                       BroadwaySurface *surface)
{
  BroadwayInputMsg ev = { {0} };

  ev.base.type = BROADWAY_EVENT_CONFIGURE_NOTIFY;
  ev.base.serial = server->saved_serial - 1;
  ev.base.time = server->last_seen_time;
  ev.configure_notify.id = surface->id;
  ev.configure_notify.x = surface->x;
  ev.configure_notify.y = surface->y;
  ev.configure_notify.width = surface->width;
  ev.configure_notify.height = surface->height;

  process_input_message (server, &ev);
}

static gboolean
any_popup_visible (BroadwayServer *server)
{
  GList *l;
  gboolean found = FALSE;

  for (l = server->surfaces; l != NULL; l = l->next)
    {
      BroadwaySurface *s = l->data;
      if (s->visible && s->is_popup)
        {
          /* Click-through popups (empty input region: GtkTextHandle cursor /
           * selection handles) are not interactive menus, so they don't mean
           * "navigation in progress" — ignore them. */
          if (!s->input_region_is_empty)
            found = TRUE;
        }
    }
  return found;
}

/* Fired ~50 ms after a popup went away. The delay matters: the app processes
 * the ENTER (high-priority socket IO) before its own popdown/focus-restore
 * idle work (lower priority), so an immediate ENTER gets clobbered. Deferring
 * lets the app settle first, then re-assert pointer focus. Re-check the
 * navigation gate at fire time in case a new popup opened meanwhile. */
static gboolean
deferred_enter_cb (gpointer data)
{
  DeferredEnter *de = data;
  BroadwayServer *server = de->server;
  BroadwaySurface *parent;

  /* Drop our entry first: the source auto-removes on return, so finalize must
   * not also try to remove it. */
  server->deferred_enters = g_slist_remove (server->deferred_enters, de);

  if (any_popup_visible (server))
    goto out;

  parent = broadway_server_lookup_surface (server, de->parent_id);
  if (parent == NULL)
    goto out;

  /* Ask the browser to re-send a pointer ENTER through the normal input path.
   * Injecting the crossing daemon-side doesn't take — the browser-originated
   * event carries the live serial/time the app's GTK needs to honor the
   * crossing — so we route it through the client. */
  if (server->output)
    {
      broadway_output_reassert_pointer (server->output, de->parent_id,
                                        server->last_x, server->last_y);
      broadway_server_flush (server);
    }

out:
  g_free (de);
  return G_SOURCE_REMOVE;
}

/* A popup (menu/dropdown/popover) just went away (was hidden or destroyed).
 * Re-assert pointer focus on its toplevel so emulated-touch input keeps
 * flowing — but only if no other popup is still up: if one is, this was menu
 * navigation (close A to open B), and entering the parent would dismiss the
 * new popup. The hidden/destroyed surface is already non-visible/removed by
 * the time we get here, so it never counts itself. Deferred (see above). */
static void
recover_pointer_focus (BroadwayServer *server,
                       gint32          transient_for)
{
  DeferredEnter *de;
  GSList *l;
  gint32 top = transient_for;

  if (transient_for <= 0)
    return;

  /* Resolve to the toplevel: the immediate parent may itself be a popup. */
  for (;;)
    {
      BroadwaySurface *s = broadway_server_lookup_surface (server, top);
      if (s == NULL || !s->is_popup)
        break;
      top = s->transient_for;
    }

  /* A popdown often hides *and* destroys the surface, and nested popups can
   * close together; coalesce so we don't schedule duplicate ENTERs for the
   * same toplevel. */
  for (l = server->deferred_enters; l != NULL; l = l->next)
    {
      if (((DeferredEnter *) l->data)->parent_id == top)
        return;
    }

  de = g_new0 (DeferredEnter, 1);
  de->server = server;
  de->parent_id = top;
  server->deferred_enters = g_slist_prepend (server->deferred_enters, de);
  de->source_id = g_timeout_add (50, deferred_enter_cb, de);
}

/* One level of the pointer-grab stack (see the struct comment). */
typedef struct {
  gint32 surface_id;
  gint32 client_id;
  gboolean owner_events;
  guint32 time;
} BroadwayGrab;

/* Mirror the innermost grab into the cached scalars the read-sites use. */
static void
update_grab_cache (BroadwayServer *server)
{
  if (server->pointer_grabs)
    {
      BroadwayGrab *g = server->pointer_grabs->data;
      server->pointer_grab_surface_id = g->surface_id;
      server->pointer_grab_client_id = g->client_id;
    }
  else
    {
      server->pointer_grab_surface_id = -1;
      server->pointer_grab_client_id = -1;
    }
}

/* A surface vanished without a clean ungrab: drop its grab level and everything
 * above it. No ungrab op - the browser drops the same levels on HIDE/DESTROY. */
static void
grab_stack_drop_through (BroadwayServer *server,
                         gint32          id)
{
  gboolean found = FALSE;
  GList *l;

  for (l = server->pointer_grabs; l != NULL; l = l->next)
    if (((BroadwayGrab *) l->data)->surface_id == id)
      {
        found = TRUE;
        break;
      }
  if (!found)
    return;

  while (server->pointer_grabs)
    {
      BroadwayGrab *g = server->pointer_grabs->data;
      gint32 sid = g->surface_id;

      g_free (g);
      server->pointer_grabs = g_list_delete_link (server->pointer_grabs,
                                                  server->pointer_grabs);
      if (sid == id)
        break;
    }

  update_grab_cache (server);
}

static guint32 *
parse_pointer_data (guint32 *p, BroadwayInputPointerMsg *data)
{
  data->mouse_surface_id = ntohl (*p++);
  data->event_surface_id = ntohl (*p++);
  data->root_x = ntohl (*p++);
  data->root_y = ntohl (*p++);
  data->win_x = ntohl (*p++);
  data->win_y = ntohl (*p++);
  data->state = ntohl (*p++);

  return p;
}

static guint32 *
parse_touch_data (guint32 *p, BroadwayInputTouchMsg *data)
{
  data->touch_type = ntohl (*p++);
  data->event_surface_id = ntohl (*p++);
  data->sequence_id = ntohl (*p++);
  data->is_emulated = ntohl (*p++);
  data->root_x = ntohl (*p++);
  data->root_y = ntohl (*p++);
  data->win_x = ntohl (*p++);
  data->win_y = ntohl (*p++);
  data->state = ntohl (*p++);

  return p;
}

static void
update_future_pointer_info (BroadwayServer *server, BroadwayInputPointerMsg *data)
{
  server->future_root_x = data->root_x;
  server->future_root_y = data->root_y;
  server->future_state = data->state;
  server->future_mouse_in_surface = data->mouse_surface_id;
}

static void
queue_input_message (BroadwayServer *server, BroadwayInputMsg *msg)
{
  server->input_messages = g_list_append (server->input_messages, g_memdup2 (msg, sizeof (BroadwayInputMsg)));
}

/* ---- Debug menu (spawn-on-demand) -------------------------------------
 * On BROADWAY_EVENT_MENU from the browser we spawn a small native GTK4 client
 * (gtk4-brotway-debugmenu) pointed at our own display, so its window composites
 * into the same view every connected browser sees. A second trigger while it is
 * up closes it (toggle). */

/* Control channel to the spawned menu: a socketpair whose child end is passed
 * by fd number (BROADWAY_DEBUGMENU_FD). The daemon pushes a "stats ..." line
 * every MENU_STATS_INTERVAL_MS and reads back action commands ("drop-session"). */
#define MENU_STATS_INTERVAL_MS 500

static GPid     menu_pid = 0;
static GSocket *menu_sock = NULL;        /* daemon end of the control socketpair */
static guint    menu_stats_timer = 0;
static GSource *menu_read_source = NULL;
static gint64   menu_spawn_failed_at = 0; /* monotonic time of last failed spawn */
static guint    menu_expect_timer = 0;    /* clears expecting_menu_surface if the
                                           * spawned menu never maps a toplevel */
static guint    menu_child_watch = 0;

#define MENU_SPAWN_RETRY_US (10 * G_USEC_PER_SEC)
#define MENU_EXPECT_TIMEOUT_SECONDS 10

static void
menu_control_teardown (void)
{
  if (menu_child_watch != 0)
    {
      g_source_remove (menu_child_watch);
      menu_child_watch = 0;
    }
  if (menu_expect_timer != 0)
    {
      g_source_remove (menu_expect_timer);
      menu_expect_timer = 0;
    }
  if (menu_stats_timer != 0)
    {
      g_source_remove (menu_stats_timer);
      menu_stats_timer = 0;
    }
  if (menu_read_source != NULL)
    {
      g_source_destroy (menu_read_source);
      g_source_unref (menu_read_source);
      menu_read_source = NULL;
    }
  g_clear_object (&menu_sock);
}

/* Cumulative totals = folded-in past outputs + the live one. */
static void
menu_current_traffic (BroadwayServer *server, guint64 *bytes, guint32 *frames)
{
  *bytes = server->session_bytes;
  *frames = server->session_frames;
  if (server->output)
    {
      *bytes += broadway_output_get_bytes_sent (server->output);
      *frames += broadway_output_get_frames (server->output);
    }
}

/* Texture buffer the browser currently holds: count + summed PNG bytes across
 * all live (refcounted) textures. This is the real footprint kept warm by the
 * content-dedup cache, so the debug menu can show it. */
static void
menu_texture_buffer (BroadwayServer *server, guint *count, guint64 *bytes)
{
  GHashTableIter iter;
  gpointer value;
  guint64 total = 0;

  *count = g_hash_table_size (server->textures);
  g_hash_table_iter_init (&iter, server->textures);
  while (g_hash_table_iter_next (&iter, NULL, &value))
    total += g_bytes_get_size (((BroadwayTexture *) value)->bytes);
  *bytes = total;
}

static gboolean
menu_push_stats (gpointer user_data)
{
  BroadwayServer *server = user_data;
  static guint32 last_frames = 0;
  static guint64 last_bytes = 0;
  static guint64 last_uploads = 0;
  static guint64 last_releases = 0;
  static gint64 last_time = 0;
  guint64 bytes;
  guint32 frames;
  guint32 d_frames;
  guint32 bpf = 0;       /* bytes per frame over the window = frame heaviness */
  gint64 now;
  double fps = 0;
  double up_s = 0, rel_s = 0;   /* texture uploads (= cache misses) / releases per sec */
  guint tex_count;
  guint64 tex_bytes;
  guint32 iv_p95 = 0, iv_max = 0, w_avg = 0, w_max = 0;   /* pacing, microseconds */
  char line[256];
  int len;

  if (menu_sock == NULL)
    return G_SOURCE_REMOVE;

  menu_current_traffic (server, &bytes, &frames);

  now = g_get_monotonic_time ();
  d_frames = frames - last_frames;
  if (last_time != 0 && now > last_time)
    fps = d_frames * (double) G_USEC_PER_SEC / (now - last_time);
  if (d_frames > 0)
    bpf = (guint32) ((bytes - last_bytes) / d_frames);
  if (last_time != 0 && now > last_time)
    {
      double secs = (now - last_time) / (double) G_USEC_PER_SEC;
      up_s = (server->texture_uploads - last_uploads) / secs;
      rel_s = (server->texture_releases - last_releases) / secs;
    }
  last_time = now;
  last_frames = frames;
  last_bytes = bytes;
  last_uploads = server->texture_uploads;
  last_releases = server->texture_releases;

  if (server->output)
    broadway_output_get_pacing (server->output, &iv_p95, &iv_max, &w_avg, &w_max);

  menu_texture_buffer (server, &tex_count, &tex_bytes);

  len = g_snprintf (line, sizeof line,
                    "stats %08x %" G_GUINT64_FORMAT " %.1f %u %d %u %" G_GUINT64_FORMAT
                    " %u %u %u %u %u %.1f %.1f %u %08x\n",
                    server->session_id, bytes, fps, server->last_latency_ms,
                    server->paint_flash ? 1 : 0, tex_count, tex_bytes,
                    iv_p95, iv_max, w_avg, w_max, bpf, up_s, rel_s,
                    server->owner_id, server->session_token);
  g_socket_send (menu_sock, line, len, NULL, NULL); /* best-effort */
  return G_SOURCE_CONTINUE;
}

/* Debug-menu actions. "reconnect" closes the browser's socket so its auto-reconnect
 * resumes in place (session_token unchanged). "drop-session" first rolls the token,
 * so the reconnecting client sees a new session and hard-resets. Closing the stream
 * routes through the normal EOF cleanup (broadway_input_read frees the input). */
static void
broadway_server_drop_client (BroadwayServer *server, gboolean new_session)
{
  if (new_session)
    {
      do
        server->session_token = g_random_int ();
      while (server->session_token == 0);
    }

  if (server->input != NULL && server->input->connection != NULL)
    g_io_stream_close (server->input->connection, NULL, NULL);
}

/* Toggle the browser's paint-flash overlay. Remembered so it survives reconnects
 * (re-sent in the connect handler). */
static void
broadway_server_set_paint_flash (BroadwayServer *server, gboolean on)
{
  server->paint_flash = on;
  if (server->output)
    {
      broadway_output_debug_flash (server->output, on);
      broadway_server_flush (server);
    }
}

/* Pin (or unpin, with 0s) the browser's logical screen size/scale. Remembered so
 * it survives reconnects (re-sent in the connect handler). The browser applies it
 * via its own resize path, so the canvas rebuilds and GTK relays out cleanly. */
static void
broadway_server_set_debug_screen (BroadwayServer *server, int w, int h, int scale)
{
  server->pinned_w = w;
  server->pinned_h = h;
  server->pinned_scale = scale;
  if (server->output)
    {
      broadway_output_debug_set_screen (server->output, w, h, scale);
      broadway_server_flush (server);
    }
}

/* Push a PNG-preset switch to the app(s) - the app encodes, not the daemon - over
 * the input-event channel. */
static void
broadway_server_set_png (BroadwayServer *server, int preset)
{
  BroadwayInputMsg ev;

  memset (&ev, 0, sizeof ev);
  ev.base.type = BROADWAY_EVENT_SET_PNG;
  ev.base.serial = broadway_server_get_next_serial (server) - 1;
  ev.base.time = broadway_server_get_last_seen_time (server);
  ev.set_png.preset = preset;

  broadway_events_got_input (&ev, -1); /* broadcast to all connected apps */
}

static gboolean
menu_on_readable (GSocket *sock, GIOCondition cond, gpointer user_data)
{
  BroadwayServer *server = user_data;
  char buf[256];
  gssize n;

  n = g_socket_receive (sock, buf, sizeof buf - 1, NULL, NULL);
  if (n <= 0)
    return G_SOURCE_REMOVE; /* peer closed; child-exit drives teardown */

  buf[n] = '\0';
  if (strncmp (buf, "drop-session", 12) == 0)
    broadway_server_drop_client (server, TRUE);
  else if (strncmp (buf, "reconnect", 9) == 0)
    broadway_server_drop_client (server, FALSE);
  else if (strncmp (buf, "paint-flash ", 12) == 0)
    broadway_server_set_paint_flash (server, buf[12] != '0');
  else if (strncmp (buf, "screen ", 7) == 0)
    {
      char **p = g_strsplit (buf + 7, " ", 3);
      if (p[0] && p[1] && p[2])
        broadway_server_set_debug_screen (server,
                                          (int) g_ascii_strtoll (p[0], NULL, 10),
                                          (int) g_ascii_strtoll (p[1], NULL, 10),
                                          (int) g_ascii_strtoll (p[2], NULL, 10));
      g_strfreev (p);
    }
  else if (strncmp (buf, "png-preset ", 11) == 0)
    {
      int preset = -1;
      if (sscanf (buf + 11, "%d", &preset) == 1)
        broadway_server_set_png (server, preset);
    }
  return G_SOURCE_CONTINUE;
}

static gboolean
menu_expect_timeout (gpointer user_data)
{
  BroadwayServer *server = user_data;

  /* The menu never mapped a toplevel; stop expecting so a later unrelated
   * surface isn't mis-tagged as the menu. */
  server->expecting_menu_surface = FALSE;
  menu_expect_timer = 0;
  return G_SOURCE_REMOVE;
}

static void
menu_child_exited (GPid pid, gint status, gpointer user_data)
{
  BroadwayServer *server = user_data;

  g_spawn_close_pid (pid);
  if (pid == menu_pid)
    menu_pid = 0;
  menu_child_watch = 0; /* one-shot; auto-removed after this callback */
  /* If it died before ever mapping a toplevel, don't mis-tag the next one. */
  server->expecting_menu_surface = FALSE;
  menu_control_teardown ();
}

static void
broadway_server_summon_menu (BroadwayServer *server)
{
  const char *cmd;
  char *argv[2];
  char **envp;
  char fdstr[16];
  int sv[2];
  GError *error = NULL;

  /* Toggle: a second summon while it is running closes it. */
  if (menu_pid != 0)
    {
      kill (menu_pid, SIGTERM);
      return;
    }

  /* EVENT_MENU comes from the untrusted browser: after a failed spawn (e.g.
   * binary not installed) back off, don't fork/exec on every event. */
  if (menu_spawn_failed_at != 0 &&
      g_get_monotonic_time () - menu_spawn_failed_at < MENU_SPAWN_RETRY_US)
    return;

  if (socketpair (AF_UNIX, SOCK_STREAM, 0, sv) != 0)
    {
      g_warning ("broadway: debug menu socketpair failed: %s", g_strerror (errno));
      return;
    }
  /* sv[0] = daemon end (close-on-exec, never leaks to the child).
   * sv[1] = child end (left inheritable so it survives exec). */
  fcntl (sv[0], F_SETFD, FD_CLOEXEC);
  g_snprintf (fdstr, sizeof fdstr, "%d", sv[1]);

  cmd = g_getenv ("BROTWAY_DEBUGMENU");
  if (cmd == NULL && (cmd = g_getenv ("BROADWAY_DEBUGMENU")) != NULL)
    g_warning ("BROADWAY_DEBUGMENU is deprecated; use BROTWAY_DEBUGMENU");
  if (cmd == NULL)
    cmd = "gtk4-brotway-debugmenu";

  argv[0] = (char *) cmd;
  argv[1] = NULL;

  envp = g_get_environ ();
  envp = g_environ_setenv (envp, "GDK_BACKEND", "broadway", TRUE);
  if (server->display != NULL)
    envp = g_environ_setenv (envp, "BROTWAY_DISPLAY", server->display, TRUE);
  envp = g_environ_setenv (envp, "BROTWAY_DEBUGMENU_FD", fdstr, TRUE);

  /* LEAVE_DESCRIPTORS_OPEN keeps sv[1] across exec; the daemon's own fds are
   * close-on-exec (GIO sets that), so only the control fd passes through. */
  if (!g_spawn_async (NULL, argv, envp,
                      G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD |
                      G_SPAWN_LEAVE_DESCRIPTORS_OPEN,
                      NULL, NULL, &menu_pid, &error))
    {
      g_warning ("broadway: failed to spawn debug menu '%s': %s",
                 cmd, error->message);
      g_clear_error (&error);
      menu_pid = 0;
      menu_spawn_failed_at = g_get_monotonic_time ();
      close (sv[0]);
    }
  else
    {
      menu_spawn_failed_at = 0;
      /* The menu is the next non-popup surface from a client that connects
       * after this point: pin it on top. The floor keeps dialogs/drag surfaces
       * from already-connected clients out; the timer stops expecting if the
       * menu never maps. */
      server->expecting_menu_surface = TRUE;
      server->menu_client_floor = server->last_gtk_client_id;
      if (menu_expect_timer != 0)
        g_source_remove (menu_expect_timer);
      menu_expect_timer = g_timeout_add_seconds (MENU_EXPECT_TIMEOUT_SECONDS,
                                                 menu_expect_timeout, server);
      menu_child_watch = g_child_watch_add (menu_pid, menu_child_exited, server);

      menu_sock = g_socket_new_from_fd (sv[0], NULL); /* takes ownership of sv[0] */
      if (menu_sock != NULL)
        {
          g_socket_set_blocking (menu_sock, FALSE);
          menu_read_source = g_socket_create_source (menu_sock, G_IO_IN, NULL);
          g_source_set_callback (menu_read_source, (GSourceFunc) menu_on_readable,
                                 server, NULL);
          g_source_attach (menu_read_source, NULL);
          menu_stats_timer = g_timeout_add (MENU_STATS_INTERVAL_MS,
                                            menu_push_stats, server);
        }
      else
        close (sv[0]);
    }

  close (sv[1]); /* the child holds its own copy now */
  g_strfreev (envp);
}

void
broadway_server_set_display (BroadwayServer *server,
                             const char     *display)
{
  g_free (server->display);
  server->display = g_strdup (display);
}

void
broadway_server_client_connected (BroadwayServer *server,
                                  guint32         client_id)
{
  /* Track the newest GTK client id so the menu matcher can tell clients that
   * connected after the summon apart from pre-existing ones. */
  if (client_id > server->last_gtk_client_id)
    server->last_gtk_client_id = client_id;
}

/* Minimum frame size (bytes) the fixed-width parser below reads for each event:
 * the 3-word base header (type, serial, time) plus the type's own fields. The
 * browser is untrusted, so a frame shorter than this would read past it - the
 * caller drops it. Keep in sync with the switch in parse_input_message and with
 * parse_pointer_data (7 words) / parse_touch_data (9 words). Variable-length
 * types list their fixed header; the tail is clamped where it's read. */
static gsize
broadway_event_min_size (guint32 type)
{
  gsize words = 3; /* base: type, serial, time */

  switch (type)
    {
    case BROADWAY_EVENT_ENTER:
    case BROADWAY_EVENT_LEAVE:               words += 7 + 1; break; /* pointer + mode */
    case BROADWAY_EVENT_POINTER_MOVE:        words += 7;     break; /* pointer */
    case BROADWAY_EVENT_BUTTON_PRESS:
    case BROADWAY_EVENT_BUTTON_RELEASE:      words += 7 + 1; break; /* pointer + button */
    case BROADWAY_EVENT_SCROLL:              words += 7 + 4; break; /* pointer + dx,dy,unit,is_stop */
    case BROADWAY_EVENT_TOUCH:               words += 9;     break; /* touch */
    case BROADWAY_EVENT_KEY_PRESS:
    case BROADWAY_EVENT_KEY_RELEASE:         words += 2;     break; /* key, state */
    case BROADWAY_EVENT_CONFIGURE_NOTIFY:    words += 5;     break; /* id, x, y, w, h */
    case BROADWAY_EVENT_ROUNDTRIP_NOTIFY:    words += 2;     break; /* id, tag */
    case BROADWAY_EVENT_SCREEN_SIZE_CHANGED: words += 3;     break; /* w, h, scale */
    case BROADWAY_EVENT_CLIPBOARD_CONTENTS:  words += 2;     break; /* id, len (text follows) */
    case BROADWAY_EVENT_PING:                words += 1;     break; /* latency */
    /* GRAB/UNGRAB_NOTIFY, SUSPEND/RESUME, MENU and unknown types: base only. */
    default: break;
    }

  return words * sizeof (guint32);
}

static void
parse_input_message (BroadwayInput *input, const unsigned char *message, gsize payload_len)
{
  BroadwayServer *server = input->server;
  BroadwayInputMsg msg;
  guint32 *p;
  gint64 time_;
  GList *l;

  memset (&msg, 0, sizeof (msg));

  /* The browser is untrusted: a frame too short for even the base header
   * (type, serial, time) would read past it. */
  if (payload_len < 3 * sizeof (guint32))
    return;

  p = (guint32 *) message;

  msg.base.type = ntohl (*p++);
  msg.base.serial = ntohl (*p++);

  time_ = ntohl (*p++);

  if (time_ == 0) {
    time_ = server->last_seen_time;
  } else {
    if (!input->seen_time) {
      input->seen_time = TRUE;
      /* Calculate time base so that any following times are normalized to start
         5 seconds after last_seen_time, to avoid issues that could appear when
         a long hiatus due to a reconnect seems to be instant */
      input->time_base = time_ - (server->last_seen_time + 5000);
    }
    time_ = time_ - input->time_base;
  }

  server->last_seen_time = time_;

  msg.base.time = time_;

  /* Reject a frame too short for the fields this event's parser will read. */
  if (payload_len < broadway_event_min_size (msg.base.type))
    return;

  switch (msg.base.type) {
  case BROADWAY_EVENT_ENTER:
  case BROADWAY_EVENT_LEAVE:
    p = parse_pointer_data (p, &msg.pointer);
    update_future_pointer_info (server, &msg.pointer);
    msg.crossing.mode = ntohl (*p++);
    break;

  case BROADWAY_EVENT_POINTER_MOVE: /* Mouse move */
    p = parse_pointer_data (p, &msg.pointer);
    update_future_pointer_info (server, &msg.pointer);
    break;

  case BROADWAY_EVENT_BUTTON_PRESS:
  case BROADWAY_EVENT_BUTTON_RELEASE:
    p = parse_pointer_data (p, &msg.pointer);
    update_future_pointer_info (server, &msg.pointer);
    msg.button.button = ntohl (*p++);
    break;

  case BROADWAY_EVENT_SCROLL:
    p = parse_pointer_data (p, &msg.pointer);
    update_future_pointer_info (server, &msg.pointer);
    msg.scroll.dx = ntohl (*p++);
    msg.scroll.dy = ntohl (*p++);
    msg.scroll.unit = ntohl (*p++);
    msg.scroll.is_stop = ntohl (*p++);
    break;

  case BROADWAY_EVENT_TOUCH:
    p = parse_touch_data (p, &msg.touch);
    break;

  case BROADWAY_EVENT_KEY_PRESS:
  case BROADWAY_EVENT_KEY_RELEASE:
    msg.key.surface_id = server->focused_surface_id;
    msg.key.key = ntohl (*p++);
    msg.key.state = ntohl (*p++);
    break;

  case BROADWAY_EVENT_GRAB_NOTIFY:
  case BROADWAY_EVENT_UNGRAB_NOTIFY:
    /* No payload beyond the base header - the client sends no fields, and
     * grab_reply.res is unused. (Previously read a 4th word the client never
     * sent, over-reading the frame by 4 bytes.) */
    break;

  case BROADWAY_EVENT_CONFIGURE_NOTIFY:
    msg.configure_notify.id = ntohl (*p++);
    msg.configure_notify.x = ntohl (*p++);
    msg.configure_notify.y = ntohl (*p++);
    msg.configure_notify.width = ntohl (*p++);
    msg.configure_notify.height = ntohl (*p++);
    break;

  case BROADWAY_EVENT_ROUNDTRIP_NOTIFY:
    msg.roundtrip_notify.id = ntohl (*p++);
    msg.roundtrip_notify.tag = ntohl (*p++);
    msg.roundtrip_notify.local = FALSE;

    /* Remove matched outstanding roundtrips */
    for (l = server->outstanding_roundtrips; l != NULL; l = l->next)
      {
        BroadwayOutstandingRoundtrip *rt = l->data;

        if (rt->id == msg.roundtrip_notify.id &&
            rt->tag == msg.roundtrip_notify.tag)
          break;
      }

    if (l == NULL)
      g_warning ("Got unexpected roundtrip reply for id %d, tag %d\n", msg.roundtrip_notify.id, msg.roundtrip_notify.tag);
    else
      {
        BroadwayOutstandingRoundtrip *rt = l->data;

        server->outstanding_roundtrips = g_list_delete_link (server->outstanding_roundtrips, l);
        g_free (rt);
      }

    break;

  case BROADWAY_EVENT_SCREEN_SIZE_CHANGED:
    msg.screen_resize_notify.width = ntohl (*p++);
    msg.screen_resize_notify.height = ntohl (*p++);
    msg.screen_resize_notify.scale = ntohl (*p++);
    break;

  case BROADWAY_EVENT_SUSPEND:
  case BROADWAY_EVENT_RESUME:
    /* No payload beyond the base header; forward to GTK to freeze/thaw. */
    break;

  case BROADWAY_EVENT_CLIPBOARD_CONTENTS:
    {
      /* type, serial, time, id, len precede the text in the frame. */
      gsize header = 5 * sizeof (guint32);
      guint32 id, len;

      if (payload_len < header)
        return; /* truncated frame */

      id = ntohl (*p++);
      len = ntohl (*p++);

      /* The browser is untrusted: never read past the frame, and cap the size. */
      if (len > payload_len - header)
        len = payload_len - header;
      if (len > BROADWAY_CLIPBOARD_MAX_SIZE)
        len = BROADWAY_CLIPBOARD_MAX_SIZE;

      broadway_clipboard_contents_received (id, (const char *) p, len);
    }
    return;

  case BROADWAY_EVENT_PING:
    /* Liveness probe: reply, don't forward to the app. The payload carries the
     * client's last measured round-trip (ms), surfaced in the debug menu. */
    if (payload_len < 4 * sizeof (guint32))
      return; /* truncated frame */
    server->last_latency_ms = ntohl (*p++);
    if (server->output)
      {
        broadway_output_pong_msg (server->output);
        broadway_server_flush (server);
      }
    return;

  case BROADWAY_EVENT_MENU:
    /* Daemon-intercepted: spawn/toggle the menu, never forward to clients. */
    broadway_server_summon_menu (server);
    return;

  default:
    g_printerr ("parse_input_message - Unknown input command %c (%s)\n", msg.base.type, message);
    break;
  }

  queue_input_message (server, &msg);
}

static inline void
hex_dump (guchar *data, gsize len)
{
#ifdef DEBUG_WEBSOCKETS
  gsize i, j;
  for (j = 0; j < len + 15; j += 16)
    {
      fprintf (stderr, "0x%.4x  ", j);
      for (i = 0; i < 16; i++)
        {
          if ((j + i) < len)
            fprintf (stderr, "%.2x ", data[j+i]);
          else
            fprintf (stderr, "  ");
          if (i == 8)
            fprintf (stderr, " ");
        }
      fprintf (stderr, " | ");

      for (i = 0; i < 16; i++)
        if ((j + i) < len && g_ascii_isalnum(data[j+i]))
          fprintf (stderr, "%c", data[j+i]);
        else
          fprintf (stderr, ".");
      fprintf (stderr, "\n");
    }
#endif
}

static void
parse_input (BroadwayInput *input)
{
  if (!input->buffer->len)
    return;

  hex_dump (input->buffer->data, input->buffer->len);

  while (input->buffer->len > 2)
    {
      gsize len, payload_len;
      BroadwayWSOpCode code;
      gboolean is_mask, fin;
      guchar *buf, *data, *mask;

      buf = input->buffer->data;
      len = input->buffer->len;

#ifdef DEBUG_WEBSOCKETS
      g_print ("Parse input first byte 0x%2x 0x%2x\n", buf[0], buf[1]);
#endif

      fin = buf[0] & 0x80;
      code = buf[0] & 0x0f;
      payload_len = buf[1] & 0x7f;
      is_mask = buf[1] & 0x80;
      data = buf + 2;

      if (payload_len == 126)
        {
          if (len < 4)
            return;
          payload_len = GUINT16_FROM_BE( *(guint16 *) data );
          data += 2;
        }
      else if (payload_len == 127)
        {
          if (len < 10)
            return;
          payload_len = GUINT64_FROM_BE( *(guint64 *) data );
          data += 8;
        }

      mask = NULL;
      if (is_mask)
        {
          if (data - buf + 4 > len)
            return;
          mask = data;
          data += 4;
        }

      if (data - buf + payload_len > len)
        return; /* wait to accumulate more */

      if (is_mask)
        {
          gsize i;
          for (i = 0; i < payload_len; i++)
            data[i] ^= mask[i%4];
        }

      switch (code) {
      case BROADWAY_WS_CNX_CLOSE:
        break; /* hang around anyway */
      case BROADWAY_WS_BINARY:
        if (!fin)
          {
#ifdef DEBUG_WEBSOCKETS
            g_warning ("can't yet accept fragmented input");
#endif
          }
        else
          {
            parse_input_message (input, data, payload_len);
          }
        break;
      case BROADWAY_WS_CNX_PING:
        if (input->output)
          broadway_output_pong (input->output);
        break;
      case BROADWAY_WS_CNX_PONG:
        break; /* we never send pings, but tolerate pongs */
      case BROADWAY_WS_TEXT:
      case BROADWAY_WS_CONTINUATION:
      default:
        {
          g_warning ("fragmented or unknown input code 0x%2x with fin set", code);
          break;
        }
      }

      g_byte_array_remove_range (input->buffer, 0, data - buf + payload_len);
    }
}


static gboolean
process_input_idle_cb (BroadwayServer *server)
{
  server->process_input_idle = 0;
  process_input_messages (server);
  return G_SOURCE_REMOVE;
}

static void
queue_process_input_at_idle (BroadwayServer *server)
{
  if (server->process_input_idle == 0)
    server->process_input_idle =
      g_idle_add_full (G_PRIORITY_DEFAULT, (GSourceFunc)process_input_idle_cb, server, NULL);
}

static gboolean
broadway_server_read_all_input_nonblocking (BroadwayInput *input)
{
  GInputStream *in;
  gssize res;
  guint8 buffer[1024];
  GError *error = NULL;

  if (input == NULL)
    return FALSE;

  in = g_io_stream_get_input_stream (input->connection);

  res = g_pollable_input_stream_read_nonblocking (G_POLLABLE_INPUT_STREAM (in),
                                                  buffer, sizeof (buffer), NULL, &error);

  if (res <= 0)
    {
      if (res < 0 &&
          g_error_matches (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
        {
          g_error_free (error);
          return TRUE;
        }

      if (input->server->input == input)
        {
          send_outstanding_roundtrips (input->server);

          input->server->input = NULL;
        }
      broadway_input_free (input);
      if (res < 0)
        {
          g_printerr ("input error %s\n", error->message);
          g_error_free (error);
        }
      return FALSE;
    }

  g_byte_array_append (input->buffer, buffer, res);

  parse_input (input);
  return TRUE;
}

static void
broadway_server_consume_all_input (BroadwayServer *server)
{
  broadway_server_read_all_input_nonblocking (server->input);

  /* Since we're parsing input but not processing the resulting messages
     we might not get a readable callback on the stream, so queue an idle to
     process the messages */
  queue_process_input_at_idle (server);
}


static gboolean
input_data_cb (GObject  *stream,
               BroadwayInput *input)
{
  BroadwayServer *server = input->server;

  if (!broadway_server_read_all_input_nonblocking (input))
    return FALSE;

  if (input->active)
    process_input_messages (server);

  return TRUE;
}

guint32
broadway_server_get_next_serial (BroadwayServer *server)
{
  if (server->output)
    return broadway_output_get_next_serial (server->output);

  return server->saved_serial;
}

void
broadway_server_get_screen_size (BroadwayServer   *server,
                                 guint32          *width,
                                 guint32          *height,
                                 guint32          *scale)
{
  *width = server->root->width;
  *height = server->root->height;
  *scale = server->screen_scale;
}

static void
broadway_server_fake_roundtrip_reply (BroadwayServer *server,
                                      int             id,
                                      guint32         tag)
{
  BroadwayInputMsg msg;

  msg.base.type = BROADWAY_EVENT_ROUNDTRIP_NOTIFY;
  msg.base.serial = 0;
  msg.base.time = server->last_seen_time;
  msg.roundtrip_notify.id = id;
  msg.roundtrip_notify.tag = tag;
  msg.roundtrip_notify.local = 1;

  queue_input_message (server, &msg);
  queue_process_input_at_idle (server);
}

void
broadway_server_flush (BroadwayServer *server)
{
  if (server->output &&
      !broadway_output_flush (server->output))
    {
      server->saved_serial = broadway_output_get_next_serial (server->output);
      server->session_bytes += broadway_output_get_bytes_sent (server->output);
      server->session_frames += broadway_output_get_frames (server->output);
      broadway_output_free (server->output);
      server->output = NULL;
      send_outstanding_roundtrips (server);
    }
}

void
broadway_server_roundtrip (BroadwayServer *server,
                           int             id,
                           guint32         tag)
{
  if (server->output)
    {
      BroadwayOutstandingRoundtrip *rt = g_new0 (BroadwayOutstandingRoundtrip, 1);
      rt->id = id;
      rt->tag = tag;
      server->outstanding_roundtrips = g_list_prepend (server->outstanding_roundtrips, rt);

      broadway_output_roundtrip (server->output, id, tag);
    }
  else
    broadway_server_fake_roundtrip_reply (server, id, tag);
}

static const char *
parse_line (const char *line, const char *key)
{
  const char *p;

  if (g_ascii_strncasecmp (line, key, strlen (key)) != 0)
    return NULL;
  p = line + strlen (key);
  if (*p != ':')
    return NULL;
  p++;
  /* Skip optional initial space */
  if (*p == ' ')
    p++;
  return p;
}

static void
send_error (HttpRequest *request,
            int error_code,
            const char *reason)
{
  char *res;

  res = g_strdup_printf ("HTTP/1.0 %d %s\r\n\r\n"
                         "<html><head><title>%d %s</title></head>"
                         "<body>%s</body></html>",
                         error_code, reason,
                         error_code, reason,
                         reason);

  /* TODO: This should really be async */
  g_output_stream_write_all (g_io_stream_get_output_stream (request->connection),
                             res, strlen (res), NULL, NULL, NULL);

  g_free (res);
  http_request_free (request);
}

/* magic from: http://tools.ietf.org/html/draft-ietf-hybi-thewebsocketprotocol-17 */
#define SEC_WEB_SOCKET_KEY_MAGIC "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/* 'x3JJHMbDL1EzLkh9GBhXDw==' generates 'HSmrc0sMlYUkAGmm5OPpG2HaGWk=' */
static char *
generate_handshake_response_wsietf_v7 (const char *key)
{
  gsize digest_len = 20;
  guchar digest[20];
  GChecksum *checksum;

  checksum = g_checksum_new (G_CHECKSUM_SHA1);
  if (!checksum)
    return NULL;

  g_checksum_update (checksum, (guchar *)key, -1);
  g_checksum_update (checksum, (guchar *)SEC_WEB_SOCKET_KEY_MAGIC, -1);

  g_checksum_get_digest (checksum, digest, &digest_len);
  g_checksum_free (checksum);

  g_assert (digest_len == 20);

  return g_base64_encode (digest, digest_len);
}

static void
start_input (HttpRequest *request, const char *query)
{
  char **lines;
  const char *p;
  int i;
  char *res;
  const char *origin, *host;
  BroadwayInput *input;
  const void *data_buffer;
  gsize data_buffer_size;
  GInputStream *in;
  const char *key;
  GSocket *socket;
  int flag = 1;

#ifdef DEBUG_WEBSOCKETS
  g_print ("incoming request:\n%s\n", request->request->str);
#endif
  lines = g_strsplit (request->request->str, "\n", 0);

  key = NULL;
  origin = NULL;
  host = NULL;
  for (i = 0; lines[i] != NULL; i++)
    {
      if ((p = parse_line (lines[i], "Sec-WebSocket-Key")))
        key = p;
      else if ((p = parse_line (lines[i], "Origin")))
        origin = p;
      else if ((p = parse_line (lines[i], "Host")))
        host = p;
      else if ((p = parse_line (lines[i], "Sec-WebSocket-Origin")))
        origin = p;
    }

  if (host == NULL)
    {
      g_strfreev (lines);
      send_error (request, 400, "Bad websocket request");
      return;
    }

  if (key != NULL)
    {
      char* accept = generate_handshake_response_wsietf_v7 (key);
      res = g_strdup_printf ("HTTP/1.1 101 Switching Protocols\r\n"
                             "Upgrade: websocket\r\n"
                             "Connection: Upgrade\r\n"
                             "Sec-WebSocket-Accept: %s\r\n"
                             "%s%s%s"
                             "Sec-WebSocket-Location: ws://%s/socket\r\n"
                             "Sec-WebSocket-Protocol: broadway\r\n"
                             "\r\n", accept,
                             origin?"Sec-WebSocket-Origin: ":"", origin?origin:"", origin?"\r\n":"",
                             host);
      g_free (accept);

#ifdef DEBUG_WEBSOCKETS
      g_print ("v7 proto response:\n%s", res);
#endif

      g_output_stream_write_all (g_io_stream_get_output_stream (request->connection),
                                 res, strlen (res), NULL, NULL, NULL);
      g_free (res);
    }
  else
    {
      g_strfreev (lines);
      send_error (request, 400, "Bad websocket request");
      return;
    }

  socket = g_socket_connection_get_socket (request->socket_connection);
  setsockopt (g_socket_get_fd (socket), IPPROTO_TCP,
              TCP_NODELAY, (char *) &flag, sizeof(int));

  input = g_new0 (BroadwayInput, 1);
  input->server = request->server;
  input->connection = g_object_ref (request->connection);

  /* ?cid=<n> identifies a reconnecting client; absent/0 means a fresh load.
   * Match at a param boundary so "mycid=" doesn't. */
  if (query)
    {
      const char *qp = query;
      while (*qp)
        {
          if (strncmp (qp, "cid=", 4) == 0)
            {
              input->client_id = (guint32) strtoul (qp + 4, NULL, 10);
              break;
            }
          qp = strchr (qp, '&');
          if (!qp)
            break;
          qp++;
        }
    }

  data_buffer = g_buffered_input_stream_peek_buffer (G_BUFFERED_INPUT_STREAM (request->data), &data_buffer_size);
  input->buffer = g_byte_array_sized_new (data_buffer_size);
  g_byte_array_append (input->buffer, data_buffer, data_buffer_size);

  input->output =
    broadway_output_new (g_io_stream_get_output_stream (request->connection), 0);

  /* This will free and close the data input stream, but we got all the buffered content already */
  http_request_free (request);

  in = g_io_stream_get_input_stream (input->connection);

  input->source = g_pollable_input_stream_create_source (G_POLLABLE_INPUT_STREAM (in), NULL);
  g_source_set_callback (input->source, (GSourceFunc)input_data_cb, input, NULL);
  g_source_attach (input->source, NULL);

  /* A rejected client frees `input` and returns FALSE; don't touch it after. */
  if (start (input))
    parse_input (input);   /* process any data already in the pipe */

  g_strfreev (lines);
}

static void
send_outstanding_roundtrips (BroadwayServer *server)
{
  GList *l;

  for (l = server->outstanding_roundtrips; l != NULL; l = l->next)
    {
      BroadwayOutstandingRoundtrip *rt = l->data;
      broadway_server_fake_roundtrip_reply (server, rt->id, rt->tag);
    }

  g_list_free_full (server->outstanding_roundtrips, g_free);
  server->outstanding_roundtrips = NULL;
}

static gboolean
start (BroadwayInput *input)
{
  BroadwayServer *server;
  guint32 req;

  input->active = TRUE;

  server = BROADWAY_SERVER (input->server);

  /* Single-display arbitration: a fresh load (or any connect when nobody owns
   * the display) takes over; a reconnect resumes only if it still owns it. */
  req = input->client_id;
  if (req != 0 && req == server->owner_id)
    ; /* owner resume */
  else if (server->owner_id == 0 || req == 0)
    server->owner_id = ++server->next_client_id; /* fresh: new owner */
  else
    {
      /* Superseded by a newer load. Reject on its own connection (no race with
       * the owner's teardown) and free it fully so no zombie lingers. */
      broadway_output_disconnected (input->output);
      broadway_output_flush (input->output);
      broadway_output_free (input->output);
      input->output = NULL;
      broadway_input_free (input);
      return FALSE;
    }

  if (server->output)
    {
      send_outstanding_roundtrips (server);
      broadway_output_disconnected (server->output);
      broadway_output_flush (server->output);
    }

  if (server->input != NULL)
    {
      send_outstanding_roundtrips (server);
      broadway_input_free (server->input);
      server->input = NULL;
    }

  server->input = input;

  if (server->output)
    {
      server->saved_serial = broadway_output_get_next_serial (server->output);
      server->session_bytes += broadway_output_get_bytes_sent (server->output);
      server->session_frames += broadway_output_get_frames (server->output);
      broadway_output_free (server->output);
    }
  server->output = input->output;

  broadway_output_set_next_serial (server->output, server->saved_serial);
  broadway_output_flush (server->output);

  /* Send the session token before the resync, so the client can reset its
   * cache before surfaces are rebuilt. */
  broadway_output_session (server->output, server->session_token, server->owner_id);

  /* Restore the paint-flash debug state for a (re)connecting client. */
  if (server->paint_flash)
    broadway_output_debug_flash (server->output, TRUE);

  /* Restore a pinned debug screen size/scale too. */
  if (server->pinned_scale != 0)
    broadway_output_debug_set_screen (server->output, server->pinned_w,
                                      server->pinned_h, server->pinned_scale);

  broadway_server_resync_surfaces (server);

  /* The resync flushes may have dropped the output on a write error. Replay the
   * whole grab stack outermost-first so the client rebuilds the chain in order. */
  if (server->pointer_grabs && server->output)
    {
      GList *l;
      for (l = g_list_last (server->pointer_grabs); l != NULL; l = l->prev)
        {
          BroadwayGrab *g = l->data;
          broadway_output_grab_pointer (server->output, g->surface_id,
                                        g->owner_events);
        }
    }

  process_input_messages (server);

  return TRUE;
}

static void
send_data (HttpRequest *request,
           const char *mimetype,
           const char *data, gsize len)
{
  char *res;

  /* No-store so a redeployed broadwayd's fresh client.html/broadway.js is always
   * fetched - otherwise the browser serves a stale cached copy (these have no
   * validator) and runs old code after an upgrade. They're tiny and loaded once
   * per page, so skipping the cache is free. */
  res = g_strdup_printf ("HTTP/1.0 200 OK\r\n"
                         "Content-Type: %s\r\n"
                         "Content-Length: %"G_GSIZE_FORMAT"\r\n"
                         "Cache-Control: no-store\r\n"
                         "\r\n",
                         mimetype, len);

  /* TODO: This should really be async */
  g_output_stream_write_all (g_io_stream_get_output_stream (request->connection),
                             res, strlen (res), NULL, NULL, NULL);
  g_free (res);
  g_output_stream_write_all (g_io_stream_get_output_stream (request->connection),
                             data, len, NULL, NULL, NULL);
  http_request_free (request);
}

#include "clienthtml.h"
#include "broadwayjs.h"

static void
got_request (HttpRequest *request)
{
  char *start, *escaped, *tmp, *version, *query;

  if (!g_str_has_prefix (request->request->str, "GET "))
    {
      send_error (request, 501, "Only GET implemented");
      return;
    }

  start = request->request->str + 4; /* Skip "GET " */

  while (*start == ' ')
    start++;

  for (tmp = start; *tmp != 0 && *tmp != ' ' && *tmp != '\n'; tmp++)
    ;
  escaped = g_strndup (start, tmp - start);
  version = NULL;
  if (*tmp == ' ')
    {
      start = tmp;
      while (*start == ' ')
        start++;
      for (tmp = start; *tmp != 0 && *tmp != ' ' && *tmp != '\n'; tmp++)
        ;
      version = g_strndup (start, tmp - start);
    }

  query = strchr (escaped, '?');
  if (query)
    *query = 0;

  if (strcmp (escaped, "/client.html") == 0 || strcmp (escaped, "/") == 0)
    send_data (request, "text/html", client_html, G_N_ELEMENTS(client_html) - 1);
  else if (strcmp (escaped, "/broadway.js") == 0)
    send_data (request, "text/javascript", broadway_js, G_N_ELEMENTS(broadway_js) - 1);
  else if (strcmp (escaped, "/socket") == 0)
    start_input (request, query ? query + 1 : NULL);
  else
    send_error (request, 404, "File not found");

  g_free (escaped);
  g_free (version);
}

static void
got_http_request_line (GInputStream *stream,
                       GAsyncResult *result,
                       HttpRequest *request)
{
  char *line;

  line = g_data_input_stream_read_line_finish (G_DATA_INPUT_STREAM (stream), result, NULL, NULL);
  if (line == NULL)
    {
      http_request_free (request);
      g_printerr ("Error reading request lines\n");
      return;
    }
  if (strlen (line) == 0)
    got_request (request);
  else
    {
      /* Protect against overflow in request length */
      if (request->request->len > 1024 * 5)
        {
          send_error (request, 400, "Request too long");
        }
      else
        {
          g_string_append_printf (request->request, "%s\n", line);
          g_data_input_stream_read_line_async (request->data, 0, NULL,
                                               (GAsyncReadyCallback)got_http_request_line, request);
        }
    }
  g_free (line);
}

static gboolean
handle_incoming_connection (GSocketService    *service,
                            GSocketConnection *connection,
                            GObject           *source_object)
{
  HttpRequest *request;
  GInputStream *in;

  request = g_new0 (HttpRequest, 1);
  request->socket_connection = g_object_ref (connection);
  request->server = BROADWAY_SERVER (source_object);
  request->request = g_string_new ("");

  if (request->server->ssl_cert && request->server->ssl_key)
    {
      GError *error = NULL;
      GTlsCertificate *certificate;

      certificate = g_tls_certificate_new_from_files (request->server->ssl_cert,
                                                      request->server->ssl_key,
                                                      &error);
      if (!certificate)
        {
          g_warning ("Cannot create TLS certificate: %s", error->message);
          g_error_free (error);
          return FALSE;
        }

      request->connection = g_tls_server_connection_new (G_IO_STREAM (connection),
                                                         certificate,
                                                         &error);
      if (!request->connection)
        {
          g_warning ("Cannot create TLS connection: %s", error->message);
          g_error_free (error);
          return FALSE;
        }

      if (!g_tls_connection_handshake (G_TLS_CONNECTION (request->connection),
                                       NULL, &error))
        {
          g_warning ("Cannot create TLS connection: %s", error->message);
          g_error_free (error);
          return FALSE;
        }
    }
  else
    {
      request->connection = G_IO_STREAM (g_object_ref (connection));
    }

  in = g_io_stream_get_input_stream (request->connection);

  request->data = g_data_input_stream_new (in);
  g_filter_input_stream_set_close_base_stream (G_FILTER_INPUT_STREAM (request->data), FALSE);
  /* Be tolerant of input */
  g_data_input_stream_set_newline_type (request->data, G_DATA_STREAM_NEWLINE_TYPE_ANY);

  g_data_input_stream_read_line_async (request->data, 0, NULL,
                                       (GAsyncReadyCallback)got_http_request_line, request);
  return TRUE;
}

BroadwayServer *
broadway_server_new (char        *address,
                     int          port,
                     const char  *ssl_cert,
                     const char  *ssl_key,
                     GError     **error)
{
  BroadwayServer *server;
  GInetAddress *inet_address;
  GSocketAddress *socket_address;

  server = g_object_new (BROADWAY_TYPE_SERVER, NULL);
  server->port = port;
  server->address = g_strdup (address);
  server->ssl_cert = g_strdup (ssl_cert);
  server->ssl_key = g_strdup (ssl_key);

  if (address == NULL)
    {
      if (!g_socket_listener_add_inet_port (G_SOCKET_LISTENER (server->service),
                                            server->port,
                                            G_OBJECT (server),
                                            error))
        {
          g_prefix_error (error, "Unable to listen to port %d: ", server->port);
          g_object_unref (server);
          return NULL;
        }
    }
  else
    {
      inet_address = g_inet_address_new_from_string (address);
      if (inet_address == NULL)
        {
          g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Invalid ip address %s: ", address);
          g_object_unref (server);
          return NULL;
        }
      socket_address = g_inet_socket_address_new (inet_address, port);
      g_object_unref (inet_address);
      if (!g_socket_listener_add_address (G_SOCKET_LISTENER (server->service),
                                          socket_address,
                                          G_SOCKET_TYPE_STREAM,
                                          G_SOCKET_PROTOCOL_TCP,
                                          G_OBJECT (server),
                                          NULL,
                                          error))
        {
          g_prefix_error (error, "Unable to listen to %s:%d: ", server->address, server->port);
          g_object_unref (socket_address);
          g_object_unref (server);
          return NULL;
        }
      g_object_unref (socket_address);
    }

  g_signal_connect (server->service, "incoming",
                    G_CALLBACK (handle_incoming_connection), NULL);
  return server;
}

BroadwayServer *
broadway_server_on_unix_socket_new (char *address, GError **error)
{
  BroadwayServer *server;
  GSocketAddress *socket_address = NULL;

  server = g_object_new (BROADWAY_TYPE_SERVER, NULL);
  server->port = -1;
  server->address = g_strdup (address);

  if (address == NULL)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Unspecified unix domain socket address");
      g_object_unref (server);
      return NULL;
    }
  else
    {
#ifdef HAVE_GIO_UNIX
      socket_address = g_unix_socket_address_new (address);
#endif
      if (socket_address == NULL)
        {
          g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA, "Invalid unix domain socket address %s: ", address);
          g_object_unref (server);
          return NULL;
        }
      if (!g_socket_listener_add_address (G_SOCKET_LISTENER (server->service),
                                          socket_address,
                                          G_SOCKET_TYPE_STREAM,
                                          G_SOCKET_PROTOCOL_DEFAULT,
                                          G_OBJECT (server),
                                          NULL,
                                          error))
        {
          g_prefix_error (error, "Unable to listen to %s: ", server->address);
          g_object_unref (socket_address);
          g_object_unref (server);
          return NULL;
        }
      g_object_unref (socket_address);
    }

  g_signal_connect (server->service, "incoming",
                    G_CALLBACK (handle_incoming_connection), NULL);
  return server;
}

guint32
broadway_server_get_last_seen_time (BroadwayServer *server)
{
  broadway_server_consume_all_input (server);
  return (guint32) server->last_seen_time;
}

void
broadway_server_query_mouse (BroadwayServer *server,
                             guint32            *surface,
                             gint32             *root_x,
                             gint32             *root_y,
                             guint32            *mask)
{
  if (server->output)
    {
      broadway_server_consume_all_input (server);
      if (root_x)
        *root_x = server->future_root_x;
      if (root_y)
        *root_y = server->future_root_y;
      if (mask)
        *mask = server->future_state;
      if (surface)
        *surface = server->future_mouse_in_surface;
      return;
    }

  /* Fallback when unconnected */
  if (root_x)
    *root_x = server->last_x;
  if (root_y)
    *root_y = server->last_y;
  if (mask)
    *mask = server->last_state;
  if (surface)
    *surface = server->mouse_in_surface_id;
}

static void restack_layers (BroadwayServer *server);

void
broadway_server_destroy_surface (BroadwayServer *server,
                                 int id,
                                 gboolean disconnected)
{
  BroadwaySurface *surface;
  gint32 transient_for = -1;
  gboolean is_popup = FALSE;
  gboolean was_focused = FALSE;

  if (server->mouse_in_surface_id == id)
    {
      /* TODO: Send leave + enter event, update cursors, etc */
      server->mouse_in_surface_id = 0;
    }

  grab_stack_drop_through (server, id);

  if (server->output)
    broadway_output_destroy_surface (server->output, id);

  surface = broadway_server_lookup_surface (server, id);
  if (surface != NULL)
    {
      /* A popup (menu, popover, …) keeps its parent for pointer recovery
       * below, independent of keyboard focus. */
      transient_for = surface->transient_for;
      is_popup = surface->is_popup;
      was_focused = (server->focused_surface_id == id);

      server->surfaces = g_list_remove (server->surfaces, surface);
      g_hash_table_remove (server->surface_id_hash,
                           GINT_TO_POINTER (id));
      broadway_surface_free (server, surface);
    }

  /* If a menu toplevel just closed, re-pin the rest (or drop menu_owner when
   * its last toplevel is gone). Harmless no-op for ordinary surfaces. */
  restack_layers (server);

  if (is_popup && transient_for > 0 && !disconnected)
    {
      if (was_focused &&
          broadway_server_lookup_surface (server, transient_for) != NULL)
        broadway_server_focus_surface (server, transient_for);
      recover_pointer_focus (server, transient_for);
    }
}

gboolean
broadway_server_surface_show (BroadwayServer *server,
                              int id)
{
  BroadwaySurface *surface;
  gboolean sent = FALSE;

  surface = broadway_server_lookup_surface (server, id);
  if (surface == NULL)
    return FALSE;

  surface->visible = TRUE;

  if (server->output)
    {
      broadway_output_show_surface (server->output, surface->id);
      sent = TRUE;
    }

  return sent;
}

gboolean
broadway_server_surface_hide (BroadwayServer *server,
                              int id)
{
  BroadwaySurface *surface;
  gboolean sent = FALSE;

  surface = broadway_server_lookup_surface (server, id);
  if (surface == NULL)
    return FALSE;

  surface->visible = FALSE;

  if (server->mouse_in_surface_id == id)
    {
      /* TODO: Send leave + enter event, update cursors, etc */
      server->mouse_in_surface_id = 0;
    }

  grab_stack_drop_through (server, id);

  if (server->output)
    {
      broadway_output_hide_surface (server->output, surface->id);
      sent = TRUE;
    }

  /* A popped-down popup (e.g. a GtkDropDown reuses its surface, hiding rather
   * than destroying it) leaves pointer focus stale just like destroy does. */
  if (surface->is_popup && surface->transient_for > 0)
    recover_pointer_focus (server, surface->transient_for);

  return sent;
}

/* Always-on-top: a surface's explicit keep_above flag, or membership in the
 * spawned debug-menu client (which is pinned implicitly without a binary change). */
static gboolean
surface_is_above (BroadwayServer *server,
                  BroadwaySurface *s)
{
  return s != NULL &&
         (s->keep_above ||
          (server->menu_owner != 0 && s->owner == server->menu_owner));
}

/* Keep keep-above toplevels (incl. the debug menu's) above every normal surface,
 * preserving their relative order. Popups follow via transient_for. Also drops a
 * stale menu_owner once its last surface is gone. Generalizes the old
 * restack_layers into a per-surface layer. */
static void
restack_layers (BroadwayServer *server)
{
  GList *l, *above = NULL;
  gboolean menu_alive = FALSE;

  for (l = server->surfaces; l != NULL; l = l->next)
    {
      BroadwaySurface *s = l->data;
      if (server->menu_owner != 0 && s->owner == server->menu_owner)
        menu_alive = TRUE;
      if (surface_is_above (server, s) && !s->is_popup)
        above = g_list_append (above, s);
    }

  if (server->menu_owner != 0 && !menu_alive)
    server->menu_owner = 0; /* the menu client is gone */

  if (above == NULL)
    return;

  /* Already the topmost block, in order? Avoid redundant raises. */
  {
    GList *a = g_list_last (server->surfaces);
    GList *b = g_list_last (above);
    while (b != NULL && a != NULL && a->data == b->data) { a = a->prev; b = b->prev; }
    if (b == NULL) { g_list_free (above); return; }
  }

  /* Move them to the top, keeping their order, and raise each in the browser. */
  for (l = above; l != NULL; l = l->next)
    {
      BroadwaySurface *s = l->data;
      server->surfaces = g_list_remove (server->surfaces, s);
      server->surfaces = g_list_append (server->surfaces, s);
      if (server->output)
        broadway_output_raise_surface (server->output, s->id);
    }

  g_list_free (above);
}

void
broadway_server_surface_raise (BroadwayServer *server,
                               int id)
{
  BroadwaySurface *surface;

  surface = broadway_server_lookup_surface (server, id);
  if (surface == NULL)
    return;

  server->surfaces = g_list_remove (server->surfaces, surface);
  server->surfaces = g_list_append (server->surfaces, surface);

  if (server->output)
    broadway_output_raise_surface (server->output, surface->id);

  /* Keep the debug-menu toplevels above the one just raised. */
  restack_layers (server);
}

void
broadway_server_set_show_keyboard (BroadwayServer *server,
                                   gboolean show)
{
  server->show_keyboard = show;

  if (server->output)
    {
      broadway_output_set_show_keyboard (server->output, server->show_keyboard);
      broadway_server_flush (server);
    }
}

void
broadway_server_set_clipboard (BroadwayServer *server,
                               const char     *text,
                               gsize           len)
{
  if (server->output)
    {
      broadway_output_set_clipboard (server->output, text, len);
      broadway_server_flush (server);
    }
}

void
broadway_server_request_clipboard (BroadwayServer *server,
                                   guint32         id)
{
  if (server->output)
    {
      broadway_output_request_clipboard (server->output, id);
      broadway_server_flush (server);
    }
}

void
broadway_server_open_uri (BroadwayServer *server,
                          const char     *uri,
                          gsize           len)
{
  if (server->output)
    {
      broadway_output_open_uri (server->output, uri, len);
      broadway_server_flush (server);
    }
}

void
broadway_server_surface_set_cursor (BroadwayServer *server,
                                    int             id,
                                    const char     *name,
                                    gsize           len)
{
  BroadwaySurface *surface;

  /* Remember it for resync: the app side dedups (impl->cursor_name) and
   * won't re-send after a browser reconnect. */
  surface = broadway_server_lookup_surface (server, id);
  if (surface)
    {
      g_free (surface->cursor_name);
      surface->cursor_name = g_strndup (name, len);
    }

  if (server->output)
    {
      broadway_output_set_cursor (server->output, id, name, len);
      broadway_server_flush (server);
    }
}

void
broadway_server_surface_set_title (BroadwayServer *server,
                                   int             id,
                                   const char     *title,
                                   gsize           len)
{
  BroadwaySurface *surface;

  /* Remember it for resync: the app side dedups (impl->title) and won't
   * re-send after a browser reconnect. */
  surface = broadway_server_lookup_surface (server, id);
  if (surface)
    {
      g_free (surface->title);
      surface->title = g_strndup (title, len);
    }

  if (server->output)
    {
      broadway_output_set_title (server->output, id, title, len);
      broadway_server_flush (server);
    }
}

void
broadway_server_surface_set_icon (BroadwayServer *server,
                                  int             id,
                                  const guchar   *data,
                                  gsize           len)
{
  BroadwaySurface *surface;

  /* Remember it for resync, like the cursor/title. len 0 clears it. */
  surface = broadway_server_lookup_surface (server, id);
  if (surface)
    {
      g_clear_pointer (&surface->icon, g_bytes_unref);
      if (len > 0)
        surface->icon = g_bytes_new (data, len);
    }

  if (server->output)
    {
      broadway_output_set_icon (server->output, id, data, len);
      broadway_server_flush (server);
    }
}

void
broadway_server_surface_lower (BroadwayServer *server,
                               int id)
{
  BroadwaySurface *surface;

  surface = broadway_server_lookup_surface (server, id);
  if (surface == NULL)
    return;

  server->surfaces = g_list_remove (server->surfaces, surface);
  server->surfaces = g_list_prepend (server->surfaces, surface);

  if (server->output)
    broadway_output_lower_surface (server->output, surface->id);
}

void
broadway_server_surface_set_transient_for (BroadwayServer *server,
                                           int id, int parent)
{
  BroadwaySurface *surface;

  surface = broadway_server_lookup_surface (server, id);
  if (surface == NULL)
    return;

  surface->transient_for = parent;

  if (server->output)
    {
      broadway_output_set_transient_for (server->output, surface->id, surface->transient_for);
      broadway_server_flush (server);
    }
}

void
broadway_server_surface_set_modal_hint (BroadwayServer *server,
                                        int id, gboolean modal_hint)
{
  BroadwaySurface *surface;

  surface = broadway_server_lookup_surface (server, id);
  if (surface == NULL)
    return;

  surface->modal_hint = modal_hint;

  if (server->output)
    {
      broadway_output_set_modal (server->output, id, modal_hint);
      broadway_server_flush (server);
    }
}

void
broadway_server_surface_set_keep_above (BroadwayServer *server,
                                        int id, gboolean keep_above)
{
  BroadwaySurface *surface;

  surface = broadway_server_lookup_surface (server, id);
  if (surface == NULL || surface->keep_above == keep_above)
    return;

  surface->keep_above = keep_above;

  /* restack_layers re-pins keep-above surfaces and emits the raises. */
  restack_layers (server);
  if (server->output)
    broadway_server_flush (server);
}

void
broadway_server_surface_set_input_region (BroadwayServer *server,
                                          int id, int mode, BroadwayRect *rect)
{
  BroadwaySurface *surface;

  surface = broadway_server_lookup_surface (server, id);
  if (surface == NULL)
    return;

  surface->input_region_mode = mode;
  surface->input_region_is_empty = (mode == 1);
  if (mode == 2)
    surface->input_region_rect = *rect;

  if (server->output)
    broadway_output_set_input_region (server->output, id, mode, rect);
}

gboolean
broadway_server_has_client (BroadwayServer *server)
{
  return server->output != NULL;
}

#define NODE_SIZE_COLOR 1
#define NODE_SIZE_FLOAT 1
#define NODE_SIZE_POINT 2
#define NODE_SIZE_MATRIX 16
#define NODE_SIZE_SIZE 2
#define NODE_SIZE_RECT (NODE_SIZE_POINT + NODE_SIZE_SIZE)
#define NODE_SIZE_RRECT (NODE_SIZE_RECT + 4 * NODE_SIZE_SIZE)
#define NODE_SIZE_COLOR_STOP (NODE_SIZE_FLOAT + NODE_SIZE_COLOR)
#define NODE_SIZE_SHADOW (NODE_SIZE_COLOR + 3 * NODE_SIZE_FLOAT)

static guint32
rotl (guint32 value, int shift)
{
  if ((shift &= 32 - 1) == 0)
    return value;
  return (value << shift) | (value >> (32 - shift));
}

static BroadwayNode *
decode_nodes (BroadwayServer *server,
              BroadwaySurface *surface,
              int len,
              guint32 data[],
              GHashTable  *client_texture_map,
              int *pos)
{
  BroadwayNode *node;
  guint32 type, id;
  guint32 i, n_stops, n_shadows, n_chars;
  guint32 size, n_children;
  gint32 texture_offset;
  guint32 hash;
  guint32 transform_type;

  g_assert (*pos < len);

  size = 0;
  n_children = 0;
  texture_offset = -1;

  type = data[(*pos)++];
  id = data[(*pos)++];
  switch (type) {
  case BROADWAY_NODE_REUSE:
    node = g_hash_table_lookup (surface->node_lookup, GINT_TO_POINTER(id));
    g_assert (node != NULL);
    return broadway_node_ref (node);
    break;
  case BROADWAY_NODE_COLOR:
    size = NODE_SIZE_RECT + NODE_SIZE_COLOR;
    break;
  case BROADWAY_NODE_BORDER:
    size = NODE_SIZE_RRECT + 4 * NODE_SIZE_FLOAT + 4 * NODE_SIZE_COLOR;
    break;
  case BROADWAY_NODE_INSET_SHADOW:
  case BROADWAY_NODE_OUTSET_SHADOW:
    size = NODE_SIZE_RRECT + NODE_SIZE_COLOR + 4 * NODE_SIZE_FLOAT;
    break;
  case BROADWAY_NODE_TEXTURE:
    texture_offset = 4;
    size = 5;
    break;
  case BROADWAY_NODE_CONTAINER:
    size = 1;
    n_children = data[*pos];
    break;
  case BROADWAY_NODE_ROUNDED_CLIP:
    size = NODE_SIZE_RRECT;
    n_children = 1;
    break;
  case BROADWAY_NODE_CLIP:
    size = NODE_SIZE_RECT;
    n_children = 1;
    break;
  case BROADWAY_NODE_TRANSFORM:
    transform_type = data[(*pos)];
    size = 1;
    if (transform_type == 0) {
      size += NODE_SIZE_POINT;
    } else if (transform_type == 1) {
      size += NODE_SIZE_MATRIX;
    } else {
      g_assert_not_reached();
    }
    n_children = 1;
    break;
  case BROADWAY_NODE_LINEAR_GRADIENT:
    size = NODE_SIZE_RECT + 2 * NODE_SIZE_POINT;
    n_stops = data[*pos + size++];
    size += n_stops * NODE_SIZE_COLOR_STOP;
    break;
  case BROADWAY_NODE_SHADOW:
    size = 1;
    n_shadows = data[*pos];
    size += n_shadows * NODE_SIZE_SHADOW;
    n_children = 1;
    break;
  case BROADWAY_NODE_OPACITY:
    size = NODE_SIZE_FLOAT;
    n_children = 1;
    break;
  case BROADWAY_NODE_DEBUG:
    n_chars = data[*pos];
    size = 1 + (n_chars + 3) / 4;
    n_children = 1;
    break;
  default:
    g_assert_not_reached ();
  }

  node = g_malloc (sizeof(BroadwayNode) + (size - 1) * sizeof(guint32) + n_children * sizeof (BroadwayNode *));
  g_ref_count_init (&node->refcount);
  node->type = type;
  node->id = id;
  node->output_id = id;
  node->texture_id = 0;
  node->n_children = n_children;
  node->children = (BroadwayNode **)((char *)node + sizeof(BroadwayNode) + (size - 1) * sizeof(guint32));
  node->n_data = size;
  for (i = 0; i < size; i++)
    node->data[i] = data[(*pos)++];

  /* Only texture nodes carry a texture id; remap it outside the copy loop so
   * the common case skips a per-word comparison. */
  if (texture_offset >= 0)
    {
      node->texture_id = GPOINTER_TO_INT (g_hash_table_lookup (client_texture_map, GINT_TO_POINTER (node->data[texture_offset])));
      broadway_server_ref_texture (server, node->texture_id);
      node->data[texture_offset] = node->texture_id;
    }

  for (i = 0; i < n_children; i++)
    node->children[i] = decode_nodes (server, surface, len, data, client_texture_map, pos);

  hash = node->type << 16;

  for (i = 0; i < size; i++)
    hash ^= rotl (node->data[i], i);

  for (i = 0; i < n_children; i++)
    hash ^= rotl (node->children[i]->hash, i);

  node->hash = hash;

  return node;
}

/* passes ownership of nodes */
void
broadway_server_surface_update_nodes (BroadwayServer   *server,
                                      int               id,
                                      guint32          data[],
                                      int              len,
                                      GHashTable      *client_texture_map)
{
  BroadwaySurface *surface;
  int pos = 0;
  BroadwayNode *root;

  surface = broadway_server_lookup_surface (server, id);
  if (surface == NULL)
    return;

  root = decode_nodes (server, surface, len, data, client_texture_map, &pos);

  if (server->output != NULL)
    broadway_output_surface_set_nodes (server->output, surface->id,
                                       root,
                                       surface->nodes,
                                       surface->node_lookup);

  if (surface->nodes)
    broadway_node_unref (server, surface->nodes);

  surface->nodes = root;

  g_hash_table_remove_all (surface->node_lookup);
  broadway_node_add_to_lookup (root, surface->node_lookup);
}

guint32
broadway_server_upload_texture (BroadwayServer   *server,
                                GBytes           *bytes)
{
  BroadwayTexture *texture;

  texture = g_new0 (BroadwayTexture, 1);
  g_ref_count_init (&texture->refcount);
  texture->id = ++server->next_texture_id;
  texture->bytes = g_bytes_ref (bytes);

  g_hash_table_replace (server->textures,
                        GINT_TO_POINTER (texture->id),
                        texture);
  server->texture_uploads++;

  if (server->output)
    broadway_output_upload_texture (server->output, texture->id, texture->bytes);

  return texture->id;
}

static void
broadway_server_ref_texture (BroadwayServer   *server,
                             guint32           id)
{
  BroadwayTexture *texture;

  texture = g_hash_table_lookup (server->textures, GINT_TO_POINTER (id));
  if (texture)
    g_ref_count_inc (&texture->refcount);
}

void
broadway_server_release_texture (BroadwayServer   *server,
                                 guint32           id)
{
  BroadwayTexture *texture;

  texture = g_hash_table_lookup (server->textures, GINT_TO_POINTER (id));

  if (texture && g_ref_count_dec (&texture->refcount))
    {
      g_hash_table_remove (server->textures, GINT_TO_POINTER (id));
      server->texture_releases++;

      if (server->output)
        broadway_output_release_texture (server->output, id);
    }
}

gboolean
broadway_server_surface_move_resize (BroadwayServer *server,
                                     int id,
                                     gboolean with_move,
                                     int x,
                                     int y,
                                     int width,
                                     int height)
{
  BroadwaySurface *surface;
  gboolean with_resize;
  gboolean sent = FALSE;

  surface = broadway_server_lookup_surface (server, id);
  if (surface == NULL)
    return FALSE;

  with_resize = width != surface->width || height != surface->height;
  surface->width = width;
  surface->height = height;

  if (server->output != NULL)
    {
      broadway_output_move_resize_surface (server->output,
                                           surface->id,
                                           with_move, x, y,
                                           with_resize, surface->width, surface->height);
      sent = TRUE;
    }
  else
    {
      if (with_move)
        {
          surface->x = x;
          surface->y = y;
        }

      fake_configure_notify (server, surface);
    }

  return sent;
}

void
broadway_server_focus_surface (BroadwayServer *server,
                               int new_focused_surface)
{
  BroadwayInputMsg focus_msg;

  if (server->focused_surface_id == new_focused_surface)
    return;

  memset (&focus_msg, 0, sizeof (focus_msg));
  focus_msg.base.type = BROADWAY_EVENT_FOCUS;
  focus_msg.base.time = broadway_server_get_last_seen_time (server);
  focus_msg.focus.old_id = server->focused_surface_id;
  focus_msg.focus.new_id = new_focused_surface;

  broadway_events_got_input (&focus_msg, -1);

  /* Keep track of the new focused surface */
  server->focused_surface_id = new_focused_surface;
}

guint32
broadway_server_grab_pointer (BroadwayServer *server,
                              int client_id,
                              int id,
                              gboolean owner_events,
                              guint32 event_mask,
                              guint32 time_)
{
  BroadwayGrab *top = server->pointer_grabs ? server->pointer_grabs->data : NULL;

  /* Reject a stale (older) grab; a newer one on a new surface nests, a re-grab
   * of the current surface updates the top in place. */
  if (top && time_ != 0 && top->time > time_)
    return GDK_GRAB_ALREADY_GRABBED;

  if (time_ == 0)
    time_ = server->last_seen_time;

  if (top && top->surface_id == id)
    {
      top->client_id = client_id;
      top->owner_events = owner_events;
      top->time = time_;
    }
  else
    {
      BroadwayGrab *g = g_new0 (BroadwayGrab, 1);
      g->surface_id = id;
      g->client_id = client_id;
      g->owner_events = owner_events;
      g->time = time_;
      server->pointer_grabs = g_list_prepend (server->pointer_grabs, g);
    }

  update_grab_cache (server);

  if (server->output)
    {
      broadway_output_grab_pointer (server->output,
                                    id,
                                    owner_events);
      broadway_server_flush (server);
    }

  /* TODO: What about surface grab events if we're not connected? */

  return GDK_GRAB_SUCCESS;
}

guint32
broadway_server_ungrab_pointer (BroadwayServer *server,
                                guint32    time_)
{
  guint32 serial;
  BroadwayGrab *top = server->pointer_grabs ? server->pointer_grabs->data : NULL;

  if (top && time_ != 0 && top->time > time_)
    return 0;

  /* TODO: What about surface grab events if we're not connected? */

  if (server->output)
    {
      serial = broadway_output_ungrab_pointer (server->output);
      broadway_server_flush (server);
    }
  else
    {
      serial = server->saved_serial;
    }

  /* Pop the innermost grab, falling back to its parent (the new top). */
  if (server->pointer_grabs)
    {
      g_free (server->pointer_grabs->data);
      server->pointer_grabs = g_list_delete_link (server->pointer_grabs,
                                                  server->pointer_grabs);
    }
  update_grab_cache (server);

  return serial;
}

guint32
broadway_server_new_surface (BroadwayServer *server,
                             guint32 client,
                             int x,
                             int y,
                             int width,
                             int height,
                             gboolean is_popup)
{
  BroadwaySurface *surface;

  surface = g_new0 (BroadwaySurface, 1);
  surface->owner = client;
  surface->id = server->id_counter++;
  surface->x = x;
  surface->y = y;
  surface->width = width;
  surface->height = height;
  surface->is_popup = is_popup;
  surface->node_lookup = g_hash_table_new (g_direct_hash, g_direct_equal);

  g_hash_table_insert (server->surface_id_hash,
                       GINT_TO_POINTER (surface->id),
                       surface);

  server->surfaces = g_list_append (server->surfaces, surface);

  if (server->output)
    broadway_output_new_surface (server->output,
                                 surface->id,
                                 surface->x,
                                 surface->y,
                                 surface->width,
                                 surface->height);
  else
    fake_configure_notify (server, surface);

  if (server->expecting_menu_surface && !is_popup &&
      client > server->menu_client_floor)
    {
      /* First toplevel from the spawned debug menu: pin its whole client
       * (menu window + gallery + dialogs) above all other surfaces. Only a
       * client that connected after the summon qualifies; client ids are
       * monotonic, so anything at or below the floor predates the spawn. */
      server->expecting_menu_surface = FALSE;
      server->menu_owner = surface->owner;
      if (menu_expect_timer != 0)
        {
          g_source_remove (menu_expect_timer);
          menu_expect_timer = 0;
        }
    }

  /* Keep the menu toplevels on top; a freshly mapped one (e.g. the gallery) was
   * just appended last, so it stacks above the earlier menu windows. */
  restack_layers (server);

  return surface->id;
}

static void
broadway_server_resync_surfaces (BroadwayServer *server)
{
  GHashTableIter iter;
  gpointer key, value;
  GList *l;

  if (server->output == NULL)
    return;

  /* First upload all textures. One giant frame gives the client no onmessage
   * (= no liveness signal) until the whole resync lands, which on a slow link
   * looks like a dead socket; but a flush per texture turns a many-texture
   * resync into one ws frame per texture - hundreds of client macrotasks
   * (Blob/Image/decode each), seconds of blank before the nodes paint. Flush
   * in ~128KB batches: liveness keeps flowing without the per-frame fan-out.
   * The flush can drop the output on a write error, so re-check it. */
  gsize pending = 0;
  g_hash_table_iter_init (&iter, server->textures);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      BroadwayTexture *texture = value;
      broadway_output_upload_texture (server->output,
                                      GPOINTER_TO_INT (key),
                                      texture->bytes);
      pending += g_bytes_get_size (texture->bytes);
      if (pending >= 128 * 1024)
        {
          broadway_server_flush (server);
          if (server->output == NULL)
            return;
          pending = 0;
        }
    }

  /* Then create all surfaces */
  for (l = server->surfaces; l != NULL; l = l->next)
    {
      BroadwaySurface *surface = l->data;

      if (surface->id == 0)
        continue; /* Skip root */

      broadway_output_new_surface (server->output,
                                   surface->id,
                                   surface->x,
                                   surface->y,
                                   surface->width,
                                   surface->height);
    }

  /* Then do everything that may reference other surfaces */
  for (l = server->surfaces; l != NULL; l = l->next)
    {
      BroadwaySurface *surface = l->data;

      if (surface->id == 0)
        continue; /* Skip root */

      if (surface->transient_for != -1)
        broadway_output_set_transient_for (server->output, surface->id,
                                           surface->transient_for);

      if (surface->nodes)
        broadway_output_surface_set_nodes (server->output, surface->id,
                                           surface->nodes,
                                           NULL, NULL);

      if (surface->input_region_mode != 0)
        broadway_output_set_input_region (server->output, surface->id,
                                          surface->input_region_mode,
                                          &surface->input_region_rect);

      if (surface->cursor_name)
        broadway_output_set_cursor (server->output, surface->id,
                                    surface->cursor_name,
                                    strlen (surface->cursor_name));

      if (surface->title)
        broadway_output_set_title (server->output, surface->id,
                                   surface->title,
                                   strlen (surface->title));

      if (surface->icon)
        {
          gsize ilen;
          const guchar *idata = g_bytes_get_data (surface->icon, &ilen);
          broadway_output_set_icon (server->output, surface->id, idata, ilen);
        }

      if (surface->modal_hint)
        broadway_output_set_modal (server->output, surface->id, TRUE);

      if (surface->visible)
        broadway_output_show_surface (server->output, surface->id);
    }

  if (server->show_keyboard)
    broadway_output_set_show_keyboard (server->output, TRUE);

  broadway_server_flush (server);
}
