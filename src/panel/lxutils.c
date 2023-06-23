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

#include <gtk/gtk.h>
#include <gtk-layer-shell.h>
#include "lxutils.h"

/*----------------------------------------------------------------------------*/
/* Macros and typedefs */
/*----------------------------------------------------------------------------*/

#define MENU_ICON_SPACE 6
#define BORDER_SIZE 1

/*----------------------------------------------------------------------------*/
/* Global data */
/*----------------------------------------------------------------------------*/

static GtkWidget *m_button, *m_menu;
static gulong m_handle;

/*----------------------------------------------------------------------------*/
/* Private functions */
/*----------------------------------------------------------------------------*/

static void menu_hidden (GtkWidget *, gpointer)
{
    GtkWidget *panel = gtk_widget_get_parent (gtk_widget_get_parent (gtk_widget_get_parent (m_button)));
    gtk_layer_set_keyboard_interactivity (GTK_WINDOW (panel), FALSE);
}

static void committed (GdkWindow *win, gpointer)
{
    g_signal_handler_disconnect (win, m_handle);
    g_signal_connect (m_menu, "hide", G_CALLBACK (menu_hidden), NULL);
    gtk_menu_popup_at_widget (GTK_MENU (m_menu), m_button, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
}

/*----------------------------------------------------------------------------*/
/* Public API */
/*----------------------------------------------------------------------------*/

void show_menu_with_kbd (GtkWidget *button, GtkWidget *menu)
{
    // simulate a leave event on the button to hide the prelight */
    GdkEventCrossing *ev = (GdkEventCrossing *) gdk_event_new (GDK_LEAVE_NOTIFY);
    ev->window = gtk_button_get_event_window (GTK_BUTTON (button));
    ev->time = GDK_CURRENT_TIME;
    ev->mode = GDK_CROSSING_NORMAL;
    ev->send_event = TRUE;
    gtk_main_do_event ((GdkEvent *) ev);

    GtkWidget *panel = gtk_widget_get_parent (gtk_widget_get_parent (gtk_widget_get_parent (button)));
    m_button = button;
    m_menu = menu;
    gtk_layer_set_keyboard_interactivity (GTK_WINDOW (panel), TRUE);
    m_handle = g_signal_connect (gtk_widget_get_window (panel), "committed", G_CALLBACK (committed), NULL);
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

/* End of file */
/*----------------------------------------------------------------------------*/
