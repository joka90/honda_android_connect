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
#include "a2dpad.h"
#include "avrcpad.h"
#include "browsing.h"
#include "dbus-common.h"
#include "../src/adapter.h"
#include "../src/device.h"
#include "glib-helper.h"
#include "telephony.h"
#include "uinput.h"
#include "../src/hcicommand.h"
#include "sdpd.h"
#include "../src/manager.h"

#include <utils/Log.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include "../btio/btio.h"
#include "manager.h"
#include "../src/recorder.h"

/* Source file ID */
#define BLUEZ_SRC_ID BLUEZ_SRC_ID_AUDIO_A2DPAD_C

#define A2DP_SDP_TIMER 10

struct avdtp_stream {
	GIOChannel *io;
	uint16_t imtu;
	uint16_t omtu;
	guint io_id;		/* Transport GSource ID */
};

struct a2dpad  {

	struct audio_device *dev;
	bdaddr_t dst;
	a2dpad_state_t state;
	GIOChannel *io;
	guint io_id;
	struct avdtp_stream *pending_open;
	uint16_t imtu;
	uint16_t svclass;
	uint16_t l2capPsm;
	guint sdp_timer;
};

struct l2cap_event {
	int16_t psm;
	uint16_t cid;
	uint8_t evt;
	bdaddr_t bdaddr;
} __packed;

static uint16_t sig_cidLocal;
static uint16_t stream_cidLocal;
static GIOChannel *g_a2dpad_l2cap_io = NULL;
static uint32_t g_a2dpad_record_id = 0;
static DBusConnection *g_a2dpad_connection = NULL;
static gboolean connect_first = FALSE;			//TRUE: 一本目の接続OK Signaling
static gboolean connect_second = FALSE;			//TRUE: 二本目の接続OK Streaming
static GIOChannel *g_l2cap_event = NULL;
static gboolean connection_initiator = FALSE;			//FALSE: 対向機からの接続
static gboolean disconnect_initiator_sig = FALSE;		//FALSE: 対向機からの切断（シグナル）
static gboolean disconnect_initiator_stream = FALSE;	//FALSE: 対向機からの切断（ストリーム）

static void a2dpad_set_state(struct a2dpad *control, a2dpad_state_t new_state);
static void a2dpad_change_state(struct a2dpad *a2dpad);
static void a2dpad_disconnect(uint16_t cid,  struct audio_device *dev);
static void l2cap_confirm(GIOChannel *chan, gpointer data);

static char *str_state[] = {
	"A2DPAD_STATE_DISCONNECTED",
	"A2DPAD_STATE_CONNECTING_1",
	"A2DPAD_STATE_CONNECTING_2",
	"A2DPAD_STATE_CONNECTED"
};

static sdp_record_t *a2dp_record( void )
{
	sdp_list_t *svclass_id, *pfseq, *apseq;
	uuid_t l2cap_uuid, avdtp_uuid, a2dp_uuid;
	sdp_profile_desc_t profile;
	sdp_list_t *aproto, *proto[2];
	sdp_record_t *record;
	sdp_data_t *psm, *version, *features;
	uint16_t lp = MNG_AVDTP_PSM;
	uint16_t a2dp_ver = 0x0102;
	uint16_t feat = 0x0002;
	uint16_t avdtp_ver = 0x0102;

	record = sdp_record_alloc();
	if (!record) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] sdp_record_alloc ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SDP_RECORD, 0, 0);
		return NULL;
	}

	//AUDIO_SINK_SVCLASS_ID 0x110b
	sdp_uuid16_create(&a2dp_uuid, AUDIO_SINK_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &a2dp_uuid);
	sdp_set_service_classes(record, svclass_id);

	//ADVANCED_AUDIO_PROFILE_ID 0x110d
	sdp_uuid16_create(&profile.uuid, ADVANCED_AUDIO_PROFILE_ID);
	profile.version = a2dp_ver;
	pfseq = sdp_list_append(0, &profile);
	sdp_set_profile_descs(record, pfseq);

	// L2CAP_UUID	0x0100
	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	//lp = MNG_AVDTP_PSM	0x0019
	psm = sdp_data_alloc(SDP_UINT16, &lp);
	proto[0] = sdp_list_append(proto[0], psm);
	apseq = sdp_list_append(0, proto[0]);

	//AVDTP_UUID	0x0019
	sdp_uuid16_create(&avdtp_uuid, AVDTP_UUID);
	proto[1] = sdp_list_append(0, &avdtp_uuid);
	//AVDTP Ver.	0x0102
	version = sdp_data_alloc(SDP_UINT16, &avdtp_ver);
	proto[1] = sdp_list_append(proto[1], version);
	apseq = sdp_list_append(apseq, proto[1]);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(record, aproto);

	//SDP_ATTR_SUPPORTED_FEATURES	0x0311
	features = sdp_data_alloc(SDP_UINT16, &feat);
	sdp_attr_add(record, SDP_ATTR_SUPPORTED_FEATURES, features);

	sdp_set_info_attr_for_avp(record, "Audio Sink");

	sdp_data_free(psm);
	sdp_data_free(version);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(pfseq, 0);
	sdp_list_free(aproto, 0);
	sdp_list_free(svclass_id, 0);

	return record;
}

static void send_signal_registerCfmL2cap(uint16_t value)
{
	BLUEZ_DEBUG_INFO("[A2DPAD] send_signal_registerCfmL2cap() Start");
	BLUEZ_DEBUG_LOG("[A2DPAD] send_signal_registerCfmL2cap() value = %d", value);

	g_dbus_emit_signal(g_a2dpad_connection, "/",
					AUDIO_A2DPAD_INTERFACE,
					"REGISTER_CFM_L2CAP",
					DBUS_TYPE_UINT16, &value,
					DBUS_TYPE_INVALID);

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_A2DP_REGISTER_CFM, value);

	return;
}


static void send_signal_deregisterCfmL2cap(uint16_t value)
{
	BLUEZ_DEBUG_INFO("[A2DPAD] send_signal_deregisterCfmL2cap() Start");

	g_dbus_emit_signal(g_a2dpad_connection, "/",
					AUDIO_A2DPAD_INTERFACE,
					"DEREGISTER_CFM_L2CAP",
					DBUS_TYPE_UINT16, &value,	
					DBUS_TYPE_INVALID);

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_A2DP_DEREGISTER_CFM, value);

	return;
}


static void send_signal_connectCfmL2cap(uint32_t value, uint16_t cid, bdaddr_t *dst, uint32_t len, uint16_t status, L2CAP_CONFIG_PARAM *l2capConfigParam)
{
	char  BdAddr[BDADDR_SIZE];
	char *pBdAddr;

	BLUEZ_DEBUG_INFO("[A2DPAD] send_signal_connectCfmL2cap() Start");

	memcpy(BdAddr, dst, sizeof(BdAddr));
	pBdAddr = &BdAddr[0];

	BLUEZ_DEBUG_LOG("[A2DPAD] signal BdAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", BdAddr[5], BdAddr[4], BdAddr[3], BdAddr[2], BdAddr[1], BdAddr[0]);
	BLUEZ_DEBUG_LOG("[A2DPAD] signal value = %d.cid = 0x%02x.status = %d.", value, cid, status);

	g_dbus_emit_signal(g_a2dpad_connection, "/",
					AUDIO_A2DPAD_INTERFACE,
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
	btd_manager_get_disconnect_info( dst, MNG_CON_A2DP_CTRL );

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_A2DP_CONNECT_CFM, value);
	return;
}

static void send_signal_connectedL2cap(uint32_t value, uint16_t cid, uint16_t psm, bdaddr_t *dst, uint32_t len, uint8_t identifier, L2CAP_CONFIG_PARAM *l2capConfigParam)
{
	BLUEZ_DEBUG_INFO("[A2DPAD] send_signal_connectedL2cap() Start");
	
	char  BdAddr[BDADDR_SIZE];
	char *pBdAddr;
	memcpy(BdAddr, dst, sizeof(BdAddr));
	pBdAddr = &BdAddr[0];

	BLUEZ_DEBUG_LOG("[A2DPAD] signal BdAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", BdAddr[5], BdAddr[4], BdAddr[3], BdAddr[2], BdAddr[1], BdAddr[0]);
	BLUEZ_DEBUG_LOG("[A2DPAD] signal value = %d.cid = 0x%02x.psm = %d. identifier = %d", value, cid, psm, identifier);

	g_dbus_emit_signal(g_a2dpad_connection, "/",
					AUDIO_A2DPAD_INTERFACE,
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
					DBUS_TYPE_BYTE,   &identifier,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bServiceType,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bMode,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bTxWindowSize,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bMaxTransmit,
					DBUS_TYPE_BYTE,   &l2capConfigParam->bFcsType,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&pBdAddr, BDADDR_SIZE,
					DBUS_TYPE_INVALID);

	/* 切断情報の初期化 */
	btd_manager_get_disconnect_info( dst, MNG_CON_A2DP_CTRL );

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_A2DP_CONNECTED, value);

	return;
}

static void send_signal_dataRcvL2cap(uint32_t value, uint16_t cid, char *data, int datalen)
{
	BLUEZ_DEBUG_INFO("[A2DPAD] send_signal_dataRcvL2cap() Start");
	BLUEZ_DEBUG_LOG("[A2DPAD] send_signal_dataRcvL2cap() cid = 0x%02x. datalen = %d.", cid, datalen);

	g_dbus_emit_signal(g_a2dpad_connection, "/",
					AUDIO_A2DPAD_INTERFACE,
					"DATA_RCV_L2CAP",
					DBUS_TYPE_UINT32, &value,
					DBUS_TYPE_UINT16, &cid,
					DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
					&data, datalen,
					DBUS_TYPE_INVALID);

	return;
}

