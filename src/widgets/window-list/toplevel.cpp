#include <gtkmm/menu.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/icontheme.h>
#include <gtkmm/gesturedrag.h>
#include <gtkmm/gesturelongpress.h>
#include <giomm/desktopappinfo.h>
#include <iostream>
#include <filesystem>

#include <gdkmm/seat.h>
#include <gdk/gdkwayland.h>
#include <cmath>


#include "toplevel.hpp"
#include "gtk-utils.hpp"
#include "panel.hpp"
#include <cassert>
extern "C" {
#include "lxutils.h"
}

namespace
{
extern zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_v1_impl;
}

namespace IconProvider
{
void set_image_from_icon(Gtk::Image& image,
    std::string app_id_list, int size, int scale);
}

class WayfireToplevel::impl
{
    zwlr_foreign_toplevel_handle_v1 *handle, *parent;
    std::vector<zwlr_foreign_toplevel_handle_v1*> children;
    uint32_t state = 0;

    WfOption <int> icon_size {"panel/icon_size"};
    Gtk::ToggleButton button;
    Gtk::HBox button_contents;
    Gtk::Image image;
    Gtk::Label label;
    Gtk::Menu menu;
    Gtk::MenuItem minimize, maximize, close;
    Glib::RefPtr<Gtk::GestureDrag> drag_gesture;
    Glib::RefPtr<Gtk::GestureLongPress> gesture;

    Glib::ustring app_id, title;

  public:
    WayfireWindowList *window_list;

    impl(WayfireWindowList *window_list, zwlr_foreign_toplevel_handle_v1 *handle)
    {
        this->handle = handle;
        this->parent = nullptr;
        zwlr_foreign_toplevel_handle_v1_add_listener(handle,
            &toplevel_handle_v1_impl, this);

        button_contents.add(image);
        button_contents.add(label);
        button_contents.set_halign(Gtk::ALIGN_START);
        button_contents.set_spacing(5);
        button.add(button_contents);
        button.set_tooltip_text("none");
        button.show_all ();

        //button.signal_clicked().connect_notify(
        //    sigc::mem_fun(this, &WayfireToplevel::impl::on_clicked));
        button.signal_size_allocate().connect_notify(
            sigc::mem_fun(this, &WayfireToplevel::impl::on_allocation_changed));
        button.property_scale_factor().signal_changed()
            .connect(sigc::mem_fun(this, &WayfireToplevel::impl::on_scale_update));
        button.signal_button_release_event().connect(
            sigc::mem_fun(this, &WayfireToplevel::impl::on_release));

        icon_size.set_callback (sigc::mem_fun (*this, &WayfireToplevel::impl::on_scale_update));

        minimize.set_label(_("Minimize"));
        maximize.set_label(_("Maximize"));
        close.set_label(_("Close"));
        minimize.signal_activate().connect(
            sigc::mem_fun(this, &WayfireToplevel::impl::on_menu_minimize));
        maximize.signal_activate().connect(
            sigc::mem_fun(this, &WayfireToplevel::impl::on_menu_maximize));
        close.signal_activate().connect(
            sigc::mem_fun(this, &WayfireToplevel::impl::on_menu_close));
        menu.attach(minimize, 0, 1, 0, 1);
        menu.attach(maximize, 0, 1, 1, 2);
        menu.attach(close, 0, 1, 2, 3);
        menu.attach_to_widget(button);
        menu.show_all();

        drag_gesture = Gtk::GestureDrag::create(button);
        drag_gesture->signal_drag_begin().connect_notify(
            sigc::mem_fun(this, &WayfireToplevel::impl::on_drag_begin));
        drag_gesture->signal_drag_update().connect_notify(
            sigc::mem_fun(this, &WayfireToplevel::impl::on_drag_update));
        drag_gesture->signal_drag_end().connect_notify(
            sigc::mem_fun(this, &WayfireToplevel::impl::on_drag_end));

        gesture = detect_long_press (button);

        this->window_list = window_list;
    }

