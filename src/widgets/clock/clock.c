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

#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n.h>

#include "clock.h"

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/

static void cal_destroyed (GtkWidget *widget, gpointer data);
static void calendar_show (ClockPlugin *cl);
static gboolean clock_button_press_event (GtkWidget *, GdkEventButton *, ClockPlugin *vol);
static gboolean clock_button_release_event (GtkWidget *, GdkEventButton *event, ClockPlugin *vol);

/*----------------------------------------------------------------------------*/
/* Plugin functions                                                           */
/*----------------------------------------------------------------------------*/

static void cal_destroyed (GtkWidget *, gpointer data)
{
    ClockPlugin *cl = (ClockPlugin *) data;
    cl->window = NULL;
}

static void calendar_show (ClockPlugin *cl)
{
    /* Create a new window. */
    cl->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name (cl->window, "panelpopup");

    gtk_container_set_border_width (GTK_CONTAINER (cl->window), 1);

    /* Add a calendar to the window. */
    gtk_container_add (GTK_CONTAINER (cl->window), gtk_calendar_new ());
    g_signal_connect (cl->window, "destroy", G_CALLBACK (cal_destroyed), cl);

    /* Realise the window */
    popup_window_at_button (cl->window, cl->plugin);
}

/* Handler for button click */
static gboolean clock_button_press_event (GtkWidget *, GdkEventButton *, ClockPlugin *cl)
{
    pressed = PRESS_NONE;
    if (cl->window) cl->popup_shown = TRUE;
    else cl->popup_shown = FALSE;
    return FALSE;
}

static gboolean clock_button_release_event (GtkWidget *, GdkEventButton *event, ClockPlugin *cl)
{
    if (pressed == PRESS_LONG) return FALSE;

    switch (event->button)
    {
        case 1: if (!cl->popup_shown) calendar_show (cl);
                return FALSE;

        case 3: return FALSE;
    }

    return TRUE;
}

/* Plugin constructor */
void clock_init (ClockPlugin *cl)
{
    /* Set up button */
    gtk_button_set_relief (GTK_BUTTON (cl->plugin), GTK_RELIEF_NONE);
    g_signal_connect (cl->plugin, "button-press-event", G_CALLBACK (clock_button_press_event), cl);
    g_signal_connect (cl->plugin, "button-release-event", G_CALLBACK (clock_button_release_event), cl);

    /* Set up long press */
    cl->gesture = add_long_press (cl->plugin, NULL, NULL);

    /* Set up variables */
    cl->window = NULL;

    /* Show the widget and return. */
    gtk_widget_show_all (cl->plugin);
}

/* Plugin destructor. */
void clock_destructor (gpointer data)
{
    ClockPlugin *cl = (ClockPlugin *) data;
    close_popup ();
    if (cl->gesture) g_object_unref (cl->gesture);
    g_free (cl);
}


/* End of file */
/*----------------------------------------------------------------------------*/
