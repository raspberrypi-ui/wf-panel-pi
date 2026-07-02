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

/* End of file */
/*============================================================================*/
