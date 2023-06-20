#ifndef WIDGETS_GPU_HPP
#define WIDGETS_GPU_HPP

#include "../widget.hpp"
#include <gtkmm/button.h>

extern "C" {
#include "gpu/gpu.h"
}

class WayfireGPU : public WayfireWidget
{
    std::unique_ptr <Gtk::Button> plugin;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};
    sigc::connection icon_timer;

    WfOption <bool> show_percentage {"panel/gpu_show_percentage"};
    WfOption <std::string> foreground_colour {"panel/gpu_foreground"};
    WfOption <std::string> background_colour {"panel/gpu_background"};

    /* plugin */
    GPUPlugin data;
    GPUPlugin *gpu;

  public:

    void init (Gtk::HBox *container) override;
    virtual ~WayfireGPU ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    bool set_icon (void);
    void settings_changed_cb (void);
    static std::string display_name (void) { return gettext ("GPU"); };
};

#endif /* end of include guard: WIDGETS_GPU_HPP */
