#include <glibmm.h>
#include "testxml.hpp"

extern "C" {
#include "lxutils.h"
}


void WayfireXMLPlugin::bar_pos_changed_cb (void)
{
    //if ((std::string) bar_pos == "bottom") up->bottom = TRUE;
    //else up->bottom = FALSE;
}

void WayfireXMLPlugin::icon_size_changed_cb (void)
{
    switch (icon_size)
    {
        case 16 :   icon->set_from_icon_name ("input-keyboard", Gtk::ICON_SIZE_SMALL_TOOLBAR);
                    break;
        case 24 :   icon->set_from_icon_name ("input-keyboard", Gtk::ICON_SIZE_LARGE_TOOLBAR);
                    break;
        case 32 :   icon->set_from_icon_name ("input-keyboard", Gtk::ICON_SIZE_DND);
                    break;
        case 48 :   icon->set_from_icon_name ("input-keyboard", Gtk::ICON_SIZE_DIALOG);
                    break;
    }
}

void WayfireXMLPlugin::command (const char *cmd)
{
    //updater_control_msg (up, cmd);
}

bool WayfireXMLPlugin::set_icon (void)
{
    icon_size_changed_cb ();
    return false;
}

void WayfireXMLPlugin::on_button_press_event (void)
{
    show_menu_with_kbd (GTK_WIDGET (plugin.get()->gobj()), GTK_WIDGET (menu.get()->gobj()));
}

void WayfireXMLPlugin::do_menu (void)
{
    printf ("menu activated\n");
}

void WayfireXMLPlugin::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    container->pack_start (*plugin, false, false);

    /* Create the icon */
    icon = std::make_unique <Gtk::Image> ();
    plugin->add (*icon.get());
    plugin->signal_clicked().connect (sigc::mem_fun (*this, &WayfireXMLPlugin::on_button_press_event));

    /* Create the menu */
    menu = std::make_unique <Gtk::Menu> ();
    item = std::make_unique <Gtk::MenuItem> ();

    item->set_label ("Item 1");
    item->signal_activate().connect(sigc::mem_fun(this, &WayfireXMLPlugin::do_menu));
    menu->attach (*item.get(), 0, 1, 0, 1);
    menu->attach_to_widget (*plugin.get());
    menu->show_all ();

    /* Setup structure */
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireXMLPlugin::set_icon));
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    //updater_init (up);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireXMLPlugin::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireXMLPlugin::bar_pos_changed_cb));
}

WayfireXMLPlugin::~WayfireXMLPlugin()
{
    icon_timer.disconnect ();
}