    int grab_off_x;
    double grab_start_x, grab_start_y;
    double grab_abs_start_x;
    bool drag_exceeds_threshold;
    bool mute_clicks = false;

    void on_drag_begin(double _x, double _y)
    {
        auto& container = window_list->box;
        /* Set grab start, before transforming it to absolute position */
        grab_start_x = _x;
        grab_start_y = _y;

        set_flat_class(false);
        window_list->box.set_top_widget(&button);

        /* Find the distance between pointer X and button origin */
        int x = container.get_absolute_position(_x, button);
        grab_abs_start_x = x;

        /* Find button corner in window-relative coords */
        int loc_x = container.get_absolute_position(0, button);
        grab_off_x = x - loc_x;

        drag_exceeds_threshold = false;
    }

    static constexpr int DRAG_THRESHOLD = 3;
    void on_drag_update(double _x, double)
    {
        auto& container = window_list->box;
        /* Window was not just clicked, but also dragged. Ignore the next click,
         * which is the one that happens when the drag gesture ends. */
        set_ignore_next_click();

        int x = _x + grab_start_x;
        x = container.get_absolute_position(x, button);
        if (std::abs(x - grab_abs_start_x) > DRAG_THRESHOLD)
        {
            drag_exceeds_threshold = true;
        }

        auto hovered_button = container.get_widget_at(x);

        if ((hovered_button != &button) && hovered_button)
        {
            auto children = container.get_unsorted_widgets();
            auto it = std::find(children.begin(), children.end(), hovered_button);
            container.reorder_child(button, it - children.begin());
        }

        /* Make sure the grabbed button always stays at the same relative position
         * to the DnD position */
        int target_x = x - grab_off_x;
        window_list->box.set_top_x(target_x);
    }

    void on_drag_end(double _x, double _y)
    {
        int x     = _x + grab_start_x;
        int y     = _y + grab_start_y;
        int width = button.get_allocated_width();
        int height = button.get_allocated_height();

        window_list->box.set_top_widget(nullptr);
        set_flat_class(!(state & WF_TOPLEVEL_STATE_ACTIVATED));

        /* When a button is dropped after dnd, we ignore the unclick
         * event so action doesn't happen in addition to dropping.
         * If the drag ends and the unclick event happens outside
         * the button, unset ignore_next_click or else the next
         * click on the button won't cause action. */
        if ((x < 0) || (x > width) || (y < 0) || (y > height))
        {
            unset_ignore_next_click();
        }

        /* When dragging with touch or pen, we allow some small movement while
         * still counting the action as button press as opposed to only dragging. */
        if (!drag_exceeds_threshold)
        {
            unset_ignore_next_click();
        }
    }

    bool on_release (GdkEventButton *event)
    {
        switch (event->button)
        {
            case 1: if (pressed == PRESS_LONG)
                        show_menu_with_kbd (GTK_WIDGET (button.gobj()), GTK_WIDGET (menu.gobj()));
                    else
                        on_clicked ();
                    break;

            case 3: show_menu_with_kbd (GTK_WIDGET (button.gobj()), GTK_WIDGET (menu.gobj()));
                    break;
        }

        pressed = PRESS_NONE;
        return true;
    }

    void on_menu_minimize()
    {
        menu.popdown();
        if (state & WF_TOPLEVEL_STATE_MINIMIZED)
        {
            zwlr_foreign_toplevel_handle_v1_unset_minimized(handle);
        } else
        {
            zwlr_foreign_toplevel_handle_v1_set_minimized(handle);
        }
    }

    void on_menu_maximize()
    {
        menu.popdown();
        if (state & WF_TOPLEVEL_STATE_MAXIMIZED)
        {
            zwlr_foreign_toplevel_handle_v1_unset_maximized(handle);
        } else
        {
            zwlr_foreign_toplevel_handle_v1_set_maximized(handle);
        }
    }

    void on_menu_close()
    {
        menu.popdown();
        zwlr_foreign_toplevel_handle_v1_close(handle);
    }

