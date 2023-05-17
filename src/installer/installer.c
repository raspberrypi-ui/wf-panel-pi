/*
Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#define I_KNOW_THE_PACKAGEKIT_GLIB2_API_IS_SUBJECT_TO_CHANGE
#include <packagekit-glib2/packagekit.h>

#include <X11/Xlib.h>

#include <libintl.h>

/* Controls */

static GtkWidget *msg_dlg, *msg_msg, *msg_pb, *msg_btn, *msg_btn2;

gboolean success = TRUE;

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/

static void message (char *msg, int prog);
static gboolean quit (GtkButton *button, gpointer data);
static gboolean reboot (GtkButton *button, gpointer data);
static PkResults *error_handler (PkTask *task, GAsyncResult *res, char *desc);
static void progress (PkProgress *progress, PkProgressType type, gpointer data);
static gboolean refresh_cache (gpointer data);
static void compare_versions (PkTask *task, GAsyncResult *res, gpointer data);
static void start_install (PkTask *task, GAsyncResult *res, gpointer data);
static void install_done (PkTask *task, GAsyncResult *res, gpointer data);
static gboolean close_end (gpointer data);


/*----------------------------------------------------------------------------*/
/* Progress / error box                                                       */
/*----------------------------------------------------------------------------*/

static void panel_update (void)
{
	char *id = getenv ("SUDO_UID");
	char *cmd = g_strdup_printf ("sudo -u \\#%s DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/%s/bus wfpanelctl updater check", id, id);
	system (cmd);
	g_free (cmd);
}

static void message (char *msg, int prog)
{
    if (!msg_dlg)
    {
        GtkBuilder *builder;

        builder = gtk_builder_new_from_file (PACKAGE_DATA_DIR "/ui/lxpanel-modal.ui");
        msg_dlg = (GtkWidget *) gtk_builder_get_object (builder, "modal");
        msg_msg = (GtkWidget *) gtk_builder_get_object (builder, "modal_msg");
        msg_pb = (GtkWidget *) gtk_builder_get_object (builder, "modal_pb");
        msg_btn = (GtkWidget *) gtk_builder_get_object (builder, "modal_ok");
        msg_btn2 = (GtkWidget *) gtk_builder_get_object (builder, "modal_cancel");
        gtk_label_set_text (GTK_LABEL (msg_msg), msg);
        g_object_unref (builder);
    }
    else gtk_label_set_text (GTK_LABEL (msg_msg), msg);

    gtk_widget_set_visible (msg_btn, prog <= -3);
    gtk_widget_set_visible (msg_btn2, prog == -4);
    gtk_widget_set_visible (msg_pb, prog > -2);
    if (prog == -3) g_signal_connect (msg_btn, "clicked", G_CALLBACK (quit), NULL);
    else if (prog == -4)
    {
        g_signal_connect (msg_btn2, "clicked", G_CALLBACK (quit), NULL);
        g_signal_connect (msg_btn, "clicked", G_CALLBACK (reboot), NULL);
        gtk_button_set_label (GTK_BUTTON (msg_btn), _("Reboot"));
        gtk_button_set_label (GTK_BUTTON (msg_btn2), _("Later"));
    }

    if (prog >= 0)
    {
        float progress = prog / 100.0;
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (msg_pb), progress);
    }
    else if (prog == -1) gtk_progress_bar_pulse (GTK_PROGRESS_BAR (msg_pb));
    gboolean vis = gtk_widget_is_visible (msg_dlg);
    gtk_widget_show (msg_dlg);
    // force a redraw if the window was not already displayed - forces a resize to correct size
    if (!vis)
    {
        gtk_window_set_decorated (GTK_WINDOW (msg_dlg), FALSE);
        gtk_widget_hide (msg_dlg);
        gtk_widget_unrealize (msg_dlg);
        gtk_widget_show (msg_dlg);
    }
}

static gboolean quit (GtkButton *button, gpointer data)
{
    if (success) panel_update ();
    if (msg_dlg)
    {
        gtk_widget_destroy (GTK_WIDGET (msg_dlg));
        msg_dlg = NULL;
    }

    gtk_main_quit ();
    return FALSE;
}

static gboolean reboot (GtkButton *button, gpointer data)
{
    if (msg_dlg)
    {
        gtk_widget_destroy (GTK_WIDGET (msg_dlg));
        msg_dlg = NULL;
    }

    system ("sync;reboot");
    return FALSE;
}


/*----------------------------------------------------------------------------*/
/* Helper functions for async operations                                      */
/*----------------------------------------------------------------------------*/

static PkResults *error_handler (PkTask *task, GAsyncResult *res, char *desc)
{
    PkResults *results;
    PkError *pkerror;
    GError *error = NULL;
    gchar *buf;

    results = pk_task_generic_finish (task, res, &error);
    if (error != NULL)
    {
        success = FALSE;
        buf = g_strdup_printf (_("Error %s - %s"), desc, error->message);
        message (buf, -3);
        g_free (buf);
        return NULL;
    }

    pkerror = pk_results_get_error_code (results);
    if (pkerror != NULL)
    {
        success = FALSE;
        buf = g_strdup_printf (_("Error %s - %s"), desc, pk_error_get_details (pkerror));
        message (buf, -3);
        g_free (buf);
        return NULL;
    }

    return results;
}

