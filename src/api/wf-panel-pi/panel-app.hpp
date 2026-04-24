#ifndef PANEL_APP_HPP
#define PANEL_APP_HPP

#include <set>
#include <string>
#include "config/config-manager.hpp"

#include <gtkmm/application.h>
#include <gdkmm/monitor.h>

using GMonitor = Glib::RefPtr<Gdk::Monitor>;
/**
 * Represents a single output
 */
struct WayfireOutput
{
    GMonitor monitor;
    struct wl_output *wo;

    WayfireOutput (const GMonitor& monitor);
    ~WayfireOutput ();
};

/**
 * A basic shell application.
 *
 * It is suitable for applications that need to show one or more windows
 * per monitor.
 */
class PanelApp
{
  private:
    class impl;
    std::unique_ptr <impl> priv;
    std::vector <std::unique_ptr <WayfireOutput>> monitors;
    sigc::connection hotplug_timer;
    static const GDBusInterfaceVTable interface_vtable;

    
    static void on_bus_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data);
    static void on_name_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data);
    static void on_name_lost (GDBusConnection *connection, const gchar *name, gpointer user_data);

    static void handle_method_call (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
        const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data);
    static GVariant *handle_get_property (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
        const gchar *property_name, GError **error, gpointer user_data);
    static gboolean handle_set_property (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
        const gchar *property_name, GVariant *value, GError **error, gpointer user_data);

    void add_output(GMonitor monitor);
    void rem_output(GMonitor monitor);
    void monitors_changed ();
    bool update_monitors ();

    void on_activate ();
    bool parse_cfgfile (const Glib::ustring & option_name, const Glib::ustring & value, bool has_value);
    void handle_new_output (WayfireOutput *);
    void handle_output_removed (WayfireOutput *);

    static std::unique_ptr <PanelApp> instance;
    std::optional<std::string> cmdline_config;

    Glib::RefPtr<Gtk::Application> app;
    virtual void run();
    void on_command (const char *, const char *);
    void update_panels ();


  public:
    PanelApp (int argc, char **argv);
    ~PanelApp ();

    static PanelApp& get();
    static void create (int argc, char **argv);

    void rescan_xml_directory (void);

    virtual std::string get_config_file ();
    void on_config_reload ();

    int inotify_fd;
    wf::config::config_manager_t config;
    bool wizard;
};

#endif /* end of include guard: PANEL_APP_HPP */
