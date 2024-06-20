#include <glibmm.h>
#include "batt.hpp"

extern "C" {
    WayfireWidget *create () { return new WayfireBatt; }
    void destroy (WayfireWidget *w) { delete w; }

    static constexpr conf_table_t conf_table[2] = {
        {CONF_INT,  "batt_num", N_("Battery number to monitor")},
        {CONF_NONE, NULL,       NULL}
    };
    const char *display_name (void) { return N_("Battery"); };
    const conf_table_t *config_params (void) { return conf_table; };
}

void WayfireBatt::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") pt->bottom = TRUE;
    else pt->bottom = FALSE;
}

void WayfireBatt::icon_size_changed_cb (void)
{
    pt->icon_size = icon_size;
    batt_update_display (pt);
}

bool WayfireBatt::set_icon (void)
{
    batt_update_display (pt);
    return false;
}

void WayfireBatt::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name (PLUGIN_NAME);
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    memset (&data, 0, sizeof (PtBattPlugin));
    pt = &data;
    pt->plugin = (GtkWidget *)((*plugin).gobj());
    pt->icon_size = icon_size;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireBatt::set_icon));
    bar_pos_changed_cb ();

    pt->batt_num = batt_num;

    /* Initialise the plugin */
    batt_init (pt);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireBatt::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireBatt::bar_pos_changed_cb));
}

WayfireBatt::~WayfireBatt()
{
    icon_timer.disconnect ();
    batt_destructor (pt);
}
