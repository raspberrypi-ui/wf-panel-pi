/*============================================================================
Copyright (c) 2023 Raspberry Pi
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
============================================================================*/

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <locale.h>
#include <dlfcn.h>

#include "plugin.h"
#include "plug_conf.h"

/*----------------------------------------------------------------------------*/
/* Macros and typedefs */
/*----------------------------------------------------------------------------*/

#define COL_NAME    0
#define COL_ID      1
#define COL_INDEX   2
#define COL_CONFIG  3

#define AVAIL 0
#define PAN_L 1
#define PAN_R 2
#define DOCK  3
#define DOCKT 4

/*----------------------------------------------------------------------------*/
/* Global data */
/*----------------------------------------------------------------------------*/

static GtkListStore *widgets;
static GtkTreeModel *filt[5], *sort[5];
static GtkWidget *tv[5];
static GtkWidget *ladd, *radd, *dadd, *tadd, *rem, *wup, *wdn, *cpl, *ok, *nb;
static int hand[5];
static gboolean found;
static GtkTreeIter sp_iter;
static GtkGesture *gesture[5];
static gboolean pressed;
static double press_x, press_y;

/*----------------------------------------------------------------------------*/
/* Function prototypes */
/*----------------------------------------------------------------------------*/

static void read_config (void);
static void read_one_config (int index, const char *section, const char *item);
static gboolean read_lib (const char *type, char **name, gboolean *config);
static gboolean add_unused (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data);
static void write_config (void);
static void write_one_config (GKeyFile *kf, int index, const char *section, const char *item);
static int selection (void);
static void add_widget (GtkButton *, gpointer data);
static void remove_widget (GtkButton *, gpointer);
static gboolean renumber (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data);
static void move_widget (GtkButton *, gpointer data);
static gboolean up (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data);
static gboolean down (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data);
static void update_plugin_spacing (GtkWidget *box);
static void configure_plugin (GtkButton *, gpointer);
static void plugin_closed (GtkButton *, gpointer);
static void update_buttons (void);
static gboolean filter_widgets (GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
static void unselect (GtkTreeView *, gpointer data);
static void close_window (GtkButton *, gpointer);
static GtkWidget *avail_menu (int ref, gdouble x, gdouble y);
static GtkWidget *panel_menu (int ref, gdouble x, gdouble y);
static gboolean popup_button (GtkWidget *, GdkEventButton event, int ref);
static void gesture_pressed (GtkGestureLongPress *, gdouble x, gdouble y, gpointer);
static void gesture_end (GtkGestureLongPress *, GdkEventSequence *, int ref);

/*----------------------------------------------------------------------------*/
/* Private functions */
/*----------------------------------------------------------------------------*/

/* Read in current configuration */

static void read_config (void)
{
    char *token, *name;
    struct dirent *dir;
    DIR *plugind;
    gboolean config;

    // add each space-separated widget from the metadata variables to the list store
    read_one_config (PAN_L, "panel", "widgets_left");
    read_one_config (PAN_R, "panel", "widgets_right");
    read_one_config (DOCK, "dock", "widgets_left");
    read_one_config (DOCKT, "dock", "widgets_right");

    // add any unused widgets to the list store so they can be added by the user
    plugind = opendir (PLUGIN_PATH);
    if (plugind)
    {
        while ((dir = readdir (plugind)) != NULL)
        {
            if (strncmp (dir->d_name, "lib", 3) || strncmp (dir->d_name + strlen (dir->d_name) - 3, ".so", 3)) continue;
            if (!strcmp (dir->d_name, "libnotify.so")) continue;
            token = g_strdup (dir->d_name + 3);
            *(token + strlen (token) - 3) = 0;

            found = FALSE;
            gtk_tree_model_foreach (GTK_TREE_MODEL (widgets), add_unused, (void *) token);
            if (!found)
            {
                read_lib (token, &name, &config);
                gtk_list_store_insert_with_values (widgets, NULL, -1,
                    COL_NAME, name,
                    COL_ID, token,
                    COL_INDEX, 0,
                    COL_CONFIG, config,
                    -1);
                g_free (name);
            }
            g_free (token);
        }
        closedir (plugind);
    }
}

/* Read in config from local configuration file, or use default */

static void read_one_config (int index, const char *section, const char *item)
{
    char *strval, *token, *name;
    int pos;
    gboolean config;

    get_config_string (section, item, &strval);
    pos = index * 100;
    token = strtok (strval, " ");
    while (token)
    {
        if (read_lib (token, &name, &config))
            gtk_list_store_insert_with_values (widgets, NULL, -1,
                COL_NAME, name,
                COL_ID, token,
                COL_INDEX, pos++,
                COL_CONFIG, config,
                -1);
        g_free (name);
        token = strtok (NULL, " ");
    }
    g_free (strval);
}

/* Helper function to read the name and configurability of a library */

static gboolean read_lib (const char *type, char **name, gboolean *config)
{
    char *libname, *package;
    void *wid_lib;
    int space;
    gboolean res = FALSE;
    char * (*func_package_name)(void);
    char * (*func_display_name)(void);
    conf_table_t * (*func_config_params)(void);
    const conf_table_t *cptr;

    *config = FALSE;
    if (sscanf (type, "spacing%d", &space) == 1)
    {
        if (space > 0)
        {
            *name = g_strdup_printf (_("Spacer (%d)"), space);
            *config = TRUE;
        }
        else
        {
            *name = g_strdup (_("Separator"));
            *config = FALSE;
        }
        return TRUE;
    }

    libname = g_strdup_printf (PLUGIN_PATH "lib%s.so", type);
    wid_lib = dlopen (libname, RTLD_LAZY);
    g_free (libname);

    if (wid_lib)
    {
        func_package_name = (char * (*) (void)) dlsym (wid_lib, "package_name");
        if (!dlerror ()) package = g_strdup (func_package_name());
        else package = NULL;

        func_display_name = (char * (*) (void)) dlsym (wid_lib, "display_name");
        if (!dlerror ())
        {
            *name = g_strdup (dgettext (package, func_display_name ()));
            res = TRUE;
        }
        else *name = g_strdup_printf (_("<Unknown>"));
        if (package) g_free (package);

        func_config_params = (conf_table_t * (*) (void)) dlsym (wid_lib, "config_params");
        if (!dlerror ())
        {
            cptr = func_config_params ();
            if (cptr->type != CONF_TYPE_NONE) *config = TRUE;
        }

        /*
         * Sigh. Due to the way libnm uses an __attribute__(constructor) function
         * to register DBus errors, this is called every time the netman plugin is
         * dlopen'ed, but if it is called more than once, it segfaults. The gating
         * variable designed to prevent it from being reopened is cleared if you
         * dlclose it once opened, so on the next dlopen it crashes. The only fix
         * short of changing the way libnm is initialised is to never dlclose that
         * particular plugin once it has been opened. This makes me a sad panda...
         */
        if (strcmp (type, "netman")) dlclose (wid_lib);
    }
    else *name = g_strdup_printf (_("<Unknown>"));

    return res;
}

/* Function to check if a widget is currently installed in panel or dock */

static gboolean add_unused (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data)
{
    char *type;
    gtk_tree_model_get (mod, iter, COL_ID, &type, -1);
    if (!g_strcmp0 (data, type)) found = TRUE;
    g_free (type);
    return found;
}

/* Write out current configuration */

static void write_config (void)
{
    char *str;
    gsize len;

    // construct the file path
    char *user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi", "wf-panel-pi.ini", NULL);

    // read in data from file to a key file
    GKeyFile *kf = g_key_file_new ();
    g_key_file_load_from_file (kf, user_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

    // iterate through the tree models
    write_one_config (kf, PAN_L, "panel", "widgets_left");
    write_one_config (kf, PAN_R, "panel", "widgets_right");
    write_one_config (kf, DOCK, "dock", "widgets_left");
    write_one_config (kf, DOCKT, "dock", "widgets_right");

    // write the modified key file out
    str = g_key_file_to_data (kf, &len, NULL);
    g_file_set_contents (user_file, str, len, NULL);

    g_free (str);
    g_key_file_free (kf);
    g_free (user_file);
}

/* Write config to local configuration file */

static void write_one_config (GKeyFile *kf, int index, const char *section, const char *item)
{
    GtkTreeIter iter;
    char *str;
    char config[1000];

    // concatenate widget names from model to a space-separated string
    config[0] = 0;
    if (gtk_tree_model_get_iter_first (sort[index], &iter))
    {
        do
        {
            gtk_tree_model_get (sort[index], &iter, COL_ID, &str, -1);
            strcat (config, str);
            strcat (config, " ");
            g_free (str);
        }
        while (gtk_tree_model_iter_next (sort[index], &iter));
    }
    g_key_file_set_string (kf, section, item, config);
}

/* Helper function to locate the currently-highlighted widget */

static int selection (void)
{
    GtkTreeSelection *sel;
    int i;

    for (i = 0; i < 5; i++)
    {
        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[i]));
        if (gtk_tree_selection_get_selected (sel, &sort[i], NULL)) return i;
    }

    return -1;
}

