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
    menu_create (vol, TRUE);

    // lock menu if a dialog is open
    if (vol->conn_dialog || vol->profiles_dialog)
        gtk_container_foreach (GTK_CONTAINER (vol->menu_devices[1]), (void *) gtk_widget_set_sensitive, FALSE);

    // show the menu
    gtk_widget_show_all (vol->menu_devices[1]);
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
        g_signal_connect (mi, "activate", G_CALLBACK (menu_set_bluetooth_device_input), (gpointer) vol);
    }
    else
    {
        g_signal_connect (mi, "activate", G_CALLBACK (menu_set_alsa_device_input), (gpointer) vol);
        gtk_widget_set_sensitive (mi, FALSE);
        gtk_widget_set_tooltip_text (mi, _("Input from this device not available in the current profile"));
    }

    // find the start point of the last section - either a separator or the beginning of the list
    list = gtk_container_get_children (GTK_CONTAINER (vol->menu_devices[1]));
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

    gtk_menu_shell_insert (GTK_MENU_SHELL (vol->menu_devices[1]), mi, count);
    g_list_free (list);
}

/*----------------------------------------------------------------------------*/
/* Plugin handlers and graphics                                               */
/*----------------------------------------------------------------------------*/

/* Update icon and tooltip */

void micpulse_update_display (VolumePulsePlugin *vol)
{
    pulse_count_devices (vol, TRUE);
    if (!vol->wizard && (vol->pa_devices + bluetooth_count_devices (vol, TRUE) > 0))
    {
        gtk_widget_show_all (vol->plugin[1]);
        gtk_widget_set_sensitive (vol->plugin[1], TRUE);
    }
    else
    {
        gtk_widget_hide (vol->plugin[1]);
        gtk_widget_set_sensitive (vol->plugin[1], FALSE);
    }

    /* read current mute and volume status */
    gboolean mute = pulse_get_mute (vol, TRUE);
    int level = pulse_get_volume (vol, TRUE);
    if (mute) level = 0;

    /* update icon */
    set_taskbar_icon (vol->tray_icon[1], mute ? "audio-input-mic-muted" : "audio-input-microphone", vol->icon_size);

    /* update popup window controls */
    if (vol->popup_window[1])
    {
        g_signal_handler_block (vol->popup_mute_check[1], vol->mute_check_handler[1]);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (vol->popup_mute_check[1]), mute);
        g_signal_handler_unblock (vol->popup_mute_check[1], vol->mute_check_handler[1]);

        g_signal_handler_block (vol->popup_volume_scale[1], vol->volume_scale_handler[1]);
        gtk_range_set_value (GTK_RANGE (vol->popup_volume_scale[1]), level);
        g_signal_handler_unblock (vol->popup_volume_scale[1], vol->volume_scale_handler[1]);

        gtk_widget_set_sensitive (vol->popup_volume_scale[1], !mute);
    }

    /* update tooltip */
    char *tooltip = g_strdup_printf ("%s %d", _("Mic volume"), level);
    gtk_widget_set_tooltip_text (vol->plugin[1], tooltip);
    g_free (tooltip);
}


/* End of file */
/*----------------------------------------------------------------------------*/
