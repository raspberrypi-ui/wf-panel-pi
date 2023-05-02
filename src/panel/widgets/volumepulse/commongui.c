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
#include "pulse.h"
#include "bluetooth.h"

#include "commongui.h"

/*----------------------------------------------------------------------------*/
/* Static function prototypes                                                 */
/*----------------------------------------------------------------------------*/

static void popup_window_scale_changed (GtkRange *range, VolumePulsePlugin *vol);
static void popup_window_mute_toggled (GtkWidget *widget, VolumePulsePlugin *vol);

/*----------------------------------------------------------------------------*/
/* Generic helper functions                                                   */
/*----------------------------------------------------------------------------*/

/* System command accepting variable arguments */

int vsystem (const char *fmt, ...)
{
    char *cmdline;
    int res;

    va_list arg;
    va_start (arg, fmt);
    g_vasprintf (&cmdline, fmt, arg);
    va_end (arg);
    res = system (cmdline);
    g_free (cmdline);
    return res;
}

/* Call the supplied system command and return a new string with the first word of the result */

char *get_string (const char *fmt, ...)
{
    char *cmdline, *line = NULL, *res = NULL;
    size_t len = 0;

    va_list arg;
    va_start (arg, fmt);
    g_vasprintf (&cmdline, fmt, arg);
    va_end (arg);

    FILE *fp = popen (cmdline, "r");
    if (fp)
    {
        if (getline (&line, &len, fp) > 0)
        {
            res = line;
            while (*res++) if (g_ascii_isspace (*res)) *res = 0;
            res = g_strdup (line);
        }
        pclose (fp);
        g_free (line);
    }
    g_free (cmdline);
    return res ? res : g_strdup ("");
}

/* Destroy a widget and null its pointer */

void close_widget (GtkWidget **wid)
{
    if (*wid)
    {
        gtk_widget_destroy (*wid);
        *wid = NULL;
    }
}

/*----------------------------------------------------------------------------*/
/* Volume scale popup window                                                  */
/*----------------------------------------------------------------------------*/

/* Create the pop-up volume window */

void popup_window_create (VolumePulsePlugin *vol)
{
    //VolumePulsePlugin *vol = lxpanel_plugin_get_data (p);

    /* Create a new window. */
    vol->popup_window = GTK_WIDGET (gtk_menu_button_get_popover (GTK_MENU_BUTTON (vol->plugin)));
    gtk_widget_set_name (vol->popup_window, "panelpopup");

    gtk_container_set_border_width (GTK_CONTAINER (vol->popup_window), 0);

    /* Create a vertical box as the child of the window. */
    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (vol->popup_window), box);

    /* Create a vertical scale as the child of the vertical box. */
    vol->popup_volume_scale = gtk_scale_new (GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT (gtk_adjustment_new (100, 0, 100, 0, 0, 0)));
    g_object_set (vol->popup_volume_scale, "height-request", 120, NULL);
    gtk_scale_set_draw_value (GTK_SCALE (vol->popup_volume_scale), FALSE);
    gtk_range_set_inverted (GTK_RANGE (vol->popup_volume_scale), TRUE);
    gtk_box_pack_start (GTK_BOX (box), vol->popup_volume_scale, TRUE, TRUE, 0);
    gtk_widget_set_can_focus (vol->popup_volume_scale, FALSE);

    /* Value-changed and scroll-event signals. */
    vol->volume_scale_handler = g_signal_connect (vol->popup_volume_scale, "value-changed", G_CALLBACK (popup_window_scale_changed), vol);
    g_signal_connect (vol->popup_volume_scale, "scroll-event", G_CALLBACK (volumepulse_mouse_scrolled), vol);

    /* Create a check button as the child of the vertical box. */
    vol->popup_mute_check = gtk_check_button_new_with_label (_("Mute"));
    gtk_box_pack_end (GTK_BOX (box), vol->popup_mute_check, FALSE, FALSE, 0);
    vol->mute_check_handler = g_signal_connect (vol->popup_mute_check, "toggled", G_CALLBACK (popup_window_mute_toggled), vol);
    gtk_widget_set_can_focus (vol->popup_mute_check, FALSE);

    /* Realise the window */
    gtk_widget_show_all (vol->popup_window);
    gtk_widget_hide (vol->popup_window);
}

