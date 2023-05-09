#include <glibmm.h>
#include "gpu.hpp"

void WayfireGPU::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") gpu->bottom = TRUE;
    else gpu->bottom = FALSE;
}

void WayfireGPU::icon_size_changed_cb (void)
{
    gpu->icon_size = icon_size;
    gpu_update_display (gpu);
}

bool WayfireGPU::set_icon (void)
{
    gpu_update_display (gpu);
    return false;
}

void WayfireGPU::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    gpu = &data;
    gpu->plugin = (GtkWidget *)((*plugin).gobj());
    gpu->icon_size = icon_size;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireGPU::set_icon));
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    gpu_init (gpu);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireGPU::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireGPU::bar_pos_changed_cb));
}

WayfireGPU::~WayfireGPU()
{
    icon_timer.disconnect ();
    gpu_destructor (gpu);
}