/* Add the currently-highlighted widget to the left or right side, depending on the value of data */

static void add_widget (GtkButton *, gpointer data)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod;
    GtkTreeIter iter, siter, citer;
    GtkTreePath *path;
    int index, lorr = (long) data;
    char *type, *name;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[AVAIL]));

    if (gtk_tree_selection_get_selected (sel, &mod, &iter))
    {
        gtk_tree_model_get (mod, &iter, COL_ID, &type, -1);
        gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (mod), &siter, &iter);
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (filt[AVAIL]), &citer, &siter);

        // just add to the bottom of the list
        index = gtk_tree_model_iter_n_children (filt[lorr], NULL);

        // change index for anything other than a space; space needs to be created
        if (!strcmp (type, "separator"))
        {
            name = g_strdup (_("Separator"));
            gtk_list_store_insert_with_values (widgets, NULL, -1,
                COL_NAME, name,
                COL_ID, "spacing0",
                COL_INDEX, lorr * 100 + index,
                COL_CONFIG, FALSE,
                -1);
            g_free (name);
        }
        else if (!strncmp (type, "spacing", 7))
        {
            name = g_strdup_printf (_("Spacer (%d)"), 4);
            gtk_list_store_insert_with_values (widgets, NULL, -1,
                COL_NAME, name,
                COL_ID, "spacing4",
                COL_INDEX, lorr * 100 + index,
                COL_CONFIG, TRUE,
                -1);
            g_free (name);
        }
        else
            gtk_list_store_set (widgets, &citer, COL_INDEX, lorr * 100 + index, -1);

        g_free (type);

        // select the added item
        gtk_tree_selection_unselect_all (sel);
        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[lorr]));
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

    if (lorr == -1) return;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[lorr]));
    if (gtk_tree_selection_get_selected (sel, &mod, &iter))
    {
        gtk_tree_model_get (mod, &iter, COL_ID, &type, COL_INDEX, &index, -1);
        gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (mod), &siter, &iter);
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (filt[lorr]), &citer, &siter);

        // change index for anything other than a space; space needs to be deleted
        if (strncmp (type, "spacing", 7))
            gtk_list_store_set (widgets, &citer, COL_INDEX, 0, -1);
        else
            gtk_list_store_remove (widgets, &citer);
        g_free (type);

        update_buttons ();
    }

    // re-number the widgets in the list below the one removed
    gtk_tree_model_foreach (filt[lorr], renumber, (void *)((long) index));
}

