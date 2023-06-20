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
    WfOption <int> search_height {"panel/smenu_search_height"};
    WfOption <bool> search_fixed {"panel/smenu_search_fixed"};
    sigc::connection icon_timer;

    /* plugin */
    MenuPlugin data;
    MenuPlugin *m;

    static constexpr conf_table_t conf_table[3] = {
        {"smenu",   "search_height",    CONF_INT,   "Search window height"},
        {"smenu",   "search_fixed",     CONF_BOOL,  "Fix size of search window"},
        {NULL,      NULL,               CONF_NONE,  NULL}
    };

  public:

    void init (Gtk::HBox *container) override;
    void command (const char *cmd) override;
    virtual ~WayfireSmenu ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    void search_param_changed_cb (void);
    bool set_icon (void);
    static std::string display_name (void) { return gettext ("Menu"); };
    static const conf_table_t *config_params (void) { return conf_table; };
};

#endif /* end of include guard: WIDGETS_SMENU_HPP */
