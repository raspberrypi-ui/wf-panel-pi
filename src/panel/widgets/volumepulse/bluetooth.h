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

extern void bluetooth_init (VolumePulsePlugin *vol);
extern void bluetooth_terminate (VolumePulsePlugin *vol);

extern gboolean bluetooth_is_connected (VolumePulsePlugin *vol, const char *path);

extern void bluetooth_set_output (VolumePulsePlugin *vol, const char *name, const char *label);
extern void bluetooth_set_input (VolumePulsePlugin *vol, const char *name, const char *label);

extern void bluetooth_remove_output (VolumePulsePlugin *vol);
extern void bluetooth_remove_input (VolumePulsePlugin *vol);

extern void bluetooth_reconnect (VolumePulsePlugin *vol, const char *name, const char *profile);

extern void bluetooth_add_devices_to_menu (VolumePulsePlugin *vol);
extern void bluetooth_add_devices_to_profile_dialog (VolumePulsePlugin *vol);
extern int bluetooth_count_devices (VolumePulsePlugin *vol, gboolean input);

/* End of file */
/*----------------------------------------------------------------------------*/
