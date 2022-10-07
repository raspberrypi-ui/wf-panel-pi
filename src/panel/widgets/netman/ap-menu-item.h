// SPDX-License-Identifier: GPL-2.0+
/* ap-menu-item.h - Class to represent a Wifi access point 
 *
 * Jonathan Blandford <jrb@redhat.com>
 * Dan Williams <dcbw@redhat.com>
 *
 * Copyright (C) 2005 - 2008 Red Hat, Inc.
 */

#ifndef __AP_MENU_ITEM_H__
#define __AP_MENU_ITEM_H__

#include <gtk/gtk.h>
#include "applet.h"
#include "nm-access-point.h"

#define NM_TYPE_NETWORK_MENU_ITEM            (nm_network_menu_item_get_type ())
#define NM_NETWORK_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_NETWORK_MENU_ITEM, NMNetworkMenuItem))
#define NM_NETWORK_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_NETWORK_MENU_ITEM, NMNetworkMenuItemClass))
#define NM_IS_NETWORK_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_NETWORK_MENU_ITEM))
#define NM_IS_NETWORK_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NM_TYPE_NETWORK_MENU_ITEM))
#define NM_NETWORK_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_NETWORK_MENU_ITEM, NMNetworkMenuItemClass))


typedef struct _NMNetworkMenuItem	    NMNetworkMenuItem;
typedef struct _NMNetworkMenuItemClass  NMNetworkMenuItemClass;

struct _NMNetworkMenuItem {
#ifdef LXPANEL_PLUGIN
	GtkCheckMenuItem parent;
#else
	GtkMenuItem parent;
#endif
};

struct _NMNetworkMenuItemClass {
#ifdef LXPANEL_PLUGIN
	GtkCheckMenuItemClass parent_class;
#else
	GtkMenuItemClass parent_class;
#endif
};


GType	   nm_network_menu_item_get_type (void) G_GNUC_CONST;
GtkWidget* nm_network_menu_item_new (NMAccessPoint *ap,
                                     guint32 dev_caps,
                                     const char *hash,
                                     gboolean has_connections,
                                     NMApplet *applet);

#ifdef LXPANEL_PLUGIN
void nm_network_menu_item_set_hotspot (NMNetworkMenuItem *item, gboolean hotspot, NMApplet *applet);
#endif

const char *nm_network_menu_item_get_ssid (NMNetworkMenuItem *item);

gboolean   nm_network_menu_item_get_is_adhoc (NMNetworkMenuItem *item);
gboolean   nm_network_menu_item_get_is_encrypted (NMNetworkMenuItem *item);

guint32    nm_network_menu_item_get_strength (NMNetworkMenuItem *item);
void       nm_network_menu_item_set_strength (NMNetworkMenuItem *item,
                                              guint8 strength,
                                              NMApplet *applet);
const char *nm_network_menu_item_get_hash (NMNetworkMenuItem * item);

gboolean   nm_network_menu_item_find_dupe (NMNetworkMenuItem *item,
                                           NMAccessPoint *ap);

void       nm_network_menu_item_add_dupe (NMNetworkMenuItem *item,
                                          NMAccessPoint *ap);

#ifdef LXPANEL_PLUGIN
void       nm_network_menu_item_set_active (NMNetworkMenuItem * item,
                                            gboolean active,
                                            NMApplet *applet);
#else
void       nm_network_menu_item_set_active (NMNetworkMenuItem * item,
                                            gboolean active);
#endif

gboolean   nm_network_menu_item_get_has_connections (NMNetworkMenuItem *item);

#endif /* __AP_MENU_ITEM_H__ */

