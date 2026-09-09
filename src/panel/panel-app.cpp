/*============================================================================
Copyright (c) 2026 Raspberry Pi
All rights reserved.

Some code based on the wf-shell project copyright (c) 2018 Ilia Bozhinov

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

#include <fcntl.h>
#include <sys/inotify.h>
#include <glibmm/main.h>
#include <giomm/dbusownname.h>
#include <giomm/dbusconnection.h>
#include <menu-cache.h>

#include <iostream>

#include "panel.hpp"

#include "panel-app.hpp"

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='com.raspberrypi.wfpanelpi'>"
  "    <method name='command'>"
  "      <arg type='s' name='plugin' direction='in'/>"
  "      <arg type='s' name='command' direction='in'/>"
  "    </method>"
  "  </interface>"
  "</node>";

std::unique_ptr <PanelApp> PanelApp::instance;
gboolean activated = FALSE;
char *confdir, *conffile;
MenuCache *mcache, *mcache_h;

PanelApp::PanelApp (int argc, char **argv)
{
    app = Gtk::Application::create (argc, argv, "com.raspberrypi.wfpanelpi", Gio::APPLICATION_FLAGS_NONE);
    app->signal_activate ().connect_notify (sigc::mem_fun (this, &PanelApp::on_activate));
}

PanelApp::~PanelApp ()
{
    if (owner_id) Gio::DBus::unown_name (owner_id);
}

void PanelApp::create (int argc, char **argv)
{
    instance = std::unique_ptr <PanelApp> (new PanelApp {argc, argv});
    instance->run ();
}

void PanelApp::run ()
{
    app->run ();
}

void PanelApp::on_activate ()
{
    if (activated) return;
    activated = TRUE;

    app->hold ();

    auto display = Gdk::Display::get_default ();
    auto wl_display = gdk_wayland_display_get_wl_display (display->gobj ());
    if (!wl_display)
    {
        std::cerr << "No Wayland display found" << std::endl;
        std::exit (-1);
    }

    // create a config file to track if it doesn't exist
    confdir = g_build_filename (g_get_user_config_dir (), "wf-panel-pi", NULL);
    if (!g_strcmp0 (getenv ("USER"), "rpi-first-boot-wizard"))
        conffile = g_build_filename (confdir, "wizard.ini", NULL);
    else
        conffile = g_build_filename (confdir, "wf-panel-pi.ini", NULL);

    g_mkdir_with_parents (confdir, S_IRUSR | S_IWUSR | S_IXUSR);
    close (open (conffile, O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));

    // setup config file tracking
    inotify_fd = inotify_init ();
    Glib::signal_io ().connect (sigc::mem_fun (this, &PanelApp::handle_inotify_event), inotify_fd, Glib::IO_IN | Glib::IO_HUP);
    inotify_add_watch (inotify_fd, conffile, IN_MODIFY);
    inotify_add_watch (inotify_fd, confdir, IN_CREATE | IN_DELETE);

    // create menu caches
    gboolean need_prefix = (g_getenv ("XDG_MENU_PREFIX") == NULL);
    mcache = menu_cache_lookup_sync (need_prefix ? "lxde-applications.menu" : "applications.menu");
    mcache_h = menu_cache_lookup_sync (need_prefix ? "lxde-applications.menu+hidden" : "applications.menu+hidden");

    // setup monitor tracking
    display->signal_monitor_added ().connect_notify ([=] (const Glib::RefPtr <Gdk::Monitor>& monitor) { monitors_changed (); });
    display->signal_monitor_removed ().connect_notify ([=] (const Glib::RefPtr <Gdk::Monitor>& monitor) { monitors_changed (); });

    // load initial monitors
    update_monitors ();

    // own on DBus
    introspection_data = Gio::DBus::NodeInfo::create_for_xml (introspection_xml);
    owner_id = Gio::DBus::own_name (Gio::DBus::BusType::BUS_TYPE_SESSION, "com.raspberrypi.wfpanelpi", sigc::mem_fun (this, &PanelApp::on_bus_acquired),
        sigc::mem_fun (this, &PanelApp::on_name_acquired), sigc::mem_fun (this, &PanelApp::on_name_lost));
}

/* Config file change tracking */

bool PanelApp::handle_inotify_event (Glib::IOCondition cond)
{
    char buf[1024 * sizeof (inotify_event)];
    read (inotify_fd, buf, 1024 * sizeof (inotify_event));

    if (panel) panel->handle_config_reload ();
    if (dock) dock->handle_config_reload ();

    inotify_add_watch (inotify_fd, conffile, IN_MODIFY);
    inotify_add_watch (inotify_fd, confdir, IN_CREATE | IN_DELETE);

    return true;
}

/* Monitor tracking */

void PanelApp::monitors_changed ()
{
    if (hotplug_timer.connected ()) hotplug_timer.disconnect ();

    panel->monitor_update_pending (true);
    dock->monitor_update_pending (true);

    hotplug_timer = Glib::signal_timeout ().connect (sigc::mem_fun (this, &PanelApp::update_monitors), 500);
}

bool PanelApp::update_monitors ()
{
    if (panel) panel->window->set_monitor ();
    else panel = std::make_unique <Panel> (false);
    if (dock) dock->window->set_monitor ();
    else dock = std::make_unique <Panel> (true);

    panel->monitor_update_pending (false);
    dock->monitor_update_pending (false);

    system ("if pgrep swaybg > /dev/null ; then pkill swaybg ; fi");

    return false;
}

/* DBus interface for commands to plugins */

void PanelApp::on_bus_acquired (const Glib::RefPtr <Gio::DBus::Connection>& connection, const Glib::ustring&)
{
    interface_vtable = new Gio::DBus::InterfaceVTable (sigc::mem_fun (this, &PanelApp::handle_method_call));
    connection->register_object ("/com/raspberrypi/wfpanelpi", introspection_data->lookup_interface(), *interface_vtable);
}

void PanelApp::on_name_acquired (const Glib::RefPtr <Gio::DBus::Connection>& connection, const Glib::ustring&)
{
}

void PanelApp::on_name_lost (const Glib::RefPtr <Gio::DBus::Connection>& connection, const Glib::ustring&)
{
}

void PanelApp::handle_method_call (const Glib::RefPtr < Gio::DBus::Connection >&, const Glib::ustring&, const Glib::ustring&,
    const Glib::ustring&, const Glib::ustring& method_name, const Glib::VariantContainerBase& parameters,
    const Glib::RefPtr <Gio::DBus::MethodInvocation>&)
{
    Glib::Variant <Glib::ustring> params;
    std::string plugin, command;

    if (method_name == "command")
    {
        parameters.get_child (params, 0);
        plugin = (std::string) params.get ();

        parameters.get_child (params, 1);
        command = (std::string) params.get ();

        if (panel) panel->handle_command_message (plugin.c_str (), command.c_str ());
        if (dock) dock->handle_command_message (plugin.c_str (), command.c_str ());
    }
}

int main (int argc, char **argv)
{
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    PanelApp::create (argc, argv);
    return 0;
}

/* End of file */
/*----------------------------------------------------------------------------*/
