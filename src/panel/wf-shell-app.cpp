#include "wf-shell-app.hpp"
#include <fcntl.h>
#include <glibmm/main.h>
#include <sys/inotify.h>
#include <gdk/gdkwayland.h>
#include <iostream>
#include <memory>
#include "config/file.hpp"

#include <unistd.h>
#include <libfm/fm-gtk.h>
#include <sys/time.h>
extern "C" {
#include "widgets/launcher.h"
}

std::string WayfireShellApp::get_config_file()
{
    if (cmdline_config.has_value())
    {
        return cmdline_config.value();
    }

    std::string config_dir;

    char *config_home = getenv("XDG_CONFIG_HOME");
    if (config_home == NULL)
    {
        config_dir = std::string(getenv("HOME")) + "/.config";
    } else
    {
        config_dir = std::string(config_home);
    }

    return config_dir + "/wf-panel-pi.ini";
}

bool WayfireShellApp::parse_cfgfile(const Glib::ustring & option_name,
    const Glib::ustring & value, bool has_value)
{
    std::cout << "Using custom config file " << value << std::endl;
    cmdline_config = value;
    return true;
}

#define INOT_BUF_SIZE (1024 * sizeof(inotify_event))
char buf[INOT_BUF_SIZE];

/* Reload file and add next inotify watch */
static void do_reload_config(WayfireShellApp *app)
{
    wf::config::load_configuration_options_from_file(
        app->config, app->get_config_file());
    app->on_config_reload();
    inotify_add_watch(app->inotify_fd, app->get_config_file().c_str(), IN_MODIFY);
}

/* Handle inotify event */
static bool handle_inotify_event(WayfireShellApp *app, Glib::IOCondition cond)
{
    /* read, but don't use */
    read(app->inotify_fd, buf, INOT_BUF_SIZE);
    do_reload_config(app);

    return true;
}

static void registry_add_object(void *data, struct wl_registry *registry,
    uint32_t name, const char *interface, uint32_t version)
{
    auto app = static_cast<WayfireShellApp*>(data);
    if (strcmp(interface, zwf_shell_manager_v2_interface.name) == 0)
    {
        app->wf_shell_manager = (zwf_shell_manager_v2*)wl_registry_bind(registry, name,
            &zwf_shell_manager_v2_interface, std::min(version, 1u));
    }
}

static void registry_remove_object(void *data, struct wl_registry *registry,
    uint32_t name)
{}

static struct wl_registry_listener registry_listener =
{
    &registry_add_object,
    &registry_remove_object
};

void WayfireShellApp::on_activate()
{
    app->hold();

    fm_gtk_init (NULL);

    if (!g_strcmp0 (getenv ("USER"), "rpi-first-boot-wizard")) wizard = true;
    else wizard = false;

    // load wf-shell if available
    auto gdk_display = gdk_display_get_default();
    auto wl_display  = gdk_wayland_display_get_wl_display(gdk_display);
    if (!wl_display)
    {
        std::cerr << "Failed to connect to wayland display!" <<
            " Are you sure you are running a wayland compositor?" << std::endl;
        std::exit(-1);
    }

    wl_registry *registry = wl_display_get_registry(wl_display);
    wl_registry_add_listener(registry, &registry_listener, this);
    wl_display_roundtrip(wl_display);

    std::vector<std::string> xmldirs(1, METADATA_DIR);

    // setup config
    close (open (get_config_file ().c_str(), O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
    init_launchers ();

    this->config = wf::config::build_configuration(
        xmldirs, RESOURCE_DIR "/defaults.ini",
        get_config_file());

    inotify_fd = inotify_init();
    do_reload_config(this);

    Glib::signal_io().connect(
        sigc::bind<0>(&handle_inotify_event, this),
        inotify_fd, Glib::IO_IN | Glib::IO_HUP);

    // Hook up monitor tracking
    auto display = Gdk::Display::get_default();
    display->signal_monitor_added().connect_notify(
        [=] (const GMonitor& monitor) { this->monitors_changed(); });
    display->signal_monitor_removed().connect_notify(
        [=] (const GMonitor& monitor) { this->monitors_changed(); });

    // initial monitors
    this->monitors_changed ();
}

bool WayfireShellApp::update_monitors ()
{
    // clear the existing monitors
    for (auto &mon : monitors)
        handle_output_removed (mon.get ());
    monitors.clear ();

    // find the new list of monitors
    auto display = Gdk::Display::get_default ();
    int num_monitors = display->get_n_monitors ();
    for (int i = 0; i < num_monitors; i++)
    {
        monitors.push_back (std::make_unique<WayfireOutput> (display->get_monitor (i), this->wf_shell_manager));
        handle_new_output (monitors.back().get());
    }

    return false;
}

void WayfireShellApp::monitors_changed ()
{
    if (hotplug_timer.connected ()) hotplug_timer.disconnect ();

    hotplug_timer = Glib::signal_timeout().connect(
        sigc::mem_fun(this, &WayfireShellApp::update_monitors), 500);
}

void WayfireShellApp::add_output(GMonitor monitor)
{
    monitors.push_back(
        std::make_unique<WayfireOutput>(monitor, this->wf_shell_manager));
    handle_new_output(monitors.back().get());
}

void WayfireShellApp::rem_output(GMonitor monitor)
{
    auto it = std::find_if(monitors.begin(), monitors.end(),
        [monitor] (auto& output) { return output->monitor == monitor; });

    if (it != monitors.end())
    {
        handle_output_removed(it->get());
    }

    auto itr = std::remove_if(monitors.begin(), monitors.end(),
        [monitor] (auto& output) { return output->monitor == monitor; });
    if (itr != monitors.end())
    {
        monitors.erase(itr, monitors.end());
    }
}

WayfireShellApp::WayfireShellApp(int argc, char **argv)
{
    app = Gtk::Application::create(argc, argv, "",
        Gio::APPLICATION_HANDLES_COMMAND_LINE);
    app->signal_activate().connect_notify(
        sigc::mem_fun(this, &WayfireShellApp::on_activate));
    app->add_main_option_entry(
        sigc::mem_fun(this, &WayfireShellApp::parse_cfgfile),
        "config", 'c', "config file to use", "file");

    // Activate app after parsing command line
    app->signal_command_line().connect_notify([=] (auto&)
    {
        app->activate();
    });
}

WayfireShellApp::~WayfireShellApp()
{}

std::unique_ptr<WayfireShellApp> WayfireShellApp::instance;
WayfireShellApp& WayfireShellApp::get()
{
    return *instance;
}

void WayfireShellApp::run()
{
    app->run();
}

/* -------------------------- WayfireOutput --------------------------------- */
WayfireOutput::WayfireOutput(const GMonitor& monitor,
    zwf_shell_manager_v2 *zwf_manager)
{
    this->monitor = monitor;
    this->wo = gdk_wayland_monitor_get_wl_output(monitor->gobj());

    if (zwf_manager)
    {
        this->output =
            zwf_shell_manager_v2_get_wf_output(zwf_manager, this->wo);
    } else
    {
        this->output = nullptr;
    }
}

WayfireOutput::~WayfireOutput()
{
    if (this->output)
    {
        zwf_output_v2_destroy(this->output);
    }
}
