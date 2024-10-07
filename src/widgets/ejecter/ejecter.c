/*
Copyright (c) 2018 Raspberry Pi (Trading) Ltd.
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
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>

#include <gio/gio.h>

#include "ejecter.h"
//#include "plugin.h"

#define DEBUG_ON
#ifdef DEBUG_ON
#define DEBUG(fmt,args...) if(getenv("DEBUG_EJ"))g_message("ej: " fmt,##args)
#else
#define DEBUG(fmt,args...)
#endif

/* Plug-in global data */

typedef struct {
    EjecterPlugin *ej;
    GDrive *drv;
} CallbackData;

typedef struct {
    GDrive *drv;
    int seq;
} EjectList;

#define HIDE_TIME_MS 5000

/* Prototypes */

static void handle_eject_clicked (GtkWidget *widget, gpointer ptr);
static void eject_done (GObject *source_object, GAsyncResult *res, gpointer ptr);
static void update_icon (EjecterPlugin *ej);
static void show_menu (EjecterPlugin *ej);
static void hide_menu (EjecterPlugin *ej);
static GtkWidget *create_menuitem (EjecterPlugin *ej, GDrive *d);

static void log_eject (EjecterPlugin *ej, GDrive *drive)
{
    EjectList *el;
    el = g_new (EjectList, 1);
    el->drv = drive;
    el->seq = -1;
    ej->ejdrives = g_list_append (ej->ejdrives, el);
}

static gboolean was_ejected (EjecterPlugin *ej, GDrive *drive)
{
    GList *l;
    gboolean ejected = FALSE;
    for (l = ej->ejdrives; l != NULL; l = l->next)
    {
        EjectList *el = (EjectList *) l->data;
        if (el->drv == drive)
        {
            ejected = TRUE;
            if (el->seq != -1) lxpanel_notify_clear (el->seq);
            ej->ejdrives = g_list_remove (ej->ejdrives, el);
            g_free (el);
        }
    }
    return ejected;
}

static void log_mount (EjecterPlugin *ej, GMount *mount)
{
    GList *l;
    GDrive *drv, *drive = g_mount_get_drive (mount);
    for (l = ej->mdrives; l != NULL; l = l->next)
    {
        drv = (GDrive *) l->data;
        if (drv == drive)
        {
            g_object_unref (drive);
            return;
        }
    }

    ej->mdrives = g_list_append (ej->mdrives, drive);
    DEBUG ("MOUNTED DRIVE %s", g_drive_get_name (drive));
}

static void log_init_mounts (EjecterPlugin *ej)
{
    ej->mdrives = NULL;
    GList *l, *mnts = g_volume_monitor_get_mounts (ej->monitor);
    for (l = mnts; l != NULL; l = l->next)
    {
        log_mount (ej, (GMount *) l->data);
        g_object_unref (l->data);
    }
    g_list_free (mnts);
}

static gboolean was_mounted (EjecterPlugin *ej, GDrive *drive)
{
    GList *l;
    GDrive *drv;
    for (l = ej->mdrives; l != NULL; l = l->next)
    {
        drv = (GDrive *) l->data;
        if (drv == drive)
        {
            ej->mdrives = g_list_remove (ej->mdrives, drv);
            g_object_unref (drv);
            return TRUE;
        }
    }
    return FALSE;
}

static void add_seq_for_drive (EjecterPlugin *ej, GDrive *drive, int seq)
{
    GList *l;
    for (l = ej->ejdrives; l != NULL; l = l->next)
    {
        EjectList *el = (EjectList *) l->data;
        if (el->drv == drive)
        {
            el->seq = seq;
            return;
        }
    }
}

static void handle_mount_in (GtkWidget *, GMount *mount, gpointer data)
{
    EjecterPlugin *ej = (EjecterPlugin *) data;
    DEBUG ("MOUNT ADDED %s", g_mount_get_name (mount));

    log_mount (ej, mount);
    if (ej->menu && gtk_widget_get_visible (ej->menu)) show_menu (ej);
    update_icon (ej);
}

static void handle_mount_out (GtkWidget *, GMount *mount, gpointer data)
{
    EjecterPlugin *ej = (EjecterPlugin *) data;
    DEBUG ("MOUNT REMOVED %s", g_mount_get_name (mount));

    if (ej->menu && gtk_widget_get_visible (ej->menu)) show_menu (ej);
    update_icon (ej);
}

