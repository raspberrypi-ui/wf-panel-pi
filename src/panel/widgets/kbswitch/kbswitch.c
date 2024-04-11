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

static void show_menu (KBSwitchPlugin *kbs)
{
    GtkWidget *item;
    int i;

    hide_menu (kbs);

    kbs->menu = gtk_menu_new ();
    gtk_menu_set_reserve_toggle_size (GTK_MENU (kbs->menu), FALSE);

    for (i = 0; i < MAX_KBDS; i++)
    {
        item = new_menu_item (kbs->kbds[i], 40, NULL, kbs->icon_size);
        gtk_widget_show_all (item);
        //g_signal_connect (item, "activate", G_CALLBACK (handle_eject_clicked), dt);
        gtk_menu_shell_append (GTK_MENU_SHELL (kbs->menu), item);
    }

    gtk_widget_show_all (kbs->menu);
    show_menu_with_kbd (kbs->plugin, kbs->menu);
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
    if (!g_strcmp0 ("switch", cmd)) 
    {
    }
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

