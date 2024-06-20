#include "spacer.hpp"

WayfireSpacing::WayfireSpacing(int pixels)
{
    box.set_size_request(pixels, 1);
}

void WayfireSpacing::init(Gtk::HBox *container)
{
    box.set_name ("spacing");
    container->pack_start(box);
    box.show_all();
}
