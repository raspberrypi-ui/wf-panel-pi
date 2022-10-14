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

gboolean set_icon (BluetoothPlugin *bt)
{
    bt_update_display (bt);
    return FALSE;
}

void WayfireBluetooth::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    bt = &data;
    bt->plugin = (GtkWidget *)((*plugin).gobj());
    bt->icon_size = icon_size;
    g_idle_add (G_SOURCE_FUNC (set_icon), bt);
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    bt_init (bt);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireBluetooth::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireBluetooth::bar_pos_changed_cb));
}

WayfireBluetooth::~WayfireBluetooth()
{
}
