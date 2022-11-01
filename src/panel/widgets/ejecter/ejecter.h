#include "../lxutils.h"

/* Plug-in global data */

typedef struct {

    GtkWidget *plugin;              /* Back pointer to the widget */
    //LXPanel *panel;                 /* Back pointer to panel */
    GtkWidget *tray_icon;           /* Displayed image */
    //config_setting_t *settings;     /* Plugin settings */
    int icon_size;                      /* Variables used under wf-panel */
    gboolean bottom;
    GtkWidget *popup;               /* Popup message */
    GtkWidget *alignment;           /* Alignment object in popup message */
    GtkWidget *box;                 /* Vbox in popup message */
    GtkWidget *menu;                /* Popup menu */
    GtkWidget *empty;               /* Menuitem shown when no devices */
    GVolumeMonitor *monitor;
    gboolean autohide;
    GList *ejdrives;
    guint hide_timer;
} EjecterPlugin;

extern void ej_init (EjecterPlugin *ej);
extern void ej_update_display (EjecterPlugin *ej);
extern gboolean ejecter_control_msg (EjecterPlugin *ej, const char *cmd);
