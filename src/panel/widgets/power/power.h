#include "lxutils.h"

typedef struct {
    GtkWidget *plugin;              /* Back pointer to the widget */
    int icon_size;                  /* Variables used under wf-panel */
    gboolean bottom;
    GtkWidget *tray_icon;           /* Displayed image */
    int show_icon;
    int last_oc;
    struct udev *udev;
    struct udev_monitor *udev_mon_oc;
    struct udev_monitor *udev_mon_lv;
    int fd_oc;
    int fd_lv;
    GThread *oc_thread;
    GThread *lv_thread;
} PowerPlugin;

extern void power_init (PowerPlugin *pt);
extern void power_update_display (PowerPlugin *pt);
extern void power_destructor (gpointer user_data);


