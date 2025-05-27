#include "launchers.hpp"
#include <giomm/file.h>
#include <glibmm/spawn.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/icontheme.h>
#include <gdk/gdkcairo.h>
#include <cassert>
#include <iostream>
#include <gtk-utils.hpp>
#include <wf-shell-app.hpp>

extern "C" {
#include "launcher.h"
#include "lxutils.h"

    WayfireWidget *create () { return new WayfireLaunchers; }
    void destroy (WayfireWidget *w) { delete w; }

    static constexpr conf_table_t conf_table[2] = {
        {CONF_TYPE_INT,     "spacing",  N_("Icon spacing"), NULL},
        {CONF_TYPE_NONE,    NULL,       NULL,               NULL}
    };
    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return N_("Launcher"); };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}

// create launcher from a .desktop file or app-id
struct DesktopLauncherInfo : public LauncherInfo
{
    Glib::RefPtr<Gio::DesktopAppInfo> app_info;
    bool load(std::string name)
    {
        // attempt from App-ID
        app_info = Gio::DesktopAppInfo::create(name);
        if (!app_info)
        {
            // attempt to interpret it as a .desktop file
            app_info = Gio::DesktopAppInfo::create_from_filename(name);
        }

        // neither interpretation succeeded
        if (!app_info)
        {
            return false;
        }

        return true;
    }

    Glib::RefPtr<Gdk::Pixbuf> get_pixbuf(int32_t size)
    {
        auto iconp = app_info->get_icon();
        if (!iconp)
        {
            std::cerr << "No icon defined for launcher" << std::endl;
            return Glib::RefPtr<Gdk::Pixbuf>();
        }
        auto icon = iconp->to_string();
        auto theme = Gtk::IconTheme::get_default();

        // look up explicit paths
        if (icon.find ("/") != std::string::npos)
            return Gdk::Pixbuf::create_from_file (icon, size, size);

        if (!theme->lookup_icon(icon, size))
        {
            std::cerr << "Failed to load icon \"" << icon << "\"" << std::endl;
            return Glib::RefPtr<Gdk::Pixbuf>();
        }

        return theme->load_icon(icon, size)
               ->scale_simple(size, size, Gdk::INTERP_BILINEAR);
    }

    std::string get_text()
    {
        return app_info->get_name();
    }

    void execute()
    {
        app_info->launch(std::vector<Glib::RefPtr<Gio::File>>());
    }

    std::string get_filename()
    {
        return app_info->get_filename();
    }

    virtual ~DesktopLauncherInfo()
    {}
};

// create a launcher from a command + icon
struct FileLauncherInfo : public LauncherInfo
{
    std::string command;
    std::string icon;
    std::string label;

    bool load(std::string command, std::string icon, std::string label)
    {
        this->command = command;
        this->icon    = icon;
        if (label == "")
        {
            this->label = command;
        } else
        {
            this->label = label;
        }

        return load_icon_pixbuf_safe(icon, 24).get() != nullptr;
    }

    Glib::RefPtr<Gdk::Pixbuf> get_pixbuf(int32_t size)
    {
        return Gdk::Pixbuf::create_from_file(icon, size, size);
    }

    std::string get_text()
    {
        return label;
    }

    void execute()
    {
        Glib::spawn_command_line_async("/bin/bash -c \'" + command + "\'");
    }

    std::string get_filename()
    {
        return "";
    }

    virtual ~FileLauncherInfo()
    {}
};

void WfLauncherButton::set_size(int size)
{
    int full_size = base_size * LAUNCHERS_ICON_SCALE;

    /* set button spacing */
    evbox.set_margin_top((full_size - size) / 2);
    evbox.set_margin_bottom((full_size - size + 1) / 2);

    evbox.set_margin_left((full_size - size) / 2);
    evbox.set_margin_right((full_size - size + 1) / 2);

    // initial scale
    on_scale_update();
}

bool WfLauncherButton::initialize(std::string name, std::string icon, std::string label)
{
    launcher_name = name;
    base_size = WfOption<int> {"panel/icon_size"};
    if (icon == "none")
    {
        auto dl = new DesktopLauncherInfo();
        if (!dl->load(name))
        {
            std::cerr << "Failed to load info for " << name << std::endl;
            return false;
        }

        info = dl;
    } else
    {
        auto fl = new FileLauncherInfo();
        if (!fl->load(name, icon, label))
        {
            std::cerr << "Failed to load icon " << icon << std::endl;
            return false;
        }

        info = fl;
    }

    evbox.add(image);
    evbox.signal_button_release_event().connect(sigc::mem_fun(this, &WfLauncherButton::on_release));
    evbox.set_relief (Gtk::RELIEF_NONE);

    gesture = detect_long_press (evbox);

    evbox.signal_map().connect([=] ()
    {
        set_size(base_size);
    });

    current_size.set(base_size, base_size);
    evbox.property_scale_factor().signal_changed()
        .connect(sigc::mem_fun(this, &WfLauncherButton::on_scale_update));

    evbox.set_tooltip_text(info->get_text());

    remove.set_label (_("Remove from Launcher"));
    remove.signal_activate().connect(sigc::mem_fun(this, &WfLauncherButton::on_remove));
    menu.attach (remove, 0, 1, 0, 1);
    menu.attach_to_widget (evbox);
    menu.show_all();

    return true;
}

