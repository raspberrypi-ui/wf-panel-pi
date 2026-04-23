#include <stdio.h>
#include <dlfcn.h>
#include <sys/time.h>

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <map>

#include <gtk-layer-shell.h>

extern "C" {
#include "configure.h"
}

#include "panel.hpp"
#include "gtk-utils.hpp"
#include "spacer.hpp"

extern "C" {
#include "lxutils.h"

GtkWidget *wpanel, *wdock;
}

WayfirePanel::WayfirePanel (WayfireOutput *output, bool real, bool dock) :
    icon_size {dock ? "panel/dock_icon_size" : "panel/icon_size"},
    gestures_touch_only {"panel/gestures_touch_only"},
    layer {dock ? "panel/dock_layer" : "panel/layer"},
    minimal_panel_height {"panel/minimal_height"},
    monitor_num {dock ? "panel/dock_monitor" : "panel/monitor"},
    left_widgets_opt {dock ? "panel/dock_widgets" : "panel/widgets_left"},
    right_widgets_opt {"panel/widgets_right"},
    exclusive {dock ? "panel/dock_exclusive" : "panel/exclusive"},
    notify_timeout {"panel/notify_timeout"},
    notifications {"panel/notify_enable"},
    libnotify {"panel/notify_libnotify"}
{
    this->output = output;
    this->real = real;
    this->dock = dock;

    this->create_window();
}

