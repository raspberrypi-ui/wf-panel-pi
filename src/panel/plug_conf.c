/*============================================================================
Copyright (c) 2026 Raspberry Pi
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
#include <libxml/xpathInternals.h>

#include "plug_conf.h"

/*----------------------------------------------------------------------------*/
/* Macros and typedefs */
/*----------------------------------------------------------------------------*/

#define XC(str) ((xmlChar *) str)

/*----------------------------------------------------------------------------*/
/* Global data */
/*----------------------------------------------------------------------------*/

GtkWidget *cdlg;

/*----------------------------------------------------------------------------*/
/* Function prototypes */
/*----------------------------------------------------------------------------*/

static char *get_config_default (const char *section, const char *key);
static gboolean get_config_bool (const char *section, const char *key);
static int get_config_int (const char *section, const char *key);
static void update_config (GtkButton *, gpointer data);
static void close_dialog (GtkButton *, gpointer data);
static void plugin_closed (GtkButton *, gpointer);
static void update_plugin_config (GtkWidget *box);

#ifdef PLUGIN_NAME
extern void update_spacing (GtkButton *, gpointer data);
#else
static void update_spacing (GtkButton *, gpointer) {}
#endif

/*----------------------------------------------------------------------------*/
/* Private functions */
/*----------------------------------------------------------------------------*/

/* Read default value for config parameter from XML file */

static char *get_config_default (const char *section, const char *key)
{
    char *file, *str;
    xmlDocPtr xDoc;
    xmlXPathObjectPtr xpathObj;
    xmlXPathContextPtr xpathCtx;
    xmlChar *cont;

    file = g_strdup_printf ("/usr/share/wf-panel-pi/metadata/%s.xml", section);

    // read in data from XML file
    xmlInitParser ();
    LIBXML_TEST_VERSION
    xDoc = xmlReadFile (file, NULL, XML_PARSE_NOBLANKS);
    g_free (file);
    if (xDoc == NULL)
    {
        xmlCleanupParser ();
        return NULL;
    }

    xpathCtx = xmlXPathNewContext (xDoc);

    str = g_strdup_printf ("/wf-panel-pi/plugin/group/option[@name='%s']/default", key);
    xpathObj = xmlXPathEvalExpression (XC (str), xpathCtx);
    g_free (str);

    if (!xmlXPathNodeSetIsEmpty (xpathObj->nodesetval))
    {
        cont = xmlNodeGetContent (xpathObj->nodesetval->nodeTab[0]);
        str = g_strdup ((char *) cont);
        xmlFree (cont);
    }
    else str = NULL;
    xmlXPathFreeObject (xpathObj);

    // cleanup XML
    xmlXPathFreeContext (xpathCtx);
    xmlFreeDoc (xDoc);
    xmlCleanupParser ();

    return str;
}

/* Get bools and ints by parsing config strings */

static gboolean get_config_bool (const char *section, const char *key)
{
    char *dest;
    gboolean res = FALSE;

    get_config_string (section, key, &dest);
    if (!g_strcmp0 (dest, "true") || !g_strcmp0 (dest, "1") || !g_strcmp0 (dest, "yes")) res = TRUE;
    g_free (dest);

    return res;
}

static int get_config_int (const char *section, const char *key)
{
    char *dest;
    int i = 0;

    get_config_string (section, key, &dest);
    sscanf (dest, "%d", &i);
    g_free (dest);

    return i;
}

/* Button handlers for dialog */

static void update_config (GtkButton *, gpointer data)
{
    update_plugin_config (GTK_WIDGET (data));
    gtk_widget_destroy (gtk_widget_get_parent (gtk_widget_get_parent (GTK_WIDGET (data))));
}

static void close_dialog (GtkButton *, gpointer data)
{
    gtk_widget_destroy (gtk_widget_get_parent (gtk_widget_get_parent (GTK_WIDGET (data))));
}

static void plugin_closed (GtkButton *, gpointer)
{
    cdlg = NULL;
}

/* Write out plugin configuration */

