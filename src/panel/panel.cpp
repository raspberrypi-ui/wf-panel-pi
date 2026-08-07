/*============================================================================
Copyright (c) 2026 Raspberry Pi
All rights reserved.

Some code based on the wf-shell project copyright (c) 2018 Ilia Bozhinov

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

#include <dlfcn.h>
#include <fnmatch.h>

#include <cstdlib>
#include <cstring>

#include <glibmm/main.h>

#include <gtk-layer-shell.h>

#include "widget.hpp"

extern "C" {
#include "plug_conf.h"
#include "lxutils.h"
}

#include "panel.hpp"

Panel::Panel (bool is_dock)
{
    dock = is_dock;

    // Load configuration files
    load_config ();

    // Check for running on a Pi
    if (!access ("/boot/firmware/config.txt", R_OK)) is_pi_var = TRUE;
    else is_pi_var = FALSE;

    // Check for running under wizard
    if (!g_strcmp0 (getenv ("USER"), "rpi-first-boot-wizard")) wizard = true;
    else wizard = false;

    // Create the window
    window = std::make_unique <AutohidingWindow> (dock);

    // GTK settings for window
    window->set_name (dock ? "DockToplevel" : "PanelToplevel");
    grid.set_name ("grid");

    // Set the icon size data pointer
    g_object_set_data ((GObject *) window->gobj (), "icon-size", &icon_size);

    // Connect to draw signal to log first draw event using journald only if RPI_LOG_FIRST_DRAW is set
    const char *rpi_log_env = std::getenv("RPI_LOG_FIRST_DRAW");
    if (rpi_log_env && (std::strcmp(rpi_log_env, "1") == 0 || std::strcmp(rpi_log_env, "true") == 0 ||
        std::strcmp(rpi_log_env, "yes") == 0 || std::strcmp(rpi_log_env, "on") == 0))
    {
        draw_connection = window->signal_draw ().connect ([=] (const Cairo::RefPtr<Cairo::Context> &cr) -> bool
        {
            // Log first draw event directly to journald with minimal information
            GLogField fields[] = {
                {"MESSAGE", "Panel first draw event", -1},
                {"PRIORITY", "5", -1}, // Notice level
                {"SYSLOG_IDENTIFIER", "wf-panel-pi", -1}};
            g_log_writer_journald (G_LOG_LEVEL_MESSAGE, fields, 3, NULL);

            // Disconnect after first draw
            draw_connection.disconnect ();
            // Return false to propagate the event further
            return false;
        });
    }

    // Monitor the draw signal to detect changes in scaling and reload icons if detected
    pending_update = false;
    scaling = window->get_scale_factor ();
    window->signal_draw ().connect ([=] (const Cairo::RefPtr<Cairo::Context> &cr) -> bool
    {
        if (pending_update) return false;
        int scale_now = window->get_scale_factor ();
        if (scaling != scale_now)
        {
            scaling = scale_now;
            update_widget_icons ();
        }

        if (dock && right_widgets.size ())
        {
            // organise the dock tray widgets so the bottom is never wider than the top, and the overall width is as narrow as possible...
            Gtk::Requisition min, pref;
            Gtk::Widget *plugin;
            int top, last, btm;
            gboolean split;

            while (1)
            {
                top = 0;
                btm = 0;
                last = 0;
                split = FALSE;

                for (auto &w : right_box.get_children ())
                {
                    if (!g_strcmp0 (w->get_name ().c_str(), "split")) split = TRUE;
                    w->get_preferred_size (min, pref);
                    last = pref.width;
                    top += pref.width;
                }

                if (split)
                {
                    while (1)
                    {
                        plugin = right_box.get_children ().back ();
                        if (!g_strcmp0 (plugin->get_name ().c_str (), "split")) break;
                        right_box.remove (*plugin);
                        right2_box.pack_start (*plugin, false, false);
                        right2_box.reorder_child (*plugin, 0);
                    }
                    break;
                }

                for (auto &w : right2_box.get_children ())
                {
                    w->get_preferred_size (min, pref);
                    btm += pref.width;
                }

                if (btm == 0 && top == 0) break;

                if (btm > top)
                {
                    // move up
                    plugin = right2_box.get_children ().front ();
                    right2_box.remove (*plugin);
                    right_box.pack_end (*plugin, false, false);
                    right_box.reorder_child (*plugin, 0);
                }
                else if (top - last >= btm + last)
                {
                    // move down
                    plugin = right_box.get_children ().back ();
                    right_box.remove (*plugin);
                    right2_box.pack_start (*plugin, false, false);
                    right2_box.reorder_child (*plugin, 0);
                }
                else break;
            }
        }

        return false;
    });

    // Create window menu
    cplug.set_label (_("Configure Plugin..."));
    cplug.signal_activate ().connect (sigc::mem_fun (this, &Panel::do_plugin_configure));
    menu.attach (cplug, 0, 1, 0, 1);

    menu.attach (sep, 0, 1, 1, 2);

    conf.set_label (_("Add / Remove Plugins..."));
    conf.signal_activate ().connect (sigc::mem_fun (this, &Panel::do_configure));
    menu.attach (conf, 0, 1, 2, 3);

    notif.set_label (_("Notifications..."));
    notif.signal_activate ().connect (sigc::mem_fun (this, &Panel::do_notify_configure));
    menu.attach (notif, 0, 1, 3, 4);

    appset.set_label (dock ? _("Dock Preferences...") : _("Taskbar Preferences..."));
    appset.signal_activate ().connect (sigc::mem_fun (this, &Panel::do_appearance_set));
    menu.attach (appset, 0, 1, 4, 5);

    menu.attach_to_widget (*window);
    menu.show_all ();

    // Set up window event handlers
    window->signal_button_press_event ().connect (sigc::mem_fun (this, &Panel::on_button_press_event));
    window->signal_button_release_event ().connect (sigc::mem_fun (this, &Panel::on_button_release_event));
    window->signal_key_press_event ().connect (sigc::mem_fun (this, &Panel::on_keypress_event));
    window->signal_delete_event ().connect (sigc::mem_fun (this, &Panel::on_delete));

    // Set up long press handler
    gesture = Gtk::GestureLongPress::create (*window);
    gesture->set_propagation_phase (Gtk::PHASE_BUBBLE);
    gesture->signal_pressed ().connect ([=] (double x, double y) {pressed = PRESS_LONG; press_x = x; press_y = y;});
    gesture->signal_end ().connect ([=] (GdkEventSequence *) {if (pressed == PRESS_LONG) pass_right_click (GTK_WIDGET (window->gobj ()), press_x, press_y);});
    gesture->set_touch_only (gestures_touch_only);

    // Create the window
    content_box.pack_start (left_box, false, false);
    if (dock)
    {
        content_box.pack_end (grid, false, false);
        grid.pack_start (right_box, false, false);
        grid.pack_end (right2_box, false, false);
    }
    else content_box.pack_end (right_box, false, false);
    window->add (content_box);
    window->show_all ();

    // Set the window display options
    set_exclusive ();

    // Setup notifications
    init_notify ();

    // Load widgets
    init_widgets ();
    update_widget_icons ();
}

Panel::~Panel ()
{
    if (!dock) wfpanel_notify_close ();
}

// Load panel configuration or use defaults

#define CFG_WIDGETS 0x01
#define CFG_NOTIFY  0x02
#define CFG_ICONS   0x04
#define CFG_EXCL    0x08

unsigned char Panel::load_config ()
{
    unsigned char changes = 0;
    char *tmp;
    int val;
    
    get_config_string (dock ? "dock" : "panel", "widgets_left", &tmp, dock ? WIDGETS_LEFT_DOCK : (wizard ? WIDGETS_LEFT_WIZARD : WIDGETS_LEFT_PANEL));
    if (g_strcmp0 (tmp, left_widgets_opt.c_str ()))
    {
        left_widgets_opt = tmp;
        changes |= CFG_WIDGETS | CFG_EXCL;
    }
    g_free (tmp);

    get_config_string (dock ? "dock" : "panel", "widgets_right", &tmp, dock ? WIDGETS_RIGHT_DOCK : (wizard ? WIDGETS_RIGHT_WIZARD : WIDGETS_RIGHT_PANEL));
    if (g_strcmp0 (tmp, right_widgets_opt.c_str ()))
    {
        right_widgets_opt = tmp;
        changes |= CFG_WIDGETS | CFG_EXCL;
    }
    g_free (tmp);

    val = get_config_int (dock ? "dock" : "panel", "icon_size", dock ? "48" : "32");
    if (icon_size != val)
    {
        icon_size = val;
        changes |= CFG_ICONS;
    }

    val = get_config_bool (dock ? "dock" : "panel", "exclusive", dock ? "false" : "true");
    if (exclusive != val)
    {
        exclusive = val;
        changes |= CFG_EXCL;
    }

    val = get_config_int ("notify", "timeout", "15");
    if (notify_timeout != val)
    {
        notify_timeout = val;
        changes |= CFG_NOTIFY;
    }
    val = get_config_bool ("notify", "enable", "true");
    if (notifications != val)
    {
        notifications = val;
        changes |= CFG_NOTIFY;
    }
    val = get_config_bool ("notify", "libnotify", "true");
    if (libnotify != val)
    {
        libnotify = val;
        changes |= CFG_NOTIFY;
    }

    gestures_touch_only = get_config_bool ("panel", "gestures_touch_only", "false");

    return changes;
}

// Set exclusive zone from the parameter value

void Panel::set_exclusive ()
{
    if (dock)
    {
        gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_LEFT, false);
        gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_RIGHT, false);
    }
    else
    {
        if (wizard)
        {
            gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_LEFT, false);
            gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_RIGHT, true);
        }
        else if (exclusive)
        {
            gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_LEFT, true);
            gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_RIGHT, true);
        }
        else
        {
            gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_LEFT, left_widgets.size () ? true : false);
            gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_RIGHT, right_widgets.size () ? true : false);
        }
    }
    window->set_auto_exclusive_zone (exclusive);
}

// Keyboard and mouse event handlers

bool Panel::on_keypress_event (GdkEventKey *event)
{
    char *str = g_strdup_printf ("key_%c", event->keyval);

    for (auto &w : left_widgets)
        if (w->widget_name == "smenu") w->widget_command (str);

    for (auto &w : right_widgets)
        if (w->widget_name == "smenu") w->widget_command (str);

    g_free (str);

    return false;
}

bool Panel::on_button_press_event (GdkEventButton *event)
{
    pressed = PRESS_SHORT;

    return false;
}

bool Panel::on_button_release_event (GdkEventButton *event)
{
    bool found = false;
    std::string pname;
    Gtk::Allocation alloc;
    char *title = NULL;

    if (pressed == PRESS_NONE) return false;
    pressed = PRESS_NONE;

    if (event->button == 3)
    {
        cplug.set_name ("gtkmm");
        cplug.hide ();
        sep.hide ();

        auto show_menu = [&] (Gtk::Widget *plugin)
        {
            if (plugin->is_visible ())
            {
                // check if the position of the mouse is within the plugin
                alloc = plugin->get_allocation ();
                if (event->x_root >= alloc.get_x () && event->x_root < alloc.get_x () + alloc.get_width () &&
                    event->y_root >= alloc.get_y () && event->y_root < alloc.get_y () + alloc.get_height ())
                {
                    pname = plugin->get_name ();
                    cplug.set_name (pname);
                    if (can_configure (pname.c_str (), &title) && pname != "spacing" && pname != "separator")
                    {
                        cplug.show ();
                        sep.show ();
                    }
                    cplug.set_sensitive (cdlg ? false : true);
                    if (title)
                    {
                        cplug.set_label (title);
                        g_free (title);
                    }
                    show_menu_with_kbd (GTK_WIDGET (plugin->gobj ()), GTK_WIDGET (menu.gobj ()), event);
                    found = true;
                }
            }
        };

        for (auto &plugin : left_box.get_children ())
            show_menu (plugin);

        for (auto &plugin : right_box.get_children ())
            show_menu (plugin);

        if (dock) for (auto &plugin : right2_box.get_children ())
            show_menu (plugin);

        // not matched any widgets - on the empty area of the bar...
        if (!found) show_menu_with_kbd_at_xy (GTK_WIDGET (window->gobj ()), GTK_WIDGET (menu.gobj ()), event);
    }
    return false;
}

// Window close handler

bool Panel::on_delete (GdkEventAny *ev)
{
    return true;
}

// Menu event handlers

void Panel::do_plugin_configure ()
{
    // defer opening the config dialog until the menu grab has released...
    Glib::signal_idle ().connect_once ([name = cplug.get_name ()] () { plugin_config_dialog (name.c_str ()); });
}

void Panel::do_configure ()
{
    if (dock) system ("rpcc widgets set_dock &");
    else system ("rpcc widgets set_bar &");
}

void Panel::do_notify_configure ()
{
    system ("rpcc notifications &");
}

void Panel::do_appearance_set ()
{
    if (dock) system ("rpcc dock &");
    else system ("rpcc taskbar &");
}

// Widget loading

std::unique_ptr <PanelWidget> Panel::widget_from_name (const char *name)
{
    if (strstr (name, "spacing"))
    {
        int width;
        if (sscanf (name + 7, "%d", &width) != 1 || width < 0) return nullptr;
        char *libname = g_strdup_printf (width ? PLUGIN_PATH "libspacing.so" : PLUGIN_PATH "libseparator.so");
        void *wid = dlopen (libname, RTLD_LAZY);
        g_free (libname);
        if (wid)
        {
            if (width)
            {
                PanelWidget *(*create_widget) (int) = (PanelWidget *(*) (int)) dlsym (wid, "create");
                return std::unique_ptr <PanelWidget> (create_widget (width));
            }
            else
            {
                PanelWidget *(*create_widget) () = (PanelWidget *(*) ()) dlsym (wid, "create");
                return std::unique_ptr <PanelWidget> (create_widget ());
            }
        }
    }

    if (g_strcmp0 (name, "none"))
    {
        char *libname = g_strdup_printf (PLUGIN_PATH "lib%s.so", name);
        void *wid = dlopen (libname, RTLD_LAZY);
        g_free (libname);
        if (wid)
        {
            PanelWidget *(*create_widget) () = (PanelWidget *(*) ()) dlsym (wid, "create");
            return std::unique_ptr <PanelWidget> (create_widget ());
        }
    }
    return nullptr;
}

void Panel::reload_widgets (std::string list, std::vector <std::unique_ptr <PanelWidget>>& container, Gtk::HBox& box)
{
    container.clear ();

    std::string widget_name;
    std::istringstream stream (list);
    while (stream >> widget_name)
    {
        auto widget = widget_from_name (widget_name.c_str ());
        if (!widget) continue;

        widget->widget_name = widget_name;
        widget->widget_init (&box);
        container.push_back (std::move (widget));

        // a badly-written widget could reset the textdomain to a local value - reset back to the system value after each load
        textdomain (GETTEXT_PACKAGE);
    }
}

void Panel::init_widgets ()
{
    reload_widgets ((std::string) left_widgets_opt, left_widgets, left_box);
    reload_widgets ((std::string) right_widgets_opt, right_widgets, right_box);
    if (((std::string) left_widgets_opt).empty () && ((std::string) right_widgets_opt).empty ()) window->hide ();
    else window->show ();
}

// Set up notifications and callbacks

void Panel::init_notify ()
{
    if (!dock) wfpanel_notify_init (notifications, libnotify, notify_timeout, window->gobj ());
}

// Update all displayed icons

void Panel::update_widget_icons ()
{
    for (auto &w : left_widgets)
        w->widget_set_icon ();

    for (auto &w : right_widgets)
        w->widget_set_icon ();

    window->update_position ();
}

// Public functions used by PanelApp

void Panel::handle_config_reload ()
{
    unsigned char changes = load_config ();

    if (changes & CFG_EXCL) set_exclusive ();
    if (changes & CFG_NOTIFY) init_notify ();
    if (changes & CFG_WIDGETS)
    {
        close_popup ();
        menu.popdown ();
        init_widgets ();
    }
    if (changes & CFG_ICONS || changes & CFG_WIDGETS) update_widget_icons ();

    window->handle_config_reload ();

    for (auto &w : left_widgets)
        w->widget_config_reload ();

    for (auto &w : right_widgets)
        w->widget_config_reload ();
}

void Panel::handle_command_message (const char *name, const char *cmd)
{
    if (!g_strcmp0 (name, "notify"))
    {
        wfpanel_notify (cmd);
        return;
    }

    if (!g_strcmp0 (name, "critical"))
    {
        wfpanel_critical (cmd);
        return;
    }

    if (!window->is_sensitive ()) return;

    for (auto &w : left_widgets)
        if (!fnmatch (name, w->widget_name.c_str (), 0)) w->widget_command (cmd);

    for (auto &w : right_widgets)
        if (!fnmatch (name, w->widget_name.c_str (), 0)) w->widget_command (cmd);
}

void Panel::monitor_update_pending (bool pend)
{
    pending_update = pend;
}

/* End of file */
/*----------------------------------------------------------------------------*/
