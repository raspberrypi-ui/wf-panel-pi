/*
 * CPU temperature plugin for LXPanel
 *
 * Based on 'cpu' and 'thermal' plugin code from LXPanel
 * Copyright for relevant code as for LXPanel
 *
 */

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
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <stdlib.h>
#include <glib/gi18n.h>

#include "cputemp.h"

#define PROC_THERMAL_DIRECTORY      "/proc/acpi/thermal_zone/"
#define PROC_THERMAL_TEMPF          "temperature"
#define PROC_THERMAL_TRIP           "trip_points"

#define SYSFS_THERMAL_DIRECTORY     "/sys/class/thermal/"
#define SYSFS_THERMAL_SUBDIR_PREFIX "thermal_zone"
#define SYSFS_THERMAL_TEMPF         "temp"


static gboolean is_pi (void)
{
    if (system ("raspi-config nonint is_pi") == 0)
        return TRUE;
    else
        return FALSE;
}

static gint proc_get_temperature (char const *sensor_path)
{
    FILE *state;
    char buf[256], sstmp[100];
    char *pstr;

    if (sensor_path == NULL) return -1;

    snprintf (sstmp, sizeof (sstmp), "%s%s", sensor_path, PROC_THERMAL_TEMPF);

    if (!(state = fopen( sstmp, "r")))
    {
        g_warning ("cputemp: cannot open %s", sstmp);
        return -1;
    }

    while (fgets (buf, 256, state) && !(pstr = strstr (buf, "temperature:")));
    if (pstr)
    {
        pstr += 12;
        while (*pstr && *pstr == ' ') ++pstr;

        pstr[strlen (pstr) - 3] = '\0';
        fclose (state);
        return atoi (pstr);
    }

    fclose (state);
    return -1;
}

static gint _get_reading (const char *path)
{
    FILE *state;
    char buf[256];
    char *pstr;

    if (!(state = fopen (path, "r")))
    {
        g_warning ("cputemp: cannot open %s", path);
        return -1;
    }

    while (fgets (buf, 256, state) && !(pstr = buf));
    if (pstr)
    {
        fclose (state);
        return atoi (pstr) / 1000;
    }

    fclose (state);
    return -1;
}

static gint sysfs_get_temperature (char const *sensor_path)
{
    char sstmp [100];

    if (sensor_path == NULL) return -1;

    snprintf (sstmp, sizeof (sstmp), "%s%s", sensor_path, SYSFS_THERMAL_TEMPF);

    return _get_reading (sstmp);
}

static gint hwmon_get_temperature (char const *sensor_path)
{
    if (sensor_path == NULL) return -1;
    return _get_reading (sensor_path);
}

static int add_sensor (CPUTempPlugin* c, char const* sensor_path, GetTempFunc get_temp)
{
    if (c->numsensors + 1 > MAX_NUM_SENSORS)
    {
        g_message ("cputemp: Too many sensors (max %d), ignoring '%s'",
            MAX_NUM_SENSORS, sensor_path);
        return -1;
    }

    c->sensor_array[c->numsensors] = g_strdup (sensor_path);
    c->get_temperature[c->numsensors] = get_temp;
    c->numsensors++;

    g_message ("cputemp: Added sensor %s", sensor_path);

    return 0;
}

static gboolean try_hwmon_sensors (CPUTempPlugin* c, const char *path)
{
    GDir *sensorsDirectory;
    const char *sensor_name;
    char sensor_path[100], buf[256];
    FILE *fp;
    gboolean found = FALSE;

    if (!(sensorsDirectory = g_dir_open (path, 0, NULL))) return found;

    while ((sensor_name = g_dir_read_name (sensorsDirectory)))
    {
        if (strncmp (sensor_name, "temp", 4) == 0 &&
            strcmp (&sensor_name[5], "_input") == 0)
        {
            snprintf (sensor_path, sizeof (sensor_path), "%s/temp%c_label", path, sensor_name[4]);
            fp = fopen (sensor_path, "r");
            buf[0] = '\0';
            if (fp)
            {
                if (fgets (buf, 256, fp))
                {
                    char *pp = strchr (buf, '\n');
                    if (pp) *pp = '\0';
                }
                fclose (fp);
            }
            snprintf (sensor_path, sizeof (sensor_path), "%s/%s", path, sensor_name);
            add_sensor (c, sensor_path, hwmon_get_temperature);
            found = TRUE;
        }
    }
    g_dir_close (sensorsDirectory);
    return found;
}

static void find_hwmon_sensors (CPUTempPlugin* c)
{
    char dir_path[100];
    char *cptr;
    int i; /* sensor type num, we'll try up to 4 */

    for (i = 0; i < 4; i++)
    {
        snprintf (dir_path, sizeof (dir_path), "/sys/class/hwmon/hwmon%d/device", i);
        if (try_hwmon_sensors (c, dir_path)) continue;
        /* no sensors found under device/, try parent dir */
        cptr = strrchr (dir_path, '/');
        *cptr = '\0';
        try_hwmon_sensors (c, dir_path);
    }
}

