#include "tray.hpp"

extern "C" {
#include "lxutils.h"

    PanelWidget *create () { return new WidgetStatusNotifier; }
    void destroy (PanelWidget *w) { delete w; }

    static conf_table_t conf_table[3] = {
        {CONF_TYPE_INT,     "smooth_scrolling_threshold",   N_("Smooth scrolling threshold"),   NULL},
        {CONF_TYPE_BOOL,    "menu_on_middle_click",         N_("Middle button activates menu"), NULL},
        {CONF_TYPE_NONE,    NULL,                           NULL,                               NULL}
    };
    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return N_("System Tray"); };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}

void WidgetStatusNotifier::init (Gtk::HBox *container)
{
    icons_hbox.set_name (PLUGIN_NAME);
    icons_hbox.set_spacing(5);
    container->add(icons_hbox);

    read_settings ();
}

void WidgetStatusNotifier::add_item (const Glib::ustring & service)
{
    if (items.count(service) != 0)
    {
        return;
    }

    items.emplace(service, service);
    icons_hbox.pack_start(items.at(service));
    icons_hbox.show_all();
    for (auto &p : items) p.second.set_params (momc, sst);  // there's probably a better way of doing this...
}

void WidgetStatusNotifier::remove_item (const Glib::ustring & service)
{
    items.erase(service);
    if (items.count(service) == 0) icons_hbox.hide();
}

bool WidgetStatusNotifier::set_icon (void)
{
    for (auto &p : items) p.second.update_icon ();
    return false;
}

void WidgetStatusNotifier::read_settings (void)
{
    conf_table[0].value = (void *) &sst;
    conf_table[1].value = (void *) &momc;

    load_configuration_data (PLUGIN_NAME, conf_table);
}

void WidgetStatusNotifier::handle_config_reload (void)
{
    load_configuration_data (PLUGIN_NAME, conf_table);
    for (auto &p : items) p.second.set_params (momc, sst);
}
