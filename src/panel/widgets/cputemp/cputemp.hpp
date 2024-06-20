#ifndef WIDGETS_CPUTEMP_HPP
#define WIDGETS_CPUTEMP_HPP

#include <widget.hpp>
#include <gtkmm/button.h>

extern "C" {
#include "cputemp.h"
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

  public:

    void init (Gtk::HBox *container) override;
    virtual ~WayfireCPUTemp ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    bool set_icon (void);
    void settings_changed_cb (void);
};

#endif /* end of include guard: WIDGETS_CPUTEMP_HPP */
