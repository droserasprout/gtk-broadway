/* gtk4-broadway-debugmenu - the native GTK4 debug menu for the Broadway daemon.
 *
 * Spawned on demand by gtk4-broadwayd (BROADWAY_EVENT_MENU) with GDK_BACKEND
 * and BROADWAY_DISPLAY pointed at that daemon, so this window composites into
 * the same display every connected browser sees. broadwayd pins it on top.
 *
 * The daemon also hands us one end of a control socketpair via the
 * BROADWAY_DEBUGMENU_FD env var: it pushes "stats <session> <bytes> <fps>
 * <latency>" lines we display, and we send back action commands ("reconnect",
 * "drop-session", "open-uri").
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <string.h>

static GMainLoop *loop;
static GtkWidget *session_label;
static GtkWidget *traffic_label;
static GtkWidget *fps_label;
static GtkWidget *latency_label;
static GtkWidget *textures_label;
static GtkWidget *paint_flash_switch;
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

/* Send a newline-terminated command back to the daemon over the control fd. */
static void
send_command (const char *cmd)
{
  if (control_sock != NULL)
    g_socket_send (control_sock, cmd, strlen (cmd), NULL, NULL); /* best-effort */
}

static void
on_reconnect_clicked (GtkButton *button, gpointer user_data)
{
  send_command ("reconnect\n");
}

static void
on_drop_session_clicked (GtkButton *button, gpointer user_data)
{
  send_command ("drop-session\n");
}

static void
on_test_uri_clicked (GtkButton *button, gpointer user_data)
{
  send_command ("open-uri\n");
}

