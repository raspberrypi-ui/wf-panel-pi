#include <glibmm.h>
#include "bluetooth.hpp"

void WayfireBluetooth::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") bt->bottom = TRUE;
    else bt->bottom = FALSE;
}

void WayfireBluetooth::icon_size_changed_cb (void)
{
    bt->icon_size = icon_size;
    bt_update_display (bt);
}

void WayfireBluetooth::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    bt = &data;
    bt->plugin = (GtkWidget *)((*plugin).gobj());

    /* Initialise the plugin */
    bt_init (bt);

    /* Setup icon size callback and force an update of the icon */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireBluetooth::icon_size_changed_cb));
    icon_size_changed_cb ();

    /* Setup bar position callback */
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireBluetooth::bar_pos_changed_cb));
    bar_pos_changed_cb ();
}

WayfireBluetooth::~WayfireBluetooth()
{
}
