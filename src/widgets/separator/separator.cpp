#include <widget.hpp>

extern "C" {
    static constexpr conf_table_t conf_table[1] = {
        {CONF_TYPE_NONE,    NULL,   NULL,   NULL,   NULL}
    };
    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return N_("Separator"); };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}
