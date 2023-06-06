#ifndef GRAPH_H
#define GRAPH_H

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

extern void graph_init (PluginGraph *graph);
extern void graph_reload (PluginGraph *graph, int icon_size, GdkRGBA background, GdkRGBA foreground, GdkRGBA throttle1, GdkRGBA throttle2);
extern void graph_new_point (PluginGraph *graph, float value, int state, char *label);

#endif
