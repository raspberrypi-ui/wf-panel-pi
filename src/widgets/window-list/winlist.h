/*============================================================================
Copyright (c) 2021-2025 Raspberry Pi
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

#include <wayland-client.h>
#include <wlr-foreign-toplevel-management-unstable-v1-client-protocol.h>

/*----------------------------------------------------------------------------*/
/* Typedefs and macros                                                        */
/*----------------------------------------------------------------------------*/

#define PLUGIN_TITLE N_("Window List")

#define STATE_ACTIVATED 0x01
#define STATE_MAXIMISED 0x02
#define STATE_MINIMISED 0x04

typedef struct 
{
    GtkWidget *plugin;
    GtkWidget *box;

    GList *windows;

    int spacing;
    int max_width;
    int item_width;
    gboolean icons_only;

    GtkWidget *dragbtn;
    float drag_start;
    GdkCursor *drag;
    gboolean dragon;

    struct zwlr_foreign_toplevel_manager_v1 *manager;
    MenuCache* menu_cache;
} WinlistPlugin;

typedef struct
{
    WinlistPlugin *plugin;
    void *handle;
    void *parent;
    char *app_id;
    char *title;
    int state;
    GtkWidget *btn;
    GtkWidget *icon;
    GtkWidget *label;
    GtkGesture *gesture;
    GtkGesture *dgesture;
} WindowItem;

extern conf_table_t conf_table[4];

/*----------------------------------------------------------------------------*/
/* Prototypes                                                                 */
/*----------------------------------------------------------------------------*/

extern void wlist_init (WinlistPlugin *wl);
extern void wlist_update_display (WinlistPlugin *wl);
extern gboolean wlist_control_msg (WinlistPlugin *wl, const char *cmd);
extern void wlist_destructor (gpointer user_data);

/* End of file */
/*----------------------------------------------------------------------------*/
