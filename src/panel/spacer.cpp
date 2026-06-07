/*============================================================================
Copyright (c) 2026 Raspberry Pi
All rights reserved.

Some code based on the wf-shell project copyright (c) 2018 Ilia Bozhinov

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

#include "panel.hpp"

#include "spacer.hpp"

WayfireSpacing::WayfireSpacing (int pixels)
{
    if (pixels)
        box.set_size_request (pixels, 1);
    else
    {
        box.set_size_request (1, -1);
        box.pack_start (da);

        da.signal_draw ().connect (sigc::mem_fun (*this, &WayfireSpacing::draw));
    }
}

void WayfireSpacing::init (Gtk::HBox *container)
{
    box.set_name ("spacing");
    container->pack_start (box, false, false);
    box.show_all ();
}

bool WayfireSpacing::draw (const Cairo::RefPtr<Cairo::Context>& cr)
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

/* End of file */
/*----------------------------------------------------------------------------*/
