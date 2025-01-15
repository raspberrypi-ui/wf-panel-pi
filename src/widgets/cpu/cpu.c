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

#include "cpu.h"

/* Periodic timer callback */

static gboolean cpu_update (CPUPlugin *c)
{
    struct cpu_stat cpu, cpu_delta;
    char buffer[256];
    FILE *stat;
    float cpu_uns;

    if (g_source_is_destroyed (g_main_current_source ())) return FALSE;

    /* Open statistics file and scan out CPU usage */
    stat = fopen ("/proc/stat", "r");
    if (stat == NULL) return TRUE;
    fgets (buffer, 256, stat);
    fclose (stat);
    if (!strlen (buffer)) return TRUE;
    if (sscanf (buffer, "cpu %llu %llu %llu %llu", &cpu.u, &cpu.n, &cpu.s, &cpu.i) == 4)
    {
        /* Compute delta from previous statistics */
        cpu_delta.u = cpu.u - c->previous_cpu_stat.u;
        cpu_delta.n = cpu.n - c->previous_cpu_stat.n;
        cpu_delta.s = cpu.s - c->previous_cpu_stat.s;
        cpu_delta.i = cpu.i - c->previous_cpu_stat.i;

        /* Copy current to previous */
        memcpy (&c->previous_cpu_stat, &cpu, sizeof (struct cpu_stat));

        /* Compute user + nice + system as a fraction of total */
        cpu_uns = cpu_delta.u + cpu_delta.n + cpu_delta.s;
        cpu_uns /= (cpu_uns + cpu_delta.i);
        if (c->show_percentage) sprintf (buffer, "C:%3.0f", cpu_uns * 100.0);
        else buffer[0] = 0;

        graph_new_point (&(c->graph), cpu_uns, 0, buffer);
    }

    return TRUE;
}

void cpu_update_display (CPUPlugin *c)
{
    GdkRGBA none = {0, 0, 0, 0};
    graph_reload (&(c->graph), c->icon_size, c->background_colour, c->foreground_colour, none, none);
}

void cpu_destructor (gpointer user_data)
{
    CPUPlugin *c = (CPUPlugin *) user_data;
    graph_free (&(c->graph));
    if (c->timer) g_source_remove (c->timer);
    if (c->gesture) g_object_unref (c->gesture);
    g_free (c);
}

void cpu_init (CPUPlugin *c)
{
    /* Allocate icon as a child of top level */
    graph_init (&(c->graph));
    gtk_container_add (GTK_CONTAINER (c->plugin), c->graph.da);

    cpu_update_display (c);

    /* Set up long press */
    c->gesture = add_long_press (c->plugin, NULL, NULL);

    /* Connect a timer to refresh the statistics. */
    c->timer = g_timeout_add (1500, (GSourceFunc) cpu_update, (gpointer) c);

    /* Show the widget and return. */
    gtk_widget_show_all (c->plugin);
}

