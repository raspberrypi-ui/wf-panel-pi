#include <glibmm.h>
#include "power.hpp"

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
    plugin->set_name ("power");
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    pt = &data;
    pt->plugin = (GtkWidget *)((*plugin).gobj());
    pt->icon_size = icon_size;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfirePower::set_icon));
    bar_pos_changed_cb ();

    pt->batt_num = batt_num;

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
