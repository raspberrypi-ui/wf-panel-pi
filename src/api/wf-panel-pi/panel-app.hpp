#ifndef PANEL_APP_HPP
#define PANEL_APP_HPP

#include <gtkmm/application.h>

#include "config/config-manager.hpp"

class Panel;

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

    sigc::connection hotplug_timer;

    Glib::RefPtr <Gio::DBus::NodeInfo> introspection_data;
    Gio::DBus::InterfaceVTable *interface_vtable;
    guint owner_id;

    int inotify_fd;

    void run ();
    void on_activate ();

    std::string get_config_file ();
    void do_reload_config ();
    bool handle_inotify_event (Glib::IOCondition cond);

    void monitors_changed ();
    bool update_monitors ();

    void on_bus_acquired (const Glib::RefPtr <Gio::DBus::Connection>&, const Glib::ustring&);
    void on_name_acquired (const Glib::RefPtr <Gio::DBus::Connection>&, const Glib::ustring&);
    void on_name_lost (const Glib::RefPtr <Gio::DBus::Connection>&, const Glib::ustring&);
    void handle_method_call (const Glib::RefPtr <Gio::DBus::Connection>&, const Glib::ustring&, const Glib::ustring&,
        const Glib::ustring&, const Glib::ustring&, const Glib::VariantContainerBase&, const Glib::RefPtr <Gio::DBus::MethodInvocation>&);
};

#endif /* end of include guard: PANEL_APP_HPP */
