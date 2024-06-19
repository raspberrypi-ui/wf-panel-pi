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
#include <locale.h>
#include <dlfcn.h>
#include <dirent.h>
#include "configure.h"
#include "conf-utils.h"

/*----------------------------------------------------------------------------*/
/* Macros and typedefs */
/*----------------------------------------------------------------------------*/

#define COL_NAME    0
#define COL_ID      1
#define COL_INDEX   2

/*----------------------------------------------------------------------------*/
/* Global data */
/*----------------------------------------------------------------------------*/

static GtkListStore *widgets;
static GtkTreeModel *filt[3], *sort[3];
static GtkWidget *dlg;
static GtkWidget *tv[3];
static GtkWidget *ladd, *radd, *rem, *wup, *wdn, *cpl;
static int hand[3];
static gboolean found;

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

static void display_name (const char *type, char **name)
{
    char *libname;
    void *wid_lib;
    int space;
    char * (*func_display_name)(void);

    if (sscanf (type, "spacing%d", &space) == 1)
    {
        if (!space) *name = g_strdup_printf (_("Spacer"));
        else *name = g_strdup_printf (_("Spacer (%d)"), space);
        return;
    }

    libname = g_strdup_printf (PLUGIN_PATH "lib%s.so", type);
    wid_lib = dlopen (libname, RTLD_LAZY);
    g_free (libname);

    if (wid_lib)
    {
        func_display_name = (char * (*) (void)) dlsym (wid_lib, "display_name");
        if (!dlerror ())
            *name = g_strdup_printf (_(func_display_name()));
        else
            *name = g_strdup_printf (_("<Unknown>"));
        dlclose (wid_lib);
    }
    else *name = g_strdup_printf (_("<Unknown>"));
}

/* Helper function to determine whether a particular widget has a config table*/

gboolean can_configure (const char *type)
{
    char *libname;
    void *wid_lib;
    gboolean can_conf = FALSE;
    conf_table_t * (*func_config_params)(void);
    const conf_table_t *cptr;

    libname = g_strdup_printf (PLUGIN_PATH "lib%s.so", type);
    wid_lib = dlopen (libname, RTLD_LAZY);
    g_free (libname);

    if (wid_lib)
    {
        func_config_params = (conf_table_t * (*) (void)) dlsym (wid_lib, "config_params");
        if (!dlerror ())
        {
            cptr = func_config_params ();
            if (cptr->type != CONF_NONE) can_conf = TRUE;
        }
        dlclose (wid_lib);
    }
    return can_conf;
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
    GtkTreePath *path;
    GtkTreeModel *mod;
    GtkTreeIter iter;
    int nitems, lorr = selection ();
    char *type = NULL;
    gboolean conf;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[1 - lorr]));
    if (lorr == 0)
    {
        gtk_widget_set_sensitive (ladd, gtk_tree_selection_get_selected (sel, NULL, NULL));
        gtk_widget_set_sensitive (radd, gtk_tree_selection_get_selected (sel, NULL, NULL));
        gtk_widget_set_sensitive (rem, FALSE);
        gtk_widget_set_sensitive (wup, FALSE);
        gtk_widget_set_sensitive (wdn, FALSE);
        gtk_widget_set_sensitive (cpl, FALSE);
    }
    else
    {
        gtk_widget_set_sensitive (ladd, FALSE);
        gtk_widget_set_sensitive (radd, FALSE);

        nitems = gtk_tree_model_iter_n_children (filt[1 - lorr], NULL);

        gtk_widget_set_sensitive (rem, nitems > 0);
        path = gtk_tree_path_new_from_indices (0, -1);
        gtk_widget_set_sensitive (wup, nitems > 0 && !gtk_tree_selection_path_is_selected (sel, path));
        path = gtk_tree_path_new_from_indices (nitems ? nitems - 1 : nitems, -1);
        gtk_widget_set_sensitive (wdn, nitems > 0 && !gtk_tree_selection_path_is_selected (sel, path));

        if (gtk_tree_selection_get_selected (sel, &mod, &iter))
        {
            gtk_tree_model_get (mod, &iter, 1, &type, -1);

            // scroll the tree view to show the highlighted item
            path = gtk_tree_model_get_path (mod, &iter);
            gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (tv[1 - lorr]), path, NULL, FALSE, 0.0, 0.0);

            // can this type be configured?
            conf = FALSE;
            if (!strncmp (type, "spacing", 7)) conf = TRUE;
            else conf = can_configure (type);
            gtk_widget_set_sensitive (cpl, conf);
        }
        if (type) g_free (type);
    }
}

