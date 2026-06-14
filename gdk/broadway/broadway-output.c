#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <cairo.h>

#include "broadway-output.h"

//#define DEBUG_NODE_SENDING
//#define DEBUG_NODE_SENDING_REMOVE

/************************************************************************
 *                Basic I/O primitives                                  *
 ************************************************************************/

#define BROADWAY_OUTPUT_IV_RING 64
/* Intervals longer than this are treated as idle gaps (the app stopped pushing
 * frames) rather than stutter, and are kept out of the pacing ring so p95/max
 * reflect only back-to-back frames. 250ms = a 4fps floor for "still animating". */
#define BROADWAY_OUTPUT_IDLE_CUTOFF_US (250 * 1000)

struct BroadwayOutput {
  GOutputStream *out;
  GString *buf;
  int error;
  guint32 serial;
  guint64 bytes_sent;   /* payload bytes flushed to the browser (this connection) */
  guint32 frames;       /* non-empty flushes ~ display pushes (this connection) */

  /* Smoothness instrumentation for the debug menu (microseconds). Interval =
   * gap between real flushes (pacing); write = time blocked in the socket
   * writev (transmit + backpressure). */
  gint64  last_flush_us;
  guint32 iv_ring[BROADWAY_OUTPUT_IV_RING];   /* rolling frame-to-frame intervals */
  guint8  iv_head;
  guint8  iv_count;
  guint64 write_us_sum;                       /* windowed: reset on read */
  guint32 write_us_max;
  guint32 write_samples;
};

static void
broadway_output_send_cmd (BroadwayOutput *output,
                          gboolean fin, BroadwayWSOpCode code,
                          const void *buf, gsize count)
{
  gboolean mask = FALSE;
  guchar header[16];
  size_t p;

  gboolean mid_header = count > 125 && count <= 65535;
  gboolean long_header = count > 65535;

  /* NB. big-endian spec => bit 0 == MSB */
  header[0] = ( (fin ? 0x80 : 0) | (code & 0x0f) );
  header[1] = ( (mask ? 0x80 : 0) |
                (mid_header ? 126 : long_header ? 127 : count) );
  p = 2;
  if (mid_header)
    {
      *(guint16 *)(header + p) = GUINT16_TO_BE( (guint16)count );
      p += 2;
    }
  else if (long_header)
    {
      *(guint64 *)(header + p) = GUINT64_TO_BE( count );
      p += 8;
    }
  // FIXME: if we are paranoid we should 'mask' the data
  /* One vectored write instead of two syscalls, and no payload copy. */
  if (count > 0)
    {
      GOutputVector vectors[2];

      vectors[0].buffer = header;
      vectors[0].size = p;
      vectors[1].buffer = buf;
      vectors[1].size = count;

      g_output_stream_writev_all (output->out, vectors, 2, NULL, NULL, NULL);
    }
  else
    {
      g_output_stream_write_all (output->out, header, p, NULL, NULL, NULL);
    }
}

void broadway_output_pong (BroadwayOutput *output)
{
  broadway_output_send_cmd (output, TRUE, BROADWAY_WS_CNX_PONG, NULL, 0);
}

/* A big texture frame can grow buf to many MB; set_size(0) keeps that capacity
 * forever. After an oversized flush, free it and start small instead. */
#define BROADWAY_OUTPUT_BUF_SHRINK_THRESHOLD (256 * 1024)

int
broadway_output_flush (BroadwayOutput *output)
{
  gsize flushed_len;

  gint64 t0;

  if (output->buf->len == 0)
    return TRUE;

  /* Pacing: record the gap since the previous real flush, but skip idle gaps so
   * p95/max measure stutter during continuous rendering, not time spent idle. */
  t0 = g_get_monotonic_time ();
  if (output->last_flush_us != 0)
    {
      guint32 iv = (guint32) (t0 - output->last_flush_us);
      if (iv <= BROADWAY_OUTPUT_IDLE_CUTOFF_US)
        {
          output->iv_ring[output->iv_head] = iv;
          output->iv_head = (output->iv_head + 1) % BROADWAY_OUTPUT_IV_RING;
          if (output->iv_count < BROADWAY_OUTPUT_IV_RING)
            output->iv_count++;
        }
    }
  output->last_flush_us = t0;

  broadway_output_send_cmd (output, TRUE, BROADWAY_WS_BINARY,
                            output->buf->str, output->buf->len);

  /* Write time: how long the (blocking) socket writev took = backpressure. */
  {
    guint32 w = (guint32) (g_get_monotonic_time () - t0);
    output->write_us_sum += w;
    output->write_samples++;
    if (w > output->write_us_max)
      output->write_us_max = w;
  }

  output->bytes_sent += output->buf->len;
  output->frames++;

  flushed_len = output->buf->len;

  if (flushed_len > BROADWAY_OUTPUT_BUF_SHRINK_THRESHOLD)
    {
      g_string_free (output->buf, TRUE);
      output->buf = g_string_new ("");
    }
  else
    g_string_set_size (output->buf, 0);

  return !output->error;

}