static gboolean renumber (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data)
{
    // if index > data, subtract 1 from index
    GtkTreeIter citer;
    int index;

    gtk_tree_model_get (mod, iter, COL_INDEX, &index, -1);
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

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[lorr]));
    if (gtk_tree_selection_get_selected (sel, &mod, &iter))
    {
        gtk_tree_model_get (mod, &iter, COL_INDEX, &index, -1);
        gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (mod), &siter, &iter);
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (filt[lorr]), &citer, &siter);

        // check not trying to move past end of list
        if (dir == 1)
        {
            if (index == lorr) return;
        }
        else
        {
            if (index == lorr * gtk_tree_model_iter_n_children (filt[lorr], NULL)) return;
        }

        // to move, swap the index of the widget moved with that of the adjacent widget
        if (dir * lorr > 0)
        {
            gtk_tree_model_foreach (filt[lorr], up, (void *)((long) index));
            gtk_list_store_set (widgets, &citer, COL_INDEX, index - 1, -1);
        }
        else
        {
            gtk_tree_model_foreach (filt[lorr], down, (void *)((long) index));
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
    gtk_tree_model_get (mod, iter, COL_INDEX, &index, -1);
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
    gtk_tree_model_get (mod, iter, COL_INDEX, &index, -1);
    if (index == ((long) data) + 1)
    {
        gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (mod), &citer, iter);
        gtk_list_store_set (widgets, &citer, COL_INDEX, index - 1, -1);
        return TRUE;
    }
    return FALSE;
}