/* Add the currently-highlighted widget to the left or right side, depending on the value of data */

static void add_widget (GtkButton *, gpointer data)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod;
    GtkTreeIter iter, siter, citer;
    GtkTreePath *path;
    int index, lorr = (long) data == 1 ? 1 : -1;;
    char *type, *name;

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
            gtk_list_store_set (widgets, &citer, COL_INDEX, lorr * (index + 1), -1);
        else
        {
            display_name ("spacing4", &name);
            gtk_list_store_insert_with_values (widgets, NULL, -1,
                COL_NAME, name,
                COL_ID, "spacing4",
                COL_INDEX, lorr * (index + 1),
                -1);
            g_free (name);
        }
        g_free (type);

        // select the added item
        gtk_tree_selection_unselect_all (sel);
        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[1 - lorr]));
        path = gtk_tree_path_new_from_indices (index, -1);
        gtk_tree_selection_select_path (sel, path);

        update_buttons ();
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
            gtk_list_store_set (widgets, &citer, COL_INDEX, 0, -1);
        else
            gtk_list_store_remove (widgets, &citer);
        g_free (type);

        update_buttons ();
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
        gtk_list_store_set (widgets, &citer, COL_INDEX, index - 1, -1);
    }
    if (index < 0 && index < ((long) data))
    {
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (mod), &citer, iter);
        gtk_list_store_set (widgets, &citer, COL_INDEX, index + 1, -1);
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
            gtk_list_store_set (widgets, &citer, COL_INDEX, index - 1, -1);
        }
        else
        {
            gtk_tree_model_foreach (filt[1 - lorr], down, (void *)((long) index));
            gtk_list_store_set (widgets, &citer, COL_INDEX, index + 1, -1);
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
        gtk_list_store_set (widgets, &citer, COL_INDEX, index + 1, -1);
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
        gtk_list_store_set (widgets, &citer, COL_INDEX, index - 1, -1);
        return TRUE;
    }
    return FALSE;
}

/* Launch customise dialog for plugin-specific options */

