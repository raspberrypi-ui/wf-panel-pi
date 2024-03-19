#include <glibmm.h>
#include <iostream>
#include "kbdlayout.hpp"

#include <xkbcommon/xkbcommon.h>

#if 0
// needed for registering a local keyboard handler, which is currently useless
// (see comments below)
#include <gdkmm/display.h>
#include <sys/mman.h>
#include <gdkmm/seat.h>
#include <gdk/gdkwayland.h>
#endif



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



// resets our menu to either its initial state (a message informing that the
// switching functionality is not available) or to a list of layouts

void WayfireKbdLayout::reset_menu(const char *layout_list) {
    menu.popdown();
    button->unset_popup();

    for (auto widgp : menu.get_children()) {
        menu.remove(*widgp);
        // see note below about explicit destructor invocation
        auto mi = dynamic_cast<WayfireKbdLayout::MenuItem *>(widgp);
        if (mi) {
            mi->~MenuItem();
            // HERE: make it a method and include it in destructor???
        }
    }
    
    if (layout_list == NULL) {
        // if we use smart pointers, MenuItem(s) get destructed after
        // init() method terminates and menu will end up being empty;
        // thus we use new() and destruct explicitly (see above)
        Gtk::MenuItem *mi = new Gtk::MenuItem(
            "The  KbdLayout  applet  only displays the  current layout\n"
            "and cannot  be used  to switch keyboard layouts. Please\n"
            "type your keyboard toggle combination instead.\n\n"
            "If you are only seeing  \"??\"  here, you must add \"kbdd\" to\n"
            "your $HOME/.config/wayfire.ini in the \"plugins = ...\" line."
            );
        menu.append(*mi);
    }
    else {
        for (auto short_name : tokenized_string(layout_list, ",").get_vector()) {
            for (auto & c : short_name) c = toupper(c);
            std::string long_name = get_long_name(short_name);
            if (long_name != "(not found)") {
                // see above comment about new()
                WayfireKbdLayout::MenuItem *mi =
                    new WayfireKbdLayout::MenuItem(this, short_name, long_name);
                menu.append(*mi);
                mi->set_activate();
            }
        }
    }

    menu.show_all();
    button->set_popup(menu);
}



// our constructor; nothing fancy here, just initialize things around

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

    // button
    button = std::make_unique<Gtk::MenuButton>();
    button->set_name ("kbdlayout");
    button->add(label);
    button->show();
#if 0
    button->signal_button_press_event().connect_notify(
         sigc::mem_fun(this, &WayfireKbdLayout::on_button_press_event));
#endif

    // menu
    reset_menu(NULL);

    // label
    label.show();
    label.set_margin_start (4);
    label.set_margin_end (4);

    // init label and tooltip
    long_names["??"] = "(no DBUS input yet)";
    update_label("??");
    container->pack_start(*button, false, false);

    // dbus
    dbus_initialize();

    // initially set font
    set_font();
    font.set_callback([=] () { set_font(); });
}

#if 0
void WayfireKbdLayout::on_button_press_event(GdkEventButton *event)
{
}
#endif



// translates a short keyboard layout name to a long one (used in menus
// and tooltips), by creating a kbd_keymap and getting the description
// from there and by remembering old mappings

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


// processes DBus "commands" sent to us via wfpanelctl; recognized "commands"
// are one of the following:
// (a) xkb_keymap-compatible layout codes like "US", "UK", etc; this updates
//     the current label and tooltip
// (b) a comma-separated list of such layouts, serving to (re) build our menu
// (c) a "-", resulting in the menu being reset to an empty/warning message
//     (this is part of the initialization sequence performed by kbdd)
// (d) a "~" resulting in the menu being reset as above and "??" being shown
//     (this is sent if kbdd is dynamically unloaded during execution)

void WayfireKbdLayout::command(const char *cmd) {
    std::string cmdstr = cmd;
    // upon startup, kbdd sends all keyboard layouts it finds in wayfire.ini, so we
    // can build a layout switch menu 
    
    if (cmdstr.find(",") != std::string::npos) {
        reset_menu(cmd);
    }
    else if (cmdstr == "-") {
        reset_menu(NULL);
    }
    else if (cmdstr == "~") {
        reset_menu(NULL);
        update_label("??");
    }
    else {
        update_label(cmd);
    }
}


// updates our label; if the label is set to "??", a switch off/on sequence
// of commands is sent to kbdd

