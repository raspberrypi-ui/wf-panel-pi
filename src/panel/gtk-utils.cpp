#include <glibmm.h>
#include "gtk-utils.hpp"

extern "C" {
#include "lxutils.h"
}

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
