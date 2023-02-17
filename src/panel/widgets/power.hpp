#ifndef WIDGETS_POWER_HPP
#define WIDGETS_POWER_HPP

#include "../widget.hpp"
#include <gtkmm/button.h>

extern "C" {
#include "power/ptbatt.h"
}

class WayfirePower : public WayfireWidget
{
    std::unique_ptr <Gtk::Button> plugin;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};

    /* plugin */
    PtBattPlugin data;
    PtBattPlugin *pt;

  public:

    void init (Gtk::HBox *container) override;
    virtual ~WayfirePower ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
};

#endif /* end of include guard: WIDGETS_POWER_HPP */
