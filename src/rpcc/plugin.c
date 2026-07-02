#include <fcntl.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <locale.h>

extern void init_main_window (void);
extern void open_config_dialog (void);

GtkWidget *main_dlg;
GtkBuilder *builder;

/*----------------------------------------------------------------------------*/
/* Plugin interface */
/*----------------------------------------------------------------------------*/

void init_plugin (GtkWidget *parent)
{
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    main_dlg = parent;
    builder = gtk_builder_new_from_file (RPCC_DATA_DIR "/ui/wf-panel-pi.ui");

    init_main_window ();
    open_config_dialog ();
}

int plugin_tabs (void)
{
    if (getenv ("WAYLAND_DISPLAY")) return 2;
    else return 0;
}

const char *tab_name (int tab)
{
    switch (tab)
    {
        case 0 : return _("Notifications");
        case 1 : return _("Widgets");
        default : return _("No such tab");
    }
}

const char *icon_name (int tab)
{
    switch (tab)
    {
        case 0 : return "dialog-warning";
        case 1 : return "applications-accessories";
        default : return NULL;
    }
}

const char *tab_id (int tab)
{
    switch (tab)
    {
        case 0 : return ("notifications");
        case 1 : return ("widgets");
        default : return NULL;
    }
}

GtkWidget *get_tab (int tab)
{
    GtkWidget *window, *plugin;

    switch (tab)
    {
        case 0 :
            window = (GtkWidget *) gtk_builder_get_object (builder, "notify_dlg");
            plugin = (GtkWidget *) gtk_builder_get_object (builder, "notif_box");
            break;

        case 1 :
            window = (GtkWidget *) gtk_builder_get_object (builder, "config_dlg");
            plugin = (GtkWidget *) gtk_builder_get_object (builder, "conf_box");
            break;

        default :
            plugin = NULL;
    }

    gtk_container_remove (GTK_CONTAINER (window), plugin);

    return plugin;
}

gboolean reboot_needed (void)
{
    return FALSE;
}

void free_plugin (void)
{
    g_object_unref (builder);
}