/* Adjust the spacing width for a spacer plugin */

static void update_plugin_spacing (GtkWidget *box)
{
    GtkWidget *hbox, *control;
    GList *children, *elem, *bchildren;
    int val;
    char *type, *name;

    children = gtk_container_get_children (GTK_CONTAINER (box));
    elem = children;
    while (elem)
    {
        hbox = GTK_WIDGET (elem->data);
        bchildren = gtk_container_get_children (GTK_CONTAINER (hbox));
        if (bchildren->next)
        {
            control = GTK_WIDGET (bchildren->next->data);
            if (!g_strcmp0 (gtk_widget_get_name (control), "spacing_width"))
            {
                val = gtk_spin_button_get_value (GTK_SPIN_BUTTON (control));
                if (val)
                {
                    // update both the widget type and the displayed name
                    type = g_strdup_printf ("spacing%d", val);
                    name = g_strdup_printf (_("Spacer (%d)"), val);
                    gtk_list_store_set (widgets, &sp_iter,
                        COL_NAME, name,
                        COL_ID, type,
                        -1);
                    g_free (type);
                    g_free (name);
                }
            }
        }
        g_list_free (bchildren);
        elem = elem->next;
    }
    g_list_free (children);
}

/* Launch the plugin configuration dialog */

static void configure_plugin (GtkButton *, gpointer)
{
    GtkTreeSelection *sel;
    GtkTreeModel *mod;
    GtkTreeIter iter, siter;
    int lorr = selection ();
    char *type;

    if (lorr != -1 && lorr != AVAIL)
    {
        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[lorr]));
        if (gtk_tree_selection_get_selected (sel, &mod, &iter))
        {
            gtk_tree_model_get (mod, &iter, COL_ID, &type, -1);
            if (!strncmp (type, "spacing", 7))
            {
                // spacing is a special case - set the global iter
                gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (mod), &siter, &iter);
                gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (filt[lorr]), &sp_iter, &siter);
            }
            plugin_config_dialog (type);
            gtk_window_set_transient_for (GTK_WINDOW (cdlg), GTK_WINDOW (main_dlg));
            g_signal_connect (cdlg, "destroy", G_CALLBACK (plugin_closed), NULL);
            g_free (type);
        }
    }

    update_buttons ();
}