static void progress (PkProgress *progress, PkProgressType type, gpointer data)
{
    char *buf;
    int role = pk_progress_get_role (progress);
    int status = pk_progress_get_status (progress);

    if (msg_dlg)
    {
        switch (role)
        {
            case PK_ROLE_ENUM_REFRESH_CACHE :       if (status == PK_STATUS_ENUM_LOADING_CACHE)
                                                        message (_("Updating package data - please wait..."), pk_progress_get_percentage (progress));
                                                    else
                                                        gtk_progress_bar_pulse (GTK_PROGRESS_BAR (msg_pb));
                                                    break;

            case PK_ROLE_ENUM_GET_DETAILS :         if (status == PK_STATUS_ENUM_LOADING_CACHE)
                                                        message (_("Checking package details - please wait..."), pk_progress_get_percentage (progress));
                                                    else
                                                        gtk_progress_bar_pulse (GTK_PROGRESS_BAR (msg_pb));
                                                    break;

            case PK_ROLE_ENUM_UPDATE_PACKAGES :    if (status == PK_STATUS_ENUM_DOWNLOAD || status == PK_STATUS_ENUM_INSTALL)
                                                    {
                                                        buf = g_strdup_printf (_("%s packages - please wait..."), status == PK_STATUS_ENUM_INSTALL ? _("Installing") : _("Downloading"));
                                                        message (buf, pk_progress_get_percentage (progress));
                                                    }
                                                    else
                                                        gtk_progress_bar_pulse (GTK_PROGRESS_BAR (msg_pb));
                                                    break;
        }
    }
}


/*----------------------------------------------------------------------------*/
/* Handlers for PackageKit asynchronous install sequence                      */
/*----------------------------------------------------------------------------*/

static gboolean refresh_cache (gpointer data)
{
    PkTask *task;

    message (_("Updating package data - please wait..."), -1);

    task = pk_task_new ();

    pk_client_refresh_cache_async (PK_CLIENT (task), TRUE, NULL, (PkProgressCallback) progress, NULL, (GAsyncReadyCallback) compare_versions, NULL);
    return FALSE;
}

static void compare_versions (PkTask *task, GAsyncResult *res, gpointer data)
{
    if (!error_handler (task, res, _("updating cache"))) return;

    message (_("Comparing versions - please wait..."), -1);

    pk_client_get_updates_async (PK_CLIENT (task), PK_FILTER_ENUM_NONE, NULL, (PkProgressCallback) progress, NULL, (GAsyncReadyCallback) start_install, data);
}

static gboolean filter_fn (PkPackage *package, gpointer user_data)
{
    if (strstr (pk_package_get_arch (package), "amd64")) return FALSE;
    return TRUE;
}

static void start_install (PkTask *task, GAsyncResult *res, gpointer data)
{
    PkPackageSack *sack = NULL, *fsack;
    gchar **ids;

    PkResults *results = error_handler (task, res, _("comparing versions"));
    if (!results) return;

    if (system ("raspi-config nonint is_pi"))
    {
        sack = pk_results_get_package_sack (results);
        fsack = pk_package_sack_filter (sack, filter_fn, data);
    }
    else
    {
        fsack = pk_results_get_package_sack (results);
    }

    if (pk_package_sack_get_size (fsack) > 0)
    {
        message (_("Installing updates - please wait..."), -1);

        ids = pk_package_sack_get_ids (fsack);
        pk_task_update_packages_async (task, ids, NULL, (PkProgressCallback) progress, NULL, (GAsyncReadyCallback) install_done, NULL);
        g_strfreev (ids);
    }
    else
    {
        message (_("System is up to date"), -3);
        g_timeout_add_seconds (2, close_end, NULL);
    }

    if (sack) g_object_unref (sack);
    g_object_unref (fsack);
}

static void install_done (PkTask *task, GAsyncResult *res, gpointer data)
{
    if (!error_handler (task, res, _("installing packages"))) return;

    if (access ("/run/reboot-required", F_OK))
    {
        message (_("System is up to date"), -3);
        g_timeout_add_seconds (2, close_end, NULL);
    }
    else message (_("System is up to date.\nA reboot is required to complete the install. Reboot now or later?"), -4);
}

static gboolean close_end (gpointer data)
{
    panel_update ();
    if (msg_dlg)
    {
        gtk_widget_destroy (GTK_WIDGET (msg_dlg));
        msg_dlg = NULL;
    }

    gtk_main_quit ();
    return FALSE;
}


/*----------------------------------------------------------------------------*/
/* Main function                                                              */
/*----------------------------------------------------------------------------*/

int main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    // GTK setup
    gtk_init (&argc, &argv);
    gtk_icon_theme_prepend_search_path (gtk_icon_theme_get_default(), PACKAGE_DATA_DIR);

    g_idle_add (refresh_cache, NULL);

    gtk_main ();

    return 0;
}


/* End of file                                                                */
/*----------------------------------------------------------------------------*/
