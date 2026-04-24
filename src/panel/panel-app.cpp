#include <fcntl.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/time.h>
#include <glibmm/main.h>
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

const GDBusInterfaceVTable PanelApp::interface_vtable =
{
  handle_method_call,
  handle_get_property,
  handle_set_property,
  NULL
};


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
    app = Gtk::Application::create (argc, argv, "", Gio::APPLICATION_FLAGS_NONE);
    app->signal_activate ().connect_notify (sigc::mem_fun (this, &PanelApp::on_activate));
    app->activate ();
}

PanelApp::~PanelApp ()
{
    g_bus_unown_name (owner_id);
    g_dbus_node_info_unref (introspection_data);
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

    if (!g_strcmp0 (getenv ("USER"), "rpi-first-boot-wizard")) wizard = true;
    else wizard = false;

    // setup config file tracking
    char *dir = g_path_get_dirname (get_config_file ().c_str ());
    g_mkdir_with_parents (dir, S_IRUSR | S_IWUSR | S_IXUSR);
    g_free (dir);
    close (open (get_config_file ().c_str (), O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));

    inotify_fd = inotify_init ();
    Glib::signal_io ().connect (sigc::mem_fun (this, &PanelApp::handle_inotify_event), inotify_fd, Glib::IO_IN | Glib::IO_HUP);

    // load initial config
    std::vector <std::string> xmldirs (1, METADATA_DIR);
    config = wf::config::build_configuration (xmldirs, "/etc/xdg/wf-panel-pi/wf-panel-pi.ini", get_config_file ());
    do_reload_config ();

    // setup monitor tracking
    display->signal_monitor_added ().connect_notify ([=] (const GMonitor& monitor) { monitors_changed (); });
    display->signal_monitor_removed ().connect_notify ([=] (const GMonitor& monitor) { monitors_changed (); });

    // load initial monitors
    update_monitors ();
    
    // own on DBus
    introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
    owner_id = g_bus_own_name (G_BUS_TYPE_SESSION, "org.wayfire.wfpanel", G_BUS_NAME_OWNER_FLAGS_NONE,
        on_bus_acquired, on_name_acquired, on_name_lost, NULL, NULL);
}

/* Config file */

std::string PanelApp::get_config_file ()
{
    std::string config_dir;

    char *config_home = getenv ("XDG_CONFIG_HOME");

    if (config_home == NULL) config_dir = std::string (getenv ("HOME")) + "/.config";
    else config_dir = std::string (config_home);

    return config_dir + "/wf-panel-pi/wf-panel-pi.ini";
}

void PanelApp::do_reload_config ()
{
    char *dir;
    
    wf::config::load_configuration_options_from_file (get().config, get().get_config_file ());

    if (panel) panel->handle_config_reload ();
    if (dock) dock->handle_config_reload ();

    inotify_add_watch (get().inotify_fd, get().get_config_file ().c_str (), IN_MODIFY);
    dir = g_path_get_dirname (get().get_config_file ().c_str ());
    inotify_add_watch (get().inotify_fd, dir, IN_CREATE | IN_DELETE);
    g_free (dir);
}

bool PanelApp::handle_inotify_event (Glib::IOCondition cond)
{
    read (get().inotify_fd, buf, INOT_BUF_SIZE);
    do_reload_config ();
    return true;
}

void PanelApp::rescan_xml_directory (void)
{
    std::vector <std::string> xmldirs (1, METADATA_DIR);
    wf::config::reload_xml_files (config, xmldirs);
}

/* Monitor (output) tracking */

void PanelApp::monitors_changed ()
{
    if (hotplug_timer.connected ()) hotplug_timer.disconnect ();

    hotplug_timer = Glib::signal_timeout ().connect (sigc::mem_fun(this, &PanelApp::update_monitors), 500);
}

bool PanelApp::update_monitors ()
{
    // clear the existing monitors
    for (auto &mon : monitors) handle_output_removed (mon.get ());
    monitors.clear ();

    // find the new list of monitors
    auto display = Gdk::Display::get_default ();
    int num_monitors = display->get_n_monitors ();
    for (int i = 0; i < num_monitors; i++)
    {
        monitors.push_back (std::make_unique <WayfireOutput> (display->get_monitor (i)));
        handle_output_added (monitors.back ().get ());
    }

    return false;
}

void PanelApp::handle_output_added (WayfireOutput *output)
{
    outputs.push_back (output);

    if (!panel)
    {
        panel = std::make_unique <Panel> (output, true, false);
        dock = std::make_unique <Panel> (output, true, true);
    }

    dummies.clear ();

    int mon_num = panel->set_monitor ();
    int dmon_num = dock->set_monitor ();

    auto mon = Gdk::Display::get_default ()->get_monitor (mon_num);
    auto dmon = Gdk::Display::get_default ()->get_monitor (dmon_num);
    for (auto& p : outputs)
    {
        if (p->monitor != mon && p->monitor != dmon)
            dummies.push_back (std::make_unique <Panel> (p, false, false));
    }
}

void PanelApp::handle_output_removed (WayfireOutput *output)
{
    outputs.erase (std::remove (outputs.begin (), outputs.end (), output), outputs.end ());
}

/* DBus interface for commands to plugins */

void PanelApp::on_bus_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
    g_dbus_connection_register_object (connection, "/org/wayfire/wfpanel", introspection_data->interfaces[0],
        &interface_vtable, user_data, NULL, NULL);
}

void PanelApp::on_name_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
}

void PanelApp::on_name_lost (GDBusConnection *connection, const gchar *name, gpointer user_data)
{
}

void PanelApp::handle_method_call (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
    const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data)
{
    if (g_strcmp0 (method_name, "command") == 0)
    {
        const gchar *plugin, *command;
        g_variant_get (parameters, "(&s&s)", &plugin, &command);
        if (get().panel) get().panel->handle_command_message (plugin, command);
        if (get().dock) get().dock->handle_command_message (plugin, command);
    }
}

GVariant *PanelApp::handle_get_property (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
    const gchar *property_name, GError **error, gpointer user_data)
{
    return NULL;
}

gboolean PanelApp::handle_set_property (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
    const gchar *property_name, GVariant *value, GError **error, gpointer user_data)
{
    return TRUE;
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
