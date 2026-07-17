/* GDK - The GIMP Drawing Kit
 * gdkdisplay-broadway.c
 * 
 * Copyright 2001 Sun Microsystems Inc.
 * Copyright (C) 2004 Nokia Corporation
 *
 * Erwann Chenede <erwann.chenede@sun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gdkdisplay-broadway.h"
#include "broadway-env.h"   /* broadway_migrate_legacy_env () */

#include "gdkcairocontext-broadway.h"
#include "gdkdisplay.h"
#include "gdkeventsource.h"
#include "gdkmonitor-broadway.h"
#include "gdkseatdefaultprivate.h"
#include "gdkdevice-broadway.h"
#include "gdkdeviceprivate.h"
#include <gdk/gdktextureprivate.h>
#include <gdk/gdkcolorstateprivate.h>
#include "gdkprivate.h"
#include "gdksurface-broadway.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>

static void   gdk_broadway_display_dispose            (GObject            *object);
static void   gdk_broadway_display_finalize           (GObject            *object);

#if 0
#define DEBUG_WEBSOCKETS 1
#endif

G_DEFINE_TYPE (GdkBroadwayDisplay, gdk_broadway_display, GDK_TYPE_DISPLAY)

/* Content-based texture dedup cache (see gdkdisplay-broadway.h). Tier 2 beneath
 * the renderer's node-level reuse: catches re-rasterized-but-identical pixels
 * (text moved by scroll, re-hovered rows, repeated icons) that get a fresh
 * GdkTexture object and would otherwise re-encode + re-upload the same PNG. */
#define BROADWAY_CONTENT_CACHE_MAX        4096       /* distinct cached textures */
#define BROADWAY_CONTENT_CACHE_MAX_PIXELS (512 * 512) /* skip dedup above this area */

typedef struct {
  guint64 hash;     /* FNV-1a-64 of downloaded raw pixels */
  guint64 hash2;    /* independent second hash of the same bytes; a single 64-bit
                     * hash colliding would alias unrelated images for the LRU's
                     * lifetime, so equality demands both */
  int     width;
  int     height;
  int     format;   /* (int) native GdkMemoryFormat: the wire PNG encodes at the
                     * texture's native depth, so same downloads != same wire bytes */
  GdkColorState *color_state; /* native color state, also wire-encoded. Borrowed
                               * in lookup keys; entries own a ref */
} ContentKey;

typedef struct {
  ContentKey key;       /* embedded; the hash-table key points at this */
  guint32    id;        /* broadway texture id this content was uploaded under */
  GList     *lru_link;  /* this entry's link in content_texture_lru */
  guint      refcount;  /* live GdkTextures sharing this id; only refcount==0 is evictable */
} ContentCacheEntry;

/* color_state is deliberately not hashed: distinct-but-equal GdkColorState
 * objects must land in the same bucket for content_key_equal to see them. */
static guint
content_key_hash (gconstpointer v)
{
  const ContentKey *k = v;

  return (guint) (k->hash ^ (k->hash >> 32) ^
                  k->hash2 ^ (k->hash2 >> 32) ^
                  ((guint64) k->width << 16) ^ (guint) k->height ^
                  ((guint64) k->format << 8));
}

static gboolean
content_key_equal (gconstpointer a,
                   gconstpointer b)
{
  const ContentKey *ka = a;
  const ContentKey *kb = b;

  return ka->hash == kb->hash && ka->hash2 == kb->hash2 &&
         ka->width == kb->width && ka->height == kb->height &&
         ka->format == kb->format &&
         gdk_color_state_equal (ka->color_state, kb->color_state);
}

/* Hash the texture's raw pixels. Returns FALSE (skip dedup) for very large
 * textures, whose download+hash cost is not worth it and which rarely re-upload
 * identically. gdk_texture_download() always yields GDK_MEMORY_DEFAULT
 * (premultiplied BGRA, 4 bpp) regardless of native format, so hash that - but
 * key on the NATIVE format and color state too: the wire PNG (gdkpng.c) encodes
 * those, so identical 8-bit sRGB downloads can still differ on the wire. */
