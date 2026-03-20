/*
Copyright (c) 2021 Raspberry Pi
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <gtk/gtk.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include "notification.h"
#include "lxutils.h"

/*----------------------------------------------------------------------------*/
/* Macros and typedefs */
/*----------------------------------------------------------------------------*/

#define TEXT_WIDTH 50
#define SPACING 5

#define INIT_MUTE 2500
#define INTERVAL_MS 500

typedef struct {
    GtkWidget *popup;               /* Popup message window*/
    guint hide_timer;               /* Timer to hide message window */
    int seq;                        /* Sequence number */
    guint hash;                     /* Hash of message string */
    char *message;
    gboolean shown;
    gboolean critical;
    int timeout;
    char *sender;                  /* DBus only */
    char **actions;                /* DBus only */
    char *icon_name;
    GdkPixbuf *icon;
} NotifyWindow;

#define DBUS_BUS_NAME       "org.freedesktop.Notifications"
#define DBUS_OBJECT_PATH    "/org/freedesktop/Notifications"
#define DBUS_INTERFACE_NAME "org.freedesktop.Notifications"

/*----------------------------------------------------------------------------*/
/* Global data */
/*----------------------------------------------------------------------------*/

static gboolean notifications;
static gint notify_timeout;
static GtkWindow *panel;

static GList *nwins = NULL;         /* List of current notifications */
static int nseq = 1;                /* Sequence number for notifications */
static gint interval_timer = 0;     /* Used to show windows one at a time */

static guint owner_id;

static GDBusConnection *dbusconn;

static GDBusNodeInfo *introspection_data = NULL;

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.freedesktop.Notifications'>"
  "  <method name='Notify'>"
  "    <arg type='s' name='app_name' direction='in' />"
  "    <arg type='u' name='id' direction='in' />"
  "    <arg type='s' name='icon' direction='in' />"
  "    <arg type='s' name='summary' direction='in' />"
  "    <arg type='s' name='body' direction='in' />"
  "    <arg type='as' name='actions' direction='in' />"
  "    <arg type='a{sv}' name='hints' direction='in' />"
  "    <arg type='i' name='timeout' direction='in' />"
  "    <arg type='u' name='return_id' direction='out' />"
  "  </method>"
  "  <method name='CloseNotification'>"
  "    <arg type='u' name='id' direction='in' />"
  "  </method>"
  "  <method name='GetCapabilities'>"
  "    <arg type='as' name='return_caps' direction='out'/>"
  "  </method>"
  "  <method name='GetServerInformation'>"
  "    <arg type='s' name='return_name' direction='out'/>"
  "    <arg type='s' name='return_vendor' direction='out'/>"
  "    <arg type='s' name='return_version' direction='out'/>"
  "    <arg type='s' name='return_spec_version' direction='out'/>"
  "  </method>"
  "  <signal name='NotificationClosed'>"
  "    <arg name='id' type='u'/>"
  "    <arg name='reason' type='u'/>"
  "  </signal>"
  "  <signal name='ActionInvoked'>"
  "    <arg name='id' type='u'/>"
  "    <arg name='action_key' type='s'/>"
  "  </signal>"
  "  </interface>"
  "</node>";

/*----------------------------------------------------------------------------*/
/* Function prototypes */
/*----------------------------------------------------------------------------*/

static void show_message (NotifyWindow *nw, char *str);
static gboolean hide_message_timeout (NotifyWindow *nw);
static void hide_message (NotifyWindow *nw, int reason);
static void replace_message (int id, const char *message);
static void update_positions (GList *item, int offset);
static gboolean window_click (GtkWidget *widget, GdkEventButton *event, NotifyWindow *nw);
static int wfpanel_notify_int (const char *message, const char *sender, gchar **actions, int timeout, const char *icon_name, GdkPixbuf *icon);

/*----------------------------------------------------------------------------*/
/* FreeDesktop notification DBus interface */
/*----------------------------------------------------------------------------*/

static void icon_free (guchar *data, gpointer)
{
    g_free (data);
}

