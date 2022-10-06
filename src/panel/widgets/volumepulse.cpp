#include <glibmm.h>
#include "volumepulse.hpp"
#include "launchers.hpp"

extern "C" {
#include "volumepulse/pulse.h"
#include "volumepulse/commongui.h"
#include "volumepulse/bluetooth.h"
}

void WayfireVolumepulse::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") vol->bottom = TRUE;
    else vol->bottom = FALSE;
}

void WayfireVolumepulse::icon_size_changed_cb (void)
{
    vol->icon_size = icon_size / LAUNCHERS_ICON_SCALE;
    volumepulse_update_display (vol);
}

void WayfireVolumepulse::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    vol = &data;
    vol->plugin = (GtkWidget *)((*plugin).gobj());

    /* Initialise the plugin */
    volumepulse_init (vol);

    /* Setup icon size callback and force an update of the icon */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireVolumepulse::icon_size_changed_cb));
    icon_size_changed_cb ();

    /* Setup bar position callback */
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireVolumepulse::bar_pos_changed_cb));
    bar_pos_changed_cb ();
}

WayfireVolumepulse::~WayfireVolumepulse()
{
}