    bool ignore_next_click = false;
    void set_ignore_next_click()
    {
        ignore_next_click = true;

        /* Make sure that the view doesn't show clicked on animations while
         * dragging (this happens only on some themes) */
        button.set_state_flags(Gtk::STATE_FLAG_SELECTED |
            Gtk::STATE_FLAG_DROP_ACTIVE | Gtk::STATE_FLAG_PRELIGHT);
    }

    void unset_ignore_next_click()
    {
        ignore_next_click = false;
        button.unset_state_flags(Gtk::STATE_FLAG_SELECTED |
            Gtk::STATE_FLAG_DROP_ACTIVE | Gtk::STATE_FLAG_PRELIGHT);
    }

    void on_clicked()
    {
        if (mute_clicks) return;

        /* If the button was dragged, we don't want to register the click.
         * Subsequent clicks should be handled though. */
        if (ignore_next_click)
        {
            unset_ignore_next_click();
            return;
        }

        bool child_activated = false;
        for (auto c : get_children())
        {
            if (window_list->toplevels[c]->get_state() & WF_TOPLEVEL_STATE_ACTIVATED)
            {
                child_activated = true;
                break;
            }
        }

        if (!(state & WF_TOPLEVEL_STATE_ACTIVATED) && !child_activated)
        {
            auto gseat = Gdk::Display::get_default()->get_default_seat();
            auto seat  = gdk_wayland_seat_get_wl_seat(gseat->gobj());
            zwlr_foreign_toplevel_handle_v1_activate(handle, seat);
        } else
        {
            send_rectangle_hint();
            if (state & WF_TOPLEVEL_STATE_MINIMIZED)
            {
                zwlr_foreign_toplevel_handle_v1_unset_minimized(handle);
            } else
            {
                zwlr_foreign_toplevel_handle_v1_set_minimized(handle);
            }
        }
    }

    void on_allocation_changed(Gtk::Allocation& alloc)
    {
        send_rectangle_hint();
        window_list->scrolled_window.queue_allocate();
    }

    void on_scale_update()
    {
        set_app_id(app_id);
    }

    void set_app_id(std::string app_id)
    {
        this->app_id = app_id;
        IconProvider::set_image_from_icon(image, app_id,
            window_list->get_icon_size(), button.get_scale_factor());
    }

    void send_rectangle_hint()
    {
        Gtk::Widget *widget = &this->button;

        int x = 0, y = 0;
        int width = button.get_allocated_width();
        int height = button.get_allocated_height();

        while (widget)
        {
            x += widget->get_allocation().get_x();
            y += widget->get_allocation().get_y();
            widget = widget->get_parent();
        }

        auto panel =
            WayfirePanelApp::get().get_panel();
        if (!panel)
        {
            return;
        }

        zwlr_foreign_toplevel_handle_v1_set_rectangle(handle,
            panel->get_wl_surface(), x, y, width, height);
    }

    int32_t max_width = 0;
    void set_title(std::string title)
    {
        this->title = title;
        button.set_tooltip_text(title);

        set_max_width(max_width);
    }

    Glib::ustring shorten_title(int show_chars)
    {
        if (show_chars == 0)
        {
            return "";
        }

        int title_len = title.length();
        Glib::ustring short_title = title.substr(0, show_chars);
        if (title_len - show_chars >= 2)
        {
            short_title += "..";
        } else if (title_len != show_chars)
        {
            short_title += ".";
        }

        return short_title;
    }

    int get_button_preferred_width()
    {
        int min_width, preferred_width;
        button.get_preferred_width(min_width, preferred_width);

        return preferred_width;
    }

    void set_max_width(int width)
    {
        this->max_width = width;
        if (max_width == 0)
        {
            this->button.set_size_request(-1, -1);
            this->label.set_label(title);
            return;
        }

        this->button.set_size_request(width, -1);

        int show_chars = 0;
        for (show_chars = title.length(); show_chars > 0; show_chars--)
        {
            this->label.set_text(shorten_title(show_chars));
            if (get_button_preferred_width() <= max_width)
            {
                break;
            }
        }

        label.set_text(shorten_title(show_chars));
    }

