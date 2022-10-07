// SPDX-License-Identifier: GPL-2.0+
/* NetworkManager Applet -- allow user control over networking
 *
 * Dan Williams <dcbw@redhat.com>
 *
 * Copyright 2008 Novell, Inc.
 * Copyright 2008 - 2014 Red Hat, Inc.
 */

#ifndef ETHERNET_DIALOG_H
#define ETHERNET_DIALOG_H

#include <gtk/gtk.h>

#include <NetworkManager.h>

GtkWidget *nma_ethernet_dialog_new (NMConnection *connection);

NMConnection *nma_ethernet_dialog_get_connection (GtkWidget *dialog);

#endif /* ETHERNET_DIALOG_H */
