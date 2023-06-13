#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "config.h"

GtkListStore *widgets;
GtkWidget *ltv, *ctv, *rtv;
GtkWidget *ladd, *lrem, *radd, *rrem;
GtkTreeModel *fleft, *fright, *fcent, *sleft, *sright, *scent;
int lh, ch, rh;
gboolean found;

#define NUM_WIDGETS 14

char *wids[NUM_WIDGETS] = {
    "smenu",
    "launchers",
    "window-list",
    "ejecter",
    "updater",
    "bluetooth",
    "netman",
    "volumepulse",
    "micpulse",
    "clock",
    "power",
    "cpu",
    "gpu",
    "cputemp"
};

static const char *display_name (char *str)
{
    if (!strncmp (str, "spacing", 7)) return _("Spacer");
    if (!g_strcmp0 (str, "smenu")) return _("Menu");
    if (!g_strcmp0 (str, "launchers")) return _("Launcher");
    if (!g_strcmp0 (str, "window-list")) return _("Window List");
    if (!g_strcmp0 (str, "ejecter")) return _("Ejecter");
    if (!g_strcmp0 (str, "updater")) return _("Updater");
    if (!g_strcmp0 (str, "bluetooth")) return _("Bluetooth");
    if (!g_strcmp0 (str, "netman")) return _("Network");
    if (!g_strcmp0 (str, "volumepulse")) return _("Volume");
    if (!g_strcmp0 (str, "micpulse")) return _("Microphone");
    if (!g_strcmp0 (str, "clock")) return _("Clock");
    if (!g_strcmp0 (str, "power")) return _("Power");
    if (!g_strcmp0 (str, "cpu")) return _("CPU");
    if (!g_strcmp0 (str, "gpu")) return _("GPU");
    if (!g_strcmp0 (str, "cputemp")) return _("CPU Temp");
    return _("<Unknown>");
}


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
    GtkTreeIter iter, siter, citer;
    int index;
    char *type;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (ctv));

    if (gtk_tree_selection_get_selected (sel, &mod, &iter))
    {
        gtk_tree_model_get (mod, &iter, 1, &type, -1);
        gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (mod), &siter, &iter);
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (fcent), &citer, &siter);
        if ((long) data == 1)
        {
            index = gtk_tree_model_iter_n_children (fleft, NULL);
            if (g_strcmp0 (type, "spacing"))
                gtk_list_store_set (widgets, &citer, 2, index + 1, -1);
            else
                gtk_list_store_insert_with_values (widgets, NULL, -1, 0, display_name ("spacing"), 1, "spacing", 2, index + 1, -1);
        }
        else
        {
            index = gtk_tree_model_iter_n_children (fright, NULL);
            if (strncmp (type, "spacing", 7))
                gtk_list_store_set (widgets, &citer, 2, -index - 1, -1);
            else
                gtk_list_store_insert_with_values (widgets, NULL, -1, 0, display_name ("spacing"), 1, "spacing", 2, -index - 1, -1);
        }
        g_free (type);
    }
}

static gboolean renum (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data)
{
    // if index > data, subtract 1
    GtkTreeIter citer;
    int index;
    gtk_tree_model_get (mod, iter, 2, &index, -1);
    if (index > 0 && index > ((long) data))
    {
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (mod), &citer, iter);
        gtk_list_store_set (widgets, &citer, 2, index - 1, -1);
    }
    if (index < 0 && index < ((long) data))
    {
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (mod), &citer, iter);
        gtk_list_store_set (widgets, &citer, 2, index + 1, -1);
    }
    return FALSE;
}

static void rem_widget (GtkButton *, gpointer data)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod, *fmod;
    GtkTreeIter iter, siter, citer;
    int index;
    char *type;

    if ((long) data == 1)
    {
        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (ltv));
        fmod = fleft;
    }
    else
    {
        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (rtv));
        fmod = fright;
    }

    if (gtk_tree_selection_get_selected (sel, &mod, &iter))
    {
        gtk_tree_model_get (mod, &iter, 1, &type, 2, &index, -1);
        gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (mod), &siter, &iter);
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (fmod), &citer, &siter);
        if (strncmp (type, "spacing", 7))
            gtk_list_store_set (widgets, &citer, 2, 0, -1);
        else
            gtk_list_store_remove (widgets, &citer);
    }

    // re-number the widgets in the list....
    gtk_tree_model_foreach (fmod, renum, (void *) index);
}