BroadwayOutput *
broadway_output_new (GOutputStream *out, guint32 serial)
{
  BroadwayOutput *output;

  output = g_new0 (BroadwayOutput, 1);

  output->out = g_object_ref (out);
  output->buf = g_string_new ("");
  output->serial = serial;

  return output;
}

void
broadway_output_free (BroadwayOutput *output)
{
  g_object_unref (output->out);
  g_string_free (output->buf, TRUE);
  free (output);
}

guint32
broadway_output_get_next_serial (BroadwayOutput *output)
{
  return output->serial;
}

guint64
broadway_output_get_bytes_sent (BroadwayOutput *output)
{
  return output->bytes_sent;
}

guint32
broadway_output_get_frames (BroadwayOutput *output)
{
  return output->frames;
}

static int
cmp_u32 (const void *a, const void *b)
{
  guint32 x = *(const guint32 *) a, y = *(const guint32 *) b;
  return (x > y) - (x < y);
}

/* Debug-menu pacing stats (microseconds). Interval p95/max are taken over a
 * rolling ring of recent frames; write avg/max are windowed and reset on read. */
void
broadway_output_get_pacing (BroadwayOutput *output,
                            guint32 *iv_p95_us, guint32 *iv_max_us,
                            guint32 *write_avg_us, guint32 *write_max_us)
{
  guint32 sorted[BROADWAY_OUTPUT_IV_RING];
  guint n = output->iv_count;

  if (n > 0)
    {
      memcpy (sorted, output->iv_ring, n * sizeof (guint32));
      qsort (sorted, n, sizeof (guint32), cmp_u32);
      *iv_p95_us = sorted[(guint) ((n - 1) * 0.95)];
      *iv_max_us = sorted[n - 1];
    }
  else
    {
      *iv_p95_us = 0;
      *iv_max_us = 0;
    }

  *write_avg_us = output->write_samples ? (guint32) (output->write_us_sum / output->write_samples) : 0;
  *write_max_us = output->write_us_max;

  output->write_us_sum = 0;
  output->write_samples = 0;
  output->write_us_max = 0;
}

void
broadway_output_set_next_serial (BroadwayOutput *output,
                                 guint32 serial)
{
  output->serial = serial;
}


/************************************************************************
 *                     Core rendering operations                        *
 ************************************************************************/

static void
append_uint8 (BroadwayOutput *output, guint8 c)
{
  g_string_append_c (output->buf, c);
}

static void
append_bool (BroadwayOutput *output, gboolean val)
{
  g_string_append_c (output->buf, val ? 1: 0);
}

static void
append_flags (BroadwayOutput *output, guint32 val)
{
  g_string_append_c (output->buf, val);
}

static void
append_uint16 (BroadwayOutput *output, guint32 v)
{
  gsize old_len = output->buf->len;
  guint8 *buf;

  g_string_set_size (output->buf, old_len + 2);
  buf = (guint8 *)output->buf->str + old_len;
  buf[0] = (v >> 0) & 0xff;
  buf[1] = (v >> 8) & 0xff;
}

static void
append_uint32 (BroadwayOutput *output, guint32 v)
{
  gsize old_len = output->buf->len;
  guint8 *buf;

  g_string_set_size (output->buf, old_len + 4);
  buf = (guint8 *)output->buf->str + old_len;
  buf[0] = (v >> 0) & 0xff;
  buf[1] = (v >> 8) & 0xff;
  buf[2] = (v >> 16) & 0xff;
  buf[3] = (v >> 24) & 0xff;
}

