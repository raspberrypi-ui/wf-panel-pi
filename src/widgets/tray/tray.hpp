#ifndef TRAY_TRAY_HPP
#define TRAY_TRAY_HPP

#include "item.hpp"
#include "host.hpp"

#include <widget.hpp>

class WidgetStatusNotifier : public PanelWidget
{
    StatusNotifierHost host = StatusNotifierHost (this);

    Gtk::HBox icons_hbox;
    std::map<Glib::ustring, StatusNotifierItem> items;

    bool momc;
    int sst;

  public:
    void widget_init (Gtk::HBox *container) override;
    void widget_set_icon (void);
    void widget_config_reload (void);

    void add_item (const Glib::ustring & service);
    void remove_item (const Glib::ustring & service);
};

#endif
