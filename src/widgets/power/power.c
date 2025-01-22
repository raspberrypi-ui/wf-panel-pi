/*============================================================================
Copyright (c) 2023-2025 Raspberry Pi Holdings Ltd.
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
============================================================================*/

#include <locale.h>
#include <glib/gi18n.h>
#include <libudev.h>

#include "lxutils.h"

#include "power.h"

/*----------------------------------------------------------------------------*/
/* Typedefs and macros                                                        */
/*----------------------------------------------------------------------------*/

#define POWER_PATH "/proc/device-tree/chosen/power/"

/* Reasons to show the icon */
#define ICON_LOW_VOLTAGE    0x01
#define ICON_OVER_CURRENT   0x02
#define ICON_BROWNOUT       0x04

/*----------------------------------------------------------------------------*/
/* Plug-in global data                                                        */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/

static gboolean is_pi (void);
static void update_icon (PowerPlugin *pt);
static void check_psu (void);
static void check_brownout (PowerPlugin *pt);
static gpointer overcurrent_thread (gpointer data);
static gboolean cb_overcurrent (gpointer data);
static gpointer lowvoltage_thread (gpointer data);
static gboolean cb_lowvoltage (gpointer data);

/*----------------------------------------------------------------------------*/
/* Function definitions                                                       */
/*----------------------------------------------------------------------------*/

/* Helpers */

static gboolean is_pi (void)
{
    if (system ("raspi-config nonint is_pi") == 0)
        return TRUE;
    else
        return FALSE;
}

/* Tests */

static void check_psu (void)
{
    if (system ("raspi-config nonint is_cmfive") == 0) return;

    FILE *fp = fopen (POWER_PATH "max_current", "rb");
    int val;

    if (fp)
    {
        unsigned char *cptr = (unsigned char *) &val;
        // you're kidding, right?
        for (int i = 3; i >= 0; i--) cptr[i] = fgetc (fp);
        if (val < 5000) wfpanel_notify (_("This power supply is not capable of supplying 5A\nPower to peripherals will be restricted"));
        fclose (fp);
    }
}

static void check_brownout (PowerPlugin *pt)
{
    FILE *fp = fopen (POWER_PATH "power_reset", "rb");
    int val;

    if (fp)
    {
        unsigned char *cptr = (unsigned char *) &val;
        for (int i = 3; i >= 0; i--) cptr[i] = fgetc (fp);
        if (val & 0x02)
        {
            wfpanel_critical (_("Reset due to low power event\nPlease check your power supply"));
            pt->show_icon |= ICON_BROWNOUT;
            update_icon (pt);
        }
        fclose (fp);
    }
}

static char *get_string (char *cmd)
{
    char *line = NULL, *res = NULL;
    size_t len = 0;
    FILE *fp = popen (cmd, "r");

    if (fp == NULL) return NULL;
    if (getline (&line, &len, fp) > 0)
    {
        res = line;
        while (*res)
        {
            if (g_ascii_isspace (*res)) *res = 0;
            res++;
        }
        res = g_strdup (line);
    }
    pclose (fp);
    g_free (line);
    return res;
}

#define MEM_WARN_THRESHOLD 2048
#define RES_HEIGHT_THRESHOLD 1200

static void check_memres (void)
{
    char *res;
    int mem, width, height, max_h = 0;

    res = get_string ("vcgencmd get_config total_mem | cut -d = -f 2");
    if (!res) return;
    if (sscanf (res, "%d", &mem) != 1) return;
    g_free (res);
    if (mem < 256 || mem > MEM_WARN_THRESHOLD) return;

    res = get_string ("wlr-randr | sed -n '/^HDMI-A-1/,/Position/{/current/p}' | sed 's/ //g' | sed 's/px.*//'");
    if (res)
    {
        if (sscanf (res, "%dx%d", &width, &height) == 2)
            if (height > max_h) max_h = height;
        g_free (res);
    }

    res = get_string ("wlr-randr | sed -n '/^HDMI-A-2/,/Position/{/current/p}' | sed 's/ //g' | sed 's/px.*//'");
    if (res)
    {
        if (sscanf (res, "%dx%d", &width, &height) == 2)
            if (height > max_h) max_h = height;
        g_free (res);
    }

    if (max_h > RES_HEIGHT_THRESHOLD)
        wfpanel_notify (_("High display resolution is using large amounts of memory.\nConsider reducing screen resolution."));
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
            if (pt->udev_mon_lv)
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
                                    gdk_threads_add_idle (cb_overcurrent, data);
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
    }
    return NULL;
}