void WayfirePanel::create_window()
{
    touch_only = gestures_touch_only;
    if (!access ("/boot/firmware/config.txt", R_OK)) is_pi_var = TRUE;
    else is_pi_var = FALSE;

    window = std::make_unique<WayfireAutohidingWindow>(output, "panel", dock);
    isize = icon_size;
    g_object_set_data ((GObject *) window->gobj(), "icon-size", &isize);
    if (dock) wdock = (GtkWidget *) window->gobj ();
    else wpanel = (GtkWidget *) window->gobj ();
    window->set_size_request(1, real ? minimal_panel_height : 1);
    if (real)
    {
        layer.set_callback([=] { set_layer (); });
        set_layer();
        if (!dock) wfpanel_notify_init (notifications, libnotify, notify_timeout, window->gobj ());
    }

    // Connect to draw signal to log first draw event using journald only if RPI_LOG_FIRST_DRAW is set
    const char *rpi_log_env = std::getenv("RPI_LOG_FIRST_DRAW");
    if (rpi_log_env && (std::strcmp(rpi_log_env, "1") == 0 ||
                        std::strcmp(rpi_log_env, "true") == 0 ||
                        std::strcmp(rpi_log_env, "yes") == 0 ||
                        std::strcmp(rpi_log_env, "on") == 0))
    {
        draw_connection = window->signal_draw().connect(
            [this](const Cairo::RefPtr<Cairo::Context> &cr) -> bool
            {
                // Log first draw event directly to journald with minimal information
                GLogField fields[] = {
                    {"MESSAGE", "Panel first draw event", -1},
                    {"PRIORITY", "5", -1}, // Notice level
                    {"SYSLOG_IDENTIFIER", "wf-panel-pi", -1}};
                g_log_writer_journald(G_LOG_LEVEL_MESSAGE, fields, 3, NULL);

                // Disconnect after first draw
                draw_connection.disconnect();
                // Return false to propagate the event further
                return false;
            });
    }

    // hang on the draw signal to detect changes in scaling and reload icons if detected
    scaling = window->get_scale_factor ();
    window->signal_draw().connect(
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

    gtk_layer_set_keyboard_mode (window->gobj(), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);

    monitor_num.set_callback ([=] { update_panels (); });

    window->set_name (dock ? "DockToplevel" : "PanelToplevel");

    conf.set_label (_("Add / Remove Plugins..."));
    conf.signal_activate().connect(sigc::mem_fun(this, &WayfirePanel::do_configure));
    menu.attach (conf, 0, 1, 0, 1);

    cplug.set_label (_("Configure Plugin..."));
    cplug.signal_activate().connect(sigc::mem_fun(this, &WayfirePanel::do_plugin_configure));
    menu.attach (cplug, 0, 1, 1, 2);

    notif.set_label (_("Notifications..."));
    notif.signal_activate().connect(sigc::mem_fun(this, &WayfirePanel::do_notify_configure));
    menu.attach (notif, 0, 1, 2, 3);

    appset.set_label (dock ? _("Dock Preferences...") : _("Taskbar Preferences..."));
    appset.signal_activate().connect(sigc::mem_fun(this, &WayfirePanel::do_appearance_set));
    menu.attach (appset, 0, 1, 3, 4);

    menu.attach_to_widget (*window);
    menu.show_all();

    window->signal_button_press_event().connect(sigc::mem_fun(this, &WayfirePanel::on_button_press_event));
    window->signal_button_release_event().connect(sigc::mem_fun(this, &WayfirePanel::on_button_release_event));

    window->signal_key_press_event().connect(sigc::mem_fun(this, &WayfirePanel::on_keypress_event));

    icon_size.set_callback ([=] { update_widget_icons (); });

    gesture = add_longpress_default (*window);

    exclusive.set_callback([=] { set_exclusive (); });

    window->show_all();
    init_layout();
    init_widgets();
    init_notify();

    window->signal_delete_event().connect(sigc::mem_fun(this, &WayfirePanel::on_delete));
}

void WayfirePanel::set_layer()
{
    if ((std::string) (layer) == "overlay")
    {
        gtk_layer_set_layer(window->gobj(), GTK_LAYER_SHELL_LAYER_OVERLAY);
        store_layer (GTK_LAYER_SHELL_LAYER_OVERLAY, dock);
    }

    if ((std::string) (layer) == "top")
    {
        gtk_layer_set_layer(window->gobj(), GTK_LAYER_SHELL_LAYER_TOP);
        store_layer (GTK_LAYER_SHELL_LAYER_TOP, dock);
    }

    if ((std::string) (layer) == "bottom")
    {
        gtk_layer_set_layer(window->gobj(), GTK_LAYER_SHELL_LAYER_BOTTOM);
        store_layer (GTK_LAYER_SHELL_LAYER_BOTTOM, dock);
    }

    if ((std::string) (layer) == "background")
    {
        gtk_layer_set_layer(window->gobj(), GTK_LAYER_SHELL_LAYER_BACKGROUND);
        store_layer (GTK_LAYER_SHELL_LAYER_BACKGROUND, dock);
    }
}

void WayfirePanel::set_exclusive()
{
    if (!real)
    {
        gtk_layer_set_layer (window->gobj(), GTK_LAYER_SHELL_LAYER_TOP);
        gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_LEFT, true);
        gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, false);
        gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);
        gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, false);
        gtk_layer_set_margin(window->gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, 1);
        gtk_layer_set_margin(window->gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, 1);
        window->set_auto_exclusive_zone (false);
    }
    else if (dock)
    {
        gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_LEFT, false);
        gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, false);
        window->set_auto_exclusive_zone (exclusive);
    }
    else
    {
        if (exclusive && !wizard)
        {
            gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_LEFT, true);
            gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, true);
            window->set_auto_exclusive_zone (true);
        }
        else
        {
            gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_LEFT, left_widgets.size () ? true : false);
            gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, right_widgets.size () ? true : false);
            window->set_auto_exclusive_zone (false);
        }
    }
}

bool WayfirePanel::on_keypress_event (GdkEventKey* event)
{
    char *str = g_strdup_printf ("key_%c", event->keyval);
    for (auto& w : left_widgets)
        if (w->widget_name == "smenu") w->command (str);
    for (auto& w : right_widgets)
        if (w->widget_name == "smenu") w->command (str);
    g_free (str);
    return false;
}

bool WayfirePanel::on_button_press_event(GdkEventButton* event)
{
    pressed = PRESS_SHORT;
    return false;
}

