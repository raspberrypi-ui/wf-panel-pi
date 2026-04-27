#ifndef WF_GTK_UTILS
#define WF_GTK_UTILS

#include <gtkmm/gesturelongpress.h>

Glib::RefPtr<Gtk::GestureLongPress> detect_long_press (Gtk::Widget& target);
Glib::RefPtr<Gtk::GestureLongPress> add_longpress_default (Gtk::Widget& target);

#endif /* end of include guard: WF_GTK_UTILS */
