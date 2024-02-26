#include "lxutils.h"

/*----------------------------------------------------------------------------*/
/* Plug-in global data                                                        */
/*----------------------------------------------------------------------------*/

typedef struct {

    GtkWidget *plugin;              /* Back pointer to the widget */
    gboolean bottom;
    GtkWidget *window;              /* Popup window */
    GtkWidget *calendar;            /* Popup calendar */
} ClockPlugin;

extern void clock_init (ClockPlugin *cl);
extern void clock_update_display (ClockPlugin *cl);
extern void clock_destructor (gpointer user_data);
