#ifndef TRAY_TRAY_HPP
#define TRAY_TRAY_HPP

#include "item.hpp"
#include "widgets/tray/host.hpp"

#include <widget.hpp>

class WayfireStatusNotifier : public WayfireWidget
{
  private:
    StatusNotifierHost host = StatusNotifierHost(this);

    Gtk::HBox icons_hbox;
    std::map<Glib::ustring, StatusNotifierItem> items;

    static constexpr conf_table_t conf_table[3] = {
        {CONF_INT,  "smooth_scrolling_threshold",  N_("Smooth scrolling threshold")},
        {CONF_BOOL, "menu_on_middle_click",        N_("Middle button activates menu")},
        {CONF_NONE, NULL,               NULL}
    };

  public:
    void init(Gtk::HBox *container) override;

    void add_item(const Glib::ustring & service);
    void remove_item(const Glib::ustring & service);
    static const char *display_name (void) { return N_("Tray"); };
    static const conf_table_t *config_params (void) { return conf_table; };
};

#endif
