/*
Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef LXUTILS_H
#define LXUTILS_H

#include <gtk/gtk.h>
#include <glib.h>
#include <locale.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include "notification.h"

#define MENU_ICON_SPACE 6

typedef enum {
  PRESS_NONE,
  PRESS_SHORT,
  PRESS_LONG
} press_t;

extern press_t pressed;
extern double press_x, press_y;

extern gboolean touch_only;

typedef struct {
    GtkWidget *da;                          /* Drawing area */
    cairo_surface_t *pixmap;                /* Pixmap to be drawn on drawing area */
    float *samples;                         /* Ring buffer of values */
    int *samp_states;                       /* Ring buffer of states used for colours */
    unsigned int ring_cursor;               /* Cursor for ring buffer */
    guint pixmap_width;                     /* Width of drawing area pixmap; also size of ring buffer; does not include border size */
    guint pixmap_height;                    /* Height of drawing area pixmap; does not include border size */
    GdkRGBA background;                     /* Graph background colour */
    GdkRGBA colours[3];                     /* Graph foreground colours - normal and throttled */
} PluginGraph;

extern char **environ;

extern GtkWindow *find_panel (GtkWidget *btn);
extern void store_layer (GtkLayerShellLayer layer);
extern void set_taskbar_icon (GtkWidget *image, const char *icon, int size);
extern void set_menu_icon (GtkWidget *image, const char *icon, int size);
extern GtkWidget *new_menu_item (const char *text, int maxlen, const char *iconname, int icon_size);
extern void update_menu_icon (GtkWidget *item, GtkWidget *image);
extern const char *get_menu_label (GtkWidget *item);
extern void append_menu_icon (GtkWidget *item, GtkWidget *image);
extern void revert_textdomain (void);

extern void graph_init (PluginGraph *graph);
extern void graph_reload (PluginGraph *graph, int icon_size, GdkRGBA background, GdkRGBA foreground, GdkRGBA throttle1, GdkRGBA throttle2);
extern void graph_new_point (PluginGraph *graph, float value, int state, char *label);
extern void graph_free (PluginGraph *graph);

extern gboolean check_menu (GtkWidget *menu);
extern void show_menu_with_kbd (GtkWidget *button, GtkWidget *menu);
extern void show_menu_with_kbd_at_xy (GtkWidget *widget, GtkWidget *menu, double x, double y);

extern void popup_window_at_button (GtkWidget *window, GtkWidget *button);
extern void close_popup (void);
extern void pass_right_click (GtkWidget *wid, double x, double y);

#define lxpanel_notify(panel,msg) wfpanel_notify(msg)
#define lxpanel_notify_clear(seq) wfpanel_notify_clear(seq)
#define lxpanel_plugin_update_menu_icon(item,icon) update_menu_icon(item,icon)
#define lxpanel_plugin_append_menu_icon(item,icon) append_menu_icon(item,icon)
#define wrap_new_menu_item(plugin,text,maxlen,icon) new_menu_item(text,maxlen,icon,plugin->icon_size)
#define wrap_set_menu_icon(plugin,image,icon) set_menu_icon(image,icon,plugin->icon_size)
#define wrap_set_taskbar_icon(plugin,image,icon) set_taskbar_icon(image,icon,plugin->icon_size)
#define wrap_get_menu_label(item) get_menu_label(item)
#define wrap_show_menu(plugin,menu) show_menu_with_kbd(plugin,menu)
#define wrap_icon_size(plugin) (plugin->icon_size)
#define wrap_is_at_bottom(plugin) (plugin->bottom)
#define wrap_popup_at_button(plugin,window,button) popup_window_at_button(window,button)

#endif

/* End of file */
/*----------------------------------------------------------------------------*/
