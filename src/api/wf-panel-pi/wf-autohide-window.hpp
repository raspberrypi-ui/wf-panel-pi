#ifndef WF_AUTOHIDE_WINDOW_HPP
#define WF_AUTOHIDE_WINDOW_HPP

#include <gtkmm/window.h>
#include <gdk/gdkwayland.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include <wf-option-wrap.hpp>
#include "config/duration.hpp"

class WayfireAutohidingWindow : public Gtk::Window
{
  public:
    WayfireAutohidingWindow (WayfireOutput *output, bool dock);
    WayfireAutohidingWindow (WayfireAutohidingWindow&&) = delete;
    WayfireAutohidingWindow (const WayfireAutohidingWindow&) = delete;
    WayfireAutohidingWindow& operator = (const WayfireAutohidingWindow&) = delete;
    WayfireAutohidingWindow& operator = (WayfireAutohidingWindow&&) = delete;
    ~WayfireAutohidingWindow ();
    wl_surface *get_wl_surface () const;
    void set_auto_exclusive_zone (bool has_zone = false);

  private:
    struct WayfireOutput *output;
    bool dock;

    WfOption<std::string> position;
    WfOption<int> doffset;
    wf::animation::simple_animation_t y_position;
    WfOption<int> edge_offset;
    WfOption<bool> autohide;

    int autohide_counter;
    bool has_auto_exclusive_zone = false;
    bool input_inside_panel = false;

    bool last_autohide_value;
    int last_zone = 0;

    sigc::connection pending_show, pending_hide;

    GtkLayerShellEdge get_anchor_edge ();
    void increase_autohide ();
    void decrease_autohide ();
    bool should_autohide () const;
    bool do_show ();
    bool do_hide ();
    void schedule_hide (int delay);
    void schedule_show (int delay);
    void update_position ();
    void update_margin ();
    void update_autohide ();
};

#endif /* end of include guard: WF_AUTOHIDE_WINDOW_HPP */