bool WayfirePanel::on_button_release_event(GdkEventButton* event)
{
    if (pressed == PRESS_NONE) return false;
    pressed = PRESS_NONE;
    if (event->button == 3)
    {
        conf_plugin = "gtkmm";
        cplug.set_sensitive (false);

        // child of window is first hbox
        std::vector<Gtk::Widget*> winch = window->get_children ();
        for (auto &tophbox : winch)
        {
            if (auto ctophbox = dynamic_cast<Gtk::Container*> (tophbox))
            {
                // top hbox has two hboxes as children - loop through both
                std::vector<Gtk::Widget*> hboxes = ctophbox->get_children ();
                for (auto &hbox : hboxes)
                {
                    if (auto chbox = dynamic_cast<Gtk::Container*> (hbox))
                    {
                        // loop through plugins in each hbox
                        std::vector<Gtk::Widget*> plugins = chbox->get_children ();
                        for (auto &plugin : plugins)
                        {
                            if (!plugin->is_visible ()) continue;

                            // check if the x position of the mouse is within the plugin
                            Gtk::Allocation alloc = plugin->get_allocation ();

                            if (event->x_root >= alloc.get_x () && event->x_root <= alloc.get_x () + alloc.get_width ())
                            {
                                conf_plugin = plugin->get_name();
                                if (conf_plugin == "spacing") cplug.hide ();
                                else cplug.show ();
                                if (can_configure (conf_plugin.c_str())) cplug.set_sensitive (true);
                                show_menu_with_kbd (GTK_WIDGET (plugin->gobj()), GTK_WIDGET (menu.gobj()));
                                return false;
                            }
                        }
                    }
                }
                // not matched any widgets - on the empty area of the bar...
                cplug.hide ();
                show_menu_with_kbd_at_xy (GTK_WIDGET (window->gobj()), GTK_WIDGET (menu.gobj()), event->x_root, event->y_root);
            }
        }
    }
    return false;
}

void WayfirePanel::do_configure()
{
    this->get_window ().set_sensitive (false);
    open_config_dialog ();
    this->get_window ().set_sensitive (true);
}

void WayfirePanel::do_plugin_configure()
{
    this->get_window ().set_sensitive (false);
    plugin_config_dialog (conf_plugin.c_str());
    this->get_window ().set_sensitive (true);
}

void WayfirePanel::do_notify_configure()
{
    system ("rpcc notifications &");
}

void WayfirePanel::do_appearance_set()
{
    if (dock) system ("rpcc dock &");
    else system ("rpcc taskbar &");
}

bool WayfirePanel::on_delete(GdkEventAny *ev)
{
    /* We ignore close events, because the panel's lifetime is bound to
     * the lifetime of the output */
    if (real && !dock) wfpanel_notify_close ();
    return true;
}

void WayfirePanel::init_layout()
{
    content_box.pack_start(left_box, false, false);
    content_box.pack_end(right_box, false, false);
    window->add(content_box);
    left_box.show();
    right_box.show();
    content_box.show();
    window->show();
    set_exclusive ();
}

std::unique_ptr<WayfireWidget> WayfirePanel::widget_from_name(std::string name)
{
    std::string spacing = "spacing";
    if (name.find(spacing) == 0)
    {
        auto pixel_str = name.substr(spacing.size());
        int pixel = std::atoi(pixel_str.c_str());

        if (pixel < 0)
        {
            std::cerr << "Invalid spacing, " << pixel << std::endl;
            return nullptr;
        }
        try
        {
            return std::unique_ptr<WayfireWidget>(new WayfireSpacing(pixel));
        }
        catch (...)
        {
            return nullptr;
        }
    }

    if (name != "none")
    {
        char *libname = g_strdup_printf (PLUGIN_PATH "lib%s.so", name.c_str());
        void *wid = dlopen (libname, RTLD_LAZY);
        g_free (libname);
        if (wid)
        {
            create_t *create_widget = (create_t *) dlsym (wid, "create");
            try
            {
                return std::unique_ptr<WayfireWidget> (create_widget ());
            }
            catch (...)
            {
                return nullptr;
            }
        }
        else std::cerr << "Could not open plugin - " << dlerror () << std::endl;
    }
    return nullptr;
}

std::vector<std::string> WayfirePanel::tokenize(std::string list)
{
    std::string token;
    std::istringstream stream(list);
    std::vector<std::string> result;

    while (stream >> token)
    {
        if (token.size())
        {
            result.push_back(token);
        }
    }

    return result;
}

