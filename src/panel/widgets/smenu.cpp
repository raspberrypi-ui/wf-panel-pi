#include <glibmm.h>
#include "smenu.hpp"
#include "launchers.hpp"

void WayfireSmenu::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") m->bottom = TRUE;
    else m->bottom = FALSE;
}

void WayfireSmenu::icon_size_changed_cb (void)
{
    m->icon_size = icon_size / LAUNCHERS_ICON_SCALE;
    menu_update_display (m);
}

void WayfireSmenu::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    m = &data;
    m->plugin = (GtkWidget *)((*plugin).gobj());
    m->icon_size = icon_size / LAUNCHERS_ICON_SCALE;

    /* Initialise the plugin */
    menu_init (m);

    /* Setup icon size callback and force an update of the icon */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireSmenu::icon_size_changed_cb));
    icon_size_changed_cb ();

    /* Setup bar position callback */
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireSmenu::bar_pos_changed_cb));
    bar_pos_changed_cb ();
}

WayfireSmenu::~WayfireSmenu()
{
}
