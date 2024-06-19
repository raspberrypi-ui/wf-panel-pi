#include <glibmm.h>
#include "volumepulse.hpp"

extern "C" {
    static constexpr conf_table_t conf_table[1] = {
        {CONF_NONE, NULL, NULL}
    };
    const char *display_name (void) { return N_("Volume"); };
    const conf_table_t *config_params (void) { return conf_table; };
    const char *plugin_name = "volumepulse";
}

void WayfireVolumepulse::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") vol->bottom = TRUE;
    else vol->bottom = FALSE;
}

void WayfireVolumepulse::icon_size_changed_cb (void)
{
    vol->icon_size = icon_size;
    volumepulse_update_display (vol);
    micpulse_update_display (vol);
}

void WayfireVolumepulse::command (const char *cmd)
{
    volumepulse_control_msg (vol, cmd);
}

bool WayfireVolumepulse::set_icon (void)
{
    volumepulse_update_display (vol);
    micpulse_update_display (vol);
    return false;
}

void WayfireVolumepulse::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin_vol = std::make_unique <Gtk::Button> ();
    plugin_vol->set_name (plugin_name);
    container->pack_start (*plugin_vol, false, false);
    plugin_mic = std::make_unique <Gtk::Button> ();
    plugin_mic->set_name (plugin_name);
    container->pack_start (*plugin_mic, false, false);

    /* Setup structure */
    memset (&data, 0, sizeof (VolumePulsePlugin));
    vol = &data;
    vol->plugin[0] = (GtkWidget *)((*plugin_vol).gobj());
    vol->plugin[1] = (GtkWidget *)((*plugin_mic).gobj());
    vol->icon_size = icon_size;
    vol->wizard = WayfireShellApp::get().wizard;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireVolumepulse::set_icon));
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    volumepulse_init (vol);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireVolumepulse::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireVolumepulse::bar_pos_changed_cb));
}

WayfireVolumepulse::~WayfireVolumepulse()
{
    icon_timer.disconnect ();
    volumepulse_destructor (vol);
}

extern "C" WayfireWidget *create () {
    return new WayfireVolumepulse;
}

extern "C" void destroy (WayfireWidget *w) {
    delete w;
}
