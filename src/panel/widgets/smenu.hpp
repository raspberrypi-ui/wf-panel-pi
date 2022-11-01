#ifndef WIDGETS_SMENU_HPP
#define WIDGETS_SMENU_HPP

#include "../widget.hpp"
#include <gtkmm/button.h>

extern "C" {
#include "smenu/menu.h"
}

class WayfireSmenu : public WayfireWidget
{
    std::unique_ptr <Gtk::Button> plugin;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};

    /* plugin */
    MenuPlugin data;
    MenuPlugin *m;

  public:

    void init (Gtk::HBox *container) override;
    void command (const char *cmd) override;
    virtual ~WayfireSmenu ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
};

#endif /* end of include guard: WIDGETS_SMENU_HPP */
