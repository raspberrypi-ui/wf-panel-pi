#include <fcntl.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/time.h>
#include <glibmm/main.h>
#include <giomm/dbusownname.h>
#include <giomm/dbusconnection.h>
#include <gdk/gdkwayland.h>

#include <iostream>
#include <memory>

extern "C" {
#include "launcher.h"
}

#include "config/file.hpp"
#include "panel.hpp"
#include "panel-app.hpp"

#define INOT_BUF_SIZE (1024 * sizeof (inotify_event))

char buf[INOT_BUF_SIZE];

// https://github.com/GNOME/glibmm/blob/master/examples/dbus/session_bus_service.cc

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

WayfireOutput::WayfireOutput (const GMonitor& monitor)
{
    this->monitor = monitor;
    this->wo = gdk_wayland_monitor_get_wl_output (monitor->gobj ());
}

WayfireOutput::~WayfireOutput ()
{
}


std::unique_ptr<PanelApp> PanelApp::instance;

PanelApp::PanelApp (int argc, char **argv)
{
    app = Gtk::Application::create (argc, argv, "com.raspberrypi.wf-panel-pi", Gio::APPLICATION_FLAGS_NONE);
    app->signal_activate ().connect_notify (sigc::mem_fun (this, &PanelApp::on_activate));
}

PanelApp::~PanelApp ()
{
    Gio::DBus::unown_name (owner_id);
}

PanelApp& PanelApp::get ()
{
    return *instance;
}

void PanelApp::create (int argc, char **argv)
{
    if (instance)
    {
        throw std::logic_error ("Running PanelApp twice!");
    }

    instance = std::unique_ptr <PanelApp> (new PanelApp {argc, argv});
    instance->run ();
}

void PanelApp::run ()
{
    app->run ();
}

void PanelApp::on_activate ()
{
    app->hold ();

    auto display = Gdk::Display::get_default ();
    auto wl_display = gdk_wayland_display_get_wl_display (display->gobj ());
    if (!wl_display)
    {
        std::cerr << "No Wayland display found" << std::endl;
        std::exit (-1);
    }

    // setup config file tracking
    char *dir = g_path_get_dirname (get_config_file ().c_str ());
    g_mkdir_with_parents (dir, S_IRUSR | S_IWUSR | S_IXUSR);
    g_free (dir);
    close (open (get_config_file ().c_str (), O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));

    inotify_fd = inotify_init ();
    Glib::signal_io ().connect (sigc::mem_fun (this, &PanelApp::handle_inotify_event), inotify_fd, Glib::IO_IN | Glib::IO_HUP);

    // load initial config
    std::vector <std::string> xmldirs (1, METADATA_DIR);
    if (!g_strcmp0 (getenv ("USER"), "rpi-first-boot-wizard"))
        config = wf::config::build_configuration (xmldirs, "/etc/xdg/wf-panel-pi/wizard.ini", get_config_file ());
    else
        config = wf::config::build_configuration (xmldirs, "/etc/xdg/wf-panel-pi/wf-panel-pi.ini", get_config_file ());
    do_reload_config ();

    // setup monitor tracking
    display->signal_monitor_added ().connect_notify ([=] (const GMonitor& monitor) { monitors_changed (); });
    display->signal_monitor_removed ().connect_notify ([=] (const GMonitor& monitor) { monitors_changed (); });

    // load initial monitors
    update_monitors ();
    
    // own on DBus
    introspection_data = Gio::DBus::NodeInfo::create_for_xml (introspection_xml);
    owner_id = Gio::DBus::own_name (Gio::DBus::BusType::BUS_TYPE_SESSION, "org.wayfire.wfpanel",
        sigc::mem_fun(this, &PanelApp::on_bus_acquired), sigc::mem_fun(this, &PanelApp::on_name_acquired), sigc::mem_fun(this, &PanelApp::on_name_lost));
}

/* Config file */

