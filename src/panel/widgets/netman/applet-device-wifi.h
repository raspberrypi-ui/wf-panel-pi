// SPDX-License-Identifier: GPL-2.0+
/* NetworkManager Applet -- allow user control over networking
 *
 * Dan Williams <dcbw@redhat.com>
 *
 * (C) Copyright 2008 Red Hat, Inc.
 */

#ifndef __APPLET_DEVICE_WIFI_H__
#define __APPLET_DEVICE_WIFI_H__

#include <gtk/gtk.h>

#include "applet.h"

NMADeviceClass *applet_device_wifi_get_class (NMApplet *applet);

void nma_menu_add_hidden_network_item (GtkWidget *menu, NMApplet *applet);
void nma_menu_add_create_network_item (GtkWidget *menu, NMApplet *applet);

#ifdef LXPANEL_PLUGIN
void nma_menu_add_create_hotspot_item (GtkWidget *menu, NMApplet *applet);
#endif
#endif /* __APPLET_DEVICE_WIFI_H__ */
