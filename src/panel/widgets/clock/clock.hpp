#ifndef WIDGETS_CLOCK_HPP
#define WIDGETS_CLOCK_HPP

#include <widget.hpp>
#include <gtkmm/button.h>
#include <gtkmm/label.h>

extern "C" {
#include "clock.h"
}

class WayfireClock : public WayfireWidget
{
    std::unique_ptr<Gtk::Button> plugin;
    Gtk::Label label;

    sigc::connection timeout;
    WfOption <std::string> format {"panel/clock_format"};
    WfOption <std::string> font {"panel/clock_font"};
    WfOption <std::string> bar_pos {"panel/position"};

    /* plugin */
    ClockPlugin data;
    ClockPlugin *clock;

  public:
    void init (Gtk::HBox *container) override;
    virtual ~WayfireClock ();
    void bar_pos_changed_cb (void);
    void set_font ();
    bool update_label ();
};

#endif /* end of include guard: WIDGETS_CLOCK_HPP */
