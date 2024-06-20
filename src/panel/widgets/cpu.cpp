#include <glibmm.h>
#include "cpu.hpp"

extern "C" {
    static constexpr conf_table_t conf_table[4] = {
        {CONF_BOOL,     "show_percentage",  N_("Show usage as percentage")},
        {CONF_COLOUR,   "foreground",       N_("Foreground colour")},
        {CONF_COLOUR,   "background",       N_("Background colour")},
        {CONF_NONE,     NULL,               NULL}
    };
    const char *display_name (void) { return N_("CPU"); };
    const conf_table_t *config_params (void) { return conf_table; };
}

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

void WayfireCPU::settings_changed_cb (void)
{
    cpu->show_percentage = show_percentage;
    if (!gdk_rgba_parse (&cpu->foreground_colour, ((std::string) foreground_colour).c_str()))
        gdk_rgba_parse (&cpu->foreground_colour, "dark gray");
    if (!gdk_rgba_parse (&cpu->background_colour, ((std::string) background_colour).c_str()))
        gdk_rgba_parse (&cpu->background_colour, "light gray");
    cpu_update_display (cpu);
}

void WayfireCPU::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name (PLUGIN_NAME);
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    memset (&data, 0, sizeof (CPUPlugin));
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
    show_percentage.set_callback (sigc::mem_fun (*this, &WayfireCPU::settings_changed_cb));
    foreground_colour.set_callback (sigc::mem_fun (*this, &WayfireCPU::settings_changed_cb));
    background_colour.set_callback (sigc::mem_fun (*this, &WayfireCPU::settings_changed_cb));

    settings_changed_cb ();
}

WayfireCPU::~WayfireCPU()
{
    icon_timer.disconnect ();
    cpu_destructor (cpu);
}

extern "C" WayfireWidget *create () {
    return new WayfireCPU;
}

extern "C" void destroy (WayfireWidget *w) {
    delete w;
}
