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

#include "volumepulse.h"
#include "commongui.h"
#include "bluetooth.h"

#include "pulse.h"

/*----------------------------------------------------------------------------*/
/* Local macros and definitions                                               */
/*----------------------------------------------------------------------------*/

/*
 * Access to the controller is via asynchronous functions which request
 * information or settings. Because the plugin itself runs synchronously,
 * all controller access functions are wrapped in code which waits for
 * them to complete. Returned values, where appropriate, are written to
 * the plugin global data structure via callbacks from the async functions.
 * The macros below are the boilerplate around each async call.
 */

#define START_PA_OPERATION \
    pa_operation *op; \
    if (!vol->pa_cont) return 0; \
    if (vol->pa_error_msg) \
    { \
        g_free (vol->pa_error_msg); \
        vol->pa_error_msg = NULL; \
    } \
    pa_threaded_mainloop_lock (vol->pa_mainloop);

#define END_PA_OPERATION(name) \
    if (!op) \
    { \
        pa_threaded_mainloop_unlock (vol->pa_mainloop); \
        pa_error_handler (vol, name); \
        return 0; \
    } \
    while (pa_operation_get_state (op) == PA_OPERATION_RUNNING) \
    { \
        pa_threaded_mainloop_wait (vol->pa_mainloop); \
    } \
    pa_operation_unref (op); \
    pa_threaded_mainloop_unlock (vol->pa_mainloop); \
    if (vol->pa_error_msg) return 0; \
    else return 1;
    
#define PA_VOL_SCALE 655    /* GTK volume scale is 0-100; PA scale is 0-65535 */

/*----------------------------------------------------------------------------*/
/* Static function prototypes                                                 */
/*----------------------------------------------------------------------------*/

static void pa_cb_state (pa_context *pacontext, void *userdata);
static void pa_error_handler (VolumePulsePlugin *vol, char *name);
static int pa_set_subscription (VolumePulsePlugin *vol);
static void pa_cb_subscription (pa_context *pacontext, pa_subscription_event_type_t event, uint32_t idx, void *userdata);
static gboolean pa_update_disp_cb (gpointer userdata);
static void pa_cb_generic_success (pa_context *context, int success, void *userdata);
static int pa_get_current_vol_mute (VolumePulsePlugin *vol, gboolean input_control);
static void pa_cb_get_current_vol_mute (pa_context *context, const pa_sink_info *i, int eol, void *userdata);
static void pa_cb_get_current_input_vol_mute (pa_context *context, const pa_source_info *i, int eol, void *userdata);
static int pa_get_channels (VolumePulsePlugin *vol);
static void pa_cb_get_channels (pa_context *context, const pa_sink_info *i, int eol, void *userdata);
static int pa_restore_volume (VolumePulsePlugin *vol);
static int pa_restore_mute (VolumePulsePlugin *vol);
static void pa_cb_get_default_sink_source (pa_context *context, const pa_server_info *i, void *userdata);
static int pa_set_default_sink (VolumePulsePlugin *vol, const char *sinkname);
static int pa_get_output_streams (VolumePulsePlugin *vol);
static void pa_cb_get_output_streams (pa_context *context, const pa_sink_input_info *i, int eol, void *userdata);
static void pa_list_move_to_default_sink (gpointer data, gpointer userdata);
static int pa_move_stream_to_default_sink (VolumePulsePlugin *vol, int index);
static int pa_set_default_source (VolumePulsePlugin *vol, const char *sourcename);
static int pa_get_input_streams (VolumePulsePlugin *vol);
static void pa_cb_get_input_streams (pa_context *context, const pa_source_output_info *i, int eol, void *userdata);
static void pa_list_move_to_default_source (gpointer data, gpointer userdata);
static int pa_move_stream_to_default_source (VolumePulsePlugin *vol, int index);
static void pa_list_mute_stream (gpointer data, gpointer userdata);
static int pa_mute_stream (VolumePulsePlugin *vol, int index);
static void pa_list_unmute_stream (gpointer data, gpointer userdata);
static int pa_unmute_stream (VolumePulsePlugin *vol, int index);
static void pa_cb_get_profile (pa_context *c, const pa_card_info *i, int eol, void *userdata);
static void pa_cb_get_info_inputs (pa_context *c, const pa_card_info *i, int eol, void *userdata);
static void pa_cb_get_info_internal (pa_context *c, const pa_card_info *i, int eol, void *userdata);
static void pa_cb_get_info_external (pa_context *c, const pa_card_info *i, int eol, void *userdata);
static gboolean pa_card_has_port (const pa_card_info *i, pa_direction_t dir);
static int pa_replace_cards_with_sinks (VolumePulsePlugin *vol);
static void pa_cb_replace_cards_with_sinks (pa_context *context, const pa_sink_info *i, int eol, void *userdata);
static void pa_replace_card_with_sink_on_match (GtkWidget *widget, gpointer data);
static void pa_card_check_bt_output_profile (GtkWidget *widget, gpointer data);
static int pa_replace_cards_with_sources (VolumePulsePlugin *vol);
static void pa_cb_replace_cards_with_sources (pa_context *context, const pa_source_info *i, int eol, void *userdata);
static void pa_replace_card_with_source_on_match (GtkWidget *widget, gpointer data);
static void pa_card_check_bt_input_profile (GtkWidget *widget, gpointer data);
static void pa_cb_add_devices_to_profile_dialog (pa_context *c, const pa_card_info *i, int eol, void *userdata);
static void pa_cb_count_inputs (pa_context *c, const pa_card_info *i, int eol, void *userdata);
static void pa_cb_count_outputs (pa_context *c, const pa_card_info *i, int eol, void *userdata);

