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
#include "avrcpad.h"
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
#include "sdpd.h"
#include "../src/manager.h"
#include "../src/recorder.h"

/* Source file ID */
#define BLUEZ_SRC_ID BLUEZ_SRC_ID_AUDIO_AVRCPAD_C

#define AVRCP_SDP_TIMER 10
#define AVRCP_DEFAULT_VERSION 0x0000
#define AVRCP_DEFAULT_FEAT 0x0000

struct avrcpad {
	struct audio_device *dev;
	avrcpad_state_t state;
	GIOChannel *io;
	guint io_id;
	uint16_t mtu;
	uint16_t svclass;
	uint16_t version;
	uint16_t supp_feat;
	bdaddr_t dst;
	L2CAP_CONFIG_PARAM l2capConfigParam;
	guint sdp_timer;
};

static gboolean connection_initiator = FALSE; //FALSE: 対向機からの接続
static gboolean disconnect_initiator = FALSE; //FALSE 相手からの切断 
static gboolean connect_flg = FALSE; //接続処理の有無（FALSE：まだ行っていない）
static gboolean sdp_flg = FALSE; //SDP 有無（FALSE：まだ行っていない）
uint16_t l2capPsmData;
static uint16_t cidLocal;
static GIOChannel *g_avrcpad_l2cap_io = NULL;
static uint32_t g_avrcpad_record_id = 0;
static DBusConnection *g_avrcpad_connection = NULL;

static int get_records(struct audio_device *device);
static DBusMessage *connect_l2cap_avrcp(DBusConnection *conn, DBusMessage *msg, void *data);
int l2cap_connect(struct audio_device *dev);
int l2cap_accept(struct audio_device *dev);

static void avrcp_set_state(struct avrcpad *control, avrcpad_state_t new_state);

static void send_signal_registerCfmL2cap(uint16_t value)
{
	BLUEZ_DEBUG_INFO("[AVRCPAD] send_signal_registerCfmL2cap() Start.");
	BLUEZ_DEBUG_LOG("[AVRCPAD] send_signal_registerCfmL2cap() value = %d.",value);

	g_dbus_emit_signal(g_avrcpad_connection, "/",
					AUDIO_AVRCPAD_INTERFACE,
					"REGISTER_CFM_L2CAP",
					DBUS_TYPE_UINT16, &value,
					DBUS_TYPE_INVALID);

	BLUEZ_DEBUG_INFO("[AVRCPAD] send_signal_registerCfmL2cap() End.");

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_AVRCP_REGISTER_CFM, value);

	return;
}

static void send_signal_deregisterCfmL2cap(uint16_t value)
{
	BLUEZ_DEBUG_INFO("[AVRCPAD] send_signal_deregisterCfmL2cap() Start.");

	g_dbus_emit_signal(g_avrcpad_connection, "/",
						AUDIO_AVRCPAD_INTERFACE,
						"DEREGISTER_CFM_L2CAP",
						DBUS_TYPE_UINT16, &value,
						DBUS_TYPE_INVALID);

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_AVRCP_DEREGISTER_CFM, value);

	return;
}

static void send_signal_connectCfmL2cap(uint32_t value, uint16_t cid, bdaddr_t *dst, uint32_t len, uint16_t status, uint16_t version, uint16_t supp_feat, L2CAP_CONFIG_PARAM *l2capConfigParam)
{
	char  BdAddr[BDADDR_SIZE];
	char *pBdAddr;

	memcpy(BdAddr, dst,  sizeof(BdAddr));
	pBdAddr = &BdAddr[0];

	BLUEZ_DEBUG_INFO("[AVRCPAD] send_signal_connectCfmL2cap() Start.");
	BLUEZ_DEBUG_LOG("[AVRCPAD] signal BdAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", BdAddr[5], BdAddr[4], BdAddr[3], BdAddr[2], BdAddr[1], BdAddr[0]);
	BLUEZ_DEBUG_LOG("[AVRCPAD] signal value = %d.cid = 0x%02x.status = %d.version = %02x.supp_feat = %02x.", value, cid, status, version, supp_feat);

	g_dbus_emit_signal(g_avrcpad_connection, "/",
					AUDIO_AVRCPAD_INTERFACE,
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
					DBUS_TYPE_UINT16, &version,
					DBUS_TYPE_UINT16, &supp_feat,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bServiceType,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bMode,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bTxWindowSize,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bMaxTransmit,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bFcsType,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&pBdAddr, BDADDR_SIZE,
					DBUS_TYPE_INVALID);

	/* 切断情報の初期化 */
	btd_manager_get_disconnect_info( dst, MNG_CON_AVRCP );

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_AVRCP_CONNECT_CFM, value);

	return;
}

static void send_signal_connectedL2cap(uint32_t value, uint16_t cid, uint16_t psm, bdaddr_t *dst, uint32_t len, uint8_t identifier, uint16_t version, uint16_t supp_feat, L2CAP_CONFIG_PARAM *l2capConfigParam)
{
	char  BdAddr[BDADDR_SIZE];
	char *pBdAddr;

	memcpy(BdAddr, dst, sizeof(BdAddr));
	pBdAddr = &BdAddr[0];

	BLUEZ_DEBUG_INFO("[AVRCPAD] send_signal_connectedL2cap() Start.");
	BLUEZ_DEBUG_LOG("[AVRCPAD] signal BdAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", BdAddr[5], BdAddr[4], BdAddr[3], BdAddr[2], BdAddr[1], BdAddr[0]);
	BLUEZ_DEBUG_LOG("[AVRCPAD] signal value = %d.cid = 0x%02x.psm = %d. identifier = %d", value, cid, psm, identifier);

	g_dbus_emit_signal(g_avrcpad_connection, "/",
					AUDIO_AVRCPAD_INTERFACE,
					"CONNECTED_L2CAP",
				  DBUS_TYPE_UINT32, &value,
				  DBUS_TYPE_UINT32, &l2capConfigParam->dwTokenRate,
				  DBUS_TYPE_UINT32, &l2capConfigParam->dwTokenBucketSize,
				  DBUS_TYPE_UINT32, &l2capConfigParam->dwPeakBandwidth,
				  DBUS_TYPE_UINT32, &l2capConfigParam->dwLatency,
				  DBUS_TYPE_UINT32, &l2capConfigParam->dwDelayVariation,
				  DBUS_TYPE_UINT16, &cid,
				  DBUS_TYPE_UINT16, &psm,
				  DBUS_TYPE_UINT16, &l2capConfigParam->wFlags,
				  DBUS_TYPE_UINT16, &l2capConfigParam->wMtu,
				  DBUS_TYPE_UINT16, &l2capConfigParam->wFlushTo,
				  DBUS_TYPE_UINT16, &l2capConfigParam->wLinkTo,
				  DBUS_TYPE_UINT16, &l2capConfigParam->wRetTimeout,
				  DBUS_TYPE_UINT16, &l2capConfigParam->wMonTimeout,
				  DBUS_TYPE_UINT16, &l2capConfigParam->wMps,
				  DBUS_TYPE_UINT16, &version,
				  DBUS_TYPE_UINT16, &supp_feat,
				  DBUS_TYPE_BYTE, &identifier,
				  DBUS_TYPE_BYTE, &l2capConfigParam->bServiceType,
				  DBUS_TYPE_BYTE, &l2capConfigParam->bMode,
				  DBUS_TYPE_BYTE, &l2capConfigParam->bTxWindowSize,
				  DBUS_TYPE_BYTE, &l2capConfigParam->bMaxTransmit,
				  DBUS_TYPE_BYTE, &l2capConfigParam->bFcsType,
				  DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
				  &pBdAddr, BDADDR_SIZE,
				  DBUS_TYPE_INVALID);

	/* 切断情報の初期化 */
	btd_manager_get_disconnect_info( dst, MNG_CON_AVRCP );

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_AVRCP_CONNECTED, value);

	return;
}

