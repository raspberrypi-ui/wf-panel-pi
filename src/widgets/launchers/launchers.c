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

#include <locale.h>
#include <glib/gi18n.h>
#include <gio/gdesktopappinfo.h>

#include "lxutils.h"
#include "launcher.h"

#include "launchers.h"

/*----------------------------------------------------------------------------*/
/* Typedefs and macros                                                        */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Global data                                                                */
/*----------------------------------------------------------------------------*/

conf_table_t conf_table[2] = {
    {CONF_TYPE_INT,     "spacing",  N_("Icon spacing"), NULL},
    {CONF_TYPE_NONE,    NULL,       NULL,               NULL}
};

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/

static void launch_id (GtkWidget *widget);
static void remove_launcher (GtkWidget *widget, gpointer);
static void popup_menu (GtkWidget *widget);
static void add_launcher (LauncherPlugin *lch, char *id);
static void load_launchers (LauncherPlugin *lch);
static void update_icons (LauncherPlugin *lch);
static gboolean handle_button_pressed (GtkWidget *btn, GdkEventButton *, gpointer userdata);
static gboolean handle_button_release (GtkWidget *btn, GdkEventButton *event, gpointer userdata);
static void handle_gesture_end (GtkGestureLongPress *, GdkEventSequence *, gpointer userdata);
static void handle_drag_begin (GtkGestureDrag *, gdouble x, gdouble, gpointer userdata);
static void handle_drag_update (GtkGestureDrag *, gdouble x, gdouble, gpointer userdata);
static void handle_drag_end (GtkGestureDrag *, gdouble, gdouble, gpointer userdata);

/*----------------------------------------------------------------------------*/
/* Function definitions                                                       */
/*----------------------------------------------------------------------------*/

static void launch_id (GtkWidget *widget)
{
    GAppInfo *info = (GAppInfo *) g_desktop_app_info_new (gtk_widget_get_name (widget));
    g_app_info_launch (info, NULL, NULL, NULL);
    g_free (info);
}

static void remove_launcher (GtkWidget *widget, gpointer)
{
    remove_from_launcher (gtk_widget_get_name (widget));
}

