#ifndef WIDGETS_SMENU_HPP
#define WIDGETS_SMENU_HPP

#include "../widget.hpp"
#include "wf-popover.hpp"

extern "C" {
#include "smenu/menu.h"
}

class WayfireSmenu : public WayfireWidget
{
    std::unique_ptr <WayfireMenuButton> plugin;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};
    WfOption <int> search_height {"panel/search_height"};
    WfOption <bool> search_fixed {"panel/search_fixed"};
    sigc::connection icon_timer;

    /* plugin */
    MenuPlugin data;
    MenuPlugin *m;

  public:

    void init (Gtk::HBox *container) override;
    void command (const char *cmd) override;
    virtual ~WayfireSmenu ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    void search_param_changed_cb (void);
    bool set_icon (void);
};

#endif /* end of include guard: WIDGETS_SMENU_HPP */
