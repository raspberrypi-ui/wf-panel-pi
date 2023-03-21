#include <glibmm.h>
#include "cpu.hpp"

void WayfireCPU::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") cpu->bottom = TRUE;
    else cpu->bottom = FALSE;
}

void WayfireCPU::icon_size_changed_cb (void)
{
    cpu->icon_size = icon_size;
    cpu_update_display (cpu);
}

bool WayfireCPU::set_icon (void)
{
    cpu_update_display (cpu);
    return false;
}

void WayfireCPU::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    cpu = &data;
    cpu->plugin = (GtkWidget *)((*plugin).gobj());
    cpu->icon_size = icon_size;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireCPU::set_icon));
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    cpu_init (cpu);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireCPU::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireCPU::bar_pos_changed_cb));
}

WayfireCPU::~WayfireCPU()
{
    icon_timer.disconnect ();
    cpu_destructor (cpu);
}