void WayfirePanel::reload_widgets(std::string list, std::vector<std::unique_ptr<WayfireWidget>>& container, Gtk::HBox& box)
{
    WayfirePanelApp::get().rescan_xml_directory ();
    container.clear();
    auto widgets = tokenize(list);
    for (auto widget_name : widgets)
    {
        auto widget = widget_from_name(widget_name);
        if (!widget)
        {
            continue;
        }

        widget->widget_name = widget_name;
        widget->init(&box);
        container.push_back(std::move(widget));

        // a badly-written widget could reset the textdomain to a local value - reset back to the system value after each load
        textdomain (GETTEXT_PACKAGE);
    }
    set_exclusive ();
}

void WayfirePanel::init_widgets()
{
    left_widgets_opt.set_callback([=] ()
    {
        reload_widgets((std::string)left_widgets_opt, left_widgets, left_box);
        if (((std::string) left_widgets_opt).empty () && (dock || ((std::string) right_widgets_opt).empty ())) window->hide ();
        else window->show ();
    });

    right_widgets_opt.set_callback([=] ()
    {
        if (dock) return;
        reload_widgets((std::string)right_widgets_opt, right_widgets, right_box);
        if (((std::string) left_widgets_opt).empty () && ((std::string) right_widgets_opt).empty ()) window->hide ();
        else window->show ();
    });

    if (!real) return;

    if (wizard)
    {
        if (dock) window->hide ();
        else
        {
            reload_widgets((std::string) "", left_widgets, left_box);
            reload_widgets((std::string) "bluetooth volumepulse squeek", right_widgets, right_box);
            window->show ();
        }
        return;
    }

    if (dock)
    {
        reload_widgets((std::string)left_widgets_opt, left_widgets, left_box);
        if (((std::string) left_widgets_opt).empty ()) window->hide ();
        else window->show ();
    }
    else
    {
        reload_widgets((std::string)left_widgets_opt, left_widgets, left_box);
        reload_widgets((std::string)right_widgets_opt, right_widgets, right_box);
        if (((std::string) left_widgets_opt).empty () && ((std::string) right_widgets_opt).empty ()) window->hide ();
        else window->show ();
    }
}

void WayfirePanel::init_notify ()
{
    if (real) wfpanel_notify_init (notifications, libnotify, notify_timeout, window->gobj ());

    notifications.set_callback([=] ()
    {
        if (real) wfpanel_notify_init (notifications, libnotify, notify_timeout, window->gobj ());
    });

    libnotify.set_callback([=] ()
    {
        if (real) wfpanel_notify_init (notifications, libnotify, notify_timeout, window->gobj ());
    });

    notify_timeout.set_callback([=] ()
    {
        if (real) wfpanel_notify_init (notifications, libnotify, notify_timeout, window->gobj ());
    });
}

wl_surface *WayfirePanel::get_wl_surface()
{
    return window->get_wl_surface();
}

Gtk::Window& WayfirePanel::get_window()
{
    return *window;
}

void WayfirePanel::handle_config_reload()
{
    for (auto& w : left_widgets)
    {
        w->handle_config_reload();
    }

    for (auto& w : right_widgets)
    {
        w->handle_config_reload();
    }
}

void WayfirePanel::message_widget (const char *name, const char *cmd)
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

    if (!this->get_window().is_sensitive()) return;

    for (auto& w : left_widgets)
        if (name == w->widget_name) w->command (cmd);
    for (auto& w : right_widgets)
        if (name == w->widget_name) w->command (cmd);
}

WayfireOutput *WayfirePanel::get_output()
{
    return this->output;
}

int WayfirePanel::set_monitor ()
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

void WayfirePanel::update_panels ()
{
    WayfirePanelApp::get().update_panels ();
}

void WayfirePanel::update_widget_icons ()
{
    isize = icon_size;
    for (auto& w : left_widgets)
        w->set_icon ();
    for (auto& w : right_widgets)
        w->set_icon ();
}

void WayfirePanel::handle_command_message (const char *plugin, const char *cmd)
{
    message_widget (plugin, cmd);
}



class WayfirePanelApp::impl
{
  public:
    std::unique_ptr<WayfirePanel> panel = NULL;
    std::unique_ptr<WayfirePanel> dock = NULL;
    std::vector<std::unique_ptr<WayfirePanel>> dummies;
    std::vector<WayfireOutput*> outputs;
};

/* Minimal DBus interface for commands to plugins */