static gboolean
content_key_for_texture (GdkTexture *texture,
                         ContentKey *key)
{
  int width = gdk_texture_get_width (texture);
  int height = gdk_texture_get_height (texture);
  gsize stride, size, i, n_words;
  guchar *data;
  const guchar *p;
  guint64 h, h2;

  if (width <= 0 || height <= 0 ||
      (gint64) width * height > BROADWAY_CONTENT_CACHE_MAX_PIXELS)
    return FALSE;

  stride = (gsize) width * 4;
  size = stride * height;
  data = g_malloc (size);
  gdk_texture_download (texture, data, stride);

  /* FNV-1a-64, 8 bytes per multiply. The buffer is one contiguous g_malloc, so
   * walk it as 64-bit words (memcpy: alignment/aliasing-safe) with a <8-byte tail.
   * h2 is an MMIX-LCG mix over the same words: a second independent hash in the
   * same pass, instead of keeping full pixel copies for byte compares. */
  h = 1469598103934665603ULL;
  h2 = 0;
  n_words = size / 8;
  p = data;
  for (i = 0; i < n_words; i++, p += 8)
    {
      guint64 w;
      memcpy (&w, p, 8);
      h = (h ^ w) * 1099511628211ULL;
      h2 = (h2 + w) * 6364136223846793005ULL + 1442695040888963407ULL;
    }
  for (i = n_words * 8; i < size; i++)
    {
      h = (h ^ data[i]) * 1099511628211ULL;
      h2 = (h2 + data[i]) * 6364136223846793005ULL + 1442695040888963407ULL;
    }

  g_free (data);

  key->hash = h;
  key->hash2 = h2;
  key->width = width;
  key->height = height;
  key->format = (int) gdk_texture_get_format (texture);
  key->color_state = gdk_texture_get_color_state (texture); /* borrowed; texture outlives the lookup */
  return TRUE;
}

static void
content_cache_touch (GdkBroadwayDisplay *display,
                     ContentCacheEntry  *entry)
{
  g_queue_unlink (&display->content_texture_lru, entry->lru_link);
  g_queue_push_head_link (&display->content_texture_lru, entry->lru_link);
}

/* Release ids over the cap, but ONLY entries with no live GdkTexture owner
 * (refcount == 0). Releasing an id a live texture still references would leave a
 * dangling broadway-data and a node pointing at a freed browser texture, so live
 * entries are kept even past the cap (bounded by GTK's own live-texture set). */
static void
content_cache_evict_if_needed (GdkBroadwayDisplay *display)
{
  GList *link = display->content_texture_lru.tail;

  while (display->content_texture_count > BROADWAY_CONTENT_CACHE_MAX && link != NULL)
    {
      ContentCacheEntry *entry = link->data;
      GList *prev = link->prev;

      if (entry->refcount == 0)
        {
          g_hash_table_remove (display->content_texture_cache, &entry->key);
          g_queue_delete_link (&display->content_texture_lru, link);
          gdk_broadway_server_release_texture (display->server, entry->id);
          gdk_color_state_unref (entry->key.color_state);
          g_free (entry);
          display->content_texture_count--;
        }
      link = prev;
    }
}

static ContentCacheEntry *
content_cache_insert (GdkBroadwayDisplay *display,
                      const ContentKey   *key,
                      guint32             id)
{
  ContentCacheEntry *entry = g_new0 (ContentCacheEntry, 1);

  entry->key = *key;
  gdk_color_state_ref (entry->key.color_state); /* entry outlives the texture */
  entry->id = id;
  entry->refcount = 1;
  g_queue_push_head (&display->content_texture_lru, entry);
  entry->lru_link = g_queue_peek_head_link (&display->content_texture_lru);
  g_hash_table_insert (display->content_texture_cache, &entry->key, entry);
  display->content_texture_count++;

  content_cache_evict_if_needed (display);
  return entry;
}

/* Free local memory only; broadwayd releases all of a client's textures on
 * disconnect, so no per-id release is needed (and the server may be gone). */
