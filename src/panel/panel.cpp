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

#include <cstdlib>
#include <cstring>

#include <gtk-layer-shell.h>

#include "gtk-utils.hpp"
#include "spacer.hpp"

extern "C" {
#include "configure.h"
#include "lxutils.h"
}

#include "panel.hpp"

Panel::Panel (bool dock) :
    icon_size {dock ? "dock/icon_size" : "panel/icon_size"},
    left_widgets_opt {dock ? "dock/widgets_left" : "panel/widgets_left"},
    right_widgets_opt {dock ? "dock/widgets_right" : "panel/widgets_right"},
    right2_widgets_opt {"dock/widgets_right2"},
    exclusive {dock ? "dock/exclusive" : "panel/exclusive"},
    gestures_touch_only {"panel/gestures_touch_only"},
    notify_timeout {"panel/notify_timeout"},
    notifications {"panel/notify_enable"},
    libnotify {"panel/notify_libnotify"}
{
    this->dock = dock;

    // Set C variables from parameters
    touch_only = gestures_touch_only;
    isize = icon_size;

    // Check for running on a Pi
    if (!access ("/boot/firmware/config.txt", R_OK)) is_pi_var = TRUE;
    else is_pi_var = FALSE;

    // Create the window
    window = std::make_unique <WayfireAutohidingWindow> (dock);

    // GTK settings for window
    window->set_name (dock ? "DockToplevel" : "PanelToplevel");
    grid.set_name ("grid");

    // Set the icon size data pointer
    g_object_set_data ((GObject *) window->gobj (), "icon-size", &isize);

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
    scaling = window->get_scale_factor ();
    window->signal_draw ().connect ([=] (const Cairo::RefPtr<Cairo::Context> &cr) -> bool
    {
        int scale_now = window->get_scale_factor ();
        if (scaling != scale_now)
        {
            scaling = scale_now;
            update_widget_icons ();
        }
        set_exclusive ();
        return false;
    });

    // Create window menu
    conf.set_label (_("Add / Remove Plugins..."));
    conf.signal_activate ().connect (sigc::mem_fun (this, &Panel::do_configure));
    menu.attach (conf, 0, 1, 0, 1);

    cplug.set_label (_("Configure Plugin..."));
    cplug.signal_activate ().connect (sigc::mem_fun (this, &Panel::do_plugin_configure));
    menu.attach (cplug, 0, 1, 1, 2);

    notif.set_label (_("Notifications..."));
    notif.signal_activate ().connect (sigc::mem_fun (this, &Panel::do_notify_configure));
    menu.attach (notif, 0, 1, 2, 3);

    appset.set_label (dock ? _("Dock Preferences...") : _("Taskbar Preferences..."));
    appset.signal_activate ().connect (sigc::mem_fun (this, &Panel::do_appearance_set));
    menu.attach (appset, 0, 1, 3, 4);

    menu.attach_to_widget (*window);
    menu.show_all ();

    // Setup window event handlers
    window->signal_button_press_event ().connect (sigc::mem_fun (this, &Panel::on_button_press_event));
    window->signal_button_release_event ().connect (sigc::mem_fun (this, &Panel::on_button_release_event));
    window->signal_key_press_event ().connect (sigc::mem_fun (this, &Panel::on_keypress_event));
    window->signal_delete_event ().connect (sigc::mem_fun (this, &Panel::on_delete));
    gesture = add_longpress_default (*window);

    // Set up parameter callbacks
    icon_size.set_callback ([=] { update_widget_icons (); });
    gestures_touch_only.set_callback ([=] { update_gestures (); });
    exclusive.set_callback ([=] { set_exclusive (); });

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
}

Panel::~Panel ()
{
    if (!dock) wfpanel_notify_close ();
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
        if (exclusive)
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
        if (w->widget_name == "smenu") w->command (str);

    for (auto &w : right_widgets)
        if (w->widget_name == "smenu") w->command (str);

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

    if (pressed == PRESS_NONE) return false;
    pressed = PRESS_NONE;

    if (event->button == 3)
    {
        cplug.set_name ("gtkmm");
        cplug.set_sensitive (false);
        cplug.hide ();

        auto show_menu = [&] (Gtk::Widget *plugin)
        {
            if (plugin->is_visible ())
            {
                // check if the position of the mouse is within the plugin
                alloc = plugin->get_allocation ();
                if (event->x_root >= alloc.get_x () && event->x_root <= alloc.get_x () + alloc.get_width () &&
                    event->y_root >= alloc.get_y () && event->y_root <= alloc.get_y () + alloc.get_height ())
                {
                    pname = plugin->get_name ();
                    cplug.set_name (pname);
                    if (can_configure (pname.c_str ())) cplug.set_sensitive (true);
                    if (pname != "spacing") cplug.show ();
                    show_menu_with_kbd (GTK_WIDGET (plugin->gobj ()), GTK_WIDGET (menu.gobj ()));
                    found = true;
                }
            }
        };

        for (auto &plugin : left_box.get_children ())
            show_menu (plugin);

        for (auto &plugin : right_box.get_children ())
            show_menu (plugin);

        for (auto &plugin : right2_box.get_children ())
            show_menu (plugin);

        // not matched any widgets - on the empty area of the bar...
        if (!found) show_menu_with_kbd_at_xy (GTK_WIDGET (window->gobj ()), GTK_WIDGET (menu.gobj ()), event->x_root, event->y_root);
    }
    return false;
}

