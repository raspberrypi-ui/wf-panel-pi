#ifndef WF_PANEL_HPP
#define WF_PANEL_HPP

#include <memory>
#include <wayland-client.h>
#include <gtkmm/window.h>

#include "widget.hpp"
#include "wf-shell-app.hpp"

class WayfirePanel
{
    public:
    WayfirePanel(WayfireOutput *output, bool real);

    wl_surface *get_wl_surface();
    Gtk::Window& get_window();
    void handle_config_reload();
    void handle_command_message (const char *plugin, const char *cmd);
    int set_monitor();
    WayfireOutput *get_output();

    private:
    class impl;
    std::unique_ptr<impl> pimpl;
};

class WayfirePanelApp : public WayfireShellApp
{
  public:
    WayfirePanel* panel_for_wl_output(wl_output *output);
    static WayfirePanelApp& get();

    /* Starts the program. get() is valid afterward the first (and the only)
     * call to create() */
    static void create(int argc, char **argv);
    ~WayfirePanelApp();

    void handle_new_output(WayfireOutput *output) override;
    void handle_output_removed(WayfireOutput *output) override;
    void on_config_reload() override;
    void update_panels ();

  private:
    WayfirePanelApp(int argc, char **argv);

    class impl;
    std::unique_ptr<impl> priv;

    void on_command (const char *plugin, const char *command);

    static void on_bus_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data);
    static void on_name_acquired (GDBusConnection *connection, const gchar *name, gpointer user_data);
    static void on_name_lost (GDBusConnection *connection, const gchar *name, gpointer user_data);

    static void handle_method_call (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
        const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data);
    static GVariant *handle_get_property (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
        const gchar *property_name, GError **error, gpointer user_data);
    static gboolean handle_set_property (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
        const gchar *property_name, GVariant *value, GError **error, gpointer user_data);

    static const GDBusInterfaceVTable interface_vtable;
};

#endif /* end of include guard: WF_PANEL_HPP */