/* Handler for "value_changed" signal on popup window vertical scale */

static void popup_window_scale_changed (GtkRange *range, VolumePulsePlugin *vol)
{
    if (pulse_get_mute (vol)) return;

    /* Update the PulseAudio volume */
    pulse_set_volume (vol, gtk_range_get_value (range));

    volumepulse_update_display (vol);
}

/* Handler for "toggled" signal on popup window mute checkbox */

static void popup_window_mute_toggled (GtkWidget *widget, VolumePulsePlugin *vol)
{
    /* Toggle the PulseAudio mute */
    pulse_set_mute (vol, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));

    volumepulse_update_display (vol);
}

/*----------------------------------------------------------------------------*/
/* Device select menu                                                         */
/*----------------------------------------------------------------------------*/

/* Create the device select menu */

void menu_create (VolumePulsePlugin *vol)
{
    GtkWidget *mi;
    GList *items;

    // create input selector
    vol->menu_devices = gtk_menu_new ();
    gtk_widget_set_name (vol->menu_devices, "panelmenu");

    // add internal devices
    pulse_add_devices_to_menu (vol, TRUE);

    // add ALSA devices
    pulse_add_devices_to_menu (vol, FALSE);

    // add Bluetooth devices
    bluetooth_add_devices_to_menu (vol);

    // update the menu item names, which are currently ALSA device names, to PulseAudio sink/source names
    pulse_update_devices_in_menu (vol);

    // show the default sink and source in the menu
    pulse_get_default_sink_source (vol);
    gtk_container_foreach (GTK_CONTAINER (vol->menu_devices), menu_mark_default, vol);

    // did we find any devices? if not, the menu will be empty...
    items = gtk_container_get_children (GTK_CONTAINER (vol->menu_devices));
    if (items == NULL)
    {
        mi = gtk_menu_item_new_with_label (_("No audio devices found"));
        gtk_widget_set_sensitive (GTK_WIDGET (mi), FALSE);
        gtk_menu_shell_append (GTK_MENU_SHELL (vol->menu_devices), mi);
    }
    else g_list_free (items);
}

/* Add a separator to the menu (but only if there isn't already one there...) */

void menu_add_separator (VolumePulsePlugin *vol, GtkWidget *menu)
{
    GtkWidget *mi;
    GList *list, *l;

    if (menu == NULL) return;
    if (vol->separator == TRUE) return;

    // find the end of the menu
    list = gtk_container_get_children (GTK_CONTAINER (menu));
    l = g_list_last (list);
    if (l && G_OBJECT_TYPE (l->data) != GTK_TYPE_SEPARATOR_MENU_ITEM)
    {
        mi = gtk_separator_menu_item_new ();
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
        vol->separator = TRUE;
    }
    g_list_free (list);
}

/* Add a tickmark to the supplied widget if it is the default item in its parent menu */

void menu_mark_default (GtkWidget *widget, gpointer data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) data;
    const char *def, *wid = gtk_widget_get_name (widget);

    if (vol->input_control) def = vol->pa_default_source;
    else def = vol->pa_default_sink;
    if (!def || !wid) return;

    // check to see if either the two names match (for an ALSA device),
    // or if the BlueZ address from the widget is in the default name */
    if (!g_strcmp0 (def, wid) || (strstr (wid, "bluez") && strstr (def, wid + 20) && !strstr (def, "monitor")))
    {
        gulong hid = g_signal_handler_find (widget, G_SIGNAL_MATCH_ID, g_signal_lookup ("activate", GTK_TYPE_CHECK_MENU_ITEM), 0, NULL, NULL, NULL);
        g_signal_handler_block (widget, hid);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (widget), TRUE);
        g_signal_handler_unblock (widget, hid);
    }
}

/* Handler for menu click to set an ALSA device as output or input */

void menu_set_alsa_device (GtkWidget *widget, VolumePulsePlugin *vol)
{
    if (vol->input_control)
        bluetooth_remove_input (vol);
    else
        bluetooth_remove_output (vol);

    pulse_unmute_all_streams (vol);

    if (vol->input_control)
    {
        pulse_change_source (vol, gtk_widget_get_name (widget));
        pulse_move_input_streams (vol);
    }
    else
    {
        pulse_change_sink (vol, gtk_widget_get_name (widget));
        pulse_move_output_streams (vol);
    }

    volumepulse_update_display (vol);
}

