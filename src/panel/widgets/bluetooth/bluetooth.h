#include "../lxutils.h"

#define ICON_CACHE_SIZE 13

/* Plug-in global data */

typedef struct {
    GtkWidget *plugin;              /* Back pointer to the widget */
    //LXPanel *panel;                 /* Back pointer to panel */
    GtkWidget *tray_icon;           /* Displayed image */
    //config_setting_t *settings;     /* Plugin settings */
    int icon_size;                      /* Variables used under wf-panel */
    gboolean bottom;
    GtkWidget *menu;                /* Popup menu */
    GtkListStore *pair_list;
    GtkListStore *unpair_list;
    GtkTreeModelFilter *filter_list; /* Filtered device list used in connect dialog */
    GtkTreeModelSort *sorted_list;  /* Sorted device list used in connect dialog */
    gchar *selection;               /* Connect dialog selected item */
    GDBusConnection *busconnection;
    GDBusObjectManager *objmanager;
    GDBusProxy *agentmanager;
    GDBusProxy *adapter;
    guint agentobj;
    gchar *pairing_object;
    gchar *device_name;
    gchar *device_path;
    GtkWidget *list_dialog, *list, *list_ok;
    GtkWidget *pair_dialog, *pair_label, *pair_entry, *pair_ok, *pair_cancel;
    GtkWidget *conn_dialog, *conn_label, *conn_ok, *conn_cancel;
    GtkEntryBuffer *pinbuf;
    GDBusMethodInvocation *invocation;
    gulong ok_instance;
    gulong cancel_instance;
    guint flash_timer;
    guint flash_state;
    GdkPixbuf *icon_ref[ICON_CACHE_SIZE];
    guint hid_autopair;
} BluetoothPlugin;

extern void bt_init (BluetoothPlugin *bt);
extern void bt_update_display (BluetoothPlugin *bt);
extern gboolean bluetooth_control_msg (BluetoothPlugin *bt, const char *cmd);
