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
/* Static function prototypes                                                 */
/*----------------------------------------------------------------------------*/

/* Helpers */
static int get_value (const char *fmt, ...);
//static void hdmi_init (VolumePulsePlugin *vol);
static const char *device_display_name (VolumePulsePlugin *vol, const char *name);

/* Menu popup */
static void menu_open_profile_dialog (GtkWidget *widget, VolumePulsePlugin *vol);

/* Profiles dialog */
static void profiles_dialog_show (VolumePulsePlugin *vol);
static void profiles_dialog_relocate_last_item (GtkWidget *box);
static void profiles_dialog_combo_changed (GtkComboBox *combo, VolumePulsePlugin *vol);
static void profiles_dialog_ok (GtkButton *button, VolumePulsePlugin *vol);
static gboolean profiles_dialog_delete (GtkWidget *wid, GdkEvent *event, VolumePulsePlugin *vol);

/*----------------------------------------------------------------------------*/
/* Generic helper functions                                                   */
/*----------------------------------------------------------------------------*/

/* Call the supplied system command and parse the result for an integer value */

static int get_value (const char *fmt, ...)
{
    char *res;
    int n, m;

    res = get_string (fmt);
    n = sscanf (res, "%d", &m);
    g_free (res);

    if (n != 1) return -1;
    else return m;
}

/* Find number of HDMI devices and device names */

void hdmi_init (VolumePulsePlugin *vol)
{
    int i, m;

    /* check xrandr for connected monitors */
    m = get_value ("xrandr -q | grep -c connected");
    if (m < 0) m = 1; /* couldn't read, so assume 1... */
    if (m > 2) m = 2;

    vol->hdmi_names[0] = NULL;
    vol->hdmi_names[1] = NULL;

    /* get the names */
    if (m == 2)
    {
        for (i = 0; i < 2; i++)
        {
            vol->hdmi_names[i] = get_string ("xrandr --listmonitors | grep %d: | cut -d ' ' -f 6", i);
        }

        /* check both devices are HDMI */
        if (vol->hdmi_names[0] && !strncmp (vol->hdmi_names[0], "HDMI", 4)
            && vol->hdmi_names[1] && !strncmp (vol->hdmi_names[1], "HDMI", 4))
                return;
    }

    /* only one device, just name it "HDMI" */
    for (i = 0; i < 2; i++)
    {
        if (vol->hdmi_names[i]) g_free (vol->hdmi_names[i]);
        vol->hdmi_names[i] = g_strdup (_("HDMI"));
    }
}

/* Remap internal to display names for BCM devices */

static const char *device_display_name (VolumePulsePlugin *vol, const char *name)
{
    if (!g_strcmp0 (name, "bcm2835 HDMI 1")) return vol->hdmi_names[0];
    else if (!g_strcmp0 (name, "vc4-hdmi")) return vol->hdmi_names[0];
    else if (!g_strcmp0 (name, "vc4-hdmi-0")) return vol->hdmi_names[0];
    else if (!g_strcmp0 (name, "bcm2835 HDMI 2")) return vol->hdmi_names[1];
    else if (!g_strcmp0 (name, "vc4-hdmi-1")) return vol->hdmi_names[1];
    else if (!g_strcmp0 (name, "bcm2835 Headphones")) return _("AV Jack");
    else return name;
}

/*----------------------------------------------------------------------------*/
/* Device select menu                                                         */
/*----------------------------------------------------------------------------*/

void vol_menu_show (VolumePulsePlugin *vol)
{
    GtkWidget *mi;
    GList *items;

    // create the menu
    menu_create (vol);

    items = gtk_container_get_children (GTK_CONTAINER (vol->menu_devices));
    if (items)
    {
        // add the profiles menu item to the top level menu
        mi = gtk_separator_menu_item_new ();
        gtk_menu_shell_append (GTK_MENU_SHELL (vol->menu_devices), mi);

        mi = gtk_menu_item_new_with_label (_("Device Profiles..."));
        g_signal_connect (mi, "activate", G_CALLBACK (menu_open_profile_dialog), (gpointer) vol);
        gtk_menu_shell_append (GTK_MENU_SHELL (vol->menu_devices), mi);

        g_list_free (items);
    }

    // lock menu if a dialog is open
    if (vol->conn_dialog || vol->profiles_dialog)
        gtk_container_foreach (GTK_CONTAINER (vol->menu_devices), (void *) gtk_widget_set_sensitive, FALSE);

    // show the menu
    gtk_widget_show_all (vol->menu_devices);
}

/* Add a device entry to the menu */

