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

#ifndef AUTOHIDE_WINDOW_HPP
#define AUTOHIDE_WINDOW_HPP

#include <gtkmm/window.h>
#include <gdk/gdkwayland.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <wf-option-wrap.hpp>
#include "config/duration.hpp"

class AutohidingWindow : public Gtk::Window
{
  public:
    AutohidingWindow (bool dock);
    AutohidingWindow (AutohidingWindow&&) = delete;
    AutohidingWindow (const AutohidingWindow&) = delete;
    AutohidingWindow& operator = (const AutohidingWindow&) = delete;
    AutohidingWindow& operator = (AutohidingWindow&&) = delete;
    ~AutohidingWindow ();
    void set_auto_exclusive_zone (bool has_zone = false);
    void set_monitor ();
    void update_position ();

  private:
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
    bool noleave = false;

    bool last_autohide_value;
    int last_zone = 0;

    // Retains a reference on the monitor last passed to gtk_layer_set_monitor.
    // gtk_layer_set_monitor stores the raw GdkMonitor pointer it is given
    // without taking a reference on it, and GDK drops its own reference as
    // soon as a monitor is unplugged. Without this, the GdkMonitor could be
    // freed while gtk-layer-shell still holds the dangling pointer, causing
    // a crash or (if the freed memory is reused for a newly plugged monitor)
    // fooling gtk-layer-shell's pointer-equality check into thinking the
    // monitor hasn't changed. This is a member rather than a static so each
    // window keeps its own monitor alive, independent of any other window.
    Glib::RefPtr <Gdk::Monitor> mon;

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

#endif /* end of include guard: AUTOHIDE_WINDOW_HPP */

/* End of file */
/*----------------------------------------------------------------------------*/
