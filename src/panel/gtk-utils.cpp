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

extern "C" {
#include "lxutils.h"
}

#include "gtk-utils.hpp"

Glib::RefPtr<Gtk::GestureLongPress> detect_long_press (Gtk::Widget& target)
{
    Glib::RefPtr<Gtk::GestureLongPress> gesture = Gtk::GestureLongPress::create (target);
    gesture->set_propagation_phase (Gtk::PHASE_BUBBLE);
    gesture->signal_pressed ().connect ([=] (double x, double y) {pressed = PRESS_LONG;});
    gesture->set_touch_only (touch_only);
    return gesture;
}

Glib::RefPtr<Gtk::GestureLongPress> add_longpress_default (Gtk::Widget& target)
{
    Glib::RefPtr<Gtk::GestureLongPress> gesture = Gtk::GestureLongPress::create (target);
    GtkWidget *wid = target.gobj ();
    gesture->set_propagation_phase (Gtk::PHASE_BUBBLE);
    gesture->signal_pressed ().connect ([=] (double x, double y) {pressed = PRESS_LONG; press_x = x; press_y = y;});
    gesture->signal_end ().connect ([=] (GdkEventSequence *) {if (pressed == PRESS_LONG) pass_right_click (wid, press_x, press_y);});
    gesture->set_touch_only (touch_only);
    return gesture;
}

/* End of file */
/*----------------------------------------------------------------------------*/
