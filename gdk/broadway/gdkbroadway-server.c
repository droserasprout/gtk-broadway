#include "config.h"

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>

#include "gdkbroadway-server.h"

#include "gdkprivate-broadway.h"
#include "gdkprivate.h"

#include <gdk/gdktextureprivate.h>
#include "loaders/gdkpngprivate.h"

#include <png.h>
#include <zlib.h>   /* Z_RLE / Z_FILTERED / ... for png_set_compression_strategy */

#include <glib.h>
#include <glib/gprintf.h>
#include <gio/gunixsocketaddress.h>
#include <gio/gunixfdmessage.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#elif defined (G_OS_WIN32)
#include <io.h>
#define ftruncate _chsize_s
#endif
#include <sys/types.h>
#ifdef G_OS_WIN32
#include <windows.h>
#endif
#include <glib/gi18n-lib.h>

typedef struct BroadwayInput BroadwayInput;

struct _GdkBroadwayServer {
  GObject parent_instance;
  GdkDisplay *display;

  guint32 next_serial;
  guint32 next_texture_id;
  GSocketConnection *connection;

  guint32 recv_buffer_size;
  guint32 recv_buffer_capacity;
  guint8 *recv_buffer;

  guint process_input_idle;
  GList *incoming;
};

#define RECV_BUFFER_INITIAL_SIZE 1024

struct _GdkBroadwayServerClass
{
  GObjectClass parent_class;
};

static gboolean input_available_cb (gpointer stream, gpointer user_data);

static GType gdk_broadway_server_get_type (void);

G_DEFINE_TYPE (GdkBroadwayServer, gdk_broadway_server, G_TYPE_OBJECT)

static void
gdk_broadway_server_init (GdkBroadwayServer *server)
{
  server->next_serial = 1;
  server->next_texture_id = 1;
  server->recv_buffer_capacity = RECV_BUFFER_INITIAL_SIZE;
  server->recv_buffer = g_malloc (server->recv_buffer_capacity);
}

static void
gdk_broadway_server_finalize (GObject *object)
{
  GdkBroadwayServer *server = GDK_BROADWAY_SERVER (object);

  g_free (server->recv_buffer);

  G_OBJECT_CLASS (gdk_broadway_server_parent_class)->finalize (object);
}

static void
gdk_broadway_server_class_init (GdkBroadwayServerClass * class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = gdk_broadway_server_finalize;
}

gboolean
_gdk_broadway_server_lookahead_event (GdkBroadwayServer  *server,
                                      const char         *types)
{
  return FALSE;
}

gulong
_gdk_broadway_server_get_next_serial (GdkBroadwayServer *server)
{
  return (gulong)server->next_serial;
}

GdkBroadwayServer *
_gdk_broadway_server_new (GdkDisplay *display,
                          const char *display_name,
                          GError **error)
{
  GdkBroadwayServer *server;
  GSocketClient *client;
  GSocketConnection *connection;
  GSocketAddress *address;
  GPollableInputStream *pollable;
  GInputStream *in;
  GSource *source;
  char *local_socket_type = NULL;
  int port;

  if (display_name == NULL)
    display_name = ":0";

  if (display_name[0] == ':' && g_ascii_isdigit(display_name[1]))
    {
      char *path, *basename;

      port = strtol (display_name + strlen (":"), NULL, 10);
      basename = g_strdup_printf ("broadway%d.socket", port + 1);
      path = g_build_filename (g_get_user_runtime_dir (), basename, NULL);
      g_free (basename);

      address = g_unix_socket_address_new_with_type (path, -1,
                                                     G_UNIX_SOCKET_ADDRESS_PATH);
      g_free (path);
    }
  else
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                   _("Broadway display type not supported: %s"), display_name);
      return NULL;
    }

  g_free (local_socket_type);

  client = g_socket_client_new ();

  connection = g_socket_client_connect (client, G_SOCKET_CONNECTABLE (address), NULL, error);

  g_object_unref (address);
  g_object_unref (client);

  if (connection == NULL)
    return NULL;

  server = g_object_new (GDK_TYPE_BROADWAY_SERVER, NULL);
  server->connection = connection;
  server->display = display;

  in = g_io_stream_get_input_stream (G_IO_STREAM (server->connection));
  pollable = G_POLLABLE_INPUT_STREAM (in);

  source = g_pollable_input_stream_create_source (pollable, NULL);
  g_source_attach (source, NULL);
  g_source_set_callback (source, (GSourceFunc)input_available_cb, server, NULL);

  return server;
}