static void handle_mount_pre (GtkWidget *, GMount *mount, gpointer data)
{
    EjecterPlugin *ej = (EjecterPlugin *) data;
    DEBUG ("MOUNT PREUNMOUNT %s", g_mount_get_name (mount));
    log_eject (ej, g_mount_get_drive (mount));
}

static void handle_volume_in (GtkWidget *, GVolume *vol, gpointer data)
{
    EjecterPlugin *ej = (EjecterPlugin *) data;
    DEBUG ("VOLUME ADDED %s", g_volume_get_name (vol));

    if (ej->menu && gtk_widget_get_visible (ej->menu)) show_menu (ej);
    update_icon (ej);
}

static void handle_volume_out (GtkWidget *, GVolume *vol, gpointer data)
{
    EjecterPlugin *ej = (EjecterPlugin *) data;
    DEBUG ("VOLUME REMOVED %s", g_volume_get_name (vol));

    if (ej->menu && gtk_widget_get_visible (ej->menu)) show_menu (ej);
    update_icon (ej);
}

static void handle_drive_in (GtkWidget *, GDrive *drive, gpointer data)
{
    EjecterPlugin *ej = (EjecterPlugin *) data;
    DEBUG ("DRIVE ADDED %s", g_drive_get_name (drive));

    if (ej->menu && gtk_widget_get_visible (ej->menu)) show_menu (ej);
    update_icon (ej);
}

static void handle_drive_out (GtkWidget *, GDrive *drive, gpointer data)
{
    EjecterPlugin *ej = (EjecterPlugin *) data;
    DEBUG ("DRIVE REMOVED %s", g_drive_get_name (drive));

    if (was_mounted (ej, drive) && !was_ejected (ej, drive))
        lxpanel_notify (_("Drive was removed without ejecting\nPlease use menu to eject before removal"));

    if (ej->menu && gtk_widget_get_visible (ej->menu)) show_menu (ej);
    update_icon (ej);
}

static void handle_eject_clicked (GtkWidget *, gpointer data)
{
    CallbackData *dt = (CallbackData *) data;
    EjecterPlugin *ej = dt->ej;
    GDrive *drv = dt->drv;
    DEBUG ("EJECT %s", g_drive_get_name (drv));

    g_drive_eject_with_operation (drv, G_MOUNT_UNMOUNT_NONE, NULL, NULL, eject_done, ej);
}

static void eject_done (GObject *source_object, GAsyncResult *res, gpointer data)
{
    EjecterPlugin *ej = (EjecterPlugin *) data;
    GDrive *drv = (GDrive *) source_object;
    char *buffer;
    GError *err = NULL;

    g_drive_eject_with_operation_finish (drv, res, &err);

    if (err == NULL)
    {
        DEBUG ("EJECT COMPLETE");
        buffer = g_strdup_printf (_("%s has been ejected\nIt is now safe to remove the device"), g_drive_get_name (drv));
        add_seq_for_drive (ej, drv, lxpanel_notify (buffer));
    }
    else
    {
        DEBUG ("EJECT FAILED");
        buffer = g_strdup_printf (_("Failed to eject %s\n%s"), g_drive_get_name (drv), err->message);
        lxpanel_notify (buffer);
    }
    g_free (buffer);
}


/* Ejecter functions */

static gboolean is_drive_mounted (GDrive *d)
{
    GList *viter, *vols = g_drive_get_volumes (d);

    for (viter = vols; viter != NULL; viter = g_list_next (viter))
    {
        if (g_volume_get_mount ((GVolume *) viter->data) != NULL) return TRUE;
    }
    return FALSE;
}

static void update_icon (EjecterPlugin *ej)
{
    if (ej->autohide)
    {
        /* loop through all devices, checking for mounted volumes */
        GList *driter, *drives = g_volume_monitor_get_connected_drives (ej->monitor);

        for (driter = drives; driter != NULL; driter = g_list_next (driter))
        {
            GDrive *drv = (GDrive *) driter->data;
            if (is_drive_mounted (drv))
            {
                gtk_widget_show_all (ej->plugin);
                gtk_widget_set_sensitive (ej->plugin, TRUE);
                return;
            }
        }
        gtk_widget_hide (ej->plugin);
        gtk_widget_set_sensitive (ej->plugin, FALSE);
    }
    else
    {
        gtk_widget_show (ej->plugin);
        gtk_widget_set_sensitive (ej->plugin, TRUE);
    }
}

