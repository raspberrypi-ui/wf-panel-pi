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

    static constexpr conf_table_t conf_table[4] = {
        {CONF_BOOL,     "show_percentage",  "Show usage as percentage"},
        {CONF_COLOUR,   "foreground",       "Foreground colour"},
        {CONF_COLOUR,   "background",       "Background colour"},
        {CONF_NONE,     NULL,               NULL}
    };

  public:

    void init (Gtk::HBox *container) override;
    virtual ~WayfireGPU ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    bool set_icon (void);
    void settings_changed_cb (void);
    static const char *display_name (void) { return "GPU"; };
    static const conf_table_t *config_params (void) { return conf_table; };
};

#endif /* end of include guard: WIDGETS_GPU_HPP */