static void find_sensors (CPUTempPlugin* c, char const* directory, char const* subdir_prefix, GetTempFunc get_temp)
{
    GDir *sensorsDirectory;
    const char *sensor_name;
    char sensor_path[100];

    if (!(sensorsDirectory = g_dir_open (directory, 0, NULL))) return;

    /* Scan the thermal_zone directory for available sensors */
    while ((sensor_name = g_dir_read_name (sensorsDirectory)))
    {
        if (sensor_name[0] == '.') continue;
        if (subdir_prefix)
        {
            if (strncmp (sensor_name, subdir_prefix, strlen (subdir_prefix)) != 0)  continue;
        }
        snprintf (sensor_path, sizeof (sensor_path), "%s%s/", directory, sensor_name);
        add_sensor (c, sensor_path, get_temp);
    }
    g_dir_close (sensorsDirectory);
}

static void check_sensors (CPUTempPlugin *c)
{
    int i;

    for (i = 0; i < c->numsensors; i++) g_free (c->sensor_array[i]);
    c->numsensors = 0;

    find_sensors (c, PROC_THERMAL_DIRECTORY, NULL, proc_get_temperature);
    find_sensors (c, SYSFS_THERMAL_DIRECTORY, SYSFS_THERMAL_SUBDIR_PREFIX, sysfs_get_temperature);
    if (c->numsensors == 0) find_hwmon_sensors (c);
    
    g_message ("cputemp: Found %d sensors", c->numsensors);
}

static gint get_temperature (CPUTempPlugin *c)
{
    gint max = -273, cur, i;

    for (i = 0; i < c->numsensors; i++)
    {
        cur = c->get_temperature[i] (c->sensor_array[i]);
        if (cur > max) max = cur;
        c->temperature[i] = cur;
    }

    return max;
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

static int get_throttle (void)
{
    char *buf;
    unsigned int val;

    buf = get_string ("vcgencmd get_throttled");
    if (!buf) return 0;
    if (sscanf (buf, "throttled=0x%x", &val) != 1) val = 0;
    g_free (buf);
    return val;
}


/* Periodic timer callback */

static gboolean cpu_update (CPUTempPlugin *c)
{
    char buffer[10];
    int temp, thr;
    float ftemp;

    if (g_source_is_destroyed (g_main_current_source ())) return FALSE;

    temp = get_temperature (c);

    sprintf (buffer, "%3d°", temp);

    ftemp = temp;
    ftemp -= c->lower_temp;
    ftemp /= (c->upper_temp - c->lower_temp);

    thr = 0;
    if (c->ispi)
    {
        temp = get_throttle ();
        if (temp & 0x04) thr = 2;
        else if (temp & 0x02) thr = 1;
    }

    graph_new_point (&(c->graph), ftemp, thr, buffer);

    return TRUE;
}


void cputemp_update_display (CPUTempPlugin *c)
{
    graph_init (&(c->graph), c->icon_size, c->background_color, c->foreground_color,
        c->low_throttle_color, c->high_throttle_color);
}

void cputemp_destructor (gpointer user_data)
{
}

/* Plugin constructor. */
void cputemp_init (CPUTempPlugin *c)
{
    const char *str;
    int val;

    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    /* Allocate icon as a child of top level */
    c->graph.da = gtk_image_new ();
    gtk_container_add (GTK_CONTAINER (c->plugin), c->graph.da);

    /* Set up variables */
    c->ispi = is_pi ();

    if (config_setting_lookup_string ("cputemp", "Foreground", &str))
    {
        if (!gdk_rgba_parse (&c->foreground_color, str))
            gdk_rgba_parse (&c->foreground_color, "dark gray");
    } else gdk_rgba_parse (&c->foreground_color, "dark gray");

    if (config_setting_lookup_string ("cputemp", "Background", &str))
    {
        if (!gdk_rgba_parse (&c->background_color, str))
            gdk_rgba_parse (&c->background_color, "light gray");
    } else gdk_rgba_parse (&c->background_color, "light gray");

    if (config_setting_lookup_string ("cputemp", "Throttle1", &str))
    {
        if (!gdk_rgba_parse (&c->low_throttle_color, str))
            gdk_rgba_parse (&c->low_throttle_color, "orange");
    } else gdk_rgba_parse (&c->low_throttle_color, "orange");

    if (config_setting_lookup_string ("cputemp", "Throttle2", &str))
    {
        if (!gdk_rgba_parse (&c->high_throttle_color, str))
            gdk_rgba_parse (&c->high_throttle_color, "red");
    } else gdk_rgba_parse (&c->high_throttle_color, "red");

    if (config_setting_lookup_int ("cputemp", "LowTemp", &val))
    {
        if (val >= 0 && val <= 100) c->lower_temp = val;
        else c->lower_temp = 40;
    }
    else c->lower_temp = 40;

    if (config_setting_lookup_int ("cputemp", "HighTemp", &val))
    {
        if (val >= 0 && val <= 150 && val > c->lower_temp) c->upper_temp = val;
        else c->upper_temp = 90;
    }
    else c->upper_temp = 90;

    /* Find the system thermal sensors */
    check_sensors (c);

    /* Connect a timer to refresh the statistics. */
    c->timer = g_timeout_add (1500, (GSourceFunc) cpu_update, (gpointer) c);

    /* Show the widget and return. */
    gtk_widget_show_all (c->plugin);
}

