#include "../widget.hpp"

extern "C" {
    static constexpr conf_table_t conf_table[3] = {
        {CONF_BOOL, "enable",   N_("Show notifications")},
        {CONF_INT,  "timeout",  N_("Time before notifications close")},
        {CONF_NONE, NULL,       NULL}
    };
    const char *display_name (void) { return N_("Notifications"); };
    const conf_table_t *config_params (void) { return conf_table; };
}
