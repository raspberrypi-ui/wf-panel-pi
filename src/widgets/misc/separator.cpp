#include <widget.hpp>
#include <gtkmm/drawingarea.h>

class WidgetSeparator : public PanelWidget
{
    Gtk::HBox box;
    Gtk::DrawingArea da;

  public:
    WidgetSeparator (void);
    virtual void widget_init (Gtk::HBox *container);
    virtual ~WidgetSeparator () {}
    bool draw (const Cairo::RefPtr<Cairo::Context>& cr);
};

extern "C" {
    PanelWidget *create () { return new WidgetSeparator; }
    void destroy (PanelWidget *w) { delete w; }

    static constexpr conf_table_t conf_table[1] = {
        {CONF_TYPE_NONE,    NULL,   NULL,   NULL,   NULL}
    };

    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return N_("Separator"); };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}

WidgetSeparator::WidgetSeparator (void)
{
    box.set_size_request (1, -1);
    box.pack_start (da);

    da.signal_draw ().connect (sigc::mem_fun (*this, &WidgetSeparator::draw));
}

void WidgetSeparator::widget_init (Gtk::HBox *container)
{
    box.set_name ("separator");
    container->pack_start (box, false, false);
    box.show_all ();
}

bool WidgetSeparator::draw (const Cairo::RefPtr<Cairo::Context>& cr)
{
    Gtk::Allocation palloc, alloc = box.get_allocation ();
    Gtk::Widget *w = dynamic_cast<Gtk::Widget*> (&box);
    while (w)
    {
        palloc = w->get_allocation ();
        w = w->get_parent ();
    }

    if (alloc.get_x () == 0 || alloc.get_x () + 1 == palloc.get_width ()) return true;

    Glib::RefPtr <Gtk::StyleContext> sc = da.get_style_context ();
    Gdk::RGBA fg = sc->get_color ();
    int height = da.get_allocated_height ();

    cr->set_source_rgb (fg.get_red (), fg.get_green (), fg.get_blue ());
    cr->rectangle (0, 0 + height >> 2, 1, height >> 1);
    cr->fill ();

    return true;
}
