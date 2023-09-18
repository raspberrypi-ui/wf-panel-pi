/*
Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
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

#include "volumepulse.h"
#include "commongui.h"
#include "pulse.h"

#include "bluetooth.h"

/*----------------------------------------------------------------------------*/
/* Local macros and definitions                                               */
/*----------------------------------------------------------------------------*/

#define BT_SERV_AUDIO_SOURCE    "0000110A"
#define BT_SERV_AUDIO_SINK      "0000110B"
#define BT_SERV_HSP             "00001108"
#define BT_SERV_HFP             "0000111E"

#define BT_PULSE_RETRIES    50

/*----------------------------------------------------------------------------*/
/* Static function prototypes                                                 */
/*----------------------------------------------------------------------------*/

static char *bt_to_pa_name (const char *bluez_name, char *type, char *profile);
static char *bt_from_pa_name (const char *pa_name);
static void bt_cb_name_owned (GDBusConnection *connection, const gchar *name, const gchar *owner, gpointer user_data);
static void bt_cb_name_unowned (GDBusConnection *connection, const gchar *name, gpointer user_data);
static void bt_cb_object_removed (GDBusObjectManager *manager, GDBusObject *object, gpointer user_data);
static void bt_cb_interface_properties (GDBusObjectManagerClient *manager, GDBusObjectProxy *object_proxy, GDBusProxy *proxy, GVariant *parameters, GStrv inval, gpointer user_data);
static void bt_connect_device (VolumePulsePlugin *vol, const char *device);
static void bt_cb_connected (GObject *source, GAsyncResult *res, gpointer user_data);
static gboolean bt_conn_get_profile (gpointer user_data);
static gboolean bt_conn_set_sink_source (gpointer user_data);
static void bt_cb_trusted (GObject *source, GAsyncResult *res, gpointer user_data);
static gboolean bt_has_service (VolumePulsePlugin *vol, const gchar *path, const gchar *service);
static void bt_connect_dialog_show (VolumePulsePlugin *vol, const char *fmt, ...);
static void bt_connect_dialog_update (VolumePulsePlugin *vol, const char *msg);
static void bt_connect_dialog_ok (GtkButton *button, VolumePulsePlugin *vol);
static gboolean bt_is_connected (VolumePulsePlugin *vol, const char *path);


/*----------------------------------------------------------------------------*/
/* Bluetooth name remapping                                                   */
/*----------------------------------------------------------------------------*/

/* Convert the BlueZ name of a device to a PulseAudio sink / source / card */

static char *bt_to_pa_name (const char *bluez_name, char *type, char *profile)
{
    unsigned int b1, b2, b3, b4, b5, b6;

    if (bluez_name == NULL) return NULL;
    if (sscanf (bluez_name, "/org/bluez/hci0/dev_%x_%x_%x_%x_%x_%x", &b1, &b2, &b3, &b4, &b5, &b6) != 6)
    {
        DEBUG ("Bluez name invalid : %s", bluez_name);
        return NULL;
    }
    return g_strdup_printf ("bluez_%s.%02X_%02X_%02X_%02X_%02X_%02X%s%s",
        type, b1, b2, b3, b4, b5, b6, profile ? "." : "", profile ? profile : "");
}

/* Convert a PulseAudio sink / source / card to a BlueZ device name */

static char *bt_from_pa_name (const char *pa_name)
{
    unsigned int b1, b2, b3, b4, b5, b6;
    char *adrs;

    if (pa_name == NULL) return NULL;
    if (strstr (pa_name, "bluez") == NULL) return NULL;
    if (strstr (pa_name, "monitor") != NULL) return NULL;
    adrs = strstr (pa_name, ".");
    if (adrs == NULL) return NULL;
    if (sscanf (adrs + 1, "%x_%x_%x_%x_%x_%x", &b1, &b2, &b3, &b4, &b5, &b6) != 6) return NULL;
    return g_strdup_printf ("/org/bluez/hci0/dev_%02X_%02X_%02X_%02X_%02X_%02X", b1, b2, b3, b4, b5, b6);
}

/*----------------------------------------------------------------------------*/
/* Bluetooth D-Bus interface                                                  */
/*----------------------------------------------------------------------------*/

/* Callback for BlueZ appearing on D-Bus */

