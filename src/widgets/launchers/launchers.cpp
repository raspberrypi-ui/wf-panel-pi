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
#include "launchers.hpp"

extern "C" {
    WayfireWidget *create () { return new WayfireLauncher; }
    void destroy (WayfireWidget *w) { delete w; }

    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return PLUGIN_TITLE; };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}

void WayfireLauncher::command (const char *cmd)
{
    launcher_control_msg (lch, cmd);
}

bool WayfireLauncher::set_icon (void)
{
    launcher_update_display (lch);
    return false;
}

void WayfireLauncher::read_settings (void)
{
    lch->spacing = spacing;
    lch->launchers = g_strdup (((std::string) launchers).c_str());
}

void WayfireLauncher::settings_changed_cb (void)
{
    read_settings ();
    launcher_update_display (lch);
}

void WayfireLauncher::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::HBox> ();
    plugin->set_name (PLUGIN_NAME);
    container->pack_start (*plugin, false, false);

    /* Setup structure */
    lch = g_new0 (LauncherPlugin, 1);
    lch->plugin = (GtkWidget *)((*plugin).gobj());
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireLauncher::set_icon));

    /* Initialise the plugin */
    read_settings ();
    launcher_init (lch);

    /* Setup callbacks */
    spacing.set_callback (sigc::mem_fun (*this, &WayfireLauncher::settings_changed_cb));
    launchers.set_callback (sigc::mem_fun (*this, &WayfireLauncher::settings_changed_cb));
}

WayfireLauncher::~WayfireLauncher()
{
    icon_timer.disconnect ();
    launcher_destructor (lch);
}

/* End of file */
/*----------------------------------------------------------------------------*/
