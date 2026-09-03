/*============================================================================
Copyright (c) 2023 Raspberry Pi
All rights reserved.

Some code taken from the lxpanel project

Copyright (c) 2006-2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
            2006-2008 Jim Huang <jserv.tw@gmail.com>
            2008 Fred Chien <fred@lxde.org>
            2009 Ying-Chun Liu (PaulLiu) <grandpaul@gmail.com>
            2009-2010 Marty Jack <martyj19@comcast.net>
            2010 Jürgen Hötzel <juergen@archlinux.org>
            2010-2011 Julien Lavergne <julien.lavergne@gmail.com>
            2012-2013 Henry Gebhardt <hsggebhardt@gmail.com>
            2012 Michael Rawson <michaelrawson76@gmail.com>
            2014 Max Krummenacher <max.oss.09@gmail.com>
            2014 SHiNE CsyFeK <csyfek@users.sourceforge.net>
            2014 Andriy Grytsenko <andrej@rep.kiev.ua>

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
============================================================================*/

#include <fcntl.h>
#include <libinput.h>
#include <libudev.h>
#include <libintl.h>
#include <linux/input.h>
#include <gtk/gtk.h>
#include <gtk-layer-shell.h>
#include <gio/gdesktopappinfo.h>

#include "plugin.h"

/*----------------------------------------------------------------------------*/
/* Macros and typedefs */
/*----------------------------------------------------------------------------*/

#define MENU_ICON_SPACE 6
#define BORDER_SIZE 1

/*----------------------------------------------------------------------------*/
/* Global data */
/*----------------------------------------------------------------------------*/

#ifdef USES_MENUCACHE
extern MenuCache *mcache_h;
#endif

press_t pressed;
double press_x, press_y;

gboolean gestures_touch_only;
gboolean is_pi_var;

GtkWindow *popwindow;
static GtkWindow *clicksink;
static int px, py, mw, mh, orient, mch;

gboolean reload;

/*----------------------------------------------------------------------------*/
/* General public API - replaces functions from lxpanel */
/*----------------------------------------------------------------------------*/

GtkWindow *find_panel (GtkWidget *btn)
{
    GtkWidget *wid = btn;
    while (!GTK_IS_WINDOW (wid) || !gtk_layer_is_layer_window (GTK_WINDOW (wid)))
    {
        if (!GTK_IS_WIDGET (wid))
            return NULL;
        wid = gtk_widget_get_parent (wid);
    }
    return GTK_WINDOW (wid);
}

gboolean in_grid (GtkWidget *btn)
{
    GtkWidget *wid = btn;
    while (!GTK_IS_WINDOW (wid) || !gtk_layer_is_layer_window (GTK_WINDOW (wid)))
    {
        if (!GTK_IS_WIDGET (wid))
            return FALSE;
        if (!g_strcmp0 (gtk_widget_get_name (wid), "grid")) return TRUE;
        wid = gtk_widget_get_parent (wid);
    }
    return FALSE;
}

gboolean panel_at_bottom (GtkWidget *btn)
{
    GtkWindow *panel = find_panel (btn);
    if (!panel) return FALSE;
    return gtk_layer_get_anchor (panel, GTK_LAYER_SHELL_EDGE_BOTTOM);
}

int get_icon_size (GtkWidget *widget)
{
    GtkWindow *panel = find_panel (widget);
    if (!panel) return 0;
    int siz = * (int *) g_object_get_data ((GObject *) panel, "icon-size");
    if (in_grid (widget)) return siz / 2;
    return siz;
}

