#ifndef WIDGETS_VOLUMEPULSE_HPP
#define WIDGETS_VOLUMEPULSE_HPP

#include <widget.hpp>
#include <gtkmm/button.h>

extern "C" {
#include "volumepulse.h"
}

class WayfireVolumepulse : public WayfireWidget
{
    std::unique_ptr <Gtk::Button> plugin_vol;
    std::unique_ptr <Gtk::Button> plugin_mic;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};
    sigc::connection icon_timer;
    bool wizard;

    /* plugin */
    VolumePulsePlugin data;
    VolumePulsePlugin *vol;

  public:

    void init (Gtk::HBox *container) override;
    void command (const char *cmd) override;
    virtual ~WayfireVolumepulse ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    bool set_icon (void);
};

#endif /* end of include guard: WIDGETS_VOLUMEPULSE_HPP */
