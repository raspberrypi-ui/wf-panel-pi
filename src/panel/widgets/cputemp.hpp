#ifndef WIDGETS_CPUTEMP_HPP
#define WIDGETS_CPUTEMP_HPP

#include "../widget.hpp"
#include <gtkmm/button.h>

extern "C" {
#include "cputemp/cputemp.h"
}

class WayfireCPUTemp : public WayfireWidget
{
    std::unique_ptr <Gtk::Button> plugin;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};
    sigc::connection icon_timer;

    WfOption <std::string> foreground_colour {"panel/cputemp_foreground"};
    WfOption <std::string> background_colour {"panel/cputemp_background"};
    WfOption <std::string> throttle1_colour {"panel/cputemp_throttle_1"};
    WfOption <std::string> throttle2_colour {"panel/cputemp_throttle_2"};
    WfOption <int> low_temp {"panel/cputemp_low_temp"};
    WfOption <int> high_temp {"panel/cputemp_high_temp"};

    /* plugin */
    CPUTempPlugin data;
    CPUTempPlugin *cput;

    static constexpr conf_table_t conf_table[7] = {
        {"cputemp", "foreground",   CONF_COLOUR,    "Foreground colour"},
        {"cputemp", "background",   CONF_COLOUR,    "Background colour"},
        {"cputemp", "throttle_1",   CONF_COLOUR,    "Colour when ARM frequency capped"},
        {"cputemp", "throttle_2",   CONF_COLOUR,    "Colour when throttled"},
        {"cputemp", "low_temp",     CONF_INT,       "Lower temperature bound"},
        {"cputemp", "high_temp",    CONF_INT,       "Upper temperature bound"},
        {NULL,      NULL,           CONF_NONE,      NULL}
    };

  public:

    void init (Gtk::HBox *container) override;
    virtual ~WayfireCPUTemp ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    bool set_icon (void);
    void settings_changed_cb (void);
    static std::string display_name (void) { return gettext ("CPU Temp"); };
    static const conf_table_t *config_params (void) { return conf_table; };
};

#endif /* end of include guard: WIDGETS_CPUTEMP_HPP */
