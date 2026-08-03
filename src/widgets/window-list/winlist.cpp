/*============================================================================
Copyright (c) 2026 Raspberry Pi
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
#include "winlist.hpp"

extern "C" {
    PanelWidget *create () { return new WidgetWinlist; }
    void destroy (PanelWidget *w) { delete w; }

    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return PLUGIN_TITLE; };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}

void WidgetWinlist::command (const char *cmd)
{
    wlist_control_msg (wl, cmd);
}

bool WidgetWinlist::set_icon (void)
{
    wlist_update_display (wl);
    return false;
}

void WidgetWinlist::read_settings (void)
{
    conf_table[0].value = (void *) &wl->max_width;
    conf_table[1].value = (void *) &wl->icons_only;
    conf_table[2].value = (void *) &wl->spacing;

    load_configuration_data (PLUGIN_NAME, conf_table);
}

void WidgetWinlist::handle_config_reload (void)
{
    load_configuration_data (PLUGIN_NAME, conf_table);

    wlist_update_display (wl);
}

void WidgetWinlist::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::ScrolledWindow> ();
    plugin->set_name (PLUGIN_NAME);
    plugin->set_propagate_natural_width (true);
    plugin->set_policy (Gtk::POLICY_EXTERNAL, Gtk::POLICY_NEVER);

    container->pack_start (*plugin, false, false);

    /* Setup structure */
    wl = g_new0 (WinlistPlugin, 1);
    wl->plugin = (GtkWidget *)((*plugin).gobj());
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WidgetWinlist::set_icon));

    /* Initialise the plugin */
    read_settings ();
    wlist_init (wl);
}

WidgetWinlist::~WidgetWinlist()
{
    icon_timer.disconnect ();
    wlist_destructor (wl);
}

/* End of file */
/*----------------------------------------------------------------------------*/
