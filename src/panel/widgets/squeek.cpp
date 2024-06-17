#include <glibmm.h>
#include "squeek.hpp"

extern "C" {
#include "lxutils.h"
}

GDBusProxy *proxy;

void WayfireSqueek::icon_size_changed_cb (void)
{
    switch (icon_size)
    {
        case 16 :   icon->set_from_icon_name ("input-keyboard", Gtk::ICON_SIZE_SMALL_TOOLBAR);
                    break;
        case 24 :   icon->set_from_icon_name ("input-keyboard", Gtk::ICON_SIZE_LARGE_TOOLBAR);
                    break;
        case 32 :   icon->set_from_icon_name ("input-keyboard", Gtk::ICON_SIZE_DND);
                    break;
        case 48 :   icon->set_from_icon_name ("input-keyboard", Gtk::ICON_SIZE_DIALOG);
                    break;
    }
}

bool WayfireSqueek::set_icon (void)
{
    icon_size_changed_cb ();
    return false;
}

void WayfireSqueek::on_button_press_event (void)
{
    GError *err = NULL;
    gboolean res;

    GVariant *val = g_dbus_proxy_get_cached_property (proxy, "Visible");
    g_variant_get (val, "b", &res);

    GVariant *set = g_variant_new ("(b)", !res);
    g_dbus_proxy_call_sync (proxy, "SetVisible", set, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
    if (err) printf ("%s\n", err->message);
}

/* Callback for BlueZ appearing on D-Bus */

static void sb_cb_name_owned (GDBusConnection *conn, const gchar *name, const gchar *, gpointer user_data)
{
    printf ("Name %s owned on D-Bus\n", name);
    GError *err = NULL;
    proxy = g_dbus_proxy_new_sync (conn, G_DBUS_PROXY_FLAGS_NONE, NULL, name, "/sm/puri/OSK0", "sm.puri.OSK0", NULL, &err);
    if (err) printf ("%s\n", err->message);
    gtk_widget_show (GTK_WIDGET (user_data));
}

/* Callback for BlueZ disappearing on D-Bus */

static void sb_cb_name_unowned (GDBusConnection *, const gchar *name, gpointer user_data)
{
    printf ("Name %s unowned on D-Bus\n", name);
    gtk_widget_hide (GTK_WIDGET (user_data));
}

void WayfireSqueek::init (Gtk::HBox *container)
{
    /* Create the button */
    plugin = std::make_unique <Gtk::Button> ();
    container->pack_start (*plugin, false, false);

    /* Create the icon */
    icon = std::make_unique <Gtk::Image> ();
    plugin->add (*icon.get());
    plugin->signal_clicked().connect (sigc::mem_fun (*this, &WayfireSqueek::on_button_press_event));

    /* Setup structure */
    icon_timer = Glib::signal_idle().connect (sigc::mem_fun (*this, &WayfireSqueek::set_icon));

    /* Setup callbacks */
    icon_size.set_callback (sigc::mem_fun (*this, &WayfireSqueek::icon_size_changed_cb));

    /* Set up callbacks to see if squeekboard is on D-Bus */
    g_bus_watch_name (G_BUS_TYPE_SESSION, "sm.puri.OSK0", G_BUS_NAME_WATCHER_FLAGS_NONE, sb_cb_name_owned, sb_cb_name_unowned, (*plugin).gobj(), NULL);
}

WayfireSqueek::~WayfireSqueek()
{
    icon_timer.disconnect ();
}
