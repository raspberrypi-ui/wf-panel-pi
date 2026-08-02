#ifndef TRAY_TRAY_HPP
#define TRAY_TRAY_HPP

#include "item.hpp"
#include "host.hpp"

#include <widget.hpp>

class WidgetStatusNotifier : public PanelWidget
{
    bool momc;
    int sst;

  private:
    StatusNotifierHost host = StatusNotifierHost(this);

    Gtk::HBox icons_hbox;
    std::map<Glib::ustring, StatusNotifierItem> items;

  public:
    void init (Gtk::HBox *container) override;

    void add_item (const Glib::ustring & service);
    void remove_item (const Glib::ustring & service);
    bool set_icon (void);
    void read_settings (void);
    void handle_config_reload (void);
};

#endif
