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
#include <dirent.h>
#include "configure.h"
#include "conf-utils.h"

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
#define DOCKB 5

/*----------------------------------------------------------------------------*/
/* Global data */
/*----------------------------------------------------------------------------*/

static GtkListStore *widgets;
static GtkTreeModel *filt[6], *sort[6];
static GtkWidget *dlg, *cdlg;
static GtkWidget *tv[6];
static GtkWidget *ladd, *radd, *dadd, *dttadd, *dtbadd, *rem, *wup, *wdn, *cpl;
static int hand[6];
static gboolean found;
static GtkTreeIter sp_iter;

/*----------------------------------------------------------------------------*/
/* Function prototypes */
/*----------------------------------------------------------------------------*/

static gboolean renumber (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data);
static gboolean up (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data);
static gboolean down (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data);
static void update_plugin_config (GtkWidget *box);
static void update_plugin_spacing (GtkWidget *box);
static gboolean add_unused (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data);
static void write_config (void);

/*----------------------------------------------------------------------------*/
/* Private functions */
/*----------------------------------------------------------------------------*/

/* Helper function to determine whether a particular widget has a config table*/

int can_configure (const char *type)
{
    char *libname;
    void *wid_lib;
    gboolean can_conf = FALSE;
    conf_table_t * (*func_config_params)(void);
    const conf_table_t *cptr;

    if (cdlg) return FALSE;

    libname = g_strdup_printf (PLUGIN_PATH "lib%s.so", type);
    wid_lib = dlopen (libname, RTLD_LAZY);
    g_free (libname);

    if (wid_lib)
    {
        func_config_params = (conf_table_t * (*) (void)) dlsym (wid_lib, "config_params");
        if (!dlerror ())
        {
            cptr = func_config_params ();
            if (cptr->type != CONF_TYPE_NONE) can_conf = TRUE;
        }
        dlclose (wid_lib);
    }
    return can_conf;
}

