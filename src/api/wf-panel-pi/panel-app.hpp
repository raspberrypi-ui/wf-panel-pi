#ifndef WF_SHELL_APP_HPP
#define WF_SHELL_APP_HPP

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

    WayfireOutput(const GMonitor& monitor);
    ~WayfireOutput();
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
    std::vector<std::unique_ptr<WayfireOutput>> monitors;
    sigc::connection hotplug_timer;

    class impl;
    static void on_bus_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data);
    static void on_name_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data);
    static void on_name_lost (GDBusConnection *connection, const gchar *name, gpointer user_data);

    static void handle_method_call (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
        const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data);
    static GVariant *handle_get_property (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
        const gchar *property_name, GError **error, gpointer user_data);
    static gboolean handle_set_property (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
        const gchar *property_name, GVariant *value, GError **error, gpointer user_data);

    static const GDBusInterfaceVTable interface_vtable;

  protected:
    /** This should be initialized by the subclass in each program which uses
     * wf-shell-app */
    static std::unique_ptr<PanelApp> instance;
    std::optional<std::string> cmdline_config;

    Glib::RefPtr<Gtk::Application> app;

    virtual void add_output(GMonitor monitor);
    virtual void rem_output(GMonitor monitor);
    virtual void monitors_changed ();
    virtual bool update_monitors ();

    virtual void on_activate();
    virtual bool parse_cfgfile(const Glib::ustring & option_name,
        const Glib::ustring & value, bool has_value);
    void handle_new_output(WayfireOutput *);
    void handle_output_removed(WayfireOutput *);

  public:
    int inotify_fd;
    wf::config::config_manager_t config;
    bool wizard;
    std::unique_ptr <impl> priv;

    PanelApp(int argc, char **argv);
    virtual ~PanelApp();

    virtual std::string get_config_file();
    virtual void run();

    void on_config_reload();
    void on_command(const char *, const char *);

    void update_panels ();

    void rescan_xml_directory (void);
    /**
     * PanelApp is a singleton class.
     * Using this function, any part of the application can get access to the
     * shell app.
     */
    static void create (int argc, char **argv);
    static PanelApp& get();
};

#endif /* end of include guard: WF_SHELL_APP_HPP */
