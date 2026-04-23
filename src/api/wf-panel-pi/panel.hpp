#ifndef WF_PANEL_HPP
#define WF_PANEL_HPP

#include <memory>
#include <wayland-client.h>

#include <gtkmm/window.h>
#include <gtkmm/menu.h>
#include <gtkmm/headerbar.h>
#include <gtkmm/hvbox.h>
#include <gtkmm/application.h>
#include <gtkmm/gesturelongpress.h>
#include <gdkmm/display.h>
#include <gdkmm/seat.h>

#include "widget.hpp"
#include "wf-shell-app.hpp"
#include "wf-autohide-window.hpp"

class WayfirePanel
{
  public:
    WayfirePanel(WayfireOutput *output, bool real, bool dock);

    wl_surface *get_wl_surface();
    Gtk::Window& get_window();
    void handle_config_reload();
    void handle_command_message (const char *plugin, const char *cmd);
    int set_monitor();
    WayfireOutput *get_output();

  private:
    std::unique_ptr<WayfireAutohidingWindow> window;

    Gtk::HBox content_box;
    Gtk::HBox left_box, right_box;
    Gtk::Menu menu;
    Gtk::MenuItem conf;
    Gtk::MenuItem cplug;
    Gtk::MenuItem notif;
    Gtk::MenuItem appset;
    std::string conf_plugin;
    Glib::RefPtr<Gtk::GestureLongPress> gesture;
    sigc::connection draw_connection;

    std::vector<std::unique_ptr<WayfireWidget>> left_widgets, right_widgets;

    WayfireOutput *output;
    bool wizard = WayfireShellApp::get().wizard;
    bool real;
    bool dock;
    int scaling;
    int isize;

    WfOption <int> icon_size;
    WfOption <std::string> layer;
    WfOption <std::string> monitor_num;
    WfOption <std::string> left_widgets_opt;
    WfOption <std::string> right_widgets_opt;
    WfOption <bool> exclusive;
    WfOption <int> minimal_panel_height;
    WfOption <bool> gestures_touch_only;
    WfOption <int> notify_timeout;
    WfOption <bool> notifications;
    WfOption <bool> libnotify;

    void set_layer();
    void set_exclusive();
    bool on_keypress_event (GdkEventKey* event);
    bool on_button_press_event(GdkEventButton* event);
    bool on_button_release_event(GdkEventButton* event);
    void do_configure();
    void do_plugin_configure();
    void do_notify_configure();
    void do_appearance_set();
    bool on_delete(GdkEventAny *ev);
    std::unique_ptr<WayfireWidget> widget_from_name(std::string name);
    static std::vector<std::string> tokenize(std::string list);
    void reload_widgets(std::string list, std::vector<std::unique_ptr<WayfireWidget>>& container, Gtk::HBox& box);
    void init_widgets();
    void init_notify ();
    void update_panels ();
    void update_widget_icons ();
    void message_widget (const char *name, const char *cmd);
};

class WayfirePanelApp : public WayfireShellApp
{
  public:
    static WayfirePanelApp& get();

    /* Starts the program. get() is valid afterward the first (and the only)
     * call to create() */
    static void create(int argc, char **argv);
    ~WayfirePanelApp();

    void handle_new_output(WayfireOutput *output) override;
    void handle_output_removed(WayfireOutput *output) override;
    void on_config_reload() override;
    void on_command (const char *plugin, const char *command) override;
    void update_panels ();
    void update_widget_icons ();

  private:
    WayfirePanelApp(int argc, char **argv);

    class impl;
    std::unique_ptr<impl> priv;

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
};

#endif /* end of include guard: WF_PANEL_HPP */
