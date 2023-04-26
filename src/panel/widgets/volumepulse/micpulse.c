/*
Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
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
/* Static function prototypes                                                 */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Device select menu                                                         */
/*----------------------------------------------------------------------------*/

void mic_menu_show (VolumePulsePlugin *vol)
{
    // create the menu
    menu_create (vol);

    // lock menu if a dialog is open
    if (vol->conn_dialog || vol->profiles_dialog)
        gtk_container_foreach (GTK_CONTAINER (vol->menu_devices), (void *) gtk_widget_set_sensitive, FALSE);

    // show the menu
    gtk_widget_show_all (vol->menu_devices);
}

/* Add a device entry to the menu */

void mic_menu_add_item (VolumePulsePlugin *vol, const char *label, const char *name)
{
    GList *list, *l;
    int count;

    GtkWidget *mi = gtk_check_menu_item_new_with_label (label);
    gtk_widget_set_name (mi, name);
    if (strstr (name, "bluez"))
    {
        g_signal_connect (mi, "activate", G_CALLBACK (menu_set_bluetooth_device), (gpointer) vol);
    }
    else
    {
        g_signal_connect (mi, "activate", G_CALLBACK (menu_set_alsa_device), (gpointer) vol);
        gtk_widget_set_sensitive (mi, FALSE);
        gtk_widget_set_tooltip_text (mi, _("Input from this device not available in the current profile"));
    }

    // find the start point of the last section - either a separator or the beginning of the list
    list = gtk_container_get_children (GTK_CONTAINER (vol->menu_devices));
    count = g_list_length (list);
    l = g_list_last (list);
    while (l)
    {
        if (G_OBJECT_TYPE (l->data) == GTK_TYPE_SEPARATOR_MENU_ITEM) break;
        count--;
        l = l->prev;
    }

    // if l is NULL, init to element after start; if l is non-NULL, init to element after separator
    if (!l) l = list;
    else l = l->next;

    // loop forward from the first element, comparing against the new label
    while (l)
    {
        if (g_strcmp0 (label, gtk_menu_item_get_label (GTK_MENU_ITEM (l->data))) < 0) break;
        count++;
        l = l->next;
    }

    gtk_menu_shell_insert (GTK_MENU_SHELL (vol->menu_devices), mi, count);
    g_list_free (list);
}

/*----------------------------------------------------------------------------*/
/* Plugin handlers and graphics                                               */
/*----------------------------------------------------------------------------*/

/* Update icon and tooltip */

void micpulse_update_display (VolumePulsePlugin *vol)
{
    pulse_count_devices (vol);
    if (vol->pa_devices + bluetooth_count_devices (vol, TRUE))
    {
        gtk_widget_show_all (vol->plugin);
        gtk_widget_set_sensitive (vol->plugin, TRUE);
    }
    else
    {
        gtk_widget_hide (vol->plugin);
        gtk_widget_set_sensitive (vol->plugin, FALSE);
    }

    /* read current mute and volume status */
    gboolean mute = pulse_get_mute (vol);
    int level = pulse_get_volume (vol);
    if (mute) level = 0;

    /* update icon */
    set_taskbar_icon (vol->tray_icon, mute ? "audio-input-mic-muted" : "audio-input-microphone", vol->icon_size);

    /* update popup window controls */
    if (vol->popup_window)
    {
        g_signal_handler_block (vol->popup_mute_check, vol->mute_check_handler);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (vol->popup_mute_check), mute);
        g_signal_handler_unblock (vol->popup_mute_check, vol->mute_check_handler);

        g_signal_handler_block (vol->popup_volume_scale, vol->volume_scale_handler);
        gtk_range_set_value (GTK_RANGE (vol->popup_volume_scale), level);
        g_signal_handler_unblock (vol->popup_volume_scale, vol->volume_scale_handler);

        gtk_widget_set_sensitive (vol->popup_volume_scale, !mute);
    }

    /* update tooltip */
    char *tooltip = g_strdup_printf ("%s %d", _("Mic volume"), level);
    gtk_widget_set_tooltip_text (vol->plugin, tooltip);
    g_free (tooltip);
}

/*----------------------------------------------------------------------------*/
/* Plugin structure                                                           */
/*----------------------------------------------------------------------------*/

/* Plugin constructor */

void micpulse_init (VolumePulsePlugin *vol)
{
    /* Allocate and initialize plugin context */
    //VolumePulsePlugin *vol = g_new0 (VolumePulsePlugin, 1);

    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    /* Allocate top level widget and set into plugin widget pointer */
    //vol->panel = panel;
    //vol->settings = settings;
    //vol->plugin = gtk_button_new ();
    //lxpanel_plugin_set_data (vol->plugin, vol, volumepulse_destructor);

    /* Allocate icon as a child of top level */
    vol->tray_icon = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (vol->plugin), vol->tray_icon);

    /* Set up button */
    gtk_button_set_relief (GTK_BUTTON (vol->plugin), GTK_RELIEF_NONE);
    g_signal_connect (vol->plugin, "button-release-event", G_CALLBACK (volumepulse_button_press_event), vol);
    g_signal_connect (vol->plugin, "scroll-event", G_CALLBACK (volumepulse_mouse_scrolled), vol);
    gtk_widget_add_events (vol->plugin, GDK_SCROLL_MASK);

    /* Set up variables */
    vol->input_control = TRUE;

    vol->menu_devices = NULL;
    vol->popup_window = NULL;
    vol->profiles_dialog = NULL;
    vol->conn_dialog = NULL;

    vol->pipewire = !system ("ps ax | grep pipewire-pulse | grep -qv grep");
    if (vol->pipewire)
    {
        DEBUG ("using pipewire");
    }
    else
    {
        DEBUG ("using pulseaudio");
    }
    /* Set up PulseAudio */
    pulse_init (vol);

    /* Set up Bluez D-Bus interface */
    bluetooth_init (vol);

    /* Create the popup volume control */
    popup_window_create (vol);

    /* Show the widget and return */
    gtk_widget_show_all (vol->plugin);
    //return vol->plugin;
}

/* Plugin destructor */

void micpulse_destructor (gpointer user_data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) user_data;

    close_widget (&vol->profiles_dialog);
    close_widget (&vol->conn_dialog);
    close_widget (&vol->popup_window);
    close_widget (&vol->menu_devices);

    bluetooth_terminate (vol);
    pulse_terminate (vol);

    /* Deallocate all memory. */
    //g_free (vol);
}

#if 0
FM_DEFINE_MODULE (lxpanel_gtk, micpulse)

/* Plugin descriptor */

LXPanelPluginInit fm_module_init_lxpanel_gtk =
{
    .name = N_("Microphone Control (PulseAudio)"),
    .description = N_("Display and control microphones for PulseAudio"),
    .new_instance = volumepulse_constructor,
    .reconfigure = volumepulse_configuration_changed,
    .control = volumepulse_control_msg,
    .gettext_package = GETTEXT_PACKAGE
};
#endif
/* End of file */
/*----------------------------------------------------------------------------*/
