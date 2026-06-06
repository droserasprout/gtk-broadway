/* gtk4-broadway-debugmenu - the native GTK4 debug menu for the Broadway daemon.
 *
 * Spawned on demand by gtk4-broadwayd (BROADWAY_EVENT_MENU) with GDK_BACKEND
 * and BROADWAY_DISPLAY pointed at that daemon, so this window composites into
 * the same display every connected browser sees. It is a plain GTK client - the
 * daemon just starts/stops the process.
 *
 * Stage 1: the window + layout. Stats are placeholders and the toggles are
 * stubs until the broadwayd control channel is wired (Stage 2).
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

static GMainLoop *loop;

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
  return FALSE; /* let the default close proceed */
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

static GtkWidget *
left_label (const char *text)
{
  GtkWidget *label = gtk_label_new (text);
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  return label;
}

int
main (void)
{
  GtkWidget *window, *box;
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

  /* Stats - placeholders until the broadwayd control channel feeds real
   * numbers (frame push rate + bytes written this session). */
  gtk_box_append (GTK_BOX (box), left_label ("Framerate: -- fps"));
  gtk_box_append (GTK_BOX (box), left_label ("Data this session: -- KiB"));

  gtk_box_append (GTK_BOX (box),
                  gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));

  /* Toggles / actions - stubs until wired to broadwayd. */
  gtk_box_append (GTK_BOX (box),
                  gtk_toggle_button_new_with_label ("Low latency (lower quality)"));
  gtk_box_append (GTK_BOX (box),
                  gtk_button_new_with_label ("Send test event"));

  gtk_box_append (GTK_BOX (box),
                  gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));

  GtkWidget *close = gtk_button_new_with_label ("Close");
  g_signal_connect (close, "clicked", G_CALLBACK (on_close_clicked), NULL);
  gtk_box_append (GTK_BOX (box), close);

  gtk_window_set_child (GTK_WINDOW (window), box);

  keys = gtk_event_controller_key_new ();
  g_signal_connect (keys, "key-pressed", G_CALLBACK (on_key_pressed), NULL);
  gtk_widget_add_controller (window, keys);

  g_signal_connect (window, "close-request", G_CALLBACK (on_close_request), NULL);

  gtk_window_present (GTK_WINDOW (window));

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  return 0;
}
