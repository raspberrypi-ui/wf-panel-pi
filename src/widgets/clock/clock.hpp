/*============================================================================
Copyright (c) 2024 Raspberry Pi
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

#ifndef WIDGETS_CLOCK_HPP
#define WIDGETS_CLOCK_HPP

#include <widget.hpp>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/window.h>
#include <gtkmm/calendar.h>
#include <gtkmm/gesturelongpress.h>

class WayfireClock : public WayfireWidget
{
    std::unique_ptr<Gtk::Button> plugin;
    std::unique_ptr<Gtk::Label> label;
    std::unique_ptr<Gtk::Window> window;
    std::unique_ptr<Gtk::Calendar> calendar;
    Glib::RefPtr<Gtk::GestureLongPress> gesture;

    sigc::connection timeout;
    WfOption <std::string> format {"panel/clock_format"};
    WfOption <std::string> font {"panel/clock_font"};
    WfOption <std::string> bar_pos {"panel/position"};

    bool popup_shown;

  public:
    void init (Gtk::HBox *container) override;
    virtual ~WayfireClock ();
    void bar_pos_changed_cb (void);
    void set_font ();
    bool update_label ();
    void show_calendar ();
    bool on_pressed (GdkEventButton *ev);
    void on_clicked ();
    void config_changed_cb ();

};

#endif /* end of include guard: WIDGETS_CLOCK_HPP */

/* End of file */
/*----------------------------------------------------------------------------*/