static void bt_cb_name_owned (GDBusConnection *, const gchar *name, const gchar *, gpointer user_data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) user_data;
    DEBUG ("Name %s owned on D-Bus (vol)", name);

    /* BlueZ exists - get an object manager for it */
    GError *error = NULL;
    vol->bt_objmanager = g_dbus_object_manager_client_new_for_bus_sync (G_BUS_TYPE_SYSTEM, 0, "org.bluez", "/", NULL, NULL, NULL, NULL, &error);
    if (error)
    {
        DEBUG ("Error getting object manager - %s", error->message);
        vol->bt_objmanager = NULL;
        g_error_free (error);
    }
}

/* Callback for BlueZ disappearing on D-Bus */

static void bt_cb_name_unowned (GDBusConnection *, const gchar *name, gpointer user_data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) user_data;
    DEBUG ("Name %s unowned on D-Bus", name);

    if (vol->bt_objmanager)
    {
        g_signal_handlers_disconnect_by_func (vol->bt_objmanager, G_CALLBACK (bt_cb_object_removed), vol);
        g_signal_handlers_disconnect_by_func (vol->bt_objmanager, G_CALLBACK (bt_cb_interface_properties), vol);
        g_object_unref (vol->bt_objmanager);
    }
    vol->bt_objmanager = NULL;
}

/* Callback for BlueZ device disconnecting */

static void bt_cb_object_removed (GDBusObjectManager *, GDBusObject *object, gpointer user_data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) user_data;

    DEBUG ("Bluetooth object %s removed", g_dbus_object_get_object_path (object));
    volumepulse_update_display (vol);
    micpulse_update_display (vol);
}

/* Callback for BlueZ device property change - used to detect connection */

static void bt_cb_interface_properties (GDBusObjectManagerClient *, GDBusObjectProxy *, GDBusProxy *proxy, GVariant *parameters, GStrv, gpointer user_data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) user_data;
    GVariant *var;

    DEBUG ("Bluetooth object %s property change", g_dbus_proxy_get_object_path (proxy));

    var = g_variant_lookup_value (parameters, "Trusted", NULL);
    if (var)
    {
        if (g_variant_get_boolean (var) == TRUE)
        {
            volumepulse_update_display (vol);
            micpulse_update_display (vol);
        }
        g_variant_unref (var);
    }
}

/* Connect a BlueZ device */

static void bt_connect_device (VolumePulsePlugin *vol, const char *device)
{
    GDBusInterface *interface = g_dbus_object_manager_get_interface (vol->bt_objmanager, device, "org.bluez.Device1");
    DEBUG ("Connecting device %s...", device);
    if (interface)
    {
        // trust and connect
        g_dbus_proxy_call (G_DBUS_PROXY (interface), "org.freedesktop.DBus.Properties.Set", 
            g_variant_new ("(ssv)", g_dbus_proxy_get_interface_name (G_DBUS_PROXY (interface)), "Trusted", g_variant_new_boolean (TRUE)),
            G_DBUS_CALL_FLAGS_NONE, -1, NULL, bt_cb_trusted, vol);
        g_dbus_proxy_call (G_DBUS_PROXY (interface), "Connect", NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, bt_cb_connected, vol);
        g_object_unref (interface);
    }
    else
    {
        DEBUG ("Couldn't get device interface from object manager");
        char *msg = g_strdup_printf (_("Bluetooth %s device not found"), vol->bt_input ? "input" : "output");
        bt_connect_dialog_update (vol, msg);
        g_free (msg);
    }
}

/* Callback for connect completed */

static void bt_cb_connected (GObject *source, GAsyncResult *res, gpointer user_data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) user_data;
    GError *error = NULL;

    GVariant *var = g_dbus_proxy_call_finish (G_DBUS_PROXY (source), res, &error);
    if (var) g_variant_unref (var);

    if (error)
    {
        DEBUG ("Connect error %s", error->message);

        // update dialog to show a warning
        bt_connect_dialog_update (vol, error->message);
        g_error_free (error);
    }
    else
    {
        DEBUG ("Connected OK");

        // start polling for the PulseAudio profile of the device
        vol->bt_retry_count = 0;
        vol->bt_retry_timer = g_timeout_add (50, bt_conn_get_profile, vol);
    }
}

/* Function polled after connection to get profile to confirm PA has found the device */