static GdkPixbuf *load_pixbuf_from_data (GVariant *value)
{
    GdkPixbuf *pixbuf = NULL;
    GVariant *pix_v = NULL;
    int w, h, str, alpha, bps, ch;
    unsigned char *pixels;

    if (g_strcmp0 (g_variant_get_type_string (value), "(iiibiiay)")) return NULL;

    g_variant_get (value, "(iiibii@ay)", &w, &h, &str, &alpha, &bps, &ch, &pix_v);
    pixels = (unsigned char *) g_memdup2 (g_variant_get_data (pix_v), g_variant_get_size (pix_v));
    g_variant_unref (pix_v);

    pixbuf = gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB, alpha, bps, w, h, str, icon_free, NULL);

    return pixbuf;
}

static void handle_method_call (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
    const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data)
{
    if (!g_strcmp0 (method_name, "GetServerInformation"))
    {
        GVariant *reply;

        reply = g_variant_new ("(ssss)", "wf-panel-pi", "RaspberryPi", "1.0", "1.2");
        g_dbus_method_invocation_return_value (invocation, reply);
        g_dbus_connection_flush (connection, NULL, NULL, NULL);
    }

    if (!g_strcmp0 (method_name, "GetCapabilities"))
    {
        GVariantBuilder *builder;
        GVariant *reply;

        builder = g_variant_builder_new (G_VARIANT_TYPE("as"));
        g_variant_builder_add (builder, "s", "actions");
        g_variant_builder_add (builder, "s", "body");
        g_variant_builder_add (builder, "s", "persistence");

        reply = g_variant_new ("(as)", builder);
        g_clear_pointer (&builder, g_variant_builder_unref);
        g_dbus_method_invocation_return_value (invocation, reply);
        g_dbus_connection_flush (connection, NULL, NULL, NULL);
    }

    if (!g_strcmp0 (method_name, "Notify"))
    {
        GVariant *reply;
        GVariantIter i;
        char *app_name, *icon_name, *summary, *body, *message;
        int repl_id, id, timeout, count;
        gchar **actions;
        GVariant *hints, *value;
        GdkPixbuf *icon_pb = NULL;

        g_variant_iter_init (&i, parameters);
        g_variant_iter_next (&i, "s", &app_name);
        g_variant_iter_next (&i, "u", &repl_id);
        g_variant_iter_next (&i, "s", &icon_name);
        g_variant_iter_next (&i, "s", &summary);
        g_variant_iter_next (&i, "s", &body);
        g_variant_iter_next (&i, "^a&s", &actions);
        g_variant_iter_next (&i, "@a{?*}", &hints);
        g_variant_iter_next (&i, "i", &timeout);

        // image-path hint overrides icon name
        value = g_variant_lookup_value (hints, "image-path", G_VARIANT_TYPE_STRING);
        if (value)
        {
            g_free (icon_name);
            icon_name = g_variant_dup_string (value, NULL);
            g_variant_unref (value);
        }

        // icon as raw data?
        value = g_variant_lookup_value (hints, "image-data", G_VARIANT_TYPE ("(iiibiiay)"));
        if (value)
        {
            icon_pb = load_pixbuf_from_data (value);
            g_variant_unref (value);
        }

        message = g_strdup_printf ("%s%s%s", summary, strlen (body) ? "\n" : "", body);
        if (repl_id)
        {
            replace_message (repl_id, message);
            id = repl_id;
        }
        else id = wfpanel_notify_int (message, sender, actions, timeout, icon_name, icon_pb);
        g_free (message);

        g_free (app_name);
        g_free (icon_name);
        g_free (summary);
        g_free (body);

        if (actions)
        {
            count = 0;
            while (1)
            {
                if (actions[count]) g_free (actions[count]);
                else break;
                count++;
            }
            g_free (actions);
        }

        reply = g_variant_new ("(u)", id);
        g_dbus_method_invocation_return_value (invocation, reply);
        g_dbus_connection_flush (connection, NULL, NULL, NULL);
    }

    if (!g_strcmp0 (method_name, "CloseNotification"))
    {
        guint32 id;
        g_variant_get (parameters, "(u)", &id);
        wfpanel_notify_clear (id);
        g_dbus_method_invocation_return_value (invocation, NULL);
        g_dbus_connection_flush (connection, NULL, NULL, NULL);
    }
}
#if 0
static GVariant *handle_get_property (GDBusConnection *, const gchar *sender, const gchar *object_path, const gchar *interface_name,
    const gchar *property_name, GError **error, gpointer user_data)
{
    return NULL;
}

