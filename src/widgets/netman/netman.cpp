#include <glibmm.h>
#include "netman.hpp"

extern "C" {
    WayfireWidget *create () { return new WayfireNetman; }
    void destroy (WayfireWidget *w) { delete w; }

    static constexpr conf_table_t conf_table[1] = {
        {CONF_NONE, NULL, NULL}
    };
    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { textdomain (GETTEXT_PACKAGE); return _("Network"); };
}

void WayfireNetman::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") nm->bottom = TRUE;
    else nm->bottom = FALSE;
}

void WayfireNetman::icon_size_changed_cb (void)
{
    nm->icon_size = icon_size;
    netman_update_display (nm);
}

void WayfireNetman::command (const char *cmd)
{
    nm_control_msg (nm, cmd);
}

bool WayfireNetman::set_icon (void)
{
    netman_update_display (nm);
    return false;
}

void WayfireNetman::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name (PLUGIN_NAME);
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    nm = (NMApplet *) g_object_new (NM_TYPE_APPLET, NULL);
    nm->plugin = (GtkWidget *)((*plugin).gobj());
    nm->icon_size = icon_size;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireNetman::set_icon));
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    netman_init (nm);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireNetman::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireNetman::bar_pos_changed_cb));
}

WayfireNetman::~WayfireNetman()
{
    icon_timer.disconnect ();
    netman_destructor (nm);
}
