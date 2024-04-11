#include "lxutils.h"

/* Plug-in global data */

typedef struct {

    GtkWidget *plugin;              /* Back pointer to the widget */
    GtkWidget *tray_icon;           /* Displayed image */
    int icon_size;                      /* Variables used under wf-panel */
    gboolean bottom;
    GtkWidget *menu;                /* Popup menu */
    GtkWidget *empty;               /* Menuitem shown when no devices */
} KBSwitchPlugin;

extern void kbs_init (KBSwitchPlugin *kbs);
extern void kbs_update_display (KBSwitchPlugin *kbs);
extern gboolean kbs_control_msg (KBSwitchPlugin *kbs, const char *cmd);
extern void kbs_destructor (gpointer user_data);
