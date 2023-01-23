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

void WayfirePower::command (const char *cmd)
{
}

gboolean set_icon (PtBattPlugin *pt)
{
    power_update_display (pt);
    return FALSE;
}

void WayfirePower::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    pt = &data;
    pt->plugin = (GtkWidget *)((*plugin).gobj());
    pt->icon_size = icon_size;
    g_idle_add (G_SOURCE_FUNC (set_icon), pt);
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    power_init (pt);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfirePower::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfirePower::bar_pos_changed_cb));
}

WayfirePower::~WayfirePower()
{
}