/*----------------------------------------------------------------------------*/
/* PulseAudio controller initialisation / teardown                            */
/*----------------------------------------------------------------------------*/

/* 
 * The PulseAudio controller runs asynchronously in a new thread.
 * The initial functions are to set up and tear down the controller,
 * which is subsequently accessed by its context, which is created
 * during the init function and a pointer to which is stored in the
 * plugin global data structure
 */

void pulse_init (VolumePulsePlugin *vol)
{
    DEBUG ("pulse_init");
    pa_proplist *paprop;
    pa_mainloop_api *paapi;

    vol->pa_cont = NULL;
    vol->pa_idle_timer = 0;
    vol->pa_mainloop = pa_threaded_mainloop_new ();
    pa_threaded_mainloop_start (vol->pa_mainloop);

    pa_threaded_mainloop_lock (vol->pa_mainloop);
    paapi = pa_threaded_mainloop_get_api (vol->pa_mainloop);

    paprop = pa_proplist_new ();
    pa_proplist_sets (paprop, PA_PROP_APPLICATION_NAME, "unknown");
    pa_proplist_sets (paprop, PA_PROP_MEDIA_ROLE, "music");
    vol->pa_cont = pa_context_new_with_proplist (paapi, "unknown", paprop);
    pa_proplist_free (paprop);

    if (vol->pa_cont == NULL)
    {
        pa_threaded_mainloop_unlock (vol->pa_mainloop);
        pa_error_handler (vol, "create context");
        return;
    }

    vol->pa_state = PA_CONTEXT_UNCONNECTED;

    pa_context_set_state_callback (vol->pa_cont, &pa_cb_state, vol);
    pa_context_connect (vol->pa_cont, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);

    while (vol->pa_state != PA_CONTEXT_READY && vol->pa_state != PA_CONTEXT_FAILED)
    {
        pa_threaded_mainloop_wait (vol->pa_mainloop);
    }

    pa_threaded_mainloop_unlock (vol->pa_mainloop);

    if (vol->pa_state != PA_CONTEXT_READY)
    {
        pa_error_handler (vol, "init context");
        return;
    }

    vol->pa_default_sink = NULL;
    vol->pa_default_source = NULL;
    vol->pa_profile = NULL;
    vol->pa_indices = NULL;

    pa_set_subscription (vol);
    pulse_get_default_sink_source (vol);
    pulse_move_output_streams (vol);
    pulse_move_input_streams (vol);
}

/* Callback for changes in context state during initialisation */

