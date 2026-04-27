#ifndef WIDGET_HPP
#define WIDGET_HPP

#include <glib/gi18n.h>
#include <gtkmm/hvbox.h>
#include <wf-option-wrap.hpp>
#include "config/types.hpp"

extern "C" {
#include "configure.h"
}

class WayfireWidget
{
  public:
    std::string widget_name; // for WayfirePanel use, widgets shouldn't change it

    virtual void init (Gtk::HBox *container) = 0;

    virtual void command (const char *cmd)
    { printf ("command : %s %s\n", widget_name.c_str(), cmd); }

    virtual void handle_config_reload ()
    {}

    virtual bool set_icon ()
    { return false; }

    virtual ~WayfireWidget ()
    {}
};

typedef WayfireWidget *create_t ();
typedef void destroy_t (WayfireWidget *);

#endif /* end of include guard: WIDGET_HPP */
