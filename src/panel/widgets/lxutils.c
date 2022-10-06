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