static gboolean
on_paint_flash_state_set (GtkSwitch *sw, gboolean state, gpointer user_data)
{
  send_command (state ? "paint-flash 1\n" : "paint-flash 0\n");
  return FALSE;  /* let the switch update its own visual state */
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

/* Daemon pushes "stats <session hex> <bytes> <fps> <latency ms>\n" every ~500ms. */
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
      unsigned int latency = 0;
      int flash = 0;
      unsigned int tex_count = 0;
      guint64 tex_bytes = 0;

      if (sscanf (p, "stats %x %" G_GUINT64_FORMAT " %lf %u %d %u %" G_GUINT64_FORMAT,
                  &sid, &bytes, &fps, &latency, &flash, &tex_count, &tex_bytes) == 7)
        {
          /* Traffic rate from the byte delta since the last push (the daemon
           * sends cumulative bytes ~every 500ms). */
          static guint64 prev_bytes = 0;
          static gint64 prev_time = 0;
          gint64 now = g_get_monotonic_time ();
          double rate = (prev_time != 0 && now > prev_time)
                          ? (double) (bytes - prev_bytes) * G_USEC_PER_SEC / (now - prev_time)
                          : 0;
          char *traffic = format_bytes (bytes);
          char *rate_s = format_bytes ((guint64) rate);
          char *texbuf = format_bytes (tex_bytes);
          char *s = g_strdup_printf ("Session: %08x", sid);
          char *t = g_strdup_printf ("Traffic: %s (%s/s)", traffic, rate_s);
          /* "fps" here is non-empty flushes per second, i.e. pushes to the browser. */
          char *f = g_strdup_printf ("Pushes: %.1f/s", fps);
          char *l = g_strdup_printf ("Latency: %u ms", latency);
          char *x = g_strdup_printf ("Textures: %u (%s)", tex_count, texbuf);

          prev_bytes = bytes;
          prev_time = now;

          gtk_label_set_text (GTK_LABEL (session_label), s);
          gtk_label_set_text (GTK_LABEL (traffic_label), t);
          gtk_label_set_text (GTK_LABEL (fps_label), f);
          gtk_label_set_text (GTK_LABEL (latency_label), l);
          gtk_label_set_text (GTK_LABEL (textures_label), x);

          /* Reflect the daemon's real paint-flash state (a freshly spawned menu
           * starts with the switch off; restore it without re-sending). */
          if (paint_flash_switch != NULL &&
              gtk_switch_get_active (GTK_SWITCH (paint_flash_switch)) != (flash != 0))
            {
              g_signal_handlers_block_by_func (paint_flash_switch,
                                               on_paint_flash_state_set, NULL);
              gtk_switch_set_active (GTK_SWITCH (paint_flash_switch), flash != 0);
              g_signal_handlers_unblock_by_func (paint_flash_switch,
                                                 on_paint_flash_state_set, NULL);
            }

          g_free (traffic);
          g_free (rate_s);
          g_free (texbuf);
          g_free (s);
          g_free (t);
          g_free (f);
          g_free (l);
          g_free (x);
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

/* A titled GtkFrame with a padded vertical box; returns the box to fill. */
static GtkWidget *
add_section (GtkWidget *parent, const char *title)
{
  GtkWidget *frame = gtk_frame_new (title);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

  gtk_widget_set_margin_top (box, 6);
  gtk_widget_set_margin_bottom (box, 6);
  gtk_widget_set_margin_start (box, 6);
  gtk_widget_set_margin_end (box, 6);
  gtk_frame_set_child (GTK_FRAME (frame), box);
  gtk_box_append (GTK_BOX (parent), frame);
  return box;
}

static GtkWidget *
add_action_button (GtkWidget *section, const char *label, GCallback cb)
{
  GtkWidget *button = gtk_button_new_with_label (label);
  g_signal_connect (button, "clicked", cb, NULL);
  gtk_box_append (GTK_BOX (section), button);
  return button;
}

/* A labelled row with a trailing GtkSwitch. state_set_cb may be NULL (no-op switch). */
static GtkWidget *
add_switch_row (GtkWidget *section, const char *label, GCallback state_set_cb)
{
  GtkWidget *row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *sw = gtk_switch_new ();

  gtk_widget_set_halign (sw, GTK_ALIGN_END);
  gtk_widget_set_hexpand (sw, TRUE);
  if (state_set_cb != NULL)
    g_signal_connect (sw, "state-set", state_set_cb, NULL);
  gtk_box_append (GTK_BOX (row), left_label (label));
  gtk_box_append (GTK_BOX (row), sw);
  gtk_box_append (GTK_BOX (section), row);
  return sw;
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
  GtkWidget *window, *box, *perf, *actions, *close;
  GtkEventController *keys;

  gtk_init ();

  /* Always dark - this is a developer overlay, not a themed app window. */
  g_object_set (gtk_settings_get_default (),
                "gtk-application-prefer-dark-theme", TRUE, NULL);

  window = gtk_window_new ();
  gtk_window_set_title (GTK_WINDOW (window), "Broadway debug");
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_top (box, 8);
  gtk_widget_set_margin_bottom (box, 8);
  gtk_widget_set_margin_start (box, 8);
  gtk_widget_set_margin_end (box, 8);
  gtk_widget_set_size_request (box, 240, -1);

  /* Performance: live stats from the daemon's control channel. */
  perf = add_section (box, "Performance");
  session_label = left_label ("Session: --------");
  traffic_label = left_label ("Traffic: -- (--/s)");
  fps_label = left_label ("Pushes: --/s");
  latency_label = left_label ("Latency: -- ms");
  textures_label = left_label ("Textures: -- (--)");
  gtk_box_append (GTK_BOX (perf), session_label);
  gtk_box_append (GTK_BOX (perf), traffic_label);
  gtk_box_append (GTK_BOX (perf), fps_label);
  gtk_box_append (GTK_BOX (perf), latency_label);
  gtk_box_append (GTK_BOX (perf), textures_label);

  /* Actions: commands sent back to the daemon. */
  actions = add_section (box, "Actions");
  add_action_button (actions, "Reconnect", G_CALLBACK (on_reconnect_clicked));
  add_action_button (actions, "Drop session", G_CALLBACK (on_drop_session_clicked));
  add_action_button (actions, "Open test URL", G_CALLBACK (on_test_uri_clicked));

  paint_flash_switch = add_switch_row (actions, "Paint flashing",
                                       G_CALLBACK (on_paint_flash_state_set));
  add_switch_row (actions, "Debug logging", NULL);  /* no-op for now */

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
