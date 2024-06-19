#include <glibmm.h>
#include "power.hpp"

extern "C" {
    static constexpr conf_table_t conf_table[1] = {
        {CONF_NONE, NULL,       NULL}
    };
    const char *display_name (void) { return N_("Power"); };
    const conf_table_t *config_params (void) { return conf_table; };
    const char *plugin_name = "power";
}

void WayfirePower::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") pt->bottom = TRUE;
    else pt->bottom = FALSE;
}

void WayfirePower::icon_size_changed_cb (void)
{
    pt->icon_size = icon_size;
    power_update_display (pt);
}

bool WayfirePower::set_icon (void)
{
    power_update_display (pt);
    return false;
}

void WayfirePower::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name (plugin_name);
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    memset (&data, 0, sizeof (PowerPlugin));
    pt = &data;
    pt->plugin = (GtkWidget *)((*plugin).gobj());
    pt->icon_size = icon_size;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfirePower::set_icon));
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    power_init (pt);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfirePower::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfirePower::bar_pos_changed_cb));
}

WayfirePower::~WayfirePower()
{
    icon_timer.disconnect ();
    power_destructor (pt);
}

extern "C" WayfireWidget *create () {
    return new WayfirePower;
}

extern "C" void destroy (WayfireWidget *w) {
    delete w;
}