static guint32
gdk_broadway_server_send_message_with_size (GdkBroadwayServer *server, BroadwayRequestBase *base,
                                            gsize size, guint32 type, int fd)
{
  GOutputStream *out;
  gsize written;
  guchar *buf;

  base->size = size;
  base->type = type;
  base->serial = server->next_serial++;

  buf = (guchar *)base;

  if (fd != -1)
    {
      GUnixFDList *fd_list = g_unix_fd_list_new_from_array (&fd, 1);
      GSocketControlMessage *control_message = g_unix_fd_message_new_with_fd_list (fd_list);
      GSocket *socket = g_socket_connection_get_socket (server->connection);
      GOutputVector vector;
      gssize bytes_written;

      vector.buffer = buf;
      vector.size = size;

      bytes_written = g_socket_send_message (socket,
                                             NULL, /* address */
                                             &vector,
                                             1,
                                             &control_message, 1,
                                             G_SOCKET_MSG_NONE,
                                             NULL,
                                             NULL);

      if (bytes_written <= 0)
        {
          g_printerr ("Unable to write to server\n");
          exit (1);
        }

      buf += bytes_written;
      size -= bytes_written;

      g_object_unref (control_message);
      g_object_unref (fd_list);
    }

  if (size > 0)
    {
      out = g_io_stream_get_output_stream (G_IO_STREAM (server->connection));
      if (!g_output_stream_write_all (out, buf, size, &written, NULL, NULL))
        {
          g_printerr ("Unable to write to server\n");
          exit (1);
        }

      g_assert (written == size);
    }


  return base->serial;
}

#define gdk_broadway_server_send_message(_server, _msg, _type) \
  gdk_broadway_server_send_message_with_size(_server, (BroadwayRequestBase *)&_msg, sizeof (_msg), _type, -1)

#define gdk_broadway_server_send_fd_message(_server, _msg, _type, _fd)      \
  gdk_broadway_server_send_message_with_size(_server, (BroadwayRequestBase *)&_msg, sizeof (_msg), _type, _fd)

/* Grow the receive buffer so it can hold at least @needed bytes. Clipboard
 * replies are variable-sized and can exceed the initial buffer; without this a
 * reply larger than the buffer could never be assembled. */
static void
ensure_recv_capacity (GdkBroadwayServer *server, guint32 needed)
{
  guint32 capacity = server->recv_buffer_capacity;

  if (needed <= capacity)
    return;

  /* Double, but stop before guint32 overflows (which would wrap to 0 and spin
   * forever / under-allocate); fall back to the exact size for the last step. */
  while (capacity < needed && capacity <= G_MAXUINT32 / 2)
    capacity *= 2;
  if (capacity < needed)
    capacity = needed;

  server->recv_buffer = g_realloc (server->recv_buffer, capacity);
  server->recv_buffer_capacity = capacity;
}

static void
parse_all_input (GdkBroadwayServer *server)
{
  guint8 *p, *end;
  guint32 size;
  guint32 incomplete_size = 0;
  BroadwayReply *reply;

  p = server->recv_buffer;
  end = p + server->recv_buffer_size;

  while (p + sizeof (guint32) <= end)
    {
      memcpy (&size, p, sizeof (guint32));
      if (p + size > end)
        {
          /* Incomplete message; remember its full size so we can grow the
           * buffer below to make room for the rest of it. */
          incomplete_size = size;
          break;
        }

      reply = g_memdup2 (p, size);
      p += size;

      server->incoming = g_list_append (server->incoming, reply);
    }

  if (p < end && p != server->recv_buffer)
    memmove (server->recv_buffer, p, end - p);
  server->recv_buffer_size = end - p;

  /* Done with p/end (into the old buffer) before any realloc that may move it. */
  if (incomplete_size > 0)
    {
      /* The largest legitimate frame is a clipboard reply (text capped at
       * BROADWAY_CLIPBOARD_MAX_SIZE) plus a small header. A larger claimed size
       * means a corrupt/hostile stream; bail rather than try to allocate it. */
      if (incomplete_size > BROADWAY_CLIPBOARD_MAX_SIZE + 1024)
        {
          g_printerr ("Broadway server sent an oversized message (%u bytes)\n",
                      incomplete_size);
          exit (1);
        }
      ensure_recv_capacity (server, incomplete_size);
    }
}

