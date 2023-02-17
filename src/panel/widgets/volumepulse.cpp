#include <glibmm.h>
#include "volumepulse.hpp"

void WayfireVolumepulse::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") vol->bottom = TRUE;
    else vol->bottom = FALSE;
}

void WayfireVolumepulse::icon_size_changed_cb (void)
{
    vol->icon_size = icon_size;
    volpulse_update_display (vol);
}

void WayfireVolumepulse::command (const char *cmd)
{
    volumepulse_control_msg (vol, cmd);
}

void WayfireVolumepulse::set_wizard (void)
{
    vol->wizard = TRUE;
}

gboolean vol_set_icon (VolumePulsePlugin *vol)
{
    volpulse_update_display (vol);
    return FALSE;
}

void WayfireVolumepulse::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    vol = &data;
    vol->plugin = (GtkWidget *)((*plugin).gobj());
    vol->icon_size = icon_size;
    g_idle_add (G_SOURCE_FUNC (vol_set_icon), vol);
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    volumepulse_init (vol);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireVolumepulse::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireVolumepulse::bar_pos_changed_cb));
}

WayfireVolumepulse::~WayfireVolumepulse()
{
}
