/* gtk4-brotway-debugmenu - the native GTK4 debug menu for the Broadway daemon.
 *
 * Spawned on demand by gtk4-broadwayd (BROADWAY_EVENT_MENU) with GDK_BACKEND
 * and BROADWAY_DISPLAY pointed at that daemon, so this window composites into
 * the same display every connected browser sees. broadwayd pins it on top.
 *
 * The daemon also hands us one end of a control socketpair via the
 * BROADWAY_DEBUGMENU_FD env var: it pushes "stats <session> <bytes> <fps>
 * <latency>" lines we display, and we send back action commands ("reconnect",
 * "drop-session").
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
static GtkWidget *pacing_label;
static GtkWidget *cost_label;
static GtkWidget *paint_flash_switch;
static GtkWidget *screen_w_spin;
static GtkWidget *screen_h_spin;
static GtkWidget *screen_scale_spin;
static GtkWidget *png_mode_dd;
static GSocket   *control_sock;
static GtkWidget *debug_window;     /* the stats overlay; owns the labels above */
static int        open_windows;     /* live top-levels; quit at zero */

static void
quit (void)
{
  if (loop != NULL)
    g_main_loop_quit (loop);
}

static void
on_window_destroy (GtkWidget *window, gpointer user_data)
{
  /* The stats labels live in the debug window; drop the dangling pointers when
   * it closes so the still-live control channel (on_control_readable) stops
   * writing to freed widgets. */
  if (window == debug_window)
    {
      debug_window = NULL;
      session_label = traffic_label = fps_label = NULL;
      latency_label = textures_label = pacing_label = cost_label = NULL;
      paint_flash_switch = NULL;
    }

  if (--open_windows <= 0)
    quit ();
}

static gboolean
on_escape (GtkEventControllerKey *controller,
           guint keyval, guint keycode,
           GdkModifierType state, gpointer window)
{
  if (keyval == GDK_KEY_Escape)
    {
      gtk_window_destroy (GTK_WINDOW (window));
      return TRUE;
    }
  return FALSE;
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
on_screen_apply_clicked (GtkButton *button, gpointer user_data)
{
  char cmd[64];
  int w = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (screen_w_spin));
  int h = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (screen_h_spin));
  int s = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (screen_scale_spin));
  g_snprintf (cmd, sizeof cmd, "screen %d %d %d\n", w, h, s);
  send_command (cmd);
}