static void
read_some_input_blocking (GdkBroadwayServer *server)
{
  GInputStream *in;
  gssize res;

  in = g_io_stream_get_input_stream (G_IO_STREAM (server->connection));

  if (server->recv_buffer_size == server->recv_buffer_capacity)
    ensure_recv_capacity (server, server->recv_buffer_capacity + 1);
  res = g_input_stream_read (in, &server->recv_buffer[server->recv_buffer_size],
                             server->recv_buffer_capacity - server->recv_buffer_size,
                             NULL, NULL);

  if (res <= 0)
    {
      g_printerr ("Unable to read from broadway server\n");
      exit (1);
    }

  server->recv_buffer_size += res;
}

static void
read_some_input_nonblocking (GdkBroadwayServer *server)
{
  GInputStream *in;
  GPollableInputStream *pollable;
  gssize res;
  GError *error;

  in = g_io_stream_get_input_stream (G_IO_STREAM (server->connection));
  pollable = G_POLLABLE_INPUT_STREAM (in);

  if (server->recv_buffer_size == server->recv_buffer_capacity)
    ensure_recv_capacity (server, server->recv_buffer_capacity + 1);
  error = NULL;
  res = g_pollable_input_stream_read_nonblocking (pollable, &server->recv_buffer[server->recv_buffer_size],
                                                  server->recv_buffer_capacity - server->recv_buffer_size,
                                                  NULL, &error);

  if (res < 0 && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK))
    {
      g_error_free (error);
      res = 0;
    }
  else if (res <= 0)
    {
      g_printerr ("Unable to read from broadway server: %s\n", error ? error->message : "eof");
      exit (1);
    }

  server->recv_buffer_size += res;
}

static BroadwayReply *
find_response_by_serial (GdkBroadwayServer *server, guint32 serial)
{
  GList *l;

  for (l = server->incoming; l != NULL; l = l->next)
    {
      BroadwayReply *reply = l->data;

      if (reply->base.in_reply_to == serial)
        return reply;
    }

  return NULL;
}

static void
process_input_messages (GdkBroadwayServer *server)
{
  BroadwayReply *reply;

  if (server->process_input_idle != 0)
    {
      g_source_remove (server->process_input_idle);
      server->process_input_idle = 0;
    }

  while (server->incoming)
    {
      reply = server->incoming->data;
      server->incoming =
        g_list_delete_link (server->incoming,
                            server->incoming);

      if (reply->base.type == BROADWAY_REPLY_EVENT)
        _gdk_broadway_events_got_input (server->display, &reply->event.msg);
      else if (reply->base.type == BROADWAY_REPLY_CLIPBOARD &&
               reply->base.size >= G_STRUCT_OFFSET (BroadwayReplyClipboard, text))
        {
          /* Don't trust the wire len: clamp to what the framed reply carries
           * so the text read never runs past the allocation. A size below the
           * fixed header would wrap the clamp. */
          gsize max = reply->base.size -
                      G_STRUCT_OFFSET (BroadwayReplyClipboard, text);
          guint32 len = reply->clipboard.len > max
                        ? (guint32) max : reply->clipboard.len;
          _gdk_broadway_clipboard_contents_received (server->display,
                                                     reply->base.in_reply_to,
                                                     reply->clipboard.text,
                                                     len);
        }
      else
        g_warning ("Unhandled reply type %d", reply->base.type);
      g_free (reply);
    }
}

