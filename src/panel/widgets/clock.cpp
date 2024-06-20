#include <glibmm.h>
#include <iostream>
#include "clock.hpp"

extern "C" {
    static constexpr conf_table_t conf_table[3] = {
        {CONF_STRING,   "format",   N_("Display format")},
        {CONF_STRING,   "font",     N_("Display font")},
        {CONF_NONE,     NULL,       NULL}
    };
    const char *display_name (void) { return N_("Clock"); };
    const conf_table_t *config_params (void) { return conf_table; };
}

bool WayfireClock::update_label ()
{
    auto time = Glib::DateTime::create_now_local ();
    auto text = time.format ((std::string) format);

    int i = 0;
    while (i < (int) text.length () && text[i] == ' ') i++;

    if (label.get_text () != text.substr (i)) label.set_text (text.substr (i));

    plugin->set_tooltip_text (time.format ("%A %x"));

    return 1;
}

void WayfireClock::set_font ()
{
    if ((std::string) font == "default") label.unset_font ();
    else label.override_font (Pango::FontDescription ((std::string) font));
}

void WayfireClock::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") clock->bottom = TRUE;
    else clock->bottom = FALSE;
}

void WayfireClock::init(Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name (PLUGIN_NAME);
    container->pack_start (*plugin, false, false);

    plugin->add (label);
    plugin->show ();
    label.show ();
    label.set_margin_start (4);
    label.set_margin_end (4);

    /* Setup structure */
    memset (&data, 0, sizeof (ClockPlugin));
    clock = &data;
    clock->plugin = (GtkWidget *)((*plugin).gobj());
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    clock_init (clock);

    /* Initially set font */
    set_font ();
    update_label ();

    /* Setup callbacks */
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfireClock::bar_pos_changed_cb));
    font.set_callback([=] () { set_font (); });
    timeout = Glib::signal_timeout().connect_seconds (sigc::mem_fun (this, &WayfireClock::update_label), 1);
}

WayfireClock::~WayfireClock()
{
    timeout.disconnect ();
}

extern "C" WayfireWidget *create () {
    return new WayfireClock;
}

extern "C" void destroy (WayfireWidget *w) {
    delete w;
}
