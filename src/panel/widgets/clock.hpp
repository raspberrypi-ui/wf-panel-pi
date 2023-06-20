#ifndef WIDGETS_CLOCK_HPP
#define WIDGETS_CLOCK_HPP

#include "../widget.hpp"
#include "wf-popover.hpp"
#include <gtkmm/calendar.h>
#include <gtkmm/label.h>

class WayfireClock : public WayfireWidget
{
    Gtk::Label label;
    Gtk::Calendar calendar;
    std::unique_ptr<WayfireMenuButton> button;

    sigc::connection timeout;
    WfOption<std::string> format{"panel/clock_format"};
    WfOption<std::string> font{"panel/clock_font"};

    void set_font();
    void on_calendar_shown();

    static constexpr conf_table_t conf_table[3] = {
        {"clock",   "format",   CONF_STRING,    "Display format"},
        {"clock",   "font",     CONF_STRING,    "Display font"},
        {NULL,      NULL,       CONF_NONE,      NULL}
    };

    public:
    void init(Gtk::HBox *container) override;
    bool update_label();
    static std::string display_name (void) { return gettext ("Clock"); };
    static const conf_table_t *config_params (void) { return conf_table; };
    ~WayfireClock();
};

#endif /* end of include guard: WIDGETS_CLOCK_HPP */