static gboolean
process_input_idle_cb (GdkBroadwayServer *server)
{
  server->process_input_idle = 0;
  process_input_messages (server);
  return G_SOURCE_REMOVE;
}

static void
queue_process_input_at_idle (GdkBroadwayServer *server)
{
  if (server->process_input_idle == 0)
    server->process_input_idle =
      g_idle_add_full (G_PRIORITY_DEFAULT, (GSourceFunc)process_input_idle_cb, server, NULL);
}

static gboolean
input_available_cb (gpointer stream, gpointer user_data)
{
  GdkBroadwayServer *server = user_data;

  read_some_input_nonblocking (server);
  parse_all_input (server);

  process_input_messages (server);

  return G_SOURCE_CONTINUE;
}

static BroadwayReply *
gdk_broadway_server_wait_for_reply (GdkBroadwayServer *server,
                                    guint32 serial)
{
  BroadwayReply *reply;

  while (TRUE)
    {
      reply = find_response_by_serial (server, serial);
      if (reply)
        {
          server->incoming = g_list_remove (server->incoming, reply);
          break;
        }

      read_some_input_blocking (server);
      parse_all_input (server);
    }

  queue_process_input_at_idle (server);
  return reply;
}

void
_gdk_broadway_server_flush (GdkBroadwayServer *server)
{
  BroadwayRequestFlush msg;

  gdk_broadway_server_send_message (server, msg, BROADWAY_REQUEST_FLUSH);
}

void
_gdk_broadway_server_sync (GdkBroadwayServer *server)
{
  BroadwayRequestSync msg;
  guint32 serial;
  BroadwayReply *reply;

  serial = gdk_broadway_server_send_message (server, msg,
                                             BROADWAY_REQUEST_SYNC);
  reply = gdk_broadway_server_wait_for_reply (server, serial);

  g_assert (reply->base.type == BROADWAY_REPLY_SYNC);

  g_free (reply);

  return;
}

void
_gdk_broadway_server_roundtrip (GdkBroadwayServer *server,
                                gint32            id,
                                guint32           tag)
{
  BroadwayRequestRoundtrip msg;

  msg.id = id;
  msg.tag = tag;
  gdk_broadway_server_send_message (server, msg,
                                    BROADWAY_REQUEST_ROUNDTRIP);
}

void
_gdk_broadway_server_query_mouse (GdkBroadwayServer *server,
                                  guint32            *surface,
                                  gint32             *root_x,
                                  gint32             *root_y,
                                  guint32            *mask)
{
  BroadwayRequestQueryMouse msg;
  guint32 serial;
  BroadwayReply *reply;

  serial = gdk_broadway_server_send_message (server, msg,
                                             BROADWAY_REQUEST_QUERY_MOUSE);
  reply = gdk_broadway_server_wait_for_reply (server, serial);

  g_assert (reply->base.type == BROADWAY_REPLY_QUERY_MOUSE);

  if (surface)
    *surface = reply->query_mouse.surface;
  if (root_x)
    *root_x = reply->query_mouse.root_x;
  if (root_y)
    *root_y = reply->query_mouse.root_y;
  if (mask)
    *mask = reply->query_mouse.mask;

  g_free (reply);
}

guint32
_gdk_broadway_server_new_surface (GdkBroadwayServer *server,
                                 int x,
                                 int y,
                                 int width,
                                 int height,
                                 gboolean is_popup)
{
  BroadwayRequestNewSurface msg;
  guint32 serial, id;
  BroadwayReply *reply;

  msg.x = x;
  msg.y = y;
  msg.width = width;
  msg.height = height;
  msg.is_popup = is_popup;
  serial = gdk_broadway_server_send_message (server, msg,
                                             BROADWAY_REQUEST_NEW_SURFACE);
  reply = gdk_broadway_server_wait_for_reply (server, serial);

  g_assert (reply->base.type == BROADWAY_REPLY_NEW_SURFACE);

  id = reply->new_surface.id;

  g_free (reply);

  return id;
}

void
_gdk_broadway_server_destroy_surface (GdkBroadwayServer *server,
                                     int id)
{
  BroadwayRequestDestroySurface msg;

  msg.id = id;
  gdk_broadway_server_send_message (server, msg,
                                    BROADWAY_REQUEST_DESTROY_SURFACE);
}