static gboolean bt_conn_get_profile (gpointer user_data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) user_data;
    char *pacard, *msg;
    int res;

    // some devices take a very long time to be valid PulseAudio cards after connection
    pacard = bt_to_pa_name (vol->bt_conname, "card", NULL);
    pulse_get_profile (vol, pacard);
    if (vol->pa_profile == NULL && vol->bt_retry_count++ < BT_PULSE_RETRIES)
    {
        g_free (pacard);
        return TRUE;
    }

    vol->bt_retry_timer = 0;
    DEBUG ("Profile polled %d times", vol->bt_retry_count);

    if (vol->pa_profile == NULL)
    {
        DEBUG ("Bluetooth device not found by PulseAudio - profile not available");

        // update dialog to show a warning
        bt_connect_dialog_update (vol, _("Audio device not found"));
    }
    else
    {
        DEBUG ("Bluetooth device found by PulseAudio with profile %s", vol->pa_profile);
        if (vol->pipewire)
            res = pulse_set_profile (vol, pacard, vol->bt_input ? "headset-head-unit" : "a2dp-sink");
        else
            res = pulse_set_profile (vol, pacard, vol->bt_input ? "handsfree_head_unit" : "a2dp_sink");

        if (!res)
        {
            DEBUG ("Failed to set device profile : %s", vol->pa_error_msg);
            msg = g_strdup_printf (_("Could not set profile for device : %s"), vol->pa_error_msg);
            bt_connect_dialog_update (vol, msg);
            g_free (msg);
        }
        else
        {
            pulse_get_profile (vol, pacard);
            DEBUG ("Profile set to %s", vol->pa_profile);

            vol->bt_retry_count = 0;
            vol->bt_retry_timer = g_timeout_add (50, bt_conn_set_sink_source, vol);
            g_free (pacard);
            return FALSE;
        }
    }
    g_free (pacard);

    volumepulse_update_display (vol);
    micpulse_update_display (vol);
    return FALSE;
}

static gboolean bt_conn_set_sink_source (gpointer user_data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) user_data;
    char *pacard, *msg;
    int res;

    if (vol->pipewire)
        pacard = bt_to_pa_name (vol->bt_conname, vol->bt_input ? "input" : "output", vol->bt_input ? "0" : "1");
    else
        pacard = bt_to_pa_name (vol->bt_conname, vol->bt_input ? "source" : "sink", vol->bt_input ? "handsfree_head_unit" : "a2dp_sink");

    if (vol->bt_input)
        res = pulse_change_source (vol, pacard);
    else
        res = pulse_change_sink (vol, pacard);

    if (!res)
    {
        if (vol->bt_retry_count++ < BT_PULSE_RETRIES)
        {
            g_free (pacard);
            return TRUE;
        }
        msg = g_strdup_printf (_("Could not change %s to device : %s"), vol->bt_input ? "input" : "output", vol->pa_error_msg);
        bt_connect_dialog_update (vol, msg);
        g_free (msg);
    }
    else
    {
        close_widget (&vol->conn_dialog);
        //vsystem ("echo %s > ~/.%s", pacard, vol->bt_input ? "btin" : "btout");
    }
    g_free (pacard);

    vol->bt_retry_timer = 0;
    DEBUG ("Set sink / source polled %d times", vol->bt_retry_count);

    volumepulse_update_display (vol);
    micpulse_update_display (vol);
    return FALSE;
}

/* Callback for trust completed */

static void bt_cb_trusted (GObject *source, GAsyncResult *res, gpointer user_data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) user_data;
    GError *error = NULL;

    GVariant *var = g_dbus_proxy_call_finish (G_DBUS_PROXY (source), res, &error);
    if (var) g_variant_unref (var);

    if (error)
    {
        DEBUG ("Trusting error %s", error->message);

        // update dialog to show a warning
        bt_connect_dialog_update (vol, error->message);
        g_error_free (error);
    }
    else
    {
        DEBUG ("Trusted OK");
    }
}

/* Check to see if a device has a particular service; i.e. is it input or output */

static gboolean bt_has_service (VolumePulsePlugin *vol, const gchar *path, const gchar *service)
{
    GDBusInterface *interface = g_dbus_object_manager_get_interface (vol->bt_objmanager, path, "org.bluez.Device1");
    GVariant *elem, *var = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (interface), "UUIDs");
    GVariantIter iter;
    g_variant_iter_init (&iter, var);
    while ((elem = g_variant_iter_next_value (&iter)))
    {
        const char *uuid = g_variant_get_string (elem, NULL);
        if (!g_ascii_strncasecmp (uuid, service, 8)) return TRUE;
        g_variant_unref (elem);
    }
    g_variant_unref (var);
    g_object_unref (interface);
    return FALSE;
}

