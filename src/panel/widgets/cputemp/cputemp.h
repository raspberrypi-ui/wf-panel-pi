#include "../lxutils.h"
#include "../graph.h"

/*----------------------------------------------------------------------------*/
/* Plug-in global data                                                        */
/*----------------------------------------------------------------------------*/

#define MAX_NUM_SENSORS 10

typedef gint (*GetTempFunc) (char const *);

/* Private context for plugin */

typedef struct
{
    GtkWidget *plugin;                      /* Back pointer to the widget */
    int icon_size;                          /* Variables used under wf-panel */
    gboolean bottom;
    PluginGraph graph;
    guint timer;				            /* Timer for periodic update */
    int numsensors;
    char *sensor_array[MAX_NUM_SENSORS];
    GetTempFunc get_temperature[MAX_NUM_SENSORS];
    gint temperature[MAX_NUM_SENSORS];
    gboolean ispi;
} CPUTempPlugin;

extern void cputemp_init (CPUTempPlugin *up);
extern void cputemp_update_display (CPUTempPlugin *up);
extern void cputemp_destructor (gpointer user_data);