    uint32_t get_state()
    {
        return this->state;
    }

    zwlr_foreign_toplevel_handle_v1 *get_parent()
    {
        return this->parent;
    }

    void set_parent(zwlr_foreign_toplevel_handle_v1 *parent)
    {
        this->parent = parent;
    }

    std::vector<zwlr_foreign_toplevel_handle_v1*>& get_children()
    {
        return this->children;
    }

    void remove_button()
    {
        auto& container = window_list->box;
        container.remove(button);
    }

    void update_menu_item_text()
    {
        if (state & WF_TOPLEVEL_STATE_MINIMIZED)
        {
            minimize.set_label(_("Unminimize"));
        } else
        {
            minimize.set_label(_("Minimize"));
        }

        if (state & WF_TOPLEVEL_STATE_MAXIMIZED)
        {
            maximize.set_label(_("Unmaximize"));
        } else
        {
            maximize.set_label(_("Maximize"));
        }
    }

    void set_flat_class(bool on)
    {
        if (on)
        {
            button.get_style_context()->add_class("flat");
        } else
        {
            button.get_style_context()->remove_class("flat");
        }
    }

    void set_state(uint32_t state)
    {
        this->state = state;
        set_flat_class(!(state & WF_TOPLEVEL_STATE_ACTIVATED));
        update_menu_item_text();
    }

    ~impl()
    {
        zwlr_foreign_toplevel_handle_v1_destroy(handle);
    }

    void handle_output_enter()
    {
        if (this->parent)
        {
            return;
        }

        auto& container = window_list->box;
        container.add(button);
        container.show_all();

        update_menu_item_text();
    }

    void handle_output_leave()
    {
    }

    void set_toggle_state ()
    {
        if (button.get_active () != this->state)
        {
            mute_clicks = true;
            button.set_active (this->state & 0x01);
            mute_clicks = false;
        }
    }
};


WayfireToplevel::WayfireToplevel(WayfireWindowList *window_list,
    zwlr_foreign_toplevel_handle_v1 *handle) :
    pimpl(new WayfireToplevel::impl(window_list, handle))
{}

void WayfireToplevel::set_width(int pixels)
{
    return pimpl->set_max_width(pixels);
}

std::vector<zwlr_foreign_toplevel_handle_v1*>& WayfireToplevel::get_children()
{
    return pimpl->get_children();
}

zwlr_foreign_toplevel_handle_v1*WayfireToplevel::get_parent()
{
    return pimpl->get_parent();
}

void WayfireToplevel::set_parent(zwlr_foreign_toplevel_handle_v1 *parent)
{
    return pimpl->set_parent(parent);
}

uint32_t WayfireToplevel::get_state()
{
    return pimpl->get_state();
}

void WayfireToplevel::update_toggle () 
{
    pimpl->set_toggle_state ();
}

WayfireToplevel::~WayfireToplevel() = default;

using toplevel_t = zwlr_foreign_toplevel_handle_v1*;
static void handle_toplevel_title(void *data, toplevel_t, const char *title)
{
    auto impl = static_cast<WayfireToplevel::impl*>(data);
    impl->set_title(title);
}

static void handle_toplevel_app_id(void *data, toplevel_t, const char *app_id)
{
    auto impl = static_cast<WayfireToplevel::impl*>(data);
    impl->set_app_id(app_id);
}

static void handle_toplevel_output_enter(void *data, toplevel_t, wl_output *output)
{
    auto impl = static_cast<WayfireToplevel::impl*>(data);
    impl->handle_output_enter();
}

static void handle_toplevel_output_leave(void *data, toplevel_t, wl_output *output)
{
    auto impl = static_cast<WayfireToplevel::impl*>(data);
    impl->handle_output_leave();
}

/* wl_array_for_each isn't supported in C++, so we have to manually
 * get the data from wl_array, see:
 *
 * https://gitlab.freedesktop.org/wayland/wayland/issues/34 */
