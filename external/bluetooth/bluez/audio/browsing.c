/*
* Copyright(C) 2012 FUJITSU TEN LIMITED
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; version 2
* of the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>
#include <errno.h>

#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <assert.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <glib.h>
#include <dbus/dbus.h>
#include <gdbus.h>

#include "log.h"

#include "device.h"
#include "avdtp.h"
#include "media.h"
#include "a2dp.h"
#include "error.h"
#include "browsing.h"
#include "dbus-common.h"
#include "../src/adapter.h"
#include "../src/device.h"
#include "glib-helper.h"
#include "telephony.h"
#include "uinput.h"
#include "a2dpad.h"

#include <utils/Log.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>

#include "../btio/btio.h"
#include "manager.h"
#include "hcicommand.h"
#include "../src/manager.h"
#include "../src/recorder.h"

/* Source file ID */
#define BLUEZ_SRC_ID BLUEZ_SRC_ID_AUDIO_BROWSING_C

struct browsing {
	struct audio_device *dev;
	browsing_state_t state;
	GIOChannel *io;
	guint io_id;
	uint16_t mtu;
	bdaddr_t dst;
	L2CAP_CONFIG_PARAM l2capConfigParam;
};

static uint16_t cidLocal;
static DBusConnection *g_browsing_connection = NULL;
static gboolean connection_initiator = FALSE; //FALSE: 対向機からの接続
static gboolean disconnect_initiator = FALSE; //FALSE 相手からの切断 
static GIOChannel *g_browsing_l2cap_io = NULL;
static uint16_t l2capPsmData;
static int l2cap_accept(struct audio_device *dev);

static void browsing_set_state(struct browsing *control, browsing_state_t new_state);

static void send_signal_connectCfmL2cap(uint32_t value, uint16_t cid, bdaddr_t *dst, uint32_t len, uint16_t status, L2CAP_CONFIG_PARAM *l2capConfigParam)
{
	char  BdAddr[BDADDR_SIZE];
	char *pBdAddr;

	BLUEZ_DEBUG_INFO("[BROWSING] send_signal_connectCfmL2cap() Start");

	memcpy(BdAddr, dst, sizeof(BdAddr));
	pBdAddr = &BdAddr[0];

	BLUEZ_DEBUG_LOG("[BROWSING] signal BdAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", BdAddr[5], BdAddr[4], BdAddr[3], BdAddr[2], BdAddr[1], BdAddr[0]);
	BLUEZ_DEBUG_LOG("[BROWSING] signal value = %d.cid = 0x%02d.status = %d.", value, cid, status);

	g_dbus_emit_signal(g_browsing_connection, "/",
					AUDIO_BROWSING_INTERFACE,
					"CONNECT_CFM_L2CAP",
					DBUS_TYPE_UINT32, &value,
					DBUS_TYPE_UINT32, &l2capConfigParam->dwTokenRate,
					DBUS_TYPE_UINT32, &l2capConfigParam->dwTokenBucketSize,
					DBUS_TYPE_UINT32, &l2capConfigParam->dwPeakBandwidth,
					DBUS_TYPE_UINT32, &l2capConfigParam->dwLatency,
					DBUS_TYPE_UINT32, &l2capConfigParam->dwDelayVariation,
					DBUS_TYPE_UINT16, &cid,
					DBUS_TYPE_UINT16, &status,
					DBUS_TYPE_UINT16, &l2capConfigParam->wFlags,
					DBUS_TYPE_UINT16, &l2capConfigParam->wMtu,
					DBUS_TYPE_UINT16, &l2capConfigParam->wFlushTo,
					DBUS_TYPE_UINT16, &l2capConfigParam->wLinkTo,
					DBUS_TYPE_UINT16, &l2capConfigParam->wRetTimeout,
					DBUS_TYPE_UINT16, &l2capConfigParam->wMonTimeout,
					DBUS_TYPE_UINT16, &l2capConfigParam->wMps,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bServiceType,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bMode,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bTxWindowSize,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bMaxTransmit,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bFcsType,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&pBdAddr, BDADDR_SIZE,
					DBUS_TYPE_INVALID);

	/* 切断情報の初期化 */
	btd_manager_get_disconnect_info( dst, MNG_CON_BROWS );

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_BROWSING_CONNECT_CFM, value);

	return;
}

