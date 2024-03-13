extern "C" {
#include "configure.h"
}

#include <glibmm/main.h>
#include <gtkmm/window.h>
#include <gtkmm/headerbar.h>
#include <gtkmm/hvbox.h>
#include <gtkmm/application.h>
#include <gdkmm/display.h>
#include <gdkmm/seat.h>
#include <gdk/gdkwayland.h>
#include <gtk-layer-shell.h>

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <sys/time.h>

#include <map>

#include "panel.hpp"
#include "gtk-utils.hpp"

#include "widgets/clock.hpp"
#include "widgets/launchers.hpp"
#include "widgets/spacing.hpp"
#include "widgets/volumepulse.hpp"
#include "widgets/smenu.hpp"
#include "widgets/netman.hpp"
#include "widgets/bluetooth.hpp"
#include "widgets/ejecter.hpp"
#include "widgets/updater.hpp"
#include "widgets/cpu.hpp"
#include "widgets/cputemp.hpp"
#include "widgets/gpu.hpp"
#include "widgets/power.hpp"
#include "widgets/batt.hpp"
#include "widgets/notify.hpp"
#include "widgets/window-list/window-list.hpp"
#include "widgets/tray/tray.hpp"

#include "wf-autohide-window.hpp"

extern "C" {
#include "lxutils.h"
}


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


class WayfirePanel::impl
{
    std::unique_ptr<WayfireAutohidingWindow> window;

    Gtk::HBox content_box;
    Gtk::HBox left_box, center_box, right_box;
    Gtk::Menu menu;
    Gtk::MenuItem conf;
    Gtk::MenuItem cplug;
    Gtk::MenuItem notif;
    Gtk::MenuItem appset;
    std::string conf_plugin;

    using Widget = std::unique_ptr<WayfireWidget>;
    using WidgetContainer = std::vector<Widget>;
    WidgetContainer left_widgets, center_widgets, right_widgets;

    WayfireOutput *output;
    bool wizard = WayfireShellApp::get().wizard;
    bool real;
    WfOption <int> icon_size {"panel/icon_size"};

#if 0
    WfOption<std::string> bg_color{"panel/background_color"};
    std::function<void()> on_window_color_updated = [=] ()
    {
        if ((std::string)bg_color == "gtk_default")
        {
            return window->unset_background_color();
        }

        Gdk::RGBA rgba;
        if ((std::string)bg_color == "gtk_headerbar")
        {
            Gtk::HeaderBar headerbar;
            rgba = headerbar.get_style_context()->get_background_color();
        } else
        {
            auto color = wf::option_type::from_string<wf::color_t>(
                ((wf::option_sptr_t<std::string>)bg_color)->get_value_str());
            if (!color)
            {
                std::cerr << "Invalid panel background color in"
                             " config file" << std::endl;
                return;
            }

            rgba.set_red(color.value().r);
            rgba.set_green(color.value().g);
            rgba.set_blue(color.value().b);
            rgba.set_alpha(color.value().a);
        }

        window->override_background_color(rgba);
    };
#endif

    WfOption<std::string> panel_layer{"panel/layer"};
    std::function<void()> set_panel_layer = [=] ()
    {
        if ((std::string)panel_layer == "overlay")
        {
            gtk_layer_set_layer(window->gobj(), GTK_LAYER_SHELL_LAYER_OVERLAY);
            store_layer (GTK_LAYER_SHELL_LAYER_OVERLAY);
        }

        if ((std::string)panel_layer == "top")
        {
            gtk_layer_set_layer(window->gobj(), GTK_LAYER_SHELL_LAYER_TOP);
            store_layer (GTK_LAYER_SHELL_LAYER_TOP);
        }

        if ((std::string)panel_layer == "bottom")
        {
            gtk_layer_set_layer(window->gobj(), GTK_LAYER_SHELL_LAYER_BOTTOM);
            store_layer (GTK_LAYER_SHELL_LAYER_BOTTOM);
        }

        if ((std::string)panel_layer == "background")
        {
            gtk_layer_set_layer(window->gobj(), GTK_LAYER_SHELL_LAYER_BACKGROUND);
            store_layer (GTK_LAYER_SHELL_LAYER_BACKGROUND);
        }
    };