int can_add (void)
{
    if (dlg) return FALSE;
    return TRUE;
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

/* Helper function to locate the currently-highlighted widget */

static int selection (void)
{
    GtkTreeSelection *sel;
    int i;

    for (i = 0; i < 6; i++)
    {
        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[i]));
        if (gtk_tree_selection_get_selected (sel, &sort[i], NULL)) return i;
    }

    return -1;
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
    gtk_widget_set_sensitive (dttadd, FALSE);
    gtk_widget_set_sensitive (dtbadd, FALSE);
    gtk_widget_set_sensitive (rem, FALSE);
    gtk_widget_set_sensitive (wup, FALSE);
    gtk_widget_set_sensitive (wdn, FALSE);
    gtk_widget_set_sensitive (cpl, FALSE);

    if (lorr == -1 || cdlg) return;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[lorr]));
    if (lorr == AVAIL)
    {
        gtk_widget_set_sensitive (ladd, gtk_tree_selection_get_selected (sel, NULL, NULL));
        gtk_widget_set_sensitive (radd, gtk_tree_selection_get_selected (sel, NULL, NULL));
        gtk_widget_set_sensitive (dadd, gtk_tree_selection_get_selected (sel, NULL, NULL));
        gtk_widget_set_sensitive (dttadd, gtk_tree_selection_get_selected (sel, NULL, NULL));
        gtk_widget_set_sensitive (dtbadd, gtk_tree_selection_get_selected (sel, NULL, NULL));
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

/* Customise dialog for plugin-specific options */

static void update_config (GtkButton *, gpointer data)
{
    update_plugin_config (GTK_WIDGET (data));
    gtk_widget_destroy (gtk_widget_get_parent (gtk_widget_get_parent (GTK_WIDGET (data))));
}

static void update_spacing (GtkButton *, gpointer data)
{
    update_plugin_spacing (GTK_WIDGET (data));
    gtk_widget_destroy (gtk_widget_get_parent (gtk_widget_get_parent (GTK_WIDGET (data))));
}

static void close_dialog (GtkButton *, gpointer data)
{
    gtk_widget_destroy (gtk_widget_get_parent (gtk_widget_get_parent (GTK_WIDGET (data))));
}

static void plugin_closed (GtkButton *, gpointer)
{
    cdlg = NULL;
    if (dlg) update_buttons ();
}

void plugin_config_dialog (const char *type)
{
    GtkBuilder *builder;
    GtkWidget *box, *hbox, *label, *control;
    GdkRGBA col;
    char *strval, *key, *name, *package;
    const conf_table_t *cptr;
    int space = -1;
    conf_table_t *(*func_config_params) (void);
    char * (*func_package_name)(void);
    char * (*func_display_name)(void);
    void *wid_lib;

    if (!strncmp (type, "spacing", 7))
    {
        // read the current spacing
        sscanf (type, "spacing%d", &space);
        type = "spacing";
    }

    /* load the information from the shared library */
    name = g_strdup_printf (PLUGIN_PATH "lib%s.so", type);
    wid_lib = dlopen (name, RTLD_LAZY);
    g_free (name);

    if (!wid_lib) return;

    // build the dialog
    builder = gtk_builder_new_from_file (PACKAGE_DATA_DIR "/ui/config.ui");
    cdlg = (GtkWidget *) gtk_builder_get_object (builder, "plugin_dlg");
    box = (GtkWidget *) gtk_builder_get_object (builder, "box");

    func_package_name = (char * (*) (void)) dlsym (wid_lib, "package_name");
    if (!dlerror ()) package = g_strdup (func_package_name());
    else package = NULL;

    func_display_name = (char * (*) (void)) dlsym (wid_lib, "display_name");
    if (!dlerror ())
        strval = g_strdup_printf (_("Configure %s"), dgettext (package, func_display_name ()));
    else
        strval = g_strdup_printf (_("Configure %s"), _("<Unknown>"));
    gtk_window_set_title (GTK_WINDOW (cdlg), strval);
    g_free (strval);

    func_config_params = (conf_table_t * (*) (void)) dlsym (wid_lib, "config_params");
    if (!dlerror ())
    {
        cptr = func_config_params ();
        while (cptr->type != CONF_TYPE_NONE)
        {
            control = NULL;
            hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
            if (cptr->type == CONF_TYPE_LABEL)
                strval = g_strdup_printf ("%s", dgettext (package, cptr->label));
            else
                strval = g_strdup_printf ("%s:", dgettext (package, cptr->label));
            label = gtk_label_new (strval);
            g_free (strval);
            gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
            key = g_strdup_printf ("%s_%s", type, cptr->name);
            switch (cptr->type)
            {
                case CONF_TYPE_BOOL :
                                    control = gtk_switch_new ();
                                    gtk_switch_set_active (GTK_SWITCH (control), get_config_bool ("panel", key));
                                    break;

                case CONF_TYPE_INT :
                                    control = gtk_spin_button_new_with_range (0, 1000, 1); //!!!!!
                                    if (space == -1)
                                        gtk_spin_button_set_value (GTK_SPIN_BUTTON (control), get_config_int ("panel", key));
                                    else
                                        gtk_spin_button_set_value (GTK_SPIN_BUTTON (control), space);
                                    break;

                case CONF_TYPE_STRING :
                                    control = gtk_entry_new ();
                                    get_config_string ("panel", key, &strval);
                                    gtk_entry_set_text (GTK_ENTRY (control), strval);
                                    g_free (strval);
                                    break;

                case CONF_TYPE_COLOUR :
                                    control = gtk_color_button_new ();
                                    gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (control), TRUE);
                                    get_config_string ("panel", key, &strval);
                                    gdk_rgba_parse (&col, strval);
                                    g_free (strval);
                                    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (control), &col);
                                    GValue gvb = G_VALUE_INIT;
                                    g_value_init (&gvb, G_TYPE_BOOLEAN);
                                    g_value_set_boolean (&gvb, TRUE);
                                    g_object_set_property (G_OBJECT (control), "show-editor", &gvb);
                                    break;

                case CONF_TYPE_FONT :
                                    control = gtk_font_button_new ();
                                    get_config_string ("panel", key, &strval);
                                    gtk_font_chooser_set_font (GTK_FONT_CHOOSER (control), strval);
                                    g_free (strval);
                                    break;

                default :           break;
            }

            if (control)
            {
                gtk_widget_set_name (control, key);
                gtk_box_pack_end (GTK_BOX (hbox), control, FALSE, FALSE, 0);
            }
            gtk_container_add (GTK_CONTAINER (box), hbox);
            g_free (key);
            cptr++;
        }
    }
    dlclose (wid_lib);
    if (package) g_free (package);

    g_signal_connect (gtk_builder_get_object (builder, "pok_btn"), "clicked", space == -1 ? G_CALLBACK (update_config) : G_CALLBACK (update_spacing), box);
    g_signal_connect (gtk_builder_get_object (builder, "pcancel_btn"), "clicked", G_CALLBACK (close_dialog), box);
    g_signal_connect (cdlg, "destroy", G_CALLBACK (plugin_closed), NULL);

    g_object_unref (builder);

    gtk_window_set_default_size (GTK_WINDOW (cdlg), 300, -1);

    gtk_widget_show_all (cdlg);
    if (dlg)
    {
        update_buttons ();
        if (space != -1) gtk_window_set_transient_for (GTK_WINDOW (cdlg), GTK_WINDOW (dlg));
    }
    gtk_window_present (GTK_WINDOW (cdlg));
}