/*----------------------------------------------------------------------------*/
/* Bluetooth connection dialog                                                */
/*----------------------------------------------------------------------------*/

/* Show the Bluetooth connection dialog */

static void bt_connect_dialog_show (VolumePulsePlugin *vol, const char *fmt, ...)
{
    GtkBuilder *builder;
    char *msg;
    va_list arg;

    va_start (arg, fmt);
    g_vasprintf (&msg, fmt, arg);
    va_end (arg);

    textdomain (GETTEXT_PACKAGE);

    builder = gtk_builder_new_from_file (PACKAGE_DATA_DIR "/ui/lxpanel-modal.ui");
    vol->conn_dialog = (GtkWidget *) gtk_builder_get_object (builder, "modal");
    vol->conn_label = (GtkWidget *) gtk_builder_get_object (builder, "modal_msg");
    vol->conn_ok = (GtkWidget *) gtk_builder_get_object (builder, "modal_ok");
    gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "modal_cancel")));
    gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "modal_pb")));
    g_object_unref (builder);

    gtk_label_set_text (GTK_LABEL (vol->conn_label), msg);
    gtk_widget_hide (vol->conn_ok);

    gtk_widget_show (vol->conn_dialog);
    gtk_window_set_decorated (GTK_WINDOW (vol->conn_dialog), FALSE);

    g_free (msg);
}

/* Update the message on the connection dialog to show an error */

static void bt_connect_dialog_update (VolumePulsePlugin *vol, const gchar *msg)
{
    if (!vol->conn_dialog) return;

    char *buffer = g_strdup_printf (_("Failed to connect to Bluetooth device - %s"), msg);
    gtk_label_set_text (GTK_LABEL (vol->conn_label), buffer);
    g_free (buffer);

    g_signal_connect (vol->conn_ok, "clicked", G_CALLBACK (bt_connect_dialog_ok), vol);
    gtk_widget_show (vol->conn_ok);
}

/* Handler for 'OK' button on connection dialog */

static void bt_connect_dialog_ok (GtkButton *, VolumePulsePlugin *vol)
{
    close_widget (&vol->conn_dialog);
}

/*----------------------------------------------------------------------------*/
/* External API                                                               */
/*----------------------------------------------------------------------------*/

/* Initialise BlueZ interface */

void bluetooth_init (VolumePulsePlugin *vol)
{
    /* Reset Bluetooth variables */
    vol->bt_retry_timer = 0;
    vol->bt_retry_timer = 0;

    /* Set up callbacks to see if BlueZ is on D-Bus */
    vol->bt_watcher_id = g_bus_watch_name (G_BUS_TYPE_SYSTEM, "org.bluez", 0, bt_cb_name_owned, bt_cb_name_unowned, vol, NULL);
}

/* Teardown BlueZ interface */

void bluetooth_terminate (VolumePulsePlugin *vol)
{
    if (vol->bt_retry_timer) g_source_remove (vol->bt_retry_timer);

    /* Remove signal handlers on D-Bus object manager */
    if (vol->bt_objmanager)
    {
        g_signal_handlers_disconnect_by_func (vol->bt_objmanager, G_CALLBACK (bt_cb_object_removed), vol);
        g_signal_handlers_disconnect_by_func (vol->bt_objmanager, G_CALLBACK (bt_cb_interface_properties), vol);
        g_object_unref (vol->bt_objmanager);
    }
    vol->bt_objmanager = NULL;

    /* Remove the watch on D-Bus */
    g_bus_unwatch_name (vol->bt_watcher_id);
}

/* Check to see if a Bluetooth device is connected */

gboolean bt_is_connected (VolumePulsePlugin *vol, const char *path)
{
    GDBusInterface *interface = g_dbus_object_manager_get_interface (vol->bt_objmanager, path, "org.bluez.Device1");
    GVariant *var = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (interface), "Connected");
    gboolean res = g_variant_get_boolean (var);
    g_variant_unref (var);
    g_object_unref (interface);
    return res;
}

/* Set a BlueZ device as the default PulseAudio sink */