bool WfLauncherButton::on_release(GdkEventButton *ev)
{
    switch (ev->button)
    {
        case 1: if (pressed == PRESS_LONG)
                    show_menu_with_kbd (GTK_WIDGET (evbox.gobj()), GTK_WIDGET (menu.gobj()));
                else
                {
                    assert(info);
                    info->execute();
                }
                break;

        case 3: show_menu_with_kbd (GTK_WIDGET (evbox.gobj()), GTK_WIDGET (menu.gobj()));
                break;
    }

    pressed = PRESS_NONE;
    return true;
}

void WfLauncherButton::on_remove ()
{
    std::string str = info->get_filename();
    size_t last = str.find_last_of('/');
    remove_from_launcher (str.substr(last + 1).c_str());
}

/* Because icons can have different sizes, we need to use a Gdk::Pixbuf
 * to convert them to the appropriate size. However, Gdk::Pixbuf operates
 * in absolute pixel size, so this doesn't work nicely with scaled outputs.
 *
 * To get around the problem, we first create the Pixbuf with a scaled size,
 * then convert it to a cairo_surface with the appropriate scale, and use this
 * cairo surface as the source for the Gtk::Image */
void WfLauncherButton::on_scale_update()
{
    int scale = image.get_scale_factor();

    // hold a reference to the RefPtr
    auto ptr_pbuff = info->get_pixbuf(current_size * image.get_scale_factor());
    if (!ptr_pbuff)
    {
        return;
    }

    set_image_pixbuf(image, ptr_pbuff, scale);
}

WfLauncherButton::WfLauncherButton()
{}
WfLauncherButton::~WfLauncherButton()
{
    delete info;
}

static bool begins_with(const std::string& string, const std::string& prefix)
{
    return string.size() >= prefix.size() &&
           string.substr(0, prefix.size()) == prefix;
}

launcher_container WayfireLaunchers::get_launchers_from_config()
{
    auto section = WayfireShellApp::get().config.get_section("panel");
    const std::string desktop_prefix    = "launcher_";
    const std::string file_icon_prefix  = "launcher_icon_";
    const std::string file_cmd_prefix   = "launcher_cmd_";
    const std::string file_label_prefix = "launcher_label_";

    launcher_container launchers;
    auto try_push_launcher = [&launchers] (
        const std::string cmd, const std::string icon, const std::string label = "")
    {
        auto launcher = new WfLauncherButton();
        if (launcher->initialize(cmd, icon, label))
        {
            launchers.push_back(std::unique_ptr<WfLauncherButton>(launcher));
        } else
        {
            delete launcher;
        }
    };

    for (auto opt : section->get_registered_options())
    {
        /* we have a command */
        if (opt->get_value_str().empty()) continue;
        if (begins_with(opt->get_name(), file_cmd_prefix))
        {
            /* extract launcher name, i.e the string after the prefix */
            auto launcher_name = opt->get_name().substr(file_cmd_prefix.size());
            /* look for the corresponding icon */
            auto icon_option = section->get_option_or(file_icon_prefix + launcher_name);
            if (icon_option)
            {
                /* bingo, found command + icon
                 * now look for the corresponding label  */
                auto label_option = section->get_option_or(file_label_prefix + launcher_name);
                if (label_option)
                {
                    /* found label */
                    try_push_launcher(opt->get_value_str(), icon_option->get_value_str(),
                        label_option->get_value_str());
                } else
                {
                    try_push_launcher(opt->get_value_str(), icon_option->get_value_str());
                }
            }
        }

        /* an entry is a deskop-file entry if the it has the desktop prefix
         * but not the file_icon, file_cmd or file_label prefix */
        if (begins_with(opt->get_name(), desktop_prefix) &&
            !begins_with(opt->get_name(), file_icon_prefix) &&
            !begins_with(opt->get_name(), file_cmd_prefix) &&
            !begins_with(opt->get_name(), file_label_prefix))
        {
            try_push_launcher(opt->get_value_str(), "none");
        }
    }

    return launchers;
}

void WayfireLaunchers::init(Gtk::HBox *container)
{
    box.set_name (PLUGIN_NAME);
    container->pack_start(box, false, false);
    handle_config_reload();
}

void WayfireLaunchers::handle_config_reload()
{
    box.set_spacing(WfOption<int>{"panel/launchers_spacing"});

    launchers = get_launchers_from_config();
    for (auto& l : launchers)
    {
        box.pack_start(l->evbox, false, false);
    }

    box.show_all();
}
