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

#ifndef PANEL_APP_HPP
#define PANEL_APP_HPP

#include <gtkmm/application.h>
#include <gdkmm/monitor.h>

class Panel;

class PanelApp
{
  public:
    PanelApp (int argc, char **argv);
    ~PanelApp ();

    static void create (int argc, char **argv);

  private:
    static std::unique_ptr <PanelApp> instance;
    Glib::RefPtr <Gtk::Application> app;

    std::unique_ptr <Panel> panel;
    std::unique_ptr <Panel> dock;

    sigc::connection hotplug_timer;

    Glib::RefPtr <Gio::DBus::NodeInfo> introspection_data;
    Gio::DBus::InterfaceVTable *interface_vtable;
    guint owner_id;

    int inotify_fd;

    void run ();
    void on_activate ();

    std::string get_config_file ();
    void do_reload_config ();
    bool handle_inotify_event (Glib::IOCondition cond);

    void monitors_changed ();
    bool update_monitors ();

    void on_bus_acquired (const Glib::RefPtr <Gio::DBus::Connection>&, const Glib::ustring&);
    void on_name_acquired (const Glib::RefPtr <Gio::DBus::Connection>&, const Glib::ustring&);
    void on_name_lost (const Glib::RefPtr <Gio::DBus::Connection>&, const Glib::ustring&);
    void handle_method_call (const Glib::RefPtr <Gio::DBus::Connection>&, const Glib::ustring&, const Glib::ustring&,
        const Glib::ustring&, const Glib::ustring&, const Glib::VariantContainerBase&, const Glib::RefPtr <Gio::DBus::MethodInvocation>&);
};

#endif /* end of include guard: PANEL_APP_HPP */

/* End of file */
/*----------------------------------------------------------------------------*/