gboolean
_gdk_broadway_server_surface_show (GdkBroadwayServer *server,
                                  int id)
{
  BroadwayRequestShowSurface msg;

  msg.id = id;
  gdk_broadway_server_send_message (server, msg,
                                    BROADWAY_REQUEST_SHOW_SURFACE);

  return TRUE;
}

gboolean
_gdk_broadway_server_surface_hide (GdkBroadwayServer *server,
                                  int id)
{
  BroadwayRequestHideSurface msg;

  msg.id = id;
  gdk_broadway_server_send_message (server, msg,
                                    BROADWAY_REQUEST_HIDE_SURFACE);

  return TRUE;
}

void
_gdk_broadway_server_surface_focus (GdkBroadwayServer *server,
                                   int id)
{
  BroadwayRequestFocusSurface msg;

  msg.id = id;
  gdk_broadway_server_send_message (server, msg,
                                    BROADWAY_REQUEST_FOCUS_SURFACE);
}

void
_gdk_broadway_server_surface_set_transient_for (GdkBroadwayServer *server,
                                               int id, int parent)
{
  BroadwayRequestSetTransientFor msg;

  msg.id = id;
  msg.parent = parent;
  gdk_broadway_server_send_message (server, msg,
                                    BROADWAY_REQUEST_SET_TRANSIENT_FOR);
}

void
_gdk_broadway_server_surface_set_modal_hint (GdkBroadwayServer *server,
                                             int id, gboolean modal_hint)
{
  BroadwayRequestSetModalHint msg;

  msg.id = id;
  msg.modal_hint = modal_hint;
  gdk_broadway_server_send_message (server, msg,
				    BROADWAY_REQUEST_SET_MODAL_HINT);
}

void
_gdk_broadway_server_surface_set_input_region (GdkBroadwayServer *server,
                                               int id, int mode,
                                               int x, int y, int width, int height)
{
  BroadwayRequestSetInputRegion msg;

  msg.id = id;
  msg.mode = mode;
  msg.rect.x = x;
  msg.rect.y = y;
  msg.rect.width = width;
  msg.rect.height = height;
  gdk_broadway_server_send_message (server, msg,
				    BROADWAY_REQUEST_SET_INPUT_REGION);
}

static int
open_shared_memory (void)
{
  static gboolean force_shm_open = FALSE;
  int ret = -1;

#if !defined (HAVE_MEMFD_CREATE)
  force_shm_open = TRUE;
#endif

  do
    {
#if defined (HAVE_MEMFD_CREATE)
      if (!force_shm_open)
        {
          ret = memfd_create ("gdk-broadway", MFD_CLOEXEC);

          /* fall back to shm_open until debian stops shipping 3.16 kernel
           * See bug 766341
           */
          if (ret < 0 && errno == ENOSYS)
            force_shm_open = TRUE;
        }
#endif

      if (force_shm_open)
        {
          char name[NAME_MAX - 1] = "";

          sprintf (name, "/gdk-broadway-%x", g_random_int ());

          ret = shm_open (name, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0600);

          if (ret >= 0)
            shm_unlink (name);
          else if (errno == EEXIST)
            continue;
        }
    }
  while (ret < 0 && errno == EINTR);

  if (ret < 0)
    g_critical (G_STRLOC ": creating shared memory file (using %s) failed: %m",
                force_shm_open? "shm_open" : "memfd_create");

  return ret;
}

/* Per-frame PNG re-encode trades encode CPU (latency) against frame size - one
 * axis, two presets:
 *   FAST    - low zlib level, adaptive filter: localhost/LAN (default).
 *   COMPACT - higher level, adaptive filter: remote/metered.
 * Seeded from BROADWAY_PNG, switchable live from the debug menu.
 *
 * Both keep adaptive filtering: SUB+RLE bloats AA text/gradients ~50%+ (measured
 * on real treeview textures), shifting cost to client transfer+decode and hurting
 * scroll smoothness. Adaptive filter is what keeps these small; the level is the
 * cheap CPU knob, so FAST drops only the level. */
