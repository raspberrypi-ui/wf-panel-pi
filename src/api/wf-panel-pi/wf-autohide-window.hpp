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

#ifndef WF_AUTOHIDE_WINDOW_HPP
#define WF_AUTOHIDE_WINDOW_HPP

#include <gtkmm/window.h>
#include <gdk/gdkwayland.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <wf-option-wrap.hpp>
#include "config/duration.hpp"

class WayfireAutohidingWindow : public Gtk::Window
{
  public:
    WayfireAutohidingWindow (GdkMonitor *mon, bool dock);
    WayfireAutohidingWindow (WayfireAutohidingWindow&&) = delete;
    WayfireAutohidingWindow (const WayfireAutohidingWindow&) = delete;
    WayfireAutohidingWindow& operator = (const WayfireAutohidingWindow&) = delete;
    WayfireAutohidingWindow& operator = (WayfireAutohidingWindow&&) = delete;
    ~WayfireAutohidingWindow ();
    void set_auto_exclusive_zone (bool has_zone = false);
    void set_monitor ();
    void update_position ();

  private:
    GdkMonitor *mon;

    WfOption <std::string> position;
    WfOption <std::string> layer;
    WfOption <std::string> monitor;
    WfOption <int> offset;
    WfOption <int> remainder;
    WfOption <bool> autohide;
    WfOption <int> duration {"panel/autohide_duration"};

    wf::animation::simple_animation_t y_position;

    int autohide_counter;
    bool has_auto_exclusive_zone = false;
    bool input_inside_panel = false;

    bool last_autohide_value;
    int last_zone = 0;

    sigc::connection pending_show, pending_hide;

    GtkLayerShellEdge get_anchor_edge ();
    void increase_autohide ();
    void decrease_autohide ();
    bool should_autohide () const;
    bool do_show ();
    bool do_hide ();
    void schedule_hide (int delay);
    void schedule_show (int delay);
    void update_margin ();
    void update_autohide ();
    void set_layer ();
};

#endif /* end of include guard: WF_AUTOHIDE_WINDOW_HPP */

/* End of file */
/*----------------------------------------------------------------------------*/
