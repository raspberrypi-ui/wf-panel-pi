#include <widget.hpp>

extern "C" {
    static constexpr conf_table_t conf_table[2] = {
        {CONF_INT,  "width",    N_("Width in pixels")},
        {CONF_NONE, NULL,       NULL}
    };
    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return N_("Spacer"); };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}