static void send_signal_dataRcvL2cap(bool value, uint16_t cid, char *data, int datalen)
{
	BLUEZ_DEBUG_INFO("[AVRCPAD] send_signal_dataRcvL2cap() Start.");

	g_dbus_emit_signal(g_avrcpad_connection, "/",
					AUDIO_AVRCPAD_INTERFACE,
					"DATA_RCV_L2CAP",
					DBUS_TYPE_UINT32, &value,
					DBUS_TYPE_UINT16, &cid,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&data, datalen,
					DBUS_TYPE_INVALID);

	return;
}

static gboolean avrcpad_cb(GIOChannel *chan, GIOCondition cond,
				gpointer data)
{
	struct avrcpad *avrcpad = data;
	unsigned char buf[1024], *operands;
	unsigned char *pbuf = buf;
	int ret, sock;
	int buf_len;
	bool value = TRUE;

	BLUEZ_DEBUG_INFO("[AVRCPAD] avrcpad_cb() Start.");

	if (cond & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)){
		BLUEZ_DEBUG_WARN("[AVRCPAD] G_IO_ERR or G_IO_HUP or G_IO_NVAL : goto failed.");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_IO, cond, 0);
		goto failed;
	}

	sock = g_io_channel_unix_get_fd(avrcpad->io);
	BLUEZ_DEBUG_LOG("[AVRCPAD] avrcpad_cb() sock = %d.", sock);

	buf_len = read(sock, buf, sizeof(buf));
	if (buf_len <= 0){
		BLUEZ_DEBUG_ERROR("[Bluez- AVRCPAD] read() buf_len = %d. errno = %s(%d)goto failed.", buf_len, strerror(errno), errno);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_READ, errno, 0);
		goto failed;
	}

	BLUEZ_DEBUG_LOG("Got %d bytes of data for AVCTP session %p", buf_len, avrcpad);

	GError *gerr = NULL;
	uint16_t cid;
	bt_io_get(chan, BT_IO_L2CAP, &gerr,
			BT_IO_OPT_CID, &cid,
			BT_IO_OPT_INVALID);
	if (gerr) {
		BLUEZ_DEBUG_ERROR("[Bluez- AVRCPAD] bt_io_get() ERROR = [ %s ]", gerr->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_IO_GET, gerr->code, 0);
		avrcp_set_state(avrcpad, AVRCP_STATE_DISCONNECTED);
		g_error_free(gerr);
		return FALSE;
	}

	BLUEZ_DEBUG_INFO("[AVRCPAD] avrcpad_cb() cid = 0x%02x", cid);
	send_signal_dataRcvL2cap(value, cid, (char *)pbuf, buf_len);

	return TRUE;

failed:
	BLUEZ_DEBUG_INFO("AVRCPAD disconnected cid  = 0x%02x ", cidLocal);
	avrcp_set_state(avrcpad, AVRCP_STATE_DISCONNECTED);
	return FALSE;
}


static void avrcpad_connect_cb(GIOChannel *chan, GError *err, gpointer data)
{
	struct avrcpad *avrcpad = data;
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
	char identifier = 0;
	int linkinfo = 0;
	uint32_t value = TRUE;
	int dstLen = sizeof(bdaddr_t);
	int ret;

	BLUEZ_DEBUG_INFO("[AVRCPAD] avrcpad_connect_cb() Start.");
	EVENT_DRC_BLUEZ_CONNECT_CB(FORMAT_ID_CONNECT_CB, CB_AVRCP_CONNECT, connection_initiator);

	if (err) {
		BLUEZ_DEBUG_ERROR("[Bluez- AVRCPAD] ERROR [ %s ]", err->message );
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
		BLUEZ_DEBUG_ERROR("[Bluez- AVRCPAD]  bt_io_get() ERROR [ %s ]", gerr->message );
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_CB_IO_GET, gerr->code, 0);
		g_error_free(gerr);
		goto failed;
	}

	BLUEZ_DEBUG_LOG("[AVRCPAD] AVCTP: connected to %s", address);

	if (!avrcpad->io) {
		avrcpad->io = g_io_channel_ref(chan);
	}

	avrcpad->mtu = imtu;
	avrcpad->io_id = g_io_add_watch(chan,
				G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
				(GIOFunc) avrcpad_cb, avrcpad);

	sock = g_io_channel_unix_get_fd(avrcpad->io);
	BLUEZ_DEBUG_LOG("[AVRCPAD] avrcpad_connect_cb() sock = %d.", sock);
	memset(&lo, 0, sizeof(lo));
	size = sizeof(lo);
	if (getsockopt(sock, SOL_L2CAP, L2CAP_OPTIONS, &lo, &size) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez- AVRCPAD] getsockopt() ERROR : %s", strerror(errno));
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

	BLUEZ_DEBUG_LOG("[AVRCPAD] avrcpad_connect_cb() getsockopt: wMtu = %d. wFlushTo = %d. bMode = %d.", l2capConfigParam.wMtu, l2capConfigParam.wFlushTo, l2capConfigParam.bMode);
	BLUEZ_DEBUG_LOG("[AVRCPAD] avrcpad_connect_cb() getsockopt: bFcsType = %d. bMaxTransmit = %d. bTxWindowSize = %d.", l2capConfigParam.bFcsType, l2capConfigParam.bMaxTransmit, l2capConfigParam.bTxWindowSize);

	cidLocal = cid;

	connect_flg = TRUE;

	value = TRUE;
	memcpy( &avrcpad->l2capConfigParam, &l2capConfigParam, sizeof( L2CAP_CONFIG_PARAM ) );
	if( connection_initiator == TRUE && sdp_flg == TRUE ) {
		/* SDP 取得済み */
		avrcp_set_state(avrcpad, AVRCP_STATE_CONNECTED);
		send_signal_connectCfmL2cap(value, cidLocal, &avrcpad->dst, dstLen, status, avrcpad->version, avrcpad->supp_feat, &avrcpad->l2capConfigParam);
		btd_manager_connect_profile( &avrcpad->dst, MNG_CON_AVRCP, 0 , cidLocal );
	} else {
		ret = get_records(avrcpad->dev);
		if( ret < 0 ) {
			/* 接続は成功しているのでデフォルト情報で接続する */
			BLUEZ_DEBUG_WARN("[ AVRCPAD] get_records() ERROR : [ %s ] (%d) Connected by Default Infomation", strerror(-ret), -ret);
			ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, ret, 0);
			EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_GET_RECORD_CB, __LINE__, &dst);
			avrcp_set_state(avrcpad, AVRCP_STATE_CONNECTED);
			if( connection_initiator == TRUE ) {
				send_signal_connectCfmL2cap(value, cidLocal, &avrcpad->dst, dstLen, status, avrcpad->version, avrcpad->supp_feat, &avrcpad->l2capConfigParam);
			} else {
				send_signal_connectedL2cap(value, cidLocal, MNG_AVCTP_PSM, &avrcpad->dst, dstLen, identifier, avrcpad->version, avrcpad->supp_feat, &avrcpad->l2capConfigParam);
			}
			btd_manager_connect_profile( &avrcpad->dst, MNG_CON_AVRCP, 0 , cidLocal );
		}
	}

	return;

