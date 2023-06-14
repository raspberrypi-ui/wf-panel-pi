#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "config.h"

GtkListStore *widgets;
GtkWidget *ltv, *ctv, *rtv;
GtkWidget *ladd, *lrem, *radd, *rrem, *lup, *ldn, *rup, *rdn, *sup, *sdn;
GtkTreeModel *fleft, *fright, *fcent, *sleft, *sright, *scent;
int lh, ch, rh;
gboolean found;
char sbuf[32];

#define DEFAULT_LEFT "smenu spacing4 launchers spacing8 window-list"
#define DEFAULT_RIGHT "ejecter updater spacing2 bluetooth spacing2 netman spacing2 volumepulse micpulse spacing2 clock spacing2 power spacing2"

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
    int space;

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

    if (sscanf (str, "spacing%d", &space) == 1)
    {
        if (!space) return _("Spacer");
        else
        {
            sprintf (sbuf, _("Spacer (%d)"), space);
            return sbuf;
        }
    }

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
    GtkTreeSelection *sel;
    int nitems;
    GtkTreePath *toppath, *endpath;
    char *pstr = NULL;

    switch ((long) data)
    {
        case  1 :   g_signal_handler_block (ctv, ch);
                    g_signal_handler_block (rtv, rh);
                    gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (ctv)));
                    gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (rtv)));
                    g_signal_handler_unblock (ctv, ch);
                    g_signal_handler_unblock (rtv, rh);

                    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (ltv));
                    nitems = gtk_tree_model_iter_n_children (fleft, NULL);
                    toppath = gtk_tree_path_new_from_string ("0");
                    pstr = g_strdup_printf ("%d", nitems ? nitems - 1 : nitems);
                    endpath = gtk_tree_path_new_from_string (pstr);
                    gtk_widget_set_sensitive (ladd, FALSE);
                    gtk_widget_set_sensitive (radd, FALSE);
                    gtk_widget_set_sensitive (lrem, nitems > 0);
                    gtk_widget_set_sensitive (rrem, FALSE);
                    gtk_widget_set_sensitive (lup, nitems > 0 && !gtk_tree_selection_path_is_selected (sel, toppath) ? TRUE : FALSE);
                    gtk_widget_set_sensitive (ldn, nitems > 0 && !gtk_tree_selection_path_is_selected (sel, endpath) ? TRUE : FALSE);
                    gtk_widget_set_sensitive (rup, FALSE);
                    gtk_widget_set_sensitive (rdn, FALSE);
                    break;

        case  0 :   g_signal_handler_block (ltv, lh);
                    g_signal_handler_block (rtv, rh);
                    gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (ltv)));
                    gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (rtv)));
                    g_signal_handler_unblock (ltv, lh);
                    g_signal_handler_unblock (rtv, rh);

                    nitems = gtk_tree_model_iter_n_children (fcent, NULL);
                    gtk_widget_set_sensitive (ladd, nitems > 0);
                    gtk_widget_set_sensitive (radd, nitems > 0);
                    gtk_widget_set_sensitive (lrem, FALSE);
                    gtk_widget_set_sensitive (rrem, FALSE);
                    gtk_widget_set_sensitive (lup, FALSE);
                    gtk_widget_set_sensitive (ldn, FALSE);
                    gtk_widget_set_sensitive (rup, FALSE);
                    gtk_widget_set_sensitive (rdn, FALSE);
                    break;

        case -1 :   g_signal_handler_block (ltv, lh);
                    g_signal_handler_block (ctv, ch);
                    gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (ltv)));
                    gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (ctv)));
                    g_signal_handler_unblock (ltv, lh);
                    g_signal_handler_unblock (ctv, ch);

                    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (rtv));
                    nitems = gtk_tree_model_iter_n_children (fright, NULL);
                    toppath = gtk_tree_path_new_from_string ("0");
                    pstr = g_strdup_printf ("%d", nitems ? nitems - 1 : nitems);
                    endpath = gtk_tree_path_new_from_string (pstr);
                    gtk_widget_set_sensitive (ladd, FALSE);
                    gtk_widget_set_sensitive (radd, FALSE);
                    gtk_widget_set_sensitive (lrem, FALSE);
                    gtk_widget_set_sensitive (rrem, nitems > 0);
                    gtk_widget_set_sensitive (lup, FALSE);
                    gtk_widget_set_sensitive (ldn, FALSE);
                    gtk_widget_set_sensitive (rup, nitems > 0 && !gtk_tree_selection_path_is_selected (sel, toppath) ? TRUE : FALSE);
                    gtk_widget_set_sensitive (rdn, nitems > 0 && !gtk_tree_selection_path_is_selected (sel, endpath) ? TRUE : FALSE);
                    break;
    }

    if (pstr) g_free (pstr);
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
            if (strncmp (type, "spacing", 7))
                gtk_list_store_set (widgets, &citer, 2, index + 1, -1);
            else
                gtk_list_store_insert_with_values (widgets, NULL, -1, 0, display_name ("spacing4"), 1, "spacing4", 2, index + 1, -1);
        }
        else
        {
            index = gtk_tree_model_iter_n_children (fright, NULL);
            if (strncmp (type, "spacing", 7))
                gtk_list_store_set (widgets, &citer, 2, -index - 1, -1);
            else
                gtk_list_store_insert_with_values (widgets, NULL, -1, 0, display_name ("spacing4"), 1, "spacing4", 2, -index - 1, -1);
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

static int selection (void)
{
    GtkTreeSelection *sel;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (ltv));
    if (gtk_tree_selection_get_selected (sel, &sleft, NULL)) return 1;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (rtv));
    if (gtk_tree_selection_get_selected (sel, &sright, NULL)) return -1;

    return 0;
}

