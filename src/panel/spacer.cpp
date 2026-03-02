#include "panel.hpp"
#include "spacer.hpp"

WayfireSpacing::WayfireSpacing(int pixels)
{
    if (pixels)
        box.set_size_request(pixels, 1);
    else
    {
        box.set_size_request (1, -1);
        box.pack_start (da);

        da.signal_draw ().connect (sigc::mem_fun (*this, &WayfireSpacing::draw));
    }
}

void WayfireSpacing::init(Gtk::HBox *container)
{
    box.set_name ("spacing");
    container->pack_start(box);
    box.show_all();
}

bool WayfireSpacing::draw(const Cairo::RefPtr<Cairo::Context>& cr)
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

    cr->set_source_rgb (fg.get_red(), fg.get_green(), fg.get_blue());
    cr->rectangle (0, 0 + height >> 2, 1, height >> 1);
    cr->fill();

    return true;
}
