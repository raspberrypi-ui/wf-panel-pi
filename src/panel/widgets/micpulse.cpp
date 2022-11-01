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

gboolean mic_set_icon (VolumePulsePlugin *vol)
{
    micpulse_update_display (vol);
    return FALSE;
}

void WayfireMicpulse::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    vol = &data;
    vol->plugin = (GtkWidget *)((*plugin).gobj());
    vol->icon_size = icon_size;
    g_idle_add (G_SOURCE_FUNC (mic_set_icon), vol);
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    micpulse_init (vol);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireMicpulse::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireMicpulse::bar_pos_changed_cb));
}

WayfireMicpulse::~WayfireMicpulse()
{
}
