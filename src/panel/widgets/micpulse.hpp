#ifndef WIDGETS_MICPULSE_HPP
#define WIDGETS_MICPULSE_HPP

#include "../widget.hpp"
#include "wf-popover.hpp"

extern "C" {
#include "volumepulse/volumepulse.h"
}

class WayfireMicpulse : public WayfireWidget
{
    std::unique_ptr <WayfireMenuButton> plugin;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};
    sigc::connection icon_timer;

    /* plugin */
    VolumePulsePlugin data;
    VolumePulsePlugin *vol;

  public:

    void init (Gtk::HBox *container) override;
    void command (const char *cmd) override;
    virtual ~WayfireMicpulse ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    bool set_icon (void);
};

#endif /* end of include guard: WIDGETS_VOLUMEPULSE_HPP */
