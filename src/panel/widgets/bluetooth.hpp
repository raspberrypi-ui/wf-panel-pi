#ifndef WIDGETS_BLUETOOTH_HPP
#define WIDGETS_BLUETOOTH_HPP

#include "../widget.hpp"
#include <gtkmm/button.h>

extern "C" {
#include "bluetooth/bluetooth.h"
}

class WayfireBluetooth : public WayfireWidget
{
    std::unique_ptr <Gtk::Button> plugin;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};
    sigc::connection icon_timer;
    bool wizard;

    /* plugin */
    BluetoothPlugin data;
    BluetoothPlugin *bt;

    static constexpr conf_table_t conf_table[1] = {
        {CONF_NONE, NULL, NULL}
    };

  public:

    void init (Gtk::HBox *container) override;
    void command (const char *cmd) override;
    virtual ~WayfireBluetooth ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    bool set_icon (void);
    static const char *display_name (void) { return "Bluetooth"; };
    static const conf_table_t *config_params (void) { return conf_table; };
};

#endif /* end of include guard: WIDGETS_BLUETOOTH_HPP */