static void update_plugin_config (GtkWidget *box)
{
    GtkWidget *hbox, *control;
    GdkRGBA col;
    GKeyFile *kf;
    GList *children, *elem, *bchildren;
    gsize len;
    char *strval, *user_file;

    user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi", "wf-panel-pi.ini", NULL);
    kf = g_key_file_new ();
    g_key_file_load_from_file (kf, user_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

    children = gtk_container_get_children (GTK_CONTAINER (box));
    elem = children;
    while (elem)
    {
        hbox = GTK_WIDGET (elem->data);
        bchildren = gtk_container_get_children (GTK_CONTAINER (hbox));
        if (bchildren->next)
        {
            control = GTK_WIDGET (bchildren->next->data);

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
            else if (GTK_IS_FONT_BUTTON (control))
            {
                strval = gtk_font_chooser_get_font (GTK_FONT_CHOOSER (control));
                g_key_file_set_string (kf, "panel", gtk_widget_get_name (control), strval);
                g_free (strval);
            }
        }
        g_list_free (bchildren);
        elem = elem->next;
    }
    g_list_free (children);

    strval = g_key_file_to_data (kf, &len, NULL);
    g_file_set_contents (user_file, strval, len, NULL);

    g_free (strval);
    g_key_file_free (kf);
    g_free (user_file);
}

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
            g_free (type);
        }
    }

    update_buttons ();
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
    read_one_config (DOCKB, "dock", "widgets_right2");

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

static gboolean add_unused (GtkTreeModel *mod, GtkTreePath *, GtkTreeIter *iter, gpointer data)
{
    char *type;
    gtk_tree_model_get (mod, iter, COL_ID, &type, -1);
    if (!g_strcmp0 (data, type)) found = TRUE;
    g_free (type);
    return found;
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
    write_one_config (kf, DOCKB, "dock", "widgets_right2");

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

    gtk_tree_model_get (model, iter, COL_INDEX, &index, -1);

    if (index >= (long) data && index < (long) data + 100) return TRUE;

    return FALSE;
}

/* Handler for cursor-changed signal to remove highlights in other tree views */

static void unselect (GtkTreeView *, gpointer data)
{
    int count;

    for (count = 0; count < 6; count++)
    {
        if ((long) data == count) continue;

        g_signal_handler_block (tv[count], hand[count]);
        gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (tv[count])));
        g_signal_handler_unblock (tv[count], hand[count]);
    }

    update_buttons ();
}

static void close_window (GtkButton *, gpointer data)
{
    if (data) write_config ();
    gtk_widget_destroy (dlg);
}

static void conf_closed (GtkButton *, gpointer)
{
    dlg = NULL;
}

/*----------------------------------------------------------------------------*/
/* Public API */
/*----------------------------------------------------------------------------*/

