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
    sigc::connection icon_timer;

    WfOption <int> batt_num {"panel/power_batt_num"};

    /* plugin */
    PtBattPlugin data;
    PtBattPlugin *pt;

    static constexpr conf_table_t conf_table[2] = {
        {CONF_INT,  "batt_num", N_("Battery number to monitor")},
        {CONF_NONE, NULL,       NULL}
    };

  public:

    void init (Gtk::HBox *container) override;
    virtual ~WayfirePower ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    bool set_icon (void);
    static const char *display_name (void) { return N_("Power"); };
    static const conf_table_t *config_params (void) { return conf_table; };
};

#endif /* end of include guard: WIDGETS_POWER_HPP */