static gboolean a2dpad_cb(GIOChannel *chan, GIOCondition cond,
				gpointer data)
{
	struct a2dpad *a2dpad = data;
	unsigned char buf[a2dpad->imtu], *operands;
	unsigned char *pbuf = buf;
	int sock;
	int buf_len;
	GError *gerr = NULL;
	uint16_t cid;
	uint32_t value = TRUE;

	BLUEZ_DEBUG_INFO("[A2DPAD] a2dpad_cb() Start.");

	if( a2dpad->io == NULL ) {
		BLUEZ_DEBUG_WARN("[A2DPAD] A2DP Signaling is already disconnected");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_DISCONNECTED, 0, 0);
		return FALSE;
	}

	if (cond & (G_IO_NVAL | G_IO_ERR | G_IO_HUP ))
	{
		BLUEZ_DEBUG_WARN("[A2DPAD] NVAL or ERR or HUP on L2CAP socket");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_IO, cond, 0);
		goto failed;
	}

	sock = g_io_channel_unix_get_fd(a2dpad->io);
	BLUEZ_DEBUG_LOG("[A2DPAD] a2dpad_cb() sock = %d.", sock);
	
	buf_len = read(sock, buf, sizeof(buf));
	if (buf_len <= 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] read buf is 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_READ, 0, 0);
		goto failed;
	}

	BLUEZ_DEBUG_LOG("[A2DPAD] Got %d bytes of data for A2DPAD session %p", buf_len, a2dpad);

	bt_io_get(chan, BT_IO_L2CAP, &gerr,
			BT_IO_OPT_CID, &cid,
			BT_IO_OPT_INVALID);
	if (gerr) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] bt_io_get() ERROR = [ %s ]", gerr->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_IO_GET, gerr->code, 0);
		g_error_free(gerr);
		goto failed;
	}
	
	BLUEZ_DEBUG_LOG("[A2DPAD] a2dpad_cb() cid = 0x%02x", cid);
	send_signal_dataRcvL2cap(value, cid, (char *)pbuf, buf_len);
	
	return TRUE;

failed:
	BLUEZ_DEBUG_LOG("[A2DPAD] A2DPAD disconnected cid = 0x%02x", sig_cidLocal);
	a2dpad_disconnect( sig_cidLocal, a2dpad->dev );
	a2dpad_change_state(a2dpad);
	return FALSE;
}

static gboolean a2dpadStream_cb(GIOChannel *chan, GIOCondition cond,
				gpointer data)
{
	struct a2dpad *a2dpad = data;
	struct avdtp_stream *avdtpStream = a2dpad->pending_open;
	unsigned char buf[avdtpStream->imtu];
	unsigned char *pbuf = buf;
	int sock;
	int buf_len;
	uint32_t value = TRUE;
	GError *gerr = NULL;
	uint16_t cid;

	BLUEZ_DEBUG_INFO("[A2DPAD] a2dpadStream_cb() Start.");

	if( avdtpStream->io == NULL ) {
		BLUEZ_DEBUG_WARN("[A2DPAD] A2DP Streaming is already disconnected");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_DISCONNECTED, 0, 0);
		return FALSE;
	}

	if (cond & (G_IO_NVAL | G_IO_ERR | G_IO_HUP))
	{
		BLUEZ_DEBUG_WARN("[A2DPAD] NVAL or ERR or HUP on L2CAP socket");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_IO, cond, 0);
		goto failed;
	}

	sock = g_io_channel_unix_get_fd(avdtpStream->io);
	BLUEZ_DEBUG_LOG("[A2DPAD] a2dpadStream_cb() sock = %d.", sock);
	
	buf_len = read(sock, buf, sizeof(buf));
	if (buf_len <= 0)
		goto failed;

	BLUEZ_DEBUG_LOG("[A2DPAD] Got %d bytes of data for AVDTP session %p", buf_len, avdtpStream);

	bt_io_get(chan, BT_IO_L2CAP, &gerr,
			BT_IO_OPT_CID, &cid,
			BT_IO_OPT_INVALID);
	if (gerr) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] bt_io_get() ERROR = [ %s ]", gerr->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_IO_GET, gerr->code, 0);
		g_error_free(gerr);
		goto failed;
	}
	
	BLUEZ_DEBUG_LOG("[A2DPAD] a2dpadStream_cb() cid = 0x%02x", cid);
	send_signal_dataRcvL2cap(value, cid, (char *)pbuf, buf_len);
	
	return TRUE;

failed:
	BLUEZ_DEBUG_LOG("[A2DPAD] A2DPAD disconnected cid = 0x%02x", stream_cidLocal);
	a2dpad_disconnect( stream_cidLocal, a2dpad->dev );
	a2dpad_change_state(a2dpad);
	return FALSE;
}

static void close_signaling(struct a2dpad *a2dpad)
{
	int sock;

	if (a2dpad->io == NULL) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] A2DP Signaling is already disconnected");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CLOSE_SIGNALING, 0, 0);
		return;
	}

	sock = g_io_channel_unix_get_fd(a2dpad->io);

	shutdown(sock, SHUT_RDWR);

	g_io_channel_shutdown(a2dpad->io, FALSE, NULL);

	g_io_channel_unref(a2dpad->io);
	a2dpad->io = NULL;
	return;
}

static void close_stream(struct avdtp_stream *stream)
{
	int sock;

	if (stream->io == NULL) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] A2DP Streaming is already disconnected");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CLOSE_STREAM, 0, 0);
		return;
	}

	sock = g_io_channel_unix_get_fd(stream->io);

	shutdown(sock, SHUT_RDWR);

	g_io_channel_shutdown(stream->io, FALSE, NULL);

	g_io_channel_unref(stream->io);
	stream->io = NULL;
	return;
}

static gboolean signaling_cb(GIOChannel *chan, GIOCondition cond, gpointer data)
{
	struct a2dpad *a2dpad = data;
	BLUEZ_DEBUG_INFO("[A2DPAD] close signaling Start");

	if (!(cond & G_IO_NVAL)) {
		close_signaling(a2dpad);
	}
	a2dpad->io_id = 0;

	return FALSE;
}

static gboolean transport_cb(GIOChannel *chan, GIOCondition cond, gpointer data)
{
	struct avdtp_stream *stream = data;
	BLUEZ_DEBUG_INFO("[A2DPAD] close stream Start");

	if (!(cond & G_IO_NVAL)) {
		close_stream(stream);
	}
	stream->io_id = 0;

	return FALSE;
}

static int set_send_buffer_size(int sk, int size)
{
	socklen_t optlen = sizeof(size);

	if (setsockopt(sk, SOL_SOCKET, SO_SNDBUF, &size, optlen) < 0) {
		int err = -errno;
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] setsockopt(SO_SNDBUF) failed: %s (%d)", strerror(-err), -err);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SET_BUFSIZE, err, 0);
		return err;
	}
	return 0;
}

static int get_send_buffer_size(int sk)
{
	int size;
	socklen_t optlen = sizeof(size);

	if (getsockopt(sk, SOL_SOCKET, SO_SNDBUF, &size, &optlen) < 0) {
		int err = -errno;
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] getsockopt(SO_SNDBUF) failed: %s (%d)", strerror(-err), -err);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_BUFSIZE, err, 0);
		return err;
	}

	/*
	 * Doubled value is returned by getsockopt since kernel uses that
	 * space for its own purposes (see man 7 socket, bookkeeping overhead
	 * for SO_SNDBUF).
	 */
	return size / 2;
}

static void handle_transport_connect(struct a2dpad *session, GIOChannel *io,
					uint16_t imtu, uint16_t omtu)
{
	struct avdtp_stream *stream = session->pending_open;
	int sk, buf_size, min_buf_size;
	GError *err = NULL;

	BLUEZ_DEBUG_INFO("[A2DPAD] handle_transport_connect. Start.");

	if (io == NULL) {
		return;
	}

	if (stream->io == NULL) {
		stream->io = g_io_channel_ref(io);
	}

	stream->omtu = omtu;
	stream->imtu = imtu;

	bt_io_set(stream->io, BT_IO_L2CAP, &err,
					BT_IO_OPT_FLUSHABLE, TRUE,
					BT_IO_OPT_INVALID);
	if (err != NULL) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] Enabling flushable packets failed: %s", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_TRANSPORT_CONNECT, err->code, 0);
		g_error_free(err);
	} else
		BLUEZ_DEBUG_LOG("[A2DPAD] Flushable packets enabled");

	sk = g_io_channel_unix_get_fd(stream->io);
	buf_size = get_send_buffer_size(sk);
	if (buf_size < 0) {
		goto proceed;
	}

	BLUEZ_DEBUG_LOG("[A2DPAD] sk %d, omtu %d, send buffer size %d", sk, omtu, buf_size);
	min_buf_size = omtu * 2;
	if (buf_size < min_buf_size) {
		BLUEZ_DEBUG_LOG("send buffer size to be increassed to %d", min_buf_size);
		set_send_buffer_size(sk, min_buf_size);
	}

proceed:
	BLUEZ_DEBUG_INFO("[A2DPAD] handle_transport_connect. END.");
}


