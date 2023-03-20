#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <menu-cache.h>
#include <libfm/fm-gtk.h>

#include "../lxutils.h"

typedef struct {
    //LXPanel *panel;
    //config_setting_t *settings;
    int icon_size;                      /* Variables used under wf-panel */
    gboolean bottom;
    GtkWidget *plugin, *img, *menu;
    GtkWidget *swin, *srch, *stv, *scr;
    GtkListStore *applist;
    char *icon;
    int padding;
    int height;
    int rheight;
    gboolean fixed;

    MenuCache* menu_cache;
    gpointer reload_notify;
    FmDndSrc *ds;
} MenuPlugin;

extern void menu_init (MenuPlugin *m);
extern void menu_update_display (MenuPlugin *m);
extern void menu_show_menu (MenuPlugin *m);
extern void menu_destructor (gpointer user_data);
