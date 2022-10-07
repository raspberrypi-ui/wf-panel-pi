// SPDX-License-Identifier: GPL-2.0+
/* NetworkManager Applet -- allow user control over networking
 *
 * Dan Williams <dcbw@redhat.com>
 *
 * (C) Copyright 2004 - 2011 Red Hat, Inc.
 */

#ifndef APPLET_VPN_REQUEST_H
#define APPLET_VPN_REQUEST_H

#include "applet.h"

size_t applet_vpn_request_get_secrets_size (void);

gboolean applet_vpn_request_get_secrets (SecretsRequest *req, GError **error);

#endif  /* APPLET_VPN_REQUEST_H */

