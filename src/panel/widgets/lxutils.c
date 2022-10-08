#include <gtk/gtk.h>
#include <gtk-layer-shell.h>

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


void set_bar_icon (GtkWidget *image, const char *icon, int size)
{
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
    GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), icon,
        size > 32 ? 24 : 16, GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
    if (pixbuf)
    {
        gtk_image_set_from_pixbuf (image, pixbuf);
        g_object_unref (pixbuf);
    }
}
