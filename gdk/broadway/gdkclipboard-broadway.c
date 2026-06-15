/* gdkclipboard-broadway.c
 *
 * Text clipboard for the GTK Broadway backend.
 * Bridges GdkClipboard <-> gtk4-broadwayd <-> browser navigator.clipboard.
 */

#include "config.h"

#include "gdkprivate-broadway.h"
#include "gdkdisplay-broadway.h"

#include "gdkclipboardprivate.h"
#include "gdkcontentformats.h"
#include "gdkcontentprovider.h"

#include <gio/gio.h>
#include <string.h>

#define TEXT_MIME "text/plain;charset=utf-8"

/* How long to wait for the browser to answer a paste request before giving up,
 * so a read never hangs forever if the tab closed or no browser is connected. */
#define CLIPBOARD_READ_TIMEOUT_SECONDS 5

typedef struct _GdkBroadwayClipboard       GdkBroadwayClipboard;
typedef struct _GdkBroadwayClipboardClass  GdkBroadwayClipboardClass;

#define GDK_TYPE_BROADWAY_CLIPBOARD  (gdk_broadway_clipboard_get_type ())
#define GDK_BROADWAY_CLIPBOARD(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GDK_TYPE_BROADWAY_CLIPBOARD, GdkBroadwayClipboard))
#define GDK_IS_BROADWAY_CLIPBOARD(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDK_TYPE_BROADWAY_CLIPBOARD))

struct _GdkBroadwayClipboard
{
  GdkClipboard parent;
  GTask        *pending_read;          /* outstanding read_async, or NULL */
  guint32       pending_serial;        /* serial of the request pending_read sent */
  GCancellable *pending_cancellable;   /* cancellable of pending_read (reffed), or NULL */
  gulong        pending_cancelled_id;  /* "cancelled" handler id on pending_cancellable, or 0 */
  guint         read_timeout_id;       /* timeout source guarding pending_read, or 0 */
  guint         reclaim_idle;
};

struct _GdkBroadwayClipboardClass
{
  GdkClipboardClass parent_class;
};

GType gdk_broadway_clipboard_get_type (void);
G_DEFINE_TYPE (GdkBroadwayClipboard, gdk_broadway_clipboard, GDK_TYPE_CLIPBOARD)

static GdkContentFormats *
text_formats (void)
{
  /* The advertised text formats never change, so build them once and hand out
   * refs to the cached instance. */
  static GdkContentFormats *formats = NULL;

  if (formats == NULL)
    {
      GdkContentFormatsBuilder *builder = gdk_content_formats_builder_new ();
      gdk_content_formats_builder_add_mime_type (builder, TEXT_MIME);
      gdk_content_formats_builder_add_mime_type (builder, "text/plain");
      formats = gdk_content_formats_builder_free_to_formats (builder);
    }

  return gdk_content_formats_ref (formats);
}

static GdkBroadwayServer *
get_server (GdkClipboard *clipboard)
{
  GdkDisplay *display = gdk_clipboard_get_display (clipboard);
  return GDK_BROADWAY_DISPLAY (display)->server;
}

static gboolean
reclaim_remote_cb (gpointer data)
{
  GdkBroadwayClipboard *cb = data;
  GdkContentFormats *formats = text_formats ();

  cb->reclaim_idle = 0;
  gdk_clipboard_claim_remote (GDK_CLIPBOARD (cb), formats);
  gdk_content_formats_unref (formats);
  return G_SOURCE_REMOVE;
}

/* Return to remote mode so future pastes fetch the live host clipboard. */
static void
schedule_reclaim_remote (GdkBroadwayClipboard *cb)
{
  if (cb->reclaim_idle == 0)
    cb->reclaim_idle = g_idle_add (reclaim_remote_cb, cb);
}

/* Fallback for content providers that can't yield a G_TYPE_STRING (e.g. a
 * GtkTextView/GtkTextBuffer selection): read the text via the local clipboard's
 * text/plain serialization. The clipboard is local here, so this serializes
 * locally and does not hit our remote read_async. */
static void
gdk_broadway_clipboard_local_text_read (GObject      *source,
                                        GAsyncResult *result,
                                        gpointer      data)
{
  GdkClipboard *clipboard = GDK_CLIPBOARD (source);
  GdkBroadwayClipboard *cb = data;
  char *text;

  text = gdk_clipboard_read_text_finish (clipboard, result, NULL);
  /* On a serialization failure (text == NULL) leave the host clipboard alone
   * instead of clobbering it with an empty string. */
  if (text != NULL)
    {
      _gdk_broadway_server_set_clipboard_text (get_server (clipboard), text);
      g_free (text);
    }

  schedule_reclaim_remote (cb);
  g_object_unref (cb);
}

