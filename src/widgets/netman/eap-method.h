// SPDX-License-Identifier: GPL-2.0+
/* NetworkManager Applet -- allow user control over networking
 *
 * Dan Williams <dcbw@redhat.com>
 *
 * Copyright 2007 - 2014 Red Hat, Inc.
 */

#ifndef EAP_METHOD_H
#define EAP_METHOD_H

void eap_method_ca_cert_ignore_save (NMConnection *connection);
void eap_method_ca_cert_ignore_load (NMConnection *connection);

#endif /* EAP_METHOD_H */