typedef struct {
  int level;     /* zlib level 0-9 */
  int filter;    /* libpng filter mask */
  int strategy;  /* zlib strategy */
} BroadwayPngPresetDef;

static const BroadwayPngPresetDef png_presets[] = {
  [BROADWAY_PNG_FAST]    = { 3, PNG_ALL_FILTERS, Z_DEFAULT_STRATEGY },
  [BROADWAY_PNG_COMPACT] = { 7, PNG_ALL_FILTERS, Z_DEFAULT_STRATEGY },
};

static int png_preset = BROADWAY_PNG_FAST;
static gsize png_preset_init = 0;

static int
broadway_png_preset (void)
{
  if (g_once_init_enter (&png_preset_init))
    {
      const char *v = g_getenv ("BROADWAY_PNG");

      if (v && *v)
        {
          if (g_ascii_strcasecmp (v, "fast") == 0)         png_preset = BROADWAY_PNG_FAST;
          else if (g_ascii_strcasecmp (v, "compact") == 0) png_preset = BROADWAY_PNG_COMPACT;
          else g_warning ("BROADWAY_PNG: unknown '%s' (fast|compact)", v);
        }
      g_once_init_leave (&png_preset_init, 1);
    }
  return png_preset;
}

/* Debug-menu switch; out-of-range ids ignored. */
void
_gdk_broadway_server_set_png_preset (int preset)
{
  (void) broadway_png_preset (); /* run env seed first */

  if (preset >= 0 && preset < (int) G_N_ELEMENTS (png_presets))
    {
      png_preset = preset;
      g_message ("broadway: PNG preset -> %s",
                 preset == BROADWAY_PNG_COMPACT ? "compact" : "fast");
    }
}

guint32
gdk_broadway_server_upload_texture (GdkBroadwayServer *server,
                                    GdkTexture        *texture)
{
  guint32 id;
  BroadwayRequestUploadTexture msg;
  GBytes *bytes;
  const guchar *data;
  gsize size;
  int fd;
  const BroadwayPngPresetDef *png_cfg = &png_presets[broadway_png_preset ()];

  bytes = gdk_save_png_full (texture, NULL,
                             png_cfg->level, png_cfg->filter, png_cfg->strategy);
  fd = open_shared_memory ();
  data = g_bytes_get_data (bytes, &size);

  id = server->next_texture_id++;

  msg.id = id;
  msg.offset = 0;
  msg.size = 0;

  while (msg.size < size)
    {
      gssize ret = write (fd, data + msg.size, size - msg.size);

      if (ret <= 0)
        {
          if (errno == EINTR)
            continue;
          break;
        }

      msg.size += ret;
    }

  g_bytes_unref (bytes);

  /* This passes ownership of fd */
  gdk_broadway_server_send_fd_message (server, msg,
                                       BROADWAY_REQUEST_UPLOAD_TEXTURE, fd);

  return id;
}


void
gdk_broadway_server_release_texture (GdkBroadwayServer *server,
                                     guint32            id)
{
  BroadwayRequestReleaseTexture msg;

  msg.id = id;

  gdk_broadway_server_send_message (server, msg,
                                    BROADWAY_REQUEST_RELEASE_TEXTURE);
}

void
gdk_broadway_server_surface_set_nodes (GdkBroadwayServer *server,
                                      guint32 id,
                                      GArray *nodes)
{
  gsize size = sizeof(BroadwayRequestSetNodes) + sizeof(guint32) * (nodes->len - 1);
  BroadwayRequestSetNodes *msg = g_alloca (size);
  int i;

  for (i = 0; i < nodes->len; i++)
    msg->data[i] = g_array_index (nodes, guint32, i);

  msg->id = id;
  gdk_broadway_server_send_message_with_size (server, (BroadwayRequestBase *) msg, size, BROADWAY_REQUEST_SET_NODES, -1);
}