static void a2dpad_connect_cb(GIOChannel *chan, GError *err, gpointer data)
{
	struct a2dpad *a2dpad = data;
	char address[18];
	uint16_t imtu;
	uint16_t omtu;
	GError *gerr = NULL;
	uint16_t cid = 0;
	bdaddr_t dst;
	uint16_t status = 0;
	L2CAP_CONFIG_PARAM l2capConfigParam;
	struct l2cap_options lo;
	int sock;
	socklen_t size;
	uint32_t value = FALSE;
	char identifier = 0;
	int dstLen = sizeof(bdaddr_t);
	int linkinfo = 0;
	struct avdtp_stream *stream ;

	BLUEZ_DEBUG_INFO("[Bluez - A2DPAD] a2dpad_connect_cb() Start.");
	EVENT_DRC_BLUEZ_A2DP_CONNECT_CB(FORMAT_ID_A2DP_CONNECT_CB, connection_initiator, connect_first, connect_second);

	if (err) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] ERROR : [ %s ]", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_CB_ERR, err->code, 0);
		goto failed;
	}

	bt_io_get(chan, BT_IO_L2CAP, &gerr,
			BT_IO_OPT_DEST, &address,
			BT_IO_OPT_OMTU, &omtu,
			BT_IO_OPT_IMTU, &imtu,
			BT_IO_OPT_CID, &cid,
			BT_IO_OPT_DEST_BDADDR, &dst,
			BT_IO_OPT_INVALID);
	if (gerr) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] bt_io_get() gerr = [ %s ]", gerr->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_CB_IO_GET, gerr->code, 0);
		g_error_free(gerr);
		goto failed;
	}

	BLUEZ_DEBUG_LOG("[A2DPAD] AVDTP: connected to %s", address);

	if( connect_first == FALSE ){

		BLUEZ_DEBUG_LOG("[A2DPAD] signaling conect start. cid = 0x%02x", cid);

		a2dpad->imtu = imtu;
		if (!a2dpad->io){
			a2dpad->io = g_io_channel_ref(chan);
		}

		BLUEZ_DEBUG_LOG("[A2DPAD] a2dpad_connect_cb() bt_io_get: a2dpad->mtu = %d.", a2dpad->imtu);

		if (a2dpad->io_id){
			g_source_remove(a2dpad->io_id);
		}

		a2dpad->io_id = g_io_add_watch_full(chan,
							G_PRIORITY_LOW,
							G_IO_IN | G_IO_ERR | G_IO_HUP
							| G_IO_NVAL,
							(GIOFunc) a2dpad_cb, a2dpad,
							NULL);

		sig_cidLocal = cid;

		sock = g_io_channel_unix_get_fd(a2dpad->io);
		BLUEZ_DEBUG_LOG("[A2DPAD] a2dpad_connect_cb() sock = %d.", sock);
		memset(&lo, 0, sizeof(lo));
		size = sizeof(lo);
		if (getsockopt(sock, SOL_L2CAP, L2CAP_OPTIONS, &lo, &size) < 0) {
			BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] getsockopt() : [ %s ]", strerror(errno));
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
		BLUEZ_DEBUG_LOG("[A2DPAD] a2dpad_connect_cb() getsockopt: wMtu = %d. wFlushTo = %d. bMode = %d.", l2capConfigParam.wMtu, l2capConfigParam.wFlushTo, l2capConfigParam.bMode);
		BLUEZ_DEBUG_LOG("[A2DPAD] a2dpad_connect_cb() getsockopt: bFcsType = %d. bMaxTransmit = %d. bTxWindowSize = %d.", l2capConfigParam.bFcsType, l2capConfigParam.bMaxTransmit, l2capConfigParam.bTxWindowSize);
	
		value = TRUE;

		if(connection_initiator){
			send_signal_connectCfmL2cap(value, cid, &dst, dstLen, status, &l2capConfigParam);
		}else{
			send_signal_connectedL2cap(value, cid, MNG_AVDTP_PSM, &dst, dstLen, identifier, &l2capConfigParam);
		}
		btd_manager_connect_profile( &dst, MNG_CON_A2DP_CTRL, 0, cid );

		connect_first = TRUE;
		if( connect_second == TRUE ) {
			a2dpad_set_state(a2dpad, A2DPAD_STATE_CONNECTED);
		}
	} else if ( connect_second == FALSE ) {

		BLUEZ_DEBUG_LOG("[A2DPAD] streaming connect start. cid = 0x%02x", cid);
		stream = a2dpad->pending_open;
		handle_transport_connect(a2dpad, chan, imtu, omtu);

		stream->imtu = imtu;
		if (!stream->io){
			stream->io = g_io_channel_ref(chan);
		}
		if (stream->io_id){
			g_source_remove(stream->io_id);
		}

		stream->io_id = g_io_add_watch_full(chan,
											G_PRIORITY_LOW,
											G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
											(GIOFunc) a2dpadStream_cb, a2dpad,
											NULL);

		stream_cidLocal = cid;

		sock = g_io_channel_unix_get_fd(stream->io);
		BLUEZ_DEBUG_LOG("[A2DPAD] a2dpad_connect_cb() sock = %d.", sock);
		memset(&lo, 0, sizeof(lo));
		size = sizeof(lo);
		if (getsockopt(sock, SOL_L2CAP, L2CAP_OPTIONS, &lo, &size) < 0) {
			BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] getsockopt() : [ %s ]", strerror(errno));
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
		BLUEZ_DEBUG_LOG("[A2DPAD] a2dpad_connect_cb() getsockopt: wMtu = %d. wFlushTo = %d. bMode = %d.", l2capConfigParam.wMtu, l2capConfigParam.wFlushTo, l2capConfigParam.bMode);
		BLUEZ_DEBUG_LOG("[A2DPAD] a2dpad_connect_cb() getsockopt: bFcsType = %d. bMaxTransmit = %d. bTxWindowSize = %d.", l2capConfigParam.bFcsType, l2capConfigParam.bMaxTransmit, l2capConfigParam.bTxWindowSize);
	
		value = TRUE;
		if(connection_initiator){
			send_signal_connectCfmL2cap(value, cid, &dst, dstLen, status, &l2capConfigParam);
		}else{
			send_signal_connectedL2cap(value, cid, MNG_AVDTP_PSM, &dst, dstLen, identifier, &l2capConfigParam); 
		}
		btd_manager_connect_profile( &dst, MNG_CON_A2DP_STRM, 0, cid );

		connect_second = TRUE;
		if( connect_first == TRUE ) {
			a2dpad_set_state(a2dpad, A2DPAD_STATE_CONNECTED);
		}
	} else {
		/* 接続済み */
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] a2dp is already connected  cid = 0x%02x", cid);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_CB_CONNECTED, cid, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_CB_CONNECTED, __LINE__, &dst);
	}
	return;
failed:
	memset( &l2capConfigParam, 0, sizeof(l2capConfigParam) );
	if(connection_initiator){
		linkinfo = btd_manager_find_connect_info_by_bdaddr( &a2dpad->dst );
		if( linkinfo == MNG_ACL_MAX ) {
			if( btd_manager_get_disconnect_info( &a2dpad->dst, MNG_CON_A2DP_CTRL ) == TRUE ) {
				value = FALSE;
			} else {
				value = 0xFF;
			}
		}
		send_signal_connectCfmL2cap(value, cid, &a2dpad->dst, dstLen, status, &l2capConfigParam);
	}
	a2dpad_disconnect( cid, a2dpad->dev );
	a2dpad_change_state(a2dpad);
	return;
}

