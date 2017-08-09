/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2006-2010  Nokia Corporation
 *  Copyright (C) 2004-2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/*----------------------------------------------------------------------------*/
// COPYRIGHT(C) FUJITSU TEN LIMITED 2012
/*----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>

#include <glib.h>

#include <dbus/dbus.h>

#include <gdbus.h>

#include "hcid.h"
#include "dbus-common.h"
#include "log.h"
#include "adapter.h"
#include "error.h"
#include "manager.h"
#include "hcicommand.h"
#include "recorder.h"

/* Source file ID */
#define BLUEZ_SRC_ID BLUEZ_SRC_ID_SRC_MANAGER_C

/* BSM MODE */
int gBsmMode = 0;

static char base_path[50] = "/org/bluez";

static DBusConnection *connection = NULL;
static int default_adapter_id = -1;
static GSList *adapters = NULL;

#include "../audio/device.h"
#include "../audio/handsfree.h"
#include "../audio/a2dpad.h"
#include "../audio/avrcpad.h"
#include "../audio/browsing.h"
#include "../audio/pbap.h"

#define MNG_SCO_MAX			3
#define MNG_LINK_MAX		( MNG_ACL_MAX + MNG_SCO_MAX )
#define MNG_PROFILE_MAX		8			//HFP、PBAP、A2DP_CTRL、A2DP_STRM、AVRCP、BROWSING、DUN、PAN
#define MNG_SCO_LINK	0x00
#define MNG_ACL_LINK	0x01
#define MNG_ESCO_LINK	0x02
#define MNG_LINK_ERROR	0xFF
#define MNG_ROLE_MASTER		0x00	/* Role is Master */
#define MNG_ROLE_SLAVE		0x01	/* Role is Slave */
#define MNG_ROLE_UNKNOWN	0xFF	/* Role is unknown or don't care */
#define MNG_MAX_FRAME_SIZE	1028

static DBusConnection *hfp_connection = NULL;
static DBusConnection *pbap_connection = NULL;
static DBusConnection *a2dp_connection = NULL;
static DBusConnection *avrcp_connection = NULL;
static DBusConnection *browsing_connection = NULL;

typedef struct {
	uint8_t use;
	bdaddr_t bdaddr;
	uint16_t handle;
	uint16_t profile;

	uint16_t ctrl_cid;
	uint16_t strm_cid;
	uint16_t avrcp_cid;
	uint16_t brows_cid;
	uint8_t hfp_chan;
	uint8_t pbap_chan;
} manager_con_info_t;

typedef struct {
	uint8_t use;
	bdaddr_t bdaddr;
	uint16_t handle;
	uint8_t link_type;
} manager_link_info_t;

typedef struct {
	uint8_t use;
	bdaddr_t bdaddr;
} manager_reject_info_t;

typedef struct {
	uint8_t use;
	bdaddr_t bdaddr;
	uint16_t profile;
} manager_discon_info_t;

typedef struct {
	uint8_t use;
	bdaddr_t bdaddr;
	int handsfree_state;
	int pbap_state;
	int a2dpad_state;
	int avrcpad_state;
	int browsing_state;
} manager_state_info_t;

static manager_con_info_t connect_info[MNG_ACL_MAX];
static manager_link_info_t link_info[MNG_LINK_MAX];
static manager_reject_info_t reject_info[MNG_LINK_MAX];
static int manager_ACL_ope = FALSE;
static uint8_t class_of_device[3];
static manager_discon_info_t disconnect_info[MNG_ACL_MAX];
static manager_state_info_t state_info[MNG_ACL_MAX];
static int manager_ACL_guard = 0;

static DBusMessage *notice_acl_idle(DBusConnection *conn, DBusMessage *msg, void *data);
static DBusMessage *notice_acl_acceptable(DBusConnection *conn, DBusMessage *msg, void *data);
static DBusMessage *set_bsm_mode(DBusConnection *conn, DBusMessage *msg, void *data);

const char *manager_get_base_path(void)
{
	return base_path;
}

static DBusMessage *default_adapter(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	DBusMessage *reply;
	struct btd_adapter *adapter;
	const gchar *path;

	adapter = manager_find_adapter_by_id(default_adapter_id);
	if (!adapter)
		return btd_error_no_such_adapter(msg);

	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return NULL;

	path = adapter_get_path(adapter);

	dbus_message_append_args(reply, DBUS_TYPE_OBJECT_PATH, &path,
				DBUS_TYPE_INVALID);

	return reply;
}

static DBusMessage *find_adapter(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	DBusMessage *reply;
	struct btd_adapter *adapter;
	const char *pattern;
	int dev_id;
	const gchar *path;

	if (!dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &pattern,
							DBUS_TYPE_INVALID))
		return NULL;

	/* hci_devid() would make sense to use here, except it is
	 * restricted to devices which are up */
	if (!strcmp(pattern, "any") || !strcmp(pattern, "00:00:00:00:00:00")) {
		path = adapter_any_get_path();
		if (path != NULL)
			goto done;
		return btd_error_no_such_adapter(msg);
	} else if (!strncmp(pattern, "hci", 3) && strlen(pattern) >= 4) {
		dev_id = atoi(pattern + 3);
		adapter = manager_find_adapter_by_id(dev_id);
	} else {
		bdaddr_t bdaddr;
		str2ba(pattern, &bdaddr);
		adapter = manager_find_adapter(&bdaddr);
	}

	if (!adapter)
		return btd_error_no_such_adapter(msg);

	path = adapter_get_path(adapter);

done:
	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return NULL;

	dbus_message_append_args(reply, DBUS_TYPE_OBJECT_PATH, &path,
							DBUS_TYPE_INVALID);

	return reply;
}

static DBusMessage *list_adapters(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	DBusMessageIter iter;
	DBusMessageIter array_iter;
	DBusMessage *reply;
	GSList *l;

	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return NULL;

	dbus_message_iter_init_append(reply, &iter);

	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
				DBUS_TYPE_OBJECT_PATH_AS_STRING, &array_iter);

	for (l = adapters; l; l = l->next) {
		struct btd_adapter *adapter = l->data;
		const gchar *path = adapter_get_path(adapter);

		dbus_message_iter_append_basic(&array_iter,
					DBUS_TYPE_OBJECT_PATH, &path);
	}

	dbus_message_iter_close_container(&iter, &array_iter);

	return reply;
}

static DBusMessage *get_properties(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	DBusMessage *reply;
	DBusMessageIter iter;
	DBusMessageIter dict;
	GSList *list;
	char **array;
	int i;

	reply = dbus_message_new_method_return(msg);
	if (!reply)
		return NULL;

	dbus_message_iter_init_append(reply, &iter);

	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY,
			DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
			DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
			DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);

	array = g_new0(char *, g_slist_length(adapters) + 1);
	for (i = 0, list = adapters; list; list = list->next) {
		struct btd_adapter *adapter = list->data;

		array[i] = (char *) adapter_get_path(adapter);
		i++;
	}
	dict_append_array(&dict, "Adapters", DBUS_TYPE_OBJECT_PATH, &array, i);
	g_free(array);

	dbus_message_iter_close_container(&iter, &dict);

	return reply;
}

static GDBusMethodTable manager_methods[] = {
	{ "GetProperties",	"",	"a{sv}",get_properties	},
	{ "DefaultAdapter",	"",	"o",	default_adapter	},
	{ "FindAdapter",	"s",	"o",	find_adapter	},
	{ "ListAdapters",	"",	"ao",	list_adapters,
						G_DBUS_METHOD_FLAG_DEPRECATED},
	{ "NOTICE_ACL_IDLE",		"",		"", notice_acl_idle,		0,	0	},
	{ "NOTICE_ACL_ACCEPTABLE",	"qay",	"", notice_acl_acceptable,	0,	0	},
	{ "SetBsmMode",				"u",    "",	set_bsm_mode,			0,	0	},
	{ }
};

static GDBusSignalTable manager_signals[] = {
	{ "PropertyChanged",			"sv"	},
	{ "AdapterAdded",				"o"		},
	{ "AdapterRemoved",				"o"		},
	{ "DefaultAdapterChanged",		"o"		},
	{ "HCI_CMD_STATUS",				"y", 			G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "HCI_CONNECT_REQ",			"uyay", 		G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "HCI_CONNECT_COMP",			"qyyyyay", 		G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "HCI_DISCONNECT_COMP",		"qyyyay",		G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "HCI_SYNC_CONNECT_COMP",		"qqqyyyyyay",	G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "HCI_SYNC_CONNECT_CHNG",		"qqqyyyyay",	G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "HCI_EVENT_RCV",				"au",			G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "CONNECT_UNKNOWN_RFCOMM",		"qay",			G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "DISCONNECT_UNKNOWN_RFCOMM",	"qay",			G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "CONNECT_UNKNOWN_L2CAP",		"qay",			G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "DISCONNECT_UNKNOWN_L2CAP",	"qay",			G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ }
};

