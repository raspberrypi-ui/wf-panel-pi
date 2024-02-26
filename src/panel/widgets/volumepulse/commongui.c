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

#define _GNU_SOURCE
#include <stdio.h>
#include <glib/gprintf.h>

#include "volumepulse.h"
#include "pulse.h"
#include "bluetooth.h"

#include "commongui.h"

/*----------------------------------------------------------------------------*/
/* Static function prototypes                                                 */
/*----------------------------------------------------------------------------*/

static void popup_window_scale_changed_vol (GtkRange *range, VolumePulsePlugin *vol);
static void popup_window_mute_toggled_vol (GtkWidget *widget, VolumePulsePlugin *vol);
static void popup_window_scale_changed_mic (GtkRange *range, VolumePulsePlugin *vol);
static void popup_window_mute_toggled_mic (GtkWidget *widget, VolumePulsePlugin *vol);
static void menu_mark_default_input (GtkWidget *widget, gpointer data);
static void menu_mark_default_output (GtkWidget *widget, gpointer data);

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

void popup_window_show (VolumePulsePlugin *vol, gboolean input_control)
{
    //VolumePulsePlugin *vol = lxpanel_plugin_get_data (p);
    int index = input_control ? 1 : 0;

    /* Create a new window. */
    vol->popup_window[index] = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name (vol->popup_window[index], "panelpopup");

    gtk_container_set_border_width (GTK_CONTAINER (vol->popup_window[index]), 0);

    /* Create a vertical box as the child of the window. */
    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (vol->popup_window[index]), box);

    /* Create a vertical scale as the child of the vertical box. */
    vol->popup_volume_scale[index] = gtk_scale_new (GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT (gtk_adjustment_new (100, 0, 100, 0, 0, 0)));
    g_object_set (vol->popup_volume_scale[index], "height-request", 120, NULL);
    gtk_scale_set_draw_value (GTK_SCALE (vol->popup_volume_scale[index]), FALSE);
    gtk_range_set_inverted (GTK_RANGE (vol->popup_volume_scale[index]), TRUE);
    gtk_box_pack_start (GTK_BOX (box), vol->popup_volume_scale[index], TRUE, TRUE, 0);
    gtk_widget_set_can_focus (vol->popup_volume_scale[index], FALSE);

    /* Value-changed and scroll-event signals. */
    vol->volume_scale_handler[index] = g_signal_connect (vol->popup_volume_scale[index], "value-changed", input_control ? G_CALLBACK (popup_window_scale_changed_mic) : G_CALLBACK (popup_window_scale_changed_vol), vol);
    g_signal_connect (vol->popup_volume_scale[index], "scroll-event", G_CALLBACK (volumepulse_mouse_scrolled), vol);

    /* Create a check button as the child of the vertical box. */
    vol->popup_mute_check[index] = gtk_check_button_new_with_label (_("Mute"));
    gtk_box_pack_end (GTK_BOX (box), vol->popup_mute_check[index], FALSE, FALSE, 0);
    vol->mute_check_handler[index] = g_signal_connect (vol->popup_mute_check[index], "toggled", input_control ? G_CALLBACK (popup_window_mute_toggled_mic) : G_CALLBACK (popup_window_mute_toggled_vol), vol);
    gtk_widget_set_can_focus (vol->popup_mute_check[index], FALSE);

    /* Realise the window */
    popup_window_at_button (&vol->popup_window[index], &vol->plugin[index], vol->bottom);
}

/* Handler for "value_changed" signal on popup window vertical scale */

static void popup_window_scale_changed_vol (GtkRange *range, VolumePulsePlugin *vol)
{
    if (pulse_get_mute (vol, FALSE)) return;

    /* Update the PulseAudio volume */
    pulse_set_volume (vol, gtk_range_get_value (range), FALSE);

    volumepulse_update_display (vol);
}

static void popup_window_scale_changed_mic (GtkRange *range, VolumePulsePlugin *vol)
{
    if (pulse_get_mute (vol, TRUE)) return;

    /* Update the PulseAudio volume */
    pulse_set_volume (vol, gtk_range_get_value (range), TRUE);

    micpulse_update_display (vol);
}

/* Handler for "toggled" signal on popup window mute checkbox */

static void popup_window_mute_toggled_vol (GtkWidget *widget, VolumePulsePlugin *vol)
{
    /* Toggle the PulseAudio mute */
    pulse_set_mute (vol, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)), FALSE);

    volumepulse_update_display (vol);
}

static void popup_window_mute_toggled_mic (GtkWidget *widget, VolumePulsePlugin *vol)
{
    /* Toggle the PulseAudio mute */
    pulse_set_mute (vol, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)), TRUE);

    micpulse_update_display (vol);
}

/*----------------------------------------------------------------------------*/
/* Device select menu                                                         */
/*----------------------------------------------------------------------------*/

/* Create the device select menu */

