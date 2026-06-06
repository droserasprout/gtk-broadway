/* gtk4-broadway-debugmenu - the native GTK4 debug menu for the Broadway daemon.
 *
 * Spawned on demand by gtk4-broadwayd (BROADWAY_EVENT_MENU) with GDK_BACKEND
 * and BROADWAY_DISPLAY pointed at that daemon, so this window composites into
 * the same display every connected browser sees. broadwayd pins it on top.
 *
 * The daemon also hands us one end of a control socketpair via the
 * BROADWAY_DEBUGMENU_FD env var: it pushes "stats <session> <bytes> <fps>"
 * lines we display, and we send back action commands ("open-uri").
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>

static GMainLoop *loop;
static GtkWidget *session_label;
static GtkWidget *traffic_label;
static GtkWidget *fps_label;
static GSocket   *control_sock;

static void
quit (void)
{
  if (loop != NULL)
    g_main_loop_quit (loop);
}

static gboolean
on_close_request (GtkWindow *window, gpointer user_data)
{
  quit ();
  return FALSE;
}

static gboolean
on_key_pressed (GtkEventControllerKey *controller,
                guint keyval, guint keycode,
                GdkModifierType state, gpointer user_data)
{
  if (keyval == GDK_KEY_Escape)
    {
      quit ();
      return TRUE;
    }
  return FALSE;
}

static void
on_close_clicked (GtkButton *button, gpointer user_data)
{
  quit ();
}

static void
on_test_uri_clicked (GtkButton *button, gpointer user_data)
{
  if (control_sock != NULL)
    g_socket_send (control_sock, "open-uri\n", 9, NULL, NULL);
}

static char *
format_bytes (guint64 b)
{
  const char *units[] = { "B", "KiB", "MiB", "GiB", "TiB" };
  double v = (double) b;
  int u = 0;

  while (v >= 1024.0 && u < 4)
    {
      v /= 1024.0;
      u++;
    }
  return g_strdup_printf (u == 0 ? "%.0f %s" : "%.1f %s", v, units[u]);
}

/* Daemon pushes "stats <session hex> <bytes> <fps>\n" every ~500ms. */
static gboolean
on_control_readable (GSocket *sock, GIOCondition cond, gpointer user_data)
{
  char buf[256];
  char *p;
  gssize n;

  n = g_socket_receive (sock, buf, sizeof buf - 1, NULL, NULL);
  if (n <= 0)
    return G_SOURCE_REMOVE; /* daemon gone */

  buf[n] = '\0';

  /* Use the last complete line in case several arrived at once. */
  p = g_strrstr (buf, "stats ");
  if (p != NULL)
    {
      unsigned int sid = 0;
      guint64 bytes = 0;
      double fps = 0;

      if (sscanf (p, "stats %x %" G_GUINT64_FORMAT " %lf", &sid, &bytes, &fps) == 3)
        {
          char *traffic = format_bytes (bytes);
          char *s = g_strdup_printf ("Session: %08x", sid);
          char *t = g_strdup_printf ("Traffic: %s", traffic);
          char *f = g_strdup_printf ("Framerate: %.1f fps", fps);

          gtk_label_set_text (GTK_LABEL (session_label), s);
          gtk_label_set_text (GTK_LABEL (traffic_label), t);
          gtk_label_set_text (GTK_LABEL (fps_label), f);

          g_free (traffic);
          g_free (s);
          g_free (t);
          g_free (f);
        }
    }
  return G_SOURCE_CONTINUE;
}

static GtkWidget *
left_label (const char *text)
{
  GtkWidget *label = gtk_label_new (text);
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  return label;
}

static void
connect_control_channel (void)
{
  const char *fdenv = g_getenv ("BROADWAY_DEBUGMENU_FD");
  int fd;

  if (fdenv == NULL)
    return;

  fd = (int) g_ascii_strtoll (fdenv, NULL, 10);
  if (fd <= 0)
    return;

  control_sock = g_socket_new_from_fd (fd, NULL);
  if (control_sock != NULL)
    {
      GSource *src;
      g_socket_set_blocking (control_sock, FALSE);
      src = g_socket_create_source (control_sock, G_IO_IN, NULL);
      g_source_set_callback (src, (GSourceFunc) on_control_readable, NULL, NULL);
      g_source_attach (src, NULL);
      g_source_unref (src);
    }
}

int
main (void)
{
  GtkWidget *window, *box, *test, *close;
  GtkEventController *keys;

  gtk_init ();

  window = gtk_window_new ();
  gtk_window_set_title (GTK_WINDOW (window), "Broadway debug");
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_top (box, 8);
  gtk_widget_set_margin_bottom (box, 8);
  gtk_widget_set_margin_start (box, 8);
  gtk_widget_set_margin_end (box, 8);
  gtk_widget_set_size_request (box, 220, -1);

  /* Live stats, updated from the daemon's control channel. */
  session_label = left_label ("Session: --------");
  traffic_label = left_label ("Traffic: --");
  fps_label = left_label ("Framerate: -- fps");
  gtk_box_append (GTK_BOX (box), session_label);
  gtk_box_append (GTK_BOX (box), traffic_label);
  gtk_box_append (GTK_BOX (box), fps_label);

  gtk_box_append (GTK_BOX (box),
                  gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));

  test = gtk_button_new_with_label ("Open test URL");
  g_signal_connect (test, "clicked", G_CALLBACK (on_test_uri_clicked), NULL);
  gtk_box_append (GTK_BOX (box), test);

  close = gtk_button_new_with_label ("Close");
  g_signal_connect (close, "clicked", G_CALLBACK (on_close_clicked), NULL);
  gtk_box_append (GTK_BOX (box), close);

  gtk_window_set_child (GTK_WINDOW (window), box);

  keys = gtk_event_controller_key_new ();
  g_signal_connect (keys, "key-pressed", G_CALLBACK (on_key_pressed), NULL);
  gtk_widget_add_controller (window, keys);

  g_signal_connect (window, "close-request", G_CALLBACK (on_close_request), NULL);

  connect_control_channel ();

  gtk_window_present (GTK_WINDOW (window));

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  return 0;
}