static void plugin_closed (GtkButton *, gpointer)
{
    update_buttons ();
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

    gtk_widget_set_sensitive (ladd, FALSE);
    gtk_widget_set_sensitive (radd, FALSE);
    gtk_widget_set_sensitive (dadd, FALSE);
    gtk_widget_set_sensitive (tadd, FALSE);
    gtk_widget_set_sensitive (rem, FALSE);
    gtk_widget_set_sensitive (wup, FALSE);
    gtk_widget_set_sensitive (wdn, FALSE);
    gtk_widget_set_sensitive (cpl, FALSE);

    gtk_widget_set_sensitive (ok, cdlg ? FALSE : TRUE);

    if (lorr == -1 || cdlg) return;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[lorr]));
    if (lorr == AVAIL)
    {
        gtk_widget_set_sensitive (ladd, gtk_tree_selection_get_selected (sel, NULL, NULL));
        gtk_widget_set_sensitive (radd, gtk_tree_selection_get_selected (sel, NULL, NULL));
        gtk_widget_set_sensitive (dadd, gtk_tree_selection_get_selected (sel, NULL, NULL));
        gtk_widget_set_sensitive (tadd, gtk_tree_selection_get_selected (sel, NULL, NULL));
        if (gtk_tree_selection_get_selected (sel, &mod, &iter))
        {
            gtk_tree_model_get (mod, &iter, COL_ID, &type, -1);
            if (!g_strcmp0 (type, "split"))
            {
                gtk_widget_set_sensitive (ladd, FALSE);
                gtk_widget_set_sensitive (radd, FALSE);
                gtk_widget_set_sensitive (dadd, FALSE);
            }
            g_free (type);
        }
    }
    else
    {
        nitems = gtk_tree_model_iter_n_children (filt[lorr], NULL);

        gtk_widget_set_sensitive (rem, nitems > 0);
        path = gtk_tree_path_new_from_indices (0, -1);
        gtk_widget_set_sensitive (wup, nitems > 0 && !gtk_tree_selection_path_is_selected (sel, path));
        path = gtk_tree_path_new_from_indices (nitems ? nitems - 1 : nitems, -1);
        gtk_widget_set_sensitive (wdn, nitems > 0 && !gtk_tree_selection_path_is_selected (sel, path));

        if (gtk_tree_selection_get_selected (sel, &mod, &iter))
        {
            gtk_tree_model_get (mod, &iter, COL_ID, &type, COL_CONFIG, &conf, -1);

            // scroll the tree view to show the highlighted item
            path = gtk_tree_model_get_path (mod, &iter);
            gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (tv[lorr]), path, NULL, FALSE, 0.0, 0.0);

            // can this type be configured?
            gtk_widget_set_sensitive (cpl, conf);
        }
        if (type) g_free (type);
    }
}

/* Filter function used by tree models to display widgets in correct places */

static gboolean filter_widgets (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    int index;

    gtk_tree_model_get (model, iter, COL_INDEX, &index, -1);

    if (index >= (long) data && index < (long) data + 100) return TRUE;

    return FALSE;
}

/* Handler for cursor-changed signal to remove highlights in other tree views */

static void unselect (GtkTreeView *, gpointer data)
{
    int count;

    for (count = 0; count < 5; count++)
    {
        if ((long) data == count) continue;

        g_signal_handler_block (tv[count], hand[count]);
        gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[count])));
        g_signal_handler_unblock (tv[count], hand[count]);
    }

    update_buttons ();
}

static void close_window (GtkButton *, gpointer)
{
    write_config ();
}

/* Popup menus */

static GtkWidget *avail_menu (int ref, gdouble x, gdouble y)
{
    GtkWidget *menu, *item;
    GtkTreePath *path;
    GtkTreeIter iter;
    char *type;
    gboolean split = FALSE;
    int page = gtk_notebook_get_current_page (GTK_NOTEBOOK (nb));

    menu = gtk_menu_new ();

    if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (tv[ref]), x, y, &path, NULL, NULL, NULL))
    {
        gtk_tree_model_get_iter (sort[ref], &iter, path);
        gtk_tree_path_free (path);
        gtk_tree_model_get (sort[ref], &iter, COL_ID, &type, -1);
        if (!g_strcmp0 (type, "split")) split = TRUE;
        g_free (type);
    }

    item = gtk_menu_item_new_with_label (page ? _("Add to Dock") : _("Add to Left"));
    g_signal_connect (item, "activate", G_CALLBACK (add_widget), page ? (void *) DOCK : (void *) PAN_L);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    if (split) gtk_widget_set_sensitive (item, FALSE);

    item = gtk_menu_item_new_with_label (page ? _("Add to Tray") : _("Add to Right"));
    g_signal_connect (item, "activate", G_CALLBACK (add_widget), page ? (void *) DOCKT : (void *) PAN_R);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    if (split && !page) gtk_widget_set_sensitive (item, FALSE);

    gtk_widget_show_all (menu);
    return menu;
}

