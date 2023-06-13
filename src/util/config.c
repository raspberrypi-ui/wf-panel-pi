#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "config.h"

GtkListStore *widgets;
GtkWidget *ltv, *ctv, *rtv;
GtkWidget *ladd, *lrem, *radd, *rrem;
int lh, ch, rh;


static gboolean filter_widgets (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    int index;

    gtk_tree_model_get (model, iter, 2, &index, -1);

    if ((long) data > 0 && index > 0) return TRUE;
    if ((long) data < 0 && index < 0) return TRUE;
    if ((long) data == 0 && index == 0) return TRUE;

    return FALSE;
}

static void unselect (GtkTreeView *, gpointer data)
{
    switch ((long) data)
    {
        case  1 :   g_signal_handler_block (ctv, ch);
                    g_signal_handler_block (rtv, rh);
                    gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (ctv)));
                    gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (rtv)));
                    g_signal_handler_unblock (ctv, ch);
                    g_signal_handler_unblock (rtv, rh);
                    gtk_widget_set_sensitive (ladd, FALSE);
                    gtk_widget_set_sensitive (radd, FALSE);
                    gtk_widget_set_sensitive (lrem, TRUE);
                    gtk_widget_set_sensitive (rrem, FALSE);
                    break;

        case  0 :   g_signal_handler_block (ltv, lh);
                    g_signal_handler_block (rtv, rh);
                    gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (ltv)));
                    gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (rtv)));
                    g_signal_handler_unblock (ltv, lh);
                    g_signal_handler_unblock (rtv, rh);
                    gtk_widget_set_sensitive (ladd, TRUE);
                    gtk_widget_set_sensitive (radd, TRUE);
                    gtk_widget_set_sensitive (lrem, FALSE);
                    gtk_widget_set_sensitive (rrem, FALSE);
                    break;

        case -1 :   g_signal_handler_block (ltv, lh);
                    g_signal_handler_block (ctv, ch);
                    gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (ltv)));
                    gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (ctv)));
                    g_signal_handler_unblock (ltv, lh);
                    g_signal_handler_unblock (ctv, ch);
                    gtk_widget_set_sensitive (ladd, FALSE);
                    gtk_widget_set_sensitive (radd, FALSE);
                    gtk_widget_set_sensitive (lrem, FALSE);
                    gtk_widget_set_sensitive (rrem, TRUE);
                    break;
    }
}

static void add_widget (GtkButton *, gpointer data)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod;
    GtkTreeIter iter, citer;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (ctv));

    if (gtk_tree_selection_get_selected (sel, &mod, &iter))
    {
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (mod), &citer, &iter);
        if ((long) data == 1)
            gtk_list_store_set (widgets, &citer, 2, 1, -1);
        else
            gtk_list_store_set (widgets, &citer, 2, -1, -1);
    }
}

static void rem_widget (GtkButton *, gpointer data)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod;
    GtkTreeIter iter, citer;

    if ((long) data == 1)
        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (ltv));
    else
        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (rtv));

    if (gtk_tree_selection_get_selected (sel, &mod, &iter))
    {
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (mod), &citer, &iter);
        gtk_list_store_set (widgets, &citer, 2, 0, -1);
    }
}


void open_config_dialog (void)
{
    GtkBuilder *builder;
    GtkWidget *dlg;
    GtkTreeModel *fleft, *fright, *fcent;
    GtkCellRenderer *trend = gtk_cell_renderer_text_new ();
    int pos = 0;

    textdomain (GETTEXT_PACKAGE);

    builder = gtk_builder_new_from_file (PACKAGE_DATA_DIR "/ui/config.ui");
    dlg = (GtkWidget *) gtk_builder_get_object (builder, "config_dlg");
    ltv = (GtkWidget *) gtk_builder_get_object (builder, "left_tv");
    ctv = (GtkWidget *) gtk_builder_get_object (builder, "cent_tv");
    rtv = (GtkWidget *) gtk_builder_get_object (builder, "right_tv");
    ladd = (GtkWidget *) gtk_builder_get_object (builder, "add_l_btn");
    radd = (GtkWidget *) gtk_builder_get_object (builder, "add_r_btn");
    lrem = (GtkWidget *) gtk_builder_get_object (builder, "rem_l_btn");
    rrem = (GtkWidget *) gtk_builder_get_object (builder, "rem_r_btn");

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

    lh = g_signal_connect (ltv, "cursor-changed", G_CALLBACK (unselect), (void *) 1);
    ch = g_signal_connect (ctv, "cursor-changed", G_CALLBACK (unselect), (void *) 0);
    rh = g_signal_connect (rtv, "cursor-changed", G_CALLBACK (unselect), (void *) -1);

    g_signal_connect (ladd, "clicked", G_CALLBACK (add_widget), (void *) 1);
    g_signal_connect (radd, "clicked", G_CALLBACK (add_widget), (void *) -1);

    g_signal_connect (lrem, "clicked", G_CALLBACK (rem_widget), (void *) 1);
    g_signal_connect (rrem, "clicked", G_CALLBACK (rem_widget), (void *) -1);

    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (ltv), -1, "Left", trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (ctv), -1, "Unused", trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (rtv), -1, "Right", trend, "text", 0, NULL);

    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        printf ("saving config\n");
    }
    gtk_widget_destroy (dlg);
}
