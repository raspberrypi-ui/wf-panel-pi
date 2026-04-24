#ifndef PANEL_APP_HPP
#define PANEL_APP_HPP

#include <set>
#include <string>

#include <gtkmm/application.h>
#include <gdkmm/monitor.h>

#include "config/config-manager.hpp"

class Panel;

using GMonitor = Glib::RefPtr<Gdk::Monitor>;
struct WayfireOutput
{
    GMonitor monitor;
    struct wl_output *wo;

    WayfireOutput (const GMonitor& monitor);
    ~WayfireOutput ();
};

class PanelApp
{
  public:
    PanelApp (int argc, char **argv);
    ~PanelApp ();

    wf::config::config_manager_t config;
    bool wizard;

    static PanelApp& get();
    static void create (int argc, char **argv);

    void rescan_xml_directory (void);

  private:
    static std::unique_ptr <PanelApp> instance;
    Glib::RefPtr <Gtk::Application> app;

    std::unique_ptr <Panel> panel;
    std::unique_ptr <Panel> dock;

    std::vector <std::unique_ptr <Panel>> dummies;

    std::vector <std::unique_ptr <WayfireOutput>> monitors;

    sigc::connection hotplug_timer;
    static const GDBusInterfaceVTable interface_vtable;
    int inotify_fd;
    guint owner_id;

    void run ();
    void on_activate ();

    std::string get_config_file ();
    void do_reload_config ();
    bool handle_inotify_event (Glib::IOCondition cond);

    void monitors_changed ();
    bool update_monitors ();

    static void on_bus_acquired (GDBusConnection *, const gchar *, gpointer);
    static void on_name_acquired (GDBusConnection *, const gchar *, gpointer);
    static void on_name_lost (GDBusConnection *, const gchar *, gpointer);
    static void handle_method_call (GDBusConnection *, const gchar *, const gchar *, const gchar *, const gchar *, GVariant *, GDBusMethodInvocation *, gpointer);
    static GVariant *handle_get_property (GDBusConnection *, const gchar *, const gchar *, const gchar *, const gchar *, GError **, gpointer);
    static gboolean handle_set_property (GDBusConnection *connection, const gchar *, const gchar *, const gchar *, const gchar *, GVariant *, GError **, gpointer);
};

#endif /* end of include guard: PANEL_APP_HPP */