static void
content_cache_destroy_all (GdkBroadwayDisplay *display)
{
  GList *l;

  if (display->content_texture_cache == NULL)
    return;

  for (l = display->content_texture_lru.head; l != NULL; l = l->next)
    {
      ContentCacheEntry *entry = l->data;

      gdk_color_state_unref (entry->key.color_state);
      g_free (entry);
    }
  g_queue_clear (&display->content_texture_lru);
  g_hash_table_destroy (display->content_texture_cache);
  display->content_texture_cache = NULL;
  display->content_texture_count = 0;
}

static void
gdk_broadway_display_init (GdkBroadwayDisplay *display)
{
  gdk_display_set_input_shapes (GDK_DISPLAY (display), FALSE);

  display->id_ht = g_hash_table_new (NULL, NULL);

  display->content_texture_cache = g_hash_table_new (content_key_hash, content_key_equal);
  g_queue_init (&display->content_texture_lru);
  display->content_texture_count = 0;

  display->monitor = g_object_new (GDK_TYPE_BROADWAY_MONITOR,
                                   "display", display,
                                   NULL);
  gdk_monitor_set_manufacturer (display->monitor, "browser");
  gdk_monitor_set_model (display->monitor, "0");
  display->scale_factor = 1;
  gdk_monitor_set_geometry (display->monitor, &(GdkRectangle) { 0, 0, 1024, 768 });
  gdk_monitor_set_physical_size (display->monitor, 1024 * 25.4 / 96, 768 * 25.4 / 96);
  gdk_monitor_set_scale_factor (display->monitor, 1);
}

static void
gdk_event_init (GdkDisplay *display)
{
  GdkBroadwayDisplay *broadway_display;

  broadway_display = GDK_BROADWAY_DISPLAY (display);
  broadway_display->event_source = _gdk_broadway_event_source_new (display);
}

void
_gdk_broadway_display_size_changed (GdkDisplay                      *display,
                                    BroadwayInputScreenResizeNotify *msg)
{
  GdkBroadwayDisplay *broadway_display = GDK_BROADWAY_DISPLAY (display);
  GdkMonitor *monitor;
  GdkRectangle current_size;
  GList *toplevels, *l;
  gboolean scale_changed;

  monitor = broadway_display->monitor;
  gdk_monitor_get_geometry (monitor, &current_size);

  if (msg->width == current_size.width &&
      msg->height == current_size.height &&
      (msg->scale == broadway_display->scale_factor ||
       broadway_display->fixed_scale))
    return;

  scale_changed = !broadway_display->fixed_scale &&
                  msg->scale != broadway_display->scale_factor;

  if (!broadway_display->fixed_scale)
    broadway_display->scale_factor = msg->scale;

  gdk_monitor_set_geometry (monitor, &(GdkRectangle) { 0, 0, msg->width, msg->height });
  gdk_monitor_set_scale_factor (monitor, msg->scale);
  gdk_monitor_set_physical_size (monitor, msg->width * 25.4 / 96, msg->height * 25.4 / 96);

  toplevels =  broadway_display->toplevels;
  for (l = toplevels; l != NULL; l = l->next)
    {
      GdkBroadwaySurface *toplevel = l->data;
      GdkSurface *surface = GDK_SURFACE (toplevel);

      if (toplevel->maximized)
        gdk_broadway_surface_move_resize (surface,
                                          0, 0,
                                          msg->width, msg->height);
      else if (toplevel->visible)
        {
          /* Re-center a non-maximized toplevel on the resized screen, so a zoom
           * change (which shrinks/grows the logical screen) can't leave a
           * centered window stranded off-screen. */
          int x = (msg->width - surface->width) / 2;
          int y = (msg->height - surface->height) / 2;

          gdk_broadway_surface_move_resize (surface, MAX (0, x), MAX (0, y),
                                            surface->width, surface->height);
        }

      /* Scale changed: force full redraw so the renderer re-rasterizes cached
       * textures at the new scale (pairs with the cache-drop in
       * gskbroadwayrenderer.c). Else static surfaces stay blurry. */
      if (scale_changed)
        gdk_surface_invalidate_rect (surface, NULL);
    }
}

