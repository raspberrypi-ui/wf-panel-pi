#ifndef LAUNCHERS_HPP
#define LAUNCHERS_HPP

#include <widget.hpp>
#include <vector>
#include <giomm/desktopappinfo.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/image.h>
#include <gtkmm/hvbox.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/button.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/gesturelongpress.h>
#include "config/duration.hpp"

#define LAUNCHERS_ICON_SCALE 1.0

struct LauncherInfo
{
    virtual Glib::RefPtr<Gdk::Pixbuf> get_pixbuf(int32_t size) = 0;
    virtual std::string get_text() = 0;
    virtual std::string get_filename() = 0;
    virtual void execute() = 0;
    virtual ~LauncherInfo()
    {}
};

class LauncherAnimation :
    public wf::animation::duration_t,
    public wf::animation::timed_transition_t
{
  public:
    LauncherAnimation(wf::option_sptr_t<int> length, int start, int end) :
        duration_t(length, wf::animation::smoothing::linear),
        timed_transition_t((duration_t&)*this)
    {
        this->set(start, end);
        this->duration_t::start();
    }
};

struct WfLauncherButton
{
    std::string launcher_name;
    int32_t base_size;

    Gtk::Image image;
    Gtk::Button evbox;
    Gtk::Menu menu;
    Gtk::MenuItem remove;
    Glib::RefPtr<Gtk::GestureLongPress> gesture;
    LauncherInfo *info = NULL;
    LauncherAnimation current_size{wf::create_option(1000), 0, 0};

    WfLauncherButton();
    WfLauncherButton(const WfLauncherButton& other) = delete;
    WfLauncherButton& operator =(const WfLauncherButton&) = delete;
    ~WfLauncherButton();

    bool initialize(std::string name, std::string icon = "none", std::string label = "");

    bool on_release(GdkEventButton *ev);
    void on_gesture_pressed(double, double);

    void on_scale_update();
    void on_remove();

    void set_size(int size);
};

using launcher_container = std::vector<std::unique_ptr<WfLauncherButton>>;
class WayfireLaunchers : public WayfireWidget
{
    Gtk::HBox box;
    launcher_container launchers;
    launcher_container get_launchers_from_config();

  public:
    virtual void init(Gtk::HBox *container);
    virtual void handle_config_reload();
    virtual ~WayfireLaunchers()
    {}
};

#endif /* end of include guard: LAUNCHERS_HPP */
