#include "lxutils.h"

typedef struct {
    GtkWidget *plugin;              /* Back pointer to the widget */
    int icon_size;                  /* Variables used under wf-panel */
    gboolean bottom;
    GtkWidget *tray_icon;           /* Displayed image */
    guint vtimer;
    gboolean ispi;
    int show_icon;
} PowerPlugin;

extern void power_init (PowerPlugin *pt);
extern void power_update_display (PowerPlugin *pt);
extern gboolean power_control_msg (PowerPlugin *ej, const char *cmd);
extern void power_destructor (gpointer user_data);


