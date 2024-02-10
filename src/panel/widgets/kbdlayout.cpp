#include <glibmm.h>
#include <iostream>
#include "kbdlayout.hpp"

#include <xkbcommon/xkbcommon.h>

// needed for registering a local keyboard handler, which is currently useless
// (see comments below)
#if 0
#include <gdkmm/display.h>

#include <sys/mman.h>
#include <gdkmm/seat.h>
#include <gdk/gdkwayland.h>
#endif


#if 0
// tokenizes a comma-separated list of layouts, currently out-of-use
// (see comments below)
class tokenized_string
{
    std::vector<std::string> tsvector;

public:
    tokenized_string() {
    }

    tokenized_string(std::string str, std::string sep) {
        size_t last = 0, next = 0;
        std::string l;
        while ((next = str.find(sep, last)) != std::string::npos) {
            l = str.substr(last, next - last);
            for (auto & c : l) c = tolower(c);
            tsvector.push_back(l);
            last = next + 1;
        }
        l = str.substr(last);
        for (auto & c : l) c = tolower(c);
        tsvector.push_back(l);
    }

    std::vector<std::string> get_vector() {
        return tsvector;
    }

    const char *element(int index) {
        if (index < 0 || (unsigned int)index >= tsvector.size()) {
            return("--");
        }
        return tsvector.at(index).c_str();
    }

    virtual ~tokenized_string() = default;
};
#endif


/* this if'ed-out block of code handles keyboard as seen from the wf-panel-pi
 * application itself; it cannot be used for any of our purposes other than
 * getting the current seat's keyboard keymap; it certainly cannot be used
 * for anything like switching layouts, which seems not to be supported by
 * wayland altogether for client applications, leaving as the only possible
 * method an external (i.e., DBus) communication to send a command to some
 * wayfire plugin that would do the actual job; this might be the subject of
 * some further development of mine, but until then, there no reason for this
 * code to be compiled
 */

#if 0
static void handle_keymap(void *data, struct wl_keyboard *wl_keyboard,
                uint32_t format, int32_t fd, uint32_t size) {
    char *map_shm = (char *) mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    std::string map_text = map_shm;
    std::size_t found = 0;
    while ((found = map_text.find("name[Group")) != std::string::npos) {
        map_text = map_text.substr(found);
        found = map_text.find("=");
        map_text = map_text.substr(found);
        std::string group_text = map_text.substr(2, 100);
        std::size_t up_to = group_text.find("\n");
        // fprintf(stderr, "%s\n", group_text.substr(0, up_to - 2).c_str());
    }

    munmap(map_shm, size);
    close(fd);
}

static void handle_enter(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface,
                      struct wl_array *keys)
{
}

static void handle_leave(void *data, struct wl_keyboard *keyboard,
                      uint32_t serial, struct wl_surface *surface)
{
}

static void handle_key(void *data, struct wl_keyboard *keyboard,
                    uint32_t serial, uint32_t time, uint32_t key,
                    uint32_t state)
{
}

static void handle_repeat_info(void *data,
                wl_keyboard *keyboard,
                int32_t rate, int32_t delay) {
}

static void handle_modifiers(void *data, struct wl_keyboard *keyboard,
                          uint32_t serial, uint32_t mods_depressed,
                          uint32_t mods_latched, uint32_t mods_locked,
                          uint32_t group)
{
}

static struct wl_keyboard_listener keyboard_listener = {
    .keymap = handle_keymap,
    .enter = handle_enter,
    .leave = handle_leave,
    .key = handle_key,
    .modifiers = handle_modifiers,
    .repeat_info = handle_repeat_info,
};