gboolean menu_create (VolumePulsePlugin *vol, gboolean input_control)
{
    GtkWidget *mi;
    GList *items;
    int index = input_control ? 1 : 0;

    // create input selector
    vol->menu_devices[index] = gtk_menu_new ();
    gtk_widget_set_name (vol->menu_devices[index], "panelmenu");

    // add internal devices
    pulse_add_devices_to_menu (vol, TRUE, input_control);

    // add ALSA devices
    pulse_add_devices_to_menu (vol, FALSE, input_control);

    // add Bluetooth devices
    bluetooth_add_devices_to_menu (vol, input_control);

    // update the menu item names, which are currently ALSA device names, to PulseAudio sink/source names
    pulse_update_devices_in_menu (vol, input_control);

    // show the default sink and source in the menu
    pulse_get_default_sink_source (vol);
    gtk_container_foreach (GTK_CONTAINER (vol->menu_devices[index]), input_control ? menu_mark_default_input : menu_mark_default_output, vol);

    // did we find any devices? if not, the menu will be empty...
    items = gtk_container_get_children (GTK_CONTAINER (vol->menu_devices[index]));
    if (items == NULL)
    {
        mi = gtk_menu_item_new_with_label (_("No audio devices found"));
        gtk_widget_set_sensitive (GTK_WIDGET (mi), FALSE);
        gtk_menu_shell_append (GTK_MENU_SHELL (vol->menu_devices[index]), mi);
        return FALSE;
    }
    else g_list_free (items);
    return TRUE;
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
    }
    vol->separator = TRUE;
    g_list_free (list);
}

/* Add a tickmark to the supplied widget if it is the default item in its parent menu */

void menu_mark_default_output (GtkWidget *widget, gpointer data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) data;
    const char *def, *wid = gtk_widget_get_name (widget);

    def = vol->pa_default_sink;
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

void menu_mark_default_input (GtkWidget *widget, gpointer data)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) data;
    const char *def, *wid = gtk_widget_get_name (widget);

    def = vol->pa_default_source;
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

void menu_set_alsa_device_output (GtkWidget *widget, VolumePulsePlugin *vol)
{
    pulse_change_sink (vol, gtk_widget_get_name (widget));
    pulse_move_output_streams (vol);

    volumepulse_update_display (vol);
}

void menu_set_alsa_device_input (GtkWidget *widget, VolumePulsePlugin *vol)
{
    pulse_change_source (vol, gtk_widget_get_name (widget));
    pulse_move_input_streams (vol);

    micpulse_update_display (vol);
}

/* Handler for menu click to set a Bluetooth device as output or input */

void menu_set_bluetooth_device_output (GtkWidget *widget, VolumePulsePlugin *vol)
{
    bluetooth_set_output (vol, gtk_widget_get_name (widget), gtk_menu_item_get_label (GTK_MENU_ITEM (widget)));
}

void menu_set_bluetooth_device_input (GtkWidget *widget, VolumePulsePlugin *vol)
{
    bluetooth_set_input (vol, gtk_widget_get_name (widget), gtk_menu_item_get_label (GTK_MENU_ITEM (widget)));
}

/*----------------------------------------------------------------------------*/
/* Plugin handlers and graphics                                               */
/*----------------------------------------------------------------------------*/

/* Handler for "button-press-event" signal on main widget. */

gboolean volumepulse_button_press_event (GtkWidget *, GdkEventButton *event, VolumePulsePlugin *vol)
{
    switch (event->button)
    {
        case 1: /* left-click - show popup */
                if (vol->popup_window[0]) close_popup (&vol->popup_window[0]);
                else popup_window_show (vol, FALSE);
                volumepulse_update_display (vol);
                return FALSE;

        case 2: /* middle-click - toggle mute */
                pulse_set_mute (vol, pulse_get_mute (vol, FALSE) ? 0 : 1, FALSE);
                break;

        case 3: /* right-click - show device list */
                close_popup (&vol->popup_window[0]);
                vol_menu_show (vol);
                show_menu_with_kbd (vol->plugin[0], vol->menu_devices[0]);
                break;
    }

    volumepulse_update_display (vol);
    return TRUE;
}

gboolean micpulse_button_press_event (GtkWidget *, GdkEventButton *event, VolumePulsePlugin *vol)
{
    switch (event->button)
    {
        case 1: /* left-click - show popup */
                if (vol->popup_window[1]) close_popup (&vol->popup_window[1]);
                else popup_window_show (vol, TRUE);
                micpulse_update_display (vol);
                return FALSE;

        case 2: /* middle-click - toggle mute */
                pulse_set_mute (vol, pulse_get_mute (vol, TRUE) ? 0 : 1, TRUE);
                break;

        case 3: /* right-click - show device list */
                close_popup (&vol->popup_window[1]);
                mic_menu_show (vol);
                show_menu_with_kbd (vol->plugin[1], vol->menu_devices[1]);
                break;
    }

    micpulse_update_display (vol);
    return TRUE;
}

/* Handler for "scroll-event" signal */

void volumepulse_mouse_scrolled (GtkScale *, GdkEventScroll *evt, VolumePulsePlugin *vol)
{
    if (pulse_get_mute (vol, FALSE)) return;

    /* Update the PulseAudio volume by a step */
    int val = pulse_get_volume (vol, FALSE);

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
    pulse_set_volume (vol, val, FALSE);

    volumepulse_update_display (vol);
}

void micpulse_mouse_scrolled (GtkScale *, GdkEventScroll *evt, VolumePulsePlugin *vol)
{
    if (pulse_get_mute (vol, TRUE)) return;

    /* Update the PulseAudio volume by a step */
    int val = pulse_get_volume (vol, TRUE);

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
    pulse_set_volume (vol, val, TRUE);

    micpulse_update_display (vol);
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

/* End of file */
/*----------------------------------------------------------------------------*/
