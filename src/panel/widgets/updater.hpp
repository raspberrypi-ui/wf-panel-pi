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
    sigc::connection icon_timer;

    WfOption <int> interval {"panel/updater_interval"};

    /* plugin */
    UpdaterPlugin data;
    UpdaterPlugin *up;

    static constexpr conf_table_t conf_table[2] = {
        {CONF_INT,  "interval", "Hours between checks for updates"},
        {CONF_NONE, NULL,       NULL}
    };

  public:

    void init (Gtk::HBox *container) override;
    void command (const char *cmd) override;
    virtual ~WayfireUpdater ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    bool set_icon (void);
    void settings_changed_cb (void);
    static const char *display_name (void) { return "Updater"; };
    static const conf_table_t *config_params (void) { return conf_table; };
};

#endif /* end of include guard: WIDGETS_UPDATER_HPP */
