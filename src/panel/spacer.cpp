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
#define ENDS 8

    cr->set_source_rgb (0.5, 0.5, 0.5);
    cr->rectangle (0, 0 + ENDS, 1, da.get_allocated_height () - 2 * ENDS);
    cr->fill();

    return true;
}
