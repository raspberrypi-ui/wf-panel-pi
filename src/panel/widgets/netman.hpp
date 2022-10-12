#ifndef WIDGETS_NETMAN_HPP
#define WIDGETS_NETMAN_HPP

#include "../widget.hpp"
#include <gtkmm/button.h>

extern "C" {
#include "netman/applet.h"
}

class WayfireNetman : public WayfireWidget
{
    std::unique_ptr <Gtk::Button> plugin;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};

    /* plugin */
    NMApplet *nm;

  public:

    void init (Gtk::HBox *container) override;
    virtual ~WayfireNetman ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
};

#endif /* end of include guard: WIDGETS_NETMAN_HPP */
