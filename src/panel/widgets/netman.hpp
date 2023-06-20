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
    sigc::connection icon_timer;

    /* plugin */
    NMApplet *nm;

    static constexpr conf_table_t conf_table[1] = {
        {NULL,  NULL,   CONF_NONE,  NULL}
    };

  public:

    void init (Gtk::HBox *container) override;
    void command (const char *cmd) override;
    virtual ~WayfireNetman ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    bool set_icon (void);
    static std::string display_name (void) { return gettext ("Network"); };
    static const conf_table_t *config_params (void) { return conf_table; };
};

#endif /* end of include guard: WIDGETS_NETMAN_HPP */
