#pragma once

#include <glib.h>
#include <gio/gio.h>
#include "broadway-protocol.h"
#include "broadway-server.h"

typedef struct BroadwayOutput BroadwayOutput;

typedef enum {
  BROADWAY_WS_CONTINUATION = 0,
  BROADWAY_WS_TEXT = 1,
  BROADWAY_WS_BINARY = 2,
  BROADWAY_WS_CNX_CLOSE = 8,
  BROADWAY_WS_CNX_PING = 9,
  BROADWAY_WS_CNX_PONG = 0xa
} BroadwayWSOpCode;

BroadwayOutput *broadway_output_new                 (GOutputStream  *out,
                                                     guint32         serial);
void            broadway_output_free                (BroadwayOutput *output);
int             broadway_output_flush               (BroadwayOutput *output);
int             broadway_output_has_error           (BroadwayOutput *output);
void            broadway_output_set_next_serial     (BroadwayOutput *output,
                                                     guint32         serial);
guint32         broadway_output_get_next_serial     (BroadwayOutput *output);
guint64         broadway_output_get_bytes_sent      (BroadwayOutput *output);
guint32         broadway_output_get_frames          (BroadwayOutput *output);
void            broadway_output_get_pacing          (BroadwayOutput *output,
                                                     guint32 *iv_p95_us, guint32 *iv_max_us,
                                                     guint32 *write_avg_us, guint32 *write_max_us);
void            broadway_output_new_surface         (BroadwayOutput *output,
                                                     int             id,
                                                     int             x,
                                                     int             y,
                                                     int             w,
                                                     int             h);
void            broadway_output_disconnected        (BroadwayOutput *output);
void            broadway_output_session             (BroadwayOutput *output,
                                                     guint32         token,
                                                     guint32         client_id);
void            broadway_output_pong_msg            (BroadwayOutput *output);
void            broadway_output_debug_flash         (BroadwayOutput *output,
                                                     gboolean        enabled);
void            broadway_output_debug_set_screen    (BroadwayOutput *output,
                                                     int w, int h, int scale);
void            broadway_output_show_surface        (BroadwayOutput *output,
                                                     int             id);
void            broadway_output_hide_surface        (BroadwayOutput *output,
                                                     int             id);
void            broadway_output_raise_surface       (BroadwayOutput *output,
                                                     int             id);
void            broadway_output_lower_surface       (BroadwayOutput *output,
                                                     int             id);
void            broadway_output_destroy_surface     (BroadwayOutput *output,
                                                     int             id);
void            broadway_output_roundtrip           (BroadwayOutput *output,
                                                     int             id,
                                                     guint32         tag);
void            broadway_output_move_resize_surface (BroadwayOutput *output,
                                                     int             id,
                                                     gboolean        has_pos,
                                                     int             x,
                                                     int             y,
                                                     gboolean        has_size,
                                                     int             w,
                                                     int             h);
void            broadway_output_set_transient_for   (BroadwayOutput *output,
                                                     int             id,
                                                     int             parent_id);
void            broadway_output_surface_set_nodes   (BroadwayOutput *output,
                                                     int             id,
                                                     BroadwayNode   *root,
                                                     BroadwayNode   *old_root,
                                                     GHashTable     *old_node_lookup);
void            broadway_output_upload_texture      (BroadwayOutput *output,
                                                     guint32         id,
                                                     GBytes         *texture);
void            broadway_output_release_texture     (BroadwayOutput *output,
                                                     guint32         id);
void            broadway_output_grab_pointer        (BroadwayOutput *output,
                                                     int             id,
                                                     gboolean        owner_event);
guint32         broadway_output_ungrab_pointer      (BroadwayOutput *output);
void            broadway_output_pong                (BroadwayOutput *output);
void            broadway_output_set_show_keyboard   (BroadwayOutput *output,
                                                     gboolean        show);
void            broadway_output_set_input_region    (BroadwayOutput *output,
                                                     int             id,
                                                     int             mode,
                                                     BroadwayRect   *rect);
void            broadway_output_reassert_pointer     (BroadwayOutput *output,
                                                     int             id,
                                                     int             x,
                                                     int             y);
void            broadway_output_set_clipboard       (BroadwayOutput *output,
                                                     const char     *text,
                                                     gsize           len);
void            broadway_output_request_clipboard   (BroadwayOutput *output,
                                                     guint32         id);
void            broadway_output_open_uri             (BroadwayOutput *output,
                                                     const char     *uri,
                                                     gsize           len);
void            broadway_output_set_cursor           (BroadwayOutput *output,
                                                     int             id,
                                                     const char     *name,
                                                     gsize           len);
void            broadway_output_set_title            (BroadwayOutput *output,
                                                     int             id,
                                                     const char     *title,
                                                     gsize           len);
void            broadway_output_set_icon             (BroadwayOutput *output,
                                                     int             id,
                                                     const guchar   *data,
                                                     gsize           len);
void            broadway_output_set_modal            (BroadwayOutput *output,
                                                     int             id,
                                                     gboolean        modal);

