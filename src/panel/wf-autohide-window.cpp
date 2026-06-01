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

#include <glibmm.h>

#include "wf-autohide-window.hpp"

#define AUTOHIDE_HIDE_DELAY 500
#define MARGIN 5

/* Public methods */

WayfireAutohidingWindow::WayfireAutohidingWindow (bool dock) :
    position {dock ? "dock/position" : "panel/position"},
    layer {dock ? "dock/layer" : "panel/layer"},
    monitor {dock ? "dock/monitor" : "panel/monitor"},
    offset {dock ? "dock/offset" : "panel/offset"},
    remainder {dock ? "dock/remainder" : "panel/remainder"},
    autohide {dock ? "dock/autohide" : "panel/autohide"},
    y_position {WfOption <int> {"panel/autohide_duration"}}
{
    set_decorated (false);
    set_resizable (false);

    gtk_layer_init_for_window (this->gobj ());
    gtk_layer_set_monitor (this->gobj (), gdk_display_get_monitor (gdk_display_get_default (), 0));
    gtk_layer_set_namespace (this->gobj (), "$unfocus panel");
    gtk_layer_set_keyboard_mode (this->gobj (), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);

    g_object_set (gtk_widget_get_settings (GTK_WIDGET (this->gobj ())), "gtk-visible-focus", GTK_POLICY_AUTOMATIC, NULL);

    last_autohide_value = autohide;
    autohide_counter = static_cast <int> (autohide);
    autohide.set_callback([=] { update_autohide (); });
    position.set_callback([=] { update_position (); });
    remainder.set_callback([=] { update_position (); });
    offset.set_callback([=] { update_position (); });
    layer.set_callback ([=] { set_layer (); });
    monitor.set_callback ([=] { set_monitor (); });

    set_auto_exclusive_zone (!autohide);
    update_position ();
    set_layer ();

    signal_draw().connect_notify ([=] (const Cairo::RefPtr <Cairo::Context>&)
    {
        update_margin ();
    });

    signal_size_allocate().connect_notify ([=] (Gtk::Allocation&)
    {
        set_auto_exclusive_zone (has_auto_exclusive_zone);
    });

    signal_enter_notify_event().connect_notify ([=] (GdkEventCrossing *)
    {
        if (!autohide) return;
        if (pending_hide.connected ()) pending_hide.disconnect ();
        input_inside_panel = true;

        schedule_show (0);
    });

    signal_leave_notify_event().connect_notify ([=] (GdkEventCrossing *ev)
    {
        if (!autohide) return;
        if (ev->detail == GDK_NOTIFY_INFERIOR) return;

        // don't hide if leaving a window towards the closest edge
        if (ev->x > MARGIN && ev->x < get_allocated_width() - MARGIN)
        {
            if (get_anchor_edge () == GTK_LAYER_SHELL_EDGE_TOP)
            {
                if (ev->y < MARGIN) return;
            }
            else
            {
                if (ev->y > get_allocated_height () - MARGIN) return;
            }
        }

        input_inside_panel = false;
        if (should_autohide ()) schedule_hide (AUTOHIDE_HIDE_DELAY);
    });
}

WayfireAutohidingWindow::~WayfireAutohidingWindow ()
{
}

void WayfireAutohidingWindow::set_auto_exclusive_zone (bool has_zone)
{
    int target_zone = has_zone ? get_allocated_height () : 0;
    has_auto_exclusive_zone = has_zone;

    if (last_zone != target_zone)
    {
        gtk_layer_set_exclusive_zone (this->gobj (), target_zone);
        last_zone = target_zone;
    }
}

void WayfireAutohidingWindow::set_monitor ()
{
    GdkDisplay *display = gdk_display_get_default ();
    GdkScreen *screen = gdk_display_get_default_screen (display);
    GdkMonitor *mon = NULL;
    int try_mon;
    const char *mnumstr = ((std::string) monitor).c_str();
    char *mname;

    if (strlen (mnumstr) == 1 && sscanf (mnumstr, "%d", &try_mon) == 1)
    {
        // single digit - interpret as monitor number
        while (try_mon >= 0)
        {
            mon = gdk_display_get_monitor (display, try_mon);
            if (mon) break;
            try_mon--;
        }
    }
    else
    {
        // output name - try to match it to a connected monitor
        for (try_mon = gdk_display_get_n_monitors (display) - 1; try_mon >= 0; try_mon--)
        {
            mon = gdk_display_get_monitor (display, try_mon);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            mname = gdk_screen_get_monitor_plug_name (screen, try_mon);
#pragma GCC diagnostic pop
            if (!g_strcmp0 (mname, mnumstr) && mon)
            {
                g_free (mname);
                break;
            }
            g_free (mname);
        }
    }

    if (mon) gtk_layer_set_monitor (this->gobj(), mon);
}

