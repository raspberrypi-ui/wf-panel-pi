#include <widget.hpp>

extern "C" {
    static constexpr conf_table_t conf_table[3] = {
        {CONF_TYPE_BOOL,    "enable",   N_("Show notifications"),               NULL},
        {CONF_TYPE_INT,     "timeout",  N_("Time before notifications close"),  NULL},
        {CONF_TYPE_NONE,    NULL,       NULL,                                   NULL}
    };
    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return N_("Notifications"); };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}