gboolean
_gdk_broadway_server_surface_move_resize (GdkBroadwayServer *server,
                                         int id,
                                         gboolean with_move,
                                         int x,
                                         int y,
                                         int width,
                                         int height)
{
  BroadwayRequestMoveResize msg;

  msg.id = id;
  msg.with_move = with_move;
  msg.x = x;
  msg.y = y;
  msg.width = width;
  msg.height = height;

  gdk_broadway_server_send_message (server, msg,
                                    BROADWAY_REQUEST_MOVE_RESIZE);

  return TRUE;
}

GdkGrabStatus
_gdk_broadway_server_grab_pointer (GdkBroadwayServer *server,
                                   int id,
                                   gboolean owner_events,
                                   guint32 event_mask,
                                   guint32 time_)
{
  BroadwayRequestGrabPointer msg;
  guint32 serial, status;
  BroadwayReply *reply;

  msg.id = id;
  msg.owner_events = owner_events;
  msg.event_mask = event_mask;
  msg.time_ = time_;

  serial = gdk_broadway_server_send_message (server, msg,
                                             BROADWAY_REQUEST_GRAB_POINTER);
  reply = gdk_broadway_server_wait_for_reply (server, serial);

  g_assert (reply->base.type == BROADWAY_REPLY_GRAB_POINTER);

  status = reply->grab_pointer.status;

  g_free (reply);

  return status;
}

guint32
_gdk_broadway_server_ungrab_pointer (GdkBroadwayServer *server,
                                     guint32    time_)
{
  BroadwayRequestUngrabPointer msg;
  guint32 serial, status;
  BroadwayReply *reply;

  msg.time_ = time_;

  serial = gdk_broadway_server_send_message (server, msg,
                                             BROADWAY_REQUEST_UNGRAB_POINTER);
  reply = gdk_broadway_server_wait_for_reply (server, serial);

  g_assert (reply->base.type == BROADWAY_REPLY_UNGRAB_POINTER);

  status = reply->ungrab_pointer.status;

  g_free (reply);

  return status;
}

void
_gdk_broadway_server_set_show_keyboard (GdkBroadwayServer *server,
                                        gboolean show)
{
  BroadwayRequestSetShowKeyboard msg;

  msg.show_keyboard = show;
  gdk_broadway_server_send_message (server, msg,
                                    BROADWAY_REQUEST_SET_SHOW_KEYBOARD);
}

void
_gdk_broadway_server_set_clipboard_text (GdkBroadwayServer *server,
                                         const char        *text)
{
  gsize len = text ? strlen (text) : 0;
  gsize size;
  BroadwayRequestSetClipboard *msg;

  if (len > BROADWAY_CLIPBOARD_MAX_SIZE)
    len = BROADWAY_CLIPBOARD_MAX_SIZE;

  size = G_STRUCT_OFFSET (BroadwayRequestSetClipboard, text) + len;
  /* Pad to 4 bytes (zero-filled) so the daemon's framing loop keeps the
   * following requests in the stream aligned. */
  size = (size + 3) & ~(gsize) 3;
  /* Allocate at least the full struct so accessing msg->len stays in bounds
   * (the text[1] member makes sizeof larger than size when len is small). */
  msg = g_malloc0 (MAX (size, sizeof *msg));

  msg->len = (guint32) len;
  if (len > 0)
    memcpy (msg->text, text, len);

  gdk_broadway_server_send_message_with_size (server, (BroadwayRequestBase *) msg,
                                              size, BROADWAY_REQUEST_SET_CLIPBOARD, -1);
  g_free (msg);
}

guint32
_gdk_broadway_server_request_clipboard (GdkBroadwayServer *server)
{
  BroadwayRequestBase msg;
  return gdk_broadway_server_send_message_with_size (server, &msg, sizeof (msg),
                                                     BROADWAY_REQUEST_REQUEST_CLIPBOARD, -1);
}

