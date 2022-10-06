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

extern void pulse_init (VolumePulsePlugin *vol);
extern void pulse_terminate (VolumePulsePlugin *vol);

extern int pulse_get_volume (VolumePulsePlugin *vol);
extern int pulse_set_volume (VolumePulsePlugin *vol, int volume);

extern int pulse_get_mute (VolumePulsePlugin *vol);
extern int pulse_set_mute (VolumePulsePlugin *vol, int mute);

extern int pulse_get_default_sink_source (VolumePulsePlugin *vol);
extern void pulse_change_sink (VolumePulsePlugin *vol, const char *sinkname);
extern void pulse_change_source (VolumePulsePlugin *vol, const char *sourcename);

extern void pulse_mute_all_streams (VolumePulsePlugin *vol);
extern void pulse_unmute_all_streams (VolumePulsePlugin *vol);

extern void pulse_move_input_streams (VolumePulsePlugin *vol);
extern void pulse_move_output_streams (VolumePulsePlugin *vol);

extern int pulse_get_profile (VolumePulsePlugin *vol, const char *card);
extern int pulse_set_profile (VolumePulsePlugin *vol, const char *card, const char *profile);

extern int pulse_add_devices_to_menu (VolumePulsePlugin *vol, gboolean internal);
extern void pulse_update_devices_in_menu (VolumePulsePlugin *vol);
extern int pulse_add_devices_to_profile_dialog (VolumePulsePlugin *vol);

extern int pulse_count_devices (VolumePulsePlugin *vol);

/* End of file */
/*----------------------------------------------------------------------------*/
