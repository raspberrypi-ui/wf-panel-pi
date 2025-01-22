/*============================================================================
Copyright (c) 2024 Raspberry Pi Holdings Ltd.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================*/

#include <glibmm.h>
#include <iostream>
#include "clock.hpp"

extern "C" {
#include "lxutils.h"

    WayfireWidget *create () { return new WayfireClock; }
    void destroy (WayfireWidget *w) { delete w; }

    static constexpr conf_table_t conf_table[3] = {
        {CONF_STRING,   "format",   N_("Display format")},
        {CONF_STRING,   "font",     N_("Display font")},
        {CONF_NONE,     NULL,       NULL}
    };
    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return N_("Clock"); };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}

bool WayfireClock::update_label ()
{
    auto time = Glib::DateTime::create_now_local ();
    auto text = time.format ((std::string) format);

    int i = 0;
    while (i < (int) text.length () && text[i] == ' ') i++;

    if (label->get_text () != text.substr (i)) label->set_text (text.substr (i));

    plugin->set_tooltip_text (time.format ("%A %x"));

    return true;
}

bool WayfireClock::on_pressed (GdkEventButton *ev)
{
    if (window && window->is_visible()) popup_shown = true;
    else popup_shown = false;
    return false;
}

void WayfireClock::on_clicked ()
{
    CHECK_LONGPRESS
    if (!popup_shown) show_calendar ();
}

void WayfireClock::show_calendar ()
{
    /* Create the window */
    window = std::make_unique <Gtk::Window> ();
    window->set_border_width (1);
    window->set_name ("panelpopup");

    /* Create a calendar and add to the window */
    calendar = std::make_unique <Gtk::Calendar> ();
    window->add (*calendar);

    /* Realise the window */
    popup_window_at_button (GTK_WIDGET (window->gobj()), GTK_WIDGET (plugin->gobj()));
}

void WayfireClock::set_font ()
{
    if ((std::string) font == "default") label->unset_font ();
    else label->override_font (Pango::FontDescription ((std::string) font));
}

void WayfireClock::config_changed_cb ()
{
    set_font ();
    update_label ();
}

void WayfireClock::init(Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name (PLUGIN_NAME);
    plugin->set_relief (Gtk::RELIEF_NONE);
    plugin->show ();

    /* Create the label */
    label = std::make_unique <Gtk::Label> ();
    label->set_margin_start (4);
    label->set_margin_end (4);
    label->show ();

    plugin->add (*label);
    container->pack_start (*plugin, false, false);

    /* Initially set font */
    set_font ();
    update_label ();

    /* Setup callbacks */
    font.set_callback (sigc::mem_fun (*this, &WayfireClock::config_changed_cb));
    format.set_callback (sigc::mem_fun (*this, &WayfireClock::config_changed_cb));
    timeout = Glib::signal_timeout().connect_seconds (sigc::mem_fun (this, &WayfireClock::update_label), 1);

    /* Setup button and gesture handlers */
    plugin->signal_button_press_event().connect(sigc::mem_fun(this, &WayfireClock::on_pressed), false);
    plugin->signal_clicked().connect(sigc::mem_fun(this, &WayfireClock::on_clicked));

    GtkGestureLongPress *ggest = (GtkGestureLongPress *) add_long_press (GTK_WIDGET (plugin->gobj()), NULL, NULL);
    gesture = Glib::wrap (ggest);
}

WayfireClock::~WayfireClock()
{
    timeout.disconnect ();
}

/* End of file */
/*----------------------------------------------------------------------------*/
