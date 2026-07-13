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

#ifndef PANEL_HPP
#define PANEL_HPP

#include <gtkmm/menu.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/hvbox.h>
#include <gtkmm/gesturelongpress.h>

#include "widget.hpp"
#include "wf-autohide-window.hpp"

class Panel
{
  public:
    Panel (GdkMonitor *mon, bool dock);
    ~Panel ();
    void handle_config_reload ();
    void handle_command_message (const char *plugin, const char *cmd);

    std::unique_ptr <WayfireAutohidingWindow> window;

  private:
    Gtk::HBox content_box;
    Gtk::HBox left_box, right_box, right2_box;
    Gtk::VBox grid;
    Gtk::Menu menu;
    Gtk::MenuItem conf;
    Gtk::MenuItem cplug;
    Gtk::MenuItem notif;
    Gtk::MenuItem appset;
    Gtk::SeparatorMenuItem sep;
    Glib::RefPtr <Gtk::GestureLongPress> gesture;
    sigc::connection draw_connection;

    std::vector <std::unique_ptr <PanelWidget>> left_widgets, right_widgets;

    GdkMonitor *mon;

    bool dock;
    int scaling;
    int isize;

    WfOption <int> icon_size;
    WfOption <std::string> left_widgets_opt;
    WfOption <std::string> right_widgets_opt;
    WfOption <bool> exclusive;
    WfOption <bool> gestures_touch_only {"panel/gestures_touch_only"};
    WfOption <int> notify_timeout {"notify/timeout"};
    WfOption <bool> notifications {"notify/enable"};
    WfOption <bool> libnotify {"notify/libnotify"};

    void set_exclusive ();
    bool on_keypress_event (GdkEventKey *event);
    bool on_button_press_event (GdkEventButton *event);
    bool on_button_release_event (GdkEventButton *event);
    bool on_delete (GdkEventAny *ev);
    void do_configure ();
    void do_plugin_configure ();
    void do_notify_configure ();
    void do_appearance_set ();
    std::unique_ptr<PanelWidget> widget_from_name (const char *name);
    void reload_widgets (std::string list, std::vector <std::unique_ptr <PanelWidget>>& container, Gtk::HBox& box);
    void init_widgets ();
    void init_notify ();
    void update_widget_icons ();
    void update_gestures ();
};

#endif /* end of include guard: PANEL_HPP */

/* End of file */
/*----------------------------------------------------------------------------*/
