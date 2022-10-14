#include <glibmm.h>
#include "micpulse.hpp"

void WayfireMicpulse::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") vol->bottom = TRUE;
    else vol->bottom = FALSE;
}

void WayfireMicpulse::icon_size_changed_cb (void)
{
    vol->icon_size = icon_size;
    micpulse_update_display (vol);
}

void WayfireMicpulse::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    vol = &data;
    vol->plugin = (GtkWidget *)((*plugin).gobj());

    /* Initialise the plugin */
    micpulse_init (vol);

    /* Setup icon size callback and force an update of the icon */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireMicpulse::icon_size_changed_cb));
    icon_size_changed_cb ();

    /* Setup bar position callback */
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireMicpulse::bar_pos_changed_cb));
    bar_pos_changed_cb ();
}

WayfireMicpulse::~WayfireMicpulse()
{
}
