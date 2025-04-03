#ifndef WIDGET_SPACER_HPP
#define WIDGET_SPACER_HPP

#include <widget.hpp>
#include <gtkmm/drawingarea.h>


class WayfireSpacing : public WayfireWidget
{
    Gtk::HBox box;
    Gtk::DrawingArea da;

  public:
    WayfireSpacing(int pixels);
    bool draw(const Cairo::RefPtr<Cairo::Context>& cr);

    virtual void init(Gtk::HBox *container);
    virtual ~WayfireSpacing()
    {}
};


#endif /* end of include guard: WIDGET_SPACER_HPP */