void WayfireKbdLayout::update_label(const char *layout) {
    std::string text = layout;

    if (label.get_text() != text) {
        if (label.get_text() == "??") {
            Glib::signal_timeout().connect_once(
                sigc::mem_fun(this, &WayfireKbdLayout::dbus_call_enable_off_on_pulse),
                300);
        }
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


// kbdd DBus introspection XML

// TODO: this introspection XML is common to this plugin and kbdd; it thus
// should be moved into some include file for both kbdd and us

static const auto introspection_data = Gio::DBus::NodeInfo::create_for_xml(
    R"(
<?xml version="1.0" encoding="UTF-8"?>
<node>
  <interface name="org.wayfire.kbdd.layout">
    <signal name="changed">
      <arg type="s" name="layout"/>
    </signal>
    <!--
        The "enable method takes one unsigned integer (status) as argument;
        it is supposed to be called by a shell app to initialize or to stop
        kbdd's keyboard layout switching protocol; if "status" is non-zero,
        kbdd will henceforth honor "switch" commands; if "status" is zero,
        switching will be disabled (default state is desabled); kbdd keeps
        track of the last focus switch before this message, assuming that
        this last-focused-to view is the shell; it then keeps track of the
        focus switches to remember which was the last focused-to view
        besides the shell; it is this last-focused-to view that gets its
        keyboard changed by a subsequent "switch" command.
    -->
    <method name="enable">
      <arg type="u" name="status" direction="in"/>
    </method>
    <!--
        The "switch" method takes as an argument a string representing a
        keyboard layout (in capitalized xkbd 2-letter format, e.g., "US",
        "RU", "GR", etc.); if this layout is valid (in the sense that it
        is a configured xkbd layout), it is accepted, otherwise it is
        rejected; if accepted, the last-non-shell view gets its stored
        keyboard layout switched accordingly, so when focus returns to
        that view, the new layout applies.
    -->
    <method name="switch">
      <arg type="s" name="layout" direction="in"/>
    </method>
  </interface>
</node>
    )");


// initialize DBus; we are only using DBus for the send-side of things,
// since wfpanelctl is used to receive DBUs messages; since only async
// method call invocations are used, we do not need a Glib main loop
// other than for initializing DBus, which we do here

void WayfireKbdLayout::dbus_initialize()
{
    // it appears this is not needed anymore
    // Gio::init();
    loop = Glib::MainLoop::create();

    connection = Gio::DBus::Connection::get_sync(Gio::DBus::BUS_TYPE_SESSION);
    if (!connection) {
        //FIXME:
        std::cerr << "get_sync() failed" << std::endl;
    }
    else {
        Gio::DBus::Proxy::create(connection, 
        "org.wayfire.kbdd.layout", "/org/wayfire/kbdd/layout",
            "org.wayfire.kbdd.layout",
            // sigc::ptr_fun(&on_dbus_proxy_available))
            [=] (Glib::RefPtr<Gio::AsyncResult>& result) {
                proxy = Gio::DBus::Proxy::create_finish(result);
                loop->quit();
            });
        // will only run until above lambda is executed, since it ends with loop->quit()
        loop->run();
    }

}


// sends to kbdd an "enable" command over DBus, passing a zero (false) or a
// non-zero (true) parameter

void WayfireKbdLayout::dbus_call_enable(bool flag)
{
    if (!connection) {
        std::cerr << "dbus_call_enable: no DBus connection!" << std::endl;
        return;
    }
    if (!proxy) {
        std::cerr << "DBus proxy not connected!" << std::endl;
        return;
    }
    std::vector<Glib::VariantBase> params_vector(1);
    params_vector[0] = Glib::Variant<guint32>::create(flag? 1 : 0);
    Glib::VariantContainerBase params = Glib::VariantContainerBase::create_tuple(params_vector);
    proxy->call("enable", params);
}


// pulses the enable status of kbdd off and then back on (during (re-)initialization)

void WayfireKbdLayout::dbus_call_enable_off_on_pulse() {
    dbus_call_enable(false);
    Glib::signal_timeout().connect_once(
        [=]() {
            dbus_call_enable(true);
        },
        200
    );
}


// sends a "switch" command to kbdd over DBus, causing kbdd to switch the keyboard
// layout of the last non-shell view to the selected one

void WayfireKbdLayout::dbus_call_switch(std::string layout)
{
    if (!connection) {
        std::cerr << "dbus_call_switch: no DBus connection!" << std::endl;
        return;
    }
    if (!proxy) {
        std::cerr << "DBus proxy not connected!" << std::endl;
        return;
    }
    std::vector<Glib::VariantBase> params_vector(1);
    params_vector[0] = Glib::Variant<Glib::ustring>::create(layout);
    Glib::VariantContainerBase params = Glib::VariantContainerBase::create_tuple(params_vector);
    proxy->call("switch", params);
}


// performs termination actions (currently, just disabling kbdd's DBus operations)

void WayfireKbdLayout::dbus_terminate()
{
    dbus_call_enable(false);
}

WayfireKbdLayout::~WayfireKbdLayout()
{
    // in case wf-panel-pi receives an interrupt, these get never executed; a caveat
    // is therefore that in such a case, kbdd remains enabled and keeps sending
    // commands
    dbus_terminate();
    timeout.disconnect();
}