void bluetooth_set_output (VolumePulsePlugin *vol, const char *name, const char *label)
{
    char *pacard;

    if (bt_is_connected (vol, name))
    {
        DEBUG ("Bluetooth output device already connected");

        pacard = bt_to_pa_name (name, "card", NULL);
        pulse_get_profile (vol, pacard);
        g_free (pacard);

        if (vol->pipewire)
        {
            pacard = bt_to_pa_name (name, "output", "1");
        }
        else
        {
            pacard = bt_to_pa_name (name, "sink", vol->pa_profile);
        }

        if (pulse_change_sink (vol, pacard))
        {
            pulse_move_output_streams (vol);
            //vsystem ("echo %s > ~/.btout", pacard);
        }
        else
        {
            bt_connect_dialog_show (vol, "");
            bt_connect_dialog_update (vol, _("Could not set device as output"));
        }
        g_free (pacard);
    }
    else
    {
        bt_connect_dialog_show (vol, _("Connecting Bluetooth device '%s' as output..."), label);
        vol->bt_conname = g_strdup (name);
        vol->bt_input = FALSE;
        bt_connect_device (vol, name);
    }
}

/* Set a BlueZ device as the default PulseAudio source */

void bluetooth_set_input (VolumePulsePlugin *vol, const char *name, const char *label)
{
    char *pacard;

    if (bt_is_connected (vol, name))
    {
        DEBUG ("Bluetooth input device already connected");

        pacard = bt_to_pa_name (name, "card", NULL);
        pulse_get_profile (vol, pacard);
        pulse_set_profile (vol, pacard, vol->pipewire ? "headset-head-unit" : "handsfree_head_unit");
        g_free (pacard);

        if (vol->pipewire)
        {
            pacard = bt_to_pa_name (name, "input", "0");
        }
        else
        {
            pacard = bt_to_pa_name (name, "source", "handsfree_head_unit");
        }

        if (pulse_change_source (vol, pacard))
        {
            pulse_move_input_streams (vol);
            //vsystem ("echo %s > ~/.btin", pacard);
        }
        else
        {
            bt_connect_dialog_show (vol, "");
            bt_connect_dialog_update (vol, _("Could not set device as output"));
        }
        g_free (pacard);
    }
    else
    {
        bt_connect_dialog_show (vol, _("Connecting Bluetooth device '%s' as input..."), label);
        vol->bt_conname = g_strdup (name);
        vol->bt_input = TRUE;
        bt_connect_device (vol, name);
    }
}

/* Loop through the devices BlueZ knows about, adding them to the device menu */

void bluetooth_add_devices_to_menu (VolumePulsePlugin *vol, gboolean input_control)
{
    vol->separator = FALSE;
    if (vol->bt_objmanager)
    {
        // iterate all the objects the manager knows about
        GList *objects = g_dbus_object_manager_get_objects (vol->bt_objmanager);
        while (objects != NULL)
        {
            GDBusObject *object = (GDBusObject *) objects->data;
            const char *objpath = g_dbus_object_get_object_path (object);
            GList *interfaces = g_dbus_object_get_interfaces (object);
            while (interfaces != NULL)
            {
                // if an object has a Device1 interface, it is a Bluetooth device - add it to the list
                GDBusInterface *interface = G_DBUS_INTERFACE (interfaces->data);
                if (g_strcmp0 (g_dbus_proxy_get_interface_name (G_DBUS_PROXY (interface)), "org.bluez.Device1") == 0)
                {
                    if (bt_has_service (vol, g_dbus_proxy_get_object_path (G_DBUS_PROXY (interface)), input_control ? BT_SERV_HFP : BT_SERV_AUDIO_SINK))
                    {
                        GVariant *name = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (interface), "Alias");
                        GVariant *icon = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (interface), "Icon");
                        GVariant *paired = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (interface), "Paired");
                        GVariant *trusted = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (interface), "Trusted");
                        if (name && icon && paired && trusted && g_variant_get_boolean (paired) && g_variant_get_boolean (trusted))
                        {
                            // create a menu if there isn't one already
                            menu_add_separator (vol, vol->menu_devices[input_control ? 1 : 0]);
                            if (input_control) mic_menu_add_item (vol, g_variant_get_string (name, NULL), objpath);
                            else vol_menu_add_item (vol, g_variant_get_string (name, NULL), objpath);
                        }
                        g_variant_unref (name);
                        g_variant_unref (icon);
                        g_variant_unref (paired);
                        g_variant_unref (trusted);
                    }
                    break;
                }
                interfaces = interfaces->next;
            }
            objects = objects->next;
        }
    }
}

