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

    virtual void init (Gtk::HBox *container) = 0;

    virtual void command (const char *cmd)
    { printf ("command : %s %s\n", widget_name.c_str(), cmd); }

    virtual void handle_config_reload ()
    {}

    virtual bool set_icon ()
    { return false; }

    virtual ~PanelWidget ()
    {}
};

typedef PanelWidget *create_t ();
typedef void destroy_t (PanelWidget *);

#endif /* end of include guard: WIDGET_HPP */
