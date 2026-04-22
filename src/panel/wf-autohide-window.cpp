#include <glib/gi18n.h>
#include "wf-autohide-window.hpp"

#include <wf-shell-app.hpp>
#include <gdk/gdkwayland.h>

#include <glibmm.h>
#include <iostream>
#include <assert.h>

#define AUTOHIDE_HIDE_DELAY 500
#define MARGIN 5

/* Public methods */

WayfireAutohidingWindow::WayfireAutohidingWindow (WayfireOutput *output, const std::string& section, bool dock) :
    position {dock ? section + "/dock_position" : section + "/position"},
    doffset {section + "/dock_offset"},
    y_position {WfOption<int>{section + "/autohide_duration"}},
    edge_offset {section + "/edge_offset"},
    autohide_opt {dock ? section + "/dock_autohide" : section + "/autohide"}
{
    this->output = output;
    this->dock = dock;

    set_decorated (false);
    set_resizable (false);
    set_name (dock ? "dock" : "panel");

    gtk_layer_init_for_window (this->gobj ());
    gtk_layer_set_monitor (this->gobj (), output->monitor->gobj ());
    gtk_layer_set_namespace (this->gobj (), "$unfocus panel");

    g_object_set (gtk_widget_get_settings (GTK_WIDGET (this->gobj ())), "gtk-visible-focus", GTK_POLICY_AUTOMATIC, NULL);

    last_autohide_value = autohide_opt;
    autohide_counter = static_cast <int> (autohide_opt);
    autohide_opt.set_callback([=] { update_autohide (); });
    position.set_callback([=] () { update_position (); });
    edge_offset.set_callback([=] () { update_position (); });
    if (dock) doffset.set_callback([=] () { update_position (); });

    set_auto_exclusive_zone (!autohide_opt);
    update_position ();

    this->signal_draw().connect_notify (
        [=] (const Cairo::RefPtr<Cairo::Context>&)
    {
        update_margin ();
    });

    this->signal_size_allocate().connect_notify (
        [=] (Gtk::Allocation&)
    {
        set_auto_exclusive_zone (has_auto_exclusive_zone);
    });

    this->signal_enter_notify_event().connect_notify (
        [=] (GdkEventCrossing *)
    {
        if (!autohide_opt) return;
        if (pending_hide.connected ()) pending_hide.disconnect ();
        input_inside_panel = true;

        schedule_show (0);
    });

    this->signal_leave_notify_event().connect_notify (
        [=] (GdkEventCrossing *ev)
    {
        if (!autohide_opt) return;
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

    if (output->output)
    {
        static const zwf_output_v2_listener listener = {
            .enter_fullscreen = [] (void *data, zwf_output_v2*)
            {
                ((WayfireAutohidingWindow*) data)->increase_autohide ();
            },
            .leave_fullscreen = [] (void *data, zwf_output_v2*)
            {
                ((WayfireAutohidingWindow*) data)->decrease_autohide ();
            }
        };
        zwf_output_v2_add_listener (output->output, &listener, this);
    }
}

WayfireAutohidingWindow::~WayfireAutohidingWindow ()
{
}

wl_surface *WayfireAutohidingWindow::get_wl_surface () const
{
    auto gdk_window = const_cast <GdkWindow *> (get_window ()->gobj ());
    return gdk_wayland_window_get_wl_surface (gdk_window);
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

/* Private methods */

GtkLayerShellEdge WayfireAutohidingWindow::get_anchor_edge ()
{
    if (WayfireShellApp::get().wizard) return GTK_LAYER_SHELL_EDGE_TOP;
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
    y_position.animate (edge_offset - get_allocated_height ());
    update_margin ();
    return false;
}

bool WayfireAutohidingWindow::do_show ()
{
    y_position.animate (dock ? doffset : 0);
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
        if (get_window () && is_visible ()) wl_surface_commit (get_wl_surface ());

        queue_draw ();
    }
}

void WayfireAutohidingWindow::update_autohide ()
{
    if (autohide_opt == last_autohide_value) return;

    if (autohide_opt) increase_autohide ();
    else decrease_autohide ();

    last_autohide_value = autohide_opt;
    set_auto_exclusive_zone (!autohide_opt);
}
