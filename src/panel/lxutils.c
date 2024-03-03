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

#include <fcntl.h>
#include <libinput.h>
#include <libudev.h>
#include <linux/input.h>
#include <gtk/gtk.h>
#include <gtk-layer-shell.h>
#include "lxutils.h"

/*----------------------------------------------------------------------------*/
/* Macros and typedefs */
/*----------------------------------------------------------------------------*/

#define MENU_ICON_SPACE 6
#define BORDER_SIZE 1

typedef struct {
    GtkWidget *button;
    GtkWidget *menu;
    gulong chandle;
    gulong mhandle;
} kb_menu_t;

/*----------------------------------------------------------------------------*/
/* Global data */
/*----------------------------------------------------------------------------*/

static GtkWidget *m_panel;
static GtkLayerShellLayer orig_layer;
static struct libinput *li;
static guint idle_id;
static GtkWidget *popwindow;
static double tx, ty;
static int px, py, pw, ph, mw, mh;

/*----------------------------------------------------------------------------*/
/* General helper functions */
/*----------------------------------------------------------------------------*/

static GtkWidget *find_panel (GtkWidget *btn)
{
    GtkWidget *wid = btn;
    while (!GTK_IS_WINDOW (wid) || !gtk_layer_is_layer_window (GTK_WINDOW (wid)))
        wid = gtk_widget_get_parent (wid);
    return wid;
}

/*----------------------------------------------------------------------------*/
/* General public API - replaces functions from lxpanel */
/*----------------------------------------------------------------------------*/

void store_layer (GtkLayerShellLayer layer)
{
    orig_layer = layer;
}

void set_taskbar_icon (GtkWidget *image, const char *icon, int size)
{
    if (!icon) return;
    GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), icon,
        size, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
    if (pixbuf)
    {
        gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
        g_object_unref (pixbuf);
    }
}