static void send_signal_connectedL2cap(uint32_t value, uint16_t cid, bdaddr_t *dst, uint32_t len, uint16_t l2capPsm, L2CAP_CONFIG_PARAM *l2capConfigParam)
{
	char  BdAddr[BDADDR_SIZE];
	char *pBdAddr;

	BLUEZ_DEBUG_INFO("[BROWSING] send_signal_connectedL2cap() Start");

	memcpy(BdAddr, dst, sizeof(BdAddr));
	pBdAddr = &BdAddr[0];

	BLUEZ_DEBUG_LOG("[BROWSING] signal BdAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", BdAddr[5], BdAddr[4], BdAddr[3], BdAddr[2], BdAddr[1], BdAddr[0]);
	BLUEZ_DEBUG_LOG("[BROWSING] signal value = %d.cid = 0x%02d.l2capPsm = %d.", value, cid, l2capPsm);

	g_dbus_emit_signal(g_browsing_connection, "/",
					AUDIO_BROWSING_INTERFACE,
					"CONNECTED_L2CAP",
					DBUS_TYPE_UINT32, &value,
					DBUS_TYPE_UINT32, &l2capConfigParam->dwTokenRate,
					DBUS_TYPE_UINT32, &l2capConfigParam->dwTokenBucketSize,
					DBUS_TYPE_UINT32, &l2capConfigParam->dwPeakBandwidth,
					DBUS_TYPE_UINT32, &l2capConfigParam->dwLatency,
					DBUS_TYPE_UINT32, &l2capConfigParam->dwDelayVariation,
					DBUS_TYPE_UINT16, &cid,
					DBUS_TYPE_UINT16, &l2capPsm,
					DBUS_TYPE_UINT16, &l2capConfigParam->wFlags,
					DBUS_TYPE_UINT16, &l2capConfigParam->wMtu,
					DBUS_TYPE_UINT16, &l2capConfigParam->wFlushTo,
					DBUS_TYPE_UINT16, &l2capConfigParam->wLinkTo,
					DBUS_TYPE_UINT16, &l2capConfigParam->wRetTimeout,
					DBUS_TYPE_UINT16, &l2capConfigParam->wMonTimeout,
					DBUS_TYPE_UINT16, &l2capConfigParam->wMps,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bServiceType,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bMode,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bTxWindowSize,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bMaxTransmit,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bFcsType,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&pBdAddr, BDADDR_SIZE,
					DBUS_TYPE_INVALID);

	/* 切断情報の初期化 */
	btd_manager_get_disconnect_info( dst, MNG_CON_BROWS );

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_BROWSING_CONNECTED, value);

	return;
}

static void send_signal_dataRcvL2cap(bool value, uint16_t cid, char *data, int datalen)
{
	BLUEZ_DEBUG_INFO("[BROWSING] send_signal_dataRcvL2cap() Start");

	g_dbus_emit_signal(g_browsing_connection, "/",
					AUDIO_BROWSING_INTERFACE,
					"DATA_RCV_L2CAP",
					DBUS_TYPE_UINT32, &value,
					DBUS_TYPE_UINT16, &cid,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&data, datalen,
					DBUS_TYPE_INVALID);

	return;
}


static gboolean browsing_cb(GIOChannel *chan, GIOCondition cond,
				gpointer data)
{
	struct browsing *browsing = data;
	unsigned char buf[1024], *operands;
	unsigned char *pbuf = buf;
	int sock;
	int buf_len;

	BLUEZ_DEBUG_INFO("[BROWSING] browsing_cb() Start.");

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)) {
		BLUEZ_DEBUG_WARN("[BROWSING] G_IO_ERR or G_IO_HUP or G_IO_NVAL goto failed.");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_IO, cond, 0);
		goto failed;
	}

	sock = g_io_channel_unix_get_fd(browsing->io);
	BLUEZ_DEBUG_LOG("[BROWSING] browsing_cb() sock = %d.", sock);
	
	buf_len = read(sock, buf, sizeof(buf));
	if (buf_len <= 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] buf_len <= 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_READ, 0, 0);
		goto failed;
	}

	BLUEZ_DEBUG_LOG("Got %d bytes of data for AVCTP session %p", buf_len, browsing);

	GError *gerr = NULL;
	uint16_t cid;
	bt_io_get(chan, BT_IO_L2CAP, &gerr,
			BT_IO_OPT_CID, &cid,
			BT_IO_OPT_INVALID);
	if (gerr) {
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] bt_io_get() ERROR = [ %s ]", gerr->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_IO_GET, gerr->code, 0);
		browsing_set_state(browsing, BROWSING_STATE_DISCONNECTED);
		g_error_free(gerr);
		return FALSE;
	}
	
	bool value = TRUE;
	BLUEZ_DEBUG_LOG("[BROWSING] browsing_cb() cid = 0x%02d", cid);
	send_signal_dataRcvL2cap(value, cid, (char *)pbuf, buf_len);
	
	return TRUE;