static void
patch_uint32 (BroadwayOutput *output, guint32 v, gsize offset)
{
  guint8 *buf;

  buf = (guint8 *)output->buf->str + offset;
  buf[0] = (v >> 0) & 0xff;
  buf[1] = (v >> 8) & 0xff;
  buf[2] = (v >> 16) & 0xff;
  buf[3] = (v >> 24) & 0xff;
}


static void
write_header(BroadwayOutput *output, char op)
{
  append_uint8 (output, op);
  append_uint32 (output, output->serial++);
}

void
broadway_output_grab_pointer (BroadwayOutput *output,
                              int id,
                              gboolean owner_event)
{
  write_header (output, BROADWAY_OP_GRAB_POINTER);
  append_uint16 (output, id);
  append_bool (output, owner_event);
}

guint32
broadway_output_ungrab_pointer (BroadwayOutput *output)
{
  guint32 serial;

  serial = output->serial;
  write_header (output, BROADWAY_OP_UNGRAB_POINTER);

  return serial;
}

void
broadway_output_new_surface(BroadwayOutput *output,
                            int id, int x, int y, int w, int h)
{
  write_header (output, BROADWAY_OP_NEW_SURFACE);
  append_uint16 (output, id);
  append_uint16 (output, x);
  append_uint16 (output, y);
  append_uint16 (output, w);
  append_uint16 (output, h);
}

void
broadway_output_disconnected (BroadwayOutput *output)
{
  write_header (output, BROADWAY_OP_DISCONNECTED);
}

void
broadway_output_session (BroadwayOutput *output, guint32 token, guint32 client_id)
{
  write_header (output, BROADWAY_OP_SESSION);
  append_uint32 (output, token);
  append_uint32 (output, client_id);
}

/* Reply to a client heartbeat over the data channel; a WebSocket control pong
 * is invisible to the page's onmessage handler. */
void
broadway_output_pong_msg (BroadwayOutput *output)
{
  write_header (output, BROADWAY_OP_PONG);
}

void
broadway_output_debug_flash (BroadwayOutput *output, gboolean enabled)
{
  write_header (output, BROADWAY_OP_DEBUG_FLASH);
  append_uint8 (output, enabled ? 1 : 0);
}

/* Pin the browser's logical screen to w x h at integer scale; (0,0,0) unpins. */
void
broadway_output_debug_set_screen (BroadwayOutput *output, int w, int h, int scale)
{
  write_header (output, BROADWAY_OP_DEBUG_SET_SCREEN);
  append_uint16 (output, w);
  append_uint16 (output, h);
  append_uint8 (output, scale);
}

void
broadway_output_show_surface(BroadwayOutput *output,  int id)
{
  write_header (output, BROADWAY_OP_SHOW_SURFACE);
  append_uint16 (output, id);
}

void
broadway_output_hide_surface(BroadwayOutput *output,  int id)
{
  write_header (output, BROADWAY_OP_HIDE_SURFACE);
  append_uint16 (output, id);
}

void
broadway_output_raise_surface(BroadwayOutput *output,  int id)
{
  write_header (output, BROADWAY_OP_RAISE_SURFACE);
  append_uint16 (output, id);
}

void
broadway_output_lower_surface(BroadwayOutput *output,  int id)
{
  write_header (output, BROADWAY_OP_LOWER_SURFACE);
  append_uint16 (output, id);
}

void
broadway_output_destroy_surface(BroadwayOutput *output,  int id)
{
  write_header (output, BROADWAY_OP_DESTROY_SURFACE);
  append_uint16 (output, id);
}

void
broadway_output_roundtrip (BroadwayOutput *output,
                           int             id,
                           guint32         tag)
{
  write_header (output, BROADWAY_OP_ROUNDTRIP);
  append_uint16 (output, id);
  append_uint32 (output, tag);
}

void
broadway_output_set_show_keyboard (BroadwayOutput *output,
                                   gboolean show)
{
  write_header (output, BROADWAY_OP_SET_SHOW_KEYBOARD);
  append_uint16 (output, show);
}

void
broadway_output_set_input_region (BroadwayOutput *output,
                                  int id, int mode, BroadwayRect *rect)
{
  write_header (output, BROADWAY_OP_SET_INPUT_REGION);
  append_uint16 (output, id);
  append_uint16 (output, mode);
  append_uint16 (output, mode == 2 ? rect->x : 0);
  append_uint16 (output, mode == 2 ? rect->y : 0);
  append_uint16 (output, mode == 2 ? rect->width : 0);
  append_uint16 (output, mode == 2 ? rect->height : 0);
}

