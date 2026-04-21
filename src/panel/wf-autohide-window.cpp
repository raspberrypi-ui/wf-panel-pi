#include <glib/gi18n.h>
#include "wf-autohide-window.hpp"
#include "wayfire-shell-unstable-v2-client-protocol.h"

#include <wf-shell-app.hpp>
#include <gdk/gdkwayland.h>

#include <glibmm.h>
#include <iostream>
#include <assert.h>

#define AUTOHIDE_SHOW_DELAY 300
#define AUTOHIDE_HIDE_DELAY 500
#define MARGIN 10

WayfireAutohidingWindow::WayfireAutohidingWindow(WayfireOutput *output,
    const std::string& section, bool dock) :
    position{section + "/position"},
    dposition{section + "/dock_position"},
    doffset{section + "/dock_offset"},
    y_position{WfOption<int>{section + "/autohide_duration"}},
    edge_offset{section + "/edge_offset"},
    autohide_opt{section + "/autohide"}
{
    this->output = output;
    this->set_decorated(false);
    this->set_resizable(false);
    this->dock = dock;

    if (this->dock) this->set_name ("dock");
    else this->set_name ("panel");

    gtk_layer_init_for_window(this->gobj());
    gtk_layer_set_monitor(this->gobj(), output->monitor->gobj());
    gtk_layer_set_namespace(this->gobj(), "$unfocus panel");

    g_object_set (gtk_widget_get_settings (GTK_WIDGET (this->gobj())), "gtk-visible-focus", GTK_POLICY_AUTOMATIC, NULL);

    this->position.set_callback([=] () { this->update_position(); });
    this->dposition.set_callback([=] () { this->update_position(); });
    this->doffset.set_callback([=] () { this->update_position(); });
    this->update_position();

    this->edge_offset.set_callback([=] () { });

    this->autohide_opt.set_callback([=] { update_autohide(); });
    set_auto_exclusive_zone(!autohide_opt);

    this->signal_draw().connect_notify(
        [=] (const Cairo::RefPtr<Cairo::Context>&) { update_margin(); });

    this->signal_size_allocate().connect_notify(
        [=] (Gtk::Allocation&)
    {
        this->set_auto_exclusive_zone(this->has_auto_exclusive_zone);
    });

    this->signal_enter_notify_event().connect_notify(
        [=] (GdkEventCrossing *)
    {
        if (!autohide_opt) return;
        if (pending_hide.connected())
        {
            pending_hide.disconnect();
        }
        input_inside_panel = true;

        schedule_show(0);
    });

    this->signal_leave_notify_event().connect_notify(
        [=] (GdkEventCrossing *ev)
    {
        if (!autohide_opt) return;
        if (ev->detail == GDK_NOTIFY_INFERIOR) return;

        // don't hide if leaving a window towards the closest edge
        if (ev->x > MARGIN && ev->x < this->get_allocated_width() - MARGIN)
        {
            std::string pos = dock ? dposition : position;
            if (pos == WF_WINDOW_POSITION_TOP)
            {
                if (ev->y < this->get_allocated_height() / 2)  return;
            }
            else
            {
                if (ev->y > this->get_allocated_height() / 2)  return;
            }
        }

        input_inside_panel = false;
        if (should_autohide())
        {
            schedule_hide(AUTOHIDE_HIDE_DELAY);
        }
    });

    if (output->output)
    {
        static const zwf_output_v2_listener listener = {
            .enter_fullscreen = [] (void *data, zwf_output_v2*)
            {
                ((WayfireAutohidingWindow*)data)->increase_autohide();
            },
            .leave_fullscreen = [] (void *data, zwf_output_v2*)
            {
                ((WayfireAutohidingWindow*)data)->decrease_autohide();
            }
        };
        zwf_output_v2_add_listener(output->output, &listener, this);
    }
}

WayfireAutohidingWindow::~WayfireAutohidingWindow()
{
}

wl_surface*WayfireAutohidingWindow::get_wl_surface() const
{
    auto gdk_window = const_cast<GdkWindow*>(this->get_window()->gobj());
    return gdk_wayland_window_get_wl_surface(gdk_window);
}

/** Verify that position is correct and return a correct position */
static std::string check_position(std::string position)
{
    if (position == WF_WINDOW_POSITION_TOP)
    {
        return WF_WINDOW_POSITION_TOP;
    }

    if (position == WF_WINDOW_POSITION_BOTTOM)
    {
        return WF_WINDOW_POSITION_BOTTOM;
    }

    std::cerr << "Bad position in config file, defaulting to top" << std::endl;
    return WF_WINDOW_POSITION_TOP;
}

