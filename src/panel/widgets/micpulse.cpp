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

void WayfireMicpulse::command (const char *cmd)
{
    volumepulse_control_msg (vol, cmd);
}

bool WayfireMicpulse::set_icon (void)
{
    micpulse_update_display (vol);
    return false;
}

void WayfireMicpulse::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <WayfireMenuButton> ("panel");
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    vol = &data;
    vol->plugin = (GtkWidget *)((*plugin).gobj());
    vol->icon_size = icon_size;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireMicpulse::set_icon));
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    micpulse_init (vol);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireMicpulse::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireMicpulse::bar_pos_changed_cb));
}

WayfireMicpulse::~WayfireMicpulse()
{
    icon_timer.disconnect ();
    micpulse_destructor (vol);
}