dbus_bool_t manager_init(DBusConnection *conn, const char *path)
{
	connection = conn;

	snprintf(base_path, sizeof(base_path), "/org/bluez/%d", getpid());

	memset( connect_info, 0, sizeof( connect_info ) );
	memset( link_info, 0, sizeof( link_info ) );
	memset( reject_info, 0, sizeof( reject_info ) );
	memset( disconnect_info, 0, sizeof( disconnect_info ) );
	memset( state_info, 0, sizeof( state_info ) );
	memset( class_of_device, 0, sizeof( class_of_device ) );
	manager_ACL_ope = FALSE;
	manager_ACL_guard = 0;

	return g_dbus_register_interface(conn, "/", MANAGER_INTERFACE,
					manager_methods, manager_signals,
					NULL, NULL, NULL);
}

static void manager_update_adapters(void)
{
	GSList *list;
	char **array;
	int i;

	array = g_new0(char *, g_slist_length(adapters) + 1);
	for (i = 0, list = adapters; list; list = list->next) {
		struct btd_adapter *adapter = list->data;

		array[i] = (char *) adapter_get_path(adapter);
		i++;
	}

	emit_array_property_changed(connection, "/",
					MANAGER_INTERFACE, "Adapters",
					DBUS_TYPE_OBJECT_PATH, &array, i);

	g_free(array);
}

static void manager_set_default_adapter(int id)
{
	struct btd_adapter *adapter;
	const gchar *path;

	default_adapter_id = id;

	adapter = manager_find_adapter_by_id(id);
	if (!adapter)
		return;

	path = adapter_get_path(adapter);

	g_dbus_emit_signal(connection, "/",
			MANAGER_INTERFACE,
			"DefaultAdapterChanged",
			DBUS_TYPE_OBJECT_PATH, &path,
			DBUS_TYPE_INVALID);
}

static void manager_remove_adapter(struct btd_adapter *adapter)
{
	uint16_t dev_id = adapter_get_dev_id(adapter);
	const gchar *path = adapter_get_path(adapter);

	adapters = g_slist_remove(adapters, adapter);

	manager_update_adapters();

	if (default_adapter_id == dev_id || default_adapter_id < 0) {
		int new_default = hci_get_route(NULL);

		manager_set_default_adapter(new_default);
	}

	g_dbus_emit_signal(connection, "/",
			MANAGER_INTERFACE, "AdapterRemoved",
			DBUS_TYPE_OBJECT_PATH, &path,
			DBUS_TYPE_INVALID);

	adapter_remove(adapter);

	if (adapters == NULL)
		btd_start_exit_timer();
}

void manager_cleanup(DBusConnection *conn, const char *path)
{
	g_slist_foreach(adapters, (GFunc) manager_remove_adapter, NULL);
	g_slist_free(adapters);

	g_dbus_unregister_interface(conn, "/", MANAGER_INTERFACE);
}

static gint adapter_id_cmp(gconstpointer a, gconstpointer b)
{
	struct btd_adapter *adapter = (struct btd_adapter *) a;
	uint16_t id = GPOINTER_TO_UINT(b);
	uint16_t dev_id = adapter_get_dev_id(adapter);

	return dev_id == id ? 0 : -1;
}

static gint adapter_cmp(gconstpointer a, gconstpointer b)
{
	struct btd_adapter *adapter = (struct btd_adapter *) a;
	const bdaddr_t *bdaddr = b;
	bdaddr_t src;

	adapter_get_address(adapter, &src);

	return bacmp(&src, bdaddr);
}

struct btd_adapter *manager_find_adapter(const bdaddr_t *sba)
{
	GSList *match;

	match = g_slist_find_custom(adapters, sba, adapter_cmp);
	if (!match)
		return NULL;

	return match->data;
}

struct btd_adapter *manager_find_adapter_by_id(int id)
{
	GSList *match;

	match = g_slist_find_custom(adapters, GINT_TO_POINTER(id),
							adapter_id_cmp);
	if (!match)
		return NULL;

	return match->data;
}

void manager_foreach_adapter(adapter_cb func, gpointer user_data)
{
	g_slist_foreach(adapters, (GFunc) func, user_data);
}

GSList *manager_get_adapters(void)
{
	return adapters;
}

void manager_add_adapter(const char *path)
{
	g_dbus_emit_signal(connection, "/",
				MANAGER_INTERFACE, "AdapterAdded",
				DBUS_TYPE_OBJECT_PATH, &path,
				DBUS_TYPE_INVALID);

	manager_update_adapters();

	btd_stop_exit_timer();
}

struct btd_adapter *btd_manager_register_adapter(int id)
{
	struct btd_adapter *adapter;
	const char *path;

	adapter = manager_find_adapter_by_id(id);
	if (adapter) {
		error("Unable to register adapter: hci%d already exist", id);
		return NULL;
	}

	adapter = adapter_create(connection, id);
	if (!adapter)
		return NULL;

	adapters = g_slist_append(adapters, adapter);

	if (!adapter_init(adapter)) {
		btd_adapter_unref(adapter);
		return NULL;
	}

	path = adapter_get_path(adapter);
	g_dbus_emit_signal(connection, "/",
				MANAGER_INTERFACE, "AdapterAdded",
				DBUS_TYPE_OBJECT_PATH, &path,
				DBUS_TYPE_INVALID);

	manager_update_adapters();

	btd_stop_exit_timer();

	if (default_adapter_id < 0)
		manager_set_default_adapter(id);

	DBG("Adapter %s registered", path);
	
	BLUEZ_DEBUG_LOG("[MGR] hcicommand_init()");
	hcicommand_init();

	return btd_adapter_ref(adapter);
}

int btd_manager_unregister_adapter(int id)
{
	struct btd_adapter *adapter;
	const gchar *path;

	adapter = manager_find_adapter_by_id(id);
	if (!adapter)
		return -1;

	path = adapter_get_path(adapter);

	info("Unregister path: %s", path);

	manager_remove_adapter(adapter);

	return 0;
}

void btd_manager_set_did(uint16_t vendor, uint16_t product, uint16_t version)
{
	GSList *l;

	for (l = adapters; l != NULL; l = g_slist_next(l)) {
		struct btd_adapter *adapter = l->data;

		btd_adapter_set_did(adapter, vendor, product, version);
	}
}

static DBusMessage *notice_acl_idle(DBusConnection *conn, DBusMessage *msg, void *data)
{
	BLUEZ_DEBUG_INFO("[MNG] notice_acl_idle() Start");

	manager_ACL_ope = FALSE;

	return dbus_message_new_method_return(msg);
}