static void a2dpad_confirm_cb(GIOChannel *chan, gpointer data)
{
	struct a2dpad *a2dpad = NULL;
	struct audio_device *dev;
	struct avdtp_stream *stream;
	char address[18];
	bdaddr_t src, dst;
	GError *err = NULL;
	gboolean ret;

	BLUEZ_DEBUG_INFO("[A2DPAD] a2dpad_confirm_cb() Start.");
	EVENT_DRC_BLUEZ_CB(FORMAT_ID_BT_CB, CB_A2DP_REGISTER);

	bt_io_get(chan, BT_IO_L2CAP, &err,
			BT_IO_OPT_SOURCE_BDADDR, &src,
			BT_IO_OPT_DEST_BDADDR, &dst,
			BT_IO_OPT_DEST, address,
			BT_IO_OPT_INVALID);
	if (err) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] ERROR : [ %s ]", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_IO_GET, 0, 0);
		g_error_free(err);
		g_io_channel_shutdown(chan, TRUE, NULL);
		return;
	}

	BLUEZ_DEBUG_WARN("[A2DPAD] Confirm from [ %02x:%02x:%02x:%02x:%02x:%02x ]", dst.b[5], dst.b[4], dst.b[3], dst.b[2], dst.b[1], dst.b[0]);
	EVENT_DRC_BLUEZ_CONFIRM(FORMAT_ID_CONFIRM, CONFIRM_A2DP, &dst);

	dev = manager_get_device(&src, &dst, TRUE);
	if (!dev) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] Unable to get audio device object for %s", address);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_GET_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_GET_DEV, __LINE__, &dst);
		goto drop;
	}

	if (!manager_allow_a2dpad_connection(dev, &dst)) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] Refusing a2dpad: too many existing connections");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_REFUSING, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_REFUSING, __LINE__, &dst);
		goto drop;
	}

	if ( a2dpad_get_state(dev) == A2DPAD_STATE_DISCONNECTED ) {
		if( btd_manager_isconnect( MNG_CON_A2DP_CTRL ) == TRUE || btd_manager_isconnect( MNG_CON_A2DP_STRM ) == TRUE ) {
			/* 切断中 */
			BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] a2dp is still under connection");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_UNDER_CONNECTION, 0, 0);
			EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_UNDER_CONNECTION, __LINE__, &dst);
			goto drop;
		}
	}

	if ( a2dpad_get_state(dev) == A2DPAD_STATE_CONNECTING_1 ) {
		if( btd_manager_isconnect( MNG_CON_A2DP_CTRL ) == FALSE ) {
			/* SIGNALING 接続中 */
			BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] SIGNALING is connecting");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_UNDER_CONNECTION, 0, 0);
			EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_UNDER_CONNECTION, __LINE__, &dst);
			goto drop;
		}
	}

	if ( a2dpad_get_state(dev) == A2DPAD_STATE_CONNECTING_2 ) {
		if( btd_manager_isconnect( MNG_CON_A2DP_CTRL ) == TRUE && btd_manager_isconnect( MNG_CON_A2DP_STRM ) == FALSE ) {
			/* STREAMING 接続中 */
			BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] STREAMING is connecting");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_UNDER_CONNECTION, 0, 0);
			EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_UNDER_CONNECTION, __LINE__, &dst);
			goto drop;
		}
	}

	if (!dev->a2dpad) {
		btd_device_add_uuid(dev->btd_dev, A2DP_SOURCE_UUID);
		BLUEZ_DEBUG_LOG("[A2DPAD] a2dpad_confirm_cb btd_device_add_uuid");
		if (!dev->a2dpad) {
			BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] dev->a2dpad is nothing --> goto drop");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_ADD_UUID, 0, 0);
			EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_ADD_UUID, __LINE__, &dst);
			goto drop;
		}
	}

	if (a2dpad_get_state(dev) == A2DPAD_STATE_CONNECTED) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] a2dp is already connected - 1");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_EXISTS, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_EXISTS, __LINE__, &dst);
		goto drop;
	}

	if( connect_first == TRUE && connect_second == TRUE ) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] a2dp is already connected - 2");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_EXISTS, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_EXISTS, __LINE__, &dst);
		goto drop;
	}

	a2dpad = dev->a2dpad;
	memcpy( &a2dpad->dst, &dst, sizeof( bdaddr_t ) );


	if( connect_first == FALSE ) {
		/* signaling */
		ret = bt_io_accept(chan, a2dpad_connect_cb, a2dpad, NULL, &err);
		if( ret != TRUE  ) {
			BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] bt_io_accept() - 1 ERROR = [ %s ]", err->message);
			ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_IO_ACCEPT, err->code, 0);
			EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_IO_ACCEPT, __LINE__, &dst);
			g_error_free(err);
			goto drop;
		}
		if( connect_second == FALSE ) {
			/* 初回 */
			a2dpad_set_state(a2dpad, A2DPAD_STATE_CONNECTING_1);
			/* 切断方向初期化 */
			disconnect_initiator_sig = FALSE;
			disconnect_initiator_stream = FALSE;
		} else {
			a2dpad_set_state(a2dpad, A2DPAD_STATE_CONNECTING_2);
		}
		if (!a2dpad->io){
			a2dpad->io = g_io_channel_ref(chan);
		}
		if ( !a2dpad->io_id ) {
			a2dpad->io_id = g_io_add_watch(chan, G_IO_ERR | G_IO_HUP | G_IO_NVAL, signaling_cb, a2dpad);
		}
	} else {
		/* streaming */
		stream = a2dpad->pending_open;
		ret = bt_io_accept(chan, a2dpad_connect_cb, a2dpad, NULL, &err);
		if( ret != TRUE  ) {
			BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] bt_io_accept() - 2 ERROR = [ %s ]", err->message);
			ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_IO_ACCEPT, err->code, 0);
			EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_IO_ACCEPT, __LINE__, &dst);
			g_error_free(err);
			goto drop;
		}
		a2dpad_set_state(a2dpad, A2DPAD_STATE_CONNECTING_2);

		if (!stream->io){
			stream->io = g_io_channel_ref(chan);
		}
		if (!stream->io_id){
			stream->io_id = g_io_add_watch(chan, G_IO_ERR | G_IO_HUP | G_IO_NVAL, transport_cb, a2dpad);
		}
	}
	connection_initiator = FALSE;

	return;

drop:
	g_io_channel_shutdown(chan, TRUE, NULL);
	return;
}

static DBusMessage *register_l2cap_a2dp(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	const char *srcAddr;
	uint32_t addrlen;
	bdaddr_t src;
	uint16_t l2capPsm;
	gboolean master = TRUE;
	GIOChannel *io;
	GError *err = NULL;
	uint16_t value = 0x00;
	sdp_record_t *record = NULL;
	uint16_t l2capPsm2;

	BLUEZ_DEBUG_INFO("[A2DPAD] register_l2cap_a2dp() Start");

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_A2DP_REGISTER, NULL);

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_UINT16, &l2capPsm,
						DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
						&srcAddr, &addrlen,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[A2DPAD] register_l2cap_a2dp() l2capPsm = %d", l2capPsm);

	bluezutil_get_bdaddr( &src );

	if (!g_a2dpad_record_id) {
		record = a2dp_record();
		if (!record) {
			BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] Unable to allocate new service record");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_SDP_RECORD, 0, 0);
			return NULL;
		}

		if (add_record_to_server(&src, record) < 0) {
			BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] Unable to register A2DP service record");
			sdp_record_free(record);
			ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_ADD_RECORD, 0, 0);
			return NULL;
		}
		g_a2dpad_record_id = record->handle;
	}

	if (!g_a2dpad_l2cap_io) {
		io = bt_io_listen(BT_IO_L2CAP, NULL, a2dpad_confirm_cb, NULL,
					NULL, &err,
					BT_IO_OPT_SOURCE_BDADDR, &src,
					BT_IO_OPT_PSM, l2capPsm,
					BT_IO_OPT_SEC_LEVEL, BT_IO_SEC_MEDIUM,
					BT_IO_OPT_MASTER, master,
					BT_IO_OPT_INVALID);
		
		if (!io) {
			BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] bt_io_listen() err = [%s]", err->message);
			ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_IO_LISTEN, 0, 0);
			g_error_free(err);
			value = 0x01;
		}
		l2capPsm2 = 30;
		if( g_l2cap_event == NULL ) {
			g_l2cap_event = bt_io_listen_l2cap2(l2cap_confirm, &err,
												BT_IO_OPT_SOURCE_BDADDR, &src,
												BT_IO_OPT_PSM, l2capPsm2,
												BT_IO_OPT_SEC_LEVEL, BT_IO_SEC_MEDIUM,
												BT_IO_OPT_MASTER, master,
												BT_IO_OPT_INVALID);
		}

		g_a2dpad_l2cap_io = io;
	}
	
	BLUEZ_DEBUG_INFO("[A2DPAD] register_l2cap_a2dp() End value = %d", value);

	connect_first = FALSE;
	connect_second = FALSE;
	connection_initiator = FALSE;
	disconnect_initiator_sig = FALSE;
	disconnect_initiator_stream = FALSE;

	send_signal_registerCfmL2cap(value);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *deregister_l2cap_a2dp(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	int sock;
	uint16_t value = TRUE;
	uint16_t l2capPsm;
	const char *srcAddr;
	uint32_t addrlen;

	BLUEZ_DEBUG_INFO("[A2DPAD] deregister_l2cap_a2dp() Start");

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_A2DP_DEREGISTER, NULL);

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_UINT16, &l2capPsm,
						DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
						&srcAddr, &addrlen,
						DBUS_TYPE_INVALID)){
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DEREGISTER, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[A2DPAD] deregister_l2cap_a2dp() l2capPsm = 0x%02x",l2capPsm);
	BLUEZ_DEBUG_LOG("[A2DPAD] deregister_l2cap_a2dp() srcAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", srcAddr[5], srcAddr[4], srcAddr[3], srcAddr[2], srcAddr[1], srcAddr[0]);

	send_signal_deregisterCfmL2cap(value);

	BLUEZ_DEBUG_INFO("[A2DPAD] deregister_l2cap_a2dp() End");
	return dbus_message_new_method_return(msg);
}

static void get_record_cb(sdp_list_t *recs, int err, gpointer user_data)
{
	struct audio_device *dev = user_data;
	struct a2dpad *a2dpad = dev->a2dpad;
	int dstLen = sizeof(bdaddr_t);
	int linkinfo = 0;
	L2CAP_CONFIG_PARAM l2capConfigParam;
	uint32_t value = FALSE;
	uint16_t cid = 0;
	uint16_t status = 0;
	GIOChannel *io;
	GError *gerr = NULL;

	BLUEZ_DEBUG_INFO("[A2DPAD] get_record_cb(). Start.");

	if( a2dpad == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] a2dpad == NULL.");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
		return;
	}

	if (a2dpad->sdp_timer) {
		g_source_remove(a2dpad->sdp_timer);
		a2dpad->sdp_timer = 0;
	}

	if (err < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] Unable to get service record: [ %s ] (%d)",strerror(-err), -err);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, err, 0);
		goto failed;
	}

	if (!recs || !recs->data) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] No records found");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
		goto failed;
	}

	a2dpad->svclass = 0;

	io = bt_io_connect(BT_IO_L2CAP, a2dpad_connect_cb, a2dpad, NULL, &gerr,
				BT_IO_OPT_SOURCE_BDADDR, &dev->src,
				BT_IO_OPT_DEST_BDADDR, &a2dpad->dst,
				BT_IO_OPT_PSM, a2dpad->l2capPsm,
				BT_IO_OPT_INVALID);
	if (gerr) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] bt_io_connect() ERROR =  [ %s ]", gerr->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
		g_error_free(gerr);
		goto failed;
	}

	a2dpad->io = io;

	BLUEZ_DEBUG_INFO("[A2DPAD] get_record_cb() End");

	return;

failed:
	memset( &l2capConfigParam, 0, sizeof(l2capConfigParam) );
	linkinfo = btd_manager_find_connect_info_by_bdaddr( &a2dpad->dst );
	if( linkinfo == MNG_ACL_MAX ) {
		if( ECONNABORTED == -err && 0 == btd_manager_get_acl_guard() ) {
			btd_manager_fake_signal_conn_complete( &a2dpad->dst );
			value = 0xFF;
		} else {
			if( btd_manager_get_disconnect_info( &a2dpad->dst, MNG_CON_A2DP_CTRL ) == TRUE ) {
				value = FALSE;
			} else {
				value = 0xFF;
			}
		}
	} else {
		value = FALSE;
	}
	
	a2dpad->svclass = 0;
	
	send_signal_connectCfmL2cap(value, cid, &a2dpad->dst, dstLen, status, &l2capConfigParam);

	a2dpad_set_state(a2dpad, A2DPAD_STATE_DISCONNECTED);

	return;
}

