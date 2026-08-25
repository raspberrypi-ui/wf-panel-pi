#include "tray.hpp"

extern "C" {
#include "plugin.h"

    PanelWidget *create () { return new WidgetStatusNotifier; }
    void destroy (PanelWidget *w) { delete w; }

    static conf_table_t conf_table[3] = {
        {CONF_TYPE_INT,     "smooth_scrolling_threshold",   N_("Smooth scrolling threshold"),   NULL,   "5"     },
        {CONF_TYPE_BOOL,    "menu_on_middle_click",         N_("Middle button activates menu"), NULL,   "false" },
        {CONF_TYPE_NONE,    NULL,                           NULL,                               NULL,   NULL    }
    };
    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return N_("System Tray"); };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
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

void WidgetStatusNotifier::widget_set_icon (void)
{
    for (auto &p : items) p.second.update_icon ();
}

void WidgetStatusNotifier::widget_config_reload (void)
{
    if (load_configuration_data (PLUGIN_NAME, conf_table))
        for (auto &p : items) p.second.set_params (momc, sst);
}

void WidgetStatusNotifier::widget_init (Gtk::HBox *container)
{
    icons_hbox.set_name (PLUGIN_NAME);
    icons_hbox.set_spacing (5);
    container->add (icons_hbox);

    conf_table[0].value = (void **) &sst;
    conf_table[1].value = (void **) &momc;

    load_configuration_data (PLUGIN_NAME, conf_table);
}