// Window close handler

bool Panel::on_delete (GdkEventAny *ev)
{
    return true;
}

// Menu event handlers

void Panel::do_configure ()
{
    window->set_sensitive (false);
    open_config_dialog ();
    window->set_sensitive (true);
}

void Panel::do_plugin_configure ()
{
    window->set_sensitive (false);
    plugin_config_dialog (cplug.get_name ().c_str ());
    window->set_sensitive (true);
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

std::unique_ptr <WayfireWidget> Panel::widget_from_name (const char *name)
{
    if (strstr (name, "spacing"))
    {
        int width;
        if (sscanf (name + 7, "%d", &width) != 1 || width < 0) return nullptr;
        else return std::unique_ptr <WayfireWidget> (new WayfireSpacing (width));
    }

    if (g_strcmp0 (name, "none"))
    {
        char *libname = g_strdup_printf (PLUGIN_PATH "lib%s.so", name);
        void *wid = dlopen (libname, RTLD_LAZY);
        g_free (libname);
        if (wid)
        {
            create_t *create_widget = (create_t *) dlsym (wid, "create");
            return std::unique_ptr <WayfireWidget> (create_widget ());
        }
    }
    return nullptr;
}

void Panel::reload_widgets (std::string list, std::vector <std::unique_ptr <WayfireWidget>>& container, Gtk::HBox& box)
{
    PanelApp::rescan_xml_directory ();

    container.clear ();

    std::string widget_name;
    std::istringstream stream (list);
    while (stream >> widget_name)
    {
        auto widget = widget_from_name (widget_name.c_str ());
        if (!widget) continue;

        widget->widget_name = widget_name;
        widget->init (&box);
        container.push_back (std::move (widget));

        // a badly-written widget could reset the textdomain to a local value - reset back to the system value after each load
        textdomain (GETTEXT_PACKAGE);
    }
    set_exclusive ();
}

void Panel::init_widgets ()
{
    if (dock)
    {
        reload_widgets ((std::string) left_widgets_opt, left_widgets, left_box);
        reload_widgets ((std::string) right_widgets_opt, right_widgets, right_box);
        reload_widgets ((std::string) right2_widgets_opt, right2_widgets, right2_box);
        if (((std::string) left_widgets_opt).empty ()) window->hide ();
        else window->show ();
    }
    else
    {
        reload_widgets ((std::string) left_widgets_opt, left_widgets, left_box);
        reload_widgets ((std::string) right_widgets_opt, right_widgets, right_box);
        if (((std::string) left_widgets_opt).empty () && ((std::string) right_widgets_opt).empty ()) window->hide ();
        else window->show ();
    }

    left_widgets_opt.set_callback ([=] ()
    {
        reload_widgets ((std::string) left_widgets_opt, left_widgets, left_box);
        if (((std::string) left_widgets_opt).empty () && (dock || ((std::string) right_widgets_opt).empty ())) window->hide ();
        else window->show ();
    });

    right_widgets_opt.set_callback ([=] ()
    {
        if (dock) return;
        reload_widgets ((std::string) right_widgets_opt, right_widgets, right_box);
        if (((std::string) left_widgets_opt).empty () && ((std::string) right_widgets_opt).empty ()) window->hide ();
        else window->show ();
    });
}

// Set up notifications and callbacks

void Panel::init_notify ()
{
    if (!dock) wfpanel_notify_init (notifications, libnotify, notify_timeout, window->gobj ());

    notifications.set_callback([=] ()
    {
        if (!dock) wfpanel_notify_init (notifications, libnotify, notify_timeout, window->gobj ());
    });

    libnotify.set_callback([=] ()
    {
        if (!dock) wfpanel_notify_init (notifications, libnotify, notify_timeout, window->gobj ());
    });

    notify_timeout.set_callback([=] ()
    {
        if (!dock) wfpanel_notify_init (notifications, libnotify, notify_timeout, window->gobj ());
    });
}

// Update all displayed icons

void Panel::update_widget_icons ()
{
    isize = icon_size;

    for (auto &w : left_widgets)
        w->set_icon ();

    for (auto &w : right_widgets)
        w->set_icon ();
}

void Panel::update_gestures ()
{
    touch_only = gestures_touch_only;
}

// Public functions used by PanelApp

void Panel::handle_config_reload ()
{
    for (auto &w : left_widgets)
        w->handle_config_reload ();

    for (auto &w : right_widgets)
        w->handle_config_reload ();
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
        if (name == w->widget_name) w->command (cmd);

    for (auto &w : right_widgets)
        if (name == w->widget_name) w->command (cmd);
}

/* End of file */
/*----------------------------------------------------------------------------*/