/* Handler for menu click to set a Bluetooth device as output or input */

void menu_set_bluetooth_device (GtkWidget *widget, VolumePulsePlugin *vol)
{
    if (vol->input_control)
        bluetooth_set_input (vol, gtk_widget_get_name (widget), gtk_menu_item_get_label (GTK_MENU_ITEM (widget)));
    else
        bluetooth_set_output (vol, gtk_widget_get_name (widget), gtk_menu_item_get_label (GTK_MENU_ITEM (widget)));

}

/*----------------------------------------------------------------------------*/
/* Plugin handlers and graphics                                               */
/*----------------------------------------------------------------------------*/

/* Handler for "button-press-event" signal on main widget. */

gboolean volumepulse_button_press_event (GtkWidget *widget, GdkEventButton *event, VolumePulsePlugin *vol)
{
    switch (event->button)
    {
        case 1: /* left-click - fallthrough to default popover handler to show popup */
                volumepulse_update_display (vol);
                return FALSE;

        case 2: /* middle-click - toggle mute */
                pulse_set_mute (vol, pulse_get_mute (vol) ? 0 : 1);
                break;

        case 3: /* right-click - show device list */
                menu_show (vol);
                show_menu_with_kbd (vol->plugin, vol->menu_devices);
                break;
    }

    volumepulse_update_display (vol);
    return TRUE;
}

/* Handler for "scroll-event" signal */

void volumepulse_mouse_scrolled (GtkScale *scale, GdkEventScroll *evt, VolumePulsePlugin *vol)
{
    if (pulse_get_mute (vol)) return;

    /* Update the PulseAudio volume by a step */
    int val = pulse_get_volume (vol);

    if (evt->direction == GDK_SCROLL_UP || evt->direction == GDK_SCROLL_LEFT
        || (evt->direction == GDK_SCROLL_SMOOTH && (evt->delta_x < 0 || evt->delta_y < 0)))
    {
        if (val < 100) val += 2;
    }
    else if (evt->direction == GDK_SCROLL_DOWN || evt->direction == GDK_SCROLL_RIGHT
        || (evt->direction == GDK_SCROLL_SMOOTH && (evt->delta_x > 0 || evt->delta_y > 0)))
    {
        if (val > 0) val -= 2;
    }
    pulse_set_volume (vol, val);

    volumepulse_update_display (vol);
}

/*----------------------------------------------------------------------------*/
/* Plugin structure                                                           */
/*----------------------------------------------------------------------------*/

#if 0
/* Callback when panel configuration changes */

void volumepulse_configuration_changed (LXPanel *panel, GtkWidget *plugin)
{
    VolumePulsePlugin *vol = lxpanel_plugin_get_data (plugin);

    volumepulse_update_display (vol);
}
#endif

/* Callback when control message arrives */

gboolean volumepulse_control_msg (VolumePulsePlugin *vol, const char *cmd)
{
    //VolumePulsePlugin *vol = lxpanel_plugin_get_data (plugin);

    if (!strncmp (cmd, "mute", 4))
    {
        pulse_set_mute (vol, pulse_get_mute (vol) ? 0 : 1);
        volumepulse_update_display (vol);
        return TRUE;
    }

    if (!strncmp (cmd, "volu", 4))
    {
        if (pulse_get_mute (vol)) pulse_set_mute (vol, 0);
        else
        {
            int volume = pulse_get_volume (vol);
            if (volume < 100)
            {
                volume += 5;
                volume /= 5;
                volume *= 5;
            }
            pulse_set_volume (vol, volume);
        }
        volumepulse_update_display (vol);
        return TRUE;
    }

    if (!strncmp (cmd, "vold", 4))
    {
        if (pulse_get_mute (vol)) pulse_set_mute (vol, 0);
        else
        {
            int volume = pulse_get_volume (vol);
            if (volume > 0)
            {
                volume -= 1; // effectively -5 + 4 for rounding...
                volume /= 5;
                volume *= 5;
            }
            pulse_set_volume (vol, volume);
        }
        volumepulse_update_display (vol);
        return TRUE;
    }

    return FALSE;
}

/* End of file */
/*----------------------------------------------------------------------------*/
