#include <gtk/gtk.h>
#include <glib.h>
#include <gtk-layer-shell/gtk-layer-shell.h>

#define GETTEXT_PACKAGE "wf-panel"
#define PACKAGE_DATA_DIR "/usr/share/lxpanel"
#define MENU_ICON_SPACE 6

extern void position_popup (GtkWindow *popup, GtkWidget *plugin, gboolean bottom);
extern void set_taskbar_icon (GtkWidget *image, const char *icon, int size);
extern void set_menu_icon (GtkWidget *image, const char *icon, int size);
extern GtkWidget *new_menu_item (const char *text, int maxlen, const char *iconname, int icon_size);
extern void update_menu_icon (GtkWidget *item, GtkWidget *image);
extern const char *get_menu_label (GtkWidget *item);

extern char **environ;