    WfOption<int> minimal_panel_height{"panel/minimal_height"};
    WfOption<std::string> css_path{"panel/css_path"};

    WfOption<std::string> monitor_num{"panel/monitor"};

    void create_window()
    {
        setlocale (LC_ALL, "");
        bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        window = std::make_unique<WayfireAutohidingWindow>(output, "panel");
        window->set_size_request(1, real ? minimal_panel_height : 1);
        if (real)
        {
            panel_layer.set_callback(set_panel_layer);
            set_panel_layer(); // initial setting
            lxpanel_notify_init (notifications, notify_timeout, window->gobj ());
        }

        gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_LEFT, wizard ? false : true);
        gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, true);
        gtk_layer_set_keyboard_mode (window->gobj(), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);

        if (!real)
        {
            gtk_layer_set_layer (window->gobj(), GTK_LAYER_SHELL_LAYER_TOP);
            gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_LEFT, true);
            gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, false);
            gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_TOP, true);
            gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, false);
            gtk_layer_set_margin(window->gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, 1);
            gtk_layer_set_margin(window->gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, 1);
        }

        monitor_num.set_callback (update_panels);

        window->set_name ("PanelToplevel");
#if 0
        bg_color.set_callback(on_window_color_updated);
        on_window_color_updated(); // set initial color