static void
on_screen_reset_clicked (GtkButton *button, gpointer user_data)
{
  send_command ("screen 0 0 0\n");  /* scale 0 = unpin */
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

  /* Debug window closed? The stats widgets are gone - nothing to update, so
   * drop the source instead of touching freed memory. */
  if (debug_window == NULL)
    return G_SOURCE_REMOVE;

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
      unsigned int iv_p95 = 0, iv_max = 0, w_avg = 0, w_max = 0, bpf = 0;
      double up_s = 0, rel_s = 0;   /* texture uploads (= cache misses) / releases per sec */
      int got;

      got = sscanf (p, "stats %x %" G_GUINT64_FORMAT " %lf %u %d %u %" G_GUINT64_FORMAT
                       " %u %u %u %u %u %lf %lf",
                    &sid, &bytes, &fps, &latency, &flash, &tex_count, &tex_bytes,
                    &iv_p95, &iv_max, &w_avg, &w_max, &bpf, &up_s, &rel_s);
      if (got >= 7)
        {
          /* Traffic rate from the byte delta since the last push (the daemon
           * sends cumulative bytes ~every 500ms). bytes < prev_bytes means the
           * daemon counter reset (reconnect): show 0, prev_bytes reseeds below. */
          static guint64 prev_bytes = 0;
          static gint64 prev_time = 0;
          gint64 now = g_get_monotonic_time ();
          double rate = (prev_time != 0 && now > prev_time && bytes >= prev_bytes)
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
          /* Upload rate = client content-cache miss rate; releases = evictions
           * (only sent by newer daemons; got==14). Sustained up/s on a static
           * screen = the dedup cache is thrashing and may want a bigger cap. */
          char *x = (got >= 14)
            ? g_strdup_printf ("Textures: %u (%s) · %.1f up/s · %.1f rel/s",
                               tex_count, texbuf, up_s, rel_s)
            : g_strdup_printf ("Textures: %u (%s)", tex_count, texbuf);

          /* Smoothness (only sent by newer daemons; got==12). p95/max in ms. */
          char *pace, *cost;
          if (got >= 12)
            {
              char *bpf_s = format_bytes (bpf);
              /* Empty pacing ring (iv_max==0) => no back-to-back frames = idle. */
              if (iv_max == 0)
                pace = g_strdup ("Frame: idle");
              else
                pace = g_strdup_printf ("Frame: p95 %.1f / max %.1f ms",
                                        iv_p95 / 1000.0, iv_max / 1000.0);
              cost = g_strdup_printf ("Write: %.1f / max %.1f ms · %s/f",
                                      w_avg / 1000.0, w_max / 1000.0, bpf_s);
              g_free (bpf_s);
            }
          else
            {
              pace = g_strdup ("Frame: --");
              cost = g_strdup ("Write: --");
            }

          prev_bytes = bytes;
          prev_time = now;

          gtk_label_set_text (GTK_LABEL (session_label), s);
          gtk_label_set_text (GTK_LABEL (traffic_label), t);
          gtk_label_set_text (GTK_LABEL (fps_label), f);
          gtk_label_set_text (GTK_LABEL (latency_label), l);
          gtk_label_set_text (GTK_LABEL (textures_label), x);
          gtk_label_set_text (GTK_LABEL (pacing_label), pace);
          gtk_label_set_text (GTK_LABEL (cost_label), cost);

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
          g_free (pace);
          g_free (cost);
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

/* A labelled row with a trailing GtkSpinButton (integer range). */
static GtkWidget *
add_spin_row (GtkWidget *section, const char *label, int min, int max, int value)
{
  GtkWidget *row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *spin = gtk_spin_button_new_with_range (min, max, 1);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), value);
  gtk_widget_set_halign (spin, GTK_ALIGN_END);
  gtk_widget_set_hexpand (spin, TRUE);
  gtk_box_append (GTK_BOX (row), left_label (label));
  gtk_box_append (GTK_BOX (row), spin);
  gtk_box_append (GTK_BOX (section), row);
  return spin;
}

/* A top-level that shares the menu's main loop and Escape-to-close; quit fires
 * when the last tracked window is gone. */
static GtkWidget *
new_tracked_window (const char *title)
{
  GtkWidget *window = gtk_window_new ();
  GtkEventController *keys = gtk_event_controller_key_new ();

  gtk_window_set_title (GTK_WINDOW (window), title);
  g_signal_connect (keys, "key-pressed", G_CALLBACK (on_escape), window);
  gtk_widget_add_controller (window, keys);
  g_signal_connect (window, "destroy", G_CALLBACK (on_window_destroy), NULL);
  open_windows++;
  return window;
}

/* Send the selected PNG preset (dropdown index == BroadwayPngPreset id). */
static void
on_png_changed (GtkDropDown *dd, GParamSpec *pspec, gpointer user_data)
{
  char cmd[32];

  g_snprintf (cmd, sizeof cmd, "png-preset %u\n", gtk_drop_down_get_selected (dd));
  send_command (cmd);
}

/* A labelled row with a trailing GtkDropDown built from a NULL-terminated label
 * list. Handler is wired by the caller (after all rows exist). */