static void show_menu (EjecterPlugin *ej)
{
    hide_menu (ej);

    ej->menu = gtk_menu_new ();
    gtk_menu_set_reserve_toggle_size (GTK_MENU (ej->menu), FALSE);

    /* loop through all devices, creating menu items for them */
    GList *driter, *drives = g_volume_monitor_get_connected_drives (ej->monitor);
    int count = 0;

    for (driter = drives; driter != NULL; driter = g_list_next (driter))
    {
        GDrive *drv = (GDrive *) driter->data;
        if (is_drive_mounted (drv))
        {
            GtkWidget *item = create_menuitem (ej, drv);
            CallbackData *dt = g_new0 (CallbackData, 1);
            dt->ej = ej;
            dt->drv = drv;
            g_signal_connect (item, "activate", G_CALLBACK (handle_eject_clicked), dt);
            gtk_menu_shell_append (GTK_MENU_SHELL (ej->menu), item);
            count++;
        }
    }

    if (count)
    {
        gtk_widget_show_all (ej->menu);
        show_menu_with_kbd (ej->plugin, ej->menu);
    }
}

static void hide_menu (EjecterPlugin *ej)
{
    if (ej->menu)
    {
		gtk_menu_popdown (GTK_MENU (ej->menu));
		gtk_widget_destroy (ej->menu);
		ej->menu = NULL;
	}
}

static GtkWidget *create_menuitem (EjecterPlugin *ej, GDrive *d)
{
    char buffer[1024];
    GList *vols;
    GVolume *v;
    GtkWidget *item, *icon, *eject;

    vols = g_drive_get_volumes (d);

    sprintf (buffer, "%s (", g_drive_get_name (d));
    GList *iter;
    gboolean first = TRUE;
    for (iter = vols; iter != NULL; iter = g_list_next (iter))
    {
        v = (GVolume *) iter->data;
        if (g_volume_get_name (v))
        {
            if (first) first = FALSE;
            else strcat (buffer, ", ");
            strcat (buffer, g_volume_get_name (v));
        }
    }
    strcat (buffer, ")");
    icon = gtk_image_new_from_gicon (g_drive_get_icon (d), GTK_ICON_SIZE_BUTTON);

    item = new_menu_item (buffer, 40, NULL, ej->icon_size);
    update_menu_icon (item, icon);

    eject = gtk_image_new ();
    set_menu_icon (eject, "media-eject", ej->icon_size);
    append_menu_icon (item, eject);

    gtk_widget_show_all (item);

    return item;
}

/* Handler for menu button click */
static void ejecter_button_press_event (GtkWidget *, EjecterPlugin * ej)
{
    //EjecterPlugin * ej = lxpanel_plugin_get_data (widget);

    /* Show or hide the popup menu on left-click */
    if (pressed != PRESS_LONG) show_menu (ej);
    pressed = PRESS_NONE;
}

static void ejecter_gesture_pressed (GtkGestureLongPress *, gdouble x, gdouble y, EjecterPlugin *)
{
    pressed = PRESS_LONG;
    press_x = x;
    press_y = y;
}

static void ejecter_gesture_end (GtkGestureLongPress *, GdkEventSequence *, EjecterPlugin *ej)
{
    if (pressed == PRESS_LONG) pass_right_click (ej->plugin, press_x, press_y);
}

/* Handler for system config changed message from panel */
void ej_update_display (EjecterPlugin * ej)
{
    //EjecterPlugin * ej = lxpanel_plugin_get_data (p);

    set_taskbar_icon (ej->tray_icon, "media-eject", ej->icon_size);
    update_icon (ej);
}

/* Handler for control message */
gboolean ejecter_control_msg (EjecterPlugin *ej, const char *cmd)
{
    //EjecterPlugin *ej = lxpanel_plugin_get_data (plugin);

    DEBUG ("Eject command device %s\n", cmd);

    /* Loop through all drives until we find the one matching the supplied device */
    GList *iter, *drives = g_volume_monitor_get_connected_drives (ej->monitor);
    for (iter = drives; iter != NULL; iter = g_list_next (iter))
    {
        GDrive *d = iter->data;
        char *id = g_drive_get_identifier (d, "unix-device");

        if (!g_strcmp0 (id, cmd)) 
        {
            DEBUG ("EXTERNAL EJECT %s", g_drive_get_name (d));
            log_eject (ej, d);
        }
        g_free (id);
    }
    g_list_free_full (drives, g_object_unref);
    return TRUE;
}

