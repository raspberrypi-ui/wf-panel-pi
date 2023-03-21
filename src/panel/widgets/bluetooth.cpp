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

void WayfireBluetooth::command (const char *cmd)
{
    bluetooth_control_msg (bt, cmd);
}

bool WayfireBluetooth::set_icon (void)
{
    bt_update_display (bt);
    return false;
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
    bt->wizard = WayfireShellApp::get().wizard;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireBluetooth::set_icon));
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    bt_init (bt);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireBluetooth::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireBluetooth::bar_pos_changed_cb));
}

WayfireBluetooth::~WayfireBluetooth()
{
    icon_timer.disconnect ();
    bluetooth_destructor (bt);
}