failed:
	BLUEZ_DEBUG_LOG("[AVRCPAD] avrcpad_connect_cb() failed.");
	memset( &l2capConfigParam, 0, sizeof(l2capConfigParam) );
	if( TRUE == connection_initiator ){ 
		linkinfo = btd_manager_find_connect_info_by_bdaddr( &avrcpad->dst );
		if( linkinfo == MNG_ACL_MAX ) {
			if( btd_manager_get_disconnect_info( &avrcpad->dst, MNG_CON_AVRCP ) == TRUE ) {
				value = FALSE;
			} else {
				value = 0xFF;
			}
		} else {
			value = FALSE;
		}
		send_signal_connectCfmL2cap(value, cid, &avrcpad->dst, dstLen, status, avrcpad->version, avrcpad->supp_feat, &l2capConfigParam);
	}
	avrcpad->svclass = 0;
	avrcp_set_state(avrcpad, AVRCP_STATE_DISCONNECTED);
	BLUEZ_DEBUG_INFO("[AVRCPAD] avrcpad_connect_cb(). failed: End.");
	return;
}

static void avrcpad_confirm_cb(GIOChannel *chan, gpointer data)
{
	struct avrcpad *avrcpad = NULL;
	struct audio_device *dev;
	char address[18];
	bdaddr_t src, dst;
	GError *err = NULL;
	int status;
	L2CAP_CONFIG_PARAM l2capConfigParam;

	BLUEZ_DEBUG_INFO("[AVRCPAD] avrcpad_confirm_cb() Start.");
	EVENT_DRC_BLUEZ_CB(FORMAT_ID_BT_CB, CB_AVRCP_REGISTER);

	bt_io_get(chan, BT_IO_L2CAP, &err,
			BT_IO_OPT_SOURCE_BDADDR, &src,
			BT_IO_OPT_DEST_BDADDR, &dst,
			BT_IO_OPT_DEST, address,
			BT_IO_OPT_INVALID);
	if (err) {
		BLUEZ_DEBUG_ERROR("[Bluez- AVRCPAD] bt_io_get() ERROR = [ %s ]", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_IO_GET, err->code, 0);
		g_error_free(err);
		g_io_channel_shutdown(chan, TRUE, NULL);
		return;
	}

	BLUEZ_DEBUG_WARN("[AVRCPAD] Confirm from [ %02x:%02x:%02x:%02x:%02x:%02x ]", dst.b[5], dst.b[4], dst.b[3], dst.b[2], dst.b[1], dst.b[0]);
	EVENT_DRC_BLUEZ_CONFIRM(FORMAT_ID_CONFIRM, CONFIRM_AVRCP, &dst);

	dev = manager_get_device(&src, &dst, TRUE);
	if (!dev) {
		BLUEZ_DEBUG_ERROR("[Bluez- AVRCPAD] Unable to get audio device object for %s", address);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_GET_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_GET_DEV, __LINE__, &dst);
		goto drop;
	}

	if (!manager_allow_avrcpad_connection(dev, &dst)) {
		BLUEZ_DEBUG_ERROR("[Bluez- AVRCPAD] Refusing avrcpad: too many existing connections");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_REFUSING, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_REFUSING, __LINE__, &dst);
		goto drop;
	}

	if( btd_manager_isconnect( MNG_CON_AVRCP ) == TRUE ) {
		/* 切断中 */
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] avrcp is still under connection");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_UNDER_CONNECTION, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_UNDER_CONNECTION, __LINE__, &dst);
		goto drop;
	}

	if (avrcpad_get_state(dev) > AVRCP_STATE_DISCONNECTED) {
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] Refusing new connection since one already exists");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_EXISTS, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_EXISTS, __LINE__, &dst);
		goto drop;
	}

	if (!dev->avrcpad) {
		btd_device_add_uuid(dev->btd_dev, AVRCP_TARGET_UUID);
		BLUEZ_DEBUG_LOG("[AVRCPAD] avrcpad_confirm_cb btd_device_add_uuid");
		if (!dev->avrcpad) {
			BLUEZ_DEBUG_ERROR("[Bluez- AVRCPAD] dev->avrcpad is nothing --> goto drop");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_ADD_UUID, 0, 0);
			EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_ADD_UUID, __LINE__, &dst);
			goto drop;
		}
	}

	avrcpad = dev->avrcpad;
	if (!avrcpad->io){
		avrcpad->io = g_io_channel_ref(chan);
	}
	memcpy( &avrcpad->dst, &dst, sizeof( bdaddr_t ) );

	disconnect_initiator = FALSE;
	avrcp_set_state(avrcpad, AVRCP_STATE_CONNECTING);

	memset( &l2capConfigParam, 0, sizeof(l2capConfigParam) );
	l2capSetCfgDefParam(&l2capConfigParam); 
	memcpy( &avrcpad->l2capConfigParam, &l2capConfigParam, sizeof( L2CAP_CONFIG_PARAM ) );

	status = l2cap_accept(dev);
	if (status < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] l2cap_accept() ERROR = %d", status);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_AUTHORIZATION, status, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_AUTHORIZATION, __LINE__, &dst);
		goto drop;
	}

	avrcpad->io_id = g_io_add_watch(chan, G_IO_ERR | G_IO_HUP | G_IO_NVAL, avrcpad_cb, avrcpad);

	BLUEZ_DEBUG_INFO("[AVRCPAD] avrcpad_confirm_cb() End.");

	return;

drop:
	g_io_channel_shutdown(chan, TRUE, NULL);
}

