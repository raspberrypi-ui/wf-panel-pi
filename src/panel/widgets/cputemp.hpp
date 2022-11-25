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

    /* plugin */
    CPUTempPlugin data;
    CPUTempPlugin *cput;

  public:

    void init (Gtk::HBox *container) override;
    void command (const char *cmd) override;
    virtual ~WayfireCPUTemp ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
};

#endif /* end of include guard: WIDGETS_CPUTEMP_HPP */