/* Plugin destructor. */
void ejecter_destructor (gpointer user_data)
{
    EjecterPlugin * ej = (EjecterPlugin *) user_data;

    /* Deallocate memory */
    if (ej->gesture) g_object_unref (ej->gesture);
    g_free (ej);
}

/* Plugin constructor. */
//static GtkWidget *ejecter_constructor (LXPanel *panel, config_setting_t *settings)
void ej_init (EjecterPlugin *ej)
{
    /* Allocate and initialize plugin context */
    //EjecterPlugin *ej = g_new0 (EjecterPlugin, 1);

    /* Allocate top level widget and set into plugin widget pointer. */
    //ej->panel = panel;
    //ej->settings = settings;
    //ej->plugin = gtk_button_new ();
    //lxpanel_plugin_set_data (ej->plugin, ej, ejecter_destructor);

    /* Allocate icon as a child of top level */
    ej->tray_icon = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (ej->plugin), ej->tray_icon);
    set_taskbar_icon (ej->tray_icon, "media-eject", ej->icon_size);
    gtk_widget_set_tooltip_text (ej->tray_icon, _("Select a drive in menu to eject safely"));

    /* Set up button */
    gtk_button_set_relief (GTK_BUTTON (ej->plugin), GTK_RELIEF_NONE);
    g_signal_connect (ej->plugin, "clicked", G_CALLBACK (ejecter_button_press_event), ej);

    /* Set up long press */
    ej->gesture = gtk_gesture_long_press_new (ej->plugin);
    gtk_gesture_single_set_touch_only (GTK_GESTURE_SINGLE (ej->gesture), TRUE);
    g_signal_connect (ej->gesture, "pressed", G_CALLBACK (ejecter_gesture_pressed), ej);
    g_signal_connect (ej->gesture, "end", G_CALLBACK (ejecter_gesture_end), ej);
    gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (ej->gesture), GTK_PHASE_BUBBLE);

    /* Set up variables */
    ej->popup = NULL;
    ej->menu = NULL;

    ej->hide_timer = 0;

    /* Get volume monitor and connect to events */
    ej->monitor = g_volume_monitor_get ();
    g_signal_connect (ej->monitor, "volume-added", G_CALLBACK (handle_volume_in), ej);
    g_signal_connect (ej->monitor, "volume-removed", G_CALLBACK (handle_volume_out), ej);
    g_signal_connect (ej->monitor, "mount-added", G_CALLBACK (handle_mount_in), ej);
    g_signal_connect (ej->monitor, "mount-removed", G_CALLBACK (handle_mount_out), ej);
    g_signal_connect (ej->monitor, "mount-pre-unmount", G_CALLBACK (handle_mount_pre), ej);
    g_signal_connect (ej->monitor, "drive-connected", G_CALLBACK (handle_drive_in), ej);
    g_signal_connect (ej->monitor, "drive-disconnected", G_CALLBACK (handle_drive_out), ej);

    log_init_mounts (ej);

    /* Show the widget and return. */
    gtk_widget_show_all (ej->plugin);
    //return ej->plugin;
}
#if 0
static gboolean ejecter_apply_configuration (gpointer user_data)
{
    EjecterPlugin *ej = lxpanel_plugin_get_data ((GtkWidget *) user_data);

    config_group_set_int (ej->settings, "AutoHide", ej->autohide);
    if (ej->autohide) update_icon (ej);
    else gtk_widget_show_all (ej->plugin);
    return FALSE;
}

static GtkWidget *ejecter_configure (LXPanel *panel, GtkWidget *p)
{
    EjecterPlugin *ej = lxpanel_plugin_get_data (p);

    return lxpanel_generic_config_dlg(_("Ejecter"), panel,
        ejecter_apply_configuration, p,
        _("Hide icon when no devices"), &ej->autohide, CONF_TYPE_BOOL,
        NULL);
}

FM_DEFINE_MODULE (lxpanel_gtk, ejecter)

/* Plugin descriptor. */
LXPanelPluginInit fm_module_init_lxpanel_gtk = {
    .name = N_("Ejecter"),
    .description = N_("Ejects mounted drives"),
    .new_instance = ejecter_constructor,
    .reconfigure = ejecter_configuration_changed,
    .button_press_event = ejecter_button_press_event,
    .config = ejecter_configure,
    .control = ejecter_control_msg,
    .gettext_package = GETTEXT_PACKAGE
};
#endif
