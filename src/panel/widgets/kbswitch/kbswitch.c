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

#include "kbswitch.h"

/* Prototypes */

static void update_icon (KBSwitchPlugin *kbs);
static void show_menu (KBSwitchPlugin *kbs);
static void hide_menu (KBSwitchPlugin *kbs);

/* Functions */

static void update_icon (KBSwitchPlugin *kbs)
{
    gtk_widget_show (kbs->plugin);
    gtk_widget_set_sensitive (kbs->plugin, TRUE);
}

static char *get_string (char *cmd)
{
    char *line = NULL, *res = NULL;
    size_t len = 0;
    FILE *fp = popen (cmd, "r");

    if (fp == NULL) return NULL;
    if (getline (&line, &len, fp) > 0)
    {
        res = line;
        while (*res)
        {
            if (g_ascii_isspace (*res)) *res = 0;
            res++;
        }
        res = g_strdup (line);
    }
    pclose (fp);
    g_free (line);
    return res;
}

static char *get_current_kbd (void)
{
    char *lay, *var, *kbd;

    lay = get_string ("grep XKB_DEFAULT_LAYOUT ~/.config/labwc/environment | cut -d = -f 2 | tr -d '\"' | rev | cut -d , -f 1 | rev");
    if (lay == NULL) lay = g_strdup ("gb");

    var = get_string ("grep XKB_DEFAULT_VARIANT ~/.config/labwc/environment | cut -d = -f 2 | tr -d '\"' | rev | cut -d , -f 1 | rev");
    if (var == NULL) var = g_strdup ("");

    kbd = g_strdup_printf ("%s_%s", lay, var);

    g_free (lay);
    g_free (var);
    return kbd;
}

static void set_current_kbd (char *kbd)
{
    char *lay, *var, *buffer;

    lay = strtok (kbd, "_");
    var = strtok (NULL, " \t\n\r");

    buffer = g_strdup_printf ("sudo raspi-config nonint do_change_keyboard_rc_gui pc105 %s %s", lay, var ? var : "");
    system (buffer);

    g_free (buffer);
}

static void handle_kbd_select (GtkWidget *widget, gpointer data)
{
    char *kbd;

    kbd = g_strdup (gtk_widget_get_name (widget));
    set_current_kbd (kbd);
    g_free (kbd);
}

static void show_menu (KBSwitchPlugin *kbs)
{
    GtkWidget *item;
    int i;
    char *curr = get_current_kbd ();

    hide_menu (kbs);
    kbs->menu = gtk_menu_new ();

    for (i = 0; i < MAX_KBDS; i++)
    {
        if (!g_strcmp0 ("none", kbs->kbds[i])) continue;
        item = gtk_check_menu_item_new_with_label (kbs->kbds[i]);
        gtk_widget_set_name (item, kbs->kbds[i]);
        if (!g_strcmp0 (curr, kbs->kbds[i]))
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);
        g_signal_connect (item, "activate", G_CALLBACK (handle_kbd_select), NULL);
        gtk_menu_shell_append (GTK_MENU_SHELL (kbs->menu), item);
    }

    gtk_widget_show_all (kbs->menu);
    show_menu_with_kbd (kbs->plugin, kbs->menu);
    g_free (curr);
}

static void hide_menu (KBSwitchPlugin *kbs)
{
    if (kbs->menu)
    {
		gtk_menu_popdown (GTK_MENU (kbs->menu));
		gtk_widget_destroy (kbs->menu);
		kbs->menu = NULL;
	}
}

/* Handler for menu button click */
static void kbs_button_press_event (GtkWidget *widget, KBSwitchPlugin *kbs)
{
    /* Show or hide the popup menu on left-click */
    show_menu (kbs);
}

/* Handler for system config changed message from panel */
void kbs_update_display (KBSwitchPlugin *kbs)
{
    set_taskbar_icon (kbs->tray_icon, "keyboard", kbs->icon_size);
    update_icon (kbs);
}

/* Handler for control message */
gboolean kbs_control_msg (KBSwitchPlugin *kbs, const char *cmd)
{
    char *curr, *kbd;
    int i;

    curr = get_current_kbd ();
    if (!g_strcmp0 ("switch", cmd)) 
    {
        for (i = 0; i < MAX_KBDS; i++)
            if (!g_strcmp0 (curr, kbs->kbds[i])) break;
        if (i == 5 || !g_strcmp0 ("none", kbs->kbds[i + 1])) i = 0;
        else i++;

        kbd = g_strdup (kbs->kbds[i]);
        set_current_kbd (kbd);
        g_free (kbd);
    }
    g_free (curr);
    return TRUE;
}

/* Plugin destructor. */
void kbs_destructor (gpointer user_data)
{
}

/* Plugin constructor. */
void kbs_init (KBSwitchPlugin *kbs)
{
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    /* Allocate icon as a child of top level */
    kbs->tray_icon = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (kbs->plugin), kbs->tray_icon);
    set_taskbar_icon (kbs->tray_icon, "keyboard", kbs->icon_size);
    gtk_widget_set_tooltip_text (kbs->tray_icon, _("Select a keyboard layout"));

    /* Set up button */
    gtk_button_set_relief (GTK_BUTTON (kbs->plugin), GTK_RELIEF_NONE);
    g_signal_connect (kbs->plugin, "clicked", G_CALLBACK (kbs_button_press_event), kbs);

    /* Set up variables */
    kbs->menu = NULL;

    /* Show the widget and return. */
    gtk_widget_show_all (kbs->plugin);
}