static sdp_record_t *avrcp_record(void)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *adapseq;
	sdp_list_t *aproto, *aproto2, *proto[2], *proto2[2];
	uuid_t l2cap, avctp, avrct, avvc;
	sdp_profile_desc_t profile;
	sdp_record_t *record;
	sdp_data_t *psm, *version, *features;
	sdp_data_t *psm2, *version2;
	uint16_t lp = MNG_AVCTP_PSM;
	uint16_t lp2 = MNG_BROWSING_PSM;
	uint16_t avrcp_ver = 0x0104;
	uint16_t avctp_ver = 0x0103;
	uint16_t feat = 0x0041;

	record = sdp_record_alloc();
	if (!record){
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] sdp_record_alloc() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SDP_RECORD, 0, 0);
		return NULL;
	}

	/* Service Class ID List */
	sdp_uuid16_create(&avrct, AV_REMOTE_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &avrct);
	sdp_uuid16_create(&avvc, VIDEO_CONF_SVCLASS_ID);
	svclass_id = sdp_list_append(svclass_id, &avvc);
	sdp_set_service_classes(record, svclass_id);

	/* Protocol Descriptor List */
	//L2CAP_UUID	0x0100
	sdp_uuid16_create(&l2cap, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap);
	//MNG_AVCTP_PSM(23) 0x17
	psm = sdp_data_alloc(SDP_UINT16, &lp);
	proto[0] = sdp_list_append(proto[0], psm);
	apseq = sdp_list_append(0, proto[0]);

	//AVCTP_UUID	0x0017
	sdp_uuid16_create(&avctp, AVCTP_UUID);
	proto[1] = sdp_list_append(0, &avctp);
	//avctp_ver 0x0103
	version = sdp_data_alloc(SDP_UINT16, &avctp_ver);
	proto[1] = sdp_list_append(proto[1], version);
	apseq = sdp_list_append(apseq, proto[1]);

	/* Bluetooth Profile Descriptor List */
	//AV_REMOTE_PROFILE_ID	0x110e
	sdp_uuid16_create(&profile.uuid, AV_REMOTE_PROFILE_ID);
	//avrcp_ver 0x0104
	profile.version = avrcp_ver;
	pfseq = sdp_list_append(0, &profile);
	sdp_set_profile_descs(record, pfseq);

	/* Additional Protocol Descriptor List */
	//L2CAP_UUID	0x0100
	proto2[0] = sdp_list_append(0, &l2cap);
	//MNG_BROWSING_PSM(27) 0x1b
	psm2 = sdp_data_alloc(SDP_UINT16, &lp2);
	proto2[0] = sdp_list_append(proto2[0], psm2);
	adapseq = sdp_list_append(0, proto2[0]);


	//AVCTP_UUID	0x0017
	sdp_uuid16_create(&avctp, AVCTP_UUID);
	proto2[1] = sdp_list_append(0, &avctp);
	//avctp_ver 0x0103
	version2 = sdp_data_alloc(SDP_UINT16, &avctp_ver);
	proto2[1] = sdp_list_append(proto2[1], version2);
	adapseq = sdp_list_append(adapseq, proto2[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(record, aproto);
	aproto2 = sdp_list_append(0, adapseq);
	sdp_set_add_access_protos(record, aproto2);

	features = sdp_data_alloc(SDP_UINT16, &feat);
	sdp_attr_add(record, SDP_ATTR_SUPPORTED_FEATURES, features);

	sdp_set_info_attr_for_avp(record, "A/V RemoteControl");

	sdp_data_free(psm);
	sdp_data_free(version);
	sdp_data_free(psm2);
	sdp_data_free(version2);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(proto2[0], 0);
	sdp_list_free(proto2[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(adapseq, 0);
	sdp_list_free(pfseq, 0);
	sdp_list_free(aproto, 0);
	sdp_list_free(aproto2, 0);
	sdp_list_free(svclass_id, 0);

	return record;
}

static int avrcpad_set_data(struct avrcpad *avrcpad, const sdp_record_t *record)
{
	sdp_list_t *profile;
	sdp_profile_desc_t *profDesc;
	int features = 0;

	BLUEZ_DEBUG_INFO("[AVRCPAD] avrcpad_set_data() Strat");
	if (sdp_get_access_protos(record, &profile) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez- AVRCPAD] Unable to get access protos from avrcpad_set_data_supp record");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_AVRCP_SET_DATA, 0, 0);
		return -1;
	}

	/* Get Version */
	if (sdp_get_profile_descs(record, &profile) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez- AVRCPAD] Unable to get profile from avrcpad record");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_AVRCP_SET_DATA, 0, 0);
		return -1;
	}

	if (profile == NULL) {
		BLUEZ_DEBUG_ERROR("[Bluez- AVRCPAD] profile is NULL ");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_AVRCP_SET_DATA, 0, 0);
		return -1;
	}

	profDesc = (sdp_profile_desc_t*)profile->data;
	avrcpad->version = profDesc->version;
	BLUEZ_DEBUG_LOG("[AVRCPAD] avrcpad_set_data() avrcpad->version = %04x.", avrcpad->version);
	sdp_list_free(profile, NULL);

	/* Get Supported Features */
	features = sdp_get_avrcp_supp_feat(record);
	if(features == -1){
		BLUEZ_DEBUG_ERROR("[Bluez- AVRCPAD] Unable to get AVRCP supp_feat from avrcpad sdp_record");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_AVRCP_SET_DATA, 0, 0);
		return -1;
	}
	avrcpad->supp_feat = features;
	BLUEZ_DEBUG_LOG("[AVRCPAD] avrcpad_set_data() avrcpad->supp_feat = %04x.", avrcpad->supp_feat);
	
	BLUEZ_DEBUG_INFO("[AVRCPAD] avrcpad_set_data() End.");
	return 0;
}

static void get_record_cb(sdp_list_t *recs, int err, gpointer user_data)
{
	struct audio_device *dev = user_data;
	struct avrcpad *avrcpad = dev->avrcpad;
	sdp_record_t *record = NULL;
	sdp_list_t *r;
	uuid_t uuid;
	int dstLen = sizeof(bdaddr_t);
	int linkinfo = 0;
	L2CAP_CONFIG_PARAM l2capConfigParam;
	uint32_t value = FALSE;
	uint16_t cid = 0;
	uint16_t status = 0;
	sdp_list_t *classes;
	uuid_t class;
	char identifier = 0;
	int ret = 0;

	BLUEZ_DEBUG_INFO("[AVRCPAD] get_record_cb(). Start.");
	EVENT_DRC_BLUEZ_CB(FORMAT_ID_BT_CB, CB_AVRCP_GET_RECORD);

	if( avrcpad == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] avrcpad == NULL.");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
		return;
	}

	if (avrcpad->sdp_timer) {
		g_source_remove(avrcpad->sdp_timer);
		avrcpad->sdp_timer = 0;
	}

	avrcpad->version = AVRCP_DEFAULT_VERSION;
	avrcpad->supp_feat = AVRCP_DEFAULT_FEAT;

	if (err < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] Unable to get service record: [ %s ] (%d)",strerror(-err), -err);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, err, 0);
		goto failed;
	}

	if (!recs || !recs->data) {
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] No records found");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
		goto failed;
	}

	sdp_uuid16_create(&uuid, avrcpad->svclass);
	for (r = recs; r != NULL; r = r->next) {
		record = r->data;
		if (sdp_get_service_classes(record, &classes) < 0) {
			BLUEZ_DEBUG_LOG("[AVRCPAD] Unable to get service classes from record");
			continue;
		}

		if( classes == NULL ){
			BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] sdp_get_service_classes() is failed");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
			goto failed;
		}

		memcpy(&class, classes->data, sizeof(uuid));

		sdp_list_free(classes, free);

		if (sdp_uuid_cmp(&class, &uuid) == 0)
		{
			break;
		}
	}
	if (r == NULL) {
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] No record found with UUID 0x%04x", avrcpad->svclass);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, avrcpad->svclass, 0);
		goto failed;
	}
	BLUEZ_DEBUG_LOG("[AVRCPAD] get_record_cb() record = %p.", record);

	if (avrcpad_set_data(avrcpad, record) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] Unable to extract AVRCP VERSION from service record");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
		goto failed;
	}

	avrcpad->svclass = 0;
	sdp_flg = TRUE;

	if( connect_flg == FALSE ) {
		ret = l2cap_connect(dev);
		if( ret < 0 ) {
			/* L2CAP の接続に失敗したので、接続失敗を上げる */
			BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] l2cap_connect() ERROR = %d", ret);
			ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_L2CAP_CONNECT, ret, 0);
			EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_L2CAP_CONNECT, __LINE__, &avrcpad->dst);
			avrcp_set_state(avrcpad, AVRCP_STATE_DISCONNECTED);
			value = FALSE;
			send_signal_connectCfmL2cap(value, cid, &avrcpad->dst, dstLen, status, avrcpad->version, avrcpad->supp_feat, &l2capConfigParam);
		}
	} else {
		value = TRUE;
		avrcp_set_state(avrcpad, AVRCP_STATE_CONNECTED);
		if(TRUE == connection_initiator){
			send_signal_connectCfmL2cap(value, cidLocal, &avrcpad->dst, dstLen, status, avrcpad->version, avrcpad->supp_feat, &avrcpad->l2capConfigParam);
		} else {
			send_signal_connectedL2cap(value, cidLocal, MNG_AVCTP_PSM, &avrcpad->dst, dstLen, identifier, avrcpad->version, avrcpad->supp_feat, &avrcpad->l2capConfigParam);
		}
		btd_manager_connect_profile( &avrcpad->dst, MNG_CON_AVRCP, 0 , cidLocal );
	}

	BLUEZ_DEBUG_INFO("[AVRCPAD] get_record_cb(). End.");
	return;