void
_gdk_broadway_server_open_uri (GdkBroadwayServer *server,
                               const char        *uri)
{
  gsize len = uri ? strlen (uri) : 0;
  gsize size;
  BroadwayRequestOpenUri *msg;

  if (len == 0)
    return;

  if (len > BROADWAY_CLIPBOARD_MAX_SIZE)
    len = BROADWAY_CLIPBOARD_MAX_SIZE;

  size = G_STRUCT_OFFSET (BroadwayRequestOpenUri, uri) + len;
  /* Pad to 4 bytes (zero-filled) to keep the daemon's request stream aligned. */
  size = (size + 3) & ~(gsize) 3;
  /* Allocate at least the full struct so accessing msg->len stays in bounds
   * (the uri[1] member makes sizeof larger than size when len is small). */
  msg = g_malloc0 (MAX (size, sizeof *msg));

  msg->len = (guint32) len;
  memcpy (msg->uri, uri, len);

  gdk_broadway_server_send_message_with_size (server, (BroadwayRequestBase *) msg,
                                              size, BROADWAY_REQUEST_OPEN_URI, -1);
  g_free (msg);
}

void
_gdk_broadway_server_surface_set_cursor (GdkBroadwayServer *server,
                                         int                id,
                                         const char        *name)
{
  gsize len = name ? strlen (name) : 0;
  gsize size;
  BroadwayRequestSetCursor *msg;

  size = G_STRUCT_OFFSET (BroadwayRequestSetCursor, name) + len;
  /* Pad to 4 bytes (zero-filled) to keep the daemon's request stream aligned. */
  size = (size + 3) & ~(gsize) 3;
  /* Allocate at least the full struct so accessing msg->len stays in bounds
   * (the name[1] member makes sizeof larger than size when len is small). */
  msg = g_malloc0 (MAX (size, sizeof *msg));

  msg->id = id;
  msg->len = (guint32) len;
  if (len > 0)
    memcpy (msg->name, name, len);

  gdk_broadway_server_send_message_with_size (server, (BroadwayRequestBase *) msg,
                                              size, BROADWAY_REQUEST_SET_CURSOR, -1);
  g_free (msg);
}

void
_gdk_broadway_server_surface_set_title (GdkBroadwayServer *server,
                                        int                id,
                                        const char        *title)
{
  gsize len = title ? strlen (title) : 0;
  gsize size;
  BroadwayRequestSetTitle *msg;

  if (len > BROADWAY_CLIPBOARD_MAX_SIZE)
    len = BROADWAY_CLIPBOARD_MAX_SIZE;

  size = G_STRUCT_OFFSET (BroadwayRequestSetTitle, title) + len;
  /* Pad to 4 bytes (zero-filled) to keep the daemon's request stream aligned. */
  size = (size + 3) & ~(gsize) 3;
  /* Allocate at least the full struct so accessing msg->len stays in bounds
   * (the title[1] member makes sizeof larger than size when len is small). */
  msg = g_malloc0 (MAX (size, sizeof *msg));

  msg->id = id;
  msg->len = (guint32) len;
  if (len > 0)
    memcpy (msg->title, title, len);

  gdk_broadway_server_send_message_with_size (server, (BroadwayRequestBase *) msg,
                                              size, BROADWAY_REQUEST_SET_TITLE, -1);
  g_free (msg);
}

void
_gdk_broadway_server_surface_set_icon (GdkBroadwayServer *server,
                                       int                id,
                                       const guchar      *data,
                                       gsize              len)
{
  gsize size;
  BroadwayRequestSetIcon *msg;

  if (len > BROADWAY_CLIPBOARD_MAX_SIZE)
    len = BROADWAY_CLIPBOARD_MAX_SIZE;

  size = G_STRUCT_OFFSET (BroadwayRequestSetIcon, data) + len;
  /* Pad to 4 bytes (zero-filled) to keep the daemon's request stream aligned. */
  size = (size + 3) & ~(gsize) 3;
  /* Allocate at least the full struct so accessing msg->len stays in bounds
   * (the data[1] member makes sizeof larger than size when len is small). */
  msg = g_malloc0 (MAX (size, sizeof *msg));

  msg->id = id;
  msg->len = (guint32) len;
  if (len > 0)
    memcpy (msg->data, data, len);

  gdk_broadway_server_send_message_with_size (server, (BroadwayRequestBase *) msg,
                                              size, BROADWAY_REQUEST_SET_ICON, -1);
  g_free (msg);
}
