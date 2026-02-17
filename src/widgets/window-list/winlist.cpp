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
#include "gtk-utils.hpp"
#include "winlist.hpp"

extern "C" {
    WayfireWidget *create () { return new WayfireWinlist; }
    void destroy (WayfireWidget *w) { delete w; }

    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return PLUGIN_TITLE; };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}

void WayfireWinlist::command (const char *cmd)
{
    wlist_control_msg (wl, cmd);
}

bool WayfireWinlist::set_icon (void)
{
    wlist_update_display (wl);
    return false;
}

void WayfireWinlist::read_settings (void)
{
    wl->spacing = spacing;
    wl->max_width = max_width;
    wl->icons_only = icons_only;
}

void WayfireWinlist::settings_changed_cb (void)
{
    read_settings ();
    wlist_update_display (wl);
}

void WayfireWinlist::init (Gtk::HBox *container)
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
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireWinlist::set_icon));

    /* Initialise the plugin */
    read_settings ();
    wlist_init (wl);

    /* Setup callbacks */
    spacing.set_callback (sigc::mem_fun (*this, &WayfireWinlist::settings_changed_cb));
    max_width.set_callback (sigc::mem_fun (*this, &WayfireWinlist::settings_changed_cb));
    icons_only.set_callback (sigc::mem_fun (*this, &WayfireWinlist::settings_changed_cb));
}

WayfireWinlist::~WayfireWinlist()
{
    icon_timer.disconnect ();
    wlist_destructor (wl);
}

/* End of file */
/*----------------------------------------------------------------------------*/