static gboolean handle_set_property (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name,
    const gchar *property_name, GVariant *value, GError **error, gpointer user_data)
{
    return TRUE;
}
#endif
static const GDBusInterfaceVTable interface_vtable =
{
    handle_method_call,
    NULL, //handle_get_property,
    NULL, //handle_set_property,
    {0}
};

static void on_bus_acquired (GDBusConnection *connection, const gchar *, gpointer user_data)
{
    g_dbus_connection_register_object (connection, DBUS_OBJECT_PATH, introspection_data->interfaces[0],
        &interface_vtable, user_data, NULL, NULL);
    dbusconn = connection;
}

static void on_name_acquired (GDBusConnection *, const gchar *, gpointer)
{
}

static void on_name_lost (GDBusConnection *, const gchar *, gpointer)
{
}

static void action_button (GtkWidget *wid, NotifyWindow *nw)
{
    GVariant *body = g_variant_new ("(us)", nw->seq, gtk_widget_get_name (wid));
    g_dbus_connection_emit_signal (dbusconn, nw->sender, DBUS_OBJECT_PATH, DBUS_INTERFACE_NAME, "ActionInvoked", body, NULL);
    hide_message (nw, 2);
}

static void closed_response (NotifyWindow *nw, int reason)
{
    GVariant *body = g_variant_new ("(uu)", nw->seq, reason);
    g_dbus_connection_emit_signal (dbusconn, nw->sender, DBUS_OBJECT_PATH, DBUS_INTERFACE_NAME, "NotificationClosed", body, NULL);
}

/*----------------------------------------------------------------------------*/
/* Private functions */
/*----------------------------------------------------------------------------*/

/* Create a notification window and position appropriately */