GdkPixbuf *load_taskbar_pixbuf (GtkWidget *image, const char *icon_name)
{
    char *fname;
    GdkPixbuf *icon = NULL;
    int scale = gtk_widget_get_scale_factor (image);
    int size = get_icon_size (image);

    if (icon_name)
    {
        if (strstr (icon_name, "/"))
            icon = gdk_pixbuf_new_from_file_at_size (icon_name, size * scale, size * scale, NULL);
        else
        {
            icon = gtk_icon_theme_load_icon_for_scale (gtk_icon_theme_get_default (), icon_name,
                size, scale, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);

            // fallback for packages using obsolete icon location
            if (!icon)
            {
                fname = g_strdup_printf ("/usr/share/pixmaps/%s", icon_name);
                icon = gdk_pixbuf_new_from_file_at_size (fname, size * scale, size * scale, NULL);
                g_free (fname);
            }
        }
    }
    if (!icon)
        icon = gtk_icon_theme_load_icon_for_scale (gtk_icon_theme_get_default (), "application-x-executable",
            size, scale, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
    return icon;
}

void set_image_from_pixbuf (GtkWidget *image, GdkPixbuf *pixbuf)
{
    int scale = gtk_widget_get_scale_factor (image);
    if (scale == 1) gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
    else
    {
        cairo_surface_t *cr = gdk_cairo_surface_create_from_pixbuf (pixbuf, scale, NULL);
        gtk_image_set_from_surface (GTK_IMAGE (image), cr);
        cairo_surface_destroy (cr);
    }
}

void set_taskbar_icon (GtkWidget *image, const char *icon)
{
    GdkPixbuf *pixbuf = load_taskbar_pixbuf (image, icon);
    if (pixbuf)
    {
        set_image_from_pixbuf (image, pixbuf);
        g_object_unref (pixbuf);
    }
}

void set_menu_icon (GtkWidget *image, const char *icon, int size)
{
    if (!icon) return;
    int scale = gtk_widget_get_scale_factor (image);
    GdkPixbuf *pixbuf = gtk_icon_theme_load_icon_for_scale (gtk_icon_theme_get_default (), icon,
        (size >= 32 ? 24 : 16), scale, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
    if (pixbuf)
    {
        set_image_from_pixbuf (image, pixbuf);
        g_object_unref (pixbuf);
    }
}

GtkWidget *new_menu_item (const char *text, int maxlen, const char *iconname, int icon_size)
{
    GtkWidget *item = gtk_menu_item_new ();
    gtk_widget_set_name (item, "panelmenuitem");
    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, MENU_ICON_SPACE);
    GtkWidget *label = gtk_label_new (text);
    GtkWidget *icon = gtk_image_new ();
    set_menu_icon (icon, iconname, icon_size);

    if (maxlen)
    {
        gtk_label_set_max_width_chars (GTK_LABEL (label), maxlen);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    }

    gtk_container_add (GTK_CONTAINER (item), box);
    gtk_container_add (GTK_CONTAINER (box), icon);
    gtk_container_add (GTK_CONTAINER (box), label);

    return item;
}

void update_menu_icon (GtkWidget *item, GtkWidget *image)
{
    GtkWidget *box = gtk_bin_get_child (GTK_BIN (item));
    GList *children = gtk_container_get_children (GTK_CONTAINER (box));
    GtkWidget *img = (GtkWidget *) children->data;
    gtk_container_remove (GTK_CONTAINER (box), img);
    gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 0);
    gtk_box_reorder_child (GTK_BOX (box), image, 0);
}

const char *get_menu_label (GtkWidget *item)
{
    if (!GTK_IS_BIN (item)) return "";
    GtkWidget *box = gtk_bin_get_child (GTK_BIN (item));
    if (!box) return "";
    GList *children = gtk_container_get_children (GTK_CONTAINER (box));
    if (!children) return "";
    while (children->data)
    {
        if (GTK_IS_LABEL ((GtkWidget *) children->data))
            return gtk_label_get_text (GTK_LABEL ((GtkWidget *) children->data));
        children = children->next;
    }
    return "";
}

void append_menu_icon (GtkWidget *item, GtkWidget *image)
{
    GtkWidget *box = gtk_bin_get_child (GTK_BIN (item));
    gtk_box_pack_end (GTK_BOX (box), image, FALSE, FALSE, 0);
}

void revert_textdomain (void)
{
    textdomain (GETTEXT_PACKAGE);
}

/*----------------------------------------------------------------------------*/
/* Plugin graph */
/*----------------------------------------------------------------------------*/

/* Redraw entire graph */

static void graph_redraw (PluginGraph *graph, char *label)
{
    unsigned int fontsize, drawing_cursor, i;
    GdkPixbuf *pixbuf;

    uint scale = gtk_widget_get_scale_factor (graph->da);

    cairo_t *cr = cairo_create (graph->pixmap);
    cairo_set_line_width (cr, scale);
    
    /* Erase pixmap */
    cairo_rectangle (cr, 0, 0, graph->pixmap_width, graph->pixmap_height);
    cairo_set_source_rgba (cr, graph->background.blue, graph->background.green, graph->background.red, graph->background.alpha);
    cairo_fill (cr);

    /* Recompute pixmap */
    drawing_cursor = graph->ring_cursor;
    for (i = 0; i < graph->pixmap_width / scale; i++)
    {
        /* Draw one bar of the graph. */
        if (graph->samples[drawing_cursor] != 0.0)
        {
            cairo_set_source_rgba (cr, graph->colours[graph->samp_states[drawing_cursor]].blue, graph->colours[graph->samp_states[drawing_cursor]].green,
                graph->colours[graph->samp_states[drawing_cursor]].red, graph->colours[graph->samp_states[drawing_cursor]].alpha);

            cairo_move_to (cr, i * scale + 0.5, graph->pixmap_height - scale);
            cairo_line_to (cr, i * scale + 0.5, graph->pixmap_height - scale - graph->samples[drawing_cursor] * (graph->pixmap_height - 2 * scale));
            cairo_stroke (cr);
        }

        /* Increment and wrap drawing cursor */
        drawing_cursor += 1;
        if (drawing_cursor >= graph->pixmap_width / scale) drawing_cursor = 0;
    }

    /* Draw border in black */
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_set_line_width (cr, scale);
    cairo_move_to (cr, scale - 1, scale - 1);
    cairo_line_to (cr, scale - 1, graph->pixmap_height - scale + 1);
    cairo_line_to (cr, graph->pixmap_width - scale + 1, graph->pixmap_height - scale + 1);
    cairo_line_to (cr, graph->pixmap_width - scale + 1, scale - 1);
    cairo_line_to (cr, scale - 1, scale - 1);
    cairo_stroke (cr);

    /* Apply label */
    fontsize = 12 * scale;
    if (graph->pixmap_width > 50 * scale) fontsize = graph->pixmap_height / 3;
    cairo_select_font_face (cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, fontsize);
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_text_extents_t extents;
    cairo_text_extents (cr, label, &extents);
    cairo_move_to (cr, (graph->pixmap_width - extents.width) / 2, (graph->pixmap_height + extents.height) / 2);
    cairo_show_text (cr, label);

    cairo_destroy (cr);

    /* Update image */
    pixbuf = gdk_pixbuf_new_from_data (cairo_image_surface_get_data (graph->pixmap), GDK_COLORSPACE_RGB, TRUE, 8, 
        graph->pixmap_width, graph->pixmap_height, graph->pixmap_width * 4, NULL, NULL);

    if (scale == 1) gtk_image_set_from_pixbuf (GTK_IMAGE (graph->da), pixbuf);
    else
    {
        cairo_surface_t *cr = gdk_cairo_surface_create_from_pixbuf (pixbuf, scale, NULL);
        gtk_image_set_from_surface (GTK_IMAGE (graph->da), cr);
        cairo_surface_destroy (cr);
    }
    g_object_unref (pixbuf);
}