void open_config_dialog (gboolean dock)
{
    GtkBuilder *builder;
    GtkCellRenderer *trend = gtk_cell_renderer_text_new ();
    int i;

    // create the list store for widgets
    widgets = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN);

    // build the dialog
    builder = gtk_builder_new_from_file (PACKAGE_DATA_DIR "/ui/config.ui");
    dlg = (GtkWidget *) gtk_builder_get_object (builder, "config_dlg");
    tv[AVAIL] = (GtkWidget *) gtk_builder_get_object (builder, "cent_tv");
    tv[PAN_L] = (GtkWidget *) gtk_builder_get_object (builder, "left_tv");
    tv[PAN_R] = (GtkWidget *) gtk_builder_get_object (builder, "right_tv");
    tv[DOCK] = (GtkWidget *) gtk_builder_get_object (builder, "dock_tv");
    tv[DOCKT] = (GtkWidget *) gtk_builder_get_object (builder, "dock_tt_tv");
    tv[DOCKB] = (GtkWidget *) gtk_builder_get_object (builder, "dock_tb_tv");
    ladd = (GtkWidget *) gtk_builder_get_object (builder, "add_l_btn");
    radd = (GtkWidget *) gtk_builder_get_object (builder, "add_r_btn");
    dadd = (GtkWidget *) gtk_builder_get_object (builder, "add_d_btn");
    dttadd = (GtkWidget *) gtk_builder_get_object (builder, "add_dtt_btn");
    dtbadd = (GtkWidget *) gtk_builder_get_object (builder, "add_dtb_btn");
    rem = (GtkWidget *) gtk_builder_get_object (builder, "rem_btn");
    wup = (GtkWidget *) gtk_builder_get_object (builder, "up_btn");
    wdn = (GtkWidget *) gtk_builder_get_object (builder, "dn_btn");
    cpl = (GtkWidget *) gtk_builder_get_object (builder, "conf_btn");

    // read in the current configuration
    read_config ();

    // set up filtering and sorting for the tree views
    for (i = 0; i < 6; i++)
    {
        filt[i] = gtk_tree_model_filter_new (GTK_TREE_MODEL (widgets), NULL);
        sort[i] = gtk_tree_model_sort_new_with_model (filt[i]);

        gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filt[i]), (GtkTreeModelFilterVisibleFunc) filter_widgets, (void *)((long) (i * 100)), NULL);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sort[i]), i == AVAIL ? COL_NAME : COL_INDEX, GTK_SORT_ASCENDING);

        gtk_tree_view_set_model (GTK_TREE_VIEW (tv[i]), sort[i]);
        hand[i] = g_signal_connect (tv[i], "cursor-changed", G_CALLBACK (unselect), (void *)((long) i));
    }

    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[AVAIL]), -1, _("Available"), trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[PAN_L]), -1, _("Panel Left"), trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[PAN_R]), -1, _("Panel Right"), trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[DOCK]), -1, _("Dock"), trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[DOCKT]), -1, _("Tray Top"), trend, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv[DOCKB]), -1, _("Tray Bottom"), trend, "text", 0, NULL);

    // connect buttton handlers
    g_signal_connect (ladd, "clicked", G_CALLBACK (add_widget), (void *) PAN_L);
    g_signal_connect (radd, "clicked", G_CALLBACK (add_widget), (void *) PAN_R);
    g_signal_connect (dadd, "clicked", G_CALLBACK (add_widget), (void *) DOCK);
    g_signal_connect (dttadd, "clicked", G_CALLBACK (add_widget), (void *) DOCKT);
    g_signal_connect (dtbadd, "clicked", G_CALLBACK (add_widget), (void *) DOCKB);

    g_signal_connect (rem, "clicked", G_CALLBACK (remove_widget), NULL);

    g_signal_connect (wup, "clicked", G_CALLBACK (move_widget), (void *) 1);
    g_signal_connect (wdn, "clicked", G_CALLBACK (move_widget), (void *) -1);

    g_signal_connect (cpl, "clicked", G_CALLBACK (configure_plugin), NULL);

    g_signal_connect (gtk_builder_get_object (builder, "cancel_btn"), "clicked", G_CALLBACK (close_window), NULL);
    g_signal_connect (gtk_builder_get_object (builder, "ok_btn"), "clicked", G_CALLBACK (close_window), (void *) 1);
    g_signal_connect (dlg, "destroy", G_CALLBACK (conf_closed), NULL);

    gtk_notebook_set_current_page (GTK_NOTEBOOK (gtk_builder_get_object (builder, "notebook1")), dock ? 1 : 0);
    update_buttons ();

    g_object_unref (builder);

    gtk_window_set_default_size (GTK_WINDOW (dlg), 640, 400);

    gtk_window_present (GTK_WINDOW (dlg));
}

/* End of file */
/*----------------------------------------------------------------------------*/