static void seat_handle_capabilities(void *data, struct wl_seat *wl_seat,
                uint32_t caps) {
    WayfireKbdLayout *me = (WayfireKbdLayout *) data;

    fprintf(stderr, "seat capability (keyboard): %d\n", (enum wl_seat_capability) caps & WL_SEAT_CAPABILITY_KEYBOARD);
    if (((enum wl_seat_capability) caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
        me->keyboard = wl_seat_get_keyboard(wl_seat);
        wl_keyboard_add_listener(me->keyboard, &keyboard_listener, data);
    }
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_handle_capabilities,
    .name = NULL,
};


static void registry_add_object(void *data, struct wl_registry *registry,
    uint32_t name, const char *interface, uint32_t version)
{
    WayfireKbdLayout *me = (WayfireKbdLayout *) data;
    fprintf(stderr, "Interface: %s\n", interface);
    if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        me->seat = (wl_seat *)wl_registry_bind(registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(me->seat, &seat_listener, data);
    }
    /*
    else
    {
        fprintf(stderr, "wayland registry interface: %s\n", interface);
    }
    */
}
 
static void registry_remove_object(void *data, struct wl_registry *registry,
    uint32_t name)
{
}

static struct wl_registry_listener registry_listener =
{
    &registry_add_object,
    &registry_remove_object
};
#endif


void WayfireKbdLayout::init(Gtk::HBox *container)
{

#if 0
    auto gdk_display = gdk_display_get_default();
    auto wl_display  = gdk_wayland_display_get_wl_display(gdk_display);
    if (!wl_display)
    {
        std::cerr << "Failed to connect to wayland display!" <<
            " Are you sure you are running a wayland compositor?" << std::endl;
        std::exit(-1);
    }

    wl_registry *registry = wl_display_get_registry(wl_display);
    wl_registry_add_listener(registry, &registry_listener, this);
    wl_display_roundtrip(wl_display);

    assert(this->seat != NULL);
#endif

    std::vector<std::string> xmldirs(1, METADATA_DIR);

    button = std::make_unique<Gtk::MenuButton>();
    button->set_name ("kbdlayout");
    button->add(label);
    button->show();

    // menu.show_all();
    // menu.signal_popped_up().connect_notify(
    //    sigc::mem_fun(this, &WayfireKbdLayout::on_menu_popped_up));

    Gtk::MenuItem *mi = new Gtk::MenuItem(
        "The  KbdLayout  applet  only displays the  current layout\n"
        "and cannot  be used  to switch keyboard layouts. Please\n"
        "type your keyboard toggle combination instead.\n\n"
        "If you are only seeing  \"??\"  here, you must add \"kbdd\" to\n"
        "your $HOME/.config/wayfire.ini in the \"plugins = ...\" line."
        );
    menu.append(*mi);
    menu.show_all();

    button->set_popup(menu);

    label.show();
    label.set_margin_start (4);
    label.set_margin_end (4);

#if 0
    // for a later version...
    button->signal_button_press_event().connect_notify(
         sigc::mem_fun(this, &WayfireKbdLayout::on_button_press_event));
#endif

    long_names["??"] = "(no DBUS input yet)";
    update_label("??");
    container->pack_start(*button, false, false);

    // initially set font
    set_font();

    font.set_callback([=] () { set_font(); });
}

#if 0
void WayfireKbdLayout::on_button_press_event(GdkEventButton *event)
{
// an idea might be to grab the keyboard here (but would need to release it afterwards)
    fprintf(stderr, "button clicked!\n");
}

void WayfireKbdLayout::on_menu_shown() {
    fprintf(stderr, "menu shown!\n");
}
#endif

std::string WayfireKbdLayout::get_long_name(std::string short_name)
{
    if (long_names.find(short_name) != long_names.end()) {
        return long_names.at(short_name);
    }

    std::string short_lc = short_name;
    for (auto &c : short_lc) c = tolower(c);
    const char *layout = short_lc.c_str();
    xkb_rule_names names;

    names.model = "pc105";
    names.rules = "base";
    names.options = "";
    names.variant = "";
    names.layout = layout;

    auto ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    auto keymap = xkb_map_new_from_names(ctx, &names,
                                         XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (keymap != NULL) {
        long_names[short_name] = xkb_keymap_layout_get_name(keymap, 0);
        xkb_context_unref(ctx);
        xkb_keymap_unref(keymap);
        return long_names.at(short_name);
    }
    else {
        xkb_context_unref(ctx);
        return "(not found)";
    }
}

void WayfireKbdLayout::command(const char *cmd) {
    std::string cmdstr = cmd;
    // in a future version (still in the oven...), upon startup, kbdd sends all
    // keyboard layouts it finds in wayfire.ini, so we can build a layout switch
    // menu; currently this is useless, since this menu cannot be used to really
    // switch layouts, so this code is left out
    if (cmdstr.find(",") != std::string::npos) {
#if 0
        menu.popdown();
        button->unset_popup();
        
        for (auto widgp : menu.get_children()) {
            menu.remove(*widgp);
            // see note below about explicit destructor invocation
            auto mi = dynamic_cast<WayfireKbdLayout::MenuItem *>(widgp);
            if (mi) {
                mi->~MenuItem();
            }
        }
        for (auto short_name : tokenized_string(cmd, ",").get_vector()) {
            for (auto & c : short_name) c = toupper(c);
            std::string long_name = get_long_name(short_name);
            if (long_name != "(not found)") {
                // if we use smart pointers, MenuItems will get destructed after
                // init() method terminates and menu will end up being empty;
                // thus we use new() and destruct explicitly (see above)
                WayfireKbdLayout::MenuItem *mi =
                    new WayfireKbdLayout::MenuItem(short_name, long_name);
                    // short_name + " (" + long_name + ")");
                menu.append(*mi);
                mi->set_activate();
            }
        }
        menu.show_all();
        button->set_popup(menu);
#endif
    }
    else {
        update_label(cmd);
    }
}

void WayfireKbdLayout::update_label(const char *layout) {
    std::string text = layout;
    if (label.get_text() != text)
    {
        label.set_text(text);
        button->set_tooltip_text(get_long_name(text));
    }
}


void WayfireKbdLayout::set_font()
{
    if ((std::string)font == "default")
    {
        label.unset_font();
    } else
    {
        label.override_font(Pango::FontDescription((std::string)font));
    }
}

WayfireKbdLayout::~WayfireKbdLayout()
{
    timeout.disconnect();
}