static void
freeze_surface_for_suspend (gpointer key, gpointer value, gpointer user_data)
{
  GdkSurface *surface = value;
  GdkBroadwaySurface *impl = GDK_BROADWAY_SURFACE (surface);

  if (impl->suspend_frozen)
    return;
  impl->suspend_frozen = TRUE;
  gdk_surface_freeze_updates (surface);
}

static void
thaw_surface_for_suspend (gpointer key, gpointer value, gpointer user_data)
{
  GdkSurface *surface = value;
  GdkBroadwaySurface *impl = GDK_BROADWAY_SURFACE (surface);

  if (!impl->suspend_frozen)
    return;
  impl->suspend_frozen = FALSE;
  gdk_surface_thaw_updates (surface);
}

/* The browser tab went hidden (suspended=TRUE) or visible (FALSE). Freeze every
 * surface's update cycle so the frame clock skips its paint phase: the GSK
 * Broadway renderer never runs, so no frames stream while hidden AND its diff
 * baseline (last_root) stays put - the first paint after thaw is an incremental
 * delta, not a full resync. The per-surface suspend_frozen flag keeps our extra
 * freeze/thaw balanced even if surfaces are created or destroyed meanwhile (we
 * only ever thaw what we actually froze). Idempotent, so a duplicate
 * SUSPEND/RESUME from a reconnecting client is a no-op. */
void
_gdk_broadway_display_set_suspended (GdkDisplay *display,
                                     gboolean    suspended)
{
  GdkBroadwayDisplay *broadway_display = GDK_BROADWAY_DISPLAY (display);

  suspended = !!suspended;
  if (broadway_display->suspended == suspended)
    return;
  broadway_display->suspended = suspended;

  g_hash_table_foreach (broadway_display->id_ht,
                        suspended ? freeze_surface_for_suspend
                                  : thaw_surface_for_suspend,
                        NULL);
}

static GdkDevice *
create_core_pointer (GdkDisplay *display)
{
  return g_object_new (GDK_TYPE_BROADWAY_DEVICE,
                       "name", "Core Pointer",
                       "source", GDK_SOURCE_MOUSE,
                       "has-cursor", TRUE,
                       "display", display,
                       NULL);
}

static GdkDevice *
create_core_keyboard (GdkDisplay *display)
{
  return g_object_new (GDK_TYPE_BROADWAY_DEVICE,
                       "name", "Core Keyboard",
                       "source", GDK_SOURCE_KEYBOARD,
                       "has-cursor", FALSE,
                       "display", display,
                       NULL);
}

static GdkDevice *
create_pointer (GdkDisplay *display)
{
  return g_object_new (GDK_TYPE_BROADWAY_DEVICE,
                       "name", "Pointer",
                       "source", GDK_SOURCE_MOUSE,
                       "has-cursor", TRUE,
                       "display", display,
                       NULL);
}

static GdkDevice *
create_keyboard (GdkDisplay *display)
{
  return g_object_new (GDK_TYPE_BROADWAY_DEVICE,
                       "name", "Keyboard",
                       "source", GDK_SOURCE_KEYBOARD,
                       "has-cursor", FALSE,
                       "display", display,
                       NULL);
}

static GdkDevice *
create_touchscreen (GdkDisplay *display)
{
  return g_object_new (GDK_TYPE_BROADWAY_DEVICE,
                       "name", "Touchscreen",
                       "source", GDK_SOURCE_TOUCHSCREEN,
                       "has-cursor", FALSE,
                       "display", display,
                       NULL);
}

