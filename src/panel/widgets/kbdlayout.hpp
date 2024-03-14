#ifndef WIDGETS_KBDLAYOUT_HPP
#define WIDGETS_KBDLAYOUT_HPP

#include "../widget.hpp"
#include <gtkmm/menubutton.h>
#include <gtkmm/label.h>
#include <giomm.h>


class WayfireKbdLayout : public WayfireWidget
{

    class MenuItem : public Gtk::MenuItem {

        std::string shrt_name;
        std::string long_name;
        WayfireKbdLayout *parent;

      public:
        /*
        MenuItem(const Glib::ustring &label, bool mnemonic = false) :
              Gtk::MenuItem(label, mnemonic)
        {
        }
        */

        MenuItem(WayfireKbdLayout *prnt, std::string shrtn, std::string longn, bool mnemonic = false) :
              Gtk::MenuItem(shrtn + " (" + longn + ")", mnemonic)
        {
           this->parent = prnt;
           this->shrt_name = shrtn;
           this->long_name = longn;
        }

        void set_activate()
        {
            this->signal_activate().connect_notify(
                sigc::mem_fun(this, &WayfireKbdLayout::MenuItem::on_clicked)
            );
        }

        std::string get_short_name()
        {
            return shrt_name;
        }

        std::string get_long_name()
        {
            return long_name;
        }

        void on_clicked()
        {
            fprintf(stderr, "%s clicked (short name=%s, long name=%s)\n",
                get_label().c_str(),
                get_short_name().c_str(),
                get_long_name().c_str());
            parent->dbus_call_switch(get_short_name());
        }
    };

    // Menu
    std::unique_ptr<Gtk::MenuButton> button;
    Gtk::Label label;
    Gtk::Menu menu;
    std::unordered_map<std::string,std::string> long_names;
    std::string get_long_name(std::string short_name);
    void reset_menu(const char *layout_list);

    // Options
    WfOption<std::string> panel_position {"panel/position"};

    // DBus
    Glib::RefPtr<Glib::MainLoop> loop;
    Glib::RefPtr<Gio::DBus::Connection> connection;
    Glib::RefPtr<Gio::DBus::Proxy> proxy;
    void dbus_initialize();
    void dbus_call_enable(bool flag);
    void dbus_call_enable_off_on_pulse();
    void dbus_call_switch(std::string layout);
    void dbus_terminate();

    // Handlers
#if 0
    void on_button_press_event(GdkEventButton *event);
#endif

    // Options
    sigc::connection timeout;
    WfOption<std::string> font{"panel/kbdlayout_font"};

    void set_font();

    static constexpr conf_table_t conf_table[2] = {
        {CONF_STRING,   "font",     N_("Display font")},
        {CONF_NONE,     NULL,       NULL}
    };

  public:
#if 0
    struct wl_seat *seat;
    struct wl_keyboard *keyboard;
#endif

    void init(Gtk::HBox *container) override;
    void command(const char *cmd) override;
    void update_label(const char *layout);
    static const char *display_name (void) { return N_("KbdLayout"); };
    static const conf_table_t *config_params (void) { return conf_table; };
    ~WayfireKbdLayout();

};

#endif /* end of include guard: WIDGETS_KBDLAYOUT_HPP */
