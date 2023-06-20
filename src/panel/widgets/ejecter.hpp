#ifndef WIDGETS_EJECTER_HPP
#define WIDGETS_EJECTER_HPP

#include "../widget.hpp"
#include <gtkmm/button.h>

extern "C" {
#include "ejecter/ejecter.h"
}

class WayfireEjecter : public WayfireWidget
{
    std::unique_ptr <Gtk::Button> plugin;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};
    sigc::connection icon_timer;

    WfOption <bool> autohide {"panel/ejecter_autohide"};

    /* plugin */
    EjecterPlugin data;
    EjecterPlugin *ej;

  public:

    void init (Gtk::HBox *container) override;
    void command (const char *cmd) override;
    virtual ~WayfireEjecter ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    bool set_icon (void);
    void settings_changed_cb (void);
    static std::string display_name (void) { return gettext ("Ejecter"); };
};

#endif /* end of include guard: WIDGETS_EJECTER_HPP */