failed:
	BLUEZ_DEBUG_LOG("BROWSING disconnected cid = 0x%02d", cidLocal);
	browsing_set_state(browsing, BROWSING_STATE_DISCONNECTED);
	return FALSE;
}

static void browsing_connect_cb(GIOChannel *chan, GError *err, gpointer data)
{
	struct browsing *browsing = data;
	char address[18];
	uint16_t imtu;
	GError *gerr = NULL;
	uint16_t cid = 0;
	bdaddr_t dst;
	uint16_t status = 0;
	L2CAP_CONFIG_PARAM l2capConfigParam;
	struct l2cap_options lo;
	int sock;
	socklen_t size;
	uint32_t value = TRUE;
	int linkinfo = 0;
	int dstLen = sizeof(bdaddr_t);

	BLUEZ_DEBUG_INFO("[BROWSING] browsing_connect_cb() Start.");
	EVENT_DRC_BLUEZ_CONNECT_CB(FORMAT_ID_CONNECT_CB, CB_BROWSING_CONNECT, TRUE);

	if (err) {
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] ERROR : [ %s ]", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_CB_ERR, err->code, 0);
		goto failed;
	}

	bt_io_get(chan, BT_IO_L2CAP, &gerr,
			BT_IO_OPT_DEST, &address,
			BT_IO_OPT_IMTU, &imtu,
			BT_IO_OPT_CID, &cid,
			BT_IO_OPT_DEST_BDADDR, &dst,
			BT_IO_OPT_INVALID);
	if (gerr) {
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] bt_io_get() gerr = [ %s ]", gerr->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_CB_IO_GET, gerr->code, 0);
		g_error_free(gerr);
		goto failed;
	}

	if (!browsing->io) {
		browsing->io = g_io_channel_ref(chan);
	}

	browsing_set_state(browsing, BROWSING_STATE_CONNECTED);
	browsing->mtu = imtu;
	browsing->io_id = g_io_add_watch(chan,
				G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
				(GIOFunc) browsing_cb, browsing);

	sock = g_io_channel_unix_get_fd(browsing->io);
	BLUEZ_DEBUG_LOG("[BROWSING] browsing_connect_cb() sock = %d.", sock);
	memset(&lo, 0, sizeof(lo));
	size = sizeof(lo);
	if (getsockopt(sock, SOL_L2CAP, L2CAP_OPTIONS, &lo, &size) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] getsockopt() ERROR = [ %s ]", strerror(errno));
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_CB_GET_SOCKOPT, errno, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_CB_GET_SOCKOPT, __LINE__, &dst);
		goto failed;
	}
	memset(&l2capConfigParam, 0, sizeof(l2capConfigParam));
	
	l2capSetCfgDefParam(&l2capConfigParam);
	
	l2capConfigParam.wMtu = lo.imtu;
	l2capConfigParam.wFlushTo = lo.flush_to;
	l2capConfigParam.bMode = lo.mode;
	l2capConfigParam.bFcsType = lo.fcs;
	l2capConfigParam.bMaxTransmit = lo.max_tx;
	l2capConfigParam.bTxWindowSize = lo.txwin_size; 

	BLUEZ_DEBUG_LOG("[BROWSING] browsing_connect_cb() getsockopt: wMtu = %d. wFlushTo = %d. bMode = %d.", l2capConfigParam.wMtu, l2capConfigParam.wFlushTo, l2capConfigParam.bMode);
	BLUEZ_DEBUG_LOG("[BROWSING] browsing_connect_cb() getsockopt: bFcsType = %d. bMaxTransmit = %d. bTxWindowSize = %d.", l2capConfigParam.bFcsType, l2capConfigParam.bMaxTransmit, l2capConfigParam.bTxWindowSize);

	cidLocal = cid;
	memcpy(&browsing->l2capConfigParam, &l2capConfigParam, sizeof(L2CAP_CONFIG_PARAM));

	if(TRUE == connection_initiator){
		send_signal_connectCfmL2cap( value, cid, &dst, dstLen, status, &l2capConfigParam );
	} else {
		send_signal_connectedL2cap( value, cid, &dst, dstLen, l2capPsmData, &l2capConfigParam );
	}
	btd_manager_connect_profile( &dst, MNG_CON_BROWS, 0, cid );

	return;