static DBusMessage *notice_acl_acceptable(DBusConnection *conn, DBusMessage *msg, void *data)
{
	const char *bdaddr;
	uint32_t len;
	bdaddr_t dst;
	uint32_t accept;
	uint32_t class;
	uint8_t link_type = ACL_LINK;

	BLUEZ_DEBUG_INFO("[MNG] notice_acl_acceptable() Start");

	if (!dbus_message_get_args(msg, NULL,
								DBUS_TYPE_UINT16, &accept,
								DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
								&bdaddr, &len,
								DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
		return NULL;
	}
	memcpy( &dst, bdaddr, sizeof(bdaddr_t) );

	/* ACL  */
	if( FALSE == accept ) {
		/* Reject */
		BLUEZ_DEBUG_WARN("[MNG] Connection Request Reject" );
		EVENT_DRC_BLUEZ_MANAGER(FORMAT_ID_BT_MANAGER, MANAGER_ACL_REJECT, &dst, accept);
		/* CONNECTION REJECTED DUE TO LIMITED RESOURCES (0X0D) */
		bluezutil_reject_conn_req( &dst, HCI_REJECTED_LIMITED_RESOURCES );
	} else {
		/* Accept */
		BLUEZ_DEBUG_WARN("[MNG] Connection Request Accept" );
		EVENT_DRC_BLUEZ_MANAGER(FORMAT_ID_BT_MANAGER, MANAGER_ACL_ACCEPT, &dst, accept);
		bluezutil_accept_conn_req( &dst, class_of_device, link_type );
		btd_manager_set_acl_ope( TRUE );
	}

	return dbus_message_new_method_return(msg);
}

static DBusMessage *set_bsm_mode(DBusConnection *conn, DBusMessage *msg, void *data) 
{
	uint32_t mode;
	
	BLUEZ_DEBUG_INFO("[MNG] start");
	if (!dbus_message_get_args(msg, NULL,
								DBUS_TYPE_UINT32, &mode, 
								DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
		return NULL;
	}
	
	BLUEZ_DEBUG_INFO("[MNG] mode 0x%02x", mode);
	gBsmMode = mode;
	
	return dbus_message_new_method_return(msg);
}

static void btd_manager_set_link_type( uint16_t handle, bdaddr_t *bdaddr,uint8_t link_type )
{
	int cnt;

	for( cnt = 0; cnt < MNG_LINK_MAX; cnt++ ) {
		if( link_info[cnt].handle == handle && link_info[cnt].use == TRUE ) {
			BLUEZ_DEBUG_ERROR("[Bluez - MNG] The handle has already existed !!  handle = 0x%04x ", handle);
			ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, handle, 0);
			return;
		}
	}

	for( cnt = 0; cnt < MNG_LINK_MAX; cnt++ ) {
		if( link_info[cnt].use == FALSE ) {
			link_info[cnt].handle = handle;
			link_info[cnt].link_type = link_type;
			memcpy( &link_info[cnt].bdaddr, bdaddr, sizeof( bdaddr_t ) );
			link_info[cnt].use = TRUE;
			break;
		}
	}

	if( cnt == MNG_LINK_MAX ){
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] Link Num Over !!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
	}

	return;
}

static uint8_t btd_manager_find_link_info_by_handle( uint16_t handle )
{
	int cnt;

	for( cnt = 0; cnt < MNG_LINK_MAX; cnt++ ) {
		if( link_info[cnt].handle == handle && link_info[cnt].use == TRUE ) {
			break;
		}
	}

	return cnt;
}

static void btd_manager_handsfree_signal_disconnected( uint16_t value )
{
	BLUEZ_DEBUG_INFO("[MNG] btd_manager_handsfree_signal_disconnected()  value = %d", value);

	if( hfp_connection == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] hfp_connection is NULL!!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
		return;
	}

	g_dbus_emit_signal(hfp_connection, "/",
						AUDIO_HANDSFREE_INTERFACE,
						"DISCONNECTED",
						DBUS_TYPE_UINT16, &value,
						DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HFP_DISCONNECTED, value);
	return;
}

static void btd_manager_pbap_signal_disconnected( uint16_t value )
{
	BLUEZ_DEBUG_INFO("[MNG] btd_manager_pbap_signal_disconnected()  value = %d", value);

	if( pbap_connection == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] pbap_connection is NULL!!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
		return;
	}

	g_dbus_emit_signal(pbap_connection,"/",
						OBEX_PBAP_INTERFACE,
						"DISCONNECTED",
						DBUS_TYPE_UINT16, &value,
						DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_PBAP_DISCONNECTED, value);
	return;
}

static void btd_manager_a2dp_signal_disconnected( uint16_t value, uint16_t cid )
{
	BLUEZ_DEBUG_INFO("[MNG] btd_manager_a2dp_signal_disconnected()  value = %d cid = 0x%02x", value, cid);

	if( a2dp_connection == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] a2dp_connection is NULL!!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
		return;
	}

	g_dbus_emit_signal(a2dp_connection, "/",
						AUDIO_A2DPAD_INTERFACE,
						"DISCONNECTED_L2CAP",
						DBUS_TYPE_UINT16, &value,
						DBUS_TYPE_UINT16, &cid,
						DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_A2DP_DISCONNECTED, value);
	return;
}

static void btd_manager_avrcp_signal_disconnected( uint16_t value, uint16_t cid )
{
	BLUEZ_DEBUG_INFO("[MNG] btd_manager_avrcp_signal_disconnected()  value = %d cid = 0x%02x", value, cid);

	if( avrcp_connection == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] avrcp_connection is NULL!!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
		return;
	}

	g_dbus_emit_signal(avrcp_connection, "/",
						AUDIO_AVRCPAD_INTERFACE,
						"DISCONNECTED_L2CAP",
						DBUS_TYPE_UINT16, &value,
						DBUS_TYPE_UINT16, &cid,
						DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_AVRCP_DISCONNECTED, value);
	return;
}

static void btd_manager_browsing_signal_disconnected( uint16_t value, uint16_t cid )
{
	BLUEZ_DEBUG_INFO("[MNG] btd_manager_browsing_signal_disconnected()  value = %d cid = 0x%02x", value, cid);

	if( browsing_connection == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] browsing_connection is NULL!!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
		return;
	}

	g_dbus_emit_signal(browsing_connection, "/",
						AUDIO_BROWSING_INTERFACE,
						"DISCONNECTED_L2CAP",
						DBUS_TYPE_UINT16, &value,
						DBUS_TYPE_UINT16, &cid,
						DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_BROWSING_DISCONNECTED, value);
	return;
}

static int btd_manager_find_connect_info_by_handle( uint16_t handle )
{
	int cnt;

	for( cnt = 0; cnt < MNG_ACL_MAX; cnt++ ) {
		if( connect_info[cnt].handle == handle && connect_info[cnt].use == TRUE ) {
			break;
		}
	}
	return cnt;
}

static void btd_manager_error_reson( uint8_t reason )
{
	switch( reason ) {
		case 0x02:
			BLUEZ_DEBUG_WARN("[MNG] reason : Unknown Connection Identifier:0x02");
			break;
		case 0x04:
			BLUEZ_DEBUG_WARN("[MNG] reason : Page Timeout:0x04");
			break;
		case 0x05:
			BLUEZ_DEBUG_WARN("[MNG] reason : Authentication Failure:0x05");
			break;
		case 0x08:
			BLUEZ_DEBUG_WARN("[MNG] reason : Connection Timeout:0x08");
			break;
		case 0x09:
			BLUEZ_DEBUG_WARN("[MNG] reason : Connection Limit Exceeded:0x09");
			break;
		case 0x0D:
			BLUEZ_DEBUG_WARN("[MNG] reason : Connection Rejected due to Limited Resources:0x0D");
			break;
		case 0x13:
			BLUEZ_DEBUG_WARN("[MNG] reason : Remote User Terminated Connection:0x13");
			break;
		case 0x14:
			BLUEZ_DEBUG_WARN("[MNG] reason : Remote Device Terminated Connection due to Low Resources:0x14");
			break;
		case 0x15:
			BLUEZ_DEBUG_WARN("[MNG] reason : Remote Device Terminated Connection due to Power Off:0x15");
			break;
		case 0x16:
			BLUEZ_DEBUG_WARN("[MNG] reason : Connection Terminated by Local Host:0x16");
			break;
		case 0x22:
			BLUEZ_DEBUG_WARN("[MNG] reason : LMP Response Timeout/LL Response Timeout:0x22");
			break;
		default :
			BLUEZ_DEBUG_WARN("[MNG] reason : ???:0x%02x", reason);
			break;
	}
	return;
}

static void btd_manager_allprofile_disconnected_not( manager_con_info_t *pInfo )
{
	BLUEZ_DEBUG_INFO("[MNG] btd_manager_allprofile_disconnected_not() Profile = 0x%04x handle = 0x%04x", pInfo->profile, pInfo->handle);

	if( pInfo->profile & MNG_CON_HANDSFREE ) {
		btd_manager_handsfree_signal_disconnected( FALSE );
	}
	if( pInfo->profile & MNG_CON_A2DP_CTRL ) {
		btd_manager_a2dp_signal_disconnected( FALSE, pInfo->ctrl_cid );
	}
	if( pInfo->profile & MNG_CON_A2DP_STRM ) {
		btd_manager_a2dp_signal_disconnected( FALSE, pInfo->strm_cid );
	}
	if( pInfo->profile & MNG_CON_AVRCP ) {
		btd_manager_avrcp_signal_disconnected( FALSE, pInfo->avrcp_cid );
	}
	if( pInfo->profile & MNG_CON_BROWS ) {
		btd_manager_browsing_signal_disconnected( FALSE, pInfo->brows_cid );
	}
	if( pInfo->profile & MNG_CON_PBAP ) {
		btd_manager_pbap_signal_disconnected( FALSE );
	}

	memset( pInfo, 0, sizeof( manager_con_info_t ) );

	return;
}

