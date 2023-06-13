#include <gtk/gtk.h>
#include <glib.h>
#include <locale.h>
#include <gtk-layer-shell/gtk-layer-shell.h>

#include "config.h"


void open_config_dialog (void)
{
    GtkBuilder *builder;
    GtkWidget *dlg;

    //textdomain (GETTEXT_PACKAGE);

    builder = gtk_builder_new_from_file (PACKAGE_DATA_DIR "/ui/config.ui");
    dlg = (GtkWidget *) gtk_builder_get_object (builder, "config_dlg");

    gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
}
