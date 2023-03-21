#include <glibmm.h>
#include "cputemp.hpp"

void WayfireCPUTemp::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") cput->bottom = TRUE;
    else cput->bottom = FALSE;
}

void WayfireCPUTemp::icon_size_changed_cb (void)
{
    cput->icon_size = icon_size;
    cputemp_update_display (cput);
}

bool WayfireCPUTemp::set_icon (void)
{
    cputemp_update_display (cput);
    return false;
}

void WayfireCPUTemp::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    cput = &data;
    cput->plugin = (GtkWidget *)((*plugin).gobj());
    cput->icon_size = icon_size;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireCPUTemp::set_icon));
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    cputemp_init (cput);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireCPUTemp::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireCPUTemp::bar_pos_changed_cb));
}

WayfireCPUTemp::~WayfireCPUTemp()
{
    icon_timer.disconnect ();
    cputemp_destructor (cput);
}