/* Loop through the devices BlueZ knows about, adding them to the profiles dialog */

void bluetooth_add_devices_to_profile_dialog (VolumePulsePlugin *vol)
{
    if (vol->bt_objmanager)
    {
        // iterate all the objects the manager knows about
        GList *objects = g_dbus_object_manager_get_objects (vol->bt_objmanager);
        while (objects != NULL)
        {
            GDBusObject *object = (GDBusObject *) objects->data;
            const char *objpath = g_dbus_object_get_object_path (object);
            GList *interfaces = g_dbus_object_get_interfaces (object);
            while (interfaces != NULL)
            {
                // if an object has a Device1 interface, it is a Bluetooth device - add it to the list
                GDBusInterface *interface = G_DBUS_INTERFACE (interfaces->data);
                if (g_strcmp0 (g_dbus_proxy_get_interface_name (G_DBUS_PROXY (interface)), "org.bluez.Device1") == 0)
                {
                    if (bt_has_service (vol, g_dbus_proxy_get_object_path (G_DBUS_PROXY (interface)), BT_SERV_HFP)
                        || bt_has_service (vol, g_dbus_proxy_get_object_path (G_DBUS_PROXY (interface)), BT_SERV_AUDIO_SINK))
                    {
                        GVariant *name = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (interface), "Alias");
                        GVariant *icon = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (interface), "Icon");
                        GVariant *paired = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (interface), "Paired");
                        GVariant *trusted = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (interface), "Trusted");
                        if (name && icon && paired && trusted && g_variant_get_boolean (paired) && g_variant_get_boolean (trusted))
                        {
                            // only disconnected devices here...
                            char *pacard = bt_to_pa_name ((char *) objpath, "card", NULL);
                            pulse_get_profile (vol, pacard);
                            if (vol->pa_profile == NULL)
                                profiles_dialog_add_combo (vol, NULL, vol->profiles_bt_box, 0, g_variant_get_string (name, NULL), NULL);
                        }
                        g_variant_unref (name);
                        g_variant_unref (icon);
                        g_variant_unref (paired);
                        g_variant_unref (trusted);
                    }
                    break;
                }
                interfaces = interfaces->next;
            }
            objects = objects->next;
        }
    }
}

/* Loop through the devices BlueZ knows about, counting them */

int bluetooth_count_devices (VolumePulsePlugin *vol, gboolean input)
{
    int count = 0;
    if (vol->bt_objmanager)
    {
        // iterate all the objects the manager knows about
        GList *objects = g_dbus_object_manager_get_objects (vol->bt_objmanager);
        while (objects != NULL)
        {
            GDBusObject *object = (GDBusObject *) objects->data;
            GList *interfaces = g_dbus_object_get_interfaces (object);
            while (interfaces != NULL)
            {
                // if an object has a Device1 interface, it is a Bluetooth device - add it to the list
                GDBusInterface *interface = G_DBUS_INTERFACE (interfaces->data);
                if (g_strcmp0 (g_dbus_proxy_get_interface_name (G_DBUS_PROXY (interface)), "org.bluez.Device1") == 0)
                {
                    if (bt_has_service (vol, g_dbus_proxy_get_object_path (G_DBUS_PROXY (interface)), input ? BT_SERV_HFP : BT_SERV_AUDIO_SINK))
                    {
                        GVariant *name = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (interface), "Alias");
                        GVariant *icon = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (interface), "Icon");
                        GVariant *paired = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (interface), "Paired");
                        GVariant *trusted = g_dbus_proxy_get_cached_property (G_DBUS_PROXY (interface), "Trusted");
                        if (name && icon && paired && trusted && g_variant_get_boolean (paired) && g_variant_get_boolean (trusted)) count++;
                        g_variant_unref (name);
                        g_variant_unref (icon);
                        g_variant_unref (paired);
                        g_variant_unref (trusted);
                    }
                    break;
                }
                interfaces = interfaces->next;
            }
            objects = objects->next;
        }
    }
    return count;
}

/* End of file */
/*----------------------------------------------------------------------------*/
