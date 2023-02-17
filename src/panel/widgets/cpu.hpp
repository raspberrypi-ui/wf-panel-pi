#ifndef WIDGETS_CPU_HPP
#define WIDGETS_CPU_HPP

#include "../widget.hpp"
#include <gtkmm/button.h>

extern "C" {
#include "cpu/cpu.h"
}

class WayfireCPU : public WayfireWidget
{
    std::unique_ptr <Gtk::Button> plugin;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};

    /* plugin */
    CPUPlugin data;
    CPUPlugin *cpu;

  public:

    void init (Gtk::HBox *container) override;
    virtual ~WayfireCPU ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
};

#endif /* end of include guard: WIDGETS_CPU_HPP */
