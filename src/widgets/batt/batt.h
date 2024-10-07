#include "lxutils.h"
#include "batt_sys.h"

typedef struct {
    GtkWidget *plugin;              /* Back pointer to the widget */
    int icon_size;                  /* Variables used under wf-panel */
    gboolean bottom;
    GtkGesture *gesture;
    GtkWidget *tray_icon;           /* Displayed image */
    battery *batt;
    GdkPixbuf *plug;
    GdkPixbuf *flash;
    guint timer;
    int batt_num;
} PtBattPlugin;

extern void batt_init (PtBattPlugin *pt);
extern void batt_update_display (PtBattPlugin *pt);
extern void batt_destructor (gpointer user_data);