void vol_menu_add_item (VolumePulsePlugin *vol, const char *label, const char *name)
{
    GList *list, *l;
    int count;
    const char *disp_label = device_display_name (vol, label);

    GtkWidget *mi = gtk_check_menu_item_new_with_label (disp_label);
    gtk_widget_set_name (mi, name);
    if (strstr (name, "bluez"))
    {
        g_signal_connect (mi, "activate", G_CALLBACK (menu_set_bluetooth_device), (gpointer) vol);
    }
    else
    {
        g_signal_connect (mi, "activate", G_CALLBACK (menu_set_alsa_device), (gpointer) vol);
        gtk_widget_set_sensitive (mi, FALSE);
        gtk_widget_set_tooltip_text (mi, _("Output to this device not available in the current profile"));
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
        if (g_strcmp0 (disp_label, gtk_menu_item_get_label (GTK_MENU_ITEM (l->data))) < 0) break;
        count++;
        l = l->next;
    }

    gtk_menu_shell_insert (GTK_MENU_SHELL (vol->menu_devices), mi, count);
    g_list_free (list);
}

/* Handler for menu click to open the profiles dialog */

static void menu_open_profile_dialog (GtkWidget *widget, VolumePulsePlugin *vol)
{
    profiles_dialog_show (vol);
}

/*----------------------------------------------------------------------------*/
/* Profiles dialog                                                            */
/*----------------------------------------------------------------------------*/

/* Create the profiles dialog */

static void profiles_dialog_show (VolumePulsePlugin *vol)
{
    GtkWidget *btn, *wid, *box;

    // create the window itself
    vol->profiles_dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (vol->profiles_dialog), _("Device Profiles"));
    gtk_window_set_position (GTK_WINDOW (vol->profiles_dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size (GTK_WINDOW (vol->profiles_dialog), 400, -1);
    gtk_container_set_border_width (GTK_CONTAINER (vol->profiles_dialog), 10);
    gtk_window_set_icon_name (GTK_WINDOW (vol->profiles_dialog), "multimedia-volume-control");
    g_signal_connect (vol->profiles_dialog, "delete-event", G_CALLBACK (profiles_dialog_delete), vol);

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
    vol->profiles_int_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
    vol->profiles_ext_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
    vol->profiles_bt_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add (GTK_CONTAINER (vol->profiles_dialog), box);
    gtk_box_pack_start (GTK_BOX (box), vol->profiles_int_box, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box), vol->profiles_ext_box, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box), vol->profiles_bt_box, FALSE, FALSE, 0);

    // first loop through cards
    pulse_add_devices_to_profile_dialog (vol);

    // then loop through Bluetooth devices
    bluetooth_add_devices_to_profile_dialog (vol);

    wid = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout (GTK_BUTTON_BOX (wid), GTK_BUTTONBOX_END);
    gtk_box_pack_start (GTK_BOX (box), wid, FALSE, FALSE, 5);

    btn = gtk_button_new_with_mnemonic (_("_OK"));
    g_signal_connect (btn, "clicked", G_CALLBACK (profiles_dialog_ok), vol);
    gtk_box_pack_end (GTK_BOX (wid), btn, FALSE, FALSE, 5);

    gtk_widget_show_all (vol->profiles_dialog);
}

/* Add a title and combo box to the profiles dialog */

void profiles_dialog_add_combo (VolumePulsePlugin *vol, GtkListStore *ls, GtkWidget *dest, int sel, const char *label, const char *name)
{
    GtkWidget *lbl, *comb;
    GtkCellRenderer *rend;
    char *ltext;

    ltext = g_strdup_printf ("%s:", device_display_name (vol, label));
    lbl = gtk_label_new (ltext);
    gtk_label_set_xalign (GTK_LABEL (lbl), 0.0);
    gtk_box_pack_start (GTK_BOX (dest), lbl, FALSE, FALSE, 5);
    g_free (ltext);

    if (ls)
    {
        comb = gtk_combo_box_new_with_model (GTK_TREE_MODEL (ls));
        gtk_widget_set_name (comb, name);
        rend = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (comb), rend, FALSE);
        gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (comb), rend, "text", 1);
    }
    else
    {
        comb = gtk_combo_box_text_new ();
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (comb), _("Device not connected"));
        gtk_widget_set_sensitive (comb, FALSE);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (comb), sel);
    gtk_box_pack_start (GTK_BOX (dest), comb, FALSE, FALSE, 5);

    profiles_dialog_relocate_last_item (dest);

    if (ls) g_signal_connect (comb, "changed", G_CALLBACK (profiles_dialog_combo_changed), vol);
}

