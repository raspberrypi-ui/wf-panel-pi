#include <glibmm.h>
#include "ejecter.hpp"

extern "C" {
    WayfireWidget *create () { return new WayfireEjecter; }
    void destroy (WayfireWidget *w) { delete w; }

    static constexpr conf_table_t conf_table[2] = {
        {CONF_BOOL, "autohide", N_("Hide icon when no devices")},
        {CONF_NONE,  NULL,       NULL}
    };
    const char *display_name (void) { return N_("Ejecter"); };
    const conf_table_t *config_params (void) { return conf_table; };
}

void WayfireEjecter::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") ej->bottom = TRUE;
    else ej->bottom = FALSE;
}

void WayfireEjecter::icon_size_changed_cb (void)
{
    ej->icon_size = icon_size;
    ej_update_display (ej);
}

void WayfireEjecter::command (const char *cmd)
{
    ejecter_control_msg (ej, cmd);
}

bool WayfireEjecter::set_icon (void)
{
    ej_update_display (ej);
    return false;
}

void WayfireEjecter::settings_changed_cb (void)
{
    ej->autohide = autohide;
    ej_update_display (ej);
}

void WayfireEjecter::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name (PLUGIN_NAME);
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    ej = (EjecterPlugin *) calloc (1, sizeof (EjecterPlugin));
    ej->plugin = (GtkWidget *)((*plugin).gobj());
    ej->icon_size = icon_size;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireEjecter::set_icon));
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    ej_init (ej);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireEjecter::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireEjecter::bar_pos_changed_cb));
    autohide.set_callback (sigc::mem_fun (*this, &WayfireEjecter::settings_changed_cb));

    settings_changed_cb ();
}

WayfireEjecter::~WayfireEjecter()
{
    icon_timer.disconnect ();
    ejecter_destructor (ej);
}
