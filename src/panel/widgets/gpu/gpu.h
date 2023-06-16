#include "../lxutils.h"
#include "../graph.h"

/*----------------------------------------------------------------------------*/
/* Plug-in global data                                                        */
/*----------------------------------------------------------------------------*/

/* Private context for plugin */

typedef struct
{
    GtkWidget *plugin;                      /* Back pointer to the widget */
    int icon_size;                          /* Variables used under wf-panel */
    gboolean bottom;
    PluginGraph graph;
    GdkRGBA foreground_colour;              /* Foreground colour for drawing area */
    GdkRGBA background_colour;              /* Background colour for drawing area */
    gboolean show_percentage;               /* Display usage as a percentage */
    guint timer;                            /* Timer for periodic update */
    unsigned long last_val[5];
    unsigned long last_timestamp;
} GPUPlugin;

extern void gpu_init (GPUPlugin *g);
extern void gpu_update_display (GPUPlugin *g);
extern void gpu_destructor (gpointer user_data);