failed:
	BLUEZ_DEBUG_LOG("[AVRCPAD] get_record_cb() failed.");
	memset( &l2capConfigParam, 0, sizeof(l2capConfigParam) );
	avrcpad->svclass = 0;
	linkinfo = btd_manager_find_connect_info_by_bdaddr( &avrcpad->dst );
	if( linkinfo == MNG_ACL_MAX ) {
		if( btd_manager_get_disconnect_info( &avrcpad->dst, MNG_CON_AVRCP ) == TRUE ) {
			value = FALSE;
		} else {
			value = 0xFF;
		}
		/* ACL が切断されているため、接続処理終了 */
		if( connection_initiator ) {
			send_signal_connectCfmL2cap(value, cid, &avrcpad->dst, dstLen, status, avrcpad->version, avrcpad->supp_feat, &l2capConfigParam);
		}
		avrcp_set_state(avrcpad, AVRCP_STATE_DISCONNECTED);
	} else {
		/* ACL は接続できているので、とりあえず接続処理を続行する */
		if( connection_initiator && connect_flg == FALSE ) {
			ret = l2cap_connect(dev);
			if( ret < 0 ) {
				/* L2CAP の接続に失敗したので、接続失敗を上げる */
				BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] l2cap_connect() ERROR = %d", ret);
				ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_L2CAP_CONNECT, ret, 0);
				EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_L2CAP_CONNECT, __LINE__, &avrcpad->dst);
				avrcp_set_state(avrcpad, AVRCP_STATE_DISCONNECTED);
				value = FALSE;
				send_signal_connectCfmL2cap(value, cid, &avrcpad->dst, dstLen, status, avrcpad->version, avrcpad->supp_feat, &l2capConfigParam);
			}
		} else {
			/* デフォルト情報で接続する */
			BLUEZ_DEBUG_WARN("[ AVRCPAD] get_records() ERROR : Connected by Default Infomation");
			value = TRUE;
			avrcp_set_state(avrcpad, AVRCP_STATE_CONNECTED);
			if(TRUE == connection_initiator){
				send_signal_connectCfmL2cap(value, cidLocal, &avrcpad->dst, dstLen, status, avrcpad->version, avrcpad->supp_feat, &avrcpad->l2capConfigParam);
			} else {
				send_signal_connectedL2cap(value, cidLocal, MNG_AVCTP_PSM, &avrcpad->dst, dstLen, identifier, avrcpad->version, avrcpad->supp_feat, &avrcpad->l2capConfigParam);
			}
			btd_manager_connect_profile( &avrcpad->dst, MNG_CON_AVRCP, 0 , cidLocal );
		}
	}

	BLUEZ_DEBUG_INFO("[AVRCPAD] get_record_cb(). failed: End.");
	return;
}

static gboolean sdp_timer_cb(gpointer data)
{
	struct audio_device *device = data;
	struct avrcpad *avrcpad = device->avrcpad;
	int value = TRUE;
	uint16_t status = 0;
	char identifier = 0;
	int dstLen = sizeof(bdaddr_t);

	BLUEZ_DEBUG_INFO("[AVRCPAD] sdp_timer_cb() Strat");

	g_source_remove(avrcpad->sdp_timer);
	avrcpad->sdp_timer = 0;

	if( avrcpad_get_state(device) != AVRCP_STATE_CONNECTING ) {
		BLUEZ_DEBUG_LOG("[AVRCPAD] alredy connected or disconnected");
		return TRUE;
	}

	BLUEZ_DEBUG_WARN("[AVRCPAD] SDP Time Out Connected by Default Infomation");
	ERR_DRC_BLUEZ(ERROR_BLUEZ_SDP_TIMEOUT, 0, 0);

	bt_cancel_discovery2(&device->src, &device->dst, avrcpad->svclass);
	avrcpad->version = AVRCP_DEFAULT_VERSION;
	avrcpad->supp_feat = AVRCP_DEFAULT_FEAT;
	avrcpad->svclass = 0;

	avrcp_set_state(avrcpad, AVRCP_STATE_CONNECTED);
	if(TRUE == connection_initiator){
		send_signal_connectCfmL2cap(value, cidLocal, &avrcpad->dst, dstLen, status, avrcpad->version, avrcpad->supp_feat, &avrcpad->l2capConfigParam);
	} else {
		send_signal_connectedL2cap(value, cidLocal, MNG_AVCTP_PSM, &avrcpad->dst, dstLen, identifier, avrcpad->version, avrcpad->supp_feat, &avrcpad->l2capConfigParam);
	}
	btd_manager_connect_profile( &avrcpad->dst, MNG_CON_AVRCP, 0 , cidLocal );

	BLUEZ_DEBUG_INFO("[AVRCPAD] sdp_timer_cb(). End.");
	return TRUE;
}

static int get_records(struct audio_device *device)
{
	struct avrcpad *avrcpad = device->avrcpad;
	uint16_t svclass;
	uuid_t uuid;
	int err;

	BLUEZ_DEBUG_INFO("[AVRCPAD] get_records() Strat");
	svclass = AV_REMOTE_TARGET_SVCLASS_ID;

	sdp_uuid16_create(&uuid, svclass);
	BLUEZ_DEBUG_LOG("[AVRCPAD] svclass = 0x%x.", svclass);

	err = bt_search_service2(&device->src, &device->dst, &uuid, get_record_cb, device, NULL);
	if (err < 0)
	{
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] bt_search_service2() err = %d.", err);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORDS, err, 0);
		return err;
	}

	avrcp_set_state(avrcpad, AVRCP_STATE_CONNECTING);

	avrcpad->svclass = svclass;

	avrcpad->sdp_timer = g_timeout_add_seconds(AVRCP_SDP_TIMER, sdp_timer_cb, device);

	BLUEZ_DEBUG_INFO("[AVRCPAD] get_records(). End.");
	return 0;
}