static GtkWidget *panel_menu (int ref, gdouble x, gdouble y)
{
    GtkWidget *menu, *item;
    GtkTreePath *path;
    GtkTreeIter iter;
    int nitems, pos;
    gboolean conf = FALSE;

    menu = gtk_menu_new ();

    if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (tv[ref]), x, y, &path, NULL, NULL, NULL))
    {
        gtk_tree_model_get_iter (sort[ref], &iter, path);
        pos = *(gtk_tree_path_get_indices (path));
        gtk_tree_path_free (path);
        gtk_tree_model_get (sort[ref], &iter, COL_CONFIG, &conf, -1);
    }

    nitems = gtk_tree_model_iter_n_children (filt[ref], NULL);

    item = gtk_menu_item_new_with_label (_("Remove"));
    g_signal_connect (item, "activate", G_CALLBACK (remove_widget), NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

    item = gtk_menu_item_new_with_label (_("Move Left"));
    g_signal_connect (item, "activate", G_CALLBACK (move_widget), (void *) 1);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    if (pos == 0) gtk_widget_set_sensitive (item, FALSE);

    item = gtk_menu_item_new_with_label (_("Move Right"));
    g_signal_connect (item, "activate", G_CALLBACK (move_widget), (void *) -1);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    if (pos >= nitems - 1) gtk_widget_set_sensitive (item, FALSE);

    item = gtk_menu_item_new_with_label (_("Configure..."));
    g_signal_connect (item, "activate", G_CALLBACK (configure_plugin), NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    if (!conf) gtk_widget_set_sensitive (item, FALSE);

    gtk_widget_show_all (menu);

    return menu;
}

static gboolean popup_button (GtkWidget *, GdkEventButton event, int ref)
{
    GtkWidget *menu;
    if (event.type == GDK_BUTTON_PRESS && event.button == 3)
    {
        menu = ref == AVAIL ? avail_menu (ref, event.x, event.y) : panel_menu (ref, event.x, event.y);
        gtk_menu_popup_at_pointer (GTK_MENU (menu), gtk_get_current_event ());
        return FALSE;
    }
    return FALSE;
}

static void gesture_pressed (GtkGestureLongPress *, gdouble x, gdouble y, gpointer)
{
    pressed = TRUE;
    press_x = x;
    press_y = y;
}

static void gesture_end (GtkGestureLongPress *, GdkEventSequence *, int ref)
{
    GtkWidget *menu;
    int x, y;

    if (pressed)
    {
        gtk_tree_view_convert_widget_to_bin_window_coords (GTK_TREE_VIEW (tv[ref]), press_x, press_y, &x, &y);
        menu = ref == AVAIL ? avail_menu (ref, x, y) : panel_menu (ref, x, y);
        GdkRectangle rect = {press_x, press_y, 0, 0};
        gtk_menu_popup_at_rect (GTK_MENU (menu), gtk_widget_get_window (tv[ref]), &rect, GDK_GRAVITY_CENTER, GDK_GRAVITY_NORTH_WEST, NULL);
    }
    pressed = FALSE;
}

/*----------------------------------------------------------------------------*/
/* Public API */
/*----------------------------------------------------------------------------*/

void init_config (void)
{
    GtkCellRenderer *trend = gtk_cell_renderer_text_new ();
    int i;

    // create the list store for widgets
    widgets = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN);

    // build the dialog
    tv[AVAIL] = (GtkWidget *) gtk_builder_get_object (builder, "cent_tv");
    tv[PAN_L] = (GtkWidget *) gtk_builder_get_object (builder, "left_tv");
    tv[PAN_R] = (GtkWidget *) gtk_builder_get_object (builder, "right_tv");
    tv[DOCK] = (GtkWidget *) gtk_builder_get_object (builder, "dock_tv");
    tv[DOCKT] = (GtkWidget *) gtk_builder_get_object (builder, "dock_tt_tv");
    ladd = (GtkWidget *) gtk_builder_get_object (builder, "add_l_btn");
    radd = (GtkWidget *) gtk_builder_get_object (builder, "add_r_btn");
    dadd = (GtkWidget *) gtk_builder_get_object (builder, "add_d_btn");
    tadd = (GtkWidget *) gtk_builder_get_object (builder, "add_t_btn");
    rem = (GtkWidget *) gtk_builder_get_object (builder, "rem_btn");
    wup = (GtkWidget *) gtk_builder_get_object (builder, "up_btn");
    wdn = (GtkWidget *) gtk_builder_get_object (builder, "dn_btn");
    cpl = (GtkWidget *) gtk_builder_get_object (builder, "conf_btn");
    ok = (GtkWidget *) gtk_builder_get_object (builder, "ok_btn");
    nb = (GtkWidget *) gtk_builder_get_object (builder, "notebook1");

    // read in the current configuration
    read_config ();

    // set up filtering and sorting for the tree views
    for (i = 0; i < 5; i++)
    {
        filt[i] = gtk_tree_model_filter_new (GTK_TREE_MODEL (widgets), NULL);
        sort[i] = gtk_tree_model_sort_new_with_model (filt[i]);

        gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filt[i]), (GtkTreeModelFilterVisibleFunc) filter_widgets, (void *)((long) (i * 100)), NULL);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sort[i]), i == AVAIL ? COL_NAME : COL_INDEX, GTK_SORT_ASCENDING);

        gtk_tree_view_set_model (GTK_TREE_VIEW (tv[i]), sort[i]);
        hand[i] = g_signal_connect (tv[i], "cursor-changed", G_CALLBACK (unselect), (void *)((long) i));
    }

    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[AVAIL]), -1, _("Available"), trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[PAN_L]), -1, _("Left"), trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[PAN_R]), -1, _("Right"), trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[DOCK]), -1, _("Dock"), trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[DOCKT]), -1, _("Tray"), trend, "text", 0, NULL);

    // connect buttton handlers
    g_signal_connect (ladd, "clicked", G_CALLBACK (add_widget), (void *) PAN_L);
    g_signal_connect (radd, "clicked", G_CALLBACK (add_widget), (void *) PAN_R);
    g_signal_connect (dadd, "clicked", G_CALLBACK (add_widget), (void *) DOCK);
    g_signal_connect (tadd, "clicked", G_CALLBACK (add_widget), (void *) DOCKT);

    g_signal_connect (rem, "clicked", G_CALLBACK (remove_widget), NULL);

    g_signal_connect (wup, "clicked", G_CALLBACK (move_widget), (void *) 1);
    g_signal_connect (wdn, "clicked", G_CALLBACK (move_widget), (void *) -1);

    g_signal_connect (cpl, "clicked", G_CALLBACK (configure_plugin), NULL);

    g_signal_connect (ok, "clicked", G_CALLBACK (close_window), NULL);

    /* set up right-click and long press */
    for (i = 0; i < 5; i++)
    {
        g_signal_connect (tv[i], "button-press-event", G_CALLBACK (popup_button), (void *) (long) i);

        gesture[i] = gtk_gesture_long_press_new (tv[i]);
        gtk_gesture_single_set_touch_only (GTK_GESTURE_SINGLE (gesture[i]), FALSE);
        g_signal_connect (gesture[i], "pressed", G_CALLBACK (gesture_pressed), NULL);
        g_signal_connect (gesture[i], "end", G_CALLBACK (gesture_end), (void *) (long) i);
        gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (gesture[i]), GTK_PHASE_TARGET);
    }
    pressed = FALSE;

    update_buttons ();
}

void set_bar (void)
{
    gtk_notebook_set_current_page (GTK_NOTEBOOK (nb), 0);
}

void set_dock (void)
{
    gtk_notebook_set_current_page (GTK_NOTEBOOK (nb), 1);
}

void update_spacing (GtkButton *, gpointer data)
{
    update_plugin_spacing (GTK_WIDGET (data));
    gtk_widget_destroy (gtk_widget_get_parent (gtk_widget_get_parent (GTK_WIDGET (data))));
}

/* End of file */
/*----------------------------------------------------------------------------*/
