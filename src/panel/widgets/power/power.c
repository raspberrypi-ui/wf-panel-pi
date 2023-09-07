/*
Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
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
#include <libudev.h>
#include <sys/select.h>
#include "power.h"

#define VMON_INTERVAL 1000
#define VMON_PATH "/sys/devices/platform/soc/soc:firmware/raspberrypi-hwmon/hwmon/hwmon%d/in0_lcrit_alarm"
#define PSU_PATH "/proc/device-tree/chosen/power/max_current"

/* Reasons to show the icon */
#define ICON_LOW_VOLTAGE 0x01
#define ICON_LOW_CURRENT 0x02

/* Plug-in global data */

/* Prototypes */

static gboolean is_pi (void);
static void update_icon (PowerPlugin *pt);
static gboolean vtimer_event (PowerPlugin *pt);
static void check_psu (PowerPlugin *pt);
static void check_low_voltage (PowerPlugin *pt);
static void check_over_current (PowerPlugin *pt);


static gboolean is_pi (void)
{
    if (system ("raspi-config nonint is_pi") == 0)
        return TRUE;
    else
        return FALSE;
}


/* Tests */

static void check_low_voltage (PowerPlugin *pt)
{
    FILE *fp;
    char *path;
    int i;

    for (i = 0; i < 3; i++)
    {
        path = g_strdup_printf (VMON_PATH, i);
        fp = fopen (path, "rb");
        g_free (path);
        if (fp) break;
    }

    if (fp)
    {
        int val = fgetc (fp);
        fclose (fp);
        if (val == '1')
        {
            lxpanel_critical (_("Low voltage warning\nPlease check your power supply"));
            pt->show_icon |= ICON_LOW_VOLTAGE;
            update_icon (pt);
        }
    }
}

static void check_over_current (PowerPlugin *pt)
{
    fd_set fds;
    int val;
    struct udev_device *dev;
    struct timeval tv;
    tv.tv_usec = 0;
    tv.tv_sec = 0;

    FD_ZERO (&fds);
    FD_SET (pt->fd, &fds);
    while (select (pt->fd + 1, &fds, NULL, NULL, &tv) > 0 && FD_ISSET (pt->fd, &fds))
    {
        dev = udev_monitor_receive_device (pt->udev_mon);
        if (dev)
        {
            if (!strcmp (udev_device_get_action (dev), "change"))
            {
                if (sscanf (udev_device_get_property_value (dev, "OVER_CURRENT_COUNT"), "%d", &val) == 1 && val != pt->last_oc)
                {
                    lxpanel_critical (_("USB overcurrent\nPlease check your connected USB devices"));
                    pt->last_oc = val;
                }
            }
            udev_device_unref (dev);
        }
    }
}

static void check_psu (PowerPlugin *pt)
{
    FILE *fp = fopen (PSU_PATH, "rb");
    int val;

    if (fp)
    {
        unsigned char *cptr = (unsigned char *) &val;
        // you're kidding, right?
        for (int i = 3; i >= 0; i--) cptr[i] = fgetc (fp);
        if (val < 5000)
        {
            lxpanel_notify (_("This power supply is not capable of supplying 5A\nPower to peripherals is restricted"));
            pt->show_icon |= ICON_LOW_CURRENT;
            update_icon (pt);
        }
        fclose (fp);
    }
}

/* Update the icon to show current status */

static void update_icon (PowerPlugin *pt)
{
    set_taskbar_icon (pt->tray_icon, "under-volt", pt->icon_size);
    if (!pt->show_icon)
    {
        gtk_widget_set_sensitive (pt->plugin, FALSE);
        gtk_widget_hide (pt->plugin);
    }
    else
    {
        gtk_widget_set_sensitive (pt->plugin, TRUE);
        gtk_widget_show (pt->plugin);
        if (pt->show_icon == ICON_LOW_VOLTAGE) gtk_widget_set_tooltip_text (pt->tray_icon, _("Low voltage has been detected"));
        else if (pt->show_icon == ICON_LOW_CURRENT) gtk_widget_set_tooltip_text (pt->tray_icon, _("Power supply not capable of supplying 5A"));
        else gtk_widget_set_tooltip_text (pt->tray_icon, _("Low voltage has been detected\n\nPower supply not capable of supplying 5A"));
    }
}

/* Timer handler for periodic tests */

static gboolean vtimer_event (PowerPlugin *pt)
{
    check_low_voltage (pt);
    check_over_current (pt);

    return TRUE;
}

/* Plugin functions */

/* Handler for system config changed message from panel */
void power_update_display (PowerPlugin *pt)
{
    update_icon (pt);
}

void power_init (PowerPlugin *pt)
{
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    /* Allocate icon as a child of top level */
    pt->tray_icon = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (pt->plugin), pt->tray_icon);

    pt->show_icon = 0;
    pt->vtimer = 0;

    /* Start timed events to monitor low voltage warnings */
    if (is_pi ())
    {
        pt->last_oc = -1;
        pt->udev = udev_new ();
        pt->udev_mon = udev_monitor_new_from_netlink (pt->udev, "kernel");
        pt->fd = udev_monitor_get_fd (pt->udev_mon);
        udev_monitor_filter_add_match_subsystem_devtype (pt->udev_mon, "usb", NULL);
        udev_monitor_enable_receiving (pt->udev_mon);

        check_psu (pt);
        pt->vtimer = g_timeout_add (VMON_INTERVAL, (GSourceFunc) vtimer_event, (gpointer) pt);
    }

    update_icon (pt);

    /* Show the widget and return */
    gtk_widget_show_all (pt->plugin);
}

void power_destructor (gpointer user_data)
{
    PowerPlugin *pt = (PowerPlugin *) user_data;

    /* Disconnect the timer. */
    if (pt->vtimer) g_source_remove (pt->vtimer);

    if (pt->udev_mon) udev_monitor_unref (pt->udev_mon);
    udev_unref (pt->udev);
}