std::string PanelApp::get_config_file ()
{
    std::string config_dir;

    char *config_home = getenv ("XDG_CONFIG_HOME");

    if (config_home == NULL) config_dir = std::string (getenv ("HOME")) + "/.config";
    else config_dir = std::string (config_home);

    if (!g_strcmp0 (getenv ("USER"), "rpi-first-boot-wizard"))
        return config_dir + "/wf-panel-pi/wizard.ini";
    else
        return config_dir + "/wf-panel-pi/wf-panel-pi.ini";
}

void PanelApp::do_reload_config ()
{
    char *dir;
    
    wf::config::load_configuration_options_from_file (config, get_config_file ());

    if (panel) panel->handle_config_reload ();
    if (dock) dock->handle_config_reload ();

    inotify_add_watch (inotify_fd, get_config_file ().c_str (), IN_MODIFY);
    dir = g_path_get_dirname (get_config_file ().c_str ());
    inotify_add_watch (inotify_fd, dir, IN_CREATE | IN_DELETE);
    g_free (dir);
}

bool PanelApp::handle_inotify_event (Glib::IOCondition cond)
{
    read (inotify_fd, buf, INOT_BUF_SIZE);
    do_reload_config ();
    return true;
}

void PanelApp::rescan_xml_directory ()
{
    std::vector <std::string> xmldirs (1, METADATA_DIR);
    wf::config::reload_xml_files (config, xmldirs);
}

/* Monitor tracking */

void PanelApp::monitors_changed ()
{
    if (hotplug_timer.connected ()) hotplug_timer.disconnect ();

    hotplug_timer = Glib::signal_timeout ().connect (sigc::mem_fun (this, &PanelApp::update_monitors), 500);
}

bool PanelApp::update_monitors ()
{
    // clear the existing monitors
    monitors.clear ();

    // find the new list of monitors
    auto display = Gdk::Display::get_default ();
    int num_monitors = display->get_n_monitors ();
    for (int i = 0; i < num_monitors; i++)
    {
        monitors.push_back (std::make_unique <WayfireOutput> (display->get_monitor (i)));

        if (!panel)
        {
            panel = std::make_unique <Panel> (monitors.back ().get (), true, false);
            dock = std::make_unique <Panel> (monitors.back ().get (), true, true);
        }
    }

    update_panels ();

    return false;
}

void PanelApp::update_panels ()
{
    // update the dummy panels
    dummies.clear ();

    int pmon_num = panel->set_monitor ();
    int dmon_num = dock->set_monitor ();

    auto pmon = Gdk::Display::get_default ()->get_monitor (pmon_num);
    auto dmon = Gdk::Display::get_default ()->get_monitor (dmon_num);

    for (auto &mon : monitors)
    {
        if (mon.get ()->monitor != pmon && mon.get ()->monitor != dmon)
            dummies.push_back (std::make_unique <Panel> (mon.get (), false, false));
    }
}

/* DBus interface for commands to plugins */

void PanelApp::on_bus_acquired (const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring&)
{
    interface_vtable = new Gio::DBus::InterfaceVTable (sigc::mem_fun (this, &PanelApp::handle_method_call));
    connection->register_object ("/org/wayfire/wfpanel", introspection_data->lookup_interface(), *interface_vtable);
}

void PanelApp::on_name_acquired (const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring&)
{
}

void PanelApp::on_name_lost (const Glib::RefPtr<Gio::DBus::Connection>& connection, const Glib::ustring&)
{
}

void PanelApp::handle_method_call (const Glib::RefPtr< Gio::DBus::Connection > &, const Glib::ustring &, const Glib::ustring &, const Glib::ustring &, const Glib::ustring &method_name, const Glib::VariantContainerBase &parameters, const Glib::RefPtr< Gio::DBus::MethodInvocation > &)
{
    Glib::Variant <Glib::ustring> params;
    std::string plugin, command;

    if (method_name == "command")
    {
        parameters.get_child (params, 0);
        plugin = (std::string) params.get ();

        parameters.get_child (params, 1);
        command = params.get ();

        if (panel) panel->handle_command_message (plugin.c_str (), command.c_str ());
        if (dock) dock->handle_command_message (plugin.c_str (), command.c_str ());
    }
}

int main (int argc, char **argv)
{
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    PanelApp::create (argc, argv);
    return 0;
}
