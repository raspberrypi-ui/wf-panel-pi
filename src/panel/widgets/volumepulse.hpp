#ifndef WIDGETS_VOLUMEPULSE_HPP
#define WIDGETS_VOLUMEPULSE_HPP

#include "../widget.hpp"
#include "wf-popover.hpp"

extern "C" {
#include "volumepulse/volumepulse.h"
}

class WayfireVolumepulse : public WayfireWidget
{
    std::unique_ptr <WayfireMenuButton> plugin;

    WfOption <int> icon_size {"panel/icon_size"};
    WfOption <std::string> bar_pos {"panel/position"};
    sigc::connection icon_timer;
    bool wizard;

    /* plugin */
    VolumePulsePlugin data;
    VolumePulsePlugin *vol;

    static constexpr conf_table_t conf_table[1] = {
        {CONF_NONE, NULL, NULL}
    };

  public:

    void init (Gtk::HBox *container) override;
    void command (const char *cmd) override;
    virtual ~WayfireVolumepulse ();
    void icon_size_changed_cb (void);
    void bar_pos_changed_cb (void);
    bool set_icon (void);
    static const char *display_name (void) { return "Volume"; };
    static const conf_table_t *config_params (void) { return conf_table; };
};

#endif /* end of include guard: WIDGETS_VOLUMEPULSE_HPP */
