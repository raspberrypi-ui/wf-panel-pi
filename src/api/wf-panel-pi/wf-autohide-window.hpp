#ifndef WF_AUTOHIDE_WINDOW_HPP
#define WF_AUTOHIDE_WINDOW_HPP

#include <gtkmm/window.h>
#include <gdk/gdkwayland.h>
#include <gtk-layer-shell.h>
#include <wf-option-wrap.hpp>
#include "config/duration.hpp"

struct WayfireOutput;
struct zwf_hotspot_v2;

#define WF_WINDOW_POSITION_TOP    "top"
#define WF_WINDOW_POSITION_BOTTOM "bottom"

struct WayfireAutohidingWindowHotspotCallbacks;
/**
 * A window which is anchored to an edge of the screen, and can autohide.
 *
 * Autohide mode requires that the compositor supports wayfire-shell.
 */
class WayfireAutohidingWindow : public Gtk::Window
{
  public:
    /**
     * WayfireAutohidingWindow's behavior can be modified with several config
     * file options:
     *
     * 1. section/position
     * 2. section/autohide_duration
     * 3. section/edge_offset
     * 4. section/autohide
     */
    WayfireAutohidingWindow(WayfireOutput *output, const std::string& section, bool dock);
    WayfireAutohidingWindow(WayfireAutohidingWindow&&) = delete;
    WayfireAutohidingWindow(const WayfireAutohidingWindow&) = delete;
    WayfireAutohidingWindow& operator =(const WayfireAutohidingWindow&) = delete;
    WayfireAutohidingWindow& operator =(WayfireAutohidingWindow&&) = delete;

    ~WayfireAutohidingWindow();
    wl_surface *get_wl_surface() const;

    /* Add one more autohide request */
    void increase_autohide();
    /* Remove one autohide request */
    void decrease_autohide();
    /* Returns true if the window should autohide */
    bool should_autohide() const;

    /* Hide or show the panel after delay milliseconds, if nothing happens
     * in the meantime */
    void schedule_hide(int delay);
    void schedule_show(int delay);

    /** When auto exclusive zone is set, the window will adjust its exclusive
     * zone based on the window size.
     *
     * Note that autohide margin isn't taken into account. */
    void set_auto_exclusive_zone(bool has_zone = false);

  private:
    WayfireOutput *output;
    GtkLayerShellLayer old_layer;
    bool dock;

    WfOption<std::string> position;
    WfOption<std::string> dposition;
    WfOption<int> doffset;
    std::string last_position;
    void update_position();

    wf::animation::simple_animation_t y_position;
    bool update_margin();

    WfOption<int> edge_offset;
    int last_edge_offset = -1;

    WfOption<bool> autohide_opt;
    WfOption<bool> dock_autohide_opt;
    bool last_autohide_value;
    void update_autohide();

    bool has_auto_exclusive_zone = false;
    int last_zone = 0;

    sigc::connection pending_show, pending_hide;
    bool m_do_show();
    bool m_do_hide();
    int autohide_counter;

    /** Show the window but hide if no pointer input */
    void m_show_uncertain();

    bool input_inside_panel     = false;
};


#endif /* end of include guard: WF_AUTOHIDE_WINDOW_HPP */
