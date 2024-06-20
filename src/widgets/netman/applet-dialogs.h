// SPDX-License-Identifier: GPL-2.0+
/* NetworkManager Applet -- allow user control over networking
 *
 * Dan Williams <dcbw@redhat.com>
 *
 * (C) Copyright 2008 Red Hat, Inc.
 */

#ifndef __APPLET_DIALOGS_H__
#define __APPLET_DIALOGS_H__

#include <gtk/gtk.h>

#include "applet.h"

void applet_info_dialog_show (NMApplet *applet);

void applet_about_dialog_show (NMApplet *applet);

GtkWidget *applet_missing_ui_warning_dialog_show (void);

GtkWidget *applet_mobile_password_dialog_new (NMConnection *connection,
                                              GtkEntry **out_secret_entry);

/******** Mobile PIN dialog ********/

GtkWidget  *applet_mobile_pin_dialog_new             (const char *unlock_required,
                                                      const char *device_description);
const char *applet_mobile_pin_dialog_get_entry1      (GtkWidget *dialog);
const char *applet_mobile_pin_dialog_get_entry2      (GtkWidget *dialog);
gboolean    applet_mobile_pin_dialog_get_auto_unlock (GtkWidget *dialog);
void        applet_mobile_pin_dialog_start_spinner   (GtkWidget *dialog, const char *text);
void        applet_mobile_pin_dialog_stop_spinner    (GtkWidget *dialog, const char *text);

#endif /* __APPLET_DIALOGS_H__ */