#endif

        if ((std::string)css_path != "")
        {
            auto css = load_css_from_path(css_path);
            if (css)
            {
                auto screen = Gdk::Screen::get_default();
                auto style_context = Gtk::StyleContext::create();
                style_context->add_provider_for_screen(
                    screen, css, GTK_STYLE_PROVIDER_PRIORITY_USER);
            }
        }

        conf.set_label (_("Add / Remove Plugins..."));
        conf.signal_activate().connect(sigc::mem_fun(this, &WayfirePanel::impl::do_configure));
        menu.attach (conf, 0, 1, 0, 1);

        cplug.set_label (_("Configure Plugin..."));
        cplug.signal_activate().connect(sigc::mem_fun(this, &WayfirePanel::impl::do_plugin_configure));
        menu.attach (cplug, 0, 1, 1, 2);

        notif.set_label (_("Notifications..."));
        notif.signal_activate().connect(sigc::mem_fun(this, &WayfirePanel::impl::do_notify_configure));
        menu.attach (notif, 0, 1, 2, 3);

        appset.set_label (_("Taskbar Preferences..."));
        appset.signal_activate().connect(sigc::mem_fun(this, &WayfirePanel::impl::do_appearance_set));
        menu.attach (appset, 0, 1, 3, 4);

        menu.attach_to_widget (*window);
        menu.show_all();

        window->signal_button_release_event().connect(sigc::mem_fun(this, &WayfirePanel::impl::on_button_release_event));

        if (wizard || !real)
        {
            window->set_auto_exclusive_zone (false);
            window->lock_autohide();
        }

        window->show_all();
        init_widgets();
        init_layout();

        window->signal_delete_event().connect(
            sigc::mem_fun(this, &WayfirePanel::impl::on_delete));
    }

    bool on_button_release_event(GdkEventButton* event)
    {
        if (!window->has_popover() && event->type == GDK_BUTTON_RELEASE && event->button == 3)
        {
            conf_plugin = "gtkmm";
            cplug.set_sensitive (false);

            // get mouse coords to parent window coords

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
                                int x, y;
                                Gdk::ModifierType mod;

                                if (!plugin->is_visible ()) continue;

                                // ignore task list (scrolled window) and spacers (boxes)
                                if (GTK_IS_SCROLLED_WINDOW (plugin->gobj()) || GTK_IS_BOX (plugin->gobj())) continue;

                                // check if the x position of the mouse is within the plugin
                                plugin->get_window()->get_device_position (Gdk::Display::get_default()->get_default_seat()->get_pointer (), x, y, mod);
                                Gtk::Allocation alloc = plugin->get_allocation ();

                                if (x >= alloc.get_x () && x <= alloc.get_x () + alloc.get_width())
                                {
                                    conf_plugin = plugin->get_name();
                                    if (conf_plugin.substr (0,5) != "gtkmm") cplug.set_sensitive (true);
                                    show_menu_with_kbd (GTK_WIDGET (plugin->gobj()), GTK_WIDGET (menu.gobj()));
                                    return true;
                                }
                            }
                        }
                    }
                    // not matched any widgets - on the empty area of the bar...
                    show_menu_with_kbd (GTK_WIDGET (window->gobj()), GTK_WIDGET (menu.gobj()));
                    return true;
                }
            }
        }
        return false;
    }

    void do_configure()
    {
        open_config_dialog ();
    }

    void do_plugin_configure()
    {
        if (conf_plugin.substr (0,5) != "gtkmm") plugin_config_dialog (conf_plugin.c_str());
    }

    void do_notify_configure()
    {
        plugin_config_dialog ("notify");
    }

    void do_appearance_set()
    {
        system ("pipanel -3 &");
    }

    bool on_delete(GdkEventAny *ev)
    {
        /* We ignore close events, because the panel's lifetime is bound to
         * the lifetime of the output */
        return true;
    }

    void init_layout()
    {
        content_box.pack_start(left_box, false, false);
        content_box.pack_end(right_box, false, false);
        if (!center_box.get_children().empty())
        {
            content_box.set_center_widget(center_box);
        }

        center_box.show_all();
        window->add(content_box);
        window->show_all();
    }

    Widget widget_from_name(std::string name)
    {
        if (name == "launchers")
        {
            return Widget(new WayfireLaunchers());
        }

        if (name == "clock")
        {
            return Widget(new WayfireClock());
        }

        if (name == "volumepulse")
            return Widget(new WayfireVolumepulse());
        if (name == "smenu")
            return Widget(new WayfireSmenu());
        if (name == "netman")
            return Widget(new WayfireNetman());
        if (name == "bluetooth")
            return Widget(new WayfireBluetooth());
        if (name == "ejecter")
            return Widget(new WayfireEjecter());
        if (name == "updater")
            return Widget(new WayfireUpdater());
        if (name == "cpu")
            return Widget(new WayfireCPU());
        if (name == "cputemp")
            return Widget(new WayfireCPUTemp());
        if (name == "gpu")
            return Widget(new WayfireGPU());
        if (name == "power")
            return Widget(new WayfirePower());
        if (name == "batt")
            return Widget(new WayfireBatt());

        if (name == "window-list")
        {
            return Widget(new WayfireWindowList(output));
        }

        if (name == "tray")
        {
            return Widget(new WayfireStatusNotifier());
        }

        std::string spacing = "spacing";
        if (name.find(spacing) == 0)
        {
            auto pixel_str = name.substr(spacing.size());
            int pixel = std::atoi(pixel_str.c_str());

            if (pixel <= 0)
            {
                std::cerr << "Invalid spacing, " << pixel << std::endl;
                return nullptr;
            }

            return Widget(new WayfireSpacing(pixel));
        }

        if (name != "none")
        {
            std::cerr << "Invalid widget: " << name << std::endl;
        }

        return nullptr;
    }

    static std::vector<std::string> tokenize(std::string list)
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

    void reload_widgets(std::string list, WidgetContainer& container,
        Gtk::HBox& box)
    {
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
        }
    }

    WfOption<std::string> left_widgets_opt{"panel/widgets_left"};
    WfOption<std::string> right_widgets_opt{"panel/widgets_right"};
    WfOption<std::string> center_widgets_opt{"panel/widgets_center"};
    void init_widgets()
    {
        if (!real) return;

        left_widgets_opt.set_callback([=] ()
        {
            reload_widgets((std::string)left_widgets_opt, left_widgets, left_box);
        });
        right_widgets_opt.set_callback([=] ()
        {
            reload_widgets((std::string)right_widgets_opt, right_widgets, right_box);
        });
        center_widgets_opt.set_callback([=] ()
        {
            reload_widgets((std::string)center_widgets_opt, center_widgets, center_box);
            if (center_box.get_children().empty())
            {
                content_box.unset_center_widget();
            } else
            {
                content_box.set_center_widget(center_box);
            }
        });

        if (wizard)
        {
            reload_widgets((std::string) "", left_widgets, left_box);
            reload_widgets((std::string) "bluetooth volumepulse", right_widgets, right_box);
            reload_widgets((std::string) "", center_widgets, center_box);
        }
        else
        {
            reload_widgets((std::string)left_widgets_opt, left_widgets, left_box);
            reload_widgets((std::string)right_widgets_opt, right_widgets, right_box);
            reload_widgets((std::string)center_widgets_opt, center_widgets, center_box);
        }
    }

    WfOption <int> notify_timeout {"panel/notify_timeout"};
    WfOption <bool> notifications {"panel/notify_enable"};

  public:
    impl(WayfireOutput *output, bool real) : output(output)
    {
        this->real = real;
        create_window();
    }

    wl_surface *get_wl_surface()
    {
        return window->get_wl_surface();
    }

    Gtk::Window& get_window()
    {
        return *window;
    }

    void handle_config_reload()
    {
        if (real) lxpanel_notify_init (notifications, notify_timeout, window->gobj ());
        for (auto& w : left_widgets)
        {
            w->handle_config_reload();
        }

        for (auto& w : right_widgets)
        {
            w->handle_config_reload();
        }

        for (auto& w : center_widgets)
        {
            w->handle_config_reload();
        }
    }

    void message_widget (const char *name, const char *cmd)
    {
        if (!g_strcmp0 (name, "notify"))
        {
            lxpanel_notify (cmd);
            return;
        }

        if (!g_strcmp0 (name, "critical"))
        {
            lxpanel_critical (cmd);
            return;
        }

        for (auto& w : left_widgets)
            if (name == w->widget_name) w->command (cmd);
        for (auto& w : right_widgets)
            if (name == w->widget_name) w->command (cmd);
        for (auto& w : center_widgets)
            if (name == w->widget_name) w->command (cmd);
    }

    WayfireOutput *get_output()
    {
        return this->output;
    }

    int set_monitor ()
    {
        GdkDisplay *dpy = gdk_display_get_default ();
        GdkScreen *scr = gdk_display_get_default_screen (dpy);
        GdkMonitor *mon;
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
                char *mname = gdk_screen_get_monitor_plug_name (scr, try_mon);
                if (!g_strcmp0 (mname, mnumstr) && mon)
                {
                    g_free (mname);
                    break;
                }
                g_free (mname);
            }
        }

        gtk_layer_set_monitor (window->gobj(), mon);
        return try_mon >= 0 ? try_mon : 0;
    }

    std::function<void()> update_panels = [=] ()
    {
        WayfirePanelApp::get().update_panels ();
    };
};