static gboolean sdp_timer_cb(gpointer data)
{
	struct audio_device *device = data;
	struct a2dpad *a2dpad = device->a2dpad;
	int value = TRUE;
	uint16_t cid = 0;
	int dstLen = sizeof(bdaddr_t);
	GIOChannel *io;
	GError *gerr = NULL;
	L2CAP_CONFIG_PARAM l2capConfigParam;
	int linkinfo = 0;
	uint16_t status = 0;

	BLUEZ_DEBUG_INFO("[A2DPAD] sdp_timer_cb(). Start.");

	g_source_remove(a2dpad->sdp_timer);
	a2dpad->sdp_timer = 0;

	if( a2dpad_get_state(device) != A2DPAD_STATE_CONNECTING_1 ) {
		BLUEZ_DEBUG_LOG("[A2DPAD] alredy connected or disconnected");
		return TRUE;
	}

	BLUEZ_DEBUG_WARN("[A2DPAD] SDP Time Out Connected by Default Infomation");
	ERR_DRC_BLUEZ(ERROR_BLUEZ_SDP_TIMEOUT, 0, 0);
	bt_cancel_discovery2(&device->src, &device->dst, a2dpad->svclass);
	a2dpad->svclass = 0;

	io = bt_io_connect(BT_IO_L2CAP, a2dpad_connect_cb, a2dpad, NULL, &gerr,
				BT_IO_OPT_SOURCE_BDADDR, &device->src,
				BT_IO_OPT_DEST_BDADDR, &a2dpad->dst,
				BT_IO_OPT_PSM, a2dpad->l2capPsm,
				BT_IO_OPT_INVALID);
	if (gerr) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] bt_io_connect() ERROR =  [ %s ]", gerr->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_IO_CONNECT, 0, 0);
		g_error_free(gerr);
		goto failed;
	}
	a2dpad->io = io;

	BLUEZ_DEBUG_INFO("[A2DPAD] sdp_timer_cb() End");

	return TRUE;

failed:
	memset( &l2capConfigParam, 0, sizeof(l2capConfigParam) );
	linkinfo = btd_manager_find_connect_info_by_bdaddr( &a2dpad->dst );
	if( linkinfo == MNG_ACL_MAX ) {
		if( btd_manager_get_disconnect_info( &a2dpad->dst, MNG_CON_A2DP_CTRL ) == TRUE ) {
			value = FALSE;
		} else {
			value = 0xFF;
		}
	} else {
		value = FALSE;
	}
	send_signal_connectCfmL2cap(value, cid, &a2dpad->dst, dstLen, status, &l2capConfigParam);

	return TRUE;
}

static int get_records(struct audio_device *device)
{
	struct a2dpad *a2dpad = device->a2dpad;
	uint16_t svclass;
	uuid_t uuid;
	int err;

	BLUEZ_DEBUG_INFO("[A2DPAD] get_records() Start.");

	svclass = AUDIO_SOURCE_SVCLASS_ID;

	sdp_uuid16_create(&uuid, svclass);

	BLUEZ_DEBUG_LOG("[A2DPAD] svclass = %x", svclass);

	err = bt_search_service2(&device->src, &device->dst, &uuid,
						get_record_cb, device, NULL);
	if (err < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] bt_search_service2() return err");
		return err;
	}

	a2dpad->svclass = svclass;
	a2dpad_set_state(a2dpad, A2DPAD_STATE_CONNECTING_1);

	a2dpad->sdp_timer = g_timeout_add_seconds(A2DP_SDP_TIMER, sdp_timer_cb, device);

	return 0;
}

static DBusMessage *connect_l2cap_a2dp(DBusConnection *conn, DBusMessage *msg, void *data)
{
	const char *dstAddr;
	uint16_t l2capPsm;
	uint32_t dstLen;
	bdaddr_t dst;
	bdaddr_t src;
	uint16_t cid  = 0;
	uint16_t status = 0;
	L2CAP_CONFIG_PARAM l2capConfigParam;
	uint32_t value = FALSE;
	int linkinfo = 0;
	GError *err = NULL;
	GIOChannel *io;
	a2dpad_state_t state;
	struct a2dpad *a2dpad;
	int ret;

	BLUEZ_DEBUG_INFO("[A2DPAD] connect_l2cap_a2dp() Start");

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_UINT16, &l2capPsm,
						DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
						&dstAddr, &dstLen,
						DBUS_TYPE_INVALID)){
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_WARN("[A2DPAD] connect_l2cap_a2dp() l2capPsm = %d dstAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", l2capPsm, dstAddr[5], dstAddr[4], dstAddr[3], dstAddr[2], dstAddr[1], dstAddr[0]);
	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_A2DP_CONNECT, dstAddr);
	
	bluezutil_get_bdaddr( &src );

	memcpy( &dst, dstAddr, sizeof(bdaddr_t) );
	struct audio_device *device = manager_find_device(NULL, NULL, &dst, AUDIO_A2DPAD_INTERFACE, FALSE);
	BLUEZ_DEBUG_LOG("[A2DPAD] connect_l2cap_a2dp() device = %p.", device);
	if ( device == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] device is NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_FIND_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_FIND_DEV, __LINE__, &dst);
		goto failed;
	}

	a2dpad = device->a2dpad;

	if (!manager_allow_a2dpad_connection(device, &dst)) {
		/* 接続が拒否された場合 */
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] Refusing a2dp: too many existing connections");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_REFUSING, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_REFUSING, __LINE__, &dst);
		goto failed;
	}

	if( A2DPAD_STATE_CONNECTING_1 == a2dpad_get_state( device ) ) {
		if( connect_first == FALSE && connect_second == FALSE){
			/* 相手からの接続中（1本目接続中） */
			BLUEZ_DEBUG_WARN("[A2DPAD] A2DPAD( 1st ) is connecting");
			connection_initiator = TRUE;
			/* 切断方向初期化 */
			disconnect_initiator_sig = FALSE;
			disconnect_initiator_stream = FALSE;
			EVENT_DRC_BLUEZ_CHGMSG(FORMAT_ID_CHGMSG, CHANGE_MESSAGE_A2DP_1ST, &dst);
			return dbus_message_new_method_return(msg);
		}
	}

	if( A2DPAD_STATE_CONNECTING_2 == a2dpad_get_state( device ) ) {
		/* 相手からの接続中（2本目接続中） */
		BLUEZ_DEBUG_WARN("[A2DPAD] A2DPAD( 2nd ) is connecting");
		connection_initiator = TRUE;
		EVENT_DRC_BLUEZ_CHGMSG(FORMAT_ID_CHGMSG, CHANGE_MESSAGE_A2DP_2ND, &dst);
		return dbus_message_new_method_return(msg);
	}

	if ( a2dpad_get_state(device) == A2DPAD_STATE_DISCONNECTED ) {
		if( btd_manager_isconnect( MNG_CON_A2DP_CTRL ) == TRUE || btd_manager_isconnect( MNG_CON_A2DP_STRM ) == TRUE ) {
			/* 切断中 */
			BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] a2dp is still under connection - 1");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_UNDER_CONNECTION, 0, 0);
			EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_UNDER_CONNECTION, __LINE__, &dst);
			goto failed;
		}
	}

	if ( a2dpad_get_state(device) == A2DPAD_STATE_CONNECTED ) {
		/* 接続済み */
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] a2dp is still under connection - 2");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_UNDER_CONNECTION, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_UNDER_CONNECTION, __LINE__, &dst);
		goto failed;
	}

	memcpy( &a2dpad->dst, &dst, sizeof( bdaddr_t ) );
	if ( a2dpad_get_state(device) == A2DPAD_STATE_DISCONNECTED ) {
		/* 一本目接続 */
		ret = get_records(device);
		if( ret != 0 ) {
			goto failed;
		}
		a2dpad->l2capPsm = l2capPsm;
		connection_initiator = TRUE;
		/* 切断方向初期化 */
		disconnect_initiator_sig = FALSE;
		disconnect_initiator_stream = FALSE;
		return dbus_message_new_method_return(msg);
	}

	io = bt_io_connect(BT_IO_L2CAP, a2dpad_connect_cb, a2dpad, NULL, &err,
				BT_IO_OPT_SOURCE_BDADDR, &src,
				BT_IO_OPT_DEST_BDADDR, &dst,
				BT_IO_OPT_PSM, l2capPsm,
				BT_IO_OPT_INVALID);
	if (err) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] bt_io_connect ERROR = [ %s ]", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_IO_CONNECT, 0, 0);
		g_error_free(err);
		goto failed;
	}
	connection_initiator = TRUE;

	if( connect_first == FALSE ){
		a2dpad->io = io;
	} else {
		a2dpad->pending_open->io = io;
	}
	state = a2dpad_get_state( device );
	if( state == A2DPAD_STATE_DISCONNECTED ) {
		a2dpad_set_state(a2dpad, A2DPAD_STATE_CONNECTING_1);
	} else {
		a2dpad_set_state(a2dpad, A2DPAD_STATE_CONNECTING_2);
	}

	BLUEZ_DEBUG_INFO("[A2DPAD] connect_l2cap_a2dp() End");

	return dbus_message_new_method_return(msg);