/* Alphabetically relocate the last item added to the profiles dialog */

static void profiles_dialog_relocate_last_item (GtkWidget *box)
{
    GtkWidget *elem, *newcomb, *newlab;
    GList *children = gtk_container_get_children (GTK_CONTAINER (box));
    int n = g_list_length (children);
    newcomb = g_list_nth_data (children, n - 1);
    newlab = g_list_nth_data (children, n - 2);
    const char *new_item = gtk_label_get_text (GTK_LABEL (newlab));
    n -= 2;
    while (n > 0)
    {
        elem = g_list_nth_data (children, n - 2);
        if (g_strcmp0 (new_item, gtk_label_get_text (GTK_LABEL (elem))) >= 0) break;
        n -= 2;
    }
    gtk_box_reorder_child (GTK_BOX (box), newlab, n);
    gtk_box_reorder_child (GTK_BOX (box), newcomb, n + 1);
    g_list_free (children);
}

/* Handler for "changed" signal from a profile combo box */

static void profiles_dialog_combo_changed (GtkComboBox *combo, VolumePulsePlugin *vol)
{
    const char *name, *option;
    GtkTreeIter iter;

    name = gtk_widget_get_name (GTK_WIDGET (combo));
    gtk_combo_box_get_active_iter (combo, &iter);
    gtk_tree_model_get (gtk_combo_box_get_model (combo), &iter, 0, &option, -1);
    pulse_set_profile (vol, name, option);

    // need to reconnect a Bluetooth device here to cause the profile to take effect...
    bluetooth_reconnect (vol, name, option);
}

/* Handler for 'OK' button on profiles dialog */

static void profiles_dialog_ok (GtkButton *button, VolumePulsePlugin *vol)
{
    close_widget (&vol->profiles_dialog);
}

/* Handler for "delete-event" signal from profiles dialog */

static gboolean profiles_dialog_delete (GtkWidget *wid, GdkEvent *event, VolumePulsePlugin *vol)
{
    close_widget (&vol->profiles_dialog);
    return TRUE;
}

/*----------------------------------------------------------------------------*/
/* Plugin handlers and graphics                                               */
/*----------------------------------------------------------------------------*/

/* Update icon and tooltip */

void volpulse_update_display (VolumePulsePlugin *vol)
{
    /* read current mute and volume status */
    gboolean mute = pulse_get_mute (vol);
    int level = pulse_get_volume (vol);
    if (mute) level = 0;

    /* update icon */
    const char *icon = "audio-volume-muted";
    if (!mute)
    {
        if (level >= 66) icon = "audio-volume-high";
        else if (level >= 33) icon = "audio-volume-medium";
        else if (level > 0) icon = "audio-volume-low";
        else icon = "audio-volume-silent";
    }
    set_taskbar_icon (GTK_IMAGE (vol->tray_icon), icon, vol->icon_size);

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
    char *tooltip = g_strdup_printf ("%s %d", _("Volume control"), level);
    gtk_widget_set_tooltip_text (vol->plugin, tooltip);
    g_free (tooltip);
}

/*----------------------------------------------------------------------------*/
/* Plugin structure                                                           */
/*----------------------------------------------------------------------------*/

/* Plugin constructor */

void volumepulse_init (VolumePulsePlugin *vol)
{
    /* Allocate and initialize plugin context */
    //VolumePulsePlugin *vol = g_new0 (VolumePulsePlugin, 1);

#ifdef ENABLE_NLS
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

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
    vol->input_control = FALSE;

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

    /* Delete any old ALSA config */
    vsystem ("rm -f ~/.asoundrc");

    /* Find HDMIs */
    hdmi_init (vol);

    /* Set up PulseAudio */
    pulse_init (vol);

    /* Set up Bluez D-Bus interface */
    bluetooth_init (vol);

    /* Show the widget and return */
    gtk_widget_show_all (vol->plugin);
    //return vol->plugin;
}

#if 0
FM_DEFINE_MODULE (lxpanel_gtk, volumepulse)

/* Plugin descriptor */

LXPanelPluginInit fm_module_init_lxpanel_gtk =
{
    .name = N_("Volume Control (PulseAudio)"),
    .description = N_("Display and control volume for PulseAudio"),
    .new_instance = volumepulse_constructor,
    .reconfigure = volumepulse_configuration_changed,
    .control = volumepulse_control_msg,
    .gettext_package = GETTEXT_PACKAGE
};
#endif
/* End of file */
/*----------------------------------------------------------------------------*/