static void btd_manager_set_disconnect_info( bdaddr_t *bdaddr, uint16_t profile )
{
	int cnt;
	int del = MNG_ACL_MAX;

	BLUEZ_DEBUG_INFO("[MNG] bdaddr = [ %02x:%02x:%02x:%02x:%02x:%02x ] profile = %04x",bdaddr->b[5],bdaddr->b[4],bdaddr->b[3],bdaddr->b[2],bdaddr->b[1],bdaddr->b[0], profile);

	for( cnt = 0; cnt < MNG_ACL_MAX; cnt++ ) {
		if( memcmp( &disconnect_info[cnt].bdaddr, bdaddr, sizeof( bdaddr_t ) ) == 0 && disconnect_info[cnt].use == TRUE ) {
			memset( &disconnect_info[cnt], 0, sizeof( manager_discon_info_t ) );
		}
		if( del == MNG_ACL_MAX && disconnect_info[cnt].use == FALSE) {
			del = cnt;
		}
	}

	if( del == MNG_ACL_MAX ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] disconnection info ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
	} else if( profile != 0 ) {
		memcpy( &disconnect_info[del].bdaddr, bdaddr, sizeof( bdaddr_t ) );
		disconnect_info[del].use = TRUE;
		disconnect_info[del].profile = profile;
	}
	return;
}

static uint16_t btd_manager_get_state_info( bdaddr_t *bdaddr )
{
	int cnt;
	uint16_t profile = 0;

	for( cnt = 0; cnt < MNG_ACL_MAX; cnt++ ) {
		if( memcmp( &state_info[cnt].bdaddr, bdaddr, sizeof( bdaddr_t ) ) == 0 && state_info[cnt].use == TRUE ) {
			break;
		}
	}

	if( cnt != MNG_ACL_MAX ) {
		if( state_info[cnt].handsfree_state > 0 ) {
			profile |= MNG_CON_HANDSFREE;
		}
		if( state_info[cnt].pbap_state > 0 ) {
			profile |= MNG_CON_PBAP;
		}
		if( state_info[cnt].a2dpad_state > 0 ) {
			profile |= MNG_CON_A2DP_CTRL;
		}
		if( state_info[cnt].avrcpad_state > 0 ) {
			profile |= MNG_CON_AVRCP;
		}
		if( state_info[cnt].browsing_state > 0 ) {
			profile |= MNG_CON_BROWS;
		}
	}

	BLUEZ_DEBUG_INFO("[MNG] bdaddr = [ %02x:%02x:%02x:%02x:%02x:%02x ] profile = %04x",bdaddr->b[5],bdaddr->b[4],bdaddr->b[3],bdaddr->b[2],bdaddr->b[1],bdaddr->b[0], profile);

	return profile;
}

uint8_t btd_manager_get_disconnect_info( bdaddr_t *bdaddr, uint16_t profile )
{
	int cnt;
	uint8_t ret = FALSE;

	for( cnt = 0; cnt < MNG_ACL_MAX; cnt++ ) {
		if( memcmp( &disconnect_info[cnt].bdaddr, bdaddr, sizeof( bdaddr_t ) ) == 0 && disconnect_info[cnt].use == TRUE ) {
			if( disconnect_info[cnt].profile & profile ) {
				ret = TRUE;
				disconnect_info[cnt].profile &= ~profile;
				if( disconnect_info[cnt].profile == 0 ) {
					memset( &disconnect_info[cnt], 0, sizeof( manager_discon_info_t ) );
				}
			}
			break;
		}
	}
	return ret;
}

void btd_manager_set_state_info( bdaddr_t *bdaddr, uint16_t profile, int state )
{
	int cnt;
	int info_no = MNG_ACL_MAX;
	int all_state = 0;

	BLUEZ_DEBUG_INFO("[MNG] START bdaddr [ %02x:%02x:%02x:%02x:%02x:%02x ] profile = %d state = %d", bdaddr->b[5], bdaddr->b[4], bdaddr->b[3], bdaddr->b[2], bdaddr->b[1], bdaddr->b[0], profile, state);

	for( cnt = 0; cnt < MNG_ACL_MAX; cnt++ ) {
		if( memcmp( &state_info[cnt].bdaddr, bdaddr, sizeof( bdaddr_t ) ) == 0 && state_info[cnt].use == TRUE ) {
			break;
		}
		if( info_no == MNG_ACL_MAX && state_info[cnt].use == FALSE ) {
			info_no = cnt;
		}
	}
	if( cnt == MNG_ACL_MAX && info_no == MNG_ACL_MAX ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] state_info Overflow");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
		return;
	}

	if( cnt != MNG_ACL_MAX ) {
		info_no = cnt;
	}

	switch( profile ) {
	case MNG_CON_HANDSFREE:
		state_info[info_no].handsfree_state = state;
		break;
	case MNG_CON_PBAP:
		state_info[info_no].pbap_state = state;
		break;
	case MNG_CON_A2DP_CTRL:
		state_info[info_no].a2dpad_state = state;
		break;
	case MNG_CON_A2DP_STRM:
		state_info[info_no].a2dpad_state = state;
		break;
	case MNG_CON_AVRCP:
		state_info[info_no].avrcpad_state = state;
		break;
	case MNG_CON_BROWS:
		state_info[info_no].browsing_state = state;
		break;
	default:
		break;
	}

	all_state = state_info[info_no].handsfree_state +
				state_info[info_no].pbap_state +
				state_info[info_no].a2dpad_state +
				state_info[info_no].avrcpad_state +
				state_info[info_no].browsing_state;

	if( all_state == 0 ) {
		/* 全て切断済みなので削除 */
		BLUEZ_DEBUG_LOG("[MNG] state_info clear");
		memset( &state_info[info_no], 0, sizeof( manager_state_info_t ) );
	} else {
		memcpy( &state_info[info_no].bdaddr, bdaddr, sizeof( bdaddr_t ) );
		state_info[info_no].use = TRUE;
	}

	return;
}

void btd_manager_handsfree_signal_disconnect_cfm( uint16_t value )
{
	BLUEZ_DEBUG_INFO("[MNG] btd_manager_handsfree_signal_disconnect_cfm()  value = %d", value);

	if( hfp_connection == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] hfp_connection is NULL!!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
		return;
	}

	g_dbus_emit_signal(hfp_connection, "/",
						AUDIO_HANDSFREE_INTERFACE,
						"DISCONNECT_CFM",
						DBUS_TYPE_UINT16, &value,
						DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HFP_DISCONNECT_CFM, value);

	return;
}

void btd_manager_pbap_signal_disconnect_cfm( uint16_t value )
{
	BLUEZ_DEBUG_INFO("[MNG] btd_manager_pbap_signal_disconnect_cfm()  value = %d", value);

	if( pbap_connection == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] pbap_connection is NULL!!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
		return;
	}

	g_dbus_emit_signal(pbap_connection, "/",
						OBEX_PBAP_INTERFACE,
						"DISCONNECT_CFM",
						DBUS_TYPE_UINT16, &value,
						DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_PBAP_DISCONNECT_CFM, value);
	return;
}

void btd_manager_a2dp_signal_disconnect_cfm( uint16_t value, uint16_t cid )
{
	BLUEZ_DEBUG_INFO("[MNG] btd_manager_a2dp_signal_disconnect_cfm()  value = %d cid = 0x%02x", value, cid);

	if( a2dp_connection == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] a2dp_connection is NULL!!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
		return;
	}

	g_dbus_emit_signal(a2dp_connection, "/",
						AUDIO_A2DPAD_INTERFACE,
						"DISCONNECT_CFM_L2CAP",
						DBUS_TYPE_UINT16, &value,
						DBUS_TYPE_UINT16, &cid,
						DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_A2DP_DISCONNECT_CFM, value);
	return;
}

void btd_manager_avrcp_signal_disconnect_cfm( uint16_t value, uint16_t cid )
{
	BLUEZ_DEBUG_INFO("[MNG] btd_manager_avrcp_signal_disconnect_cfm()  value = %d cid = 0x%02x", value, cid);

	if( avrcp_connection == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] avrcp_connection is NULL!!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
		return;
	}

	g_dbus_emit_signal(avrcp_connection, "/",
						AUDIO_AVRCPAD_INTERFACE,
						"DISCONNECT_CFM_L2CAP",
						DBUS_TYPE_UINT16, &value,
						DBUS_TYPE_UINT16, &cid,
						DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_AVRCP_DISCONNECT_CFM, value);
	return;
}