GdkDisplay *
_gdk_broadway_display_open (const char *display_name)
{
  GdkDisplay *display;
  GdkBroadwayDisplay *broadway_display;
  GError *error = NULL;
  GdkSeat *seat;

  broadway_migrate_legacy_env ();

  display = g_object_new (GDK_TYPE_BROADWAY_DISPLAY, NULL);
  broadway_display = GDK_BROADWAY_DISPLAY (display);

  broadway_display->core_pointer = create_core_pointer (display);
  broadway_display->core_keyboard = create_core_keyboard (display);
  broadway_display->pointer = create_pointer (display);
  broadway_display->keyboard = create_keyboard (display);
  broadway_display->touchscreen = create_touchscreen (display);

  _gdk_device_set_associated_device (broadway_display->core_pointer, broadway_display->core_keyboard);
  _gdk_device_set_associated_device (broadway_display->core_keyboard, broadway_display->core_pointer);
  _gdk_device_set_associated_device (broadway_display->pointer, broadway_display->core_pointer);
  _gdk_device_set_associated_device (broadway_display->keyboard, broadway_display->core_keyboard);
  _gdk_device_set_associated_device (broadway_display->touchscreen, broadway_display->core_pointer);
  _gdk_device_add_physical_device (broadway_display->core_pointer, broadway_display->touchscreen);

  seat = gdk_seat_default_new_for_logical_pair (broadway_display->core_pointer,
                                                broadway_display->core_keyboard);

  gdk_display_add_seat (display, seat);
  gdk_seat_default_add_physical_device (GDK_SEAT_DEFAULT (seat), broadway_display->pointer);
  gdk_seat_default_add_physical_device (GDK_SEAT_DEFAULT (seat), broadway_display->keyboard);
  gdk_seat_default_add_physical_device (GDK_SEAT_DEFAULT (seat), broadway_display->touchscreen);
  g_object_unref (seat);

  gdk_event_init (display);

  if (display_name == NULL)
    display_name = g_getenv ("BROTWAY_DISPLAY");

  broadway_display->server = _gdk_broadway_server_new (display, display_name, &error);
  if (broadway_display->server == NULL)
    {
      GDK_DEBUG (MISC, "Unable to init Broadway server: %s", error->message);
      g_error_free (error);
      return NULL;
    }

  display->clipboard = gdk_broadway_clipboard_new (display);

  _gdk_broadway_display_init_settings (display);

  g_signal_emit_by_name (display, "opened");

  return display;
}

static const char *
gdk_broadway_display_get_name (GdkDisplay *display)
{
  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);

  return (char *) "Broadway";
}

static void
gdk_broadway_display_beep (GdkDisplay *display)
{
  g_return_if_fail (GDK_IS_DISPLAY (display));
}

static void
gdk_broadway_display_sync (GdkDisplay *display)
{
  GdkBroadwayDisplay *broadway_display = GDK_BROADWAY_DISPLAY (display);

  g_return_if_fail (GDK_IS_BROADWAY_DISPLAY (display));

  _gdk_broadway_server_sync (broadway_display->server);
}

static void
gdk_broadway_display_flush (GdkDisplay *display)
{
  GdkBroadwayDisplay *broadway_display = GDK_BROADWAY_DISPLAY (display);

  g_return_if_fail (GDK_IS_BROADWAY_DISPLAY (display));

  _gdk_broadway_server_flush (broadway_display->server);
}

static void
gdk_broadway_display_dispose (GObject *object)
{
  GdkBroadwayDisplay *self = GDK_BROADWAY_DISPLAY (object);

  if (self->event_source)
    {
      g_source_destroy (self->event_source);
      g_source_unref (self->event_source);
      self->event_source = NULL;
    }
  if (self->monitors)
    {
      g_list_store_remove_all (self->monitors);
      g_clear_object (&self->monitors);
    }

  G_OBJECT_CLASS (gdk_broadway_display_parent_class)->dispose (object);
}

static void
gdk_broadway_display_finalize (GObject *object)
{
  GdkBroadwayDisplay *broadway_display = GDK_BROADWAY_DISPLAY (object);

  /* Keymap */
  if (broadway_display->keymap)
    g_object_unref (broadway_display->keymap);

  _gdk_broadway_cursor_display_finalize (GDK_DISPLAY(broadway_display));

  _gdk_broadway_display_finalize_settings (GDK_DISPLAY (broadway_display));

  content_cache_destroy_all (broadway_display);

  g_object_unref (broadway_display->monitor);

  G_OBJECT_CLASS (gdk_broadway_display_parent_class)->finalize (object);
}

static void
gdk_broadway_display_notify_startup_complete (GdkDisplay  *display,
					      const char *startup_id)
{
}

static gulong
gdk_broadway_display_get_next_serial (GdkDisplay *display)
{
  GdkBroadwayDisplay *broadway_display;
  broadway_display = GDK_BROADWAY_DISPLAY (display);

  return _gdk_broadway_server_get_next_serial (broadway_display->server);
}

