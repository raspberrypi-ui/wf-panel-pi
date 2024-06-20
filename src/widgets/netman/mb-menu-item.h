// SPDX-License-Identifier: GPL-2.0+
/* ap-menu-item.h - Class to represent a Wifi access point 
 *
 * Jonathan Blandford <jrb@redhat.com>
 * Dan Williams <dcbw@redhat.com>
 *
 * Copyright (C) 2005 - 2010 Red Hat, Inc.
 */

#ifndef _MB_MENU_ITEM_H_
#define _MB_MENU_ITEM_H_

#include <gtk/gtk.h>
#include "applet.h"
#include "mobile-helpers.h"

#define NM_TYPE_MB_MENU_ITEM            (nm_mb_menu_item_get_type ())
#define NM_MB_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_MB_MENU_ITEM, NMMbMenuItem))
#define NM_MB_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_MB_MENU_ITEM, NMMbMenuItemClass))
#define NM_IS_MB_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_MB_MENU_ITEM))
#define NM_IS_MB_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NM_TYPE_MB_MENU_ITEM))
#define NM_MB_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_MB_MENU_ITEM, NMMbMenuItemClass))

typedef struct {
	GtkMenuItem image_item;
} NMMbMenuItem;

typedef struct {
	GtkMenuItemClass parent_class;
} NMMbMenuItemClass;


GType	   nm_mb_menu_item_get_type (void) G_GNUC_CONST;

GtkWidget *nm_mb_menu_item_new (const char *connection_name,
                                guint32 strength,
                                const char *provider,
                                gboolean active,
                                guint32 technology,
                                guint32 state,
                                gboolean enabled,
                                NMApplet *applet);

#endif /* _MB_MENU_ITEM_H_ */