static void show_message (NotifyWindow *nw, char *str)
{
    GtkWidget *box, *lbl, *bbox, *btn;
    int dim, offset;
    char *fmt, *cptr;
    GList *item;
    NotifyWindow *nwl;

    /*
     * In order to get a window which looks exactly like a system tooltip, client-side decoration
     * must be requested for it. This cannot be done by any public API call in GTK+3.24, but there is an
     * internal call _gtk_window_request_csd which sets the csd_requested flag in the class' private data.
     * The code below is compatible with a hacked GTK+3 library which uses GTK_WINDOW_POPUP + 1 as the type
     * for a window with CSD requested. It should also not fall over with the standard library...
     */
    nw->popup = gtk_window_new (GTK_WINDOW_POPUP + 1);
    if (!nw->popup) nw->popup = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_window_set_type_hint (GTK_WINDOW (nw->popup), GDK_WINDOW_TYPE_HINT_TOOLTIP);
    gtk_window_set_resizable (GTK_WINDOW (nw->popup), FALSE);

    GtkStyleContext *context = gtk_widget_get_style_context (nw->popup);
    gtk_style_context_add_class (context, GTK_STYLE_CLASS_TOOLTIP);

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add (GTK_CONTAINER (nw->popup), box);

    if (nw->critical)
    {
        GtkWidget *image = gtk_image_new_from_icon_name ("dialog-warning", GTK_ICON_SIZE_DND);
        gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 0);
    }
    else if (nw->icon)
    {
    }
    else if (nw->icon_name)
    {
        GtkWidget *image = gtk_image_new ();
        set_taskbar_icon (image, nw->icon_name);
        gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 0);
    }

    fmt = g_strcompress (str);

    // setting gtk_label_set_max_width_chars looks awful, so we have to do this...
    cptr = fmt;
    dim = 0;
    while (*cptr)
    {
        if (*cptr == ' ' && dim >= TEXT_WIDTH) *cptr = '\n';
        if (*cptr == '\n') dim = 0;
        cptr++;
        dim++;
    }

    lbl = gtk_label_new (fmt);
    gtk_label_set_justify (GTK_LABEL (lbl), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start (GTK_BOX (box), lbl, FALSE, FALSE, 0);
    g_free (fmt);

    if (nw->actions != NULL && nw->actions[0] != NULL)
    {
        int nbtn = 0;
        bbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
        gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
        gtk_box_set_spacing (GTK_BOX (bbox), 5);
        gtk_box_pack_start (GTK_BOX (box), bbox, FALSE, FALSE, 0);
        while (1)
        {
            btn = gtk_button_new_with_label (nw->actions[nbtn * 2 + 1]);
            g_signal_connect (btn, "clicked", G_CALLBACK (action_button), nw);
            gtk_widget_set_name (btn, nw->actions[nbtn * 2]);
            gtk_box_pack_start (GTK_BOX (bbox), btn, FALSE, FALSE, 0);
            nbtn++;
            if (!nw->actions[nbtn * 2]) break;
        }
    }

    // calculate vertical offset for new window - if critical, at top, else immediately below any criticals
    offset = SPACING;
    if (!nw->critical)
    {
        for (item = nwins; item != NULL; item = item->next)
        {
            nwl = (NotifyWindow *) item->data;
            if (!nwl->critical) break;
            if (nwl->shown)
            {
                gtk_window_get_size (GTK_WINDOW (nwl->popup), NULL, &dim);
                offset += dim + SPACING;
            }
        }
    }

    // layer shell setup
    gtk_layer_init_for_window (GTK_WINDOW(nw->popup));
    gtk_layer_set_anchor (GTK_WINDOW(nw->popup), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
    gtk_layer_set_anchor (GTK_WINDOW(nw->popup), GTK_LAYER_SHELL_EDGE_BOTTOM, FALSE);
    gtk_layer_set_anchor (GTK_WINDOW(nw->popup), GTK_LAYER_SHELL_EDGE_LEFT, FALSE);
    gtk_layer_set_anchor (GTK_WINDOW(nw->popup), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
    gtk_layer_set_margin (GTK_WINDOW(nw->popup), GTK_LAYER_SHELL_EDGE_TOP, offset);
    gtk_layer_set_margin (GTK_WINDOW(nw->popup), GTK_LAYER_SHELL_EDGE_RIGHT, SPACING);
    gtk_layer_set_monitor (GTK_WINDOW(nw->popup), gtk_layer_get_monitor (panel));

    g_signal_connect (G_OBJECT (nw->popup), "button-press-event", G_CALLBACK (window_click), nw);
    gtk_widget_show_all (nw->popup);
    if (!nw->critical && nw->timeout > 0) nw->hide_timer = g_timeout_add (nw->timeout, (GSourceFunc) hide_message_timeout, nw);
}

/* Destroy a notification window and remove from list */

static gboolean hide_message_timeout (NotifyWindow *nw)
{
    hide_message (nw, 1);
    return FALSE;
}

static void hide_message (NotifyWindow *nw, int reason)
{
    GList *item;
    int w, h;

    // shuffle notifications below up
    if (nw->popup)
    {
        item = g_list_find (nwins, nw);
        gtk_window_get_size (GTK_WINDOW (nw->popup), &w, &h);
        update_positions (item->next, - (h + SPACING));
        gtk_widget_destroy (nw->popup);
    }

    if (nw->hide_timer) g_source_remove (nw->hide_timer);

    if (nw->sender && reason != -1) closed_response (nw, reason);
    nwins = g_list_remove (nwins, nw);
    g_free (nw->message);
    if (nw->sender) g_free (nw->sender);
    if (nw->actions)
    {
        w = 0;
        while (1)
        {
            if (nw->actions[w]) g_free (nw->actions[w]);
            else break;
            w++;
        }
        g_free (nw->actions);
    }
    g_free (nw);
}

static void replace_message (int id, const char *message)
{
    NotifyWindow *nw;
    GtkWidget *wid;
    GList *children, *item, *wchild;

    // loop through windows in the list, looking for the hash
    for (item = nwins; item != NULL; item = item->next)
    {
        nw = (NotifyWindow *) item->data;
        if (nw->seq == id)
        {
            g_free (nw->message);
            nw->message = g_strdup (message);

            wid = gtk_bin_get_child (GTK_BIN (nw->popup));
            children = gtk_container_get_children (GTK_CONTAINER (wid));
            wchild = children;
            while (wchild)
            {
                if (GTK_IS_LABEL (wchild->data))
                {
                    gtk_label_set_text (GTK_LABEL (wchild->data), message);
                    break;
                }
                wchild = wchild->next;
            }
            g_list_free (children);
        }
    }
}

/* Relocate notifications below the supplied item by the supplied vertical offset */

static void update_positions (GList *item, int offset)
{
    NotifyWindow *nw;

    for (; item != NULL; item = item->next)
    {
        nw = (NotifyWindow *) item->data;
        gtk_layer_set_margin (GTK_WINDOW(nw->popup), GTK_LAYER_SHELL_EDGE_TOP,
            gtk_layer_get_margin (GTK_WINDOW(nw->popup), GTK_LAYER_SHELL_EDGE_TOP) + offset);
    }
}

/* Handler for mouse click in notification window - closes window */

static gboolean window_click (GtkWidget *, GdkEventButton *, NotifyWindow *nw)
{
    hide_message (nw, 2);
    return FALSE;
}

/* Timer handler to show next window */

static gboolean show_next (gpointer)
{
    NotifyWindow *nw;
    GList *item;
    int w, h;

    if (nwins)
    {
        // loop through notifications in the list, finding the oldest which is unshown
        for (item = g_list_last (nwins); item != NULL; item = item->prev)
        {
            nw = (NotifyWindow *) item->data;

            // is this one shown?
            if (nw->shown) continue;
            nw->shown = TRUE;

            // if not, show the window
            show_message (nw, nw->message);

            // shuffle existing notifications down
            gtk_window_get_size (GTK_WINDOW (nw->popup), &w, &h);
            update_positions (item->next, h + SPACING);

            // if there is a newer notification, re-call the timer else stop
            if (item->prev) interval_timer = g_timeout_add (INTERVAL_MS, (GSourceFunc) show_next, NULL);
            else interval_timer = 0;
            return FALSE;
        }
    }

    interval_timer = 0;
    return FALSE;
}

/*----------------------------------------------------------------------------*/
/* Public API */
/*----------------------------------------------------------------------------*/

void wfpanel_notify_init (gboolean enable, gint timeout, GtkWindow *win)
{
    notifications = enable;
    notify_timeout = timeout;
    panel = win;

    // watch DBus for libnotify events
    introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
    owner_id = g_bus_own_name (G_BUS_TYPE_SESSION, DBUS_BUS_NAME, G_BUS_NAME_OWNER_FLAGS_NONE,
        on_bus_acquired, on_name_acquired, on_name_lost, NULL, NULL);

    // set timer for initial display of notifications
    interval_timer = g_timeout_add (INIT_MUTE, (GSourceFunc) show_next, NULL);
}

static int wfpanel_notify_int (const char *message, const char *sender, gchar **actions, int timeout, const char *icon_name, GdkPixbuf *icon)
{
    NotifyWindow *nw;
    GList *item;
    int tmax;

    // check for notifications being disabled
    if (!notifications) return 0;

    // check to see if this notification is already in the list - just bump it to the top if so...
    guint hash = g_str_hash (message);

    // loop through windows in the list, looking for the hash
    for (item = nwins; item != NULL; item = item->next)
    {
        nw = (NotifyWindow *) item->data;
        if (nw->hash == hash)
        {
            // if hash matches a critical, do nothing with the new notification, otherwise hide the window
            if (nw->critical) return -1;
            else hide_message (nw, -1);
        }
    }

    // create a new notification window and add it to the front of the list, but after any criticals
    for (item = nwins; item != NULL; item = item->next)
    {
        nw = (NotifyWindow *) item->data;
        if (!nw->critical) break;
    }
    nw = g_new (NotifyWindow, 1);
    nwins = g_list_insert_before (nwins, item, nw);

    // set the sequence number for this notification
    nseq++;
    if (nseq == -1) nseq++;     // use -1 for invalid sequence code
    nw->seq = nseq;
    nw->hash = hash;
    nw->popup = NULL;
    nw->message = g_strdup (message);
    nw->shown = FALSE;
    nw->critical = FALSE;
    tmax = notify_timeout * 1000;
    if (timeout > -1 && timeout < tmax) tmax = timeout;
    nw->timeout = tmax;
    nw->sender = sender ? g_strdup (sender) : NULL;
    if (!actions) nw->actions = NULL;
    else
    {
        int count = 0;
        nw->actions = malloc (sizeof (gchar *));
        nw->actions[count] = NULL;
        while (actions[count])
        {
            nw->actions = realloc (nw->actions, ((count + 1) * 2 + 1) * sizeof (gchar *));
            nw->actions[count] = g_strdup (actions[count]);
            count++;
            nw->actions[count] = g_strdup (actions[count]);
            count++;
            nw->actions[count] = NULL;
        }
    }

    nw->icon = NULL;
    nw->icon_name = NULL;
    if (icon) nw->icon = icon;
    else if (icon_name) nw->icon_name = g_strdup (icon_name);

    // if the timer isn't running, show the notification immediately and start the timer
    if (interval_timer == 0)
    {
        show_next (NULL);
        interval_timer = g_timeout_add (INTERVAL_MS, (GSourceFunc) show_next, NULL);
    }

    return nseq;
}

int wfpanel_notify (const char *message)
{
    return wfpanel_notify_int (message, NULL, NULL, -1, NULL, NULL);
}

int wfpanel_critical (const char *message)
{
    NotifyWindow *nw;
    GList *item;

    // check to see if this notification is already in the list - just bump it to the top if so...
    guint hash = g_str_hash (message);

    // loop through windows in the list, looking for the hash
    for (item = nwins; item != NULL; item = item->next)
    {
        // if hash matches, hide the window
        nw = (NotifyWindow *) item->data;
        if (nw->hash == hash) hide_message (nw, -1);
    }

    // create a new notification window and add it to the front of the list
    nw = g_new (NotifyWindow, 1);
    nwins = g_list_prepend (nwins, nw);

    // set the sequence number for this notification
    nseq++;
    if (nseq == -1) nseq++;     // use -1 for invalid sequence code
    nw->seq = nseq;
    nw->hash = hash;
    nw->popup = NULL;
    nw->message = g_strdup (message);
    nw->shown = FALSE;
    nw->critical = TRUE;
    nw->timeout = 0;
    nw->sender = NULL;
    nw->icon = NULL;
    nw->icon_name = NULL;

    // if the timer isn't running, show the notification immediately and start the timer
    if (interval_timer == 0)
    {
        show_next (NULL);
        interval_timer = g_timeout_add (INTERVAL_MS, (GSourceFunc) show_next, NULL);
    }

    return nseq;
}

void wfpanel_notify_clear (int seq)
{
    NotifyWindow *nw;
    GList *item;

    // loop through windows in the list, looking for the sequence number
    for (item = nwins; item != NULL; item = item->next)
    {
        // if sequence number matches, hide the window
        nw = (NotifyWindow *) item->data;
        if (nw->seq == seq)
        {
            hide_message (nw, 3);
            return;
        }
    }
}

void wfpanel_notify_close (void)
{
    g_bus_unown_name (owner_id);
    g_dbus_node_info_unref (introspection_data);
}

/* End of file */
/*----------------------------------------------------------------------------*/
