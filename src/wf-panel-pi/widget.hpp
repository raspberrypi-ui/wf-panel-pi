#ifndef WIDGET_HPP
#define WIDGET_HPP

#include <glib/gi18n.h>
#include <gtkmm/hvbox.h>

extern "C" {
#include "plug_conf.h"
}

class PanelWidget
{
  public:
    std::string widget_name;

    virtual void widget_init (Gtk::HBox *)
    {}

    virtual ~PanelWidget ()
    {}

    virtual void widget_command (const char *)
    {}

    virtual void widget_config_reload ()
    {}

    virtual void widget_set_icon ()
    {}
};

#endif /* end of include guard: WIDGET_HPP */