/* Called when local content is set (app copied) or when we claim_remote (local == FALSE). */
static gboolean
gdk_broadway_clipboard_claim (GdkClipboard       *clipboard,
                              GdkContentFormats  *formats,
                              gboolean            local,
                              GdkContentProvider *content)
{
  GdkBroadwayClipboard *cb = GDK_BROADWAY_CLIPBOARD (clipboard);
  gboolean res;

  res = GDK_CLIPBOARD_CLASS (gdk_broadway_clipboard_parent_class)->claim (clipboard, formats, local, content);

  if (local && content != NULL)
    {
      GValue value = G_VALUE_INIT;

      g_value_init (&value, G_TYPE_STRING);
      if (gdk_content_provider_get_value (content, &value, NULL))
        {
          /* Fast path: providers built from a string value
           * (gdk_clipboard_set / "Copy URL" / "Copy all text"). */
          const char *text = g_value_get_string (&value);
          _gdk_broadway_server_set_clipboard_text (get_server (clipboard), text ? text : "");
          schedule_reclaim_remote (cb);
        }
      else
        {
          /* Providers that only serialize to a mime type (e.g. a GtkTextView
           * selection via copy-clipboard): read the text asynchronously, then
           * reclaim remote in the callback. */
          gdk_clipboard_read_text_async (clipboard, NULL,
                                         gdk_broadway_clipboard_local_text_read,
                                         g_object_ref (cb));
        }
      g_value_unset (&value);
    }

  return res;
}

/* Detach the timeout + cancellable handler guarding the current pending read.
 * Safe to call from within the "cancelled" handler. Does not touch pending_read. */
static void
broadway_clipboard_detach_read_guards (GdkBroadwayClipboard *cb)
{
  if (cb->read_timeout_id != 0)
    {
      g_source_remove (cb->read_timeout_id);
      cb->read_timeout_id = 0;
    }
  if (cb->pending_cancellable != NULL)
    {
      if (cb->pending_cancelled_id != 0)
        g_signal_handler_disconnect (cb->pending_cancellable, cb->pending_cancelled_id);
      cb->pending_cancelled_id = 0;
      g_clear_object (&cb->pending_cancellable);
    }
}

/* Take ownership of the pending read task (caller must unref) and detach its guards. */
static GTask *
broadway_clipboard_steal_read (GdkBroadwayClipboard *cb)
{
  GTask *task = cb->pending_read;
  cb->pending_read = NULL;
  broadway_clipboard_detach_read_guards (cb);
  return task;
}

static void
read_cancelled_cb (GCancellable *cancellable,
                   gpointer      data)
{
  GdkBroadwayClipboard *cb = data;
  GTask *task;

  if (cb->pending_read == NULL)
    return;

  task = broadway_clipboard_steal_read (cb);
  g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_CANCELLED, "Clipboard read cancelled");
  g_object_unref (task);
}

static gboolean
read_timeout_cb (gpointer data)
{
  GdkBroadwayClipboard *cb = data;
  GTask *task;

  /* This source is removing itself; clear the id first so detach won't re-remove it. */
  cb->read_timeout_id = 0;

  if (cb->pending_read == NULL)
    return G_SOURCE_REMOVE;

  task = broadway_clipboard_steal_read (cb);
  g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_TIMED_OUT,
                           "Timed out waiting for the host clipboard");
  g_object_unref (task);
  return G_SOURCE_REMOVE;
}