static void rem_widget (GtkButton *, gpointer)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod, *fmod;
    GtkTreeIter iter, siter, citer;
    int index;
    char *type;

    switch (selection ())
    {
        case 1 :    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (ltv));
                    fmod = fleft;
                    break;
        case -1 :   sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (rtv));
                    fmod = fright;
                    break;
        default :   return;
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

static void move_widget (GtkButton *, gpointer data)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod, *fmod;
    GtkTreeIter iter, siter, citer;
    int index, lorr = selection ();

    switch (lorr)
    {
        case 1 :    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (ltv));
                    fmod = fleft;
                    break;
        case -1 :   sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (rtv));
                    fmod = fright;
                    break;
        default :   return;
    }

    if (gtk_tree_selection_get_selected (sel, &mod, &iter))
    {
        gtk_tree_model_get (mod, &iter, 2, &index, -1);
        gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (mod), &siter, &iter);
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (fmod), &citer, &siter);

        if ((long) data == 1)
        {
            if (index == lorr) return;
        }
        else
        {
            if (index == lorr * gtk_tree_model_iter_n_children (fmod, NULL)) return;
        }

        if ((long) data == lorr)
        {
            gtk_tree_model_foreach (fmod, up, (void *) index);
            gtk_list_store_set (widgets, &citer, 2, index - 1, -1);
        }
        else
        {
            gtk_tree_model_foreach (fmod, down, (void *) index);
            gtk_list_store_set (widgets, &citer, 2, index + 1, -1);
        }

        unselect (NULL, (void *) lorr);
    }
}

