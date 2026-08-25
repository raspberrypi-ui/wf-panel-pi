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

#include "rpcc.h"

/*----------------------------------------------------------------------------*/
/* Macros                                                                     */
/*----------------------------------------------------------------------------*/

#define TIMEOUT_MS 500

/*----------------------------------------------------------------------------*/
/* Globals                                                                    */
/*----------------------------------------------------------------------------*/

/* Controls */

static GtkWidget *sw_notify, *sw_libnotify, *spin_timeout;
static gboolean notify, libnotify;
static int timeout;
static int write_timer;

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Handlers for main window user interaction                                  */
/*----------------------------------------------------------------------------*/

static char *wfpanel_file (gboolean global)
{
    return g_build_filename (global ? "/etc/xdg" : g_get_user_config_dir (), "wf-panel-pi", "wf-panel-pi.ini", NULL);
}

static void check_directory (const char *path)
{
    char *dir = g_path_get_dirname (path);
    g_mkdir_with_parents (dir, S_IRUSR | S_IWUSR | S_IXUSR);
    g_free (dir);
}

static void load_wfpanel_settings (void)
{
    char *user_config_file;
    GKeyFile *kf;
    GError *err;
    gint val;
    gboolean res;

    notify = TRUE;
    libnotify = TRUE;
    timeout = 15;

    // read in data from file to a key file
    user_config_file = wfpanel_file (TRUE);
    kf = g_key_file_new ();
    if (g_key_file_load_from_file (kf, user_config_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
    {
        // get data from the key file
        err = NULL;
        res = g_key_file_get_boolean (kf, "notify", "enable", &err);
        if (!err) notify = res;

        err = NULL;
        res = g_key_file_get_boolean (kf, "notify", "libnotify", &err);
        if (!err) libnotify = res;

        err = NULL;
        val = g_key_file_get_integer (kf, "notify", "timeout", &err);
        if (err == NULL && val >= 0 && val <= 60) timeout = val;
    }
    g_key_file_free (kf);
    g_free (user_config_file);

    user_config_file = wfpanel_file (FALSE);
    kf = g_key_file_new ();
    if (g_key_file_load_from_file (kf, user_config_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
    {
        // get data from the key file - try the old values first and then override with the new ones if present
        err = NULL;
        res = g_key_file_get_boolean (kf, "panel", "notify_enable", &err);
        if (!err) notify = res;

        err = NULL;
        res = g_key_file_get_boolean (kf, "panel", "notify_libnotify", &err);
        if (!err) libnotify = res;

        err = NULL;
        val = g_key_file_get_integer (kf, "panel", "notify_timeout", &err);
        if (err == NULL && val >= 0 && val <= 60) timeout = val;

        err = NULL;
        res = g_key_file_get_boolean (kf, "notify", "enable", &err);
        if (!err) notify = res;

        err = NULL;
        res = g_key_file_get_boolean (kf, "notify", "libnotify", &err);
        if (!err) libnotify = res;

        err = NULL;
        val = g_key_file_get_integer (kf, "notify", "timeout", &err);
        if (err == NULL && val >= 0 && val <= 60) timeout = val;
    }
    g_key_file_free (kf);
    g_free (user_config_file);
}

static void save_wfpanel_settings (void)
{
    char *user_config_file, *str;
    GKeyFile *kf;
    gsize len;

    user_config_file = wfpanel_file (FALSE);
    check_directory (user_config_file);

    // process wfpanel config data
    kf = g_key_file_new ();
    g_key_file_load_from_file (kf, user_config_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

    // remove legacy values
    g_key_file_remove_key (kf, "panel", "notify_enable", NULL);
    g_key_file_remove_key (kf, "panel", "notify_libnotify", NULL);
    g_key_file_remove_key (kf, "panel", "notify_timeout", NULL);

    // set new values
    g_key_file_set_boolean (kf, "notify", "enable", notify);
    g_key_file_set_boolean (kf, "notify", "libnotify", libnotify);
    g_key_file_set_integer (kf, "notify", "timeout", timeout);

    str = g_key_file_to_data (kf, &len, NULL);
    g_file_set_contents (user_config_file, str, len, NULL);
    g_free (str);

    g_key_file_free (kf);
    g_free (user_config_file);
}

static void on_notify_toggle (GtkSwitch *btn, gpointer, gpointer)
{
    notify = gtk_switch_get_active (btn);
    gtk_widget_set_sensitive (sw_libnotify, notify);
    if (!notify && libnotify)
    {
        libnotify = FALSE;
        gtk_switch_set_active (GTK_SWITCH (sw_libnotify), libnotify);
    }
    else save_wfpanel_settings ();
}

static void on_libnotify_toggle (GtkSwitch *btn, gpointer, gpointer)
{
    libnotify = gtk_switch_get_active (btn);
    save_wfpanel_settings ();
}

static gboolean timeout_handler (gpointer data)
{
    timeout = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data));
    save_wfpanel_settings ();
    write_timer = 0;
    return FALSE;
}

static void on_timeout_changed (GtkSpinButton *spin, gpointer)
{
    if (write_timer) g_source_remove (write_timer);
    write_timer = g_timeout_add (TIMEOUT_MS, timeout_handler, spin);
}

void init_notify (void)
{
    GtkAdjustment *adj;

    sw_notify = (GtkWidget *) gtk_builder_get_object (builder, "sw_notify");
    sw_libnotify = (GtkWidget *) gtk_builder_get_object (builder, "sw_libnotify");
    spin_timeout = (GtkWidget *) gtk_builder_get_object (builder, "spin_timeout");

    load_wfpanel_settings ();

    gtk_switch_set_active (GTK_SWITCH (sw_notify), notify);
    g_signal_connect (sw_notify, "notify::active", G_CALLBACK (on_notify_toggle), NULL);

    gtk_switch_set_active (GTK_SWITCH (sw_libnotify), libnotify);
    gtk_widget_set_sensitive (sw_libnotify, notify);
    g_signal_connect (sw_libnotify, "notify::active", G_CALLBACK (on_libnotify_toggle), NULL);

    adj = gtk_adjustment_new (timeout, 0, 60, 5, 0, 0);
    gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (spin_timeout), adj);
    g_signal_connect (spin_timeout, "value-changed", G_CALLBACK (on_timeout_changed), NULL);
}

/* End of file                                                                */
/*----------------------------------------------------------------------------*/