static void update_plugin_config (GtkWidget *box)
{
    GtkWidget *hbox, *control;
    GdkRGBA col;
    GKeyFile *kf;
    GList *children, *elem, *bchildren;
    gsize len;
    char *strval, *user_file, *sec, *param;

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
            sec = g_strdup (gtk_widget_get_name (control));
            param = strchr (sec, '/');
            *param++ = 0;

            if (GTK_IS_SWITCH (control))
                g_key_file_set_boolean (kf, sec, param, gtk_switch_get_active (GTK_SWITCH (control)));
            else if (GTK_IS_SPIN_BUTTON (control))
                g_key_file_set_integer (kf, sec, param, gtk_spin_button_get_value (GTK_SPIN_BUTTON (control)));
            else if (GTK_IS_ENTRY (control))
                g_key_file_set_string (kf, sec, param, gtk_entry_get_text (GTK_ENTRY (control)));
            else if (GTK_IS_COLOR_BUTTON (control))
            {
                gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (control), &col);
                strval = gdk_rgba_to_string (&col);
                g_key_file_set_string (kf, sec, param, strval);
                g_free (strval);
            }
            else if (GTK_IS_FONT_BUTTON (control))
            {
                strval = gtk_font_chooser_get_font (GTK_FONT_CHOOSER (control));
                g_key_file_set_string (kf, sec, param, strval);
                g_free (strval);
            }

            g_free (sec);
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

/*----------------------------------------------------------------------------*/
/* Public functions */
/*----------------------------------------------------------------------------*/

/* Read in a config string from user and system files, or get XML default */

void get_config_string (const char *section, const char *key, char **dest)
{
    char *str, *leg;
    GKeyFile *kf;
    GError *err;

    // read in data from file to a key file
    str = g_build_filename (g_get_user_config_dir (), "wf-panel-pi", "wf-panel-pi.ini", NULL);
    kf = g_key_file_new ();
    g_key_file_load_from_file (kf, str, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
    g_free (str);

    err = NULL;
    str = g_key_file_get_string (kf, section, key, &err);
    if (err == NULL && str)
    {
        *dest = str;
        g_key_file_free (kf);
        return;
    }

    // read old style XML in case this is a legacy file
    err = NULL;
    leg = g_strdup_printf ("%s_%s", section, key);
    str = g_key_file_get_string (kf, "panel", leg, &err);
    g_free (leg);
    if (err == NULL && str)
    {
        *dest = str;
        g_key_file_free (kf);
        return;
    }

    g_key_file_free (kf);

    kf = g_key_file_new ();
    g_key_file_load_from_file (kf, "/etc/xdg/wf-panel-pi/wf-panel-pi.ini", G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

    err = NULL;
    str = g_key_file_get_string (kf, section, key, &err);
    if (err == NULL && str)
    {
        *dest = str;
        g_key_file_free (kf);
        return;
    }
    g_key_file_free (kf);

    *dest = get_config_default (section, key);
}

/* Helper function to determine whether a particular widget has a config table */

gboolean can_configure (const char *type, char **name)
{
    char *libname, *package;
    void *wid_lib;
    gboolean can_conf = FALSE;
    conf_table_t * (*func_config_params)(void);
    char * (*func_package_name)(void);
    char * (*func_display_name)(void);
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

        *name = NULL;
        func_package_name = (char * (*) (void)) dlsym (wid_lib, "package_name");
        if (!dlerror ())
        {
            package = g_strdup (func_package_name());
            func_display_name = (char * (*) (void)) dlsym (wid_lib, "display_name");
            if (!dlerror ()) *name = g_strdup_printf (_("Configure %s Widget..."), dgettext (package, func_display_name ()));
            g_free (package);
        }
        if (*name == NULL) *name = g_strdup (_("Configure Plugin..."));
        dlclose (wid_lib);
    }
    return can_conf;
}

/* Plugin-specific configuration dialog */

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
            key = g_strdup_printf ("%s/%s", type, cptr->name);
            switch (cptr->type)
            {
                case CONF_TYPE_BOOL :
                                    control = gtk_switch_new ();
                                    gtk_switch_set_active (GTK_SWITCH (control), get_config_bool (type, cptr->name));
                                    break;

                case CONF_TYPE_INT :
                                    control = gtk_spin_button_new_with_range (0, 1000, 1); //!!!!!
                                    if (space == -1)
                                        gtk_spin_button_set_value (GTK_SPIN_BUTTON (control), get_config_int (type, cptr->name));
                                    else
                                        gtk_spin_button_set_value (GTK_SPIN_BUTTON (control), space);
                                    break;

                case CONF_TYPE_STRING :
                                    control = gtk_entry_new ();
                                    get_config_string (type, cptr->name, &strval);
                                    gtk_entry_set_text (GTK_ENTRY (control), strval);
                                    g_free (strval);
                                    break;

                case CONF_TYPE_COLOUR :
                                    control = gtk_color_button_new ();
                                    gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (control), TRUE);
                                    get_config_string (type, cptr->name, &strval);
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
                                    get_config_string (type, cptr->name, &strval);
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
    gtk_window_present (GTK_WINDOW (cdlg));
}

/* End of file */
/*----------------------------------------------------------------------------*/