static void popup_menu (GtkWidget *widget)
{
    GtkWidget *menu, *item;

    menu = gtk_menu_new ();
    item = gtk_menu_item_new_with_label (_("Remove from Launcher"));
    gtk_widget_set_name (item, gtk_widget_get_name (widget));
    g_signal_connect (item, "activate", G_CALLBACK (remove_launcher), NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    gtk_widget_show_all (menu);
    gtk_menu_popup_at_widget (GTK_MENU (menu), widget, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
}

static void add_launcher (LauncherPlugin *lch, char *id)
{
    GtkWidget *btn, *icon;
    GAppInfo *info;
    GtkGesture *drag;
    GIcon *ic;
    char *str;

    str = g_strdup_printf ("%s.desktop", id);
    info = (GAppInfo *) g_desktop_app_info_new (str);

    btn = gtk_button_new ();
    gtk_widget_set_tooltip_text (btn, g_app_info_get_name (info));
    gtk_widget_set_name (btn, str);
    g_signal_connect (btn, "button-press-event", G_CALLBACK (handle_button_pressed), lch);
    g_signal_connect (btn, "button-release-event", G_CALLBACK (handle_button_release), lch);
    g_free (str);
    
    add_long_press (btn, G_CALLBACK (handle_gesture_end), lch);

    drag = gtk_gesture_drag_new (btn);
    g_signal_connect (drag, "drag-begin", G_CALLBACK (handle_drag_begin), lch);
    g_signal_connect (drag, "drag-update", G_CALLBACK (handle_drag_update), lch);
    g_signal_connect (drag, "drag-end", G_CALLBACK (handle_drag_end), lch);

    ic = g_app_info_get_icon (info);

    str = g_icon_to_string (ic);
    icon = gtk_image_new ();
    wrap_set_taskbar_icon (lch, icon, str);
    g_free (str);

    g_free (info);

    gtk_container_add (GTK_CONTAINER (btn), icon);
    gtk_container_add (GTK_CONTAINER (lch->plugin), btn);
    gtk_widget_show_all (lch->plugin);
}

static void load_launchers (LauncherPlugin *lch)
{
    char *lstr, *launcher;

    lstr = g_strdup (lch->launchers);
    launcher = strtok (lstr, " ");
    while (launcher)
    {
        add_launcher (lch, launcher);
        launcher = strtok (NULL, " ");
    }
    g_free (lstr);
}

static void update_icons (LauncherPlugin *lch)
{
    GList *list = gtk_container_get_children (GTK_CONTAINER (lch->plugin));
    g_list_free_full (list, (GDestroyNotify) gtk_widget_destroy);

    gtk_box_set_spacing (GTK_BOX (lch->plugin), lch->spacing);
    load_launchers (lch);
}

/*----------------------------------------------------------------------------*/
/* Handlers                                                                   */
/*----------------------------------------------------------------------------*/

static gboolean handle_button_pressed (GtkWidget *btn, GdkEventButton *, gpointer userdata)
{
    LauncherPlugin *lch = (LauncherPlugin *) userdata;
    lch->dragbtn = btn;
    pressed = PRESS_NONE;
    return FALSE;
}

static gboolean handle_button_release (GtkWidget *btn, GdkEventButton *event, gpointer userdata)
{
    LauncherPlugin *lch = (LauncherPlugin *) userdata;

    if (lch->dragon)
    {
        lch->dragon = FALSE;
        return FALSE;
    }

    if (pressed == PRESS_LONG) return FALSE;

    switch (event->button)
    {
        case 1:     launch_id (btn);
                    return FALSE;

        case 3:     popup_menu (btn);
                    return TRUE;
    }

    return FALSE;
}

static void handle_gesture_end (GtkGestureLongPress *, GdkEventSequence *, gpointer userdata)
{
    LauncherPlugin *lch = (LauncherPlugin *) userdata;

    if (lch->dragon) return;

    if (pressed == PRESS_LONG)
    {
        popup_menu (lch->dragbtn);
    }
}

static void handle_drag_begin (GtkGestureDrag *, gdouble x, gdouble, gpointer userdata)
{
    LauncherPlugin *lch = (LauncherPlugin *) userdata;
    lch->drag_start = x;
}

static void handle_drag_update (GtkGestureDrag *, gdouble x, gdouble, gpointer userdata)
{
    LauncherPlugin *lch = (LauncherPlugin *) userdata;
    GtkStyleContext *sc;
    GList *children, *index;
    int moveby;

    if (!lch->dragon && x < DRAG_THRESH && x > -DRAG_THRESH) return;

    lch->dragon = TRUE;
    pressed = PRESS_NONE;
    gdk_window_set_cursor (gtk_widget_get_window (lch->plugin), lch->drag);
    sc = gtk_widget_get_style_context (lch->dragbtn);
    gtk_style_context_add_class (sc, "drag");

    moveby = 0;
    if (lch->drag_start + x < -DRAG_THRESH) moveby = -1;
    if (lch->drag_start + x > get_icon_size () + DRAG_THRESH) moveby = 1;
    if (!moveby) return;

    children = gtk_container_get_children (GTK_CONTAINER (lch->plugin));
    index = children;
    while (index)
    {
        if (index->data == lch->dragbtn) break;
        moveby++;
        index = index->next;
    }
    g_list_free (children);

    if (moveby >= 0) gtk_box_reorder_child (GTK_BOX (lch->plugin), lch->dragbtn, moveby);

    gtk_widget_queue_allocate (lch->plugin);
}

static void handle_drag_end (GtkGestureDrag *, gdouble, gdouble, gpointer userdata)
{
    LauncherPlugin *lch = (LauncherPlugin *) userdata;
    GtkStyleContext *sc;
    GList *children, *index;
    char *launchers = NULL, *name, *tmp;

    children = gtk_container_get_children (GTK_CONTAINER (lch->plugin));
    index = children;
    while (index)
    {
        name = g_strdup (gtk_widget_get_name (GTK_WIDGET (index->data)));
        *strrchr (name, '.') = 0;
        tmp = g_strdup_printf ("%s%s ", launchers ? launchers : "", name);
        g_free (name);
        g_free (launchers);
        launchers = tmp;
        index = index->next;
    }
    g_list_free (children);

    replace_launchers (launchers);
    g_free (launchers);

    lch->dragon = FALSE;
    gdk_window_set_cursor (gtk_widget_get_window (lch->plugin), NULL);
    sc = gtk_widget_get_style_context (lch->dragbtn);
    gtk_style_context_remove_class (sc, "drag");
}

/*----------------------------------------------------------------------------*/
/* wf-panel plugin functions                                                  */
/*----------------------------------------------------------------------------*/

/* Handler for system config changed message from panel */
void launcher_update_display (LauncherPlugin *lch)
{
    update_icons (lch);
}

/* Handler for control message */
gboolean launcher_control_msg (LauncherPlugin *, const char *)
{
    return FALSE;
}

void launcher_init (LauncherPlugin *lch)
{
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

    /* Set up variables */
    lch->dragon = FALSE;
    gtk_box_set_spacing (GTK_BOX (lch->plugin), lch->spacing);
    lch->drag = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_HAND1);
    
    /* Load the launchers */
    load_launchers (lch);
}

void launcher_destructor (gpointer user_data)
{
    LauncherPlugin *lch = (LauncherPlugin *) user_data;

    /* Deallocate memory */
    if (lch->launchers) g_free (lch->launchers);
    lch->launchers = NULL;
    if (lch->drag) g_object_unref (lch->drag);
    lch->drag = NULL;

    g_free (lch);
}

/* End of file */
/*----------------------------------------------------------------------------*/
