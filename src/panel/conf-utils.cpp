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

#include "wf-option-wrap.hpp"
#include "panel.hpp"
#include "configure.h"

/*----------------------------------------------------------------------------*/
/* C++ utility functions required by configure.c */
/*----------------------------------------------------------------------------*/

extern "C" {

#include "conf-utils.h"

gboolean get_config_bool (const char *key)
{
    char *cname = g_strdup_printf ("panel/%s", key);
    WfOption <bool> bool_option {cname};
    g_free (cname);
    if (bool_option) return TRUE;
    else return FALSE;
}

int get_config_int (const char *key)
{
    char *cname = g_strdup_printf ("panel/%s", key);
    WfOption <int> int_option {cname};
    g_free (cname);
    return int_option;
}

void get_config_string (const char *key, char **dest)
{
    char *cname = g_strdup_printf ("panel/%s", key);
    WfOption <std::string> string_option {cname};
    g_free (cname);
    *dest = g_strdup_printf ("%s", ((std::string) string_option).c_str());
}

};

/* End of file */
/*----------------------------------------------------------------------------*/