failed:
	memset( &l2capConfigParam, 0, sizeof(l2capConfigParam) );
	linkinfo = btd_manager_find_connect_info_by_bdaddr( &dst );
	if( linkinfo == MNG_ACL_MAX ) {
		value = 0xFF;
		btd_manager_fake_signal_conn_complete( &dst );
	} else {
		value = FALSE;
	}
	send_signal_connectCfmL2cap(value, cid, &dst, dstLen, status, &l2capConfigParam);

	return dbus_message_new_method_return(msg);
}

static void a2dpad_disconnect(uint16_t cid,  struct audio_device *dev)
{
	struct a2dpad *a2dpad = dev->a2dpad;
	struct avdtp_stream *stream = NULL;

	BLUEZ_DEBUG_INFO("[A2DPAD] a2dpad_disconnect Start");

	if (!a2dpad) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] a2dpad is NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_NULL, 0, 0);
		return;
	}

	if (a2dpad->svclass) {
		bt_cancel_discovery2(&dev->src, &dev->dst, a2dpad->svclass);
		a2dpad->svclass = 0;
	}

	if (a2dpad->sdp_timer) {
		g_source_remove(a2dpad->sdp_timer);
		a2dpad->sdp_timer = 0;
	}

	stream = a2dpad->pending_open;

	if( cid == 0 )
	{
		BLUEZ_DEBUG_LOG("[A2DPAD] a2dpad_disconnect Signaling and Streaming");

		if (a2dpad->io) {
			g_io_channel_shutdown(a2dpad->io, TRUE, NULL);
			g_io_channel_unref(a2dpad->io);
			a2dpad->io = NULL;
		}

		if (a2dpad->io_id) {
			g_source_remove(a2dpad->io_id);
			a2dpad->io_id = 0;
		}
	
		if (stream && stream->io) {
			g_io_channel_shutdown(stream->io, TRUE, NULL);
			g_io_channel_unref(stream->io);
			stream->io = NULL;
		}

		if (stream && stream->io_id) {
			g_source_remove(stream->io_id);
			stream->io_id = 0;
		}
		connect_first = FALSE;
		connect_second = FALSE;
	}
	else if( cid == sig_cidLocal )
	{
		BLUEZ_DEBUG_LOG("[A2DPAD] a2dpad_disconnect Signaling");
		if (a2dpad->io) {
			g_io_channel_shutdown(a2dpad->io, TRUE, NULL);
			g_io_channel_unref(a2dpad->io);
			a2dpad->io = NULL;
		}

		if (a2dpad->io_id) {
			g_source_remove(a2dpad->io_id);
			a2dpad->io_id = 0;
		}
		connect_first = FALSE;
	}
	else if( cid == stream_cidLocal )
	{
		BLUEZ_DEBUG_LOG("[A2DPAD] a2dpad_disconnect Streaming");
		if (stream && stream->io) {
			g_io_channel_shutdown(stream->io, TRUE, NULL);
			g_io_channel_unref(stream->io);
			stream->io = NULL;
		}

		if (stream && stream->io_id) {
			g_source_remove(stream->io_id);
			stream->io_id = 0;

		}
		connect_second = FALSE;
	}
	else
	{
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] cid is incorrect : cid = 0x%02x", cid);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_ERROR, cid, 0);
	}

	return;
}

static void a2dpad_change_state(struct a2dpad *a2dpad)
{
	a2dpad_state_t old_state = a2dpad->state;
	a2dpad_state_t new_state = A2DPAD_STATE_DISCONNECTED;

	switch (old_state) {
	case A2DPAD_STATE_DISCONNECTED:
	case A2DPAD_STATE_CONNECTING_1:
		new_state = A2DPAD_STATE_DISCONNECTED;
		break;
	case A2DPAD_STATE_CONNECTING_2:
	case A2DPAD_STATE_CONNECTED:
		new_state = A2DPAD_STATE_CONNECTING_1;
		break;
	default:
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] Invalid A2DPAD state %d", old_state);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CHANGE_STATE, old_state, 0);
		return;
	}
	a2dpad_set_state(a2dpad, new_state);

	return;
}


static void a2dpad_set_state(struct a2dpad *control, a2dpad_state_t new_state)
{
	struct audio_device *dev = control->dev;
	a2dpad_state_t old_state = control->state;

	switch (new_state) {
	case A2DPAD_STATE_DISCONNECTED:
		BLUEZ_DEBUG_LOG("[A2DPAD] A2DPAD Disconnected");
		a2dpad_disconnect(0, dev);
		connection_initiator = FALSE;	//接続方向の初期化
		break;
	case A2DPAD_STATE_CONNECTING_1:
		BLUEZ_DEBUG_LOG("[A2DPAD] A2DPAD Connecting 1st");
		break;
	case A2DPAD_STATE_CONNECTING_2:
		BLUEZ_DEBUG_LOG("[A2DPAD] A2DPAD Connecting 2nd");
		break;
	case A2DPAD_STATE_CONNECTED:
		BLUEZ_DEBUG_LOG("[A2DPAD] A2DPAD Connected");
		break;
	default:
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] Invalid A2DPAD state %d", new_state);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SET_STATE, new_state, 0);
		return;
	}

	control->state = new_state;
	EVENT_DRC_BLUEZ_STATE(FORMAT_ID_STATE, STATE_A2DP, &dev->dst, new_state);
	btd_manager_set_state_info( &dev->dst, MNG_CON_A2DP_CTRL, new_state );

	BLUEZ_DEBUG_LOG("[A2DPAD] State changed %s: %s -> %s", dev->path, str_state[old_state], str_state[new_state]);
	return;
}

static DBusMessage *disconnect_l2cap_a2dp(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	struct audio_device *device;
	struct a2dpad *a2dpad = NULL;
	const char *dstAddr;
	uint32_t len;
	bdaddr_t dst;
	uint16_t l2capCid;

	BLUEZ_DEBUG_INFO("[A2DPAD] disconnect_l2cap_a2dp() Start");

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_UINT16, &l2capCid,
						DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
						&dstAddr, &len,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[A2DPAD] disconnect_l2cap_a2dp(), dstAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", dstAddr[5], dstAddr[4], dstAddr[3], dstAddr[2], dstAddr[1], dstAddr[0]);

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_A2DP_DISCONNECT, dstAddr);

	memcpy( &dst, dstAddr, sizeof(bdaddr_t) );
	device = manager_find_device(NULL, NULL, &dst, AUDIO_A2DPAD_INTERFACE , FALSE);
	if ( device == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] device ERROR. device = NULL.");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_FIND_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DISCONNECT_FIND_DEV, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}

	a2dpad = device->a2dpad;

	if( l2capCid == sig_cidLocal && a2dpad->io != NULL ) {
		disconnect_initiator_sig = TRUE;
	}
	if( l2capCid == stream_cidLocal && a2dpad->pending_open->io != NULL ) {
		disconnect_initiator_stream = TRUE;
	}

	/* 既に切断処理が行われているか判定 */
	if( disconnect_initiator_sig == TRUE || disconnect_initiator_stream == TRUE ) {
		a2dpad_disconnect(l2capCid, device);
		a2dpad_change_state(a2dpad);
	}

	/* 既に切断通知をあげているか判定 */
	if( l2capCid == sig_cidLocal && btd_manager_isconnect( MNG_CON_A2DP_CTRL ) == FALSE ) {
		BLUEZ_DEBUG_LOG("[A2DPAD] a2dp signaling already disconnected");
		btd_manager_a2dp_signal_disconnect_cfm( FALSE, l2capCid );
		disconnect_initiator_sig = FALSE;
	}
	if( l2capCid == stream_cidLocal && btd_manager_isconnect( MNG_CON_A2DP_STRM ) == FALSE ) {
		BLUEZ_DEBUG_LOG("[A2DPAD] a2dp streaming already disconnected");
		btd_manager_a2dp_signal_disconnect_cfm( FALSE, l2capCid );
		disconnect_initiator_stream = FALSE;
	}

	BLUEZ_DEBUG_INFO("[A2DPAD] disconnect_l2cap_a2dp() End");

	return dbus_message_new_method_return(msg);
}