void
gdk_broadway_display_show_keyboard (GdkBroadwayDisplay *display)
{
  g_return_if_fail (GDK_IS_BROADWAY_DISPLAY (display));

  _gdk_broadway_server_set_show_keyboard (display->server, TRUE);
}

void
gdk_broadway_display_hide_keyboard (GdkBroadwayDisplay *display)
{
  g_return_if_fail (GDK_IS_BROADWAY_DISPLAY (display));

  _gdk_broadway_server_set_show_keyboard (display->server, FALSE);
}

/*
 * gdk_broadway_display_show_uri:
 * @display: a broadway display
 * @uri: the URI to open
 *
 * Asks the browser viewing the Broadway session to open @uri in a new tab.
 * There is no system URI handler in a headless Broadway/container session, so
 * this routes the URI over the protocol to the browser instead.
 */
void
gdk_broadway_display_show_uri (GdkBroadwayDisplay *display,
                               const char         *uri)
{
  g_return_if_fail (GDK_IS_BROADWAY_DISPLAY (display));
  g_return_if_fail (uri != NULL);

  _gdk_broadway_server_open_uri (display->server, uri);
}

/**
 * gdk_broadway_display_set_surface_scale:
 * @display: (type GdkBroadwayDisplay): the display
 * @scale: The new scale value, as an integer >= 1
 *
 * Forces a specific window scale for all windows on this display,
 * instead of using the default or user configured scale. This
 * is can be used to disable scaling support by setting @scale to
 * 1, or to programmatically set the window scale.
 *
 * Once the scale is set by this call it will not change in
 * response to later user configuration changes.
 *
 * Since: 4.4
 * Deprecated: 4.18: The Broadway backend will be removed in GTK 5
 */
void
gdk_broadway_display_set_surface_scale (GdkDisplay *display,
                                        int         scale)
{
  GdkBroadwayDisplay *self;

  g_return_if_fail (GDK_IS_BROADWAY_DISPLAY (display));
  g_return_if_fail (scale > 0);

  self = GDK_BROADWAY_DISPLAY (display);

  self->scale_factor = scale;
  self->fixed_scale = TRUE;
  gdk_monitor_set_scale_factor (self->monitor, scale);
}

/**
 * gdk_broadway_display_get_surface_scale:
 * @display: (type GdkBroadwayDisplay): the display
 *
 * Gets the surface scale that was previously set by the client or
 * gdk_broadway_display_set_surface_scale().
 *
 * Returns: the scale for surfaces
 *
 * Since: 4.4
 * Deprecated: 4.18: The Broadway backend will be removed in GTK 5
 */
int
gdk_broadway_display_get_surface_scale (GdkDisplay *display)
{
  GdkBroadwayDisplay *self;

  g_return_val_if_fail (GDK_IS_BROADWAY_DISPLAY (display), 1);

  self = GDK_BROADWAY_DISPLAY (display);

  return self->scale_factor;
}

static GListModel *
gdk_broadway_display_get_monitors (GdkDisplay *display)
{
  GdkBroadwayDisplay *self = GDK_BROADWAY_DISPLAY (display);

  if (self->monitors == NULL)
    {
      self->monitors = g_list_store_new (GDK_TYPE_MONITOR);
      g_list_store_append (self->monitors, self->monitor);
    }

  return G_LIST_MODEL (self->monitors);
}

typedef struct {
  int id;
  GdkDisplay *display;
  GList *textures;
  ContentCacheEntry *entry;  /* non-NULL: a cache entry shares this id (refcounted);
                              * NULL: legacy/oversized, this object owns the id */
} BroadwayTextureData;

static void
broadway_texture_data_free (BroadwayTextureData *data)
{
  GdkBroadwayDisplay *broadway_display = GDK_BROADWAY_DISPLAY (data->display);

  if (data->entry != NULL)
    data->entry->refcount--;   /* keep the id cached for reuse; released on eviction */
  else
    gdk_broadway_server_release_texture (broadway_display->server, data->id);
  g_object_unref (data->display);
  g_free (data);
}