void
broadway_output_reassert_pointer (BroadwayOutput *output,
                                  int id, int x, int y)
{
  write_header (output, BROADWAY_OP_REASSERT_POINTER);
  append_uint16 (output, id);
  append_uint16 (output, x);
  append_uint16 (output, y);
}

void
broadway_output_set_clipboard (BroadwayOutput *output,
                               const char     *text,
                               gsize           len)
{
  write_header (output, BROADWAY_OP_SET_CLIPBOARD);
  append_uint32 (output, (guint32) len);
  g_string_append_len (output->buf, text, len);
}

void
broadway_output_request_clipboard (BroadwayOutput *output,
                                   guint32         id)
{
  write_header (output, BROADWAY_OP_REQUEST_CLIPBOARD);
  append_uint32 (output, id);
}

void
broadway_output_open_uri (BroadwayOutput *output,
                          const char     *uri,
                          gsize           len)
{
  write_header (output, BROADWAY_OP_OPEN_URI);
  append_uint32 (output, (guint32) len);
  g_string_append_len (output->buf, uri, len);
}

void
broadway_output_set_cursor (BroadwayOutput *output,
                            int             id,
                            const char     *name,
                            gsize           len)
{
  write_header (output, BROADWAY_OP_SET_CURSOR);
  append_uint16 (output, id);
  append_uint32 (output, (guint32) len);
  g_string_append_len (output->buf, name, len);
}

void
broadway_output_set_title (BroadwayOutput *output,
                           int             id,
                           const char     *title,
                           gsize           len)
{
  write_header (output, BROADWAY_OP_SET_TITLE);
  append_uint16 (output, id);
  append_uint32 (output, (guint32) len);
  g_string_append_len (output->buf, title, len);
}

void
broadway_output_set_icon (BroadwayOutput *output,
                          int             id,
                          const guchar   *data,
                          gsize           len)
{
  write_header (output, BROADWAY_OP_SET_ICON);
  append_uint16 (output, id);
  append_uint32 (output, (guint32) len);
  if (len > 0)
    g_string_append_len (output->buf, (const char *) data, len);
}

void
broadway_output_move_resize_surface (BroadwayOutput *output,
                                     int             id,
                                     gboolean        has_pos,
                                     int             x,
                                     int             y,
                                     gboolean        has_size,
                                     int             w,
                                     int             h)
{
  int val;

  if (!has_pos && !has_size)
    return;

  write_header (output, BROADWAY_OP_MOVE_RESIZE);
  val = (!!has_pos) | ((!!has_size) << 1);
  append_uint16 (output, id);
  append_flags (output, val);
  if (has_pos)
    {
      append_uint16 (output, x);
      append_uint16 (output, y);
    }
  if (has_size)
    {
      append_uint16 (output, w);
      append_uint16 (output, h);
    }
}

void
broadway_output_set_transient_for (BroadwayOutput *output,
                                   int             id,
                                   int             parent_id)
{
  write_header (output, BROADWAY_OP_SET_TRANSIENT_FOR);
  append_uint16 (output, id);
  append_uint16 (output, parent_id);
}

static int append_node_depth = -1;

static void
append_type (BroadwayOutput *output, guint32 type, BroadwayNode *node)
{
#ifdef DEBUG_NODE_SENDING
  g_print ("%*s%s(%d/%d)", append_node_depth*2, "", broadway_node_type_names[type], node->id, node->output_id);
  if (type == BROADWAY_NODE_TEXTURE)
    g_print (" tx=%u", node->data[4]);
  g_print ("\n");
#endif

  append_uint32 (output, type);
}

static BroadwayNode *
lookup_old_node (GHashTable *old_node_lookup,
                 guint32 id)
{
  if (old_node_lookup)
    return g_hash_table_lookup (old_node_lookup, GINT_TO_POINTER (id));

  return NULL;
}


/***********************************
 * This outputs the tree to the client, while at the same time diffing
 * against the old tree.  This allows us to avoid sending certain
 * parts.
 *
 * Reusing existing dom nodes are problematic because doing so
 * automatically inherits all their children.  There are two cases
 * where we do this:
 *
 * If the entire sub tree is identical we emit a KEEP_ALL node which
 * just reuses the entire old dom subtree.
 *
 * If a the node is unchanged (but some descendant may have changed),
 * and all parents are also unchanged, then we can just avoid
 * changing the dom node at all, and we emit a KEEP_THIS node.
 *
 ***********************************/