WayfirePanel::WayfirePanel(WayfireOutput *output, bool real) : pimpl(new impl(output, real))
{}
wl_surface*WayfirePanel::get_wl_surface()
{
    return pimpl->get_wl_surface();
}

Gtk::Window& WayfirePanel::get_window()
{
    return pimpl->get_window();
}

void WayfirePanel::handle_config_reload()
{
    return pimpl->handle_config_reload();
}
void WayfirePanel::handle_command_message (const char *plugin, const char *cmd) { pimpl->message_widget (plugin, cmd); }
int WayfirePanel::set_monitor () { return pimpl->set_monitor (); }
WayfireOutput* WayfirePanel::get_output () { return pimpl->get_output (); }

class WayfirePanelApp::impl
{
  public:
    std::unique_ptr<WayfirePanel> panel = NULL;
    std::vector<std::unique_ptr<WayfirePanel>> dummies;
    std::vector<WayfireOutput*> outputs;
};

void WayfirePanelApp::on_config_reload()
{
    if (priv->panel)
        priv->panel->handle_config_reload();
}

void WayfirePanelApp::on_command (const char *plugin, const char *command)
{
    if (priv->panel)
        priv->panel->handle_command_message (plugin, command);
}

void WayfirePanelApp::handle_new_output(WayfireOutput *output)
{
    priv->outputs.push_back (output);
    if (!priv->panel)
        priv->panel = std::unique_ptr<WayfirePanel> (new WayfirePanel (output, true));

    update_panels ();
}