static void
attach_broadway_data (GdkDisplay        *display,
                      GdkTexture        *texture,
                      guint32            id,
                      ContentCacheEntry *entry)
{
  BroadwayTextureData *data = g_new0 (BroadwayTextureData, 1);

  data->id = id;
  data->display = g_object_ref (display);
  data->entry = entry;
  g_object_set_data_full (G_OBJECT (texture), "broadway-data", data,
                          (GDestroyNotify) broadway_texture_data_free);
}

guint32
gdk_broadway_display_ensure_texture (GdkDisplay *display,
                                     GdkTexture *texture)
{
  GdkBroadwayDisplay *broadway_display = GDK_BROADWAY_DISPLAY (display);
  BroadwayTextureData *data;
  ContentCacheEntry *entry;
  ContentKey key;
  guint32 id;

  /* Fast path: this exact object was already uploaded - no re-hash. */
  data = g_object_get_data (G_OBJECT (texture), "broadway-data");
  if (data != NULL)
    {
      /* Keep a still-referenced texture hot so a per-frame redraw isn't evicted
       * early. entry is NULL on the legacy/oversized path (no LRU link). */
      if (data->entry != NULL)
        content_cache_touch (broadway_display, data->entry);
      return data->id;
    }

  if (content_key_for_texture (texture, &key))
    {
      entry = g_hash_table_lookup (broadway_display->content_texture_cache, &key);
      if (entry != NULL)
        {
          /* Identical pixels already on the wire: reuse the id, skip the upload. */
          entry->refcount++;
          content_cache_touch (broadway_display, entry);
          attach_broadway_data (display, texture, entry->id, entry);
          return entry->id;
        }

      id = gdk_broadway_server_upload_texture (broadway_display->server, texture);
      entry = content_cache_insert (broadway_display, &key, id);
      attach_broadway_data (display, texture, id, entry);
      return id;
    }

  /* Oversized / un-hashable: legacy path, the object owns the id. */
  id = gdk_broadway_server_upload_texture (broadway_display->server, texture);
  attach_broadway_data (display, texture, id, NULL);
  return id;
}

static gboolean
flush_idle (gpointer data)
{
  GdkDisplay *display = data;
  GdkBroadwayDisplay *broadway_display = GDK_BROADWAY_DISPLAY (display);

  broadway_display->idle_flush_id = 0;
  gdk_display_flush (display);

  return FALSE;
}

void
gdk_broadway_display_flush_in_idle (GdkDisplay *display)
{
  GdkBroadwayDisplay *broadway_display = GDK_BROADWAY_DISPLAY (display);

  if (broadway_display->idle_flush_id == 0)
    {
      broadway_display->idle_flush_id = g_idle_add (flush_idle, g_object_ref (display));
      gdk_source_set_static_name_by_id (broadway_display->idle_flush_id, "[gtk] flush_idle");
    }
}


static void
gdk_broadway_display_class_init (GdkBroadwayDisplayClass * class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GdkDisplayClass *display_class = GDK_DISPLAY_CLASS (class);

  object_class->dispose = gdk_broadway_display_dispose;
  object_class->finalize = gdk_broadway_display_finalize;

  display_class->toplevel_type = GDK_TYPE_BROADWAY_TOPLEVEL;
  display_class->popup_type = GDK_TYPE_BROADWAY_POPUP;
  display_class->cairo_context_type = GDK_TYPE_BROADWAY_CAIRO_CONTEXT;

  display_class->get_name = gdk_broadway_display_get_name;
  display_class->beep = gdk_broadway_display_beep;
  display_class->sync = gdk_broadway_display_sync;
  display_class->flush = gdk_broadway_display_flush;
  display_class->queue_events = _gdk_broadway_display_queue_events;

  display_class->get_next_serial = gdk_broadway_display_get_next_serial;
  display_class->notify_startup_complete = gdk_broadway_display_notify_startup_complete;
  display_class->get_keymap = _gdk_broadway_display_get_keymap;

  display_class->get_monitors = gdk_broadway_display_get_monitors;
  display_class->get_setting = _gdk_broadway_display_get_setting;
}