static DBusMessage *register_l2cap_avrcp(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	const char *srcAddr;
	uint32_t addrlen;
	bdaddr_t src;
	uint16_t avrcpPsm;
	uint16_t browsingPsm;
	gboolean master = TRUE;
	GIOChannel *io;
	GError *err = NULL;
	uint16_t value = 0x00;
	sdp_record_t *record;
	gboolean tmp;
	int ret;

	BLUEZ_DEBUG_INFO("[AVRCPAD] register_l2cap_avrcp() Start.");

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_AVRCP_REGISTER, NULL);

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_UINT16, &avrcpPsm,
						DBUS_TYPE_UINT16, &browsingPsm,
						DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
						&srcAddr, &addrlen,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[AVRCPAD] register_l2cap_avrcp() avrcpPsm = %d browsingPsm = %d", avrcpPsm, browsingPsm);
	BLUEZ_DEBUG_LOG("[AVRCPAD] register_l2cap_avrcp() srcAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", srcAddr[5], srcAddr[4], srcAddr[3], srcAddr[2], srcAddr[1], srcAddr[0]);

	bluezutil_get_bdaddr( &src );

	BLUEZ_DEBUG_LOG("[AVRCPAD] register_l2cap_avrcp() avrcp_record Start.");
	
	if (!g_avrcpad_record_id) {
		record = avrcp_record();
		if (!record) {
			BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] Unable to allocate new service record");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_SDP_RECORD, 0, 0);
			return NULL;
		}

		if (add_record_to_server(&src, record) < 0) {
			BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] Unable to register AVRCP controller service record");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_ADD_RECORD, 0, 0);
			sdp_record_free(record);
			return NULL;
		}
		g_avrcpad_record_id = record->handle;
	}

	if (!g_avrcpad_l2cap_io) {
		io = bt_io_listen(BT_IO_L2CAP, NULL, avrcpad_confirm_cb, NULL,
					NULL, &err,
					BT_IO_OPT_SOURCE_BDADDR, &src,
					BT_IO_OPT_PSM, avrcpPsm,
					BT_IO_OPT_SEC_LEVEL, BT_IO_SEC_MEDIUM,
					BT_IO_OPT_MASTER, master,
					BT_IO_OPT_INVALID);
		
		if (!io) {
			BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] bt_io_listen() err = [ %s ]", err->message);
			ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_IO_LISTEN, err->code, 0);
			g_error_free(err);
			value = 0x01;
		}
		g_avrcpad_l2cap_io = io;
	}
	
	BLUEZ_DEBUG_INFO("[AVRCPAD] register_l2cap_avrcp() End value = %d", value);

	connection_initiator = FALSE;
	disconnect_initiator = FALSE;

	ret = manager_register_browsing( &src, browsingPsm );
	if( ret != TRUE ) {
		value = 0x01;
	}

	send_signal_registerCfmL2cap(value);

	return dbus_message_new_method_return(msg);
}


static DBusMessage *deregister_l2cap_avrcp(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	uint16_t value = TRUE;
	uint16_t avrcpPsm;
	uint16_t browsingPsm;
	const char *srcAddr;
	uint32_t addrlen;
	int ret;
	bdaddr_t src;

	BLUEZ_DEBUG_INFO("[AVRCPAD] deregister_l2cap_avrcp() Start.");

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_AVRCP_DEREGISTER, NULL);

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_UINT16, &avrcpPsm,
						DBUS_TYPE_UINT16, &browsingPsm,
						DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
						&srcAddr, &addrlen,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DEREGISTER, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[AVRCPAD] deregister_l2cap_avrcp() avrcpPsm = %x browsingPsm = %x", avrcpPsm, browsingPsm);
	BLUEZ_DEBUG_LOG("[AVRCPAD] deregister_l2cap_avrcp() srcAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", srcAddr[5], srcAddr[4], srcAddr[3], srcAddr[2], srcAddr[1], srcAddr[0]);

	bluezutil_get_bdaddr( &src );

	ret = manager_deregister_browsing( &src, browsingPsm );
	if( ret != TRUE ) {
		value = FALSE;
	}
	send_signal_deregisterCfmL2cap(value);

	BLUEZ_DEBUG_INFO("[AVRCPAD] deregister_l2cap_avrcp() End.");
	return dbus_message_new_method_return(msg);
}

int l2cap_connect(struct audio_device *dev)
{
	GError *err = NULL;
	GIOChannel *io;
	struct avrcpad *avrcpad = dev->avrcpad;

	BLUEZ_DEBUG_INFO("[AVRCPAD] l2cap_connect() Start.");

	io = bt_io_connect(BT_IO_L2CAP, avrcpad_connect_cb, avrcpad, NULL, &err,
				BT_IO_OPT_SOURCE_BDADDR, &dev->src,
				BT_IO_OPT_DEST_BDADDR, &dev->dst,
				BT_IO_OPT_PSM, l2capPsmData,
				BT_IO_OPT_INVALID);
	if (err) {
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] bt_io_connec()t ERROR = [ %s ]", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_IO_CONNECT, err->code, 0);
		g_error_free(err);
		return -EIO;
	}
	
	avrcpad->io = io;	
	BLUEZ_DEBUG_INFO("[AVRCPAD] l2cap_connect() io = %p", io);
	BLUEZ_DEBUG_INFO("[AVRCPAD] l2cap_connect() End.");
	return 0;
}

int l2cap_accept(struct audio_device *dev)
{
	GError *err = NULL;
	GIOChannel *io;
	struct avrcpad *avrcpad = dev->avrcpad;
	
	BLUEZ_DEBUG_INFO("[AVRCPAD] l2cap_accept() Start.");

	if (!bt_io_accept(avrcpad->io, avrcpad_connect_cb, avrcpad, NULL, &err)) {
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] bt_io_accept() ERROR = [ %s ]", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_IO_ACCEPT, err->code, 0);
		g_error_free(err);
		return -EIO;
	}
	
	BLUEZ_DEBUG_INFO("[AVRCPAD] l2cap_accept() End.");
	return 0;
}


