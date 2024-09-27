#include "lxutils.h"

/*----------------------------------------------------------------------------*/
/* Plug-in global data                                                        */
/*----------------------------------------------------------------------------*/

typedef struct {

    GtkWidget *plugin;              /* Back pointer to the widget */
    GtkWidget *window;              /* Popup window */
    gboolean bottom;
    gboolean pressed;
} ClockPlugin;

extern void clock_init (ClockPlugin *cl);
extern void clock_update_display (ClockPlugin *cl);
extern void clock_destructor (gpointer user_data);
