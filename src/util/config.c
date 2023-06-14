/*
Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "config.h"

/*----------------------------------------------------------------------------*/
/* Macros and typedefs */
/*----------------------------------------------------------------------------*/

#define DEFAULT_LEFT "smenu spacing4 launchers spacing8 window-list"
#define DEFAULT_RIGHT "ejecter updater spacing2 bluetooth spacing2 netman spacing2 volumepulse micpulse spacing2 clock spacing2 power spacing2"

#define NUM_WIDGETS 14

/*----------------------------------------------------------------------------*/
/* Global data */
/*----------------------------------------------------------------------------*/

static GtkListStore *widgets;
static GtkTreeModel *filt[3], *sort[3];
static GtkWidget *tv[3];
static GtkWidget *ladd, *radd, *rem, *wup, *wdn, *sup, *sdn;
static int hand[3];
static gboolean found;
static char sbuf[32];

/*----------------------------------------------------------------------------*/
/* Function prototypes */
/*----------------------------------------------------------------------------*/

static gboolean renumber (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data);
static gboolean up (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data);
static gboolean down (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data);
static gboolean add_unused (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data);

/*----------------------------------------------------------------------------*/
/* Private functions */
/*----------------------------------------------------------------------------*/

/* Helper function to get the displayed name for a particular widget */

static const char *display_name (const char *str)
{
    int space;

    if (!g_strcmp0 (str, "bluetooth")) return _("Bluetooth");
    if (!g_strcmp0 (str, "clock")) return _("Clock");
    if (!g_strcmp0 (str, "cpu")) return _("CPU");
    if (!g_strcmp0 (str, "cputemp")) return _("CPU Temp");
    if (!g_strcmp0 (str, "ejecter")) return _("Ejecter");
    if (!g_strcmp0 (str, "gpu")) return _("GPU");
    if (!g_strcmp0 (str, "launchers")) return _("Launcher");
    if (!g_strcmp0 (str, "micpulse")) return _("Microphone");
    if (!g_strcmp0 (str, "netman")) return _("Network");
    if (!g_strcmp0 (str, "power")) return _("Power");
    if (!g_strcmp0 (str, "smenu")) return _("Menu");
    if (!g_strcmp0 (str, "updater")) return _("Updater");
    if (!g_strcmp0 (str, "volumepulse")) return _("Volume");
    if (!g_strcmp0 (str, "window-list")) return _("Window List");

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

/* Helper function to locate the currently-highlighted widget */

static int selection (void)
{
    GtkTreeSelection *sel;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[0]));
    if (gtk_tree_selection_get_selected (sel, &sort[0], NULL)) return 1;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[2]));
    if (gtk_tree_selection_get_selected (sel, &sort[2], NULL)) return -1;

    return 0;
}

/* Enable or disable buttons according to current highlight */

static void update_buttons (void)
{
    GtkTreeSelection *sel;
    GtkTreePath *toppath, *endpath;
    GtkTreeModel *mod;
    GtkTreeIter iter;
    int nitems, lorr = selection ();
    char *pstr = NULL, *type = NULL;

    if (lorr == 0)
    {
        nitems = gtk_tree_model_iter_n_children (filt[1], NULL);
        gtk_widget_set_sensitive (ladd, nitems > 0);
        gtk_widget_set_sensitive (radd, nitems > 0);
        gtk_widget_set_sensitive (rem, FALSE);
        gtk_widget_set_sensitive (wup, FALSE);
        gtk_widget_set_sensitive (wdn, FALSE);
        gtk_widget_set_sensitive (sup, FALSE);
        gtk_widget_set_sensitive (sdn, FALSE);
    }
    else
    {
        gtk_widget_set_sensitive (ladd, FALSE);
        gtk_widget_set_sensitive (radd, FALSE);

        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[1 - lorr]));
        nitems = gtk_tree_model_iter_n_children (filt[1 - lorr], NULL);
        toppath = gtk_tree_path_new_from_string ("0");
        pstr = g_strdup_printf ("%d", nitems ? nitems - 1 : nitems);
        endpath = gtk_tree_path_new_from_string (pstr);
        g_free (pstr);

        gtk_widget_set_sensitive (rem, nitems > 0);
        gtk_widget_set_sensitive (wup, nitems > 0 && !gtk_tree_selection_path_is_selected (sel, toppath) ? TRUE : FALSE);
        gtk_widget_set_sensitive (wdn, nitems > 0 && !gtk_tree_selection_path_is_selected (sel, endpath) ? TRUE : FALSE);

        if (gtk_tree_selection_get_selected (sel, &mod, &iter))
        {
            gtk_tree_model_get (mod, &iter, 1, &type, -1);
            if (!sscanf (type, "spacing%d", &nitems)) nitems = 0;
        }
        gtk_widget_set_sensitive (sup, type && !strncmp (type, "spacing", 7));
        gtk_widget_set_sensitive (sdn, type && !strncmp (type, "spacing", 7) && nitems > 1);
        if (type) g_free (type);
    }
}

