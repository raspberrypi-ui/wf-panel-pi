#ifndef WIDGETS_BATT_HPP
#define WIDGETS_BATT_HPP

#include <widget.hpp>
#include <gtkmm/button.h>

extern "C" {
#include "batt.h"
}

class WayfireBatt : public WayfireWidget
{
    std::unique_ptr <Gtk::Button> plugin;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};
    sigc::connection icon_timer;

    WfOption <int> batt_num {"panel/batt_batt_num"};

    /* plugin */
    PtBattPlugin *pt;

  public:

    void init (Gtk::HBox *container) override;
    virtual ~WayfireBatt ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    bool set_icon (void);
};

#endif /* end of include guard: WIDGETS_BATT_HPP */