void btd_manager_browsing_signal_disconnect_cfm( uint16_t value, uint16_t cid )
{
	BLUEZ_DEBUG_INFO("[MNG] btd_manager_browsing_signal_disconnect_cfm()  value = %d cid = 0x%02x", value, cid);

	if( browsing_connection == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] browsing_connection is NULL!!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
		return;
	}

	g_dbus_emit_signal(browsing_connection, "/",
						AUDIO_BROWSING_INTERFACE,
						"DISCONNECT_CFM_L2CAP",
						DBUS_TYPE_UINT16, &value,
						DBUS_TYPE_UINT16, &cid,
						DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_BROWSING_DISCONNECT_CFM, value);
	return;
}

int btd_manager_find_connect_info_by_bdaddr( bdaddr_t *bdaddr )
{
	int cnt;

	for( cnt = 0; cnt < MNG_ACL_MAX; cnt++ ) {
		if( memcmp( &connect_info[cnt].bdaddr, bdaddr, sizeof( bdaddr_t ) ) == 0 && connect_info[cnt].use == TRUE ) {
			break;
		}
	}
	return cnt;
}

void btd_manager_receive_cmd_status( uint8_t status, uint8_t ncmd, uint16_t opcode )
{
	BLUEZ_DEBUG_WARN("[MNG] HCI_CMD_STATUS : status = 0x%x opcode = 0x%04x", status, opcode);

	g_dbus_emit_signal(connection, "/",
					MANAGER_INTERFACE,
					"HCI_CMD_STATUS",
					DBUS_TYPE_BYTE, &status,
					DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_HCI_SIGNAL(FORMAT_ID_DBUS_HCI_SIGNAL, SIGNAL_HCI_CMD_STATUS, 0, status);

	/* ACL 確立までのガード */
	btd_manager_set_acl_guard( TRUE );

	return;
}

void btd_manager_receive_conn_request( bdaddr_t *bdaddr, uint8_t *cod, uint8_t linktype )
{

	char  BdAddr[BDADDR_SIZE];
	char *pBdAddr;
	uint32_t class = cod[0] | (cod[1] << 8)
				| (cod[2] << 16);

	memcpy( BdAddr, bdaddr, sizeof( BdAddr ) );
	pBdAddr = &BdAddr[0];

	BLUEZ_DEBUG_WARN("[MNG] HCI_CONNECT_REQ : cod = 0x%0x linktype = %d  bdaddr =[ %02x:%02x:%02x:%02x:%02x:%02x ]", class, linktype, BdAddr[5], BdAddr[4], BdAddr[3], BdAddr[2], BdAddr[1], BdAddr[0]);

	g_dbus_emit_signal(connection, "/",
					MANAGER_INTERFACE,
					"HCI_CONNECT_REQ",
					DBUS_TYPE_UINT32, &class,
					DBUS_TYPE_BYTE, &linktype,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&pBdAddr, BDADDR_SIZE,
					DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_HCI_SIGNAL(FORMAT_ID_DBUS_HCI_SIGNAL, SIGNAL_HCI_CONNECT_REQ, linktype, 0);

	memcpy( &class_of_device, cod, sizeof( class_of_device ) );

	return;
}

void btd_manager_receive_conn_complete( uint8_t status, uint16_t handle, bdaddr_t *bdaddr,
										uint8_t link_type, uint8_t encr_mode )
{
	int cnt;
	char addr[BDADDR_SIZE] = {0};
	char *paddr;
	char role = MNG_ROLE_UNKNOWN;

	BLUEZ_DEBUG_WARN("[MNG] HCI_CONNECT_COMP : handle = 0x%04x link_type = 0x%02x status = 0x%02x bdaddr =[ %02x:%02x:%02x:%02x:%02x:%02x ]", handle, link_type, status, bdaddr->b[5], bdaddr->b[4], bdaddr->b[3], bdaddr->b[2], bdaddr->b[1], bdaddr->b[0]);

	memcpy( addr, bdaddr, sizeof( addr ) );
	paddr = &addr[0];

	g_dbus_emit_signal(connection, "/",
					MANAGER_INTERFACE,
					"HCI_CONNECT_COMP",
					DBUS_TYPE_UINT16, &handle,
					DBUS_TYPE_BYTE, &status,
					DBUS_TYPE_BYTE, &link_type,
					DBUS_TYPE_BYTE, &encr_mode,
					DBUS_TYPE_BYTE, &role,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&paddr, BDADDR_SIZE,
					DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_HCI_SIGNAL(FORMAT_ID_DBUS_HCI_SIGNAL, SIGNAL_HCI_CONNECT_COMP, link_type, status);

	if( status != 0x00 ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] Connection Complete ERROR : Status = 0x%02x ", status);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, status, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_SRC_MNG, __LINE__, bdaddr);
		btd_manager_error_reson( status );
		if ( link_type == MNG_ACL_LINK ) {
			/* ACL 確立ガード解除 */
			btd_manager_set_acl_guard( FALSE );
		}
		return;
	}

	btd_manager_set_link_type( handle, bdaddr, link_type );

	if ( link_type != MNG_ACL_LINK ) {
		return;
	}
	/* ACL 確立ガード解除 */
	btd_manager_set_acl_guard( FALSE );

	for( cnt = 0; cnt < MNG_ACL_MAX; cnt++ ) {
		if( connect_info[cnt].handle == handle && connect_info[cnt].use == TRUE ) {
			BLUEZ_DEBUG_ERROR("[Bluez - MNG] The handle has already existed !!  handle = 0x%04x ", handle);
			ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, handle, 0);
			EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_SRC_MNG, __LINE__, bdaddr);
			return;
		}
	}

	for( cnt = 0; cnt < MNG_ACL_MAX; cnt++ ) {
		if( connect_info[cnt].use == FALSE ) {
			BLUEZ_DEBUG_LOG("[MNG] New handle = 0x%04x bdaddr =[ %02x:%02x:%02x:%02x:%02x:%02x ]", handle, addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
			connect_info[cnt].handle = handle;
			memcpy( &connect_info[cnt].bdaddr, bdaddr, sizeof( bdaddr_t ) );
			connect_info[cnt].profile = 0;
			connect_info[cnt].use = TRUE;
			break;
		}
	}

	if( cnt == MNG_ACL_MAX ){
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] ACL Link Num Over !!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_SRC_MNG, __LINE__, bdaddr);
	}

	return;
}

void btd_manager_fake_signal_conn_complete( bdaddr_t *bdaddr )
{
	uint8_t status = 0x04;
	uint16_t handle = 0;
	uint8_t link_type = MNG_ACL_LINK;
	uint8_t encr_mode = 0;
	char addr[BDADDR_SIZE] = {0};
	char *paddr;
	char role = MNG_ROLE_UNKNOWN;

	BLUEZ_DEBUG_WARN("[MNG] FAKE HCI_CONNECT_COMP : handle = 0x%04x link_type = 0x%02x status = 0x%02x bdaddr =[ %02x:%02x:%02x:%02x:%02x:%02x ]", handle, link_type, status, bdaddr->b[5], bdaddr->b[4], bdaddr->b[3], bdaddr->b[2], bdaddr->b[1], bdaddr->b[0]);

	memcpy( addr, bdaddr, sizeof( addr ) );
	paddr = &addr[0];

	g_dbus_emit_signal(connection, "/",
					MANAGER_INTERFACE,
					"HCI_CONNECT_COMP",
					DBUS_TYPE_UINT16, &handle,
					DBUS_TYPE_BYTE, &status,
					DBUS_TYPE_BYTE, &link_type,
					DBUS_TYPE_BYTE, &encr_mode,
					DBUS_TYPE_BYTE, &role,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&paddr, BDADDR_SIZE,
					DBUS_TYPE_INVALID);

	EVENT_DRC_BLUEZ_HCI_SIGNAL(FORMAT_ID_DBUS_HCI_SIGNAL, SIGNAL_HCI_CONNECT_COMP, link_type, status);

	return;
}