void plugin_config_dialog (const char *type)
{
    char *strval, *key, *user_file, *name;
    const conf_table_t *cptr;
    GtkWidget *cdlg, *box, *hbox, *label, *control;
    GdkRGBA col;
    GKeyFile *kf;
    GList *children, *elem;
    gsize len;
    conf_table_t *(*func_config_params) (void);
    void *wid_lib;

    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    display_name (type, &name);
    strval = g_strdup_printf (_("Configure %s"), name);
    g_free (name);

    cdlg = gtk_dialog_new_with_buttons (strval, GTK_WINDOW (dlg), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        _("Cancel"), GTK_RESPONSE_CANCEL, _("OK"), GTK_RESPONSE_OK, NULL);
    g_free (strval);
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top (box, 10);
    gtk_widget_set_margin_bottom (box, 10);
    gtk_widget_set_margin_start (box, 10);
    gtk_widget_set_margin_end (box, 10);
    gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (cdlg))), box);

    /* load the config table from the shared library */
    name = g_strdup_printf (PLUGIN_PATH "lib%s.so", type);
    wid_lib = dlopen (name, RTLD_LAZY);
    g_free (name);

    if (wid_lib)
    {
        func_config_params = (conf_table_t * (*) (void)) dlsym (wid_lib, "config_params");
        if (!dlerror ())
        {
            cptr = func_config_params ();
            while (cptr->type != CONF_NONE)
            {
                hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
                strval = g_strdup_printf ("%s:", _(cptr->label));
                label = gtk_label_new (strval);
                g_free (strval);
                gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
                key = g_strdup_printf ("%s_%s", type, cptr->name);
                switch (cptr->type)
                {
                    case CONF_BOOL :    control = gtk_switch_new ();
                                        gtk_switch_set_active (GTK_SWITCH (control), get_config_bool (key));
                                        break;

                    case CONF_INT :     control = gtk_spin_button_new_with_range (0, 1000, 1); //!!!!!
                                        gtk_spin_button_set_value (GTK_SPIN_BUTTON (control), get_config_int (key));
                                        break;

                    case CONF_STRING :  control = gtk_entry_new ();
                                        get_config_string (key, &strval);
                                        gtk_entry_set_text (GTK_ENTRY (control), strval);
                                        g_free (strval);
                                        break;

                    case CONF_COLOUR :  control = gtk_color_button_new ();
                                        get_config_string (key, &strval);
                                        gdk_rgba_parse (&col, strval);
                                        g_free (strval);
                                        gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (control), &col);
                                        break;

                    case CONF_NONE :    break;
                }
                gtk_widget_set_name (control, key);
                gtk_box_pack_end (GTK_BOX (hbox), control, FALSE, FALSE, 0);
                gtk_container_add (GTK_CONTAINER (box), hbox);
                g_free (key);
                cptr++;
            }
        }
        dlclose (wid_lib);
    }

    gtk_widget_show_all (cdlg);

    if (gtk_dialog_run (GTK_DIALOG (cdlg)) == GTK_RESPONSE_OK)
    {
        user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi.ini", NULL);
        kf = g_key_file_new ();
        g_key_file_load_from_file (kf, user_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

        children = gtk_container_get_children (GTK_CONTAINER (box));
        elem = children;
        while (elem)
        {
            hbox = GTK_WIDGET (elem->data);
            control = GTK_WIDGET (gtk_container_get_children (GTK_CONTAINER (hbox))->next->data);

            if (GTK_IS_SWITCH (control))
                g_key_file_set_boolean (kf, "panel", gtk_widget_get_name (control), gtk_switch_get_active (GTK_SWITCH (control)));
            else if (GTK_IS_SPIN_BUTTON (control))
                g_key_file_set_integer (kf, "panel", gtk_widget_get_name (control), gtk_spin_button_get_value (GTK_SPIN_BUTTON (control)));
            else if (GTK_IS_ENTRY (control))
                g_key_file_set_string (kf, "panel", gtk_widget_get_name (control), gtk_entry_get_text (GTK_ENTRY (control)));
            else if (GTK_IS_COLOR_BUTTON (control))
            {
                gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (control), &col);
                strval = gdk_rgba_to_string (&col);
                g_key_file_set_string (kf, "panel", gtk_widget_get_name (control), strval);
                g_free (strval);
            }

            elem = elem->next;
        }

        strval = g_key_file_to_data (kf, &len, NULL);
        g_file_set_contents (user_file, strval, len, NULL);

        g_free (strval);
        g_key_file_free (kf);
        g_free (user_file);
    }

    gtk_widget_destroy (cdlg);
}

static int space_config_dialog (int space)
{
    char *strval, *name;
    GtkWidget *cdlg, *box, *hbox, *label, *control;

    display_name ("spacing0", &name);
    strval = g_strdup_printf (_("Configure %s"), name);
    g_free (name);

    cdlg = gtk_dialog_new_with_buttons (strval, GTK_WINDOW (dlg), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        _("Cancel"), GTK_RESPONSE_CANCEL, _("OK"), GTK_RESPONSE_OK, NULL);
    g_free (strval);
    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top (box, 10);
    gtk_widget_set_margin_bottom (box, 10);
    gtk_widget_set_margin_start (box, 10);
    gtk_widget_set_margin_end (box, 10);
    gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (cdlg))), box);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
    label = gtk_label_new (_("Width in pixels:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    control = gtk_spin_button_new_with_range (1, 100, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (control), space);
    gtk_box_pack_end (GTK_BOX (hbox), control, FALSE, FALSE, 0);

    gtk_container_add (GTK_CONTAINER (box), hbox);
    gtk_widget_show_all (cdlg);

    if (gtk_dialog_run (GTK_DIALOG (cdlg)) == GTK_RESPONSE_OK)
    {
        space = gtk_spin_button_get_value (GTK_SPIN_BUTTON (control));
    }

    gtk_widget_destroy (cdlg);
    return space;
}

static void configure_plugin (GtkButton *, gpointer)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod;
    GtkTreeIter iter, siter, citer;
    int index, lorr = selection ();
    char *type, *name;

    if (lorr)
    {
        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[1 - lorr]));
        if (gtk_tree_selection_get_selected (sel, &mod, &iter))
        {
            gtk_tree_model_get (mod, &iter, 1, &type, -1);
            if (strncmp (type, "spacing", 7)) plugin_config_dialog (type);
            else
            {
                gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (mod), &siter, &iter);
                gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (filt[1 - lorr]), &citer, &siter);

                // read the current spacing
                sscanf (type, "spacing%d", &index);
                g_free (type);

                index = space_config_dialog (index);

                // update both the widget type and the displayed name
                type = g_strdup_printf ("spacing%d", index);
                display_name (type, &name);
                gtk_list_store_set (widgets, &citer,
                    COL_NAME, name,
                    COL_ID, type,
                    -1);
                g_free (name);
            }
            g_free (type);
        }
    }
}