failed:
	memset( &l2capConfigParam, 0, sizeof(l2capConfigParam) );

	if( TRUE == connection_initiator ){ 
		linkinfo = btd_manager_find_connect_info_by_bdaddr( &browsing->dst );
		if( linkinfo == MNG_ACL_MAX ) {
			if( btd_manager_get_disconnect_info( &browsing->dst, MNG_CON_BROWS ) == TRUE ) {
				value = FALSE;
			} else {
				value = 0xFF;
			}
		} else {
			value = FALSE;
		}
		send_signal_connectCfmL2cap( value, cid, &browsing->dst, dstLen, status, &l2capConfigParam );
	}
	browsing_set_state(browsing, BROWSING_STATE_DISCONNECTED);
	return;
}

static DBusMessage *connect_l2cap_browsing(DBusConnection *conn, DBusMessage *msg, void *data)
{
	int status = 0;
	const char *dstAddr;
	uint16_t l2capPsm = 0;
	int dstLen = sizeof(bdaddr_t);
	bdaddr_t src;
	bdaddr_t dst;
	L2CAP_CONFIG_PARAM l2capConfigParam;
	int linkinfo = 0;
	uint32_t value = FALSE;
	uint16_t cid = 0;
	GError *err = NULL;
	GIOChannel *io;
	struct browsing *browsing = NULL;
	struct audio_device *device = NULL;

	BLUEZ_DEBUG_INFO("[BROWSING] connect_l2cap_browsing() Start ");

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_UINT16, &l2capPsm,
						DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
						&dstAddr, &dstLen,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] dbus_message_get_args() ERROR ");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_WARN("[BROWSING] connect_l2cap_browsing() l2capPsm = %d dstAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", l2capPsm, dstAddr[5], dstAddr[4], dstAddr[3], dstAddr[2], dstAddr[1], dstAddr[0]);

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_BROWSING_CONNECT, dstAddr);

	bluezutil_get_bdaddr( &src );

	memcpy( &dst, dstAddr, sizeof(bdaddr_t) );
	device = manager_find_device(NULL, NULL, &dst, AUDIO_BROWSING_INTERFACE, FALSE);
	if ( device == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] device ERROR. device = NULL. ");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_FIND_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_FIND_DEV, __LINE__, &dst);
		goto failed;
	}

	if (!manager_allow_browsing_connection(device, &dst)) {
		/* 接続が拒否された場合 */
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] Refusing browsing: too many existing connections");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_REFUSING, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_REFUSING, __LINE__, &dst);
		goto failed;
	}

	if( BROWSING_STATE_CONNECTING == browsing_get_state( device ) ) {
		/* 相手からの接続中 */
		BLUEZ_DEBUG_WARN("[BROWSING] BROWSING is connecting");
		/* 自分から接続（相手からの接続シーケンスで接続を完了させる） */
		connection_initiator = TRUE;
		disconnect_initiator = FALSE;
		EVENT_DRC_BLUEZ_CHGMSG(FORMAT_ID_CHGMSG, CHANGE_MESSAGE_BROWSING, &dst);
		return dbus_message_new_method_return(msg);
	}

	if( btd_manager_isconnect( MNG_CON_BROWS ) == TRUE && BROWSING_STATE_CONNECTED != browsing_get_state( device ) ) {
		/* 切断中 */
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] browsing is disconnecting");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_UNDER_CONNECTION, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_UNDER_CONNECTION, __LINE__, &dst);
		goto failed;
	}

	browsing = device->browsing;

	if( btd_manager_isconnect( MNG_CON_BROWS ) == TRUE && BROWSING_STATE_CONNECTED == browsing_get_state( device ) ) {
		/* 接続クロス（DBUS でクロスしたため、Connected を L2CAP（BSM） で破棄している ） */
		BLUEZ_DEBUG_WARN("[BROWSING] browsing is alreday connected. Return connectCfm OK");
		EVENT_DRC_BLUEZ_CHGMSG(FORMAT_ID_CHGMSG, CHANGE_MESSAGE_BROWSING_CROSS, &dst);
		value = TRUE;
		memcpy( &l2capConfigParam, &browsing->l2capConfigParam, sizeof(l2capConfigParam) );
		cid = cidLocal;
		send_signal_connectCfmL2cap( value, cid, &dst, dstLen, status, &l2capConfigParam );
		return dbus_message_new_method_return(msg);
	}

	browsing = device->browsing;

	memcpy( &browsing->dst, &dst, sizeof( bdaddr_t ) );
	io = bt_io_connect(BT_IO_L2CAP, browsing_connect_cb, browsing, NULL, &err,
				BT_IO_OPT_SOURCE_BDADDR, &src,
				BT_IO_OPT_DEST_BDADDR, &dst,
				BT_IO_OPT_PSM, l2capPsm,
				BT_IO_OPT_MODE, L2CAP_MODE_ERTM,
				BT_IO_OPT_INVALID);
	if (err) {
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] bt_io_connect ERROR = [ %s ]", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_IO_CONNECT, err->code, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_IO_CONNECT, __LINE__, &dst);
		g_error_free(err);
		goto failed;
	}
	connection_initiator = TRUE;
	/* 切断方向初期化 */
	disconnect_initiator = FALSE;
	browsing->io = io;
	browsing_set_state(browsing, BROWSING_STATE_CONNECTING);

	BLUEZ_DEBUG_INFO("[BROWSING] connect_l2cap_browsing() End ");
	return dbus_message_new_method_return(msg);