void btd_manager_receive_disconn_complete( uint8_t status, uint16_t handle, uint8_t reason )
{
	int link_no;
	int info_no;
	manager_con_info_t *pInfo;
	manager_link_info_t *pLink;
	uint8_t link_type = MNG_LINK_ERROR;
	char addr[BDADDR_SIZE] = {0};
	char *paddr;
	bdaddr_t bdaddr;
	uint16_t profile = 0;

	memset( &bdaddr, 0, sizeof( bdaddr_t ) );
	link_no = btd_manager_find_link_info_by_handle( handle );
	if( link_no == MNG_LINK_MAX ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] Link is not found.");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
	} else {
		pLink = &link_info[link_no];
		memcpy( addr, &pLink->bdaddr, sizeof( addr ) );
		memcpy( &bdaddr, &pLink->bdaddr, sizeof( bdaddr ) );
		if ( pLink->link_type == MNG_ESCO_LINK ) {
			link_type = MNG_SCO_LINK;
		} else {
			link_type = pLink->link_type;
		}
		memset( pLink, 0, sizeof( manager_link_info_t ) );
	}
	paddr = &addr[0];

	BLUEZ_DEBUG_WARN("[MNG] HCI_DISCONNECT_COMP : handle = 0x%04x link_type = 0x%02x status = 0x%02x reason = 0x%02x bdaddr =[ %02x:%02x:%02x:%02x:%02x:%02x ]", handle, link_type, status, reason, bdaddr.b[5], bdaddr.b[4], bdaddr.b[3], bdaddr.b[2], bdaddr.b[1], bdaddr.b[0]);
	btd_manager_error_reson( reason );

	g_dbus_emit_signal(connection, "/",
					MANAGER_INTERFACE,
					"HCI_DISCONNECT_COMP",
					DBUS_TYPE_UINT16, &handle,
					DBUS_TYPE_BYTE, &status,
					DBUS_TYPE_BYTE, &reason,
					DBUS_TYPE_BYTE, &link_type,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&paddr, BDADDR_SIZE,
					DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_HCI_SIGNAL(FORMAT_ID_DBUS_HCI_SIGNAL, SIGNAL_HCI_DISCONNECT_COMP, link_type, status);

	if( link_type != MNG_ACL_LINK ) {
		return;
	}

	/* 接続している profile に disconnected を投げる */
	info_no = btd_manager_find_connect_info_by_handle( handle );
	if( info_no == MNG_ACL_MAX ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] A handle is not found.");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
	} else {
		pInfo = &connect_info[info_no];
		profile = btd_manager_get_state_info( &bdaddr );
		profile &= ~pInfo->profile;
		btd_manager_allprofile_disconnected_not( pInfo );
	}
	/* 接続中の場合のため、情報を残す */
	btd_manager_set_disconnect_info( &bdaddr, profile );

	return;
}

void btd_manager_receive_sync_conn_complete( uint8_t status, uint16_t handle, bdaddr_t *bdaddr,
											 uint8_t link_type, uint8_t trans_interval, 
											 uint8_t retrans_window, uint16_t rx_pkt_len, 
											 uint16_t tx_pkt_len, uint8_t air_mode )
{
	char  addr[BDADDR_SIZE] = {0};
	char *paddr;

	BLUEZ_DEBUG_WARN("[MNG] HCI_SYNC_CONNECT_COMP : handle = 0x%04x link_type = 0x%02x status = 0x%02x", handle, link_type, status);

	memcpy( addr, bdaddr, sizeof( addr ) );
	paddr = &addr[0];

	g_dbus_emit_signal(connection, "/",
					MANAGER_INTERFACE,
					"HCI_SYNC_CONNECT_COMP",
					DBUS_TYPE_UINT16, &handle,
					DBUS_TYPE_UINT16, &rx_pkt_len,
					DBUS_TYPE_UINT16, &tx_pkt_len,
					DBUS_TYPE_BYTE, &status,
					DBUS_TYPE_BYTE, &link_type,
					DBUS_TYPE_BYTE, &trans_interval,
					DBUS_TYPE_BYTE, &retrans_window,
					DBUS_TYPE_BYTE, &air_mode,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&paddr, BDADDR_SIZE,
					DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_HCI_SIGNAL(FORMAT_ID_DBUS_HCI_SIGNAL, SIGNAL_HCI_SYNC_CONNECT_COMP, link_type, status);

	if( status != 0x00 ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] Sync Connection Complete ERROR : Status = 0x%02x ", status);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, status, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_SRC_MNG, __LINE__, bdaddr);
		return;
	}

	btd_manager_set_link_type( handle, bdaddr, link_type );

	return;
}

void btd_manager_receive_sync_conn_changed( uint8_t status, uint16_t handle, 
											uint8_t trans_interval, uint8_t retrans_window, 
											uint16_t rx_pkt_len, uint16_t tx_pkt_len )
{
	int link_no;
	manager_link_info_t *pLink;
	char  addr[BDADDR_SIZE] = {0};
	char *paddr;
	uint8_t link_type = MNG_LINK_ERROR;


	link_no = btd_manager_find_link_info_by_handle( handle );
	if( link_no == MNG_LINK_MAX ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] Link is not found.");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
	} else {
		pLink = &link_info[link_no];
		memcpy( addr, &pLink->bdaddr, sizeof( addr ) );
		link_type = pLink->link_type;
	}
	paddr = &addr[0];

	BLUEZ_DEBUG_WARN("[MNG] HCI_SYNC_CONNECT_CHNG : handle = 0x%04x link_type = 0x%02x status = 0x%02x", handle, link_type, status);

	g_dbus_emit_signal(connection, "/",
					MANAGER_INTERFACE,
					"HCI_SYNC_CONNECT_CHNG",
					DBUS_TYPE_UINT16, &handle,
					DBUS_TYPE_UINT16, &rx_pkt_len,
					DBUS_TYPE_UINT16, &tx_pkt_len,
					DBUS_TYPE_BYTE, &status,
					DBUS_TYPE_BYTE, &link_type,
					DBUS_TYPE_BYTE, &trans_interval,
					DBUS_TYPE_BYTE, &retrans_window,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&paddr, BDADDR_SIZE,
					DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_HCI_SIGNAL(FORMAT_ID_DBUS_HCI_SIGNAL, SIGNAL_HCI_SYNC_CONNECT_CHNG, link_type, status);

	return;
}

void btd_manager_receive_hci_event_packet( uint8_t * data, int len )
{
	int cnt;
	char packet[MNG_MAX_FRAME_SIZE] = {0};
	char *ppacket;
	char role = MNG_ROLE_UNKNOWN;
	uint32_t length;

	memcpy( packet, data, len );
	length = (uint32_t)len;
	ppacket = &packet[0];

	BLUEZ_DEBUG_WARN("[MNG] HCI_EVENT_RCV : len = %d", len);

	g_dbus_emit_signal(connection, "/",
					MANAGER_INTERFACE,
					"HCI_EVENT_RCV",
					DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
					&ppacket, length,
					DBUS_TYPE_INVALID);

	return;
}


void btd_manager_connect_profile( bdaddr_t *bdaddr, uint16_t profile, uint8_t chan, uint16_t cid )
{
	int info_no;
	manager_con_info_t *pInfo;

	BLUEZ_DEBUG_INFO("[MNG] btd_manager_connect_profile() Start");

	info_no = btd_manager_find_connect_info_by_bdaddr( bdaddr );

	if( info_no == MNG_ACL_MAX ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] A handle is not found.  profile = %04x", profile);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, profile, 0);
	} else {
		pInfo = &connect_info[info_no];
		BLUEZ_DEBUG_LOG("[MNG] The handle has existed.  handle = 0x%04x profile = %04x", pInfo->handle, profile);
		if( pInfo->profile & profile ) {
			BLUEZ_DEBUG_ERROR("[Bluez - MNG] Profile has already connected !!  profile = %04x", profile);
			ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, profile, 0);
		} else {
			pInfo->profile |=  profile;
			BLUEZ_DEBUG_LOG("[MNG] profile = %04x chan = 0x%02x cid = 0x%04x", pInfo->profile, chan, cid);
			switch( profile ) {
			case MNG_CON_HANDSFREE:
				pInfo->hfp_chan = chan;
				break;
			case MNG_CON_PBAP:
				pInfo->pbap_chan = chan;
				break;
			case MNG_CON_A2DP_CTRL:
				pInfo->ctrl_cid = cid;
				break;
			case MNG_CON_A2DP_STRM:
				pInfo->strm_cid = cid;
				break;
			case MNG_CON_AVRCP:
				pInfo->avrcp_cid = cid;
				break;
			case MNG_CON_BROWS:
				pInfo->brows_cid = cid;
				break;
			default:
				break;
			}
		}
	}

	BLUEZ_DEBUG_INFO("[MNG] btd_manager_connect_profile() End");
	return;
}