/* Read in config from local configuration file, or use default */

static void read_config (void)
{
    char *strval, *token, *name;
    int pos;
    struct dirent *dir;
    DIR *plugind;

    // add each space-separated widget from the metadata variables to the list store
    get_config_string ("widgets_left", &strval);
    pos = 1;
    token = strtok (strval, " ");
    while (token)
    {
        display_name (token, &name);
        gtk_list_store_insert_with_values (widgets, NULL, -1,
            COL_NAME, name,
            COL_ID, token,
            COL_INDEX, pos++,
            -1);
        g_free (name);
        token = strtok (NULL, " ");
    }
    g_free (strval);

    get_config_string ("widgets_right", &strval);
    pos = -1;
    token = strtok (strval, " ");
    while (token)
    {
        display_name (token, &name);
        gtk_list_store_insert_with_values (widgets, NULL, -1,
            COL_NAME, name,
            COL_ID, token,
            COL_INDEX, pos--,
            -1);
        g_free (name);
        token = strtok (NULL, " ");
    }
    g_free (strval);

    // add any unused widgets to the list store so they can be added by the user
    plugind = opendir (PLUGIN_PATH);
    if (plugind)
    {
        while ((dir = readdir (plugind)) != NULL)
        {
            if (strncmp (dir->d_name, "lib", 3) || strncmp (dir->d_name + strlen (dir->d_name) - 3, ".so", 3)) continue;
            if (!strcmp (dir->d_name, "libnotify.so")) continue;
            token = g_strdup_printf (dir->d_name + 3);
            *(token + strlen (token) - 3) = 0;

            found = FALSE;
            gtk_tree_model_foreach (GTK_TREE_MODEL (widgets), add_unused, (void *) token);
            if (!found)
            {
                display_name (token, &name);
                gtk_list_store_insert_with_values (widgets, NULL, -1,
                    COL_NAME, name,
                    COL_ID, token,
                    COL_INDEX, 0,
                    -1);
                g_free (name);
            }
            g_free (token);
        }
        closedir (plugind);
    }

    // always add spacing
    display_name ("spacing0", &name);
    gtk_list_store_insert_with_values (widgets, NULL, -1,
        COL_NAME, name,
        COL_ID, "spacing0",
        COL_INDEX, 0,
        -1);
    g_free (name);
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
    cpl = (GtkWidget *) gtk_builder_get_object (builder, "conf_btn");

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

    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[0]), -1, _("Left Side"), trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[1]), -1, _("Available"), trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[2]), -1, _("Right Side"), trend, "text", 0, NULL);

    // connect buttton handlers
    g_signal_connect (ladd, "clicked", G_CALLBACK (add_widget), (void *) 1);
    g_signal_connect (radd, "clicked", G_CALLBACK (add_widget), (void *) -1);

    g_signal_connect (rem, "clicked", G_CALLBACK (remove_widget), NULL);

    g_signal_connect (wup, "clicked", G_CALLBACK (move_widget), (void *) 1);
    g_signal_connect (wdn, "clicked", G_CALLBACK (move_widget), (void *) -1);

    g_signal_connect (cpl, "clicked", G_CALLBACK (configure_plugin), NULL);

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
