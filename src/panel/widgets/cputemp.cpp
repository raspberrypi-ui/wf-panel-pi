#include <glibmm.h>
#include "cputemp.hpp"

extern "C" {
    WayfireWidget *create () { return new WayfireCPUTemp; }
    void destroy (WayfireWidget *w) { delete w; }

    static constexpr conf_table_t conf_table[7] = {
        {CONF_COLOUR,   "foreground",   N_("Foreground colour")},
        {CONF_COLOUR,   "background",   N_("Background colour")},
        {CONF_COLOUR,   "throttle_1",   N_("Colour when ARM frequency capped")},
        {CONF_COLOUR,   "throttle_2",   N_("Colour when throttled")},
        {CONF_INT,      "low_temp",     N_("Lower temperature bound")},
        {CONF_INT,      "high_temp",    N_("Upper temperature bound")},
        {CONF_NONE,     NULL,           NULL}
    };
    const char *display_name (void) { return N_("CPU Temp"); };
    const conf_table_t *config_params (void) { return conf_table; };
}

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

void WayfireCPUTemp::settings_changed_cb (void)
{
    if (!gdk_rgba_parse (&cput->foreground_colour, ((std::string) foreground_colour).c_str()))
        gdk_rgba_parse (&cput->foreground_colour, "dark gray");
    if (!gdk_rgba_parse (&cput->background_colour, ((std::string) background_colour).c_str()))
        gdk_rgba_parse (&cput->background_colour, "light gray");
    if (!gdk_rgba_parse (&cput->low_throttle_colour, ((std::string) throttle1_colour).c_str()))
        gdk_rgba_parse (&cput->low_throttle_colour, "orange");
    if (!gdk_rgba_parse (&cput->high_throttle_colour, ((std::string) throttle2_colour).c_str()))
        gdk_rgba_parse (&cput->high_throttle_colour, "red");

    if (low_temp >= 0 && low_temp <= 100) cput->lower_temp = low_temp;
    else cput->lower_temp = 40;

    if (high_temp >= 0 && high_temp <= 150 && high_temp > cput->lower_temp) cput->upper_temp = high_temp;
    else cput->upper_temp = 90;

    cputemp_update_display (cput);
}

void WayfireCPUTemp::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name (PLUGIN_NAME);
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    memset (&data, 0, sizeof (CPUTempPlugin));
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
    foreground_colour.set_callback (sigc::mem_fun (*this, &WayfireCPUTemp::settings_changed_cb));
    background_colour.set_callback (sigc::mem_fun (*this, &WayfireCPUTemp::settings_changed_cb));
    throttle1_colour.set_callback (sigc::mem_fun (*this, &WayfireCPUTemp::settings_changed_cb));
    throttle2_colour.set_callback (sigc::mem_fun (*this, &WayfireCPUTemp::settings_changed_cb));
    low_temp.set_callback (sigc::mem_fun (*this, &WayfireCPUTemp::settings_changed_cb));
    high_temp.set_callback (sigc::mem_fun (*this, &WayfireCPUTemp::settings_changed_cb));

    settings_changed_cb ();
}

WayfireCPUTemp::~WayfireCPUTemp()
{
    icon_timer.disconnect ();
    cputemp_destructor (cput);
}