static DBusMessage *connect_l2cap_avrcp(DBusConnection *conn, DBusMessage *msg, void *data)
{
	uint16_t status = 0;
	const char *dstAddr;
	uint16_t l2capPsm;
	uint32_t dstLen;
	bdaddr_t src;
	bdaddr_t dst;
	uint16_t version = 0;
	uint16_t supp_feat = 0;
	uint32_t value = TRUE;
	L2CAP_CONFIG_PARAM l2capConfigParam;
	uint16_t cid  = 0;
	struct avrcpad *avrcpad = NULL;
	int linkinfo = 0;
	struct audio_device *device = NULL;
	char address[18];
	char dstAddress[18];
	int ret = 0;

	BLUEZ_DEBUG_INFO("[AVRCPAD] connect_l2cap_avrcp() Start.");

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_UINT16, &l2capPsm,
						DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
						&dstAddr, &dstLen,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_WARN("[AVRCPAD] connect_l2cap_avrcp() l2capPsm = %d dstAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", l2capPsm, dstAddr[5], dstAddr[4], dstAddr[3], dstAddr[2], dstAddr[1], dstAddr[0]);

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_AVRCP_CONNECT, dstAddr);

	bluezutil_get_bdaddr( &src );

	memcpy( &dst, dstAddr, sizeof(bdaddr_t) );
	device = manager_find_device(NULL, NULL, &dst, AUDIO_AVRCPAD_INTERFACE, FALSE);
	if ( device == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] device is NULL.");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_FIND_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_FIND_DEV, __LINE__, &dst);
		goto failed;
	}
	BLUEZ_DEBUG_LOG("[AVRCPAD]device = %p.", device);

	if (!manager_allow_avrcpad_connection(device, &dst)) {
		/* 接続が拒否された場合 */
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] Refusing avrcp: too many existing connections");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_REFUSING, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_REFUSING, __LINE__, &dst);
		goto failed;
	}

	if( AVRCP_STATE_CONNECTING == avrcpad_get_state( device ) ) {
		/* 相手からの接続中 */
		BLUEZ_DEBUG_WARN("[AVRCPAD] AVRCP is connecting");
		/* 自分から接続（相手からの接続シーケンスで接続を完了させる） */
		l2capPsmData = l2capPsm;
		connection_initiator = TRUE;
		disconnect_initiator = FALSE;
		EVENT_DRC_BLUEZ_CHGMSG(FORMAT_ID_CHGMSG, CHANGE_MESSAGE_AVRCP, &dst);
		return dbus_message_new_method_return(msg);
	}

	if( btd_manager_isconnect( MNG_CON_AVRCP ) == TRUE ) {
		/* 切断中 / 接続済み */
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] avrcp is still under connection");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_UNDER_CONNECTION, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_UNDER_CONNECTION, __LINE__, &dst);
		goto failed;
	}

	ba2str(&src, address);
	ba2str(&dst, dstAddress);
	BLUEZ_DEBUG_LOG("[AVRCPAD] l2cap_connect()  dst = %s. src = %s.", dstAddress, address);

	l2capPsmData = l2capPsm;
	connection_initiator = TRUE;
	disconnect_initiator = FALSE;
	avrcpad =  device->avrcpad;
	memcpy( &avrcpad->dst, &dst, sizeof( bdaddr_t ) );

	memset( &l2capConfigParam, 0, sizeof(l2capConfigParam) );
	l2capSetCfgDefParam(&l2capConfigParam); 
	memcpy( &avrcpad->l2capConfigParam, &l2capConfigParam, sizeof( L2CAP_CONFIG_PARAM ) );
	ret = get_records(device);
	if( ret < 0 ) {
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] get_records() ERROR : [ %s ] (%d)", strerror(-ret), -ret);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, ret, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_GET_RECORD_CB, __LINE__, &dst);
		goto failed;
	}

	BLUEZ_DEBUG_INFO("[AVRCPAD] connect_l2cap_avrcp() End.");
	return dbus_message_new_method_return(msg);

failed:
	linkinfo = btd_manager_find_connect_info_by_bdaddr( &dst );
	if( linkinfo == MNG_ACL_MAX ) {
		if( btd_manager_get_disconnect_info( &dst, MNG_CON_AVRCP ) == TRUE ) {
			value = FALSE;
		} else {
			value = 0xFF;
		}
	} else {
		value = FALSE;
	}
	memset( &l2capConfigParam, 0, sizeof(l2capConfigParam) );
	send_signal_connectCfmL2cap(value, cid, &dst, dstLen, status, version, supp_feat, &l2capConfigParam);

	return dbus_message_new_method_return(msg);
}

static void avrcpad_disconnected(struct audio_device *dev)
{
	struct avrcpad *avrcpad = dev->avrcpad;

	if (!avrcpad) {
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] avrcpad is NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_NULL, 0, 0);
		return;
	}

	if (avrcpad->svclass) {
		bt_cancel_discovery2(&dev->src, &dev->dst, avrcpad->svclass);
		avrcpad->svclass = 0;
	}

	if (avrcpad->sdp_timer) {
		g_source_remove(avrcpad->sdp_timer);
		avrcpad->sdp_timer = 0;
	}

	if (avrcpad->io) {
		g_io_channel_shutdown(avrcpad->io, TRUE, NULL);
		g_io_channel_unref(avrcpad->io);
		avrcpad->io = NULL;
	}

	if (avrcpad->io_id) {
		g_source_remove(avrcpad->io_id);
		avrcpad->io_id = 0;
	}

	return;
}

static void avrcp_set_state(struct avrcpad *control, avrcpad_state_t new_state)
{
	GSList *l;
	struct audio_device *dev = control->dev;
	avrcpad_state_t old_state = control->state;
	gboolean value;

	switch (new_state) {
	case AVRCP_STATE_DISCONNECTED:
		BLUEZ_DEBUG_INFO("[AVRCPAD] AVCTP Disconnected");
		avrcpad_disconnected(control->dev);
		connection_initiator = FALSE;	//接続方向初期化
		connect_flg = FALSE;	//接続処理の有無
		sdp_flg =FALSE;	//SDP 処理有無
		break;
	case AVRCP_STATE_CONNECTING:
		BLUEZ_DEBUG_LOG("[AVRCPAD] AVCTP Connecting");
		break;
	case AVRCP_STATE_CONNECTED:
		BLUEZ_DEBUG_LOG("[AVRCPAD] AVCTP Connected");
		break;
	default:
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] Invalid AVCTP state = %d", new_state);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SET_STATE, new_state, 0);
		return;
	}
	control->state = new_state;
	EVENT_DRC_BLUEZ_STATE(FORMAT_ID_STATE, STATE_AVRCP, &dev->dst, new_state);
	btd_manager_set_state_info( &dev->dst, MNG_CON_AVRCP, new_state );
	return;
}


static void avrcp_disconnect(struct audio_device *dev)
{
	struct avrcpad *avrcpad = dev->avrcpad;
	uint16_t value= TRUE;

	if (!(avrcpad && avrcpad->io)) {
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] avrcpad is already disconnected");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_NULL, 0, 0);
		return;
	}

	avrcp_set_state(avrcpad, AVRCP_STATE_DISCONNECTED);
	return;
}


static DBusMessage *disconnect_l2cap_avrcp(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	struct audio_device *device;
	const char *dstAddr;
	uint32_t len;
	bdaddr_t dst;
	uint16_t l2capPsm;

	BLUEZ_DEBUG_INFO("[AVRCPAD] disconnect_l2cap_avrcp() Start.");

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_UINT16, &l2capPsm,
						DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
						&dstAddr, &len,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[AVRCPAD] disconnect_l2cap_avrcp(), dstAddr = [ %02x:%02x:%02x:%02x:%02x:%02x]", dstAddr[5], dstAddr[4], dstAddr[3], dstAddr[2], dstAddr[1], dstAddr[0]);

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_AVRCP_DISCONNECT, dstAddr);

	memcpy( &dst, dstAddr, sizeof(bdaddr_t) );
	device = manager_find_device(NULL, NULL, &dst, AUDIO_AVRCPAD_INTERFACE , FALSE);
	if ( device == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] device ERR. device = NULL.");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_FIND_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DISCONNECT_FIND_DEV, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}

	if( AVRCP_STATE_DISCONNECTED != avrcpad_get_state(device) ) {
		disconnect_initiator = TRUE;
	}

	avrcp_disconnect(device);

	/* 既に切断通知をあげているか判定 */
	if( btd_manager_isconnect( MNG_CON_AVRCP ) == FALSE ) {
		BLUEZ_DEBUG_LOG("[AVRCPAD] avrcp already disconnected");
		btd_manager_avrcp_signal_disconnect_cfm( FALSE, cidLocal );
		disconnect_initiator = FALSE;
	}

	BLUEZ_DEBUG_INFO("[AVRCPAD] disconnect_l2cap_avrcp() End.");

	return dbus_message_new_method_return(msg);
}