static GDBusNodeInfo *introspection_data = NULL;

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.wayfire.wfpanel'>"
  "    <annotation name='org.wayfire.wfpanel.Annotation' value='OnInterface'/>"
  "    <method name='command'>"
  "      <annotation name='org.wayfire.wfpanel.Annotation' value='OnMethod'/>"
  "      <arg type='s' name='plugin' direction='in'/>"
  "      <arg type='s' name='command' direction='in'/>"
  "    </method>"
  "  </interface>"
  "</node>";

const GDBusInterfaceVTable WayfirePanelApp::interface_vtable =
{
  handle_method_call,
  handle_get_property,
  handle_set_property,
  NULL
};

void WayfirePanelApp::handle_method_call (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
    const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data)
{
    if (g_strcmp0 (method_name, "command") == 0)
    {
        const gchar *plugin, *command;
        g_variant_get (parameters, "(&s&s)", &plugin, &command);
        get().on_command (plugin, command);
    }
}

GVariant *WayfirePanelApp::handle_get_property (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
    const gchar *property_name, GError **error, gpointer user_data)
{
    return NULL;
}

gboolean WayfirePanelApp::handle_set_property (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
    const gchar *property_name, GVariant *value, GError **error, gpointer user_data)
{
    return TRUE;
}

void WayfirePanelApp::on_bus_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
    g_dbus_connection_register_object (connection, "/org/wayfire/wfpanel", introspection_data->interfaces[0],
        &interface_vtable, user_data, NULL, NULL);
}

void WayfirePanelApp::on_name_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
}

void WayfirePanelApp::on_name_lost (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
}

void WayfirePanelApp::on_config_reload()
{
    if (priv->panel)
        priv->panel->handle_config_reload();
    if (priv->dock)
        priv->dock->handle_config_reload();
}

void WayfirePanelApp::on_command (const char *plugin, const char *command)
{
    if (priv->panel)
        priv->panel->handle_command_message (plugin, command);
    if (priv->dock)
        priv->dock->handle_command_message (plugin, command);
}

void WayfirePanelApp::handle_new_output(WayfireOutput *output)
{
    priv->outputs.push_back (output);
    if (!priv->panel)
    {
        priv->panel = std::make_unique<WayfirePanel> (output, true, false);
        priv->dock = std::make_unique<WayfirePanel> (output, true, true);
    }
    update_panels ();
}

void WayfirePanelApp::update_panels ()
{
    priv->dummies.clear ();

    int mon_num = priv->panel->set_monitor ();
    int dmon_num = priv->dock->set_monitor ();

    auto mon = Gdk::Display::get_default()->get_monitor (mon_num);
    auto dmon = Gdk::Display::get_default()->get_monitor (dmon_num);
    for (auto& p : priv->outputs)
    {
        if (p->monitor != mon && p->monitor != dmon)
            priv->dummies.push_back (std::make_unique<WayfirePanel> (p, false, false));
    }
}

void WayfirePanelApp::handle_output_removed(WayfireOutput *output)
{
    priv->outputs.erase (std::remove(priv->outputs.begin(), priv->outputs.end(), output), priv->outputs.end());
}

WayfirePanelApp& WayfirePanelApp::get()
{
    if (!instance)
    {
        throw std::logic_error("Calling WayfirePanelApp::get() before starting app!");
    }

    return dynamic_cast<WayfirePanelApp&>(*instance.get());
}

void WayfirePanelApp::create(int argc, char **argv)
{
    if (instance)
    {
        throw std::logic_error("Running WayfirePanelApp twice!");
    }

    introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
    guint owner_id = g_bus_own_name (G_BUS_TYPE_SESSION, "org.wayfire.wfpanel", G_BUS_NAME_OWNER_FLAGS_NONE,
        on_bus_acquired, on_name_acquired, on_name_lost, NULL, NULL);

    instance = std::unique_ptr<WayfireShellApp>(new WayfirePanelApp{argc, argv});
    instance->run();

    g_bus_unown_name (owner_id);
    g_dbus_node_info_unref (introspection_data);
}

WayfirePanelApp::~WayfirePanelApp() = default;
WayfirePanelApp::WayfirePanelApp(int argc, char **argv) :
    WayfireShellApp(argc, argv), priv(new impl())
{}

int main(int argc, char **argv)
{
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    WayfirePanelApp::create(argc, argv);
    return 0;
}
