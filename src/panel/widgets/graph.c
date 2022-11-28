#include "lxutils.h"
#include "graph.h"

#define BORDER_SIZE 1

/* Redraw entire graph */

static void graph_redraw (PluginGraph *graph, char *label)
{
    unsigned int fontsize, drawing_cursor, i;
    GdkRGBA background;
    GdkRGBA colours[3];
    GdkPixbuf *pixbuf;

    gdk_rgba_parse (&background, "light gray");
    gdk_rgba_parse (&colours[0], "dark gray");
    gdk_rgba_parse (&colours[1], "orange");
    gdk_rgba_parse (&colours[2], "red");

    cairo_t *cr = cairo_create (graph->pixmap);
    cairo_set_line_width (cr, 1.0);
    
    /* Erase pixmap */
    cairo_rectangle (cr, 0, 0, graph->pixmap_width, graph->pixmap_height);
    cairo_set_source_rgba (cr, background.blue, background.green, background.red, background.alpha);
    cairo_fill (cr);

    /* Recompute pixmap */
    drawing_cursor = graph->ring_cursor;
    for (i = 0; i < graph->pixmap_width; i++)
    {
        /* Draw one bar of the graph. */
        if (graph->samples[drawing_cursor] != 0.0)
        {
            cairo_set_source_rgba (cr, colours[graph->samp_states[drawing_cursor]].blue, colours[graph->samp_states[drawing_cursor]].green,
                colours[graph->samp_states[drawing_cursor]].red, colours[graph->samp_states[drawing_cursor]].alpha);        

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

void graph_init (PluginGraph *graph, int icon_size)
{
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


/* Add  new data point to the graph */

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