/* Add the currently-highlighted widget to the left or right side, depending on the value of data */

static void add_widget (GtkButton *, gpointer data)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod;
    GtkTreeIter iter, siter, citer;
    int index, lorr = (long) data == 1 ? 1 : -1;;
    char *type;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[1]));

    if (gtk_tree_selection_get_selected (sel, &mod, &iter))
    {
        gtk_tree_model_get (mod, &iter, 1, &type, -1);
        gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (mod), &siter, &iter);
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (filt[1]), &citer, &siter);

        // just add to the bottom of the list
        index = gtk_tree_model_iter_n_children (filt[1 - lorr], NULL);

        // change index for anything other than a space; space needs to be created
        if (strncmp (type, "spacing", 7))
            gtk_list_store_set (widgets, &citer, 2, lorr * (index + 1), -1);
        else
            gtk_list_store_insert_with_values (widgets, NULL, -1, 0, display_name ("spacing4"), 1, "spacing4", 2, lorr * (index + 1), -1);
        g_free (type);
    }
}

/* Remove the currently-highlighted widget */

static void remove_widget (GtkButton *, gpointer)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod;
    GtkTreeIter iter, siter, citer;
    int index, lorr = selection ();
    char *type;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[1 - lorr]));
    if (gtk_tree_selection_get_selected (sel, &mod, &iter))
    {
        gtk_tree_model_get (mod, &iter, 1, &type, 2, &index, -1);
        gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (mod), &siter, &iter);
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (filt[1 - lorr]), &citer, &siter);

        // change index for anything other than a space; space needs to be deleted
        if (strncmp (type, "spacing", 7))
            gtk_list_store_set (widgets, &citer, 2, 0, -1);
        else
            gtk_list_store_remove (widgets, &citer);
        g_free (type);
    }

    // re-number the widgets in the list below the one removed
    gtk_tree_model_foreach (filt[1 - lorr], renumber, (void *)((long) index));
}

static gboolean renumber (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data)
{
    // if index > data, subtract 1 from index
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

/* Move the currently-highlighted widget left or right */

static void move_widget (GtkButton *, gpointer data)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod;
    GtkTreeIter iter, siter, citer;
    int index, lorr = selection (), dir = (long) data == 1 ? 1 : -1;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[1 - lorr]));
    if (gtk_tree_selection_get_selected (sel, &mod, &iter))
    {
        gtk_tree_model_get (mod, &iter, 2, &index, -1);
        gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (mod), &siter, &iter);
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (filt[1 - lorr]), &citer, &siter);

        // check not trying to move past end of list
        if (dir == 1)
        {
            if (index == lorr) return;
        }
        else
        {
            if (index == lorr * gtk_tree_model_iter_n_children (filt[1 - lorr], NULL)) return;
        }

        // to move, swap the index of the widget moved with that of the adjacent widget
        if (dir == lorr)
        {
            gtk_tree_model_foreach (filt[1 - lorr], up, (void *)((long) index));
            gtk_list_store_set (widgets, &citer, 2, index - 1, -1);
        }
        else
        {
            gtk_tree_model_foreach (filt[1 - lorr], down, (void *)((long) index));
            gtk_list_store_set (widgets, &citer, 2, index + 1, -1);
        }

        update_buttons ();
    }
}

