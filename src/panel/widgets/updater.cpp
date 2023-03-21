#include <glibmm.h>
#include "updater.hpp"

void WayfireUpdater::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") up->bottom = TRUE;
    else up->bottom = FALSE;
}

void WayfireUpdater::icon_size_changed_cb (void)
{
    up->icon_size = icon_size;
    updater_update_display (up);
}

void WayfireUpdater::command (const char *cmd)
{
    updater_control_msg (up, cmd);
}

bool WayfireUpdater::set_icon (void)
{
    updater_update_display (up);
    return false;
}

void WayfireUpdater::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    up = &data;
    up->plugin = (GtkWidget *)((*plugin).gobj());
    up->icon_size = icon_size;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireUpdater::set_icon));
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    updater_init (up);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireUpdater::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireUpdater::bar_pos_changed_cb));
}

WayfireUpdater::~WayfireUpdater()
{
    icon_timer.disconnect ();
    updater_destructor (up);
}
