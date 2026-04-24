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
#include "panel-app.hpp"
#include "wf-autohide-window.hpp"

class Panel
{
  public:
    Panel (WayfireOutput *output, bool real, bool dock);
    void handle_config_reload ();
    void handle_command_message (const char *plugin, const char *cmd);
    int set_monitor ();

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
    Glib::RefPtr <Gtk::GestureLongPress> gesture;
    sigc::connection draw_connection;

    std::vector <std::unique_ptr <WayfireWidget>> left_widgets, right_widgets;

    WayfireOutput *output;
    bool wizard = PanelApp::get().wizard;
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

    void set_layer ();
    void set_exclusive ();
    bool on_keypress_event (GdkEventKey* event);
    bool on_button_press_event (GdkEventButton* event);
    bool on_button_release_event (GdkEventButton* event);
    bool on_delete (GdkEventAny *ev);
    void do_configure ();
    void do_plugin_configure ();
    void do_notify_configure ();
    void do_appearance_set ();
    std::unique_ptr<WayfireWidget> widget_from_name (const char *name);
    void reload_widgets (std::string list, std::vector <std::unique_ptr <WayfireWidget>>& container, Gtk::HBox& box);
    void init_widgets ();
    void init_notify ();
    void update_widget_icons ();
    void update_panels ();
};

#endif /* end of include guard: WF_PANEL_HPP */