static DBusMessage *data_snd_l2cap_avrcp(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	struct audio_device *device;
	const char *senddata;
	va_list ap;
	uint32_t len;
	const char *dstAddr;
	uint32_t addrlen;
	bdaddr_t dst;
	uint16_t l2capPsm;

	BLUEZ_DEBUG_INFO("[AVRCPAD] data_snd_l2cap_avrcp() Start.");

	if (!dbus_message_get_args(msg, NULL,
							   DBUS_TYPE_UINT16, &l2capPsm,
							   DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
							   &dstAddr, &addrlen,
							   DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
							   &senddata, &len,
							   DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[AVRCPAD] data_snd_l2cap_avrcp() senddata = %s. len = %d.", senddata, len);
	BLUEZ_DEBUG_LOG("[AVRCPAD] data_snd_l2cap_avrcp() dstAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", dstAddr[5], dstAddr[4], dstAddr[3], dstAddr[2], dstAddr[1], dstAddr[0]);

	memcpy( &dst, dstAddr, sizeof(bdaddr_t) );
	device = manager_find_device(NULL, NULL, &dst, AUDIO_AVRCPAD_INTERFACE, FALSE);
	if ( device == NULL || device->avrcpad == NULL || device->avrcpad->io == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] ERROR. avrcp is already closed.");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_FIND_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DATASEND_FIND_DEV, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}

	struct avrcpad *avrcpad = device->avrcpad;
	int sk = g_io_channel_unix_get_fd(avrcpad->io);

	BLUEZ_DEBUG_LOG("[AVRCPAD] data_snd_l2cap_avrcp() sk = %d.", sk);
	if (write(sk, senddata, len) < 0){
		BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] data_snd_l2cap_avrcp() write() ERR. errno = %s(%d).", strerror(errno), errno);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_WRITE, errno, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DATASEND_WRITE, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}

	BLUEZ_DEBUG_LOG("[AVRCPAD] data_snd_l2cap_avrcp() End.");
	return dbus_message_new_method_return(msg);
}

static GDBusMethodTable avrcpad_methods[] = {
	{ "REGISTER_L2CAP", 	"qqay",		"", register_l2cap_avrcp,	0,	0	},
	{ "DEREGISTER_L2CAP",	"qqay",		"", deregister_l2cap_avrcp,	0,	0	},
	{ "CONNECT_L2CAP",		"qay",		"", connect_l2cap_avrcp,	0,	0	},
	{ "DISCONNECT_L2CAP",	"qay",		"", disconnect_l2cap_avrcp,	0,	0	},
	{ "DATA_SND_L2CAP", 	"qayay",	"", data_snd_l2cap_avrcp,	0,	0	},
	{ NULL, NULL, NULL, NULL, 0, 0 }
};

static GDBusSignalTable avrcpad_signals[] = {
	{ "REGISTER_CFM_L2CAP", 	"q",							G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "DEREGISTER_CFM_L2CAP",	"q",							G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "CONNECT_CFM_L2CAP",		"uuuuuuqqqqqqqqqqqyyyyyay",		G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "DISCONNECT_CFM_L2CAP",	"qq",							G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "CONNECTED_L2CAP",		"uuuuuuqqqqqqqqqqqyyyyyyay",	G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "DISCONNECTED_L2CAP", 	"qq",							G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "DATA_RCV_L2CAP", 		"uqay",							G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ NULL, NULL, G_DBUS_SIGNAL_FLAG_DEPRECATED }
};


static void path_unregister(void *data)
{
	BLUEZ_DEBUG_LOG("[AVRCPAD] Unregistered interface %s on path %s", AUDIO_AVRCPAD_INTERFACE, "/");
}

void avrcpad_unregister(struct audio_device *dev)
{
	struct avrcpad *avrcpad = dev->avrcpad;
	L2CAP_CONFIG_PARAM l2capConfigParam;

	BLUEZ_DEBUG_LOG("[AVRCPAD] avrcpad unregister");

	if ( connection_initiator == TRUE && avrcpad->state == AVRCP_STATE_CONNECTING ) {
		/* ACL が張られていない場合、接続完了通知（失敗）をあげる必要がある */
		memset( &l2capConfigParam, 0, sizeof(l2capConfigParam) );
		send_signal_connectCfmL2cap(0xFF, 0, &dev->dst, 6, 0, 0, 0, &l2capConfigParam);
	}
	avrcpad_disconnected(dev);

	g_free(avrcpad);

	dev->avrcpad = NULL;
	return;
}

struct avrcpad	*avrcpad_init(struct audio_device *dev)
{
	struct avrcpad *avrcpad;

	BLUEZ_DEBUG_INFO("[AVRCPAD] avrcp_init() Start.");
	EVENT_DRC_BLUEZ_INIT(FORMAT_ID_BT_INIT, INIT_AVRCP);

	avrcpad = g_new0(struct avrcpad, 1);

	avrcpad->dev = dev;
	avrcpad->state = AVRCP_STATE_DISCONNECTED;

	BLUEZ_DEBUG_INFO("[AVRCPAD] avrcp_init() End.");

	return avrcpad;
}


int avrcpad_dbus_init(void)
{
	BLUEZ_DEBUG_INFO("[AVRCPAD] avrcp_dbus_init() Start.");
	EVENT_DRC_BLUEZ_INIT(FORMAT_ID_BT_INIT, INIT_DBUS_AVRCP);

	if (g_avrcpad_connection != NULL  ) {
		g_dbus_unregister_interface(g_avrcpad_connection, "/" ,AUDIO_AVRCPAD_INTERFACE);
		g_avrcpad_connection = NULL;
	}

	g_avrcpad_connection = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);

	if (g_dbus_register_interface(g_avrcpad_connection, "/",
					AUDIO_AVRCPAD_INTERFACE,
					avrcpad_methods, avrcpad_signals,
					NULL, NULL, path_unregister) == FALSE) {
		 BLUEZ_DEBUG_ERROR("[Bluez - AVRCPAD] g_dbus_register_interface() ERROR");
		 ERR_DRC_BLUEZ(ERROR_BLUEZ_DBUS_INIT, 0, 0);
		 return -1;
	}

	btd_manager_set_connection_avrcp( g_avrcpad_connection );
	BLUEZ_DEBUG_INFO("[AVRCPAD] avrcp_dbus_init() End.");
	return 0;
}

avrcpad_state_t avrcpad_get_state(struct audio_device *dev)
{
	struct avrcpad *avrcpad = dev->avrcpad;

	if( avrcpad == NULL ) {
		return AVRCP_STATE_DISCONNECTED;
	}
	return avrcpad->state;
}
uint16_t avrcpad_get_cid(void)
{
	return cidLocal;
}
int avrcpad_get_disconnect_initiator(void)
{
	return disconnect_initiator;
}
void avrcpad_set_disconnect_initiator(int flg)
{
	disconnect_initiator = flg;
	return;
}