static void mod_space (GtkButton *, gpointer data)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod, *fmod;
    GtkTreeIter iter, siter, citer;
    int index;
    char *type;

    switch (selection ())
    {
        case 1 :    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (ltv));
                    fmod = fleft;
                    break;
        case -1 :   sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (rtv));
                    fmod = fright;
                    break;
        default :   return;
    }

    if (gtk_tree_selection_get_selected (sel, &mod, &iter))
    {
        gtk_tree_model_get (mod, &iter, 1, &type, -1);
        gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (mod), &siter, &iter);
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (fmod), &citer, &siter);

        if (!strncmp (type, "spacing", 7))
        {
            sscanf (type, "spacing%d", &index);
            g_free (type);

            index += (long) data;
            if (index < 1) index = 1;

            type = g_strdup_printf ("spacing%d\n", index);

            gtk_list_store_set (widgets, &citer, 0, display_name (type), 1, type, -1);
        }
        g_free (type);
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

    GKeyFile *kf;
    char *lvalue, *rvalue, *token;
    int pos;
    char *user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi.ini", NULL);

    kf = g_key_file_new ();
    if (g_key_file_load_from_file (kf, user_file, 0, NULL))
    {
        lvalue = g_key_file_get_string (kf, "panel", "widgets_left", NULL);
        if (lvalue == NULL) lvalue = g_strdup (DEFAULT_LEFT);

        rvalue = g_key_file_get_string (kf, "panel", "widgets_right", NULL);
        if (rvalue == NULL) rvalue = g_strdup (DEFAULT_RIGHT);
    }
    else
    {
        lvalue = g_strdup (DEFAULT_LEFT);
        rvalue = g_strdup (DEFAULT_RIGHT);
    }

    pos = 1;
    token = strtok (lvalue, " ");
    while (token)
    {
        gtk_list_store_insert_with_values (widgets, NULL, -1, 0, display_name (token), 1, token, 2, pos++, -1);
        token = strtok (NULL, " ");
    }

    pos = -1;
    token = strtok (rvalue, " ");
    while (token)
    {
        gtk_list_store_insert_with_values (widgets, NULL, -1, 0, display_name (token), 1, token, 2, pos--, -1);
        token = strtok (NULL, " ");
    }

    g_free (lvalue);
    g_free (rvalue);
    g_key_file_free (kf);
    g_free (user_file);

    for (pos = 0; pos < NUM_WIDGETS; pos++)
    {
        found = FALSE;
        gtk_tree_model_foreach (GTK_TREE_MODEL (widgets), add_unused, wids[pos]);
        if (!found) gtk_list_store_insert_with_values (widgets, NULL, -1, 0, display_name (wids[pos]), 1, wids[pos], 2, 0, -1);
    }
    gtk_list_store_insert_with_values (widgets, NULL, -1, 0, display_name ("spacing0"), 1, "spacing0", 2, 0, -1);
}

void open_config_dialog (void)
{
    GtkBuilder *builder;
    GtkWidget *dlg;
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
    sup = (GtkWidget *) gtk_builder_get_object (builder, "spacei_btn");
    sdn = (GtkWidget *) gtk_builder_get_object (builder, "spaced_btn");

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

    g_signal_connect (lrem, "clicked", G_CALLBACK (rem_widget), NULL);
    g_signal_connect (rrem, "clicked", G_CALLBACK (rem_widget), NULL);

    g_signal_connect (lup, "clicked", G_CALLBACK (move_widget), (void *) 1);
    g_signal_connect (rup, "clicked", G_CALLBACK (move_widget), (void *) 1);
    g_signal_connect (ldn, "clicked", G_CALLBACK (move_widget), (void *) -1);
    g_signal_connect (rdn, "clicked", G_CALLBACK (move_widget), (void *) -1);

    g_signal_connect (sup, "clicked", G_CALLBACK (mod_space), (void *) 1);
    g_signal_connect (sdn, "clicked", G_CALLBACK (mod_space), (void *) -1);

    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (ltv), -1, "Left", trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (ctv), -1, "Unused", trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (rtv), -1, "Right", trend, "text", 0, NULL);

    gtk_window_set_default_size (GTK_WINDOW(dlg), 500, 300);

    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        GtkTreeIter iter;
        char *str;
        char config[1000];
        GKeyFile *kf;
        gsize len;

        // construct the file path
        char *user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi.ini", NULL);

        // read in data from file to a key file
        kf = g_key_file_new ();
        g_key_file_load_from_file (kf, user_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

        config[0] = 0;
        if (gtk_tree_model_get_iter_first (sleft, &iter))
        {
            do
            {
                gtk_tree_model_get (sleft, &iter, 1, &str, -1);
                strcat (config, str);
                strcat (config, " ");
                g_free (str);
            }
            while (gtk_tree_model_iter_next (sleft, &iter));
        }
        g_key_file_set_string (kf, "panel", "widgets_left", config);

        config[0] = 0;
        if (gtk_tree_model_get_iter_first (sright, &iter))
        {
            do
            {
                gtk_tree_model_get (sright, &iter, 1, &str, -1);
                strcat (config, str);
                strcat (config, " ");
                g_free (str);
            }
            while (gtk_tree_model_iter_next (sright, &iter));
        }
        g_key_file_set_string (kf, "panel", "widgets_right", config);

        // write the modified key file out
        str = g_key_file_to_data (kf, &len, NULL);
        g_file_set_contents (user_file, str, len, NULL);

        g_free (str);
        g_key_file_free (kf);
        g_free (user_file);
    }

    gtk_widget_destroy (dlg);
}
