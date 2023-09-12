#ifndef WIDGET_HPP
#define WIDGET_HPP

#include <glib/gi18n.h>
#include <gtkmm/hvbox.h>
#include <wf-option-wrap.hpp>
#include <wayfire/config/types.hpp>
#include "configure.h"

#define DEFAULT_PANEL_HEIGHT "48"
#define DEFAULT_ICON_SIZE 32

#define PANEL_POSITION_BOTTOM "bottom"
#define PANEL_POSITION_TOP "top"

class wayfire_config;
class WayfireWidget
{
  public:
    std::string widget_name; // for WayfirePanel use, widgets shouldn't change it

    virtual void init(Gtk::HBox *container) = 0;
    virtual void command (const char *cmd)
    {
        printf ("command : %s %s\n", widget_name.c_str(), cmd);
    }
    virtual void handle_config_reload()
    {}
    virtual ~WayfireWidget()
    {}
};

#endif /* end of include guard: WIDGET_HPP */
