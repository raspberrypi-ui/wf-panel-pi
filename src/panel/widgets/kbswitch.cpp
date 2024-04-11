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
    int i;

    for (i = 0; i < MAX_KBDS; i++) g_free (kbs->kbds[i]);
    kbs->kbds[0] = g_strdup (((std::string) keyboard_0).c_str ());
    kbs->kbds[1] = g_strdup (((std::string) keyboard_1).c_str ());
    kbs->kbds[2] = g_strdup (((std::string) keyboard_2).c_str ());
    kbs->kbds[3] = g_strdup (((std::string) keyboard_3).c_str ());
    kbs->kbds[4] = g_strdup (((std::string) keyboard_4).c_str ());

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
    keyboard_0.set_callback (sigc::mem_fun (*this, &WayfireKBSwitch::settings_changed_cb));
    keyboard_1.set_callback (sigc::mem_fun (*this, &WayfireKBSwitch::settings_changed_cb));
    keyboard_2.set_callback (sigc::mem_fun (*this, &WayfireKBSwitch::settings_changed_cb));
    keyboard_3.set_callback (sigc::mem_fun (*this, &WayfireKBSwitch::settings_changed_cb));
    keyboard_4.set_callback (sigc::mem_fun (*this, &WayfireKBSwitch::settings_changed_cb));

    settings_changed_cb ();
}

WayfireKBSwitch::~WayfireKBSwitch()
{
    icon_timer.disconnect ();
    kbs_destructor (kbs);
}