static gboolean up (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data)
{
    // find list entry with index = data - 1, make it data
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
    // find list entry with index = data + 1, make it data
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

/* Change the width of the currently-highlighted space widget */

static void change_space (GtkButton *, gpointer data)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod;
    GtkTreeIter iter, siter, citer;
    int index, lorr = selection (), dir = (long) data == 1 ? 1 : -1;
    char *type;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[1 - lorr]));
    if (gtk_tree_selection_get_selected (sel, &mod, &iter))
    {
        gtk_tree_model_get (mod, &iter, 1, &type, -1);
        gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (mod), &siter, &iter);
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (filt[1 - lorr]), &citer, &siter);

        if (!strncmp (type, "spacing", 7))
        {
            // read the current spacing
            sscanf (type, "spacing%d", &index);
            g_free (type);

            // increment or decrement, pegging at 1
            index += dir;
            if (index < 1) index = 1;

            // update both the widget type and the displayed name
            type = g_strdup_printf ("spacing%d", index);
            gtk_list_store_set (widgets, &citer, 0, display_name (type), 1, type, -1);
        }
        g_free (type);

        update_buttons ();
    }
}

/* Read in config from local configuration file, or use default */

static void read_config (void)
{
    const char *all_wids[NUM_WIDGETS] = {
        "bluetooth",
        "clock",
        "cpu",
        "cputemp",
        "ejecter",
        "gpu",
        "launchers",
        "micpulse",
        "netman",
        "power",
        "smenu",
        "updater",
        "volumepulse",
        "window-list"
    };
    char *lvalue, *rvalue, *token;
    int pos;

    // construct the file path
    char *user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi.ini", NULL);

    // read in data from file to a key file
    GKeyFile *kf = g_key_file_new ();
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

    // add each space-separated widget from the lines in the file to the list store
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

    // add any unused widgets to the list store so they can be added by the user
    for (pos = 0; pos < NUM_WIDGETS; pos++)
    {
        found = FALSE;
        gtk_tree_model_foreach (GTK_TREE_MODEL (widgets), add_unused, (void *) all_wids[pos]);
        if (!found) gtk_list_store_insert_with_values (widgets, NULL, -1, 0, display_name (all_wids[pos]), 1, all_wids[pos], 2, 0, -1);
    }
    gtk_list_store_insert_with_values (widgets, NULL, -1, 0, display_name ("spacing0"), 1, "spacing0", 2, 0, -1);
}

static gboolean add_unused (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data)
{
    char *type;
    gtk_tree_model_get (mod, iter, 1, &type, -1);
    if (!g_strcmp0 (data, type)) found = TRUE;
    g_free (type);
    return found;
}

/* Write config to local configuration file */

static void write_config (void)
{
    GtkTreeIter iter;
    char *str;
    char config[1000];
    gsize len;

    // construct the file path
    char *user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi.ini", NULL);

    // read in data from file to a key file
    GKeyFile *kf = g_key_file_new ();
    g_key_file_load_from_file (kf, user_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

    // iterate through the two tree models, concatenating widget names to a space-separated string
    config[0] = 0;
    if (gtk_tree_model_get_iter_first (sort[0], &iter))
    {
        do
        {
            gtk_tree_model_get (sort[0], &iter, 1, &str, -1);
            strcat (config, str);
            strcat (config, " ");
            g_free (str);
        }
        while (gtk_tree_model_iter_next (sort[0], &iter));
    }
    g_key_file_set_string (kf, "panel", "widgets_left", config);

    config[0] = 0;
    if (gtk_tree_model_get_iter_first (sort[2], &iter))
    {
        do
        {
            gtk_tree_model_get (sort[2], &iter, 1, &str, -1);
            strcat (config, str);
            strcat (config, " ");
            g_free (str);
        }
        while (gtk_tree_model_iter_next (sort[2], &iter));
    }
    g_key_file_set_string (kf, "panel", "widgets_right", config);

    // write the modified key file out
    str = g_key_file_to_data (kf, &len, NULL);
    g_file_set_contents (user_file, str, len, NULL);

    g_free (str);
    g_key_file_free (kf);
    g_free (user_file);
}

/* Filter function used by tree models to display widgets in correct places */

static gboolean filter_widgets (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    int index;

    gtk_tree_model_get (model, iter, 2, &index, -1);

    if ((long) data > 0 && index > 0) return TRUE;
    if ((long) data < 0 && index < 0) return TRUE;
    if ((long) data == 0 && index == 0) return TRUE;

    return FALSE;
}

/* Handler for cursor-changed signal to remove highlights in other tree views */

static void unselect (GtkTreeView *, gpointer data)
{
    int count;

    for (count = 0; count < 3; count++)
    {
        if ((long) data + count == 1) continue;

        g_signal_handler_block (tv[count], hand[count]);
        gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[count])));
        g_signal_handler_unblock (tv[count], hand[count]);
    }

    update_buttons ();
}

