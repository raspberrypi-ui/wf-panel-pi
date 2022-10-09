extern void position_popup (GtkWindow *popup, GtkWidget *plugin, gboolean bottom);
extern void set_bar_icon (GtkWidget *image, const char *icon, int size);
extern void set_menu_icon (GtkWidget *image, const char *icon, int size);
extern GtkWidget *new_menu_item (const char *text, int maxlen, const char *iconname, int icon_size);
extern void update_menu_icon (GtkWidget *item, GtkWidget *image);
extern const char *get_menu_label (GtkWidget *item);
