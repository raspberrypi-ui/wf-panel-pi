#ifndef WIDGETS_UPDATER_HPP
#define WIDGETS_UPDATER_HPP

#include "../widget.hpp"
#include <gtkmm/button.h>

extern "C" {
#include "updater/updater.h"
}

class WayfireUpdater : public WayfireWidget
{
    std::unique_ptr <Gtk::Button> plugin;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};

    /* plugin */
    UpdaterPlugin data;
    UpdaterPlugin *up;

  public:

    void init (Gtk::HBox *container) override;
    void command (const char *cmd) override;
    virtual ~WayfireUpdater ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
};

#endif /* end of include guard: WIDGETS_UPDATER_HPP */
