#ifndef WIDGET_SPACER_HPP
#define WIDGET_SPACER_HPP

#include <widget.hpp>
#include <gtkmm/drawingarea.h>


class WidgetSpacing : public PanelWidget
{
    Gtk::HBox box;
    Gtk::DrawingArea da;

  public:
    WidgetSpacing(int pixels);
    bool draw(const Cairo::RefPtr<Cairo::Context>& cr);

    virtual void init(Gtk::HBox *container);
    virtual ~WidgetSpacing()
    {}
};

class WidgetSplit : public PanelWidget
{
    Gtk::HBox box;

  public:
    WidgetSplit(void);

    virtual void init(Gtk::HBox *container);
    virtual ~WidgetSplit()
    {}
};

#endif /* end of include guard: WIDGET_SPACER_HPP */
