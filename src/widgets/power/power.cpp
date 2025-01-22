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
#include "power.hpp"

extern "C" {
    WayfireWidget *create () { return new WayfirePower; }
    void destroy (WayfireWidget *w) { delete w; }

    static constexpr conf_table_t conf_table[1] = {
        {CONF_NONE, NULL,       NULL}
    };
    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return N_("System Monitor"); };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}

void WayfirePower::bar_pos_changed_cb (void)
{
    if ((std::string) bar_pos == "bottom") pt->bottom = TRUE;
    else pt->bottom = FALSE;
}

void WayfirePower::icon_size_changed_cb (void)
{
    pt->icon_size = icon_size;
    power_update_display (pt);
}

bool WayfirePower::set_icon (void)
{
    power_update_display (pt);
    return false;
}

void WayfirePower::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    plugin->set_name (PLUGIN_NAME);
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    pt = g_new0 (PowerPlugin, 1);
    pt->plugin = (GtkWidget *)((*plugin).gobj());
    pt->icon_size = icon_size;
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfirePower::set_icon));
    bar_pos_changed_cb ();

    /* Initialise the plugin */
    power_init (pt);

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfirePower::icon_size_changed_cb));
    bar_pos.set_callback (sigc::mem_fun (*this, &WayfirePower::bar_pos_changed_cb));
}

WayfirePower::~WayfirePower()
{
    icon_timer.disconnect ();
    power_destructor (pt);
}

/* End of file */
/*----------------------------------------------------------------------------*/
