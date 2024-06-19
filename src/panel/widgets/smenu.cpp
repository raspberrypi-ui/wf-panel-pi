#include <glibmm.h>
#include "smenu.hpp"

extern "C" {
    static constexpr conf_table_t conf_table[3] = {
        {CONF_INT,  "search_height",    N_("Search window height")},
        {CONF_BOOL, "search_fixed",     N_("Fix size of search window")},
        {CONF_NONE, NULL,               NULL}
    };
    const char *display_name (void) { return N_("Menu"); };
    const conf_table_t *config_params (void) { return conf_table; };
}

void WayfireSmenu::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") m->bottom = TRUE;
    else m->bottom = FALSE;
}

void WayfireSmenu::icon_size_changed_cb (void)
{
    m->icon_size = icon_size;
    menu_update_display (m);
}

void WayfireSmenu::search_param_changed_cb (void)
{
    m->height = search_height;
    m->fixed = search_fixed;
}

void WayfireSmenu::command (const char *cmd)
{
    if (!g_strcmp0 (cmd, "menu")) menu_show_menu (m);
}

bool WayfireSmenu::set_icon (void)
{
    menu_update_display (m);
    return false;
}

void WayfireSmenu::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name ("smenu");
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    memset (&data, 0, sizeof (MenuPlugin));
    m = &data;
    m->plugin = (GtkWidget *)((*plugin).gobj());
    m->icon_size = icon_size;
    m->height = search_height;
    m->fixed = search_fixed;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireSmenu::set_icon));
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    menu_init (m);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireSmenu::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireSmenu::bar_pos_changed_cb));
    search_height.set_callback (sigc::mem_fun (*this, &WayfireSmenu::search_param_changed_cb));
    search_fixed.set_callback (sigc::mem_fun (*this, &WayfireSmenu::search_param_changed_cb));
}

WayfireSmenu::~WayfireSmenu()
{
    icon_timer.disconnect ();
    menu_destructor (m);
}

extern "C" WayfireWidget *create () {
    return new WayfireSmenu;
}

extern "C" void destroy (WayfireWidget *w) {
    delete w;
}
