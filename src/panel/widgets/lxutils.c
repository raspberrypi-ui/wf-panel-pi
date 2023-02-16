#include <gtk/gtk.h>
#include <gtk-layer-shell.h>

#define MENU_ICON_SPACE 6

GtkWidget *m_button, *m_menu;
gulong m_handle;

static void menu_hidden (GtkWidget *menu, gpointer user_data)
{
    GtkWidget *panel = gtk_widget_get_parent (gtk_widget_get_parent (gtk_widget_get_parent (m_button)));
    gtk_layer_set_keyboard_interactivity (panel, FALSE);
}

static void committed (GdkWindow *win, gpointer user_data)
{
    g_signal_handler_disconnect (win, m_handle);
    g_signal_connect (m_menu, "hide", G_CALLBACK (menu_hidden), NULL);
    gtk_menu_popup_at_widget (GTK_MENU (m_menu), m_button, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
}

void show_menu_with_kbd (GtkWidget *button, GtkWidget *menu)
{
    // simulate a leave event on the button to hide the prelight */
    GdkEventCrossing *ev = gdk_event_new (GDK_LEAVE_NOTIFY);
    ev->window = gtk_button_get_event_window (button);
    ev->time = GDK_CURRENT_TIME;
    ev->mode = GDK_CROSSING_NORMAL;
    ev->send_event = TRUE;
    gtk_main_do_event (ev);

    GtkWidget *panel = gtk_widget_get_parent (gtk_widget_get_parent (gtk_widget_get_parent (button)));
    m_button = button;
    m_menu = menu;
    gtk_layer_set_keyboard_interactivity (panel, TRUE);
    m_handle = g_signal_connect (gtk_widget_get_window (panel), "committed", G_CALLBACK (committed), NULL);
}

void position_popup (GtkWindow *popup, GtkWidget *plugin, gboolean bottom)
{
    GtkAllocation allocation;

    gtk_layer_init_for_window (popup);

    /* set the anchor for the popup layer */
    gtk_layer_set_anchor (popup, GTK_LAYER_SHELL_EDGE_TOP, bottom ? FALSE : TRUE);
    gtk_layer_set_anchor (popup, GTK_LAYER_SHELL_EDGE_BOTTOM, bottom ? TRUE : FALSE);
    gtk_layer_set_anchor (popup, GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
    gtk_layer_set_anchor (popup, GTK_LAYER_SHELL_EDGE_RIGHT, FALSE);

    /* set the margin for the popup layer */
    gtk_widget_get_allocation (plugin, &allocation);
    gtk_layer_set_margin (popup, GTK_LAYER_SHELL_EDGE_LEFT, allocation.x);
}

void set_taskbar_icon (GtkWidget *image, const char *icon, int size)
{
    if (!icon) return;
    GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), icon,
        size, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
    if (pixbuf)
    {
        gtk_image_set_from_pixbuf (image, pixbuf);
        g_object_unref (pixbuf);
    }
}

void set_menu_icon (GtkWidget *image, const char *icon, int size)
{
    if (!icon) return;
    GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), icon,
        size > 32 ? 24 : 16, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
    if (pixbuf)
    {
        gtk_image_set_from_pixbuf (image, pixbuf);
        g_object_unref (pixbuf);
    }
}

GtkWidget *new_menu_item (const char *text, int maxlen, const char *iconname, int icon_size)
{
    GtkWidget *item = gtk_menu_item_new ();
    gtk_widget_set_name (item, "panelmenuitem");
    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, MENU_ICON_SPACE);
    GtkWidget *label = gtk_label_new (text);
    GtkWidget *icon = gtk_image_new ();
    set_menu_icon (icon, iconname, icon_size);

    if (maxlen)
    {
        gtk_label_set_max_width_chars (GTK_LABEL (label), maxlen);
        gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
    }

    gtk_container_add (GTK_CONTAINER (item), box);
    gtk_container_add (GTK_CONTAINER (box), icon);
    gtk_container_add (GTK_CONTAINER (box), label);

    return item;
}

void update_menu_icon (GtkWidget *item, GtkWidget *image)
{
    GtkWidget *box = gtk_bin_get_child (GTK_BIN (item));
    GList *children = gtk_container_get_children (GTK_CONTAINER (box));
    GtkWidget *img = (GtkWidget *) children->data;
    gtk_container_remove (GTK_CONTAINER (box), img);
    gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 0);
    gtk_box_reorder_child (GTK_BOX (box), image, 0);
}

const char *get_menu_label (GtkWidget *item)
{
    if (!GTK_IS_BIN (item)) return "";
    GtkWidget *box = gtk_bin_get_child (GTK_BIN (item));
    if (!box) return "";
    GList *children = gtk_container_get_children (GTK_CONTAINER (box));
    if (!children) return "";
    while (children->data)
    {
        if (GTK_IS_LABEL ((GtkWidget *) children->data))
            return gtk_label_get_text (GTK_LABEL ((GtkWidget *) children->data));
        children = children->next;
    }
    return "";
}

void append_menu_icon (GtkWidget *item, GtkWidget *image)
{
    GtkWidget *box = gtk_bin_get_child (GTK_BIN (item));
    gtk_box_pack_end (GTK_BOX (box), image, FALSE, FALSE, 0);
}

gboolean config_setting_lookup_int (const char *plugin, const char *setting, int *value)
{
    GKeyFile *kf;
    GError *err = NULL;
    gboolean res = FALSE;
    int val;
    char *user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi.ini", NULL);

    kf = g_key_file_new ();
    if (g_key_file_load_from_file (kf, user_file, 0, NULL))
    {
        val = g_key_file_get_integer (kf, plugin, setting, &err);
        if (err == NULL)
        {
            *value = val;
            res = TRUE;
        }
    }

    g_key_file_free (kf);
    g_free (user_file);
    return res;
}