failed:
	memset( &l2capConfigParam, 0, sizeof(l2capConfigParam) );

	linkinfo = btd_manager_find_connect_info_by_bdaddr( &dst );
	if( linkinfo == MNG_ACL_MAX ) {
		if( btd_manager_get_disconnect_info( &dst, MNG_CON_BROWS ) == TRUE ) {
			value = FALSE;
		} else {
			value = 0xFF;
		}
	} else {
		value = FALSE;
	}
	send_signal_connectCfmL2cap( value, cid, &dst, dstLen, status, &l2capConfigParam );

	return dbus_message_new_method_return(msg);
}

static void browsing_disconnected(struct audio_device *dev)
{
	struct browsing *browsing = dev->browsing;

	if (!browsing){
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_NULL, 0, 0);
		return;
	}

	if (browsing->io) {
		g_io_channel_shutdown(browsing->io, TRUE, NULL);
		g_io_channel_unref(browsing->io);
		browsing->io = NULL;
	}

	if (browsing->io_id) {
		g_source_remove(browsing->io_id);
		browsing->io_id = 0;
	}
	memset(&browsing->l2capConfigParam, 0, sizeof(L2CAP_CONFIG_PARAM));

	return;
}

static void browsing_set_state(struct browsing *control, browsing_state_t new_state)
{
	struct audio_device *dev = control->dev;
	browsing_state_t old_state = control->state;

	switch (new_state) {
	case BROWSING_STATE_DISCONNECTED:
		BLUEZ_DEBUG_LOG("[BROWSING] Browsing Disconnected");
		browsing_disconnected(control->dev);
		break;
	case BROWSING_STATE_CONNECTING:
		BLUEZ_DEBUG_LOG("[BROWSING] Browsing Connecting");
		break;
	case BROWSING_STATE_CONNECTED:
		BLUEZ_DEBUG_LOG("[BROWSING] Browsing Connected");
		break;
	default:
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] Invalid BROWSING state %d", new_state);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SET_STATE, new_state, 0);
		return;
	}

	control->state = new_state;
	EVENT_DRC_BLUEZ_STATE(FORMAT_ID_STATE, STATE_BROWSING, &dev->dst, new_state);
	btd_manager_set_state_info( &dev->dst, MNG_CON_BROWS, new_state );
	return;
}


static void browsing_disconnect(struct audio_device *dev)
{
	struct browsing *browsing = dev->browsing;
	uint16_t value = TRUE;

	if (!(browsing && browsing->io)) {
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] already disconnected");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_NULL, 0, 0);
		return;
	}

	browsing_set_state(browsing, BROWSING_STATE_DISCONNECTED);
	return;
}

static DBusMessage *disconnect_l2cap_browsing(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	struct audio_device *device;
	const char *dstAddr;
	uint32_t len;
	bdaddr_t dst;
	uint16_t l2capPsm;

	BLUEZ_DEBUG_INFO("[BROWSING] disconnect_l2cap_browsing() Start ");

	if (!dbus_message_get_args(msg, NULL,
								DBUS_TYPE_UINT16, &l2capPsm,
								DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
								&dstAddr, &len,
								DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] dbus_message_get_args() ERROR ");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[BROWSING] disconnect_l2cap_browsing(), dstAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", dstAddr[5], dstAddr[4], dstAddr[3], dstAddr[2], dstAddr[1], dstAddr[0]);

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_BROWSING_DISCONNECT, dstAddr);

	memcpy( &dst, dstAddr, sizeof(bdaddr_t) );
	device = manager_find_device(NULL, NULL, &dst, AUDIO_BROWSING_INTERFACE , FALSE);
	if ( device == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] device ERROR. device = NULL. ");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_FIND_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DISCONNECT_FIND_DEV, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}

	if( BROWSING_STATE_DISCONNECTED != browsing_get_state(device) ) {
		disconnect_initiator = TRUE;
	}

	browsing_disconnect(device);

	/* 既に切断通知をあげているか判定 */
	if( btd_manager_isconnect( MNG_CON_BROWS ) == FALSE ) {
		BLUEZ_DEBUG_LOG("[BROWSING] browsing streaming already disconnected");
		btd_manager_browsing_signal_disconnect_cfm( FALSE, cidLocal );
		disconnect_initiator = FALSE;
	}

	BLUEZ_DEBUG_INFO("[BROWSING] disconnect_l2cap_browsing() End ");

	return dbus_message_new_method_return(msg);
}