static GtkLayerShellEdge get_anchor_edge(std::string position, std::string dposition, bool dock)
{
    position = check_position(dock ? dposition : position);
    if (position == WF_WINDOW_POSITION_TOP)
    {
        return GTK_LAYER_SHELL_EDGE_TOP;
    }

    if (position == WF_WINDOW_POSITION_BOTTOM)
    {
        return GTK_LAYER_SHELL_EDGE_BOTTOM;
    }

    assert(false); // not reached because check_position()
}

void WayfireAutohidingWindow::m_show_uncertain()
{
    schedule_show(16); // add some delay to finish setting up the window
    /* And don't forget to hide the window afterwards, if autohide is enabled */
    if (should_autohide())
    {
        pending_hide = Glib::signal_timeout().connect([=] ()
        {
            schedule_hide(0);
            return false;
        }, AUTOHIDE_HIDE_DELAY);
    }
}

void WayfireAutohidingWindow::update_position()
{
    /* Reset old anchors */
    gtk_layer_set_anchor(this->gobj(), GTK_LAYER_SHELL_EDGE_TOP, false);
    gtk_layer_set_anchor(this->gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, false);

    /* Set new anchor */
    GtkLayerShellEdge anchor = WayfireShellApp::get().wizard ? GTK_LAYER_SHELL_EDGE_TOP : get_anchor_edge(position, dposition, dock);
    gtk_layer_set_anchor(this->gobj(), anchor, true);

    /* When the position changes, show an animation from the new edge. */
    y_position.animate(-this->get_allocated_height(), -this->get_allocated_height());
    m_show_uncertain();
}

void WayfireAutohidingWindow::set_auto_exclusive_zone(bool has_zone)
{
    this->has_auto_exclusive_zone = has_zone;
    int target_zone = has_zone ? get_allocated_height() : 0;

    if (this->last_zone != target_zone)
    {
        gtk_layer_set_exclusive_zone(this->gobj(), target_zone);
        last_zone = target_zone;
    }
}

void WayfireAutohidingWindow::increase_autohide()
{
    ++autohide_counter;
    if (should_autohide())
    {
        schedule_hide(0);
    }
}

void WayfireAutohidingWindow::decrease_autohide()
{
    autohide_counter = std::max(autohide_counter - 1, 0);
    if (!should_autohide())
    {
        schedule_show(0);
    }
}

bool WayfireAutohidingWindow::should_autohide() const
{
    return autohide_counter && !this->input_inside_panel;
}

bool WayfireAutohidingWindow::m_do_hide()
{
    y_position.animate(-get_allocated_height() + edge_offset);
    update_margin();
    return false; // disconnect
}

void WayfireAutohidingWindow::schedule_hide(int delay)
{
    pending_show.disconnect();
    if (delay == 0)
    {
        m_do_hide();
        return;
    }

    if (!pending_hide.connected())
    {
        pending_hide = Glib::signal_timeout().connect(
            sigc::mem_fun(this, &WayfireAutohidingWindow::m_do_hide), delay);
    }
}

bool WayfireAutohidingWindow::m_do_show()
{
    y_position.animate(dock ? doffset : 0);
    update_margin();
    return false; // disconnect
}

void WayfireAutohidingWindow::schedule_show(int delay)
{
    pending_hide.disconnect();
    if (delay == 0)
    {
        m_do_show();
        return;
    }

    if (!pending_show.connected())
    {
        pending_show = Glib::signal_timeout().connect(
            sigc::mem_fun(this, &WayfireAutohidingWindow::m_do_show), delay);
    }
}

bool WayfireAutohidingWindow::update_margin()
{
    if (y_position.running())
    {
        gtk_layer_set_margin(this->gobj(),
            WayfireShellApp::get().wizard ? GTK_LAYER_SHELL_EDGE_TOP : get_anchor_edge(position, dposition, dock), y_position);

        // queue_draw does not work when the panel is hidden
        // so calling wl_surface_commit to make WM show the panel back
        if (get_window() && this->is_visible ())
        {
            wl_surface_commit(get_wl_surface());
        }

        this->queue_draw();
        return true;
    }

    return false;
}

void WayfireAutohidingWindow::update_autohide()
{
    if (autohide_opt == last_autohide_value)
    {
        return;
    }

    if (autohide_opt)
    {
        increase_autohide();
    } else
    {
        decrease_autohide();
    }

    last_autohide_value = autohide_opt;
    set_auto_exclusive_zone(!autohide_opt);
}