/* Initialise graph for a particular size */

void graph_reload (PluginGraph *graph, int icon_size, GdkRGBA background, GdkRGBA foreground, GdkRGBA throttle1, GdkRGBA throttle2)
{
    int scale = gtk_widget_get_scale_factor (graph->da);

    /* Load colours */
    graph->background = background;
    graph->colours[0] = foreground;
    graph->colours[1] = throttle1;
    graph->colours[2] = throttle2;

    /* Allocate pixmap and statistics buffer without border pixels. */
    guint new_pixmap_height = icon_size - (BORDER_SIZE << 1);
    guint new_pixmap_width = (new_pixmap_height * 3) >> 1;
    if (new_pixmap_width < 50) new_pixmap_width = 50;

    new_pixmap_width *= scale;
    new_pixmap_height *= scale;

    if ((new_pixmap_width > 0) && (new_pixmap_height > 0))
    {
        /* If statistics buffer does not exist or it changed size, reallocate and preserve existing data. */
        if ((graph->samples == NULL) || (new_pixmap_width != graph->pixmap_width))
        {
            float *new_samples = g_new0 (float, new_pixmap_width);
            int *new_samp_states = g_new0 (int, new_pixmap_width);
            if (graph->samples != NULL)
            {
                if (new_pixmap_width > graph->pixmap_width)
                {
                    /* New allocation is larger. Introduce new "oldest" samples of zero following the cursor. */
                    memcpy (&new_samples[0], &graph->samples[0], graph->ring_cursor * sizeof (float));
                    memcpy (&new_samples[new_pixmap_width - graph->pixmap_width + graph->ring_cursor], &graph->samples[graph->ring_cursor], (graph->pixmap_width - graph->ring_cursor) * sizeof (float));
                    memcpy (&new_samp_states[0], &graph->samp_states[0], graph->ring_cursor * sizeof (int));
                    memcpy (&new_samp_states[new_pixmap_width - graph->pixmap_width + graph->ring_cursor], &graph->samp_states[graph->ring_cursor], (graph->pixmap_width - graph->ring_cursor) * sizeof (int));
                }
                else if (graph->ring_cursor <= new_pixmap_width)
                {
                    /* New allocation is smaller, but still larger than the ring buffer cursor. Discard the oldest samples following the cursor. */
                    memcpy (&new_samples[0], &graph->samples[0], graph->ring_cursor * sizeof (float));
                    memcpy (&new_samples[graph->ring_cursor], &graph->samples[graph->pixmap_width - new_pixmap_width + graph->ring_cursor], (new_pixmap_width - graph->ring_cursor) * sizeof (float));
                    memcpy (&new_samp_states[0], &graph->samp_states[0], graph->ring_cursor * sizeof (int));
                    memcpy (&new_samp_states[graph->ring_cursor], &graph->samp_states[graph->pixmap_width - new_pixmap_width + graph->ring_cursor], (new_pixmap_width - graph->ring_cursor) * sizeof (int));
                }
                else
                {
                    /* New allocation is smaller, and also smaller than the ring buffer cursor. Discard all oldest samples following the ring buffer cursor and additional samples at the beginning of the buffer. */
                    memcpy (&new_samples[0], &graph->samples[graph->ring_cursor - new_pixmap_width], new_pixmap_width * sizeof (float));
                    memcpy (&new_samp_states[0], &graph->samp_states[graph->ring_cursor - new_pixmap_width], new_pixmap_width * sizeof (int));
                    graph->ring_cursor = 0;
                }
                g_free (graph->samples);
                g_free (graph->samp_states);
            }
            graph->samples = new_samples;
            graph->samp_states = new_samp_states;
        }

        /* Allocate or reallocate pixmap. */
        graph->pixmap_width = new_pixmap_width;
        graph->pixmap_height = new_pixmap_height;
        if (graph->pixmap) cairo_surface_destroy (graph->pixmap);
        graph->pixmap = cairo_image_surface_create (CAIRO_FORMAT_RGB24, graph->pixmap_width, graph->pixmap_height);

        /* Redraw pixmap at the new size. */
        graph_redraw (graph, "");
    }
}

