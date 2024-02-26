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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>

#include "clock.h"

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/

static void calendar_show (ClockPlugin *cl);
static gboolean clock_button_press_event (GtkWidget *widget, GdkEventButton *event, ClockPlugin *cl);

/*----------------------------------------------------------------------------*/
/* Plugin functions                                                           */
/*----------------------------------------------------------------------------*/

static void calendar_show (ClockPlugin *cl)
{
    /* Create a new window. */
    cl->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name (cl->window, "panelpopup");

    gtk_container_set_border_width (GTK_CONTAINER (cl->window), 0);

    /* Create a vertical box as the child of the window. */
    cl->calendar = gtk_calendar_new ();
    gtk_container_add (GTK_CONTAINER (cl->window), cl->calendar);

    /* Realise the window */
    popup_window_at_button (&cl->window, &cl->plugin, cl->bottom);
}

/* Handler for menu button click */
static gboolean clock_button_press_event (GtkWidget *, GdkEventButton *event, ClockPlugin *cl)
{
    if (event->button == 1)
    {
        if (cl->window) close_popup (&cl->window);
        else calendar_show (cl);
        return TRUE;
    }

    if (event->button == 3)
    {
        if (cl->window)
        {
            close_popup (&cl->window);
            return TRUE;
        }
    }
    return FALSE;
}

/* Plugin constructor */
void clock_init (ClockPlugin *cl)
{
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    /* Set up button */
    gtk_button_set_relief (GTK_BUTTON (cl->plugin), GTK_RELIEF_NONE);
    g_signal_connect (cl->plugin, "button-release-event", G_CALLBACK (clock_button_press_event), cl);

    /* Set up variables */
    cl->window = NULL;
    cl->calendar = NULL;

    /* Show the widget and return. */
    gtk_widget_show_all (cl->plugin);
}

/* Plugin destructor. */
void clock_destructor (gpointer)
{
}


/* End of file */
/*----------------------------------------------------------------------------*/