void set_menu_icon (GtkWidget *image, const char *icon, int size)
{
    if (!icon) return;
    GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), icon,
        size > 32 ? 24 : 16, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
    if (pixbuf)
    {
        gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
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

/*----------------------------------------------------------------------------*/
/* Plugin graph */
/*----------------------------------------------------------------------------*/

/* Redraw entire graph */

static void graph_redraw (PluginGraph *graph, char *label)
{
    unsigned int fontsize, drawing_cursor, i;
    GdkPixbuf *pixbuf;

    cairo_t *cr = cairo_create (graph->pixmap);
    cairo_set_line_width (cr, 1.0);
    
    /* Erase pixmap */
    cairo_rectangle (cr, 0, 0, graph->pixmap_width, graph->pixmap_height);
    cairo_set_source_rgba (cr, graph->background.blue, graph->background.green, graph->background.red, graph->background.alpha);
    cairo_fill (cr);

    /* Recompute pixmap */
    drawing_cursor = graph->ring_cursor;
    for (i = 0; i < graph->pixmap_width; i++)
    {
        /* Draw one bar of the graph. */
        if (graph->samples[drawing_cursor] != 0.0)
        {
            cairo_set_source_rgba (cr, graph->colours[graph->samp_states[drawing_cursor]].blue, graph->colours[graph->samp_states[drawing_cursor]].green,
                graph->colours[graph->samp_states[drawing_cursor]].red, graph->colours[graph->samp_states[drawing_cursor]].alpha);

            cairo_move_to (cr, i + 0.5, graph->pixmap_height);
            cairo_line_to (cr, i + 0.5, graph->pixmap_height - graph->samples[drawing_cursor] * graph->pixmap_height);
            cairo_stroke (cr);
        }

        /* Increment and wrap drawing cursor */
        drawing_cursor += 1;
        if (drawing_cursor >= graph->pixmap_width) drawing_cursor = 0;
    }

    /* Draw border in black */
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_set_line_width (cr, 1);
    cairo_move_to (cr, 0, 0);
    cairo_line_to (cr, 0, graph->pixmap_height);
    cairo_line_to (cr, graph->pixmap_width, graph->pixmap_height);
    cairo_line_to (cr, graph->pixmap_width, 0);
    cairo_line_to (cr, 0, 0);
    cairo_stroke (cr);

    /* Apply label */
    fontsize = 12;
    if (graph->pixmap_width > 50) fontsize = graph->pixmap_height / 3;
    cairo_select_font_face (cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cr, fontsize);
    cairo_set_source_rgb (cr, 0, 0, 0);
    cairo_move_to (cr, (graph->pixmap_width >> 1) - ((fontsize * 5) / 4), ((graph->pixmap_height + fontsize) >> 1) - 1);
    cairo_show_text (cr, label);

    cairo_destroy (cr);

    /* Update image */
    pixbuf = gdk_pixbuf_new_from_data (cairo_image_surface_get_data (graph->pixmap), GDK_COLORSPACE_RGB, TRUE, 8, 
        graph->pixmap_width, graph->pixmap_height, graph->pixmap_width * 4, NULL, NULL);
    gtk_image_set_from_pixbuf (GTK_IMAGE (graph->da), pixbuf);
}

/* Initialise graph for a particular size */

void graph_reload (PluginGraph *graph, int icon_size, GdkRGBA background, GdkRGBA foreground, GdkRGBA throttle1, GdkRGBA throttle2)
{
    /* Load colours */
    graph->background = background;
    graph->colours[0] = foreground;
    graph->colours[1] = throttle1;
    graph->colours[2] = throttle2;

    /* Allocate pixmap and statistics buffer without border pixels. */
    guint new_pixmap_height = icon_size - (BORDER_SIZE << 1);
    guint new_pixmap_width = (new_pixmap_height * 3) >> 1;
    if (new_pixmap_width < 50) new_pixmap_width = 50;

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
    graph->ring_cursor = 0;
}

/*----------------------------------------------------------------------------*/
/* Menu popup with keyboard handling */
/*----------------------------------------------------------------------------*/

static gboolean hide_prelight (GtkWidget *btn)
{
    gtk_widget_unset_state_flags (btn, GTK_STATE_FLAG_PRELIGHT);
    return FALSE;
}

static void menu_hidden (GtkWidget *, kb_menu_t *data)
{
    g_signal_handler_disconnect (data->menu, data->mhandle);  // Take this out and watch the crashes....
    gtk_layer_set_layer (GTK_WINDOW (m_panel), orig_layer);
    gtk_layer_set_keyboard_interactivity (GTK_WINDOW (m_panel), FALSE);
    if (data->button) g_idle_add ((GSourceFunc) hide_prelight, data->button);
    g_free (data);
}

static void committed (GdkWindow *win, kb_menu_t *data)
{
    // spoof event just to suppress warnings...
    GdkEventButton *ev = (GdkEventButton *) gdk_event_new (GDK_NOTHING);
    ev->send_event = TRUE;
    gdk_event_set_device ((GdkEvent *) ev, gdk_seat_get_pointer (gdk_display_get_default_seat (gdk_display_get_default ())));

    g_signal_handler_disconnect (win, data->chandle);
    data->mhandle = g_signal_connect (data->menu, "hide", G_CALLBACK (menu_hidden), data);
    if (data->button)
    {
        gtk_menu_popup_at_widget (GTK_MENU (data->menu), data->button, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, (GdkEvent *) ev);
    }
    else
    {
        GtkAllocation alloc;
        int x, y;
        gtk_widget_get_allocation (GTK_WIDGET (m_panel), &alloc);
        gdk_window_get_device_position (gtk_widget_get_window (m_panel), gdk_seat_get_pointer (gdk_display_get_default_seat (gdk_display_get_default ())), &x, &y, NULL);
        GdkRectangle rect = {x, 0, 0, alloc.height};
        gtk_menu_popup_at_rect (GTK_MENU (data->menu), gtk_widget_get_window (m_panel), &rect, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, (GdkEvent *) ev);
    }
}

void show_menu_with_kbd (GtkWidget *widget, GtkWidget *menu)
{
    close_popup ();

    kb_menu_t *data = g_new (kb_menu_t, 1);

    m_panel = find_panel (widget);

    if (GTK_IS_BUTTON (widget)) data->button = widget;
    else data->button = NULL;
    data->menu = menu;

    gtk_layer_set_layer (GTK_WINDOW (m_panel), GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_keyboard_interactivity (GTK_WINDOW (m_panel), TRUE);
    data->chandle = g_signal_connect (gtk_widget_get_window (m_panel), "committed", G_CALLBACK (committed), data);
}

/*----------------------------------------------------------------------------*/
/* Window popup with close on click-away */
/*----------------------------------------------------------------------------*/

static int open_restricted (const char *path, int flags, void *)
{
    int fd = open (path, flags);
    return fd < 0 ? -errno : fd;
}

static void close_restricted (int fd, void *)
{
    close (fd);
}

static const struct libinput_interface interface = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted,
};

static gboolean check_libinput_events (gpointer)
{
    GdkWindow *win, *wwin;
    gboolean match;
    struct libinput_event *ev;

    libinput_dispatch (li);
    if ((ev = libinput_get_event (li)) != 0)
    {
        enum libinput_event_type type = libinput_event_get_type (ev);

        if (type == LIBINPUT_EVENT_POINTER_BUTTON)
        {
            if (libinput_event_pointer_get_button_state (libinput_event_get_pointer_event (ev)) == LIBINPUT_BUTTON_STATE_RELEASED)
            {
                win = gdk_device_get_window_at_position (gdk_seat_get_pointer (
                    gdk_display_get_default_seat (gdk_display_get_default ())), NULL, NULL);
                if (!win) close_popup ();
                else
                {
                    // is the popup a parent of the window under the pointer?
                    match = FALSE;
                    wwin = gtk_widget_get_window (popwindow);
                    while ((win = gdk_window_get_parent (win)) != NULL)
                        if (win == wwin)
                            match = TRUE;
                    if (!match) close_popup ();
                }
            }
            libinput_event_destroy (ev);
        }

        if (type == LIBINPUT_EVENT_KEYBOARD_KEY)
        {
            if (libinput_event_keyboard_get_key (libinput_event_get_keyboard_event (ev)) == KEY_ESC)
                close_popup ();
            libinput_event_destroy (ev);
        }

        if (type == LIBINPUT_EVENT_TOUCH_UP)
        {
            // was the touch inside the co-ords of the popup?
            if (tx < px || tx > px + pw || ty < py || ty > py + ph)
                close_popup ();
            libinput_event_destroy (ev);
        }

        if (type == LIBINPUT_EVENT_TOUCH_DOWN)
        {
            struct libinput_event_touch *tev = libinput_event_get_touch_event (ev);
            tx = libinput_event_touch_get_x_transformed (tev, mw);
            ty = libinput_event_touch_get_y_transformed (tev, mh);
            libinput_event_destroy (ev);
        }
    }
    return TRUE;
}

void popup_window_at_button (GtkWidget *window, GtkWidget *button, gboolean bottom)
{
    GdkRectangle rect;
    GtkAllocation alloc;
    GtkWidget *wid;
    int i, orient;
    FILE *fp;

    close_popup ();

    gtk_layer_init_for_window (GTK_WINDOW (window));
    gtk_widget_show_all (window);

    // get the dimensions of the panel
    wid = find_panel (button);
    gtk_widget_get_allocation (wid, &alloc);
    px = alloc.width;
    py = alloc.height;

    // get the dimensions of the popup itself and ensure the popup fits on the screen
    gtk_widget_get_allocation (window, &alloc);
    pw = alloc.width;
    ph = alloc.height;
    px -= pw;

    // get the dimensions of the button - align left edge of popup with left edge of button
    gtk_widget_get_allocation (button, &alloc);
    if (alloc.x <= px) px = alloc.x;

    // get the dimensions of the monitor - correct the y-coord of the plugin if at bottom
    gdk_monitor_get_geometry (gtk_layer_get_monitor (GTK_WINDOW (wid)), &rect);
    mh = rect.height;
    mw = rect.width;
    if (bottom) py = mh - py - ph;

    orient = 0;
    GdkDisplay *disp = gdk_display_get_default();
    GdkMonitor *mon = gtk_layer_get_monitor (GTK_WINDOW (wid));
    for (i = 0; i < gdk_display_get_n_monitors (disp); i++)
    {
        // yes, I know get_monitor_plug_name is deprecated, but the recommended replacement doesn't actually do the same thing...
        if (mon == gdk_display_get_monitor (disp, i))
        {
            char *cmd = g_strdup_printf ("wlr-randr | sed -nr '/%s/,/^~ /{s/Transform:\\s*(.*)/\\1/p}' | tr -d ' '",
                gdk_screen_get_monitor_plug_name (gdk_display_get_default_screen (disp), i));
            if ((fp = popen (cmd, "r")) != NULL)
            {
                fscanf (fp, "%d", &orient);
                pclose (fp);
            }
            g_free (cmd);
        }
    }

    gtk_layer_set_layer (GTK_WINDOW (window), GTK_LAYER_SHELL_LAYER_TOP);
    gtk_layer_set_anchor (GTK_WINDOW (window), bottom ? GTK_LAYER_SHELL_EDGE_BOTTOM : GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor (GTK_WINDOW (window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_margin (GTK_WINDOW (window), GTK_LAYER_SHELL_EDGE_LEFT, px);
    gtk_layer_set_monitor (GTK_WINDOW (window), gtk_layer_get_monitor (GTK_WINDOW (wid)));
    gtk_layer_set_keyboard_interactivity (GTK_WINDOW (window), TRUE);

    gtk_window_present (GTK_WINDOW (window));

    // remap touch for rotated displays
    switch (orient)
    {
        case 90 :   i = px;
                    px = py;
                    py = mw - i - pw;
                    break;

        case 180:   px = mw - px - pw;
                    py = mh - py - ph;
                    break;

        case 270:   i = py;
                    py = px;
                    px = mh - i - ph;
                    break;
    }
    if (orient == 90 || orient == 270)
    {
        i = pw;
        pw = ph;
        ph = i;

        i = mw;
        mw = mh;
        mh = i;
    }

    popwindow = window;

    li = libinput_udev_create_context (&interface, NULL, udev_new ());
    libinput_udev_assign_seat (li, "seat0");
    libinput_dispatch (li);
    idle_id = g_idle_add ((GSourceFunc) check_libinput_events, NULL);
}

void close_popup (void)
{
    if (popwindow) gtk_widget_destroy (popwindow);
    popwindow = NULL;
    if (idle_id) g_source_remove (idle_id);
    idle_id = 0;
}

/* End of file */
/*----------------------------------------------------------------------------*/
