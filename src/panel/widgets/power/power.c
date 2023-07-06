/*
Copyright (c) 2018 Raspberry Pi (Trading) Ltd.
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
#include <stdint.h>
#include <string.h>
#include <glib/gi18n.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <asm/ioctl.h>
#include "power.h"

//#include "plugin.h"

#define VMON_INTERVAL 15000
#define VMON_PATH "/sys/devices/platform/soc/soc:firmware/raspberrypi-hwmon/hwmon/hwmon1/in0_lcrit_alarm"

/* Plug-in global data */

/* Prototypes */

static gboolean is_pi (void);
static void update_icon (PowerPlugin *pt);
static gboolean vtimer_event (PowerPlugin *pt);


static gboolean is_pi (void)
{
    if (system ("raspi-config nonint is_pi") == 0)
        return TRUE;
    else
        return FALSE;
}

/* Read the current charge state and update the icon accordingly */

static void update_icon (PowerPlugin *pt)
{
    set_taskbar_icon (pt->tray_icon, "under-volt", pt->icon_size);
    gtk_widget_set_sensitive (pt->plugin, FALSE);
    if (1) gtk_widget_hide (pt->plugin);
    else
    {
        gtk_widget_show (pt->plugin);
        gtk_widget_set_tooltip_text (pt->tray_icon, "You should have bought a proper power supply...");
    }
}

static gboolean vtimer_event (PowerPlugin *pt)
{
    FILE *fp = fopen (VMON_PATH, "rb");
    if (fp)
    {
        int val = fgetc (fp);
        fclose (fp);
        if (val == '1')
            lxpanel_notify (_("Low voltage warning\nPlease check your power supply"));
    }
    return TRUE;
}

/* Plugin functions */

/* Handler for system config changed message from panel */
void power_update_display (PowerPlugin *pt)
{
    update_icon (pt);
}


void power_destructor (gpointer user_data)
{
    PowerPlugin *pt = (PowerPlugin *) user_data;

    /* Disconnect the timer. */
    if (pt->vtimer) g_source_remove (pt->vtimer);

    /* Deallocate memory */
    //g_free (pt);
}

/* Plugin constructor. */
void power_init (PowerPlugin *pt)
{
    /* Allocate and initialize plugin context */
    //PtBattPlugin *pt = g_new0 (PtBattPlugin, 1);

    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    /* Allocate top level widget and set into plugin widget pointer. */
    //pt->panel = panel;
    //pt->settings = settings;
    //pt->plugin = gtk_event_box_new ();
    //lxpanel_plugin_set_data (pt->plugin, pt, ptbatt_destructor);

    /* Allocate icon as a child of top level */
    pt->tray_icon = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (pt->plugin), pt->tray_icon);

    pt->ispi = is_pi ();

    /* Start timed events to monitor low voltage warnings */
    if (pt->ispi) pt->vtimer = g_timeout_add (VMON_INTERVAL, (GSourceFunc) vtimer_event, (gpointer) pt);

    update_icon (pt);

    /* Show the widget and return */
    gtk_widget_show_all (pt->plugin);
    //return pt->plugin;
}

#if 0

FM_DEFINE_MODULE(lxpanel_gtk, ptbatt)

/* Plugin descriptor. */
LXPanelPluginInit fm_module_init_lxpanel_gtk = {
    .name = N_("Power & Battery"),
    .description = N_("Monitors voltage and laptop battery"),
    .new_instance = ptbatt_constructor,
    .reconfigure = ptbatt_configuration_changed,
    .gettext_package = GETTEXT_PACKAGE
};
#endif
