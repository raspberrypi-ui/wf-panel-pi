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

#define PSU_PATH "/proc/device-tree/chosen/power/max_current"

/* Reasons to show the icon */
#define ICON_LOW_VOLTAGE 0x01
#define ICON_LOW_CURRENT 0x02

/* Plug-in global data */

/* Prototypes */

static gboolean is_pi (void);
static void update_icon (PowerPlugin *pt);
static void check_psu (PowerPlugin *pt);
static gpointer overcurrent_thread (gpointer data);
static gboolean cb_overcurrent (gpointer data);
static gpointer lowvoltage_thread (gpointer data);
static gboolean cb_lowvoltage (gpointer data);

/* Helpers */

static gboolean is_pi (void)
{
    if (system ("raspi-config nonint is_pi") == 0)
        return TRUE;
    else
        return FALSE;
}

/* Tests */

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

/* Monitoring threads and callbacks */

static gpointer overcurrent_thread (gpointer data)
{
    PowerPlugin *pt = (PowerPlugin *) data;
    fd_set fds;
    int val;
    struct udev_device *dev;
    FILE *fp;
    char *path;

    FD_ZERO (&fds);
    FD_SET (pt->fd_oc, &fds);
    while (select (pt->fd_oc + 1, &fds, NULL, NULL, NULL) > 0)
    {
        if (FD_ISSET (pt->fd_oc, &fds))
        {
            dev = udev_monitor_receive_device (pt->udev_mon_oc);
            if (dev)
            {
                if (!g_strcmp0 (udev_device_get_action (dev), "change"))
                {
                    path = g_strdup_printf ("/sys/%s/disable", udev_device_get_property_value (dev, "OVER_CURRENT_PORT"));
                    fp = fopen (path, "rb");
                    if (fp)
                    {
                        if (fgetc (fp) == 0x31)
                        {
                            if (sscanf (udev_device_get_property_value (dev, "OVER_CURRENT_COUNT"), "%d", &val) == 1 && val != pt->last_oc)
                            {
                                gdk_threads_add_idle (cb_overcurrent, NULL);
                                pt->last_oc = val;
                            }
                        }
                        fclose (fp);
                    }
                    g_free (path);
                }
                udev_device_unref (dev);
            }
        }
    }
    return NULL;
}

static gboolean cb_overcurrent (gpointer)
{
    lxpanel_critical (_("USB overcurrent\nPlease check your connected USB devices"));
    return FALSE;
}

static gpointer lowvoltage_thread (gpointer data)
{
    PowerPlugin *pt = (PowerPlugin *) data;
    fd_set fds;
    struct udev_device *dev;
    FILE *fp;
    char *path;

    FD_ZERO (&fds);
    FD_SET (pt->fd_lv, &fds);
    while (select (pt->fd_lv + 1, &fds, NULL, NULL, NULL) > 0)
    {
        if (FD_ISSET (pt->fd_lv, &fds))
        {
            dev = udev_monitor_receive_device (pt->udev_mon_lv);
            if (dev)
            {
                if (!g_strcmp0 (udev_device_get_action (dev), "change") && !g_strcmp0 (udev_device_get_sysname (dev), "hwmon1"))
                {
                    path = g_strdup_printf ("%s/in0_lcrit_alarm", udev_device_get_syspath (dev));
                    fp = fopen (path, "rb");
                    if (fp)
                    {
                        if (fgetc (fp) == 0x31)
                        {
                            gdk_threads_add_idle (cb_lowvoltage, data);
                        }
                        fclose (fp);
                    }
                    g_free (path);
                }
                udev_device_unref (dev);
            }
        }
    }
    return NULL;
}

static gboolean cb_lowvoltage (gpointer data)
{
    PowerPlugin *pt = (PowerPlugin *) data;

    lxpanel_critical (_("Low voltage warning\nPlease check your power supply"));
    pt->show_icon |= ICON_LOW_VOLTAGE;
    update_icon (pt);
    return FALSE;
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

    /* Start timed events to monitor low voltage warnings */
    if (is_pi ())
    {
        pt->last_oc = -1;
        pt->udev = udev_new ();

        /* Configure udev monitors */
        pt->udev_mon_oc = udev_monitor_new_from_netlink (pt->udev, "kernel");
        udev_monitor_filter_add_match_subsystem_devtype (pt->udev_mon_oc, "usb", NULL);
        udev_monitor_enable_receiving (pt->udev_mon_oc);
        pt->fd_oc = udev_monitor_get_fd (pt->udev_mon_oc);

        pt->udev_mon_lv = udev_monitor_new_from_netlink (pt->udev, "kernel");
        udev_monitor_filter_add_match_subsystem_devtype (pt->udev_mon_lv, "hwmon", NULL);
        udev_monitor_enable_receiving (pt->udev_mon_lv);
        pt->fd_lv = udev_monitor_get_fd (pt->udev_mon_lv);

        /* Start threads to monitor udev */
        g_thread_new (NULL, overcurrent_thread, pt);
        g_thread_new (NULL, lowvoltage_thread, pt);

        check_psu (pt);
    }

    update_icon (pt);

    /* Show the widget and return */
    gtk_widget_show_all (pt->plugin);
}

void power_destructor (gpointer user_data)
{
    PowerPlugin *pt = (PowerPlugin *) user_data;

    // kill the threads?

    if (pt->udev_mon_oc) udev_monitor_unref (pt->udev_mon_oc);
    if (pt->udev_mon_lv) udev_monitor_unref (pt->udev_mon_lv);
    udev_unref (pt->udev);
}