uint16_t btd_manager_judgment_rfcomm( bdaddr_t *bdaddr, uint8_t chan, uint8_t dir )
{
	int info_no;
	manager_con_info_t *pInfo;
	uint16_t profile = 0;

	info_no = btd_manager_find_connect_info_by_bdaddr( bdaddr );
	if( info_no == MNG_ACL_MAX ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] A handle is not found !!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
	} else {
		pInfo = &connect_info[info_no];

		if( pInfo->hfp_chan == chan && pInfo->pbap_chan == chan && dir == 1 ) {
			profile =  MNG_CON_PBAP;
		} else if ( pInfo->hfp_chan == chan && pInfo->pbap_chan == chan && dir == 0 ) {
			profile =  MNG_CON_HANDSFREE;
		} else if ( pInfo->hfp_chan == chan ) {
			profile =  MNG_CON_HANDSFREE;
		} else if ( pInfo->pbap_chan == chan ) {
			profile =  MNG_CON_PBAP;
		} else {
			BLUEZ_DEBUG_WARN("[MNG] profile does not understand.");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
			profile = 0;
		}
	}
	return profile;
}

void btd_manager_disconnect_cfm_profile_rfcomm( bdaddr_t *bdaddr, uint16_t value, uint8_t chan, uint16_t profile )
{
	int info_no;
	manager_con_info_t *pInfo;

	info_no = btd_manager_find_connect_info_by_bdaddr( bdaddr );
	if( info_no == MNG_ACL_MAX ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] A handle is not found !!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
	} else {
		pInfo = &connect_info[info_no];

		if( pInfo->hfp_chan == chan && profile == MNG_CON_HANDSFREE ) {
			BLUEZ_DEBUG_WARN("[MNG] MNG_CON_HANDSFREE btd_manager_handsfree_signal_disconnect_cfm() value = 0x%04x", value );
			btd_manager_handsfree_signal_disconnect_cfm( value );
			pInfo->hfp_chan = 0;
			pInfo->profile &= ~MNG_CON_HANDSFREE;
		} else if( pInfo->pbap_chan == chan && profile == MNG_CON_PBAP ) {
			BLUEZ_DEBUG_WARN("[MNG] MNG_CON_PBAP btd_manager_pbap_signal_disconnect_cfm() value = 0x%04x", value );
			btd_manager_pbap_signal_disconnect_cfm( value );
			pInfo->pbap_chan = 0;
			pInfo->profile &= ~MNG_CON_PBAP;
		} else {
			BLUEZ_DEBUG_WARN("[MNG] Channel is not connect chan = 0x%02x", chan);
			EVENT_DRC_BLUEZ_MANAGER(FORMAT_ID_BT_MANAGER, MANAGER_DISC_CFM_CHAN, bdaddr, chan);
		}
	}
	return;
}

void btd_manager_disconnected_profile_rfcomm( bdaddr_t *bdaddr, uint16_t value, uint8_t chan, uint16_t profile )
{
	int info_no;
	manager_con_info_t *pInfo;

	info_no = btd_manager_find_connect_info_by_bdaddr( bdaddr );
	if( info_no == MNG_ACL_MAX ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] A handle is not found !!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
	} else {
		pInfo = &connect_info[info_no];

		if( pInfo->hfp_chan == chan && profile == MNG_CON_HANDSFREE ) {
			BLUEZ_DEBUG_WARN("[MNG] MNG_CON_HANDSFREE btd_manager_handsfree_signal_disconnected() value = 0x%04x", value );
			btd_manager_handsfree_signal_disconnected( value );
			pInfo->hfp_chan = 0;
			pInfo->profile &= ~MNG_CON_HANDSFREE;
		} else if( pInfo->pbap_chan == chan && profile == MNG_CON_PBAP ) {
			BLUEZ_DEBUG_WARN("[MNG] MNG_CON_PBAP btd_manager_pbap_signal_disconnected() value = 0x%04x", value );
			btd_manager_pbap_signal_disconnected( value );
			pInfo->pbap_chan = 0;
			pInfo->profile &= ~MNG_CON_PBAP;
		} else {
			BLUEZ_DEBUG_WARN("[MNG] Channel is not connect chan = 0x%02x", chan);
			EVENT_DRC_BLUEZ_MANAGER(FORMAT_ID_BT_MANAGER, MANAGER_DISCONNECTED_CHAN, bdaddr, chan);
		}
	}
	return;
}

void btd_manager_disconnect_cfm_profile_l2cap( bdaddr_t *bdaddr, uint16_t value, uint16_t cid )
{
	int info_no;
	manager_con_info_t *pInfo;

	info_no = btd_manager_find_connect_info_by_bdaddr( bdaddr );
	if( info_no == MNG_ACL_MAX ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] A handle is not found !!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
	} else {
		pInfo = &connect_info[info_no];

		if( pInfo->ctrl_cid == cid ) {
			BLUEZ_DEBUG_WARN("[MNG] MNG_CON_A2DP_CTRL btd_manager_a2dp_signal_disconnect_cfm() value = 0x%04x", value );
			btd_manager_a2dp_signal_disconnect_cfm( value, cid );
			pInfo->ctrl_cid = 0;
			pInfo->profile &= ~MNG_CON_A2DP_CTRL;
		} else if( pInfo->strm_cid == cid ) {
			BLUEZ_DEBUG_WARN("[MNG] MNG_CON_A2DP_STRM btd_manager_a2dp_signal_disconnect_cfm() value = 0x%04x", value );
			btd_manager_a2dp_signal_disconnect_cfm( value, cid );
			pInfo->strm_cid = 0;
			pInfo->profile &= ~MNG_CON_A2DP_STRM;
		} else if( pInfo->avrcp_cid == cid ) {
			BLUEZ_DEBUG_WARN("[MNG] MNG_CON_AVRCP btd_manager_avrcp_signal_disconnect_cfm() value = 0x%04x", value );
			btd_manager_avrcp_signal_disconnect_cfm( value, cid );
			pInfo->avrcp_cid = 0;
			pInfo->profile &= ~MNG_CON_AVRCP;
		} else if( pInfo->brows_cid == cid ) {
			BLUEZ_DEBUG_WARN("[MNG] MNG_CON_BROWS btd_manager_browsing_signal_disconnect_cfm() value = 0x%04x", value );
			btd_manager_browsing_signal_disconnect_cfm( value, cid );
			pInfo->brows_cid = 0;
			pInfo->profile &= ~MNG_CON_BROWS;
		} else {
			BLUEZ_DEBUG_WARN("[MNG] cid is not connect  cid = 0x%04x", cid);
			EVENT_DRC_BLUEZ_MANAGER(FORMAT_ID_BT_MANAGER, MANAGER_DISC_CFM_LCAP, bdaddr, cid);
		}
	}
	return;
}

