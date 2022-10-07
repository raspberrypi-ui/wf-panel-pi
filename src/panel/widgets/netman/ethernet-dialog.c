// SPDX-License-Identifier: GPL-2.0+
/* NetworkManager Applet -- allow user control over networking
 *
 * Dan Williams <dcbw@redhat.com>
 *
 * Copyright 2008 Novell, Inc.
 * Copyright 2008 - 2014 Red Hat, Inc.
 */

#include "nm-default.h"


#include "nma-ws.h"
#include "ethernet-dialog.h"
#include "applet-dialogs.h"
#include "eap-method.h"

static void
stuff_changed_cb (NMAWs *ws, gpointer user_data)
{
	GtkWidget *button = GTK_WIDGET (user_data);

	gtk_widget_set_sensitive (button, nma_ws_validate (ws, NULL));
}

static void
dialog_set_network_name (NMConnection *connection, GtkEntry *entry)
{
	NMSettingConnection *setting;

	setting = nm_connection_get_setting_connection (connection);

	gtk_widget_set_sensitive (GTK_WIDGET (entry), FALSE);
	gtk_entry_set_text (entry, nm_setting_connection_get_id (setting));
}

static NMAWs *
dialog_set_security (NMConnection *connection,
                     GtkBuilder *builder,
                     GtkBox *box)
{
	GList *children;
	GList *iter;
	NMAWs8021x *ws;

	ws = nma_ws_802_1x_new (connection, FALSE, TRUE);
	if (G_IS_INITIALLY_UNOWNED (ws))
		g_object_ref_sink (ws);

	/* Remove any previous wireless security widgets */
	children = gtk_container_get_children (GTK_CONTAINER (box));
	for (iter = children; iter; iter = iter->next)
		gtk_container_remove (GTK_CONTAINER (box), GTK_WIDGET (iter->data));
	g_list_free (children);

	gtk_box_pack_start (box, GTK_WIDGET (ws), TRUE, TRUE, 0);

	return NMA_WS (ws);
}

GtkWidget *
nma_ethernet_dialog_new (NMConnection *connection)
{
	GtkBuilder *builder;
	GtkWidget *dialog;
	GError *error = NULL;
	NMAWs *security;

	builder = gtk_builder_new ();

	if (!gtk_builder_add_from_resource (builder, "/org/freedesktop/network-manager-applet/8021x.ui", &error)) {
		g_warning ("Couldn't load builder resource: %s", error->message);
		g_error_free (error);
		applet_missing_ui_warning_dialog_show ();
		g_object_unref (builder);
		return NULL;
	}

	dialog = (GtkWidget *) gtk_builder_get_object (builder, "8021x_dialog");
	if (!dialog) {
		g_warning ("Couldn't find wireless_dialog widget.");
		applet_missing_ui_warning_dialog_show ();
		g_object_unref (builder);
		return NULL;
	}

	gtk_window_set_title (GTK_WINDOW (dialog), _("802.1X authentication"));
	gtk_window_set_icon_name (GTK_WINDOW (dialog), "dialog-password");
	dialog_set_network_name (connection, GTK_ENTRY (gtk_builder_get_object (builder, "network_name_entry")));

	/* Handle CA cert ignore stuff */
	eap_method_ca_cert_ignore_load (connection);

	security = dialog_set_security (connection, builder, GTK_BOX (gtk_builder_get_object (builder, "security_vbox")));
	g_signal_connect (security, "ws-changed", G_CALLBACK (stuff_changed_cb), GTK_WIDGET (gtk_builder_get_object (builder, "ok_button")));
	g_object_set_data_full (G_OBJECT (dialog),
	                        "security", security,
	                        (GDestroyNotify) g_object_unref);

	g_object_set_data_full (G_OBJECT (dialog),
	                        "connection", g_object_ref (connection),
	                        (GDestroyNotify) g_object_unref);

	/* Ensure the builder gets destroyed when the dialog goes away */
	g_object_set_data_full (G_OBJECT (dialog),
	                        "builder", builder,
	                        (GDestroyNotify) g_object_unref);

	return dialog;
}

NMConnection *
nma_ethernet_dialog_get_connection (GtkWidget *dialog)
{
	NMConnection *connection;
	NMAWs *security;

	g_return_val_if_fail (dialog != NULL, NULL);

	connection = g_object_get_data (G_OBJECT (dialog), "connection");
	security = g_object_get_data (G_OBJECT (dialog), "security");

	/* Fill up the 802.1x setting */
	nma_ws_fill_connection (security, connection);
	nm_connection_remove_setting (connection, NM_TYPE_SETTING_WIRELESS_SECURITY);

	/* Save new CA cert ignore values to GSettings */
	eap_method_ca_cert_ignore_save (connection);

	return connection;
}