static void
gdk_broadway_clipboard_read_async (GdkClipboard        *clipboard,
                                   GdkContentFormats   *formats,
                                   int                  io_priority,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
  GdkBroadwayClipboard *cb = GDK_BROADWAY_CLIPBOARD (clipboard);
  GTask *task;

  task = g_task_new (clipboard, cancellable, callback, user_data);
  g_task_set_priority (task, io_priority);
  g_task_set_source_tag (task, gdk_broadway_clipboard_read_async);

  /* We only ever serve text; if the caller wants some other mime type, fail
   * rather than hand back host text mislabeled as the requested format. */
  if (!gdk_content_formats_contain_mime_type (formats, TEXT_MIME) &&
      !gdk_content_formats_contain_mime_type (formats, "text/plain"))
    {
      g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               "Broadway clipboard only provides text");
      g_object_unref (task);
      return;
    }

  if (cb->pending_read != NULL)
    {
      GTask *old = broadway_clipboard_steal_read (cb);
      g_task_return_new_error (old, G_IO_ERROR, G_IO_ERROR_CANCELLED,
                               "Superseded by a newer clipboard read");
      g_object_unref (old);
    }

  cb->pending_read = task;

  if (cancellable != NULL)
    {
      cb->pending_cancellable = g_object_ref (cancellable);
      cb->pending_cancelled_id = g_signal_connect_object (cancellable, "cancelled",
                                                          G_CALLBACK (read_cancelled_cb),
                                                          cb, 0);
    }

  /* Already cancelled before we sent anything: fail right away. */
  if (cancellable != NULL && g_cancellable_is_cancelled (cancellable))
    {
      read_cancelled_cb (cancellable, cb);
      return;
    }

  /* Guard against a reply never arriving (tab closed / no browser connected). */
  cb->read_timeout_id = g_timeout_add_seconds (CLIPBOARD_READ_TIMEOUT_SECONDS,
                                               read_timeout_cb, cb);

  cb->pending_serial = _gdk_broadway_server_request_clipboard (get_server (clipboard));
}

static GInputStream *
gdk_broadway_clipboard_read_finish (GdkClipboard  *clipboard,
                                    GAsyncResult  *result,
                                    const char   **out_mime_type,
                                    GError       **error)
{
  GInputStream *stream;

  g_return_val_if_fail (g_task_is_valid (result, clipboard), NULL);

  stream = g_task_propagate_pointer (G_TASK (result), error);
  if (out_mime_type)
    *out_mime_type = g_intern_static_string (TEXT_MIME);

  return stream;
}

void
_gdk_broadway_clipboard_contents_received (GdkDisplay *display,
                                           guint32     in_reply_to,
                                           const char *text,
                                           gsize       len)
{
  GdkClipboard *clipboard = gdk_display_get_clipboard (display);
  GdkBroadwayClipboard *cb;
  GTask *task;
  GInputStream *stream;
  char *copy;

  if (!GDK_IS_BROADWAY_CLIPBOARD (clipboard))
    return;

  cb = GDK_BROADWAY_CLIPBOARD (clipboard);
  if (cb->pending_read == NULL)
    return;

  /* Only the reply to the request our current read actually sent counts;
   * a stale/superseded reply (from an earlier or timed-out request) is dropped. */
  if (in_reply_to != cb->pending_serial)
    return;

  task = broadway_clipboard_steal_read (cb);

  copy = g_strndup (text, len);
  stream = g_memory_input_stream_new_from_data (copy, len, g_free);
  g_task_return_pointer (task, stream, g_object_unref);
  g_object_unref (task);
}

static void
gdk_broadway_clipboard_finalize (GObject *object)
{
  GdkBroadwayClipboard *cb = GDK_BROADWAY_CLIPBOARD (object);

  if (cb->reclaim_idle != 0)
    g_source_remove (cb->reclaim_idle);
  if (cb->pending_read != NULL)
    {
      GTask *task = broadway_clipboard_steal_read (cb);
      g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_CANCELLED, "Clipboard finalized");
      g_object_unref (task);
    }

  G_OBJECT_CLASS (gdk_broadway_clipboard_parent_class)->finalize (object);
}

static void
gdk_broadway_clipboard_init (GdkBroadwayClipboard *cb)
{
}

static void
gdk_broadway_clipboard_class_init (GdkBroadwayClipboardClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GdkClipboardClass *clipboard_class = GDK_CLIPBOARD_CLASS (class);

  object_class->finalize = gdk_broadway_clipboard_finalize;
  clipboard_class->claim = gdk_broadway_clipboard_claim;
  clipboard_class->read_async = gdk_broadway_clipboard_read_async;
  clipboard_class->read_finish = gdk_broadway_clipboard_read_finish;
}

GdkClipboard *
gdk_broadway_clipboard_new (GdkDisplay *display)
{
  GdkBroadwayClipboard *cb;
  GdkContentFormats *formats;

  cb = g_object_new (GDK_TYPE_BROADWAY_CLIPBOARD, "display", display, NULL);

  /* Start remote so the very first paste fetches the host clipboard. */
  formats = text_formats ();
  gdk_clipboard_claim_remote (GDK_CLIPBOARD (cb), formats);
  gdk_content_formats_unref (formats);

  return GDK_CLIPBOARD (cb);
}
