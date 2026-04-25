#include <stdio.h>
#include <dlfcn.h>
#include <sys/time.h>

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <map>

#include <gtk-layer-shell.h>

#include "panel.hpp"
#include "gtk-utils.hpp"
#include "spacer.hpp"

extern "C" {
#include "configure.h"
#include "lxutils.h"
}

Panel::Panel (WayfireOutput *output, bool real, bool dock) :
    icon_size {dock ? "dock/icon_size" : "panel/icon_size"},
    layer {dock ? "dock/layer" : "panel/layer"},
    monitor_num {dock ? "dock/monitor" : "panel/monitor"},
    left_widgets_opt {dock ? "dock/widgets_left" : "panel/widgets_left"},
    right_widgets_opt {"panel/widgets_right"},
    exclusive {dock ? "dock/exclusive" : "panel/exclusive"},
    minimal_panel_height {"panel/minimal_height"},
    gestures_touch_only {"panel/gestures_touch_only"},
    notify_timeout {"panel/notify_timeout"},
    notifications {"panel/notify_enable"},
    libnotify {"panel/notify_libnotify"}
{
    this->output = output;
    this->real = real;
    this->dock = dock;

    // Set C variables from parameters
    touch_only = gestures_touch_only;
    isize = icon_size;

    // Check for running on a Pi
    if (!access ("/boot/firmware/config.txt", R_OK)) is_pi_var = TRUE;
    else is_pi_var = FALSE;

    // Create the window
    window = std::make_unique <WayfireAutohidingWindow> (output, dock);

    // GTK settings for window
    window->set_size_request (1, real ? minimal_panel_height : 1);
    window->set_name (dock ? "DockToplevel" : "PanelToplevel");

    // Set the icon size data pointer
    g_object_set_data ((GObject *) window->gobj (), "icon-size", &isize);

    // Connect to draw signal to log first draw event using journald only if RPI_LOG_FIRST_DRAW is set
    const char *rpi_log_env = std::getenv("RPI_LOG_FIRST_DRAW");
    if (rpi_log_env && (std::strcmp(rpi_log_env, "1") == 0 ||
                        std::strcmp(rpi_log_env, "true") == 0 ||
                        std::strcmp(rpi_log_env, "yes") == 0 ||
                        std::strcmp(rpi_log_env, "on") == 0))
    {
        draw_connection = window->signal_draw ().connect (
            [this](const Cairo::RefPtr<Cairo::Context> &cr) -> bool
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
    window->signal_draw ().connect (
        [this](const Cairo::RefPtr<Cairo::Context> &cr) -> bool
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
    if (real)
    {
        icon_size.set_callback ([=] { update_widget_icons (); });
        exclusive.set_callback ([=] { set_exclusive (); });
        layer.set_callback ([=] { set_layer (); });
        monitor_num.set_callback ([=] { update_panels (); });
    }

    // Create the window
    content_box.pack_start (left_box, false, false);
    content_box.pack_end (right_box, false, false);
    window->add (content_box);
    left_box.show ();
    right_box.show ();
    content_box.show ();

    // Set the window display options
    set_layer ();
    set_exclusive ();

    // Load widgets
    init_widgets ();

    // Setup notifications
    init_notify ();

    // Show the window
    window->show_all ();
}

// Set the window layer from the parameter value

void Panel::set_layer ()
{
    if (!real) return;

    if ((std::string) layer == "overlay")
    {
        gtk_layer_set_layer (window->gobj (), GTK_LAYER_SHELL_LAYER_OVERLAY);
        store_layer (GTK_LAYER_SHELL_LAYER_OVERLAY, dock);
    }

    if ((std::string) layer == "top")
    {
        gtk_layer_set_layer (window->gobj (), GTK_LAYER_SHELL_LAYER_TOP);
        store_layer (GTK_LAYER_SHELL_LAYER_TOP, dock);
    }

    if ((std::string) layer == "bottom")
    {
        gtk_layer_set_layer (window->gobj (), GTK_LAYER_SHELL_LAYER_BOTTOM);
        store_layer (GTK_LAYER_SHELL_LAYER_BOTTOM, dock);
    }

    if ((std::string) layer == "background")
    {
        gtk_layer_set_layer (window->gobj (), GTK_LAYER_SHELL_LAYER_BACKGROUND);
        store_layer (GTK_LAYER_SHELL_LAYER_BACKGROUND, dock);
    }
}

// Set exclusive zone from the parameter value

void Panel::set_exclusive ()
{
    if (!real)
    {
        gtk_layer_set_layer (window->gobj (), GTK_LAYER_SHELL_LAYER_TOP);
        gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_LEFT, true);
        gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_RIGHT, false);
        gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_TOP, true);
        gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_BOTTOM, false);
        gtk_layer_set_margin (window->gobj (), GTK_LAYER_SHELL_EDGE_RIGHT, 1);
        gtk_layer_set_margin (window->gobj (), GTK_LAYER_SHELL_EDGE_BOTTOM, 1);
        window->set_auto_exclusive_zone (false);
    }
    else if (dock)
    {
        gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_LEFT, false);
        gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_RIGHT, false);
        window->set_auto_exclusive_zone (exclusive);
    }
    else
    {
        if (exclusive && !wizard)
        {
            gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_LEFT, true);
            gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_RIGHT, true);
            window->set_auto_exclusive_zone (true);
        }
        else
        {
            gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_LEFT, left_widgets.size () ? true : false);
            gtk_layer_set_anchor (window->gobj (), GTK_LAYER_SHELL_EDGE_RIGHT, right_widgets.size () ? true : false);
            window->set_auto_exclusive_zone (false);
        }
    }
}

