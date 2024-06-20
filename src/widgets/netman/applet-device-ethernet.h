// SPDX-License-Identifier: GPL-2.0+
/* NetworkManager Applet -- allow user control over networking
 *
 * Dan Williams <dcbw@redhat.com>
 *
 * (C) Copyright 2008 Red Hat, Inc.
 * (C) Copyright 2008 Novell, Inc.
 */

#ifndef __APPLET_DEVICE_ETHERNET_H__
#define __APPLET_DEVICE_ETHERNET_H__

#include "applet.h"

NMADeviceClass *applet_device_ethernet_get_class (NMApplet *applet);

#endif /* __APPLET_DEVICE_ETHERNET_H__ */
