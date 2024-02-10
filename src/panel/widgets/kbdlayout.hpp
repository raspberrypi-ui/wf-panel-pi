#ifndef WIDGETS_KBDLAYOUT_HPP
#define WIDGETS_KBDLAYOUT_HPP

#include "../widget.hpp"
#include <gtkmm/menubutton.h>
#include <gtkmm/label.h>


class WayfireKbdLayout : public WayfireWidget
{

// currently no use for a real menu, see comments in kbdlayout.cpp
#if 0
    class MenuItem : public Gtk::MenuItem {

        std::string shrt_name;
        std::string long_name;

      public:
        /*
        MenuItem(const Glib::ustring &label, bool mnemonic = false) :
              Gtk::MenuItem(label, mnemonic)
        {
        }
        */

        MenuItem(std::string shrtn, std::string longn, bool mnemonic = false) :
              Gtk::MenuItem(shrtn + " (" + longn + ")", mnemonic)
        {
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
            fprintf(stderr, "%s clicked\n", get_label().c_str());
        }
    };
#endif

    std::unique_ptr<Gtk::MenuButton> button;
    Gtk::Label label;
    Gtk::Menu menu;
    std::unordered_map<std::string,std::string> long_names;
    std::string get_long_name(std::string short_name);

    WfOption<std::string> panel_position {"panel/position"};


    sigc::connection timeout;
    WfOption<std::string> font{"panel/kbdlayout_font"};

    void set_font();
#if 0
    // mostly debugging stuff, left out
    void on_button_press_event(GdkEventButton *event);
    void on_menu_shown();
#endif

    static constexpr conf_table_t conf_table[3] = {
        {CONF_STRING,   "font",     N_("Display font")},
        {CONF_NONE,     NULL,       NULL}
    };

  public:
    struct wl_seat *seat;
    struct wl_keyboard *keyboard;

    void init(Gtk::HBox *container) override;
    void command(const char *cmd) override;
    void update_label(const char *layout);
    static const char *display_name (void) { return N_("KbdLayout"); };
    static const conf_table_t *config_params (void) { return conf_table; };
    ~WayfireKbdLayout();

};

#endif /* end of include guard: WIDGETS_KBDLAYOUT_HPP */
