#include <glibmm.h>
#include "kbswitch.hpp"

void WayfireKBSwitch::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") kbs->bottom = TRUE;
    else kbs->bottom = FALSE;
}

void WayfireKBSwitch::icon_size_changed_cb (void)
{
    kbs->icon_size = icon_size;
    kbs_update_display (kbs);
}

void WayfireKBSwitch::command (const char *cmd)
{
    kbs_control_msg (kbs, cmd);
}

bool WayfireKBSwitch::set_icon (void)
{
    kbs_update_display (kbs);
    return false;
}

void WayfireKBSwitch::settings_changed_cb (void)
{
    kbs_update_display (kbs);
}

void WayfireKBSwitch::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name ("kbswitch");
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    kbs = &data;
    kbs->plugin = (GtkWidget *)((*plugin).gobj());
    kbs->icon_size = icon_size;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireKBSwitch::set_icon));
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    kbs_init (kbs);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireKBSwitch::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireKBSwitch::bar_pos_changed_cb));

    settings_changed_cb ();
}

WayfireKBSwitch::~WayfireKBSwitch()
{
    icon_timer.disconnect ();
    kbs_destructor (kbs);
}
