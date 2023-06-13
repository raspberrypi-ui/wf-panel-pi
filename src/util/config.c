#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "config.h"


static gboolean filter_widgets (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    int index;

    gtk_tree_model_get (model, iter, 2, &index, -1);

    if ((long) data > 0 && index > 0) return TRUE;
    if ((long) data < 0 && index < 0) return TRUE;
    if ((long) data == 0 && index == 0) return TRUE;

    return FALSE;
}


void open_config_dialog (void)
{
    GtkBuilder *builder;
    GtkWidget *dlg, *ltv, *ctv, *rtv;
    GtkListStore *widgets;
    GtkTreeModel *fleft, *fright, *fcent;
    GtkCellRenderer *trend = gtk_cell_renderer_text_new ();
    int pos = 0;

    textdomain (GETTEXT_PACKAGE);

    builder = gtk_builder_new_from_file (PACKAGE_DATA_DIR "/ui/config.ui");
    dlg = (GtkWidget *) gtk_builder_get_object (builder, "config_dlg");
    ltv = (GtkWidget *) gtk_builder_get_object (builder, "left_tv");
    ctv = (GtkWidget *) gtk_builder_get_object (builder, "cent_tv");
    rtv = (GtkWidget *) gtk_builder_get_object (builder, "right_tv");

    widgets = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

    gtk_list_store_insert_with_values (widgets, NULL, pos++, 0, "Menu", 1, "menu", 2, 1, -1);
    gtk_list_store_insert_with_values (widgets, NULL, pos++, 0, "Launcher", 1, "launchers", 2, 2, -1);
    gtk_list_store_insert_with_values (widgets, NULL, pos++, 0, "Bluetooth", 1, "bluetooth", 2, -1, -1);
    gtk_list_store_insert_with_values (widgets, NULL, pos++, 0, "Clock", 1, "clock", 2, 0, -1);

    fleft = gtk_tree_model_filter_new (GTK_TREE_MODEL (widgets), NULL);
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (fleft), (GtkTreeModelFilterVisibleFunc) filter_widgets, (void *) 1, NULL);

    fcent = gtk_tree_model_filter_new (GTK_TREE_MODEL (widgets), NULL);
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (fcent), (GtkTreeModelFilterVisibleFunc) filter_widgets, (void *) 0, NULL);

    fright = gtk_tree_model_filter_new (GTK_TREE_MODEL (widgets), NULL);
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (fright), (GtkTreeModelFilterVisibleFunc) filter_widgets, (void *) -1, NULL);

    gtk_tree_view_set_model (GTK_TREE_VIEW (ltv), fleft);
    gtk_tree_view_set_model (GTK_TREE_VIEW (ctv), fcent);
    gtk_tree_view_set_model (GTK_TREE_VIEW (rtv), fright);

    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (ltv), -1, "Left", trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (ctv), -1, "Unused", trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (rtv), -1, "Right", trend, "text", 0, NULL);

    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        printf ("saving config\n");
    }
    gtk_widget_destroy (dlg);
}