// Keyboard and mouse event handlers

bool Panel::on_keypress_event (GdkEventKey* event)
{
    char *str = g_strdup_printf ("key_%c", event->keyval);

    for (auto& w : left_widgets)
        if (w->widget_name == "smenu") w->command (str);
    for (auto& w : right_widgets)
        if (w->widget_name == "smenu") w->command (str);
    g_free (str);

    return false;
}

bool Panel::on_button_press_event (GdkEventButton* event)
{
    pressed = PRESS_SHORT;

    return false;
}

bool Panel::on_button_release_event (GdkEventButton* event)
{
    if (pressed == PRESS_NONE) return false;
    pressed = PRESS_NONE;

    if (event->button == 3)
    {
        conf_plugin = "gtkmm";
        cplug.set_sensitive (false);

        int i;
        for (i = 0; i < 2; i++)
        {
            // loop through plugins in each hbox
            for (auto &plugin : (i == 0 ? left_box : right_box).get_children ())
            {
                if (!plugin->is_visible ()) continue;

                // check if the x position of the mouse is within the plugin
                Gtk::Allocation alloc = plugin->get_allocation ();

                if (event->x_root >= alloc.get_x () && event->x_root <= alloc.get_x () + alloc.get_width ())
                {
                    conf_plugin = plugin->get_name ();
                    if (conf_plugin == "spacing") cplug.hide ();
                    else cplug.show ();
                    if (can_configure (conf_plugin.c_str ())) cplug.set_sensitive (true);
                    show_menu_with_kbd (GTK_WIDGET (plugin->gobj ()), GTK_WIDGET (menu.gobj ()));
                    return false;
                }
            }
        }

        // not matched any widgets - on the empty area of the bar...
        cplug.hide ();
        show_menu_with_kbd_at_xy (GTK_WIDGET (window->gobj ()), GTK_WIDGET (menu.gobj ()), event->x_root, event->y_root);
    }
    return false;
}

// Window close handler

bool Panel::on_delete (GdkEventAny *ev)
{
    if (real && !dock) wfpanel_notify_close ();

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
    plugin_config_dialog (conf_plugin.c_str ());
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
    PanelApp::get ().rescan_xml_directory ();

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
    if (!real) return;

    if (wizard)
    {
        if (dock) window->hide ();
        else
        {
            reload_widgets ((std::string) "", left_widgets, left_box);
            reload_widgets ((std::string) "bluetooth volumepulse squeek", right_widgets, right_box);
            window->show ();
        }
        return;
    }

    if (dock)
    {
        reload_widgets ((std::string) left_widgets_opt, left_widgets, left_box);
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
    if (real && !dock) wfpanel_notify_init (notifications, libnotify, notify_timeout, window->gobj ());

    notifications.set_callback([=] ()
    {
        if (real && !dock) wfpanel_notify_init (notifications, libnotify, notify_timeout, window->gobj ());
    });

    libnotify.set_callback([=] ()
    {
        if (real && !dock) wfpanel_notify_init (notifications, libnotify, notify_timeout, window->gobj ());
    });

    notify_timeout.set_callback([=] ()
    {
        if (real && !dock) wfpanel_notify_init (notifications, libnotify, notify_timeout, window->gobj ());
    });
}

// Update all displayed icons

void Panel::update_widget_icons ()
{
    isize = icon_size;

    for (auto& w : left_widgets)
    {
        w->set_icon ();
    }

    for (auto& w : right_widgets)
    {
        w->set_icon ();
    }
}

// Update monitor assignments

void Panel::update_panels ()
{
    PanelApp::get ().update_panels ();
}

// Public functions used by PanelApp

void Panel::handle_config_reload ()
{
    for (auto& w : left_widgets)
    {
        w->handle_config_reload ();
    }

    for (auto& w : right_widgets)
    {
        w->handle_config_reload ();
    }
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

    for (auto& w : left_widgets)
    {
        if (name == w->widget_name) w->command (cmd);
    }

    for (auto& w : right_widgets)
    {
        if (name == w->widget_name) w->command (cmd);
    }
}

int Panel::set_monitor ()
{
    GdkDisplay *dpy = gdk_display_get_default ();
    GdkScreen *scr = gdk_display_get_default_screen (dpy);
    GdkMonitor *mon = NULL;
    int try_mon;
    const char *mnumstr = ((std::string) monitor_num).c_str();

    if (strlen (mnumstr) == 1 && sscanf (mnumstr, "%d", &try_mon) == 1)
    {
        // single digit - interpret as monitor number
        while (try_mon >= 0)
        {
            mon = gdk_display_get_monitor (dpy, try_mon);
            if (mon) break;
            try_mon--;
        }
    }
    else
    {
        // output name - try to match it to a connected monitor
        for (try_mon = gdk_display_get_n_monitors (dpy) - 1; try_mon >= 0; try_mon--)
        {
            mon = gdk_display_get_monitor (dpy, try_mon);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            char *mname = gdk_screen_get_monitor_plug_name (scr, try_mon);
#pragma GCC diagnostic pop
            if (!g_strcmp0 (mname, mnumstr) && mon)
            {
                g_free (mname);
                break;
            }
            g_free (mname);
        }
    }

    if (mon) gtk_layer_set_monitor (window->gobj(), mon);
    return try_mon >= 0 ? try_mon : 0;
}