static void
append_node (BroadwayOutput *output,
             BroadwayNode   *node,
             GHashTable     *old_node_lookup)
{
  guint32 i;
  BroadwayNode *reused_node;

  append_node_depth++;

  reused_node = lookup_old_node (old_node_lookup, node->id);
  if (reused_node)
    {
      broadway_node_mark_deep_consumed (reused_node, TRUE);
      append_type (output, BROADWAY_NODE_REUSE, node);
      append_uint32 (output, node->output_id);
    }
  else
    {
      append_type (output, node->type, node);
      append_uint32 (output, node->output_id);
      for (i = 0; i < node->n_data; i++)
        append_uint32 (output, node->data[i]);
      for (i = 0; i < node->n_children; i++)
        append_node (output,
                     node->children[i],
                     old_node_lookup);
    }

  append_node_depth--;
}

static gboolean
should_reuse_node (BroadwayOutput *output,
                   BroadwayNode   *node,
                   BroadwayNode   *old_node)
{
  int i;
  guint32 new_texture;

  if (old_node->reused)
    return FALSE;

  if (node->type != old_node->type)
    return FALSE;

  if (broadway_node_equal (node, old_node))
    return TRUE;

  switch (node->type) {
  case BROADWAY_NODE_TRANSFORM:
#ifdef DEBUG_NODE_SENDING
   g_print ("Patching transform node %d/%d\n",
            old_node->id, old_node->output_id);
#endif
    append_uint32 (output, BROADWAY_NODE_OP_PATCH_TRANSFORM);
    append_uint32 (output, old_node->output_id);
    for (i = 0; i < node->n_data; i++)
      append_uint32 (output, node->data[i]);
    return TRUE;

  case BROADWAY_NODE_TEXTURE:
    /* Check that the size, etc is the same */
    for (i = 0; i < 4; i++)
      if (node->data[i] != old_node->data[i])
        return FALSE;

    new_texture = node->data[4];

#ifdef DEBUG_NODE_SENDING
   g_print ("Patching texture node %d/%d to tx=%d\n",
            old_node->id, old_node->output_id,
            new_texture);
#endif
    append_uint32 (output, BROADWAY_NODE_OP_PATCH_TEXTURE);
    append_uint32 (output, old_node->output_id);
    append_uint32 (output, new_texture);
    return TRUE;
    break;
  default:
    return FALSE;
  }
}


static BroadwayNode *
append_node_ops (BroadwayOutput *output,
                 BroadwayNode   *node,
                 BroadwayNode   *parent,
                 BroadwayNode   *previous_sibling,
                 BroadwayNode   *old_node,
                 GHashTable     *old_node_lookup)
{
  BroadwayNode *reused_node;
  guint32 i;

  /* Maybe can be reused from the last tree. */
  reused_node = lookup_old_node (old_node_lookup, node->id);
  if (reused_node)
    {
      g_assert (node == reused_node);
      g_assert (reused_node->reused);
      g_assert (!reused_node->consumed); /* Should only be once in the tree, and not consumed otherwise */

      broadway_node_mark_deep_consumed (reused_node, TRUE);

      if (node == old_node)
        {
          /* The node in the old tree at the current position is the same, so
             we need to do nothing, just don't delete it (which we won't since
             its marked used) */
        }
      else
        {
          /* We can reuse it, bu it comes from a different place or
             order, if so we need to move it in place */
#ifdef DEBUG_NODE_SENDING
          g_print ("Move old node %d/%d to parent %d/%d after %d/%d\n",
                   reused_node->id, reused_node->output_id,
                   parent ? parent->id : 0,
                   parent ? parent->output_id : 0,
                   previous_sibling ? previous_sibling->id : 0,
                   previous_sibling ? previous_sibling->output_id : 0);
#endif
          append_uint32 (output, BROADWAY_NODE_OP_MOVE_AFTER_CHILD);
          append_uint32 (output, parent ? parent->output_id : 0);
          append_uint32 (output, previous_sibling ? previous_sibling->output_id : 0);
          append_uint32 (output, reused_node->output_id);
        }

      return reused_node;
    }

  /* If the next node in place is shallowly equal (but not necessarily
   * deep equal) we reuse it and tweak its children as needed.
   * Except we avoid this for reused node as those make more sense to reuse deeply.
   */

  if (old_node && should_reuse_node (output, node, old_node))
    {
      int old_i = 0;
      BroadwayNode *last_child = NULL;

      old_node->consumed = TRUE; // Don't reuse again

      // We rewrite this new node as it now represents the old node in the browser
      node->output_id = old_node->output_id;

      /* However, we might need to rewrite then children of old_node */
      for (i = 0; i < node->n_children; i++)
        {
          BroadwayNode *child = node->children[i];

          /* Find the next (or first) non-consumed old child, if any */
          while (old_i < old_node->n_children &&
                 old_node->children[old_i]->consumed)
            old_i++;

          last_child =
            append_node_ops (output,
                             child,
                             node, /* parent */
                             last_child,
                             (old_i < old_node->n_children) ? old_node->children[old_i] : NULL,
                             old_node_lookup);
        }

      /* Remaining old nodes are either reused elsewhere, or end up marked not consumed so are deleted at the end */
      return old_node;
    }

  /* Fallback to create a new tree */
#ifdef DEBUG_NODE_SENDING
   g_print ("Insert nodes in parent %d/%d, after sibling %d/%d\n",
            parent ? parent->id : 0,
            parent ? parent->output_id : 0,
            previous_sibling ? previous_sibling->id : 0,
            previous_sibling ? previous_sibling->output_id : 0);
#endif
   append_uint32 (output, BROADWAY_NODE_OP_INSERT_NODE);
   append_uint32 (output, parent ? parent->output_id : 0);
   append_uint32 (output, previous_sibling ? previous_sibling->output_id : 0);

   append_node(output, node, old_node_lookup);

   return node;
}

