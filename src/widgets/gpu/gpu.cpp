#include <glibmm.h>
#include "gpu.hpp"

extern "C" {
    WayfireWidget *create () { return new WayfireGPU; }
    void destroy (WayfireWidget *w) { delete w; }

    static constexpr conf_table_t conf_table[4] = {
        {CONF_BOOL,     "show_percentage",  N_("Show usage as percentage")},
        {CONF_COLOUR,   "foreground",       N_("Foreground colour")},
        {CONF_COLOUR,   "background",       N_("Background colour")},
        {CONF_NONE,     NULL,               NULL}
    };
    const char *display_name (void) { return N_("GPU"); };
    const conf_table_t *config_params (void) { return conf_table; };
}

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

void WayfireGPU::settings_changed_cb (void)
{
    gpu->show_percentage = show_percentage;
    if (!gdk_rgba_parse (&gpu->foreground_colour, ((std::string) foreground_colour).c_str()))
        gdk_rgba_parse (&gpu->foreground_colour, "dark gray");
    if (!gdk_rgba_parse (&gpu->background_colour, ((std::string) background_colour).c_str()))
        gdk_rgba_parse (&gpu->background_colour, "light gray");
    gpu_update_display (gpu);
}

void WayfireGPU::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name (PLUGIN_NAME);
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    memset (&data, 0, sizeof (GPUPlugin));
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
    show_percentage.set_callback (sigc::mem_fun (*this, &WayfireGPU::settings_changed_cb));
    foreground_colour.set_callback (sigc::mem_fun (*this, &WayfireGPU::settings_changed_cb));
    background_colour.set_callback (sigc::mem_fun (*this, &WayfireGPU::settings_changed_cb));

    settings_changed_cb ();
}

WayfireGPU::~WayfireGPU()
{
    icon_timer.disconnect ();
    gpu_destructor (gpu);
}
