/*
 * gdkdisplay-broadway.h
 * 
 * Copyright 2001 Sun Microsystems Inc. 
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

#pragma once

#include "gdkbroadwaydisplay.h"

#include "gdkdisplayprivate.h"
#include "gdkkeys.h"
#include "gdksurface.h"
#include "gdkbroadway-server.h"
#include "gdkmonitorprivate.h"

G_BEGIN_DECLS

struct _GdkBroadwayDisplay
{
  GdkDisplay parent_instance;

  GHashTable *id_ht;
  GList *toplevels;

  GdkDevice *core_pointer;
  GdkDevice *core_keyboard;
  GdkDevice *pointer;
  GdkDevice *keyboard;
  GdkDevice *touchscreen;

  GSource *event_source;

  /* Keyboard related information */
  GdkKeymap *keymap;

  GdkBroadwayServer *server;
  gpointer move_resize_data;

  GListStore *monitors;
  GdkMonitor *monitor;
  int scale_factor;
  gboolean fixed_scale;

  /* Cross-time content cache: dedups texture *pixels* across GdkTexture object
   * lifetimes. On scroll/hover the source GdkTexture is freed and a fresh one
   * with identical pixels is made next frame, so object-identity dedup misses.
   * This keeps the browser-side texture id alive until LRU eviction so the
   * identical pixels are not re-uploaded. */
  GHashTable *content_texture_cache; /* ContentKey* -> ContentCacheEntry* */
  GQueue      content_texture_lru;   /* MRU head, LRU tail; node->data == entry */
  gsize       content_texture_count;

  guint idle_flush_id;

  gboolean suspended; /* tab hidden: surfaces frozen so we stream no frames */

  /* Browser prefers-color-scheme, read from broadwayd's Settings portal and
   * exposed as gtk-interface-color-scheme so plain GTK apps follow it too. */
  GDBusProxy *settings_portal; /* NULL if no portal available */
  int color_scheme;            /* GtkInterfaceColorScheme value */
  gboolean color_scheme_forced; /* BROTWAY_COLOR_SCHEME pins it; the portal is ignored */
};

struct _GdkBroadwayDisplayClass
{
  GdkDisplayClass parent_class;
};

G_END_DECLS

