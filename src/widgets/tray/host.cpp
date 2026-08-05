#include "host.hpp"
#include "tray.hpp"
#include "watcher.hpp"

StatusNotifierHost::StatusNotifierHost(WidgetStatusNotifier *tray) :
    dbus_name_id(Gio::DBus::own_name(Gio::DBus::BUS_TYPE_SESSION,
        "org.kde.StatusNotifierHost-" + std::to_string(getpid()) + "-" +
        std::to_string(++hosts_counter),
        sigc::mem_fun(*this, &StatusNotifierHost::on_bus_acquired))),
    tray(tray)
{}

void StatusNotifierHost::on_bus_acquired(const Glib::RefPtr<Gio::DBus::Connection> & connection,
    const Glib::ustring & name)
{
    watcher_id = Gio::DBus::watch_name(
        connection, Watcher::SNW_NAME,
        [this, host_name = name, destroyed = destroyed] (const Glib::RefPtr<Gio::DBus::Connection> & connection,
                                  const Glib::ustring & name,
                                  const Glib::ustring & name_owner)
    {
        if (*destroyed) return;

        Gio::DBus::Proxy::create(
            connection, Watcher::SNW_NAME, Watcher::SNW_PATH, Watcher::SNW_IFACE,
            [this, host_name, destroyed] (const Glib::RefPtr<Gio::AsyncResult> & result)
        {
            if (*destroyed) return;

            watcher_proxy = Gio::DBus::Proxy::create_finish(result);

            watcher_proxy->call("RegisterStatusNotifierHost",
                Glib::Variant<std::tuple<Glib::ustring>>::create({host_name}));
            watcher_proxy->signal_signal().connect([this, destroyed] (const Glib::ustring & sender_name,
                                                           const Glib::ustring & signal_name,
                                                           const Glib::VariantContainerBase & params)
            {
                if (*destroyed) return;

                if (!params.is_of_type(Glib::VariantType("(s)")))
                {
                    return;
                }

                Glib::Variant<Glib::ustring> item_path;
                params.get_child(item_path);
                if (signal_name == "StatusNotifierItemRegistered")
                {
                    tray->add_item(item_path.get());
                } else if (signal_name == "StatusNotifierItemUnregistered")
                {
                    tray->remove_item(item_path.get());
                }
            });
            Glib::Variant<std::vector<Glib::ustring>> registered_items_var;
            watcher_proxy->get_cached_property(registered_items_var, "RegisteredStatusNotifierItems");
            if (!registered_items_var.gobj()) return;
            for (const auto & service : registered_items_var.get())
            {
                tray->add_item(service);
            }
        });
    },
        [this, destroyed = destroyed] (const Glib::RefPtr<Gio::DBus::Connection> & connection, const Glib::ustring & name)
    {
        if (*destroyed) return;

        Gio::DBus::unwatch_name(watcher_id);
    });
}

StatusNotifierHost::~StatusNotifierHost()
{
    *destroyed = true;
    Gio::DBus::unwatch_name(watcher_id);
    Gio::DBus::unown_name(dbus_name_id);
}
