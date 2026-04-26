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

    static PanelApp& get();
    static void create (int argc, char **argv);

    void rescan_xml_directory ();
    void update_panels ();

  private:
    static std::unique_ptr <PanelApp> instance;
    Glib::RefPtr <Gtk::Application> app;

    std::unique_ptr <Panel> panel;
    std::unique_ptr <Panel> dock;

    std::vector <std::unique_ptr <Panel>> dummies;

    std::vector <std::unique_ptr <WayfireOutput>> monitors;

    sigc::connection hotplug_timer;

    Glib::RefPtr <Gio::DBus::NodeInfo> introspection_data;
    Gio::DBus::InterfaceVTable *interface_vtable;

    int inotify_fd;
    guint owner_id;

    void run ();
    void on_activate ();

    std::string get_config_file ();
    void do_reload_config ();
    bool handle_inotify_event (Glib::IOCondition cond);

    void monitors_changed ();
    bool update_monitors ();

    void on_bus_acquired (const Glib::RefPtr <Gio::DBus::Connection>& connection, const Glib::ustring&);
    void on_name_acquired (const Glib::RefPtr <Gio::DBus::Connection>& connection, const Glib::ustring&);
    void on_name_lost (const Glib::RefPtr <Gio::DBus::Connection>& connection, const Glib::ustring&);
    void handle_method_call (const Glib::RefPtr <Gio::DBus::Connection> &, const Glib::ustring &, const Glib::ustring &, const Glib::ustring &, const Glib::ustring &, const Glib::VariantContainerBase &, const Glib::RefPtr< Gio::DBus::MethodInvocation > &);
    void handle_get_property (Glib::VariantBase &, const Glib::RefPtr< Gio::DBus::Connection > &, const Glib::ustring &, const Glib::ustring &, const Glib::ustring &, const Glib::ustring &);
    void handle_set_property (const Glib::RefPtr< Gio::DBus::Connection > &, const Glib::ustring &, const Glib::ustring &, const Glib::ustring &, const Glib::ustring &, const Glib::VariantBase &);
};

#endif /* end of include guard: PANEL_APP_HPP */
