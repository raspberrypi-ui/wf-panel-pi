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

gboolean set_icon (CPUPlugin *cpu)
{
    cpu_update_display (cpu);
    return FALSE;
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
    g_idle_add (G_SOURCE_FUNC (set_icon), cpu);
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    cpu_init (cpu);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireCPU::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireCPU::bar_pos_changed_cb));
}

WayfireCPU::~WayfireCPU()
{
}
