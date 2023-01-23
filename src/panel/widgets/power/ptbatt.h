#include "../lxutils.h"
#include "batt_sys.h"

typedef struct {
    GtkWidget *plugin;              /* Back pointer to the widget */
    int icon_size;                  /* Variables used under wf-panel */
    gboolean bottom;
    GtkWidget *tray_icon;           /* Displayed image */
    battery *batt;
    GdkPixbuf *plug;
    GdkPixbuf *flash;
    GtkWidget *popup;               /* Popup message */
    GtkWidget *alignment;           /* Alignment object in popup message */
    GtkWidget *box;                 /* Vbox in popup message */
    guint timer;
    guint vtimer;
    gboolean pt_batt_avail;
    void *context;
    void *requester;
    gboolean ispi;
} PtBattPlugin;

extern void power_init (PtBattPlugin *pt);
extern void power_update_display (PtBattPlugin *pt);