void WayfirePanelApp::update_panels ()
{
    priv->dummies.clear ();

    int mon_num = priv->panel->set_monitor ();

    auto mon = Gdk::Display::get_default()->get_monitor (mon_num);
    for (auto& p : priv->outputs)
    {
        if (p->monitor != mon)
            priv->dummies.push_back (std::unique_ptr<WayfirePanel> (new WayfirePanel(p, false)));
    }
}

WayfirePanel* WayfirePanelApp::panel_for_wl_output(wl_output *output)
{
    for (auto& p : priv->dummies)
    {
        if (p.get()->get_output()->wo == output)
            return p.get();
    }
    return priv->panel.get();
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

const char *WayfirePanelApp::display_name (std::string type)
{
    if (type == "bluetooth") return WayfireBluetooth::display_name();
    if (type == "clock") return WayfireClock::display_name();
    if (type == "cpu") return WayfireCPU::display_name();
    if (type == "cputemp") return WayfireCPUTemp::display_name();
    if (type == "ejecter") return WayfireEjecter::display_name();
    if (type == "gpu") return WayfireGPU::display_name();
    if (type == "launchers") return WayfireLaunchers::display_name();
    if (type == "netman") return WayfireNetman::display_name();
    if (type == "power") return WayfirePower::display_name();
    if (type == "batt") return WayfireBatt::display_name();
    if (type == "smenu") return WayfireSmenu::display_name();
    if (type == "updater") return WayfireUpdater::display_name();
    if (type == "volumepulse") return WayfireVolumepulse::display_name();
    if (type == "window-list") return WayfireWindowList::display_name();
    if (type == "notify") return WayfireNotify::display_name();
    if (type == "tray") return WayfireStatusNotifier::display_name();
    return "<Unknown>";
}

const conf_table_t *WayfirePanelApp::config_params (std::string type)
{
    if (type == "bluetooth") return WayfireBluetooth::config_params();
    if (type == "clock") return WayfireClock::config_params();
    if (type == "cpu") return WayfireCPU::config_params();
    if (type == "cputemp") return WayfireCPUTemp::config_params();
    if (type == "ejecter") return WayfireEjecter::config_params();
    if (type == "gpu") return WayfireGPU::config_params();
    if (type == "launchers") return WayfireLaunchers::config_params();
    if (type == "netman") return WayfireNetman::config_params();
    if (type == "power") return WayfirePower::config_params();
    if (type == "batt") return WayfireBatt::config_params();
    if (type == "smenu") return WayfireSmenu::config_params();
    if (type == "updater") return WayfireUpdater::config_params();
    if (type == "volumepulse") return WayfireVolumepulse::config_params();
    if (type == "window-list") return WayfireWindowList::config_params();
    if (type == "notify") return WayfireNotify::config_params();
    if (type == "tray") return WayfireStatusNotifier::config_params();
    return NULL;
}

WayfirePanelApp::~WayfirePanelApp() = default;
WayfirePanelApp::WayfirePanelApp(int argc, char **argv) :
    WayfireShellApp(argc, argv), priv(new impl())
{}

int main(int argc, char **argv)
{
    WayfirePanelApp::create(argc, argv);
    return 0;
}
