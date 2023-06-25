/*
Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
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
} NotifyWindow;


/*----------------------------------------------------------------------------*/
/* Global data */
/*----------------------------------------------------------------------------*/

static gboolean notifications;
static gint notify_timeout;
static GtkWindow *panel;

static GList *nwins = NULL;         /* List of current notifications */
static int nseq = 0;                /* Sequence number for notifications */
static gint interval_timer = 0;     /* Used to show windows one at a time */

/*----------------------------------------------------------------------------*/
/* Function prototypes */
/*----------------------------------------------------------------------------*/

static void show_message (NotifyWindow *nw, char *str);
static gboolean hide_message (NotifyWindow *nw);
static void update_positions (GList *item, int offset);
static gboolean window_click (GtkWidget *widget, GdkEventButton *event, NotifyWindow *nw);

/*----------------------------------------------------------------------------*/
/* Private functions */
/*----------------------------------------------------------------------------*/

/* Create a notification window and position appropriately */

static void show_message (NotifyWindow *nw, char *str)
{
    GtkWidget *box, *lbl;
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
        GtkWidget *image = gtk_image_new_from_icon_name ("dialog-warning", 32);
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
    if (!nw->critical && notify_timeout > 0) nw->hide_timer = g_timeout_add (notify_timeout * 1000, (GSourceFunc) hide_message, nw);
}

/* Destroy a notification window and remove from list */

static gboolean hide_message (NotifyWindow *nw)
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

    nwins = g_list_remove (nwins, nw);
    g_free (nw->message);
    g_free (nw);
    return FALSE;
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
    hide_message (nw);
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

void lxpanel_notify_init (gboolean enable, gint timeout, GtkWindow *win)
{
    notifications = enable;
    notify_timeout = timeout;
    panel = win;

    // set timer for initial display of notifications
    interval_timer = g_timeout_add (INIT_MUTE, (GSourceFunc) show_next, NULL);
}

int lxpanel_notify (const char *message)
{
    NotifyWindow *nw;
    GList *item;

    // check for notifications being disabled
    if (!notifications) return 0;

    // check to see if this notification is already in the list - just bump it to the top if so...
    guint hash = g_str_hash (message);

    // loop through windows in the list, looking for the hash
    for (item = nwins; item != NULL; item = item->next)
    {
        // if hash matches, hide the window
        nw = (NotifyWindow *) item->data;
        if (nw->hash == hash) hide_message (nw);
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

    // if the timer isn't running, show the notification immediately and start the timer
    if (interval_timer == 0)
    {
        show_next (NULL);
        interval_timer = g_timeout_add (INTERVAL_MS, (GSourceFunc) show_next, NULL);
    }

    return nseq;
}

int lxpanel_critical (const char *message)
{
    NotifyWindow *nw;
    GList *item;

    // check to see if this notification is already in the list - just bump it to the top if so...
    guint hash = g_str_hash (message) + 1;

    // loop through windows in the list, looking for the hash
    for (item = nwins; item != NULL; item = item->next)
    {
        // if hash matches, hide the window
        nw = (NotifyWindow *) item->data;
        if (nw->hash == hash) hide_message (nw);
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

    // if the timer isn't running, show the notification immediately and start the timer
    if (interval_timer == 0)
    {
        show_next (NULL);
        interval_timer = g_timeout_add (INTERVAL_MS, (GSourceFunc) show_next, NULL);
    }

    return nseq;
}

void lxpanel_notify_clear (int seq)
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
            hide_message (nw);
            return;
        }
    }
}


/* End of file */
/*----------------------------------------------------------------------------*/