/* Private methods */

GtkLayerShellEdge WayfireAutohidingWindow::get_anchor_edge ()
{
    if ((std::string) position == "bottom") return GTK_LAYER_SHELL_EDGE_BOTTOM;
    return GTK_LAYER_SHELL_EDGE_TOP;
}

void WayfireAutohidingWindow::increase_autohide ()
{
    autohide_counter++;
    if (should_autohide ()) schedule_hide (0);
}

void WayfireAutohidingWindow::decrease_autohide ()
{
    autohide_counter--;
    if (autohide_counter < 0) autohide_counter = 0;
    if (!should_autohide ()) schedule_show (0);
}

bool WayfireAutohidingWindow::should_autohide () const
{
    return autohide_counter && !input_inside_panel;
}

bool WayfireAutohidingWindow::do_hide ()
{
    y_position.animate (remainder - get_allocated_height ());
    update_margin ();
    return false;
}

bool WayfireAutohidingWindow::do_show ()
{
    y_position.animate (offset);
    update_margin ();
    return false;
}

void WayfireAutohidingWindow::schedule_hide (int delay)
{
    pending_show.disconnect ();
    if (delay == 0) do_hide ();
    else if (!pending_hide.connected ())
    {
        pending_hide = Glib::signal_timeout ().connect (sigc::mem_fun (this, &WayfireAutohidingWindow::do_hide), delay);
    }
}

void WayfireAutohidingWindow::schedule_show (int delay)
{
    pending_hide.disconnect ();
    if (delay == 0) do_show ();
    else if (!pending_show.connected ())
    {
        pending_show = Glib::signal_timeout ().connect (sigc::mem_fun (this, &WayfireAutohidingWindow::do_show), delay);
    }
}

void WayfireAutohidingWindow::update_position ()
{
    /* Reset old anchors */
    gtk_layer_set_anchor (this->gobj (), GTK_LAYER_SHELL_EDGE_TOP, false);
    gtk_layer_set_anchor (this->gobj (), GTK_LAYER_SHELL_EDGE_BOTTOM, false);

    /* Set new anchor */
    gtk_layer_set_anchor (this->gobj (), get_anchor_edge (), true);

    /* When the position changes, show an animation from the new edge. */
    y_position.animate (-get_allocated_height ());

    /* Show the window */
    schedule_show (0);

    /* Hide the window afterwards if autohide is enabled */
    if (should_autohide ()) schedule_hide (AUTOHIDE_HIDE_DELAY);
}

void WayfireAutohidingWindow::update_margin ()
{
    if (y_position.running ())
    {
        gtk_layer_set_margin (this->gobj (), get_anchor_edge (), y_position);

        // queue_draw does not work when the panel is hidden
        // so calling wl_surface_commit to make WM show the panel back
        if (get_window () && is_visible ())
        {
            GdkWindow *gdk_window = get_window ()->gobj ();
            wl_surface_commit (gdk_wayland_window_get_wl_surface (gdk_window));
        }

        queue_draw ();
    }
}

void WayfireAutohidingWindow::update_autohide ()
{
    if (autohide == last_autohide_value) return;

    if (autohide) increase_autohide ();
    else decrease_autohide ();

    last_autohide_value = autohide;
    set_auto_exclusive_zone (!autohide);
    set_layer ();
}

void WayfireAutohidingWindow::set_layer ()
{
    if (autohide)
    {
        // if autohide is enabled, keep on top
        gtk_layer_set_layer (this->gobj (), GTK_LAYER_SHELL_LAYER_TOP);
        return;
    }

    if ((std::string) layer == "overlay") gtk_layer_set_layer (this->gobj (), GTK_LAYER_SHELL_LAYER_OVERLAY);
    if ((std::string) layer == "top") gtk_layer_set_layer (this->gobj (), GTK_LAYER_SHELL_LAYER_TOP);
    if ((std::string) layer == "bottom") gtk_layer_set_layer (this->gobj (), GTK_LAYER_SHELL_LAYER_BOTTOM);
    if ((std::string) layer == "background") gtk_layer_set_layer (this->gobj (), GTK_LAYER_SHELL_LAYER_BACKGROUND);
}

/* End of file */
/*----------------------------------------------------------------------------*/
