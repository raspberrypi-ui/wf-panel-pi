/*
Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <pulse/pulseaudio.h>

#include "../lxutils.h"
//#include "plugin.h"

#define DEBUG_ON
#ifdef DEBUG_ON
#define DEBUG(fmt,args...) if(getenv("DEBUG_VP"))g_message("vp: " fmt,##args)
#else
#define DEBUG(fmt,args...)
#endif

#define volumepulse_update_display(vol) do { if (vol->input_control) micpulse_update_display (vol); else volpulse_update_display (vol); } while (0)
#define menu_show(vol) do { if (vol->input_control) mic_menu_show (vol); else vol_menu_show (vol); } while (0)
#define menu_add_item(vol,label,name) do { if (vol->input_control) mic_menu_add_item (vol, label, name); else vol_menu_add_item (vol, label, name); } while (0)

#ifndef VOLUMEPULSE_STRUCT
#define VOLUMEPULSE_STRUCT

typedef struct {
    /* plugin */
    GtkWidget *plugin;                  /* Pointer to widget */
    //LXPanel *panel;                   /* Back pointer to panel */
    //config_setting_t *settings;       /* Plugin settings */

    int icon_size;                      /* Variables used under wf-panel */
    gboolean bottom;
    gboolean wizard;

    gboolean pipewire;                  /* Pipewire running? */

    /* graphics */
    GtkWidget *tray_icon;               /* Displayed icon */
    GtkWidget *popup_window;            /* Top level window for popup */
    GtkWidget *popup_volume_scale;      /* Scale for volume */
    GtkWidget *popup_mute_check;        /* Checkbox for mute state */
    GtkWidget *menu_devices;            /* Right-click menu */
    GtkWidget *profiles_dialog;         /* Device profiles dialog */
    GtkWidget *profiles_int_box;        /* Vbox for profile combos */
    GtkWidget *profiles_ext_box;        /* Vbox for profile combos */
    GtkWidget *profiles_bt_box;         /* Vbox for profile combos */
    GtkWidget *conn_dialog;             /* Connection dialog box */
    GtkWidget *conn_label;              /* Dialog box text field */
    GtkWidget *conn_ok;                 /* Dialog box button */
    guint volume_scale_handler;         /* Handler for volume_scale widget */
    guint mute_check_handler;           /* Handler for mute_check widget */
    gboolean separator;                 /* Flag to show whether a menu separator has been added */
    gboolean input_control;             /* Flag to show whether this is an input or output controller */

    /* HDMI devices */
    char *hdmi_names[2];                /* Display names of HDMI devices */

    /* PulseAudio interface */
    pa_threaded_mainloop *pa_mainloop;  /* Controller loop variable */
    pa_context *pa_cont;                /* Controller context */
    pa_context_state_t pa_state;        /* Current controller state */
    char *pa_default_sink;              /* Current default sink name */
    char *pa_default_source;            /* Current default source name */
    char *pa_profile;                   /* Current profile for card */
    int pa_channels;                    /* Number of channels on default sink */
    int pa_volume;                      /* Volume setting on default sink */
    int pa_mute;                        /* Mute setting on default sink */
    GList *pa_indices;                  /* Indices for current streams */
    char *pa_error_msg;                 /* Error message from success / fail callback */
    int pa_devices;                     /* Counter for pulse devices */
    guint pa_idle_timer;

    /* Bluetooth interface */
    GDBusObjectManager *bt_objmanager;  /* D-Bus BlueZ object manager */
    guint bt_watcher_id;                /* D-Bus BlueZ watcher ID */
    GList *bt_ops;                      /* List of Bluetooth connect and disconnect operations */
    char *bt_iname;                     /* Input device name for use in list */
    char *bt_oname;                     /* Output device name for use in list */
    gboolean bt_input;                  /* Flag to show if current connect operation is for input or output */
    gboolean bt_force_hsp;              /* Flag to override automatic profile selection */
    int bt_profile_count;               /* Counter for polling read of profile on connection */
    guint bt_idle_timer;
} VolumePulsePlugin;

#endif

/* Functions in volumepulse.c needed in other modules */

extern void vol_menu_show (VolumePulsePlugin *vol);
extern void mic_menu_show (VolumePulsePlugin *vol);
extern void vol_menu_add_item (VolumePulsePlugin *vol, const char *label, const char *name);
extern void mic_menu_add_item (VolumePulsePlugin *vol, const char *label, const char *name);
extern void profiles_dialog_add_combo (VolumePulsePlugin *vol, GtkListStore *ls, GtkWidget *dest, int sel, const char *label, const char *name);
extern void volpulse_update_display (VolumePulsePlugin *vol);
extern void micpulse_update_display (VolumePulsePlugin *vol);
extern void hdmi_init (VolumePulsePlugin *vol);
extern void volumepulse_init (VolumePulsePlugin *vol);
extern void micpulse_init (VolumePulsePlugin *vol);
extern gboolean volumepulse_control_msg (VolumePulsePlugin *vol, const char *cmd);
extern void volumepulse_destructor (gpointer user_data);
extern void micpulse_destructor (gpointer user_data);


/* End of file */
/*----------------------------------------------------------------------------*/
