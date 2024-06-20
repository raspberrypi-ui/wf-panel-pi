#include "tray.hpp"

extern "C" {
    static constexpr conf_table_t conf_table[3] = {
        {CONF_INT,  "smooth_scrolling_threshold",  N_("Smooth scrolling threshold")},
        {CONF_BOOL, "menu_on_middle_click",        N_("Middle button activates menu")},
        {CONF_NONE, NULL,               NULL}
    };
    const char *display_name (void) { return N_("System Tray"); };
    const conf_table_t *config_params (void) { return conf_table; };
}

void WayfireStatusNotifier::init(Gtk::HBox *container)
{
    icons_hbox.set_name (PLUGIN_NAME);
    icons_hbox.set_spacing(5);
    container->add(icons_hbox);
}

void WayfireStatusNotifier::add_item(const Glib::ustring & service)
{
    if (items.count(service) != 0)
    {
        return;
    }

    items.emplace(service, service);
    icons_hbox.pack_start(items.at(service));
    icons_hbox.show_all();
}

void WayfireStatusNotifier::remove_item(const Glib::ustring & service)
{
    items.erase(service);
}

extern "C" WayfireWidget *create () {
    return new WayfireStatusNotifier;
}

extern "C" void destroy (WayfireWidget *w) {
    delete w;
}