/* Remove non-consumed nodes */
static void
append_node_removes (BroadwayOutput *output,
                     BroadwayNode   *node)
{
  // TODO: Use an array of nodes instead
  if (!node->consumed)
    {
#ifdef DEBUG_NODE_SENDING_REMOVE
          g_print ("Remove old node non-consumed node %d/%d\n",
                   node->id, node->output_id);
#endif
      append_uint32 (output, BROADWAY_NODE_OP_REMOVE_NODE);
      append_uint32 (output, node->output_id);
    }

  for (int i = 0; i < node->n_children; i++)
    append_node_removes (output, node->children[i]);
}



void
broadway_output_surface_set_nodes (BroadwayOutput *output,
                                   int             id,
                                   BroadwayNode   *root,
                                   BroadwayNode   *old_root,
                                   GHashTable     *old_node_lookup)
{
  gsize header_pos, size_pos, start, end;
  guint32 saved_serial;


  if (old_root)
    {
      broadway_node_mark_deep_consumed (old_root, FALSE);
      broadway_node_mark_deep_reused (old_root, FALSE);
      /* This will modify children of old_root if any are shared */
      broadway_node_mark_deep_reused (root, TRUE);
    }

  header_pos = output->buf->len;
  saved_serial = output->serial;

  write_header (output, BROADWAY_OP_SET_NODES);

  append_uint16 (output, id);

  size_pos = output->buf->len;
  append_uint32 (output, 0);

  start = output->buf->len;
#ifdef DEBUG_NODE_SENDING
  g_print ("====== node ops for surface %d =======\n", id);
#endif
  append_node_ops (output, root, NULL, NULL, old_root, old_node_lookup);
  if (old_root)
    append_node_removes (output, old_root);
  end = output->buf->len;

  /* No ops: the client tree already matches, so drop the empty SET_NODES
   * rather than send a no-op. Roll the serial back too. */
  if (end == start)
    {
      g_string_set_size (output->buf, header_pos);
      output->serial = saved_serial;
      return;
    }

  patch_uint32 (output, (end - start) / 4, size_pos);
}

void
broadway_output_upload_texture (BroadwayOutput *output,
                                guint32 id,
                                GBytes *texture)
{
  gsize len = g_bytes_get_size (texture);
  write_header (output, BROADWAY_OP_UPLOAD_TEXTURE);
  append_uint32 (output, id);
  append_uint32 (output, (guint32)len);
  g_string_append_len (output->buf, g_bytes_get_data (texture, NULL), len);
}

void
broadway_output_release_texture (BroadwayOutput *output,
                                 guint32 id)
{
  write_header (output, BROADWAY_OP_RELEASE_TEXTURE);
  append_uint32 (output, id);
}