static DBusMessage *data_snd_l2cap_browsing(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	struct audio_device *device;
	const char *senddata;
	uint32_t len;
	const char *dstAddr;
	uint32_t addrlen;
	bdaddr_t dst;
	uint16_t l2capPsm;
	struct browsing *browsing = NULL;
	int sk = 0;

	BLUEZ_DEBUG_INFO("[BROWSING] data_snd_l2cap_browsing() Start");

	if (!dbus_message_get_args(msg, NULL,
							   DBUS_TYPE_UINT16, &l2capPsm,
							   DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
							   &dstAddr, &addrlen,
							   DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
							   &senddata, &len,
							   DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] dbus_message_get_args() ERROR ");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[BROWSING] data_snd_l2cap_browsing() senddata = %s. len = %d. ", senddata, len);
	BLUEZ_DEBUG_LOG("[BROWSING] data_snd_l2cap_browsing() dstAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", dstAddr[5], dstAddr[4], dstAddr[3], dstAddr[2], dstAddr[1], dstAddr[0]);

	memcpy( &dst, dstAddr, sizeof(bdaddr_t) );
	device = manager_find_device(NULL, NULL, &dst, AUDIO_BROWSING_INTERFACE, FALSE);
	if ( device == NULL || device->browsing == NULL || device->browsing->io == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] browsing is already closed. ");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_FIND_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DATASEND_FIND_DEV, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}

	browsing = device->browsing;
	sk = g_io_channel_unix_get_fd(browsing->io);

	BLUEZ_DEBUG_LOG("[BROWSING] data_snd_l2cap_browsing() sk = %d. ", sk);
	if (write(sk, senddata, len) < 0){
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] write() ERROR. errno = %d. ", errno);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_WRITE, errno, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DATASEND_WRITE, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}

	BLUEZ_DEBUG_INFO("[BROWSING] data_snd_l2cap_browsing() End ");
	return dbus_message_new_method_return(msg);
}

static GDBusMethodTable browsing_methods[] = {
	{ "CONNECT_L2CAP",		"qay",		"", connect_l2cap_browsing,		0,	0	},
	{ "DISCONNECT_L2CAP",	"qay",		"", disconnect_l2cap_browsing,	0,	0	},
	{ "DATA_SND_L2CAP", 	"qayay",	"", data_snd_l2cap_browsing,	0,	0	},
	{ NULL, NULL, NULL, NULL, 0, 0 }
};

static GDBusSignalTable browsing_signals[] = {
	{ "CONNECT_CFM_L2CAP",		"uuuuuuqqqqqqqqqyyyyyay",	G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "CONNECTED_L2CAP",		"uuuuuuqqqqqqqqqyyyyyay",	G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "DISCONNECT_CFM_L2CAP",	"qq",						G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "DISCONNECTED_L2CAP", 	"qq",						G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "DATA_RCV_L2CAP", 		"uqay", 					G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ NULL, NULL, G_DBUS_SIGNAL_FLAG_DEPRECATED }
};

static void path_unregister(void *data)
{
	BLUEZ_DEBUG_LOG("[BROWSING] Unregistered interface %s on path %s", AUDIO_BROWSING_INTERFACE, "/");
}

void browsing_unregister(struct audio_device *dev)
{
	struct browsing *browsing  = dev->browsing;
	L2CAP_CONFIG_PARAM l2capConfigParam;

	BLUEZ_DEBUG_LOG("[BROWSING] browsing unregister");

	if ( connection_initiator == TRUE && browsing_get_state( dev ) == BROWSING_STATE_CONNECTING ) {
		/* ACL が張られていない場合、接続完了通知（失敗）をあげる必要がある */
		memset( &l2capConfigParam, 0, sizeof(l2capConfigParam) );
		send_signal_connectCfmL2cap( 0xFF, 0, &dev->dst, 6, 0, &l2capConfigParam );
	}

	browsing_disconnected(dev);

	g_free(browsing);

	dev->browsing = NULL;

	return;
}

struct browsing  *browsing_init(struct audio_device *dev)
{
	struct browsing *browsing;

	BLUEZ_DEBUG_INFO("[BROWSING] browsing_init() Start ");
	EVENT_DRC_BLUEZ_INIT(FORMAT_ID_BT_INIT, INIT_BROWSING);

