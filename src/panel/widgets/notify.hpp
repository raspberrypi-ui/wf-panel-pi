#ifndef WIDGETS_NOTIFY_HPP
#define WIDGETS_NOTIFY_HPP

#include "../widget.hpp"

class WayfireNotify : public WayfireWidget
{
    static constexpr conf_table_t conf_table[3] = {
        {CONF_BOOL, "enable",   N_("Show notifications")},
        {CONF_INT,  "timeout",  N_("Time before notifications close")},
        {CONF_NONE, NULL,       NULL}
    };

  public:
    static const char *display_name (void) { return N_("Notifications"); };
    static const conf_table_t *config_params (void) { return conf_table; };
};

#endif /* end of include guard: WIDGETS_NOTIFY_HPP */