static void pa_cb_state (pa_context *pacontext, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    if (pacontext == NULL)
    {
        vol->pa_state = PA_CONTEXT_FAILED;
        pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
        return;
    }

    vol->pa_state = pa_context_get_state (pacontext);

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

/* Teardown PulseAudio controller */

void pulse_terminate (VolumePulsePlugin *vol)
{
    if (vol->pa_idle_timer) g_source_remove (vol->pa_idle_timer);
    if (vol->pa_mainloop != NULL)
    {
        /* Disconnect the controller context */
        if (vol->pa_cont != NULL)
        {
            pa_threaded_mainloop_lock (vol->pa_mainloop);
            pa_context_disconnect (vol->pa_cont);
            pa_context_unref (vol->pa_cont);
            vol->pa_cont = NULL;
            pa_threaded_mainloop_unlock (vol->pa_mainloop);
        }

        /* Terminate the control loop */
        pa_threaded_mainloop_stop (vol->pa_mainloop);
        pa_threaded_mainloop_free (vol->pa_mainloop);
    }
}

/* Handler for unrecoverable errors - terminates the controller */

static void pa_error_handler (VolumePulsePlugin *vol, char *name)
{
    if (vol->pa_cont != NULL)
    {
        int code = pa_context_errno (vol->pa_cont);
        g_warning ("%s: err:%d %s\n", name, code, pa_strerror (code));
    }
    pulse_terminate (vol);
}

/*----------------------------------------------------------------------------*/
/* Event notification                                                         */
/*----------------------------------------------------------------------------*/

/* Subscribe to notifications from the Pulse server */

static int pa_set_subscription (VolumePulsePlugin *vol)
{
    pa_context_set_subscribe_callback (vol->pa_cont, &pa_cb_subscription, vol);
    START_PA_OPERATION
    op = pa_context_subscribe (vol->pa_cont, PA_SUBSCRIPTION_MASK_ALL, &pa_cb_generic_success, vol);
    END_PA_OPERATION ("subscribe")
}

/* Callback for notifications from the Pulse server */

static void pa_cb_subscription (pa_context *, pa_subscription_event_type_t event, uint32_t, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    const char *fac, *type;
    int newcard = 0;

    switch (event & PA_SUBSCRIPTION_EVENT_FACILITY_MASK)
    {
        case PA_SUBSCRIPTION_EVENT_SINK : fac = "sink"; break;
        case PA_SUBSCRIPTION_EVENT_SOURCE : fac = "source"; break;
        case PA_SUBSCRIPTION_EVENT_SINK_INPUT : fac = "sink input"; break;
        case PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT : fac = "source output"; break;
        case PA_SUBSCRIPTION_EVENT_MODULE : fac = "module"; break;
        case PA_SUBSCRIPTION_EVENT_CLIENT : fac = "client"; break;
        case PA_SUBSCRIPTION_EVENT_SAMPLE_CACHE : fac = "sample cache"; break;
        case PA_SUBSCRIPTION_EVENT_SERVER : fac = "server"; break;
        case PA_SUBSCRIPTION_EVENT_CARD : fac = "card"; newcard++; break;
        default : fac = "unknown";
    }
    switch (event & PA_SUBSCRIPTION_EVENT_TYPE_MASK)
    {
        case PA_SUBSCRIPTION_EVENT_NEW : type = "New"; newcard++; break;
        case PA_SUBSCRIPTION_EVENT_CHANGE : type = "Change"; break;
        case PA_SUBSCRIPTION_EVENT_REMOVE : type = "Remove"; break;
        default : type = "unknown";
    }

#ifdef DEBUG_ON
    DEBUG ("PulseAudio event : %s %s", type, fac);
#endif
    if (vol->bt_card_found == FALSE && newcard) vol->bt_card_found = TRUE;

    vol->pa_idle_timer = g_idle_add (pa_update_disp_cb, vol);

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

/* Function to update display called when idle after a notification - needs not to be in main loop  */

static gboolean pa_update_disp_cb (gpointer userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    vol->pa_idle_timer = 0;
    volumepulse_update_display (vol);
    micpulse_update_display (vol);
    return FALSE;
}

/* Callback for PulseAudio operations which report success/fail */

static void pa_cb_generic_success (pa_context *context, int success, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    if (!success)
    {
        DEBUG ("pulse success callback failed : %s", pa_strerror (pa_context_errno (context)));
        vol->pa_error_msg = g_strdup (pa_strerror (pa_context_errno (context)));
    }

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

/*----------------------------------------------------------------------------*/
/* Volume and mute control                                                    */
/*----------------------------------------------------------------------------*/

/* 
 * For get operations, the generic get_sink_info operation is called on the
 * current default sink; the values are written into the global structure
 * by the callbacks, and the top-level functions return them from there.
 * For set operations, the specific set_sink_xxx operations are called.
 */

int pulse_get_volume (VolumePulsePlugin *vol, gboolean input_control)
{
    pa_get_current_vol_mute (vol, input_control);
    return vol->pa_volume / PA_VOL_SCALE;
}

int pulse_set_volume (VolumePulsePlugin *vol, int volume, gboolean input_control)
{
    pa_cvolume cvol;
    int i;

    vol->pa_volume = volume * PA_VOL_SCALE;
    if (vol->pa_volume < 0) vol->pa_volume = 0;
    if (vol->pa_volume > 65535) vol->pa_volume = 65535;
    cvol.channels = vol->pa_channels;
    for (i = 0; i < cvol.channels; i++) cvol.values[i] = vol->pa_volume;

    DEBUG ("pulse_set_volume %d %d", volume, input_control);
    START_PA_OPERATION
    if (input_control)
        op = pa_context_set_source_volume_by_name (vol->pa_cont, vol->pa_default_source, &cvol, &pa_cb_generic_success, vol);
    else
        op = pa_context_set_sink_volume_by_name (vol->pa_cont, vol->pa_default_sink, &cvol, &pa_cb_generic_success, vol);
    END_PA_OPERATION ("set_sink_volume_by_name")
}

int pulse_get_mute (VolumePulsePlugin *vol, gboolean input_control)
{
    pa_get_current_vol_mute (vol, input_control);
    return vol->pa_mute;
}

int pulse_set_mute (VolumePulsePlugin *vol, int mute, gboolean input_control)
{
    vol->pa_mute = mute;

    DEBUG ("pulse_set_mute %d %d", mute, input_control);
    START_PA_OPERATION
    if (input_control)
        op = pa_context_set_source_mute_by_name (vol->pa_cont, vol->pa_default_source, vol->pa_mute, &pa_cb_generic_success, vol);
    else
        op = pa_context_set_sink_mute_by_name (vol->pa_cont, vol->pa_default_sink, vol->pa_mute, &pa_cb_generic_success, vol);
    END_PA_OPERATION ("set_sink_mute_by_name");
}

/* Query the controller for the volume and mute settings for the current default sink */

static int pa_get_current_vol_mute (VolumePulsePlugin *vol, gboolean input_control)
{
    START_PA_OPERATION
    if (input_control)
        op = pa_context_get_source_info_by_name (vol->pa_cont, vol->pa_default_source, &pa_cb_get_current_input_vol_mute, vol);
    else
        op = pa_context_get_sink_info_by_name (vol->pa_cont, vol->pa_default_sink, &pa_cb_get_current_vol_mute, vol);
    END_PA_OPERATION ("get_sink_info_by_name")
}

/* Callback for volume / mute query */

static void pa_cb_get_current_vol_mute (pa_context *, const pa_sink_info *i, int eol, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    if (!eol)
    {
        vol->pa_channels = i->volume.channels;
        vol->pa_volume = i->volume.values[0];
        vol->pa_mute = i->mute;
    }

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

static void pa_cb_get_current_input_vol_mute (pa_context *, const pa_source_info *i, int eol, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    if (!eol)
    {
        vol->pa_channels = i->volume.channels;
        vol->pa_volume = i->volume.values[0];
        vol->pa_mute = i->mute;
    }

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

/* Set volume for new sink to global value read from old sink */

static int pa_restore_volume (VolumePulsePlugin *vol)
{
    pa_cvolume cvol;
    int i;

    cvol.channels = vol->pa_channels;
    for (i = 0; i < cvol.channels; i++) cvol.values[i] = vol->pa_volume;

    DEBUG ("pa_restore_volume");
    START_PA_OPERATION
    op = pa_context_set_sink_volume_by_name (vol->pa_cont, vol->pa_default_sink, &cvol, &pa_cb_generic_success, vol);
    END_PA_OPERATION ("set_sink_volume_by_name")
}

/* Set mute for new sink to global value read from old sink */

static int pa_restore_mute (VolumePulsePlugin *vol)
{
    DEBUG ("pa_restore_mute");
    START_PA_OPERATION
    op = pa_context_set_sink_mute_by_name (vol->pa_cont, vol->pa_default_sink, vol->pa_mute, &pa_cb_generic_success, vol);
    END_PA_OPERATION ("set_sink_mute_by_name");
}

/* Query the controller for the number of channels on the current default sink */

static int pa_get_channels (VolumePulsePlugin *vol)
{
    START_PA_OPERATION
    op = pa_context_get_sink_info_by_name (vol->pa_cont, vol->pa_default_sink, &pa_cb_get_channels, vol);
    END_PA_OPERATION ("get_sink_info_by_name")
}

/* Callback for volume / mute query */

static void pa_cb_get_channels (pa_context *, const pa_sink_info *i, int eol, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    if (!eol)
    {
        vol->pa_channels = i->volume.channels;
    }

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

/*----------------------------------------------------------------------------*/
/* Sink and source control                                                    */
/*----------------------------------------------------------------------------*/

/* Update the names of the current default sink and source in the plugin data structure */

int pulse_get_default_sink_source (VolumePulsePlugin *vol)
{
    DEBUG ("pulse_get_default_sink_source");
    START_PA_OPERATION
    op = pa_context_get_server_info (vol->pa_cont, &pa_cb_get_default_sink_source, vol);
    END_PA_OPERATION ("get_server_info")
}

/* Callback for default sink and source query */

static void pa_cb_get_default_sink_source (pa_context *, const pa_server_info *i, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    DEBUG ("pa_cb_get_default_sink_source %s %s", i->default_sink_name, i->default_source_name);
    if (vol->pa_default_sink) g_free (vol->pa_default_sink);
    vol->pa_default_sink = g_strdup (i->default_sink_name);

    if (vol->pa_default_source) g_free (vol->pa_default_source);
    vol->pa_default_source = g_strdup (i->default_source_name);

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

/*
 * To change sink, first the default sink is updated to the new sink.
 * Then, all currently active output streams are listed in pa_indices.
 * Finally, all streams listed in pa_indices are moved to the new sink.
 */

int pulse_change_sink (VolumePulsePlugin *vol, const char *sinkname)
{
    DEBUG ("pulse_change_sink %s", sinkname);
    if (vol->pa_default_sink) g_free (vol->pa_default_sink);
    vol->pa_default_sink = g_strdup (sinkname);

    if (!pa_set_default_sink (vol, sinkname))
    {
        DEBUG ("pulse_change_sink error");
        return 0;
    }
    else
    {
        pa_get_channels (vol);
        pa_restore_volume (vol);
        pa_restore_mute (vol);
    }
    DEBUG ("pulse_change_sink done");
    return 1;
}

/* Create a list of current output streams and move each to the default sink */

void pulse_move_output_streams (VolumePulsePlugin *vol)
{
    DEBUG ("pulse_move_output_streams");
    vol->pa_indices = NULL;
    pa_get_output_streams (vol);
    g_list_foreach (vol->pa_indices, pa_list_move_to_default_sink, vol);
    g_list_free (vol->pa_indices);
    DEBUG ("pulse_move_output_streams done");
}

/* Call the PulseAudio set default sink operation */

static int pa_set_default_sink (VolumePulsePlugin *vol, const char *sinkname)
{
    DEBUG ("pa_set_default_sink %s", sinkname);
    START_PA_OPERATION
    op = pa_context_set_default_sink (vol->pa_cont, sinkname, &pa_cb_generic_success, vol);
    END_PA_OPERATION ("set_default_sink")
}

/* Query the controller for a list of current output streams */

static int pa_get_output_streams (VolumePulsePlugin *vol)
{
    DEBUG ("pa_get_output_streams");
    START_PA_OPERATION
    op = pa_context_get_sink_input_info_list (vol->pa_cont, &pa_cb_get_output_streams, vol);
    END_PA_OPERATION ("get_sink_input_info_list")
}

/* Callback for output stream query */

static void pa_cb_get_output_streams (pa_context *, const pa_sink_input_info *i, int eol, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    if (!eol)
    {
        DEBUG ("pa_cb_get_output_streams %d", i->index);
        vol->pa_indices = g_list_append (vol->pa_indices, (void *) i->index);
    }

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

/* Callback for per-stream operation by looping through pa_indices, moving stream for each */

static void pa_list_move_to_default_sink (gpointer data, gpointer userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    pa_move_stream_to_default_sink (vol, (int) data);
}

/* Call the PulseAudio move stream operation for the supplied index to move the stream to the default sink */

static int pa_move_stream_to_default_sink (VolumePulsePlugin *vol, int index)
{
    DEBUG ("pa_move_stream_to_default_sink %s %d", vol->pa_default_sink, index);
    START_PA_OPERATION
    op = pa_context_move_sink_input_by_name (vol->pa_cont, index, vol->pa_default_sink, &pa_cb_generic_success, vol);
    END_PA_OPERATION ("move_sink_input_by_name")
}

/*
 * To change source, first the default source is updated to the new source.
 * Then, all currently active input streams are listed in pa_indices.
 * Finally, all streams listed in pa_indices are moved to the new source.
 */
 
int pulse_change_source (VolumePulsePlugin *vol, const char *sourcename)
{
    DEBUG ("pulse_change_source %s", sourcename);
    if (vol->pa_default_source) g_free (vol->pa_default_source);
    vol->pa_default_source = g_strdup (sourcename);

    if (!pa_set_default_source (vol, sourcename))
    {
        DEBUG ("pulse_change_source error");
        return 0;
    }

    DEBUG ("pulse_change_source done");
    return 1;
}

/* Call the PulseAudio set default source operation */

static int pa_set_default_source (VolumePulsePlugin *vol, const char *sourcename)
{
    DEBUG ("pa_set_default_source %s", sourcename);
    START_PA_OPERATION
    op = pa_context_set_default_source (vol->pa_cont, sourcename, &pa_cb_generic_success, vol);
    END_PA_OPERATION ("set_default_source")
}

/* Create a list of current input streams and move each to the default source */

void pulse_move_input_streams (VolumePulsePlugin *vol)
{
    DEBUG ("pulse_move_input_streams");
    vol->pa_indices = NULL;
    pa_get_input_streams (vol);
    g_list_foreach (vol->pa_indices, pa_list_move_to_default_source, vol);
    g_list_free (vol->pa_indices);
    DEBUG ("pulse_move_input_streams done");
}

/* Query the controller for a list of current output streams */

static int pa_get_input_streams (VolumePulsePlugin *vol)
{
    DEBUG ("pa_get_input_streams");
    START_PA_OPERATION
    op = pa_context_get_source_output_info_list (vol->pa_cont, &pa_cb_get_input_streams, vol);
    END_PA_OPERATION ("get_sink_input_info_list")
}

/* Callback for input stream query */

static void pa_cb_get_input_streams (pa_context *, const pa_source_output_info *i, int eol, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    if (!eol)
    {
        DEBUG ("pa_cb_get_input_streams %d", i->index);
        vol->pa_indices = g_list_append (vol->pa_indices, (void *) i->index);
    }

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

/* Callback for per-stream operation by looping through pa_indices, moving stream for each */

static void pa_list_move_to_default_source (gpointer data, gpointer userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    pa_move_stream_to_default_source (vol, (int) data);
}

/* Call the PulseAudio move stream operation for the supplied index to move the stream to the default source */

static int pa_move_stream_to_default_source (VolumePulsePlugin *vol, int index)
{
    DEBUG ("pa_move_stream_to_default_source %s %d", vol->pa_default_source, index);
    START_PA_OPERATION
    op = pa_context_move_source_output_by_name (vol->pa_cont, index, vol->pa_default_source, &pa_cb_generic_success, vol);
    END_PA_OPERATION ("move_source_output_by_name")
}

/*----------------------------------------------------------------------------*/
/* Output control                                                             */
/*----------------------------------------------------------------------------*/

void pulse_mute_all_streams (VolumePulsePlugin *vol)
{
    DEBUG ("pulse_mute_all_streams");

    vol->pa_indices = NULL;
    pa_get_output_streams (vol);
    g_list_foreach (vol->pa_indices, pa_list_mute_stream, vol);
    g_list_free (vol->pa_indices);
    DEBUG ("pulse_mute_all_streams done");
}

/* Callback for per-stream operation by looping through pa_indices, muting stream for each */

static void pa_list_mute_stream (gpointer data, gpointer userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    pa_mute_stream (vol, (int) data);
}

/* Call the PulseAudio mute stream operation for the supplied index*/

static int pa_mute_stream (VolumePulsePlugin *vol, int index)
{
    DEBUG ("pa_mute_stream %d", index);
    START_PA_OPERATION
    op = pa_context_set_sink_input_mute (vol->pa_cont, index, 1, &pa_cb_generic_success, vol);
    END_PA_OPERATION ("set_sink_input_mute")
}

void pulse_unmute_all_streams (VolumePulsePlugin *vol)
{
    DEBUG ("pulse_unmute_all_streams");

    vol->pa_indices = NULL;
    pa_get_output_streams (vol);
    g_list_foreach (vol->pa_indices, pa_list_unmute_stream, vol);
    g_list_free (vol->pa_indices);
    DEBUG ("pulse_unmute_all_streams done");
}

/* Callback for per-stream operation by looping through pa_indices, unmuting stream for each */

static void pa_list_unmute_stream (gpointer data, gpointer userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    pa_unmute_stream (vol, (int) data);
}

/* Call the PulseAudio unmute stream operation for the supplied index*/

static int pa_unmute_stream (VolumePulsePlugin *vol, int index)
{
    DEBUG ("pa_unmute_stream %d", index);
    START_PA_OPERATION
    op = pa_context_set_sink_input_mute (vol->pa_cont, index, 0, &pa_cb_generic_success, vol);
    END_PA_OPERATION ("set_sink_input_mute")
}

/*----------------------------------------------------------------------------*/
/* Profiles                                                                   */
/*----------------------------------------------------------------------------*/

/* 
 * Read the profile of the supplied card - this is polled to check if a Bluetooth device has
 * successfully connected to PulseAudio, hence why the profile is NULLed before starting so
 * that old profile data does not spuriously cause the poll to succeed prematurely.
 */ 

int pulse_get_profile (VolumePulsePlugin *vol, const char *card)
{
    if (vol->pa_profile)
    {
        g_free (vol->pa_profile);
        vol->pa_profile = NULL;
    }

    START_PA_OPERATION
    op = pa_context_get_card_info_by_name (vol->pa_cont, card, &pa_cb_get_profile, vol);
    END_PA_OPERATION ("get_card_info_by_name")
}

/* Callback for profile query */

static void pa_cb_get_profile (pa_context *, const pa_card_info *i, int eol, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    if (!eol)
    {
        DEBUG ("pa_cb_get_profile %s", i->active_profile2->name);
        if (vol->pa_profile) g_free (vol->pa_profile);
        vol->pa_profile = g_strdup (i->active_profile2->name);
    }

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

/* Call the PulseAudio set profile operation for the supplied card */

int pulse_set_profile (VolumePulsePlugin *vol, const char *card, const char *profile)
{
    DEBUG ("pulse_set_profile %s %s", card, profile);
    START_PA_OPERATION
    op = pa_context_set_card_profile_by_name (vol->pa_cont, card, profile, &pa_cb_generic_success, vol);
    END_PA_OPERATION ("set_card_profile_by_name")
}

/*----------------------------------------------------------------------------*/
/* Device menu                                                                */
/*----------------------------------------------------------------------------*/

/* 
 * To populate the device select menu, the controller is initially queried for the
 * list of audio cards, which are stored with their card names. After discovery is
 * complete, the controller is queried for the list of sinks and sources, which is
 * used to replace the card name with the relevant sink or source name, allowing
 * cards which are have the wrong profile set to be shown greyed-out in the menu.
 */
 
/* Loop through all cards, adding each to relevant part of device menu */

int pulse_add_devices_to_menu (VolumePulsePlugin *vol, gboolean internal, gboolean input_control)
{
    if (internal && input_control) return 0;
    vol->separator = FALSE;
    DEBUG ("pulse_add_devices_to_menu %d %d", input_control, internal);
    START_PA_OPERATION
    if (input_control)
        op = pa_context_get_card_info_list (vol->pa_cont, &pa_cb_get_info_inputs, vol);
    else if (internal)
        op = pa_context_get_card_info_list (vol->pa_cont, &pa_cb_get_info_internal, vol);
    else
        op = pa_context_get_card_info_list (vol->pa_cont, &pa_cb_get_info_external, vol);
    END_PA_OPERATION ("get_card_info_list")
}

/*
 * Callbacks for card info query, each of which checks to see if the device should
 * be in the menu in question and adding it if so
 */

static void pa_cb_get_info_inputs (pa_context *, const pa_card_info *i, int eol, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    if (!eol)
    {
        if (pa_card_has_port (i, PA_DIRECTION_INPUT))
        {
            const char *nam = pa_proplist_gets (i->proplist, "alsa.card_name");
            const char *dev = pa_proplist_gets (i->proplist, "alsa.card");
            if (nam)
            {
                DEBUG ("pa_cb_get_info_inputs %s", dev);
                mic_menu_add_item (vol, nam, dev);
            }
        }
    }

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

static void pa_cb_get_info_internal (pa_context *, const pa_card_info *i, int eol, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    if (!eol)
    {
        if (!g_strcmp0 (pa_proplist_gets (i->proplist, "device.description"), "Built-in Audio"))
        {
            if (pa_card_has_port (i, PA_DIRECTION_OUTPUT))
            {
                const char *nam = pa_proplist_gets (i->proplist, "alsa.card_name");
                const char *dev = pa_proplist_gets (i->proplist, "alsa.card");
                if (nam)
                {
                    if (!strcmp (nam, "bcm2835 Headphones") && vsystem ("raspi-config nonint has_analog")) return;
                    DEBUG ("pa_cb_get_info_internal %s", dev);
                    vol_menu_add_item (vol, nam, dev);
                }
            }
        }
    }

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

static void pa_cb_get_info_external (pa_context *, const pa_card_info *i, int eol, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    if (!eol)
    {
        if (g_strcmp0 (pa_proplist_gets (i->proplist, "device.description"), "Built-in Audio"))
        {
            if (pa_card_has_port (i, PA_DIRECTION_OUTPUT))
            {
                const char *nam = pa_proplist_gets (i->proplist, "alsa.card_name");
                const char *dev = pa_proplist_gets (i->proplist, "alsa.card");
                if (nam)
                {
                    DEBUG ("pa_cb_get_info_external %s", dev);
                    menu_add_separator (vol, vol->menu_devices[0]);
                    vol_menu_add_item (vol, nam, dev);
                }
            }
        }
    }

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

/* Function to determine whether or not a card has either input or output ports */

static gboolean pa_card_has_port (const pa_card_info *i, pa_direction_t dir)
{
    pa_card_port_info **port = i->ports;
    if (!i->n_ports) return FALSE;
    while (*port)
    {
        if ((*port)->direction == (int) dir) return TRUE;
        port++;
    }
    return FALSE;
}

/* Loop through all sinks and sources, updating device menu as appropriate */

void pulse_update_devices_in_menu (VolumePulsePlugin *vol, gboolean input_control)
{
    if (input_control) pa_replace_cards_with_sources (vol);
    else pa_replace_cards_with_sinks (vol);
}

/* Query controller for list of sinks */

static int pa_replace_cards_with_sinks (VolumePulsePlugin *vol)
{
    DEBUG ("pa_replace_cards_with_sinks");
    START_PA_OPERATION
    op = pa_context_get_sink_info_list (vol->pa_cont, &pa_cb_replace_cards_with_sinks, vol);
    END_PA_OPERATION ("get_sink_info_list")
}

/* Callback for sink list query, which updates ALSA devices in menu as appropriate */

static void pa_cb_replace_cards_with_sinks (pa_context *, const pa_sink_info *i, int eol, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    if (!eol && vol->menu_devices[0])
    {
        const char *api = pa_proplist_gets (i->proplist, "device.api");
        if (!g_strcmp0 (api, "alsa"))
            gtk_container_foreach (GTK_CONTAINER (vol->menu_devices[0]), pa_replace_card_with_sink_on_match, (void *) i);
        else
            gtk_container_foreach (GTK_CONTAINER (vol->menu_devices[0]), pa_card_check_bt_output_profile, (void *) i);
    }

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

/* Callback for per-menu-item operation which checks to see if each matches the card name and updates with sink data if so */

static void pa_replace_card_with_sink_on_match (GtkWidget *widget, gpointer data)
{
    pa_sink_info *i = (pa_sink_info *) data;
    const char *dev = pa_proplist_gets (i->proplist, "alsa.card");

    if (!strcmp (dev, gtk_widget_get_name (widget)))
    {
        gtk_widget_set_name (widget, i->name);
        gtk_widget_set_sensitive (widget, TRUE);
        gtk_widget_set_tooltip_text (widget, NULL);
    }
}

/* Callback for per-menu-item operation which checks to see if a Bluetooth device is in a profile with an output */

static void pa_card_check_bt_output_profile (GtkWidget *widget, gpointer data)
{
    pa_sink_info *i = (pa_sink_info *) data;
    const char *btpath = pa_proplist_gets (i->proplist, "bluez.path");

    if (!g_strcmp0 (btpath, gtk_widget_get_name (widget)))
    {
        const char *profile = pa_proplist_gets (i->proplist, "bluetooth.protocol");
        if (!g_strcmp0 (profile, "a2dp_sink") || !g_strcmp0 (profile, "headset_head_unit"))
        {
            gtk_widget_set_sensitive (widget, TRUE);
            gtk_widget_set_tooltip_text (widget, NULL);
        }
    }
}

/* Query controller for list of sources */

static int pa_replace_cards_with_sources (VolumePulsePlugin *vol)
{
    DEBUG ("pa_replace_cards_with_sources");
    START_PA_OPERATION
    op = pa_context_get_source_info_list (vol->pa_cont, &pa_cb_replace_cards_with_sources, vol);
    END_PA_OPERATION ("get_source_info_list")
}

/* Callback for source list query, which updates ALSA devices in menu as appropriate */

static void pa_cb_replace_cards_with_sources (pa_context *, const pa_source_info *i, int eol, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    if (!eol && vol->menu_devices[1])
    {
        const char *api = pa_proplist_gets (i->proplist, "device.api");
        if (!g_strcmp0 (api, "alsa"))
            gtk_container_foreach (GTK_CONTAINER (vol->menu_devices[1]), pa_replace_card_with_source_on_match, (void *) i);
        else
            gtk_container_foreach (GTK_CONTAINER (vol->menu_devices[1]), pa_card_check_bt_input_profile, (void *) i);
    }

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

/* Callback for per-menu-item operation which checks to see if each matches the card name and updates with source data if so */

static void pa_replace_card_with_source_on_match (GtkWidget *widget, gpointer data)
{
    pa_source_info *i = (pa_source_info *) data;
    const char *dev = pa_proplist_gets (i->proplist, "alsa.card");

    if (!strcmp (dev, gtk_widget_get_name (widget)))
    {
        gtk_widget_set_name (widget, i->name);
        gtk_widget_set_sensitive (widget, TRUE);
        gtk_widget_set_tooltip_text (widget, NULL);
    }
}

/* Callback for per-menu-item operation which checks to see if a Bluetooth device is in a profile with an input */

static void pa_card_check_bt_input_profile (GtkWidget *widget, gpointer data)
{
    pa_source_info *i = (pa_source_info *) data;
    const char *btpath = pa_proplist_gets (i->proplist, "bluez.path");

    if (!g_strcmp0 (btpath, gtk_widget_get_name (widget)))
    {
        const char *profile = pa_proplist_gets (i->proplist, "bluetooth.protocol");
        if (!g_strcmp0 (profile, "headset_head_unit"))
        {
            gtk_widget_set_sensitive (widget, TRUE);
            gtk_widget_set_tooltip_text (widget, NULL);
        }
    }
}

/*----------------------------------------------------------------------------*/
/* Profiles dialog                                                            */
/*----------------------------------------------------------------------------*/

/* Query controller for list of cards */

int pulse_add_devices_to_profile_dialog (VolumePulsePlugin *vol)
{
    DEBUG ("pulse_add_devices_to_profile_dialog");
    START_PA_OPERATION
    op = pa_context_get_card_info_list (vol->pa_cont, &pa_cb_add_devices_to_profile_dialog, vol);
    END_PA_OPERATION ("get_card_info_list")
}

/* Callback for card list query - reads profiles for card and adds as a combo box to profiles dialog */

static void pa_cb_add_devices_to_profile_dialog (pa_context *, const pa_card_info *i, int eol, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    GtkListStore *ls;
    int index = 0, sel = -1;

    if (!eol)
    {
        // loop through profiles, adding each to list store
        ls = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
        pa_card_profile_info2 **profile = i->profiles2;
        while (*profile)
        {
            if (*profile == i->active_profile2) sel = index;
            gtk_list_store_insert_with_values (ls, NULL, index++, 0, (*profile)->name, 1, (*profile)->description, -1);
            profile++;
        }

        if (!g_strcmp0 (pa_proplist_gets (i->proplist, "device.api"), vol->pipewire ? "bluez5" : "bluez"))
            profiles_dialog_add_combo (vol, ls, vol->profiles_bt_box, sel, pa_proplist_gets (i->proplist, "device.description"), i->name);
        else
        {
            if (g_strcmp0 (pa_proplist_gets (i->proplist, "device.description"), "Built-in Audio"))
                profiles_dialog_add_combo (vol, ls, vol->profiles_ext_box, sel, pa_proplist_gets (i->proplist, "alsa.card_name"), i->name);
            else if (pa_card_has_port (i, PA_DIRECTION_OUTPUT))
                profiles_dialog_add_combo (vol, ls, vol->profiles_int_box, sel, pa_proplist_gets (i->proplist, "alsa.card_name"), i->name);
        }
    }

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

/*----------------------------------------------------------------------------*/
/* Utility functions                                                          */
/*----------------------------------------------------------------------------*/

/* Get a count of the number of input or output devices */

int pulse_count_devices (VolumePulsePlugin *vol, gboolean input_control)
{
    vol->pa_devices = 0;
    START_PA_OPERATION
    if (input_control)
        op = pa_context_get_card_info_list (vol->pa_cont, &pa_cb_count_inputs, vol);
    else
        op = pa_context_get_card_info_list (vol->pa_cont, &pa_cb_count_outputs, vol);
    END_PA_OPERATION ("get_card_info_list")
}

/*
 * Callbacks for card count query, each of which just increments the global device counter
 * whenever a matching device is found
 */

static void pa_cb_count_inputs (pa_context *, const pa_card_info *i, int eol, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    if (!eol)
    {
        if (pa_card_has_port (i, PA_DIRECTION_INPUT))
        {
            const char *nam = pa_proplist_gets (i->proplist, "alsa.card_name");
            if (nam) vol->pa_devices++;
        }
    }

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

static void pa_cb_count_outputs (pa_context *, const pa_card_info *i, int eol, void *userdata)
{
    VolumePulsePlugin *vol = (VolumePulsePlugin *) userdata;

    if (!eol)
    {
        if (pa_card_has_port (i, PA_DIRECTION_OUTPUT))
        {
            const char *nam = pa_proplist_gets (i->proplist, "alsa.card_name");
            if (nam) vol->pa_devices++;
        }
    }

    pa_threaded_mainloop_signal (vol->pa_mainloop, 0);
}

/* End of file */
/*----------------------------------------------------------------------------*/
