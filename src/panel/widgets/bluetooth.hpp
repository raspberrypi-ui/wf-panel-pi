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
    bool wizard = FALSE;

    /* plugin */
    BluetoothPlugin data;
    BluetoothPlugin *bt;

  public:

    void init (Gtk::HBox *container) override;
    void command (const char *cmd) override;
    virtual ~WayfireBluetooth ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    void set_wizard (void) override;
};

#endif /* end of include guard: WIDGETS_BLUETOOTH_HPP */