	browsing = g_new0(struct browsing, 1);

	browsing->dev = dev;
	BLUEZ_DEBUG_INFO("[BROWSING] browsing_init() End ");

	return browsing;
}

int browsing_dbus_init(void)
{
	BLUEZ_DEBUG_INFO("[BROWSING] browsing_dbus_init() Start ");
	EVENT_DRC_BLUEZ_INIT(FORMAT_ID_BT_INIT, INIT_DBUS_BROWSING);

	if (g_browsing_connection != NULL  ) {
		g_dbus_unregister_interface(g_browsing_connection, "/", AUDIO_BROWSING_INTERFACE);
		g_browsing_connection = NULL;
	}

	g_browsing_connection = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
	if (g_dbus_register_interface(g_browsing_connection, "/",
					AUDIO_BROWSING_INTERFACE,
					browsing_methods, browsing_signals,
					NULL, NULL, path_unregister) == FALSE) {
		 BLUEZ_DEBUG_ERROR("[Bluez - BROWSING]  g_dbus_register_interface() ERROR ");
		 ERR_DRC_BLUEZ(ERROR_BLUEZ_DBUS_INIT, 0, 0);
		 return -1;
	}

	btd_manager_set_connection_browsing( g_browsing_connection );
	BLUEZ_DEBUG_LOG("[BROWSING] browsing_dbus_init End.");
	return 0;
}

browsing_state_t browsing_get_state(struct audio_device *dev)
{
	struct browsing *browsing = dev->browsing;

	if( browsing == NULL ) {
		return BROWSING_STATE_DISCONNECTED;
	}
	return browsing->state;
}
uint16_t browsing_get_cid(void)
{
	return cidLocal;
}
int browsing_get_disconnect_initiator(void)
{
	return disconnect_initiator;
}
void browsing_set_disconnect_initiator(int flg)
{
	disconnect_initiator = flg;
	return;
}

static int l2cap_accept(struct audio_device *dev)
{
	GError *err = NULL;
	GIOChannel *io;
	struct browsing *browsing = dev->browsing;
	
	BLUEZ_DEBUG_INFO("[BROWSING] l2cap_accept() Start.");

	if (!bt_io_accept(browsing->io, browsing_connect_cb, browsing, NULL, &err)) {
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] bt_io_accept() ERROR = [ %s ]", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_IO_ACCEPT, err->code, 0);
		g_error_free(err);
		return -EIO;
	}

	BLUEZ_DEBUG_INFO("[BROWSING] l2cap_accept() End.");
	return 0;
}

static void browsing_confirm_cb(GIOChannel *chan, gpointer data)
{
	struct browsing *browsing = NULL;
	struct audio_device *dev;
	char address[18];
	bdaddr_t src, dst;
	GError *err = NULL;
	int status;

	BLUEZ_DEBUG_INFO("[BROWSING] browsing_confirm_cb() Start.");
	EVENT_DRC_BLUEZ_CB(FORMAT_ID_BT_CB, CB_BROWSING_REGISTER);

	bt_io_get(chan, BT_IO_L2CAP, &err,
			BT_IO_OPT_SOURCE_BDADDR, &src,
			BT_IO_OPT_DEST_BDADDR, &dst,
			BT_IO_OPT_DEST, address,
			BT_IO_OPT_INVALID);
	if (err) {
		BLUEZ_DEBUG_ERROR("[Bluez- BROWSING] bt_io_get() ERROR = [ %s ]", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_IO_GET, err->code, 0);
		g_error_free(err);
		g_io_channel_shutdown(chan, TRUE, NULL);
		return;
	}

	BLUEZ_DEBUG_WARN("[BROWSING] Confirm from [ %02x:%02x:%02x:%02x:%02x:%02x ]", dst.b[5], dst.b[4], dst.b[3], dst.b[2], dst.b[1], dst.b[0]);
	EVENT_DRC_BLUEZ_CONFIRM(FORMAT_ID_CONFIRM, CONFIRM_BROWSING, &dst);

	dev = manager_get_device(&src, &dst, TRUE);
	if (!dev) {
		BLUEZ_DEBUG_ERROR("[Bluez- BROWSING] Unable to get audio device object for %s", address);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_GET_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_GET_DEV, __LINE__, &dst);
		goto drop;
	}

	if (!manager_allow_browsing_connection(dev, &dst)) {
		BLUEZ_DEBUG_ERROR("[Bluez- BROWSING] Refusing browsing: too many existing connections");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_REFUSING, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_REFUSING, __LINE__, &dst);
		goto drop;
	}

	if( btd_manager_isconnect( MNG_CON_BROWS ) == TRUE ) {
		/* 切断中 */
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] browsing is still under connection");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_UNDER_CONNECTION, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_UNDER_CONNECTION, __LINE__, &dst);
		goto drop;
	}

	if (browsing_get_state(dev) > BROWSING_STATE_DISCONNECTED) {
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] Refusing new connection since one already exists");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_EXISTS, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_EXISTS, __LINE__, &dst);
		goto drop;
	}

	if (!dev->browsing) {
		/* BROWSING は UUID が無いので AVRCP で代用 */
		btd_device_add_uuid(dev->btd_dev, AVRCP_TARGET_UUID);
		BLUEZ_DEBUG_LOG("[BROWSING] browsing_confirm_cb btd_device_add_uuid");
		if (!dev->browsing) {
			BLUEZ_DEBUG_ERROR("[Bluez- BROWSING] dev->browsing is nothing --> goto drop");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_ADD_UUID, 0, 0);
			EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_ADD_UUID, __LINE__, &dst);
			goto drop;
		}
	}

	browsing = dev->browsing;
	if (!browsing->io){
		browsing->io = g_io_channel_ref(chan);
	}
	memcpy( &browsing->dst, &dst, sizeof( bdaddr_t ) );

	connection_initiator = FALSE;
	disconnect_initiator = FALSE;
	browsing_set_state(browsing, BROWSING_STATE_CONNECTING);

	status = l2cap_accept(dev);
	if (status < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] l2cap_accept() ERROR = %d", status);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_AUTHORIZATION, status, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_AUTHORIZATION, __LINE__, &dst);
		goto drop;
	}

	browsing->io_id = g_io_add_watch(chan, G_IO_ERR | G_IO_HUP | G_IO_NVAL, browsing_cb, browsing);

	BLUEZ_DEBUG_INFO("[BROWSING] browsing_confirm_cb() End.");

	return;

