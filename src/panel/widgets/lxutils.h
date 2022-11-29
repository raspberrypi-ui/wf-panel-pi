#include <gtk/gtk.h>
#include <glib.h>
#include <gtk-layer-shell/gtk-layer-shell.h>

#define GETTEXT_PACKAGE "wf-panel"
#define MENU_ICON_SPACE 6

extern void show_menu_with_kbd (GtkWidget *button, GtkWidget *menu);
extern void position_popup (GtkWindow *popup, GtkWidget *plugin, gboolean bottom);
extern void set_taskbar_icon (GtkWidget *image, const char *icon, int size);
extern void set_menu_icon (GtkWidget *image, const char *icon, int size);
extern GtkWidget *new_menu_item (const char *text, int maxlen, const char *iconname, int icon_size);
extern void update_menu_icon (GtkWidget *item, GtkWidget *image);
extern const char *get_menu_label (GtkWidget *item);
extern void append_menu_icon (GtkWidget *item, GtkWidget *image);
extern void lxpanel_notify_init (gboolean enable, gint timeout);
extern int lxpanel_notify (const char *message);
extern void lxpanel_notify_clear (int seq);

extern char **environ;