/*----------------------------------------------------------------------------*/
/* Public API */
/*----------------------------------------------------------------------------*/

void open_config_dialog (void)
{
    GtkBuilder *builder;
    GtkWidget *dlg;
    GtkCellRenderer *trend = gtk_cell_renderer_text_new ();
    int i;

    textdomain (GETTEXT_PACKAGE);

    // create the list store for widgets
    widgets = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);

    // build the dialog
    builder = gtk_builder_new_from_file (PACKAGE_DATA_DIR "/ui/config.ui");
    dlg = (GtkWidget *) gtk_builder_get_object (builder, "config_dlg");
    tv[0] = (GtkWidget *) gtk_builder_get_object (builder, "left_tv");
    tv[1] = (GtkWidget *) gtk_builder_get_object (builder, "cent_tv");
    tv[2] = (GtkWidget *) gtk_builder_get_object (builder, "right_tv");
    ladd = (GtkWidget *) gtk_builder_get_object (builder, "add_l_btn");
    radd = (GtkWidget *) gtk_builder_get_object (builder, "add_r_btn");
    rem = (GtkWidget *) gtk_builder_get_object (builder, "rem_btn");
    wup = (GtkWidget *) gtk_builder_get_object (builder, "up_btn");
    wdn = (GtkWidget *) gtk_builder_get_object (builder, "dn_btn");
    sup = (GtkWidget *) gtk_builder_get_object (builder, "spacei_btn");
    sdn = (GtkWidget *) gtk_builder_get_object (builder, "spaced_btn");

    // read in the current configuration
    read_config ();

    // set up filtering and sorting for the tree views
    for (i = 0; i < 3; i++)
    {
        filt[i] = gtk_tree_model_filter_new (GTK_TREE_MODEL (widgets), NULL);
        sort[i] = gtk_tree_model_sort_new_with_model (filt[i]);
    }

    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filt[0]), (GtkTreeModelFilterVisibleFunc) filter_widgets, (void *) 1, NULL);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sort[0]), 2, GTK_SORT_ASCENDING);

    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filt[1]), (GtkTreeModelFilterVisibleFunc) filter_widgets, (void *) 0, NULL);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sort[1]), 0, GTK_SORT_ASCENDING);

    gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filt[2]), (GtkTreeModelFilterVisibleFunc) filter_widgets, (void *) -1, NULL);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sort[2]), 2, GTK_SORT_DESCENDING);

    for (i = 0; i < 3; i++)
    {
        gtk_tree_view_set_model (GTK_TREE_VIEW (tv[i]), sort[i]);
        hand[i] = g_signal_connect (tv[i], "cursor-changed", G_CALLBACK (unselect), (void *)((long)(1 - i)));
    }

    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[0]), -1, _("Left"), trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[1]), -1, _("Available"), trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[2]), -1, _("Right"), trend, "text", 0, NULL);

    // connect buttton handlers
    g_signal_connect (ladd, "clicked", G_CALLBACK (add_widget), (void *) 1);
    g_signal_connect (radd, "clicked", G_CALLBACK (add_widget), (void *) -1);

    g_signal_connect (rem, "clicked", G_CALLBACK (remove_widget), NULL);

    g_signal_connect (wup, "clicked", G_CALLBACK (move_widget), (void *) 1);
    g_signal_connect (wdn, "clicked", G_CALLBACK (move_widget), (void *) -1);

    g_signal_connect (sup, "clicked", G_CALLBACK (change_space), (void *) 1);
    g_signal_connect (sdn, "clicked", G_CALLBACK (change_space), (void *) -1);

    gtk_window_set_default_size (GTK_WINDOW (dlg), 500, 300);

    // run the dialog
    if (gtk_dialog_run (GTK_DIALOG (dlg)) == GTK_RESPONSE_OK)
    {
        write_config ();
    }
    gtk_widget_destroy (dlg);
}

/* End of file */
/*----------------------------------------------------------------------------*/
