/*
Copyright (c) 2023 Raspberry Pi
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

void add_to_launcher (const char *name)
{
    GKeyFile *kf, *kfs;
    char *str, *list, *new_list;
    gsize len;
    GError *err = NULL;

    // construct the file path
    char *user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi", "wf-panel-pi.ini", NULL);

    // read in data from file to a key file
    kf = g_key_file_new ();
    list = NULL;
    if (g_key_file_load_from_file (kf, user_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
    {
        list = g_key_file_get_string (kf, "panel", "launchers", &err);
    }

    // no launchers entry in user file - try loading from system file
    if (!list || (err && err->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND))
    {
        kfs = g_key_file_new ();
        g_key_file_load_from_file (kfs, "/etc/xdg/wf-panel-pi/wf-panel-pi.ini", G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
        list = g_key_file_get_string (kfs, "panel", "launchers", NULL);
        g_key_file_free (kfs);
    }

    // strip .desktop suffix
    str = g_strdup (name);
    *strrchr (str, '.') = 0;

    // prepend new item to list
    new_list = g_strdup_printf ("%s%s%s", str, list ? " " : "", list ? list : "");

    g_key_file_set_string (kf, "panel", "launchers", new_list);

    g_free (new_list);
    g_free (list);
    g_free (str);

    // write the modified key file out
    str = g_key_file_to_data (kf, &len, NULL);
    g_file_set_contents (user_file, str, len, NULL);

    g_free (str);
    g_key_file_free (kf);
    g_free (user_file);
}

void remove_from_launcher (const char *name)
{
    GKeyFile *kf, *kfs;
    char *str, *list, *new_list, *tok, *tmp;
    gsize len;
    GError *err = NULL;

    // construct the file path
    char *user_file = g_build_filename (g_get_user_config_dir (), "wf-panel-pi", "wf-panel-pi.ini", NULL);

    // read in data from file to a key file
    kf = g_key_file_new ();
    list = NULL;
    if (g_key_file_load_from_file (kf, user_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
    {
        list = g_key_file_get_string (kf, "panel", "launchers", &err);
    }

    // no launchers entry in user file - try loading from system file
    if (!list || (err && err->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND))
    {
        kfs = g_key_file_new ();
        g_key_file_load_from_file (kfs, "/etc/xdg/wf-panel-pi/wf-panel-pi.ini", G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
        list = g_key_file_get_string (kfs, "panel", "launchers", NULL);
        g_key_file_free (kfs);
    }

    // strip .desktop suffix
    str = g_strdup (name);
    *strrchr (str, '.') = 0;

    // loop through list, creating new list from non-matching elements
    new_list = NULL;
    tok = strtok (list, " ");
    while (tok)
    {
        if (strcmp (str, tok))
        {
            if (new_list)
            {
                tmp = g_strdup_printf ("%s %s", new_list, tok);
                g_free (new_list);
                new_list = tmp;
            }
            else new_list = g_strdup_printf ("%s", tok);
        }
        tok = strtok (NULL, " ");
    }

    g_key_file_set_string (kf, "panel", "launchers", new_list ? new_list : "");

    g_free (new_list);
    g_free (list);
    g_free (str);

    // write the modified key file out
    str = g_key_file_to_data (kf, &len, NULL);
    g_file_set_contents (user_file, str, len, NULL);

    g_free (str);
    g_key_file_free (kf);
    g_free (user_file);
}

/* End of file */
/*----------------------------------------------------------------------------*/
