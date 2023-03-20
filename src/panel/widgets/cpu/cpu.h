#include "../lxutils.h"
#include "../graph.h"

/*----------------------------------------------------------------------------*/
/* Plug-in global data                                                        */
/*----------------------------------------------------------------------------*/

typedef unsigned long long CPUTick;		/* Value from /proc/stat */

struct cpu_stat
{
    CPUTick u, n, s, i;				/* User, nice, system, idle */
};

/* Private context for plugin */

typedef struct
{
    GtkWidget *plugin;                      /* Back pointer to the widget */
    int icon_size;                          /* Variables used under wf-panel */
    gboolean bottom;
    PluginGraph graph;
    guint timer;				            /* Timer for periodic update */
    struct cpu_stat previous_cpu_stat;		/* Previous value of cpu_stat */
} CPUPlugin;

extern void cpu_init (CPUPlugin *up);
extern void cpu_update_display (CPUPlugin *up);
extern void cpu_destructor (gpointer user_data);
