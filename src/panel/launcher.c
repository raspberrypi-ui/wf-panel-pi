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

#include <stdio.h>
#include <glib.h>

/* Helper functions for modifying launchers in panel config */

void init_launchers (void)
{
    char *str;
    gsize len;

    // construct the file path
    char *user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi.ini", NULL);

    // read in data from file to a key file
    GKeyFile *kf = g_key_file_new ();
    g_key_file_load_from_file (kf, user_file, (GKeyFileFlags) (G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS), NULL);

    if (!g_key_file_has_group (kf, "panel"))
    {
        // write the default launcher config
        g_key_file_set_string (kf, "panel", "launcher_000001", "lxde-x-www-browser.desktop");
        g_key_file_set_string (kf, "panel", "launcher_000002", "pcmanfm.desktop");
        g_key_file_set_string (kf, "panel", "launcher_000003", "lxterminal.desktop");

        // write the modified key file out
        str = g_key_file_to_data (kf, &len, NULL);
        g_file_set_contents (user_file, str, len, NULL);
        g_free (str);
    }

    g_key_file_free (kf);
    g_free (user_file);
}

void add_to_launcher (const char *name)
{
    char *str, *lname;
    gsize len, i;
    int ind, max = 0;

    // construct the file path
    char *user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi.ini", NULL);

    // read in data from file to a key file
    GKeyFile *kf = g_key_file_new ();
    g_key_file_load_from_file (kf, user_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

    // find the largest index for a launcher currently in use
    gchar **keys = g_key_file_get_keys (kf, "panel", &len, NULL);
    for (i = 0; i < len; i++)
    {
        if (sscanf (keys[i], "launcher_%d", &ind) == 1)
            if (ind > max) max = ind;
    }
    g_strfreev (keys);

    // use that index to calculate the name of the new key
    lname = g_strdup_printf ("launcher_%06d", max + 1);
    g_key_file_set_string (kf, "panel", lname, name);

    // write the modified key file out
    str = g_key_file_to_data (kf, &len, NULL);
    g_file_set_contents (user_file, str, len, NULL);

    g_free (lname);
    g_free (str);
    g_key_file_free (kf);
    g_free (user_file);
}

void remove_from_launcher (const char *name)
{
    char *str = NULL;
    gsize len, i;
    int ind;

    // construct the file path
    char *user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi.ini", NULL);

    // read in data from file to a key file
    GKeyFile *kf = g_key_file_new ();
    g_key_file_load_from_file (kf, user_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

    // loop through all launcher keys looking for this desktop file
    gchar **keys = g_key_file_get_keys (kf, "panel", &len, NULL);
    for (i = 0; i < len; i++)
    {
        if (sscanf (keys[i], "launcher_%d", &ind) == 1)
        {
            gchar *val = g_key_file_get_string (kf, "panel", keys[i], NULL);
            if (!g_strcmp0 (name, val))
            {
                g_key_file_remove_key (kf, "panel", keys[i], NULL);

                // write the modified key file out
                str = g_key_file_to_data (kf, &len, NULL);
                g_file_set_contents (user_file, str, len, NULL);
                break;
            }
        }
    }
    g_strfreev (keys);

    if (str) g_free (str);
    g_key_file_free (kf);
    g_free (user_file);
}


/* End of file */
/*----------------------------------------------------------------------------*/
