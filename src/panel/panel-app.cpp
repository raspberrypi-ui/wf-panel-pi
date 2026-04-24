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

WayfireOutput::WayfireOutput (const GMonitor& monitor)
{
    this->monitor = monitor;
    this->wo = gdk_wayland_monitor_get_wl_output (monitor->gobj ());
}

WayfireOutput::~WayfireOutput ()
{
}


class PanelApp::impl
{
  public:
    std::unique_ptr <Panel> panel = NULL;
    std::unique_ptr <Panel> dock = NULL;
    std::vector <std::unique_ptr <Panel>> dummies;
    std::vector <WayfireOutput*> outputs;
};

std::unique_ptr<PanelApp> PanelApp::instance;

PanelApp::PanelApp (int argc, char **argv) : priv (new impl ())
{
    app = Gtk::Application::create (argc, argv, "", Gio::APPLICATION_HANDLES_COMMAND_LINE);
    app->signal_activate ().connect_notify (sigc::mem_fun (this, &PanelApp::on_activate));
    app->add_main_option_entry (sigc::mem_fun (this, &PanelApp::parse_cfgfile), "config", 'c', "config file to use", "file");

    // Activate app after parsing command line
    app->signal_command_line ().connect_notify ([=] (auto&) { app->activate (); });
}

PanelApp::~PanelApp ()
{
    g_bus_unown_name (owner_id);
    g_dbus_node_info_unref (introspection_data);
}

void PanelApp::run ()
{
    app->run ();
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

void PanelApp::on_activate ()
{
    app->hold ();

    auto gdk_display = gdk_display_get_default ();
    auto wl_display  = gdk_wayland_display_get_wl_display (gdk_display);
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

    std::vector <std::string> xmldirs (1, METADATA_DIR);
    config = wf::config::build_configuration (xmldirs, "/etc/xdg/wf-panel-pi/wf-panel-pi.ini", get_config_file ());
    do_reload_config ();

    // setup monitor tracking
    auto display = Gdk::Display::get_default ();
    display->signal_monitor_added ().connect_notify ([=] (const GMonitor& monitor) { monitors_changed (); });
    display->signal_monitor_removed ().connect_notify ([=] (const GMonitor& monitor) { monitors_changed (); });

    // initial monitors
    update_monitors ();
    
    // own on DBus
    introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
    owner_id = g_bus_own_name (G_BUS_TYPE_SESSION, "org.wayfire.wfpanel", G_BUS_NAME_OWNER_FLAGS_NONE,
        on_bus_acquired, on_name_acquired, on_name_lost, NULL, NULL);
}

/* Config file tracking */

std::string PanelApp::get_config_file ()
{
    std::string config_dir;

    if (cmdline_config.has_value ()) return cmdline_config.value ();

    char *config_home = getenv ("XDG_CONFIG_HOME");

    if (config_home == NULL) config_dir = std::string (getenv ("HOME")) + "/.config";
    else config_dir = std::string (config_home);

    return config_dir + "/wf-panel-pi/wf-panel-pi.ini";
}

bool PanelApp::parse_cfgfile (const Glib::ustring & option_name, const Glib::ustring & value, bool has_value)
{
    std::cout << "Using custom config file " << value << std::endl;
    cmdline_config = value;
    return true;
}

void PanelApp::do_reload_config ()
{
    char *dir = g_path_get_dirname (get ().get_config_file ().c_str ());
    wf::config::load_configuration_options_from_file (get ().config, get ().get_config_file ());
    get ().on_config_reload ();
    inotify_add_watch (get ().inotify_fd, get ().get_config_file ().c_str (), IN_MODIFY);
    inotify_add_watch (get ().inotify_fd, dir, IN_CREATE | IN_DELETE);
    g_free (dir);
}

bool PanelApp::handle_inotify_event (Glib::IOCondition cond)
{
    /* read, but don't use */
    read (get ().inotify_fd, buf, INOT_BUF_SIZE);
    do_reload_config ();
    return true;
}


void PanelApp::rescan_xml_directory (void)
{
    std::vector <std::string> xmldirs (1, METADATA_DIR);
    wf::config::reload_xml_files (this->config, xmldirs);
}

void PanelApp::on_config_reload ()
{
    if (priv->panel)
        priv->panel->handle_config_reload ();

    if (priv->dock)
        priv->dock->handle_config_reload ();
}

/* Monitor (output) tracking */

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
        handle_new_output (monitors.back ().get ());
    }

    return false;
}

void PanelApp::monitors_changed ()
{
    if (hotplug_timer.connected ()) hotplug_timer.disconnect ();

    hotplug_timer = Glib::signal_timeout ().connect (sigc::mem_fun(this, &PanelApp::update_monitors), 500);
}

void PanelApp::add_output (GMonitor monitor)
{
    monitors.push_back(std::make_unique <WayfireOutput> (monitor));
    handle_new_output (monitors.back ().get ());
}

void PanelApp::rem_output (GMonitor monitor)
{
    auto it = std::find_if (monitors.begin (), monitors.end (),
        [monitor] (auto& output) { return output->monitor == monitor; });

    if (it != monitors.end ()) handle_output_removed (it->get ());

    auto itr = std::remove_if (monitors.begin (), monitors.end (),
        [monitor] (auto& output) { return output->monitor == monitor; });

    if (itr != monitors.end ()) monitors.erase (itr, monitors.end ());
}

void PanelApp::handle_new_output (WayfireOutput *output)
{
    priv->outputs.push_back (output);
    if (!priv->panel)
    {
        priv->panel = std::make_unique <Panel> (output, true, false);
        priv->dock = std::make_unique <Panel> (output, true, true);
    }
    update_panels ();
}

void PanelApp::handle_output_removed (WayfireOutput *output)
{
    priv->outputs.erase (std::remove (priv->outputs.begin (), priv->outputs.end (), output), priv->outputs.end ());
}

void PanelApp::update_panels ()
{
    priv->dummies.clear ();

    int mon_num = priv->panel->set_monitor ();
    int dmon_num = priv->dock->set_monitor ();

    auto mon = Gdk::Display::get_default ()->get_monitor (mon_num);
    auto dmon = Gdk::Display::get_default ()->get_monitor (dmon_num);
    for (auto& p : priv->outputs)
    {
        if (p->monitor != mon && p->monitor != dmon)
            priv->dummies.push_back (std::make_unique <Panel> (p, false, false));
    }
}

/* DBus interface for commands to plugins */

const GDBusInterfaceVTable PanelApp::interface_vtable =
{
  handle_method_call,
  handle_get_property,
  handle_set_property,
  NULL
};

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
        get ().on_command (plugin, command);
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

void PanelApp::on_command (const char *plugin, const char *command)
{
    if (priv->panel)
        priv->panel->handle_command_message (plugin, command);

    if (priv->dock)
        priv->dock->handle_command_message (plugin, command);
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
