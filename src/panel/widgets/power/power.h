#include "lxutils.h"

typedef struct {
    GtkWidget *plugin;              /* Back pointer to the widget */
    int icon_size;                  /* Variables used under wf-panel */
    gboolean bottom;
    GtkWidget *tray_icon;           /* Displayed image */
    guint vtimer;
    gboolean ispi;
} PowerPlugin;

extern void power_init (PowerPlugin *pt);
extern void power_update_display (PowerPlugin *pt);
extern void power_destructor (gpointer user_data);