void btd_manager_disconnected_profile_l2cap( bdaddr_t *bdaddr, uint16_t value, uint16_t cid )
{
	int info_no;
	manager_con_info_t *pInfo;

	info_no = btd_manager_find_connect_info_by_bdaddr( bdaddr );
	if( info_no == MNG_ACL_MAX ) {
		BLUEZ_DEBUG_ERROR("[Bluez - MNG] A handle is not found !!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SRC_MNG, 0, 0);
	} else {
		pInfo = &connect_info[info_no];

		if( pInfo->ctrl_cid == cid ) {
			BLUEZ_DEBUG_WARN("[MNG] MNG_CON_A2DP_CTRL btd_manager_a2dp_signal_disconnected() value = 0x%04x", value );
			btd_manager_a2dp_signal_disconnected( value, cid );
			pInfo->ctrl_cid = 0;
			pInfo->profile &= ~MNG_CON_A2DP_CTRL;
		} else if( pInfo->strm_cid == cid ) {
			BLUEZ_DEBUG_WARN("[MNG] MNG_CON_A2DP_STRM btd_manager_a2dp_signal_disconnected() value = 0x%04x", value );
			btd_manager_a2dp_signal_disconnected( value, cid );
			pInfo->strm_cid = 0;
			pInfo->profile &= ~MNG_CON_A2DP_STRM;
		} else if( pInfo->avrcp_cid == cid ) {
			BLUEZ_DEBUG_WARN("[MNG] MNG_CON_AVRCP btd_manager_avrcp_signal_disconnected() value = 0x%04x", value );
			btd_manager_avrcp_signal_disconnected( value, cid );
			pInfo->avrcp_cid = 0;
			pInfo->profile &= ~MNG_CON_AVRCP;
		} else if( pInfo->brows_cid == cid ) {
			BLUEZ_DEBUG_WARN("[MNG] MNG_CON_BROWS btd_manager_browsing_signal_disconnected() value = 0x%04x", value );
			btd_manager_browsing_signal_disconnected( value, cid );
			pInfo->brows_cid = 0;
			pInfo->profile &= ~MNG_CON_BROWS;
		} else {
			BLUEZ_DEBUG_WARN("[MNG] cid is not connect  cid = 0x%04x", cid);
			EVENT_DRC_BLUEZ_MANAGER(FORMAT_ID_BT_MANAGER, MANAGER_DISCONNECTED_L2CAP, bdaddr, cid);
		}
	}
	return;
}

void btd_manager_connect_unknown_profile_rfcomm( bdaddr_t *bdaddr, uint16_t cid )
{
	char  addr[BDADDR_SIZE] = {0};
	char *paddr;

	BLUEZ_DEBUG_WARN("[MNG] CONNECT_UNKNOWN_RFCOMM : cid = 0x%02x bdaddr =[ %02x:%02x:%02x:%02x:%02x:%02x ]", cid, bdaddr->b[5], bdaddr->b[4], bdaddr->b[3], bdaddr->b[2], bdaddr->b[1], bdaddr->b[0]);

	memcpy( addr, bdaddr, sizeof( addr ) );
	paddr = &addr[0];

	g_dbus_emit_signal(connection, "/",
					MANAGER_INTERFACE,
					"CONNECT_UNKNOWN_RFCOMM",
					DBUS_TYPE_UINT16, &cid,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&paddr, BDADDR_SIZE,
					DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_UNKNOWN_RFCOMM_CONNECT, cid);

	return;
}

void btd_manager_disconnect_unknown_profile_rfcomm( bdaddr_t *bdaddr, uint16_t cid )
{
	char  addr[BDADDR_SIZE] = {0};
	char *paddr;

	BLUEZ_DEBUG_WARN("[MNG] DISCONNECT_UNKNOWN_RFCOMM : cid = 0x%02x bdaddr =[ %02x:%02x:%02x:%02x:%02x:%02x ]", cid, bdaddr->b[5], bdaddr->b[4], bdaddr->b[3], bdaddr->b[2], bdaddr->b[1], bdaddr->b[0]);

	memcpy( addr, bdaddr, sizeof( addr ) );
	paddr = &addr[0];

	g_dbus_emit_signal(connection, "/",
					MANAGER_INTERFACE,
					"DISCONNECT_UNKNOWN_RFCOMM",
					DBUS_TYPE_UINT16, &cid,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&paddr, BDADDR_SIZE,
					DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_UNKNOWN_RFCOMM_DISCONNECT, cid);

	return;
}


void btd_manager_connect_unknown_profile_l2cap( bdaddr_t *bdaddr, uint16_t psm )
{
	char  addr[BDADDR_SIZE] = {0};
	char *paddr;

	BLUEZ_DEBUG_WARN("[MNG] CONNECT_UNKNOWN_L2CAP : psm = 0x%02x bdaddr =[ %02x:%02x:%02x:%02x:%02x:%02x ]", psm, bdaddr->b[5], bdaddr->b[4], bdaddr->b[3], bdaddr->b[2], bdaddr->b[1], bdaddr->b[0]);

	memcpy( addr, bdaddr, sizeof( addr ) );
	paddr = &addr[0];

	g_dbus_emit_signal(connection, "/",
					MANAGER_INTERFACE,
					"CONNECT_UNKNOWN_L2CAP",
					DBUS_TYPE_UINT16, &psm,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&paddr, BDADDR_SIZE,
					DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_UNKNOWN_L2CAP_CONNECT, psm);

	return;
}

void btd_manager_disconnect_unknown_profile_l2cap( bdaddr_t *bdaddr, uint16_t psm )
{
	char  addr[BDADDR_SIZE] = {0};
	char *paddr;

	BLUEZ_DEBUG_WARN("[MNG] DISCONNECT_UNKNOWN_L2CAP : psm = 0x%02x bdaddr =[ %02x:%02x:%02x:%02x:%02x:%02x ]", psm, bdaddr->b[5], bdaddr->b[4], bdaddr->b[3], bdaddr->b[2], bdaddr->b[1], bdaddr->b[0]);

	memcpy( addr, bdaddr, sizeof( addr ) );
	paddr = &addr[0];

	g_dbus_emit_signal(connection, "/",
					MANAGER_INTERFACE,
					"DISCONNECT_UNKNOWN_L2CAP",
					DBUS_TYPE_UINT16, &psm,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&paddr, BDADDR_SIZE,
					DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_UNKNOWN_L2CAP_DISCONNECT, psm);

	return;
}


void btd_manager_set_connection_hfp( DBusConnection * con )
{
	hfp_connection = con;
}

void btd_manager_set_connection_pbap( DBusConnection * con )
{
	pbap_connection = con;
}

void btd_manager_set_connection_a2dp( DBusConnection * con )
{
	a2dp_connection = con;
}

void btd_manager_set_connection_avrcp( DBusConnection * con )
{
	avrcp_connection = con;
}

void btd_manager_set_connection_browsing( DBusConnection * con )
{
	browsing_connection = con;
}

int btd_manager_isconnect( uint16_t profile )
{
	int info_no;
	manager_con_info_t *pInfo;

	for( info_no = 0; info_no < MNG_ACL_MAX; info_no++ ) {
		pInfo = &connect_info[info_no];

		if( pInfo->profile & profile ) {
			return TRUE;
		}
	}
	return FALSE;
}

int btd_manager_get_acl_num( void )
{
	int cnt;
	int acl_num = 0;

	for( cnt = 0; cnt < MNG_ACL_MAX; cnt++ ) {
		if( connect_info[cnt].use == TRUE ) {
			acl_num ++;
		}
	}
	return acl_num;
}

int btd_manager_get_acl_ope( void )
{
	return manager_ACL_ope;
}

void btd_manager_set_acl_ope( int ope )
{
	manager_ACL_ope = ope;
	return;
}

int btd_manager_get_acl_guard( void )
{
	return manager_ACL_guard;
}

void btd_manager_set_acl_guard( int ope )
{
	if( ope == TRUE ) {
		BLUEZ_DEBUG_LOG("[MNG] manager_ACL_guard ++" );
		manager_ACL_guard++;
	} else {
		BLUEZ_DEBUG_LOG("[MNG] manager_ACL_guard --" );
		manager_ACL_guard--;
		if ( manager_ACL_guard < 0 ) {
			BLUEZ_DEBUG_ERROR("[MNG] manager_ACL_guard Reset" );
			manager_ACL_guard = 0;
		}
	}
	return;
}

int btd_manager_check_acl( bdaddr_t *bdaddr )
{
	int acl_num = 0;
	int acl_ope;
	int acl_guard;
	int ret = FALSE;

	acl_num = btd_manager_get_acl_num();
	acl_ope = btd_manager_get_acl_ope();
	acl_guard = btd_manager_get_acl_guard();
	if( TRUE == acl_ope || acl_num == 3 || acl_guard > 0 ) {
		BLUEZ_DEBUG_WARN("[MNG] Connection Request Reject ope = %d num = %d guard = %d", acl_ope, acl_num, acl_guard );
		EVENT_DRC_BLUEZ_MANAGER(FORMAT_ID_BT_MANAGER, MANAGER_CONNREQ_REJECT, bdaddr, acl_num);
		ret =TRUE;
	}
	return ret;
}

void btd_manager_set_reject_info( bdaddr_t *bdaddr )
{
	int cnt;

	for( cnt = 0; cnt < MNG_LINK_MAX; cnt++ ) {
		if( reject_info[cnt].use == FALSE ) {
			memcpy( &reject_info[cnt].bdaddr, bdaddr, sizeof( bdaddr_t ) );
			reject_info[cnt].use = TRUE;
			break;
		}
	}
	return;
}

uint8_t btd_manager_find_reject_info( bdaddr_t *bdaddr )
{
	int cnt;

	for( cnt = 0; cnt < MNG_LINK_MAX; cnt++ ) {
		if( reject_info[cnt].use == TRUE && memcmp( &reject_info[cnt].bdaddr, bdaddr, sizeof( bdaddr_t ) ) == 0 ) {
			break;
		}
	}

	if( MNG_LINK_MAX == cnt ) {
		return FALSE;
	}
	reject_info[cnt].use = FALSE;
	memset( &reject_info[cnt].bdaddr, 0, sizeof( bdaddr_t ) );
	return TRUE;
}