static gboolean up (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data)
{
    // find index - 1, make it index
    GtkTreeIter citer;
    int index;
    gtk_tree_model_get (mod, iter, 2, &index, -1);
    if (index == ((long) data) - 1)
    {
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (mod), &citer, iter);
        gtk_list_store_set (widgets, &citer, 2, index + 1, -1);
        return TRUE;
    }
    return FALSE;
}

static gboolean down (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data)
{
    // find index + 1, make it index
    GtkTreeIter citer;
    int index;
    gtk_tree_model_get (mod, iter, 2, &index, -1);
    if (index == ((long) data) + 1)
    {
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (mod), &citer, iter);
        gtk_list_store_set (widgets, &citer, 2, index - 1, -1);
        return TRUE;
    }
    return FALSE;
}

static void up_widget (GtkButton *, gpointer data)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod, *fmod;
    GtkTreeIter iter, siter, citer;
    int index;

    if ((long) data == 1)
    {
        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (ltv));
        fmod = fleft;
    }
    else
    {
        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (rtv));
        fmod = fright;
    }

    if (gtk_tree_selection_get_selected (sel, &mod, &iter))
    {
        gtk_tree_model_get (mod, &iter, 2, &index, -1);
        gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (mod), &siter, &iter);
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (fmod), &citer, &siter);

        if ((long) data == 1)
        {
            if (index == 1) return;
            gtk_tree_model_foreach (fmod, up, (void *) index);
            gtk_list_store_set (widgets, &citer, 2, index - 1, -1);
        }
        else
        {
            if (index == -1) return;
            gtk_tree_model_foreach (fmod, down, (void *) index);
            gtk_list_store_set (widgets, &citer, 2, index + 1, -1);
        }
    }
}

static void down_widget (GtkButton *, gpointer data)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod, *fmod;
    GtkTreeIter iter, siter, citer;
    int index;

    if ((long) data == 1)
    {
        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (ltv));
        fmod = fleft;
    }
    else
    {
        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (rtv));
        fmod = fright;
    }

    if (gtk_tree_selection_get_selected (sel, &mod, &iter))
    {
        gtk_tree_model_get (mod, &iter, 2, &index, -1);
        gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (mod), &siter, &iter);
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (fmod), &citer, &siter);

        if ((long) data == 1)
        {
            if (index == gtk_tree_model_iter_n_children (fmod, NULL)) return;
            gtk_tree_model_foreach (fmod, down, (void *) index);
            gtk_list_store_set (widgets, &citer, 2, index + 1, -1);
        }
        else
        {
            if (index == - gtk_tree_model_iter_n_children (fmod, NULL)) return;
            gtk_tree_model_foreach (fmod, up, (void *) index);
            gtk_list_store_set (widgets, &citer, 2, index - 1, -1);
        }
    }
}

static gboolean add_unused (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data)
{
    char *type;
    gtk_tree_model_get (mod, iter, 1, &type, -1);
    if (!g_strcmp0 (data, type))
    {
        found = TRUE;
        return TRUE;
    }
    else return FALSE;
}

void init_config_list (void)
{
    widgets = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

     // !!!!!! handle the case where there is no local config !!!!!!
    GKeyFile *kf;
    GError *err = NULL;
    gboolean res = FALSE;
    char *value, *token;
    int pos;
    char *user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi.ini", NULL);

    kf = g_key_file_new ();
    if (g_key_file_load_from_file (kf, user_file, 0, NULL))
    {
        value = g_key_file_get_string (kf, "panel", "widgets_left", &err);
        if (err == NULL) res = TRUE;
        else value = NULL;

        pos = 1;
        token = strtok (value, " ");
        while (token)
        {
            gtk_list_store_insert_with_values (widgets, NULL, -1, 0, display_name (token), 1, token, 2, pos++, -1);
            token = strtok (NULL, " ");
        }

        value = g_key_file_get_string (kf, "panel", "widgets_right", &err);
        if (err == NULL) res = TRUE;
        else value = NULL;

        pos = -1;
        token = strtok (value, " ");
        while (token)
        {
            gtk_list_store_insert_with_values (widgets, NULL, -1, 0, display_name (token), 1, token, 2, pos--, -1);
            token = strtok (NULL, " ");
        }
    }

    g_key_file_free (kf);
    g_free (user_file);

    for (pos = 0; pos < NUM_WIDGETS; pos++)
    {
        found = FALSE;
        gtk_tree_model_foreach (GTK_TREE_MODEL (widgets), add_unused, wids[pos]);
        if (!found) gtk_list_store_insert_with_values (widgets, NULL, -1, 0, display_name (wids[pos]), 1, wids[pos], 2, 0, -1);
    }
    gtk_list_store_insert_with_values (widgets, NULL, -1, 0, display_name ("spacing"), 1, "spacing", 2, 0, -1);
}