template<class T>
static void array_for_each(wl_array *array, std::function<void(T)> func)
{
    assert(array->size % sizeof(T) == 0); // do not use malformed arrays
    for (T *entry = (T*)array->data; (char*)entry < ((char*)array->data + array->size); entry++)
    {
        func(*entry);
    }
}

static void handle_toplevel_state(void *data, toplevel_t, wl_array *state)
{
    uint32_t flags = 0;
    array_for_each<uint32_t>(state, [&flags] (uint32_t st)
    {
        if (st == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED)
        {
            flags |= WF_TOPLEVEL_STATE_ACTIVATED;
        }

        if (st == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED)
        {
            flags |= WF_TOPLEVEL_STATE_MAXIMIZED;
        }

        if (st == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED)
        {
            flags |= WF_TOPLEVEL_STATE_MINIMIZED;
        }
    });

    auto impl = static_cast<WayfireToplevel::impl*>(data);
    impl->set_state(flags);
    impl->window_list->update_toggle_states ();
}

static void handle_toplevel_done(void *data, toplevel_t)
{
// auto impl = static_cast<WayfireToplevel::impl*> (data);
}

static void remove_child_from_parent(WayfireToplevel::impl *impl, toplevel_t child)
{
    auto parent = impl->get_parent();
    auto& parent_toplevel = impl->window_list->toplevels[parent];
    if (child && parent && parent_toplevel)
    {
        auto& children = parent_toplevel->get_children();
        children.erase(std::find(children.begin(), children.end(), child));
    }
}

static void handle_toplevel_closed(void *data, toplevel_t handle)
{
    // WayfirePanelApp::get().handle_toplevel_closed(handle);
    auto impl = static_cast<WayfireToplevel::impl*>(data);
    remove_child_from_parent(impl, handle);
    impl->window_list->handle_toplevel_closed(handle);
}

static void handle_toplevel_parent(void *data, toplevel_t handle, toplevel_t parent)
{
    auto impl = static_cast<WayfireToplevel::impl*>(data);
    if (!parent)
    {
        if (impl->get_parent())
        {
            impl->handle_output_enter();
        }

        remove_child_from_parent(impl, handle);
        impl->set_parent(parent);
        return;
    }

    if (impl->window_list->toplevels[parent])
    {
        auto& children = impl->window_list->toplevels[parent]->get_children();
        children.push_back(handle);
    }

    impl->set_parent(parent);
    impl->remove_button();
}

namespace
{
struct zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_v1_impl = {
    .title  = handle_toplevel_title,
    .app_id = handle_toplevel_app_id,
    .output_enter = handle_toplevel_output_enter,
    .output_leave = handle_toplevel_output_leave,
    .state  = handle_toplevel_state,
    .done   = handle_toplevel_done,
    .closed = handle_toplevel_closed,
    .parent = handle_toplevel_parent
};
}