static GtkWidget *
add_dropdown_row (GtkWidget *section, const char *label, const char * const *labels)
{
  GtkWidget *row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *dd = gtk_drop_down_new_from_strings (labels);

  gtk_widget_set_halign (dd, GTK_ALIGN_END);
  gtk_widget_set_hexpand (dd, TRUE);
  gtk_box_append (GTK_BOX (row), left_label (label));
  gtk_box_append (GTK_BOX (row), dd);
  gtk_box_append (GTK_BOX (section), row);
  return dd;
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
  GtkWidget *window, *root, *left, *right, *perf, *smooth, *actions, *screen, *screen_btns;
  GtkCssProvider *css;

  gtk_init ();

  /* Always dark - this is a developer overlay, not a themed app window. */
  g_object_set (gtk_settings_get_default (),
                "gtk-application-prefer-dark-theme", TRUE, NULL);

  /* Compact: trim default padding/font so this dense overlay stays small. */
  css = gtk_css_provider_new ();
  gtk_css_provider_load_from_string (css,
      "* { font-size: 11px; }"
      "button { min-height: 0; padding: 1px 6px; }"
      "spinbutton, spinbutton entry { min-height: 0; }"
      "spinbutton button { min-width: 0; padding: 0 2px; }"
      "switch { min-height: 16px; }"
      "frame > border { padding: 2px; }");
  gtk_style_context_add_provider_for_display (gdk_display_get_default (),
      GTK_STYLE_PROVIDER (css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (css);

  window = new_tracked_window ("Broadway debug");
  debug_window = window;
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

  /* Two columns: live metrics left, controls right - keeps the window short. */
  root = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_top (root, 6);
  gtk_widget_set_margin_bottom (root, 6);
  gtk_widget_set_margin_start (root, 6);
  gtk_widget_set_margin_end (root, 6);

  left = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  right = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_valign (left, GTK_ALIGN_START);
  gtk_widget_set_valign (right, GTK_ALIGN_START);
  gtk_box_append (GTK_BOX (root), left);
  gtk_box_append (GTK_BOX (root), right);

  /* Performance: live stats from the daemon's control channel. */
  perf = add_section (left, "Performance");
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

  /* Smoothness: frame pacing + transmit cost (newer daemons only). */
  smooth = add_section (left, "Smoothness");
  pacing_label = left_label ("Frame: p95 -- / max -- ms");
  cost_label = left_label ("Write: -- / max -- ms");
  gtk_box_append (GTK_BOX (smooth), pacing_label);
  gtk_box_append (GTK_BOX (smooth), cost_label);

  /* Actions: commands sent back to the daemon. */
  actions = add_section (right, "Actions");
  add_action_button (actions, "Reconnect", G_CALLBACK (on_reconnect_clicked));
  add_action_button (actions, "Drop session", G_CALLBACK (on_drop_session_clicked));

  paint_flash_switch = add_switch_row (actions, "Paint flashing",
                                       G_CALLBACK (on_paint_flash_state_set));
  add_switch_row (actions, "Debug logging", NULL);  /* no-op for now */

  /* PNG encoding preset: one row, switches the app's per-frame encoder at
   * runtime. Order matches BroadwayPngPreset (Fast=0, Compact=1). */
  {
    static const char * const modes[] = { "Fast (LAN)", "Compact (remote)", NULL };

    png_mode_dd = add_dropdown_row (actions, "PNG encoding", modes);
    /* Connect after construction so the initial selected=0 doesn't fire a send. */
    g_signal_connect (png_mode_dd, "notify::selected", G_CALLBACK (on_png_changed), NULL);
  }

  /* Screen: pin the browser's logical size + integer render scale (natural size). */
  screen = add_section (right, "Screen");
  screen_w_spin = add_spin_row (screen, "Width", 64, 8192, 1280);
  screen_h_spin = add_spin_row (screen, "Height", 64, 8192, 800);
  screen_scale_spin = add_spin_row (screen, "Scale", 1, 4, 1);
  screen_btns = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (screen_btns), TRUE);
  add_action_button (screen_btns, "Apply", G_CALLBACK (on_screen_apply_clicked));
  add_action_button (screen_btns, "Reset", G_CALLBACK (on_screen_reset_clicked));
  gtk_box_append (GTK_BOX (screen), screen_btns);

  gtk_window_set_child (GTK_WINDOW (window), root);

  connect_control_channel ();

  gtk_window_present (GTK_WINDOW (window));

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  return 0;
}