/* Add new data point to the graph */

void graph_new_point (PluginGraph *graph, float value, int state, char *label)
{
    if (value < 0.0) value = 0.0;
    else if (value > 1.0) value = 1.0;
    graph->samples[graph->ring_cursor] = value;
    graph->samp_states[graph->ring_cursor] = state;

    graph->ring_cursor += 1;
    if (graph->ring_cursor >= graph->pixmap_width) graph->ring_cursor = 0;

    graph_redraw (graph, label);
}

void graph_init (PluginGraph *graph)
{
    graph->da = gtk_image_new ();
    graph->samples = NULL;
    graph->samp_states = NULL;
    graph->ring_cursor = 0;
    graph->pixmap = NULL;
}

void graph_free (PluginGraph *graph)
{
    if (graph->pixmap) cairo_surface_destroy (graph->pixmap);
    if (graph->samples) g_free (graph->samples);
    if (graph->samp_states) g_free (graph->samp_states);
}

/*----------------------------------------------------------------------------*/
/* Menu popup */
/*----------------------------------------------------------------------------*/

static gboolean hide_prelight (GtkWidget *btn)
{
    if (btn && GTK_IS_WIDGET (btn)) gtk_widget_unset_state_flags (btn, GTK_STATE_FLAG_PRELIGHT);
    return FALSE;
}

static int get_menu_padding (void)
{
    GtkWidget *men = gtk_menu_new ();
    GtkStyleContext *sc = gtk_widget_get_style_context (men);
    GtkBorder pad;
    gtk_style_context_get_padding (sc, gtk_style_context_get_state (sc), &pad);
    gtk_widget_destroy (men);
    return pad.left;
}

static void count_item (GtkWidget *, gpointer data)
{
    (* (int *) data)++;
}

gboolean check_menu (GtkWidget *menu)
{
    if (!GTK_IS_MENU (menu)) return FALSE;
    int count = 0;
    gtk_container_foreach (GTK_CONTAINER (menu), count_item, &count);
    if (count == 0) return FALSE;
    return TRUE;
}

static void generate_leave_event (GtkWidget *wid)
{
    GtkWindow *panel = find_panel (wid);
    if (!panel) return;
    GdkEventCrossing *ev = (GdkEventCrossing *) gdk_event_new (GDK_LEAVE_NOTIFY);
    ev->send_event = TRUE;
    ev->window = gtk_widget_get_window (GTK_WIDGET (panel));
    ev->subwindow = gtk_widget_get_window (GTK_WIDGET (panel));
    ev->mode = GDK_CROSSING_NORMAL;
    ev->detail = GDK_NOTIFY_NONLINEAR_VIRTUAL;
    gdk_event_set_device ((GdkEvent *) ev, gdk_seat_get_pointer (gdk_display_get_default_seat (gdk_display_get_default ())));
    gdk_event_put ((GdkEvent *) ev);
    gtk_window_set_focus (panel, NULL);     // otherwise the widget retains focus and hitting enter reopens the menu
}

static void menu_closed (GtkWidget *men, GtkWidget *wid)
{
    g_signal_handler_disconnect (men, mch);
    generate_leave_event (wid);
}

void show_menu_with_kbd (GtkWidget *widget, GtkWidget *menu, GdkEventButton *event)
{
    close_popup ();

    int pad = get_menu_padding ();
    GValue val = G_VALUE_INIT;
    g_value_init (&val, G_TYPE_INT);
    g_value_set_int (&val, panel_at_bottom (widget) ? -pad : pad);
    g_object_set_property ((GObject *) menu, "rect-anchor-dy", &val);

    gtk_menu_popup_at_widget (GTK_MENU (menu), widget, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, (GdkEvent *) event);
    mch = g_signal_connect (menu, "deactivate", G_CALLBACK (menu_closed), widget);
    g_idle_add ((GSourceFunc) hide_prelight, widget);
}