/* Icon loading functions */
namespace IconProvider
{
using Icon = Glib::RefPtr<Gio::Icon>;

namespace
{
std::string tolower(std::string str)
{
    for (auto& c : str)
    {
        c = std::tolower(c);
    }

    return str;
}
}

/* Gio::DesktopAppInfo
 *
 * Usually knowing the app_id, we can get a desktop app info from Gio
 * The filename is either the app_id + ".desktop" or lower_app_id + ".desktop" */
Icon get_from_desktop_app_info(std::string app_id)
{
    Glib::RefPtr<Gio::DesktopAppInfo> app_info;
        std::error_code ec;
        std::string dirs, dir, stem, id;
        std::size_t start, end;

        // an app-id should just match a desktop file...
        if (std::getenv ("XDG_DATA_DIRS")) dirs = std::getenv ("XDG_DATA_DIRS");
        else dirs = "/usr/share";
        start = 0;
        end = dirs.find (":");
        while (1)
        {
            dir = dirs.substr (start, end - start);
            size_t ind = dir.find ('~');
            if (ind != std::string::npos) dir.replace (ind, 1, std::getenv ("HOME"));

            app_info = Gio::DesktopAppInfo::create_from_filename (dir + "/applications/" + app_id + ".desktop");
            if (app_info) return app_info->get_icon();

            app_info = Gio::DesktopAppInfo::create_from_filename (dir + "/applications/" + tolower (app_id) + ".desktop");
            if (app_info) return app_info->get_icon();

            // iterate if not at end of directory list
            if (end == std::string::npos) break;
            start = end + 1;
            end = dirs.find (":", start);
        }

        // well, that didn't work then...

        // take the first part of the app-id, up to any . or the end
        std::string cmp_id1 = tolower (app_id.substr (0, app_id.find (".")));
        // take the last part of the app-id, from any . or the start
        std::string cmp_id2 = tolower (app_id.substr (app_id.find_last_of (".") + 1));

        // loop through all directories in XDG_DATA_DIRS
        start = 0;
        end = dirs.find (":");
        while (1)
        {
            dir = dirs.substr (start, end - start);
            size_t ind = dir.find ('~');
            if (ind != std::string::npos) dir.replace (ind, 1, std::getenv ("HOME"));

            // loop through all files in the applications subdirectory
            for (const auto & entry : std::filesystem::directory_iterator (dir + "/applications/", ec))
            {
                if (entry.path().extension().string() != ".desktop") continue;
                stem = entry.path().stem().string();
                id = tolower (stem.substr (stem.find_last_of (".") + 1));

                // do a caseless compare of the last part of the desktop file name against the start of the application id
                if (id == cmp_id1 || id == cmp_id2)
                {
                    app_info = Gio::DesktopAppInfo::create_from_filename (entry.path().string());
                    if (app_info) return app_info->get_icon();
                }
            }

            // iterate if not at end of directory list
            if (end == std::string::npos) break;
            start = end + 1;
            end = dirs.find (":", start);
        }

        // if we failed to match, try again, but now just try to match partial strings
        start = 0;
        end = dirs.find (":");
        while (1)
        {
            dir = dirs.substr (start, end - start);

            for (const auto & entry : std::filesystem::directory_iterator (dir + "/applications/", ec))
            {
                if (entry.path().extension().string() != ".desktop") continue;
                stem = entry.path().stem().string();
                id = tolower (stem.substr (stem.find_last_of (".") + 1));

                if (cmp_id1.length() > 1 && (id.find (cmp_id1) != std::string::npos || cmp_id1.find (id) != std::string::npos)
                    || cmp_id2.length () > 1 && (id.find (cmp_id2) != std::string::npos || cmp_id2.find (id) != std::string::npos))
                {
                    app_info = Gio::DesktopAppInfo::create_from_filename (entry.path().string());
                    if (app_info) return app_info->get_icon();
                }
            }

            if (end == std::string::npos) break;
            start = end + 1;
            end = dirs.find (":", start);
        }

        // there are limits to what it is worth trying....

    return Icon{};
}

/* Second method: Just look up the built-in icon theme,
 * perhaps some icon can be found there */

void set_image_from_icon(Gtk::Image& image,
    std::string app_id_list, int size, int scale)
{
    std::string app_id;
    std::istringstream stream(app_id_list);

    /* Wayfire sends a list of app-id's in space separated format, other compositors
     * send a single app-id, but in any case this works fine */
    while (stream >> app_id)
    {
        auto icon = get_from_desktop_app_info(app_id);
        std::string icon_name = "unknown";

        if (!icon)
        {
            /* Perhaps no desktop app info, but we might still be able to
             * get an icon directly from the icon theme */
            if (Gtk::IconTheme::get_default()->lookup_icon(app_id, 24))
            {
                icon_name = app_id;
            }
            else if (Gtk::IconTheme::get_default()->lookup_icon(tolower(app_id), 24))
            {
                    icon_name = tolower(app_id);
            }
        } else
        {
            icon_name = icon->to_string();
        }

        WfIconLoadOptions options;
        options.user_scale = scale;
        set_image_icon(image, icon_name, size, options);

        /* finally found some icon */
        if (icon_name != "unknown")
        {
            break;
        }
    }
}
}
