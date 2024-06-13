#ifndef WIDGETS_TESTXML_HPP
#define WIDGETS_TESTXML_HPP

#include "../widget.hpp"
#include <gtkmm/button.h>
#include <gtkmm/menu.h>
#include <gtkmm/image.h>


class WayfireXMLPlugin : public WayfireWidget
{
    std::unique_ptr <Gtk::Button> plugin;
    std::unique_ptr <Gtk::Menu> menu;
    std::unique_ptr <Gtk::Image> icon;
    std::unique_ptr <Gtk::MenuItem> item;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};
    sigc::connection icon_timer;

    static constexpr conf_table_t conf_table[1] = {
        {CONF_NONE, NULL,       NULL}
    };

  public:

    void init (Gtk::HBox *container) override;
    void command (const char *cmd) override;
    virtual ~WayfireXMLPlugin ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    bool set_icon (void);
    void on_button_press_event (void);
    void do_menu (void);
    static const char *display_name (void) { return N_("XML Test"); };
    static const conf_table_t *config_params (void) { return conf_table; };
};

#endif /* end of include guard: WIDGETS_TESTXML_HPP */