void open_config_dialog (void)
{
    GtkBuilder *builder;
    GtkWidget *dlg;
    GtkWidget *lup, *ldn, *rup, *rdn;
    GtkCellRenderer *trend = gtk_cell_renderer_text_new ();

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
    lup = (GtkWidget *) gtk_builder_get_object (builder, "up_l_btn");
    rup = (GtkWidget *) gtk_builder_get_object (builder, "up_r_btn");
    ldn = (GtkWidget *) gtk_builder_get_object (builder, "dn_l_btn");
    rdn = (GtkWidget *) gtk_builder_get_object (builder, "dn_r_btn");

    init_config_list ();

    fleft = gtk_tree_model_filter_new (GTK_TREE_MODEL (widgets), NULL);
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (fleft), (GtkTreeModelFilterVisibleFunc) filter_widgets, (void *) 1, NULL);
    sleft = gtk_tree_model_sort_new_with_model (fleft);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sleft), 2, GTK_SORT_ASCENDING);

    fcent = gtk_tree_model_filter_new (GTK_TREE_MODEL (widgets), NULL);
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (fcent), (GtkTreeModelFilterVisibleFunc) filter_widgets, (void *) 0, NULL);
    scent = gtk_tree_model_sort_new_with_model (fcent);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (scent), 0, GTK_SORT_ASCENDING);

    fright = gtk_tree_model_filter_new (GTK_TREE_MODEL (widgets), NULL);
    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (fright), (GtkTreeModelFilterVisibleFunc) filter_widgets, (void *) -1, NULL);
    sright = gtk_tree_model_sort_new_with_model (fright);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sright), 2, GTK_SORT_DESCENDING);

    gtk_tree_view_set_model (GTK_TREE_VIEW (ltv), sleft);
    gtk_tree_view_set_model (GTK_TREE_VIEW (ctv), scent);
    gtk_tree_view_set_model (GTK_TREE_VIEW (rtv), sright);

    lh = g_signal_connect (ltv, "cursor-changed", G_CALLBACK (unselect), (void *) 1);
    ch = g_signal_connect (ctv, "cursor-changed", G_CALLBACK (unselect), (void *) 0);
    rh = g_signal_connect (rtv, "cursor-changed", G_CALLBACK (unselect), (void *) -1);

    g_signal_connect (ladd, "clicked", G_CALLBACK (add_widget), (void *) 1);
    g_signal_connect (radd, "clicked", G_CALLBACK (add_widget), (void *) -1);

    g_signal_connect (lrem, "clicked", G_CALLBACK (rem_widget), (void *) 1);
    g_signal_connect (rrem, "clicked", G_CALLBACK (rem_widget), (void *) -1);

    g_signal_connect (lup, "clicked", G_CALLBACK (up_widget), (void *) 1);
    g_signal_connect (rup, "clicked", G_CALLBACK (up_widget), (void *) -1);
    g_signal_connect (ldn, "clicked", G_CALLBACK (down_widget), (void *) 1);
    g_signal_connect (rdn, "clicked", G_CALLBACK (down_widget), (void *) -1);

    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (ltv), -1, "Left", trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (ctv), -1, "Unused", trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (rtv), -1, "Right", trend, "text", 0, NULL);

    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        printf ("saving config\n");
    }
    gtk_widget_destroy (dlg);
}