drop:
	g_io_channel_shutdown(chan, TRUE, NULL);
}

gboolean register_l2cap_browsing( bdaddr_t *bdaddr, uint16_t browsingPsm )
{
	gboolean master = TRUE;
	GIOChannel *io;
	GError *err = NULL;
	gboolean value = TRUE;
	sdp_record_t *record;
	gboolean tmp;
	bdaddr_t src;

	BLUEZ_DEBUG_INFO("[BROWSING] register_l2cap_browsing() Start.");
	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_BROWSING_REGISTER, NULL);
	BLUEZ_DEBUG_LOG("[BROWSING] register_l2cap_browsing() browsingPsm = %d", browsingPsm);
	bluezutil_get_bdaddr( &src );

	if (!g_browsing_l2cap_io) {
		io = bt_io_listen(BT_IO_L2CAP, NULL, browsing_confirm_cb, NULL,
					NULL, &err,
					BT_IO_OPT_SOURCE_BDADDR, &src,
					BT_IO_OPT_PSM, browsingPsm,
					BT_IO_OPT_SEC_LEVEL, BT_IO_SEC_MEDIUM,
					BT_IO_OPT_MASTER, master,
					BT_IO_OPT_MODE, L2CAP_MODE_ERTM,
					BT_IO_OPT_INVALID);
		
		if (!io) {
			BLUEZ_DEBUG_ERROR("[Bluez - BROWSING] bt_io_listen() err = [ %s ]", err->message);
			ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_IO_LISTEN, err->code, 0);
			g_error_free(err);
			value = FALSE;
		}
		g_browsing_l2cap_io = io;
		l2capPsmData = browsingPsm;
	}
	
	BLUEZ_DEBUG_INFO("[BROWSING] register_l2cap_browsing() End value = %d", value);

	connection_initiator = FALSE;
	disconnect_initiator = FALSE;

	return value;
}

gboolean deregister_l2cap_browsing( bdaddr_t *bdaddr, uint16_t browsingPsm )
{
	gboolean value = TRUE;

	BLUEZ_DEBUG_INFO("[BROWSING] deregister_l2cap_browsing() Start.");
	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_BROWSING_DEREGISTER, NULL);

	BLUEZ_DEBUG_LOG("[BROWSING] deregister_l2cap_browsing() browsingPsm = %x", browsingPsm);
	BLUEZ_DEBUG_LOG("[BROWSING] deregister_l2cap_browsing() bdaddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", bdaddr->b[5], bdaddr->b[4], bdaddr->b[3], bdaddr->b[2], bdaddr->b[1], bdaddr->b[0]);

	BLUEZ_DEBUG_INFO("[BROWSING] deregister_l2cap_browsing() End.");

	return value;
}