static DBusMessage *data_snd_l2cap_a2dp(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	struct audio_device *device;
	const char *senddata;
	uint32_t len;
	const char *dstAddr;
	uint32_t addrlen;
	bdaddr_t dst;
	uint16_t l2capPsm;
	uint32_t i;
	struct a2dpad *a2dpad = NULL;
	int sk = 0;

	BLUEZ_DEBUG_INFO("[A2DPAD] data_snd_l2cap_a2dp() Start");

	if (!dbus_message_get_args(msg, NULL,
							   DBUS_TYPE_UINT16, &l2capPsm,
							   DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
							   &dstAddr, &addrlen,
							   DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
							   &senddata, &len,
							   DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[A2DPAD] data_snd_l2cap_a2dp() dstAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", dstAddr[5], dstAddr[4], dstAddr[3], dstAddr[2], dstAddr[1], dstAddr[0]);
	BLUEZ_DEBUG_LOG("[A2DPAD] data_snd_l2cap_a2dp() len = %d.", len);
	for( i = 0; i < len; i++ ) {
		BLUEZ_DEBUG_LOG("[A2DPAD] senddata[%d] = 0x%02x", i, senddata[i]);
	}

	memcpy( &dst, dstAddr, sizeof(bdaddr_t) );
	device = manager_find_device(NULL, NULL, &dst, AUDIO_A2DPAD_INTERFACE, FALSE);
	if ( device == NULL || device->a2dpad == NULL || device->a2dpad->io == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] ERROR. a2dp is closed.");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_FIND_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DATASEND_FIND_DEV, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}
	a2dpad = device->a2dpad;

	sk = g_io_channel_unix_get_fd(a2dpad->io);
	BLUEZ_DEBUG_LOG("[A2DPAD] data_snd_l2cap_a2dp() sk = %d.", sk);
	if (write(sk, senddata, len) < 0){
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] write() ERROR. errno = %d.", errno);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_WRITE, errno, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DATASEND_WRITE, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}

	BLUEZ_DEBUG_INFO("[A2DPAD] data_snd_l2cap_a2dp() End");
	return dbus_message_new_method_return(msg);
}

static DBusMessage *cancel_ACL_a2dp(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	struct audio_device *device;
	struct a2dpad *a2dpad = NULL;
	const char *dstAddr;
	uint32_t len;
	bdaddr_t dst;
	uint16_t l2capCid;
	uint32_t value = 0xFF;
	uint16_t cid = sig_cidLocal;
	int dstLen = sizeof(bdaddr_t);
	uint16_t status = 0;
	L2CAP_CONFIG_PARAM l2capConfigParam;

	BLUEZ_DEBUG_INFO("[A2DPAD] cancel_ACL_a2dp() Start");

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
						&dstAddr, &len,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CANCEL_ACL, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[A2DPAD] cancel_ACL_a2dp(), dstAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", dstAddr[5], dstAddr[4], dstAddr[3], dstAddr[2], dstAddr[1], dstAddr[0]);

	memcpy( &dst, dstAddr, sizeof(bdaddr_t) );
	device = manager_find_device(NULL, NULL, &dst, AUDIO_A2DPAD_INTERFACE , FALSE);
	if ( device == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] device ERROR. device = NULL.");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CANCEL_ACL, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CANCEL_ACL, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}

	a2dpad = device->a2dpad;

	a2dpad_disconnect(0, device);
	a2dpad_change_state(a2dpad);

	memset( &l2capConfigParam, 0, sizeof(l2capConfigParam) );
	send_signal_connectCfmL2cap(value, cid, &dst, dstLen, status, &l2capConfigParam);

	BLUEZ_DEBUG_INFO("[A2DPAD] cancel_ACL_a2dp() End");

	return dbus_message_new_method_return(msg);
}


static GDBusMethodTable a2dpad_methods[] = {
	{ "REGISTER_L2CAP", 	"qay",		"", register_l2cap_a2dp,	0,	0 	},
	{ "DEREGISTER_L2CAP",	"qay",		"", deregister_l2cap_a2dp,	0,	0	},
	{ "CONNECT_L2CAP",		"qay",		"", connect_l2cap_a2dp,		0,	0	},
	{ "DISCONNECT_L2CAP",	"qay",		"", disconnect_l2cap_a2dp,	0,	0	},
	{ "DATA_SND_L2CAP", 	"qayay",	"", data_snd_l2cap_a2dp,	0,	0 	},
	{ "CANCEL_ACL_A2DP",	"ay",		"", cancel_ACL_a2dp,		0,	0	},
	{ NULL, NULL, NULL, NULL, 0, 0 }
};

static GDBusSignalTable a2dpad_signals[] = {
	{ "REGISTER_CFM_L2CAP", 	"q",						G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "DEREGISTER_CFM_L2CAP",	"q",						G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "CONNECT_CFM_L2CAP",		"uuuuuuqqqqqqqqqyyyyyay",	G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "DISCONNECT_CFM_L2CAP",	"qq",						G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "CONNECTED_L2CAP",		"uuuuuuqqqqqqqqqyyyyyyay",	G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "DISCONNECTED_L2CAP", 	"qq",						G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ "DATA_RCV_L2CAP", 		"uqay", 					G_DBUS_SIGNAL_FLAG_DEPRECATED },
	{ NULL, NULL, G_DBUS_SIGNAL_FLAG_DEPRECATED }
};


static void path_unregister(void *data)
{
	BLUEZ_DEBUG_LOG("[A2DPAD] Unregistered interface %s on path %s",
		AUDIO_A2DPAD_INTERFACE, "/");
}

void a2dpad_unregister(struct audio_device *dev)
{
	struct a2dpad *a2dpad = dev->a2dpad;
	int linkinfo = 0;
	L2CAP_CONFIG_PARAM l2capConfigParam;
	uint16_t cid = 0;
	int32_t value = 0xFF;

	BLUEZ_DEBUG_INFO("[A2DPAD] a2dpad unregister Start");

	if ( a2dpad->state > A2DPAD_STATE_DISCONNECTED ) {
		/* 自分から接続 */
		if( connection_initiator == TRUE && a2dpad->state != A2DPAD_STATE_CONNECTED ) {
			if( connect_first == FALSE ) {
				cid = sig_cidLocal;
			} else if( connect_second == FALSE  ) {
				cid = stream_cidLocal;
			} else {
				cid = 0;
			}
			/* ACL が張られていない場合、接続完了通知（失敗）をあげる必要がある */
			linkinfo = btd_manager_find_connect_info_by_bdaddr( &dev->dst );
			if( linkinfo == MNG_ACL_MAX ) {
				if( btd_manager_get_disconnect_info( &dev->dst, MNG_CON_A2DP_CTRL ) == TRUE ) {
					value = FALSE;
				} else {
					value = 0xFF;
				}
				memset( &l2capConfigParam, 0, sizeof(l2capConfigParam) );
				BLUEZ_DEBUG_WARN("[A2DPAD] Unregister occurred : connect_cfm return NG(0x%02x)", value);
				send_signal_connectCfmL2cap(value, cid, &dev->dst, 6, 0, &l2capConfigParam);
			}
		}
		a2dpad_set_state(a2dpad, A2DPAD_STATE_DISCONNECTED);
	}

	g_free(a2dpad);

	dev->a2dpad = NULL;
}

struct a2dpad  *a2dpad_init(struct audio_device *dev)
{
	struct a2dpad *a2dpad;

	BLUEZ_DEBUG_INFO("[A2DPAD] a2dpad_init() Start");
	EVENT_DRC_BLUEZ_INIT(FORMAT_ID_BT_INIT, INIT_A2DP);

	a2dpad = g_new0(struct a2dpad, 1);

	a2dpad->dev = dev;
	a2dpad->pending_open = g_new0(struct avdtp_stream, 1);
	BLUEZ_DEBUG_INFO("[A2DPAD] a2dpad_init() End");

	return a2dpad;
}


int a2dpad_dbus_init(void)
{
	BLUEZ_DEBUG_INFO("[A2DPAD] a2dpad_dbus_init() Start");
	EVENT_DRC_BLUEZ_INIT(FORMAT_ID_BT_INIT, INIT_DBUS_A2DP);

	if (g_a2dpad_connection != NULL  ) {
		g_dbus_unregister_interface(g_a2dpad_connection, "/", AUDIO_A2DPAD_INTERFACE);
		g_a2dpad_connection = NULL;
	}

	g_a2dpad_connection = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
	if (g_dbus_register_interface(g_a2dpad_connection, "/",
					AUDIO_A2DPAD_INTERFACE,
					a2dpad_methods, a2dpad_signals,
					NULL, NULL, path_unregister) == FALSE) {
		 BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] g_dbus_register_interface() ERROR");
		 ERR_DRC_BLUEZ(ERROR_BLUEZ_DBUS_INIT, 0, 0);
		 return -1;
	}
	btd_manager_set_connection_a2dp( g_a2dpad_connection );

	BLUEZ_DEBUG_INFO("[A2DPAD] a2dpad_dbus_init()->g_dbus_register_interface().SUCCESS");
	return 0;
}

void l2capSetCfgDefParam( L2CAP_CONFIG_PARAM *cfgParam )
{
	cfgParam->wFlags			 = L2CAP_CONFIG_DEF_FLAGS;
	cfgParam->wMtu				 = L2CAP_CONFIG_DEF_MTU;
	cfgParam->wFlushTo			 = L2CAP_CONFIG_DEF_FLUSH_TO;
	cfgParam->wLinkTo			 = L2CAP_CONFIG_DEF_LINK_TO;
	cfgParam->bServiceType		 = L2CAP_CONFIG_DEF_SERVICE_TYPE;
	cfgParam->dwTokenRate		 = L2CAP_CONFIG_DEF_TOKEN_RATE;
	cfgParam->dwTokenBucketSize  = L2CAP_CONFIG_DEF_TOKEN_BUCKET_SIZE;
	cfgParam->dwPeakBandwidth	 = L2CAP_CONFIG_DEF_PEAK_BANDWIDTH;
	cfgParam->dwLatency 		 = L2CAP_CONFIG_DEF_LATENCY;
	cfgParam->dwDelayVariation	 = L2CAP_CONFIG_DEF_DELAY_VARIATION;
	cfgParam->bMode 			 = L2CAP_CONFIG_DEF_MODE_FLAGS;
	cfgParam->bTxWindowSize 	 = L2CAP_CONFIG_DEF_TX_WINDOW_SIZE;
	cfgParam->bMaxTransmit		 = L2CAP_CONFIG_DEF_MAX_TRANSMIT;
	cfgParam->wRetTimeout		 = L2CAP_CONFIG_DEF_RET_TIMEOUT;
	cfgParam->wMonTimeout		 = L2CAP_CONFIG_DEF_MON_TIMEOUT;
	cfgParam->wMps				 = L2CAP_CONFIG_DEF_MPS;
	cfgParam->bFcsType			 = L2CAP_CONFIG_DEF_FCS;
}

a2dpad_state_t a2dpad_get_state(struct audio_device *dev)
{
	struct a2dpad *a2dpad = dev->a2dpad;

	if( dev->a2dpad == NULL ) {
		return A2DPAD_STATE_DISCONNECTED;
	}
	return a2dpad->state;
}

static gboolean l2cap_psm_check(int16_t psm)
{
	gboolean ret = FALSE;

	if( psm == MNG_RFCOMM_PSM ||
		psm == MNG_AVCTP_PSM ||
		psm == MNG_AVDTP_PSM ||
		psm == MNG_BROWSING_PSM ) {
		ret = TRUE;
	}
	return ret;
}

// L2CAP 切断イベント受信関連
static gboolean datareceive_l2cap_cb(GIOChannel *chan, GIOCondition cond, void *user_data)
{
	unsigned char buf[20];
	unsigned char *pbuf = buf;
	ssize_t bytes_read;
	int buf_len;
	int fd;
	uint16_t value = TRUE;
	int cnt;
	uint16_t cid;
	struct l2cap_event rcvdata;
	char address[18] = { 0 };

	if (cond & G_IO_NVAL)
	{
		value = false;
		BLUEZ_DEBUG_ERROR("[Bluez - L2CAP] value = %d", value);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_REV_L2CAP_CB, 0, 0);
		return FALSE;
	}

	BLUEZ_DEBUG_INFO("[L2CAP] datareceive_l2cap_cb() Strat");

	if (cond & (G_IO_ERR | G_IO_HUP)) {
		BLUEZ_DEBUG_ERROR("[Bluez - L2CAP] ERR or HUP on L2CAP socket");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_REV_L2CAP_CB, cond, 0);
		return FALSE;
	}

	fd = g_io_channel_unix_get_fd(chan);

	memset(&buf, 0x00, sizeof( buf ));
	bytes_read = read(fd, buf, sizeof(buf) - 1);

	buf_len = bytes_read;

	BLUEZ_DEBUG_LOG("[L2CAP] datareceive_l2cap_cb() buf_len(%d)", buf_len, buf);

	if ( bytes_read < 0 || bytes_read < ( int )sizeof( struct l2cap_event ) )
	{
		BLUEZ_DEBUG_ERROR("[Bluez - L2CAP] Socket Data ERROR bytes_read = %d", bytes_read);
		return TRUE;
	}

	memcpy( &rcvdata, buf, sizeof( rcvdata ) );

	ba2str(&rcvdata.bdaddr, address);

	BLUEZ_DEBUG_WARN("[L2CAP] psm = 0x%02x cid = 0x%04x evt = 0x%02x bdaddr = [ %s ] ",  rcvdata.psm, rcvdata.cid, rcvdata.evt, address );

	/* イベント解析 */
	switch( rcvdata.evt ) {

		case MNG_CONNECT_CFM:
			if( !l2cap_psm_check( rcvdata.psm ) ) {
				/* 不明な L2CAP 接続応答 */
				btd_manager_connect_unknown_profile_l2cap( &rcvdata.bdaddr, rcvdata.psm );
			} else if( rcvdata.psm == MNG_RFCOMM_PSM ) {
				BLUEZ_DEBUG_WARN("[L2CAP] MNG_CONNECT_CFM RFCOMM psm = 0x%02x", rcvdata.psm );
			} else if( rcvdata.psm == MNG_AVDTP_PSM ) {
				BLUEZ_DEBUG_WARN("[L2CAP] MNG_CONNECT_CFM A2DP psm = 0x%02x", rcvdata.psm );
			} else if( rcvdata.psm == MNG_AVCTP_PSM ) {
				BLUEZ_DEBUG_WARN("[L2CAP] MNG_CONNECT_CFM AVRCP psm = 0x%02x", rcvdata.psm );
			} else if( rcvdata.psm == MNG_BROWSING_PSM ) {
				BLUEZ_DEBUG_WARN("[L2CAP] MNG_CONNECT_CFM BROWSING psm = 0x%02x", rcvdata.psm );
			} else {
				/* 有り得ない PSM */
				BLUEZ_DEBUG_ERROR("[Bluez - L2CAP] MNG_CONNECT_CFM psm = 0x%02x", rcvdata.psm );
			}
			break;

		case MNG_CONNECTED:
			if( !l2cap_psm_check( rcvdata.psm ) ) {
				/* 不明な L2CAP 接続要求 */
				btd_manager_connect_unknown_profile_l2cap( &rcvdata.bdaddr, rcvdata.psm );
			} else if( rcvdata.psm == MNG_RFCOMM_PSM ) {
				BLUEZ_DEBUG_WARN("[L2CAP] MNG_CONNECTED RFCOMM psm = 0x%02x", rcvdata.psm );
			} else if( rcvdata.psm == MNG_AVDTP_PSM ) {
				BLUEZ_DEBUG_WARN("[L2CAP] MNG_CONNECTED A2DP psm = 0x%02x", rcvdata.psm );
			} else if( rcvdata.psm == MNG_AVCTP_PSM ) {
				BLUEZ_DEBUG_WARN("[L2CAP] MNG_CONNECTED AVRCP psm = 0x%02x", rcvdata.psm );
			} else if( rcvdata.psm == MNG_BROWSING_PSM ) {
				BLUEZ_DEBUG_WARN("[L2CAP] MNG_CONNECTED BROWSING psm = 0x%02x", rcvdata.psm );
			} else {
				/* 有り得ない PSM */
				BLUEZ_DEBUG_ERROR("[Bluez - L2CAP] MNG_CONNECTED psm = 0x%02x", rcvdata.psm );
			}
			break;

		case MNG_DISCONNECT_CFM:
			/* 切断応答を受取った */
			if( !l2cap_psm_check( rcvdata.psm ) ) {
				/* 不明な L2CAP 切断応答 */
				btd_manager_disconnect_unknown_profile_l2cap( &rcvdata.bdaddr, rcvdata.psm );
				break;
			} 
			if( rcvdata.cid == sig_cidLocal ) {
				if ( disconnect_initiator_sig != TRUE ) {
					btd_manager_disconnected_profile_l2cap( &rcvdata.bdaddr, TRUE, rcvdata.cid );
				} else {
					btd_manager_disconnect_cfm_profile_l2cap( &rcvdata.bdaddr, TRUE, rcvdata.cid );
				}
				disconnect_initiator_sig = FALSE;
				break;
			}
			if( rcvdata.cid == stream_cidLocal ) {
				if ( disconnect_initiator_stream != TRUE ) {
					btd_manager_disconnected_profile_l2cap( &rcvdata.bdaddr, TRUE, rcvdata.cid );
				} else {
					btd_manager_disconnect_cfm_profile_l2cap( &rcvdata.bdaddr, TRUE, rcvdata.cid );
				}
				disconnect_initiator_stream = FALSE;
				break;
			}
			if( rcvdata.cid == avrcpad_get_cid() ) {
				if ( avrcpad_get_disconnect_initiator() != TRUE ) {
					btd_manager_disconnected_profile_l2cap( &rcvdata.bdaddr, TRUE, rcvdata.cid );
				} else {
					btd_manager_disconnect_cfm_profile_l2cap( &rcvdata.bdaddr, TRUE, rcvdata.cid );
				}
				avrcpad_set_disconnect_initiator( FALSE );
				break;
			}
			if( rcvdata.cid == browsing_get_cid() ) {
				if ( browsing_get_disconnect_initiator() != TRUE ) {
					btd_manager_disconnected_profile_l2cap( &rcvdata.bdaddr, TRUE, rcvdata.cid );
				} else {
					btd_manager_disconnect_cfm_profile_l2cap( &rcvdata.bdaddr, TRUE, rcvdata.cid );
				}
				browsing_set_disconnect_initiator( FALSE );
				break;
			}
			btd_manager_disconnected_profile_l2cap( &rcvdata.bdaddr, TRUE, rcvdata.cid );
			break;

		case MNG_DISCONNECTED:
			/* 切断指示を受取った */
			if( !l2cap_psm_check( rcvdata.psm ) ) {
				/* 不明な L2CAP 切断指示 */
				btd_manager_disconnect_unknown_profile_l2cap( &rcvdata.bdaddr, rcvdata.psm );
				break;
			} 
			btd_manager_disconnected_profile_l2cap( &rcvdata.bdaddr, TRUE, rcvdata.cid );
			if( rcvdata.cid == sig_cidLocal ) {
				disconnect_initiator_sig = FALSE;
				break;
			}
			if( rcvdata.cid == stream_cidLocal ) {
				disconnect_initiator_stream = FALSE;
				break;
			}
			if( rcvdata.cid == avrcpad_get_cid() ) {
				avrcpad_set_disconnect_initiator( FALSE );
				break;
			}
			if( rcvdata.cid == browsing_get_cid() ) {
				browsing_set_disconnect_initiator( FALSE );
				break;
			}
			break;

		default:
			BLUEZ_DEBUG_ERROR("[Bluez - L2CAP] DATA error!!");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_REV_L2CAP_CB, rcvdata.evt, 0);
			break;
	}

	return TRUE;
}

void l2cap_accept_cb(GIOChannel *chan, GError *err, gpointer user_data)
{
	BLUEZ_DEBUG_INFO("[A2DPAD] l2cap_accept_cb() Strat");

	g_io_add_watch(chan, G_IO_IN | G_IO_ERR | G_IO_HUP| G_IO_NVAL,
			(GIOFunc) datareceive_l2cap_cb, NULL);

	return;
}

static void l2cap_confirm(GIOChannel *chan, gpointer data)
{
	GError *err = NULL;
	GIOChannel *io;
	gboolean ret;

	BLUEZ_DEBUG_INFO("[A2DPAD] l2cap_confirm() Strat");

	io = g_io_channel_ref(chan);

	ret = bt_io_accept(io, l2cap_accept_cb, NULL, NULL, &err);
	if ( ret != TRUE ) {
		BLUEZ_DEBUG_ERROR("[Bluez - A2DPAD] bt_io_accept() : [ %s ]", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_IO_ACCEPT, err->code, 0);
		g_error_free(err);
	}
	BLUEZ_DEBUG_INFO("[A2DPAD] l2cap_confirm() End");

	return;
}
