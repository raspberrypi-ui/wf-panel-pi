// SPDX-License-Identifier: GPL-2.0+
/* NetworkManager Applet -- allow user control over networking
 *
 * (C) Copyright 2012 Aleksander Morgado <aleksander@gnu.org>
 */

#ifndef __APPLET_DEVICE_BROADBAND_H__
#define __APPLET_DEVICE_BROADBAND_H__

#include "applet.h"

NMADeviceClass *applet_device_broadband_get_class (NMApplet *applet);

void applet_broadband_connect_network (NMApplet *applet,
                                       NMDevice *device);

#endif /* __APPLET_DEVICE_BROADBAND_H__ */