static gboolean cb_overcurrent (gpointer data)
{
    PowerPlugin *pt = (PowerPlugin *) data;

    wfpanel_critical (_("USB overcurrent\nPlease check your connected USB devices"));
    pt->show_icon |= ICON_OVER_CURRENT;
    update_icon (pt);
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
            if (pt->udev_mon_lv)
            {
                dev = udev_monitor_receive_device (pt->udev_mon_lv);
                if (dev)
                {
                    if (!g_strcmp0 (udev_device_get_action (dev), "change") && !strncmp (udev_device_get_sysname (dev), "hwmon", 5))
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
    }
    return NULL;
}

static gboolean cb_lowvoltage (gpointer data)
{
    PowerPlugin *pt = (PowerPlugin *) data;

    wfpanel_critical (_("Low voltage warning\nPlease check your power supply"));
    pt->show_icon |= ICON_LOW_VOLTAGE;
    update_icon (pt);
    return FALSE;
}

/* Update the icon to show current status */

static void update_icon (PowerPlugin *pt)
{
    char *tooltip;

    set_taskbar_icon (pt->tray_icon, "under-volt", pt->icon_size);
    gtk_widget_set_sensitive (pt->plugin, pt->show_icon);

    if (!pt->show_icon) gtk_widget_hide (pt->plugin);
    else
    {
        gtk_widget_show (pt->plugin);
        tooltip = g_strconcat (pt->show_icon & ICON_LOW_VOLTAGE ? _("PSU low voltage detected\n") : "",
            pt->show_icon & ICON_OVER_CURRENT ? _("USB over current detected\n") : "",
            pt->show_icon & ICON_BROWNOUT ? _("Low power reset has occurred\n") : "", NULL);
        tooltip[strlen (tooltip) - 1] = 0;
        gtk_widget_set_tooltip_text (pt->tray_icon, tooltip);
        g_free (tooltip);
    }
}

static void show_info (GtkWidget *, gpointer)
{
    system ("x-www-browser https://rptl.io/rpi5-power-supply-info &");
}

/*----------------------------------------------------------------------------*/
/* wf-panel plugin functions                                                  */
/*----------------------------------------------------------------------------*/

/* Handler for button click */
static void power_button_clicked (GtkWidget *, PowerPlugin *pt)
{
    CHECK_LONGPRESS
    gtk_widget_show_all (pt->menu);
    show_menu_with_kbd (pt->plugin, pt->menu);
}

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

    /* Allocate icon as a child of top level */
    pt->tray_icon = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (pt->plugin), pt->tray_icon);

    /* Set up button */
    gtk_button_set_relief (GTK_BUTTON (pt->plugin), GTK_RELIEF_NONE);
    g_signal_connect (pt->plugin, "clicked", G_CALLBACK (power_button_clicked), pt);

    /* Set up long press */
    pt->gesture = add_long_press (pt->plugin, NULL, NULL);

    pt->show_icon = 0;
    pt->oc_thread = NULL;
    pt->lv_thread = NULL;
    pt->udev_mon_oc = NULL;
    pt->udev_mon_lv = NULL;
    pt->udev = NULL;

    pt->menu = gtk_menu_new ();
    GtkWidget *item = gtk_menu_item_new_with_label (_("Power Information..."));
    g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (show_info), NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (pt->menu), item);

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
        pt->oc_thread = g_thread_new (NULL, overcurrent_thread, pt);
        pt->lv_thread = g_thread_new (NULL, lowvoltage_thread, pt);

        check_psu ();
        check_brownout (pt);
        check_memres ();
    }

    update_icon (pt);

    /* Show the widget and return */
    gtk_widget_show_all (pt->plugin);
}

void power_destructor (gpointer user_data)
{
    PowerPlugin *pt = (PowerPlugin *) user_data;

    if (pt->oc_thread) g_thread_unref (pt->oc_thread);
    if (pt->lv_thread) g_thread_unref (pt->lv_thread);
    if (pt->udev_mon_oc) udev_monitor_unref (pt->udev_mon_oc);
    pt->udev_mon_oc = NULL;
    if (pt->udev_mon_lv) udev_monitor_unref (pt->udev_mon_lv);
    pt->udev_mon_lv = NULL;
    if (pt->udev) udev_unref (pt->udev);
    if (pt->gesture) g_object_unref (pt->gesture);
    g_free (pt);
}

/* End of file */
/*----------------------------------------------------------------------------*/