void show_menu_with_kbd_at_xy (GtkWidget *widget, GtkWidget *menu, GdkEventButton *event)
{
    close_popup ();

    int pad = get_menu_padding ();
    GValue val = G_VALUE_INIT;
    g_value_init (&val, G_TYPE_INT);
    g_value_set_int (&val, panel_at_bottom (widget) ? -pad : pad);
    g_object_set_property ((GObject *) menu, "rect-anchor-dy", &val);

    GdkRectangle rect;
    GtkWindow *panel = find_panel (widget);
    if (!panel) return;
    gtk_widget_get_allocation (GTK_WIDGET (panel), &rect);
    rect.x = event->x_root;
    rect.y = 0;
    gtk_menu_popup_at_rect (GTK_MENU (menu), gtk_widget_get_window (GTK_WIDGET (panel)), &rect, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, (GdkEvent *) event);
    mch = g_signal_connect (menu, "deactivate", G_CALLBACK (menu_closed), widget);
}

/*----------------------------------------------------------------------------*/
/* Window popup with close on click-away */
/*----------------------------------------------------------------------------*/

static gboolean handle_clickaway (GtkWidget *, GdkEventButton *, GtkWidget *button)
{
    close_popup ();
    generate_leave_event (button);
    return FALSE;
}

void popup_window_at_button (GtkWidget *window, GtkWidget *button)
{
    GdkDisplay *disp;
    GdkMonitor *mon;
    GdkRectangle rect;
    GtkCssProvider *prov;
    int i, pw, panw;
    gboolean bottom;
    FILE *fp;
    char *cmd, *mname;

    GtkWindow *panel = find_panel (button);
    if (!panel) return;
    mon = gtk_layer_get_monitor (panel);

    close_popup ();

    clicksink = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
    gtk_layer_init_for_window (clicksink);
    gtk_layer_set_anchor (clicksink, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor (clicksink, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_anchor (clicksink, GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor (clicksink, GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
    gtk_layer_set_exclusive_zone (clicksink, -1);
    gtk_layer_set_monitor (clicksink, mon);
    gtk_widget_set_name (GTK_WIDGET (clicksink), "clicksink");

    prov = gtk_css_provider_new ();
    gtk_css_provider_load_from_data (prov, "#clicksink { background-color: transparent; }", -1, NULL);
    gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
        GTK_STYLE_PROVIDER (prov), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref (prov);

    gtk_widget_show (GTK_WIDGET (clicksink));
    gtk_window_present (clicksink);
    g_signal_connect (clicksink, "button-release-event", G_CALLBACK (handle_clickaway), button);

    popwindow = GTK_WINDOW (window);

    disp = gdk_display_get_default ();
    gtk_layer_init_for_window (popwindow);
    gtk_widget_show_all (window);

    // get the dimensions of the panel
    bottom = gtk_layer_get_anchor (panel, GTK_LAYER_SHELL_EDGE_BOTTOM);
    gtk_widget_get_allocation (GTK_WIDGET (panel), &rect);
    px = rect.width;
    py = gtk_layer_get_margin (panel, bottom ? GTK_LAYER_SHELL_EDGE_BOTTOM : GTK_LAYER_SHELL_EDGE_TOP);
    if (gtk_layer_get_exclusive_zone (panel) <= 0) py += rect.height;
    panw = px;

    // get the dimensions of the popup itself and ensure the popup fits on the screen
    gtk_widget_get_allocation (window, &rect);
    pw = rect.width;
    px -= pw;

    // get the dimensions of the button - align left edge of popup with left edge of button
    gtk_widget_get_allocation (button, &rect);
    if (rect.x <= px) px = rect.x;

    // get the dimensions of the monitor - correct the y-coord of the plugin if at bottom
    gdk_monitor_get_geometry (mon, &rect);
    mh = rect.height;
    mw = rect.width;

    orient = 0;
    for (i = 0; i < gdk_display_get_n_monitors (disp); i++)
    {
        if (mon == gdk_display_get_monitor (disp, i))
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            // yes, I know get_monitor_plug_name is deprecated, but the recommended replacement doesn't actually do the same thing...
            mname = gdk_screen_get_monitor_plug_name (gdk_display_get_default_screen (disp), i);
#pragma GCC diagnostic pop
            cmd = g_strdup_printf ("wlr-randr | sed -nr '/%s/,/^~ /{s/Transform:\\s*(.*)/\\1/p}' | tr -d ' '", mname);
            if ((fp = popen (cmd, "r")) != NULL)
            {
                if (fscanf (fp, "%d", &orient) != 1) orient = 0;
                pclose (fp);
            }
            g_free (cmd);
            g_free (mname);
        }
    }

    gtk_layer_set_layer (popwindow, GTK_LAYER_SHELL_LAYER_TOP);

    gtk_layer_set_anchor (popwindow, bottom ? GTK_LAYER_SHELL_EDGE_BOTTOM : GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_margin (popwindow, bottom ? GTK_LAYER_SHELL_EDGE_BOTTOM : GTK_LAYER_SHELL_EDGE_TOP, get_menu_padding () + py);

    if (gtk_layer_get_anchor (panel, GTK_LAYER_SHELL_EDGE_LEFT))
    {
        gtk_layer_set_anchor (popwindow, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_margin (popwindow, GTK_LAYER_SHELL_EDGE_LEFT, px);
    }
    else if (gtk_layer_get_anchor (panel, GTK_LAYER_SHELL_EDGE_RIGHT))
    {
        gtk_layer_set_anchor (popwindow, GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_margin (popwindow, GTK_LAYER_SHELL_EDGE_RIGHT, panw - pw - px);
    }
    else
    {
        // no anchor - panel in centre of screen...
        gtk_layer_set_anchor (popwindow, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_margin (popwindow, GTK_LAYER_SHELL_EDGE_LEFT, (mw / 2)  - (panw / 2) + px);
    }

    gtk_layer_set_monitor (popwindow, mon);
    gtk_layer_set_keyboard_mode (popwindow, GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);

    gtk_window_present (popwindow);
    g_idle_add ((GSourceFunc) hide_prelight, button);
}

void close_popup (void)
{
    if (popwindow) gtk_widget_destroy (GTK_WIDGET (popwindow));
    if (clicksink) gtk_widget_destroy (GTK_WIDGET (clicksink));
    popwindow = NULL;
    clicksink = NULL;
}

/*----------------------------------------------------------------------------*/
/* Long press gestures */
/*----------------------------------------------------------------------------*/

void pass_right_click (GtkWidget *wid, double x, double y)
{
    GtkAllocation alloc;
    GdkEventButton *ev;
    GtkWidget *w;
    gboolean ret;

    gtk_widget_get_allocation (wid, &alloc);
    ev = (GdkEventButton *) gdk_event_new (GDK_BUTTON_PRESS);
    ev->send_event = TRUE;
    ev->button = 3;
    ev->window = gtk_widget_get_window (wid);
    ev->x_root = x + alloc.x;
    ev->y_root = y + alloc.y;
    gdk_event_set_device ((GdkEvent *) ev, gdk_seat_get_pointer (gdk_display_get_default_seat (gdk_display_get_default ())));
    w = wid;
    while (!GTK_IS_WINDOW (w)) w = gtk_widget_get_parent (w);
    g_signal_emit_by_name (w, "button-press-event", ev, &ret);
    ev->type = GDK_BUTTON_RELEASE;
    g_signal_emit_by_name (w, "button-release-event", ev, &ret);
    pressed = PRESS_LONG;
}

static void gesture_pressed (GtkGestureLongPress *, gdouble x, gdouble y, gpointer)
{
    pressed = PRESS_LONG;
    press_x = x;
    press_y = y;
}

static void gesture_end_default (GtkGestureLongPress *, GdkEventSequence *, GtkWidget *target)
{
    if (pressed == PRESS_LONG) pass_right_click (target, press_x, press_y);
}

GtkGesture *add_long_press (GtkWidget *target, GCallback callback, gpointer data)
{
    GtkGesture *gesture = gtk_gesture_long_press_new (target);
    gtk_gesture_single_set_touch_only (GTK_GESTURE_SINGLE (gesture), gestures_touch_only);
    g_signal_connect (gesture, "pressed", G_CALLBACK (gesture_pressed), NULL);
    if (callback) g_signal_connect (gesture, "end", G_CALLBACK (callback), data);
    else g_signal_connect (gesture, "end", G_CALLBACK (gesture_end_default), target);
    gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (gesture), GTK_PHASE_BUBBLE);
    return gesture;
}

gboolean is_pi (void)
{
    return is_pi_var;
}

/*----------------------------------------------------------------------------*/
/* Reading config from key files */
/*----------------------------------------------------------------------------*/

gboolean load_configuration_data (const char *type, conf_table_t *conf_table)
{
    conf_table_t *cptr = &conf_table[0];
    char *str, *ostr;
    GdkRGBA *ocol;
    int orig;
    gboolean changed = FALSE;

    while (cptr->type != CONF_TYPE_NONE)
    {
        switch (cptr->type)
        {
            case CONF_TYPE_BOOL :
                orig = *((gboolean *) cptr->value);
                *((gboolean *) cptr->value) = get_config_bool (type, cptr->name, cptr->def_val);
                if (*((gboolean *) cptr->value) != orig) changed = TRUE;
                break;

            case CONF_TYPE_INT :
                orig = *((int *) cptr->value);
                *((int *) cptr->value) = get_config_int (type, cptr->name, cptr->def_val);
                if (*((int *) cptr->value) != orig) changed = TRUE;
                break;

            case CONF_TYPE_STRING :
            case CONF_TYPE_FONT :
                ostr = g_strdup (((char *) *cptr->value));
                get_config_string (type, cptr->name, (char **) cptr->value, cptr->def_val);
                if (g_strcmp0 ((char *) *cptr->value, ostr)) changed = TRUE;
                g_free (ostr);
                break;

            case CONF_TYPE_COLOUR :
                ocol = gdk_rgba_copy ((GdkRGBA *) cptr->value);
                get_config_string (type, cptr->name, &str, cptr->def_val);
                gdk_rgba_parse ((GdkRGBA *) cptr->value, str);
                if (!gdk_rgba_equal ((GdkRGBA *) cptr->value, ocol)) changed = TRUE;
                g_free (str);
                gdk_rgba_free (ocol);
                break;

            default: break;
        }
        cptr++;
    }
    return changed;
}

void save_configuration_data (const char *type, conf_table_t *conf_table)
{
    conf_table_t *cptr = &conf_table[0];
    char *strval, *user_file;
    GKeyFile *kf;
    gsize len;

    user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi", "wf-panel-pi.ini", NULL);
    kf = g_key_file_new ();
    g_key_file_load_from_file (kf, user_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

    while (cptr->type != CONF_TYPE_NONE)
    {
        switch (cptr->type)
        {
            case CONF_TYPE_BOOL :
                g_key_file_set_boolean (kf, type, cptr->name, *((gboolean *) cptr->value));
                break;

            case CONF_TYPE_INT :
                g_key_file_set_integer (kf, type, cptr->name, *((int *) cptr->value));
                break;

            case CONF_TYPE_STRING :
            case CONF_TYPE_FONT :
                g_key_file_set_string (kf, type, cptr->name, (char *) cptr->value);
                break;

            case CONF_TYPE_COLOUR :
                strval = gdk_rgba_to_string ((GdkRGBA *) cptr->value);
                g_key_file_set_string (kf, type, cptr->name, strval);
                g_free (strval);
                break;

            default: break;
        }
        cptr++;
    }

    strval = g_key_file_to_data (kf, &len, NULL);
    g_file_set_contents (user_file, strval, len, NULL);

    g_free (strval);
    g_key_file_free (kf);
    g_free (user_file);
}

/*----------------------------------------------------------------------------*/
/* Menu cache search                                                          */
/*----------------------------------------------------------------------------*/

#ifdef USES_MENUCACHE

/* This is an attempt to score how similar two strings are by comparing how many letters
 * at the start of each are identical, and how many letters at the end are identical.
 * It's not perfect... */

static float score_match (const char *str1, const char *str2)
{
    int score, pos1, pos2;
    char *str1l, *str2l;
    float result;

    if (!str1 || !str2) return 0.0;

    str1l = g_ascii_strdown (str1, -1);
    str2l = g_ascii_strdown (str2, -1);
    score = 0;

    // count matching characters from start
    pos1 = 0;
    while (str1l[pos1] && str2l[pos1] && str1l[pos1] == str2l[pos1])
    {
        score++;
        pos1++;
    }

    // count matching characters from end
    pos1 = strlen (str1l) - 1;
    pos2 = strlen (str2l) - 1;
    while (pos1 && pos2 && str1l[pos1] == str2l[pos2])
    {
        score++;
        pos1--;
        pos2--;
    }

    result = score;
    if (strlen (str1l) > strlen (str2l)) result /= strlen (str2l);
    else result /= strlen (str1l);

    g_free (str1l);
    g_free (str2l);

    return result;
}

static char *get_exe (const char *cmdline)
{
    // g_path_get_basename fails with quoted paths, so...
    char *buf, *start, *end, *ret;
    char del;

    buf = g_strdup (cmdline);

    start = buf;
    if (strchr ("'\"", *start))
    {
        del = *start;
        start++;
    }
    else del = ' ';

    end = start;
    while (*end)
    {
        if (*end == del) break;
        end++;
    }
    *end = 0;

    if (strrchr (start, '/')) start = strrchr (start, '/') + 1;
    ret = g_strdup (start);
    g_free (buf);

    return ret;
}

char *menu_cache_id (const char *app_id)
{
    MenuCacheItem *item;
    GSList *list, *iter;
    GAppInfo *info;
    char *id, *exec, *s2, *best = NULL;
    float res, score;
    const char *ex, *s1;

    // loop through the cache to find the best match
    score = 0.0;
    list = menu_cache_list_all_apps (mcache_h);
    iter = list;
    while (iter)
    {
        item = (MenuCacheItem *) iter->data;

        // first check that the cache item is a valid desktop info, i.e. has an associated exec
        id = g_strdup (menu_cache_item_get_id (item));
        info = (GAppInfo *) g_desktop_app_info_new (id);
        if (!info)
        {
            g_free (id);
            iter = iter->next;
            continue;
        }
        else g_object_unref (info);

        // strip the .desktop from the end for matching purposes
        *strrchr (id, '.') = 0;

        // if there is a caseless match with the app-id, this is correct - return it
        if (!g_ascii_strncasecmp (app_id, id, 1000))
        {
            g_slist_free_full (list, (GDestroyNotify) ((void *) menu_cache_item_unref));
            return id;
        }

        // try matching the part of the id after a final .
        s1 = strrchr (app_id, '.') ? strrchr (app_id, '.') + 1 : app_id;
        s2 = strrchr (id, '.') ? strrchr (id, '.') + 1 : id;
        if (!g_ascii_strncasecmp (s1, s2, 1000))
        {
            g_slist_free_full (list, (GDestroyNotify) ((void *) menu_cache_item_unref));
            return id;
        }

        iter = iter->next;
    }

    // no joy - try matching executable names
    iter = list;
    while (iter)
    {
        item = (MenuCacheItem *) iter->data;

        // first check that the cache item is a valid desktop info, i.e. has an associated exec
        id = g_strdup (menu_cache_item_get_id (item));
        info = (GAppInfo *) g_desktop_app_info_new (id);
        if (!info)
        {
            g_free (id);
            iter = iter->next;
            continue;
        }
        else g_object_unref (info);

        // strip the .desktop from the end for matching purposes
        *strrchr (id, '.') = 0;

        // get the executable name
        ex = menu_cache_app_get_exec ((MenuCacheApp *) item);
        if (ex) exec = get_exe (ex);
        else exec = NULL;

        // if there is a caseless match with the executable, this is correct - return it
        if (exec && !g_ascii_strncasecmp (app_id, exec, 1000))
        {
            g_free (exec);
            if (best) g_free (best);
            g_slist_free_full (list, (GDestroyNotify) ((void *) menu_cache_item_unref));
            return id;
        }

        // look for matching characters at start and end
        res = score_match (app_id, id);
        if (res > score)
        {
            score = res;
            if (best) g_free (best);
            best = g_strdup (id);
        }

        if (exec)
        {
            res = score_match (app_id, exec);
            if (res > score)
            {
                score = res;
                if (best) g_free (best);
                best = g_strdup (id);
            }
            g_free (exec);
        }

        g_free (id);
        iter = iter->next;
    }
    g_slist_free_full (list, (GDestroyNotify) ((void *) menu_cache_item_unref));
    return best;
}

MenuCacheItem *get_cache_item (const char *app_id)
{
    MenuCacheItem *item;
    char *id, *str;

    id = menu_cache_id (app_id);
    str = g_strdup_printf ("%s.desktop", id);
    item = menu_cache_find_item_by_id (mcache_h, str);

    g_free (str);
    g_free (id);

    return item;
}

#endif

/*----------------------------------------------------------------------------*/
/* Launcher management                                                        */
/*----------------------------------------------------------------------------*/

static void edit_launchers (const char *name, gboolean add)
{
    GKeyFile *kf, *kfs;
    char *str, *list, *new_list, *tok, *tmp;
    gsize len;
    GError *err = NULL;

    // construct the file path
    char *user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi", "wf-panel-pi.ini", NULL);

    // read in data from file to a key file
    kf = g_key_file_new ();
    list = NULL;
    if (g_key_file_load_from_file (kf, user_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
    {
        list = g_key_file_get_string (kf, "panel", "launchers", &err);
    }

    // no launchers entry in user file - try loading from system file
    if (!list || (err && err->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND))
    {
        kfs = g_key_file_new ();
        g_key_file_load_from_file (kfs, "/etc/xdg/wf-panel-pi/wf-panel-pi.ini", G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
        list = g_key_file_get_string (kfs, "panel", "launchers", NULL);
        g_key_file_free (kfs);
    }

    // strip .desktop suffix
    str = g_strdup (name);
    if (strstr (str, ".desktop")) *strrchr (str, '.') = 0;

    new_list = NULL;

    // remove item from elsewhere in list
    tok = strtok (list, " ");
    while (tok)
    {
        if (strcmp (str, tok))
        {
            if (new_list)
            {
                tmp = g_strdup_printf ("%s %s", new_list, tok);
                g_free (new_list);
                new_list = tmp;
            }
            else new_list = g_strdup_printf ("%s", tok);
        }
        tok = strtok (NULL, " ");
    }

    // append to list if adding
    if (add)
    {
        if (new_list)
        {
                tmp = g_strdup_printf ("%s %s", new_list, str);
                g_free (new_list);
                new_list = tmp;
        }
        else new_list = g_strdup_printf ("%s", str);
    }

    g_key_file_set_string (kf, "panel", "launchers", new_list ? new_list : "");

    g_free (new_list);
    g_free (list);
    g_free (str);

    // write the modified key file out
    str = g_key_file_to_data (kf, &len, NULL);
    g_file_set_contents (user_file, str, len, NULL);

    g_free (str);
    g_key_file_free (kf);
    g_free (user_file);
}

void add_to_launcher (const char *name)
{
    edit_launchers (name, TRUE);
}

void remove_from_launcher (const char *name)
{
    edit_launchers (name, FALSE);
}

void replace_launchers (const char *launchers)
{
    GKeyFile *kf;
    char *str;
    gsize len;

    // construct the file path
    char *user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi", "wf-panel-pi.ini", NULL);

    // read in data from file to a key file
    kf = g_key_file_new ();
    g_key_file_load_from_file (kf, user_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

    g_key_file_set_string (kf, "panel", "launchers", launchers);

    // write the modified key file out
    str = g_key_file_to_data (kf, &len, NULL);
    g_file_set_contents (user_file, str, len, NULL);

    g_free (str);
    g_key_file_free (kf);
    g_free (user_file);
}

/* End of file */
/*----------------------------------------------------------------------------*/
