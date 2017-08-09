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
#include <fcntl.h>

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
#include "handsfree.h"
#include "pbap.h"
#include "dbus-common.h"
#include "../src/adapter.h"
#include "../src/device.h"
#include "glib-helper.h"
#include "telephony.h"

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
#define BLUEZ_SRC_ID BLUEZ_SRC_ID_AUDIO_HANDSFREE_C

#define BUF_SIZE 1024
#define HF_SDP_TIMER 10
#define HF_SDP_DEFAULT_NETWORK 0
#define HF_SDP_DEFAULT_FEATURES 0x00000009

#define __srv_channel(dlci)	(dlci >> 1)
#define __dir(dlci) 		(dlci & 0x01)

static gboolean connection_initiator = FALSE; //FALSE 相手からの接続 
static GIOChannel *g_hf_rfcomm_io = NULL;
static uint32_t g_hf_record_id = 0;
static DBusConnection *g_hf_connection = NULL;
static GIOChannel *g_rfcomm_event    = NULL;
static GIOChannel *g_rfcomm_event_io = NULL;
static gboolean disconnect_initiator = FALSE; //FALSE 相手からの切断 
static int g_hf_sdp_rfcommCh = 0x01;	/* HFP は 1（上位からもらえる）  */
static int g_hf_rfcommCh = 0xFF;		/* 相手の SDP に従う  */

static char *str_state[] = {
	"HANDSFREE_STATE_DISCONNECTED",
	"HANDSFREE_STATE_CONNECTING",
	"HANDSFREE_STATE_CONNECT_AFTER_SDP",
	"HANDSFREE_STATE_CONNECTED",
	"HANDSFREE_STATE_PLAY_IN_PROGRESS",
	"HANDSFREE_STATE_PLAYING"
};

struct pending_connect {
	uint16_t svclass;
};

struct handsfree {
	struct audio_device *dev;
	uint32_t hfp_handle;
	int rfcomm_ch;
	GIOChannel *rfcomm;
	GIOChannel *tmp_rfcomm;
	GIOChannel *sco;
	guint sco_id;
	gboolean hfp_active;
	handsfree_state_t state;
	struct pending_connect *pending;
	guint sdp_timer;
	uint16_t svclass_did;
	gboolean sdp_flg;
	uint16_t features;
	uint8_t network;
};

struct rfcomm_event {
	uint8_t dlci;
	uint8_t evt;
	bdaddr_t bdaddr;
} __packed;

static sdp_record_t *hfp_sdp_record(uint8_t ch);
int rfcomm_connect(int rfChannel, struct audio_device *dev);
int rfcomm_send(struct handsfree *hf, int len, const char *sendData, ...);
static int get_records(struct audio_device *device);
static int get_records_after_connect(struct audio_device *device);
static int get_records_after_connect_did(struct audio_device *device);
static int get_records_did(struct audio_device *device);
void handsfree_shutdown(struct audio_device *dev);
static void rfcomm_confirm(GIOChannel *chan, gpointer data);

static void send_signal_connected(int32_t value, uint16_t features, uint8_t network, uint8_t svch, bdaddr_t *dst )
{
	char  bdaddr[BDADDR_SIZE];
	char *pbdaddr;

	memcpy(bdaddr, dst, sizeof(bdaddr));
	pbdaddr = &bdaddr[0];

	BLUEZ_DEBUG_LOG("[HFP] bdaddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", bdaddr[5], bdaddr[4], bdaddr[3], bdaddr[2], bdaddr[1], bdaddr[0]);
	BLUEZ_DEBUG_LOG("[HFP] value = %d features = %d network = %d svch = %d", value, features, network, svch);

	g_dbus_emit_signal(g_hf_connection, "/",
						AUDIO_HANDSFREE_INTERFACE,
						"CONNECTED",
						DBUS_TYPE_INT32, &value,
						DBUS_TYPE_UINT16, &features,	//FEATURES
						DBUS_TYPE_BYTE, &network,		//NETWORK
						DBUS_TYPE_BYTE, &svch, 			//Sever CH
						DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
						&pbdaddr,  6,
						DBUS_TYPE_INVALID);

	/* 切断情報の初期化 */
	btd_manager_get_disconnect_info( dst, MNG_CON_HANDSFREE );

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HFP_CONNECTED, value);

	return;
}

static void send_signal_connectcfm(int32_t value, uint16_t features, uint8_t network, uint8_t svch, bdaddr_t *dst )
{
	BLUEZ_DEBUG_LOG("[HFP] value = %d features = %d network = %d svch = %d", value, features, network, svch);

	g_dbus_emit_signal(g_hf_connection, "/",
						AUDIO_HANDSFREE_INTERFACE,
						"CONNECT_CFM",
						DBUS_TYPE_INT32, &value,
						DBUS_TYPE_UINT16, &features,	//FEATURES
						DBUS_TYPE_BYTE, &network,		//NETWORK
						DBUS_TYPE_BYTE, &svch, 			//Sever CH
						DBUS_TYPE_INVALID);

	/* 切断情報の初期化 */
	btd_manager_get_disconnect_info( dst, MNG_CON_HANDSFREE );

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HFP_CONNECT_CFM, value);

	return;
}

static int handsfree_send_atcmd(struct handsfree *hf, int len, const char *format, va_list ap)
{
	char rsp[BUF_SIZE];
	ssize_t total_written, count;
	int fd;
	
	BLUEZ_DEBUG_INFO("[HFP] handsfree_send_atcmd() Start.");
	
	vsnprintf(rsp, sizeof(rsp), format, ap);
	BLUEZ_DEBUG_LOG("[HF_022_001]--- handsfree_send_atcmd vsnprintf ---");
	count = len;

	BLUEZ_DEBUG_LOG("[HFP] AT_C Len:%d", (int)count);
	
	if (count <= 0)
	{
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] AT_CMD LEN = 0");
		BLUEZ_DEBUG_LOG("[HF_022_002]--- handsfree_send_atcmd -EINVAL ---");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SEND_ATCMD, count, 0);
		return -EINVAL;
	}

	if (!hf->rfcomm) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] handsfree is not connected");
		BLUEZ_DEBUG_LOG("[HF_022_003]--- handsfree_send_atcmd -EIO ---");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SEND_ATCMD, 0, 0);
		return -EIO;
	}

	total_written = 0;
	fd = g_io_channel_unix_get_fd(hf->rfcomm);
	BLUEZ_DEBUG_LOG("[HF_022_004]--- handsfree_send_atcmd fd = %d ---", fd);

	while (total_written < (count)) {
		ssize_t written;
		written = write(fd, rsp + total_written,
				count - total_written);
		BLUEZ_DEBUG_LOG("[HF_022_005]--- handsfree_send_atcmd written = %d ---", (int)written);
		if (written < 0){
			BLUEZ_DEBUG_ERROR("[Bluez - HFP] Error written < 0");
			BLUEZ_DEBUG_LOG("[HF_022_006]--- handsfree_send_atcmd errno = %d ---", errno);
			ERR_DRC_BLUEZ(ERROR_BLUEZ_SEND_ATCMD, errno, 0);
			return -errno;
		}
		BLUEZ_DEBUG_MUST_LOG("[HFP] AT_Snd \"%s\"", rsp + total_written);
		BLUEZ_DEBUG_LOG("[HFP] written:%d", (int)written);
		total_written += written;
		BLUEZ_DEBUG_LOG("[HFP] total_written:%d", (int)total_written);
	}
	BLUEZ_DEBUG_INFO("[HFP] handsfree_send_atcmd() End.");
	return 0;
}

static int handsfree_send_event(struct handsfree *hf, int len, const char *format, va_list ap)
{
	char rsp[BUF_SIZE];
	ssize_t total_written, count;
	int fd;
	
	BLUEZ_DEBUG_INFO("[HFP] handsfree_send_event() Start.");
	
	vsnprintf(rsp, sizeof(rsp), format, ap);
	BLUEZ_DEBUG_LOG("[HF_022_001]--- handsfree_send_event vsnprintf ---");
	count = len;

	BLUEZ_DEBUG_LOG("[HFP] AT_C Len:%d", (int)count);
	
	if (count <= 0)
	{
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] AT_CMD LEN = 0");
		BLUEZ_DEBUG_LOG("[HF_022_002]--- handsfree_send_event -EINVAL ---");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SEND_ATCMD, count, 0);
		return -EINVAL;
	}
	
	if (!g_rfcomm_event_io) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] handsfree is not connected");
		BLUEZ_DEBUG_LOG("[HF_022_003]--- handsfree_send_event -EIO ---");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SEND_ATCMD, 0, 0);
		return -EIO;
	}

	total_written = 0;
	fd = g_io_channel_unix_get_fd(g_rfcomm_event_io);
	BLUEZ_DEBUG_LOG("[HF_022_004]--- handsfree_send_event fd = %d ---", fd);

	while (total_written < (count)) {
		ssize_t written;
		written = write(fd, rsp + total_written,
				count - total_written);
		BLUEZ_DEBUG_LOG("[HF_022_005]--- handsfree_send_event written = %d ---", (int)written);
		if (written < 0){
			BLUEZ_DEBUG_ERROR("[Bluez - HFP] Error written < 0");
			BLUEZ_DEBUG_LOG("[HF_022_006]--- handsfree_send_event errno = %d ---", errno);
			ERR_DRC_BLUEZ(ERROR_BLUEZ_SEND_ATCMD, errno, 0);
			return -errno;
		}
		BLUEZ_DEBUG_LOG("[HFP] written:%d", (int)written);
		total_written += written;
		BLUEZ_DEBUG_LOG("[HFP] total_written:%d", (int)total_written);
	}
	BLUEZ_DEBUG_INFO("[HFP] handsfree_send_atcmd() End.");
	return 0;
}

//ソケット受信関数(コールバック)
static gboolean datareceive_hf_cb(GIOChannel *chan, GIOCondition cond,
								struct audio_device *device)
{
	struct handsfree *hf;
	unsigned char buf[BUF_SIZE];
	unsigned char log[BUF_SIZE];
	unsigned char *pbuf = buf;
	ssize_t bytes_read;
	int buf_len;
	size_t free_space;
	int fd;
	uint16_t value = TRUE;

	BLUEZ_DEBUG_INFO("[HFP] datareceive_hf_cb() Start.");

	if (cond & G_IO_NVAL)
	{
		BLUEZ_DEBUG_WARN("[HFP] HFP NVAL on RFCOMM socket");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_IO, 0, 0);
		return FALSE;
	}

	if( handsfree_get_state(device) == HANDSFREE_STATE_DISCONNECTED ) {
		BLUEZ_DEBUG_WARN("[HFP] HFP is already disconnected");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_DISCONNECTED, 0, 0);
		return TRUE;
	}

	hf = device->handsfree;

	if (cond & (G_IO_ERR | G_IO_HUP)) {
		BLUEZ_DEBUG_WARN("[HFP] ERR or HUP on RFCOMM socket");
		BLUEZ_DEBUG_LOG("[HF_012_002]--- datareceive_hf_cb goto failed; ---");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_IO, cond, 0);
		goto failed;
	}

	fd = g_io_channel_unix_get_fd(chan);
	BLUEZ_DEBUG_LOG("[HF_012_003]--- datareceive_hf_cb fd = %d ---", fd);

	memset( buf, 0x00, sizeof(buf) );

	bytes_read = read(fd, buf, sizeof(buf) - 1);
	BLUEZ_DEBUG_LOG("[HF_012_004]--- datareceive_hf_cb bytes_read = %d ---", (int)bytes_read);

	buf_len = bytes_read;

	memset( log, 0x00, sizeof(buf) );
	memcpy( log, &buf[2], buf_len - 4 );
	BLUEZ_DEBUG_MUST_LOG("[HFP] AT_Rcv \"%s\"", log);

	if (bytes_read < 0)
	{
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] bytes_read < 0");
		BLUEZ_DEBUG_LOG("[HF_012_005]--- datareceive_hf_cb bytes_< 0 ---");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_READ, 0, 0);
		return TRUE;
	}

	free_space = sizeof(buf) - 1;

	if (free_space < (size_t) bytes_read) {
		/* Very likely that the HF is sending us garbage so
		 * just ignore the data and disconnect */
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] Too much data to fit incomming buffer");
		BLUEZ_DEBUG_LOG("[HF_012_006]--- datareceive_hf_cb goto failed; ---");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_OVER_BUF, free_space, bytes_read);
		goto failed;
	}

	g_dbus_emit_signal(g_hf_connection, "/",
				AUDIO_HANDSFREE_INTERFACE,
				"DATA_RCV",
				DBUS_TYPE_UINT16, &value,		//データ受信通知の成功/失敗
				DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
				&pbuf, buf_len,
				DBUS_TYPE_INVALID);
	BLUEZ_DEBUG_LOG("[HF_012_011]--- datareceive_hf_cb g_dbus_emit_signal ---");

	BLUEZ_DEBUG_LOG("[HF_012_012]--- datareceive_hf_cb Return TRUE---");
	BLUEZ_DEBUG_INFO("[HFP] datareceive_hf_cb() End.");
	return TRUE;

failed:
	handsfree_set_state(device, HANDSFREE_STATE_DISCONNECTED);
	return FALSE;
}

static void pending_connect_init(struct handsfree *hf, handsfree_state_t target_state)
{
	BLUEZ_DEBUG_INFO("[HFP] pending_connect_init() Start.");

	hf->pending = g_new0(struct pending_connect, 1);
	BLUEZ_DEBUG_LOG("[HF_014_002]--- pending_connect_init hf->pending = %p ---", hf->pending);
	BLUEZ_DEBUG_INFO("[HFP] pending_connect_init() End.");
	return;
}

static void pending_connect_finalize(struct audio_device *dev)
{
	struct handsfree *hf = dev->handsfree;
	struct pending_connect *p = hf->pending;

	BLUEZ_DEBUG_INFO("[HFP] pending_connect_finalize() Start.");

	if (p == NULL)
	{
		BLUEZ_DEBUG_LOG("[HFP] pending_connect_finalize p == NULL");
		return;
	}

	if (p->svclass)
	{
		bt_cancel_discovery2(&dev->src, &dev->dst, p->svclass);
		BLUEZ_DEBUG_LOG("[HF_015_002]--- pending_connect_finalize bt_cancel_discovery2 ---");
	}

	g_free(p);
	BLUEZ_DEBUG_LOG("[HF_015_011]--- pending_connect_finalize g_free ---");

	hf->pending = NULL;
	BLUEZ_DEBUG_INFO("[HFP] pending_connect_finalize() End.");
}

static gboolean sco_cb(GIOChannel *chan, GIOCondition cond, struct audio_device *device)
{
	BLUEZ_DEBUG_INFO("[HFP] sco_cb() Start.");
	if (cond & G_IO_NVAL)
	{
		BLUEZ_DEBUG_WARN("[HFP] G_IO_NVAL ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SCO_CB, 0, 0);
		return FALSE;
	}

	pending_connect_finalize(device);
	BLUEZ_DEBUG_LOG("[HF_025_002]--- sco_cb pending_connect_finalize ---");
	handsfree_set_state(device, HANDSFREE_STATE_CONNECTED);
	BLUEZ_DEBUG_LOG("[HF_025_003]--- sco_cb handsfree_set_state ---");
	BLUEZ_DEBUG_INFO("[HFP] sco_cb() End.");

	return FALSE;
}

GIOChannel *handsfree_get_rfcomm(struct audio_device *dev)
{
	struct handsfree *hf = dev->handsfree;

	BLUEZ_DEBUG_INFO("[HFP] handsfree_get_rfcomm() Start.");

	BLUEZ_DEBUG_LOG("[HF_029_001]--- handsfree_get_rfcomm hf->tmp_rfcomm = %p ---", hf->tmp_rfcomm);
	return hf->tmp_rfcomm;
}

int handsfree_connect_rfcomm(struct audio_device *dev, GIOChannel *io)
{
	struct handsfree *hf = dev->handsfree;

	BLUEZ_DEBUG_INFO("[HFP] handsfree_connect_rfcomm() Start.");

	if (hf->tmp_rfcomm)
	{
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] tmp_rfcomm == NULL : -EALREADY");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_RFCOMM_CONNECTED, 0, 0);
		return -EALREADY;
	}

	hf->tmp_rfcomm = g_io_channel_ref(io);
	BLUEZ_DEBUG_LOG("[HF_032_002]--- handsfree_connect_rfcomm hf->tmp_rfcomm = %p ---", hf->tmp_rfcomm);
	BLUEZ_DEBUG_INFO("[HFP] handsfree_connect_rfcomm() End.");
	return 0;
}

static int handsfree_close_rfcomm(struct audio_device *dev)
{
	struct handsfree *hf = dev->handsfree;
	GIOChannel *rfcomm = hf->tmp_rfcomm ? hf->tmp_rfcomm : hf->rfcomm;

	BLUEZ_DEBUG_INFO("[HFP] handsfree_close_rfcomm() Start.");

	if (rfcomm) {
		g_io_channel_shutdown(rfcomm, TRUE, NULL);
		BLUEZ_DEBUG_LOG("[HF_019_001]--- handsfree_close_rfcomm g_io_channel_shutdown ---");
		g_io_channel_unref(rfcomm);
		BLUEZ_DEBUG_LOG("[HF_019_002]--- handsfree_close_rfcomm g_io_channel_unref ---");
		hf->tmp_rfcomm = NULL;
		hf->rfcomm = NULL;
	}

	if (hf->sdp_timer) {
		g_source_remove(hf->sdp_timer);
		hf->sdp_timer = 0;
	}
	if (hf->svclass_did != 0)
	{
		bt_cancel_discovery2(&dev->src, &dev->dst, hf->svclass_did);
		hf->svclass_did = 0;
	}

	BLUEZ_DEBUG_INFO("[HFP] handsfree_close_rfcomm() End.");

	return 0;
}

static void close_sco(struct audio_device *device)
{
	struct handsfree *hf = device->handsfree;

	BLUEZ_DEBUG_INFO("[HFP] close_sco() Start.");

	if (hf->sco) {
		int sock = g_io_channel_unix_get_fd(hf->sco);
		BLUEZ_DEBUG_LOG("[HF_026_001]--- close_sco sock = %d ---", sock);
		shutdown(sock, SHUT_RDWR);
		BLUEZ_DEBUG_LOG("[HF_026_002]--- close_sco shutdown ---");
		g_io_channel_shutdown(hf->sco, TRUE, NULL);
		BLUEZ_DEBUG_LOG("[HF_026_003]--- close_sco g_io_channel_shutdown ---");
		g_io_channel_unref(hf->sco);
		BLUEZ_DEBUG_LOG("[HF_026_004]--- close_sco g_io_channel_unref ---");
		hf->sco = NULL;
	}

	if (hf->sco_id) {
		g_source_remove(hf->sco_id);
		BLUEZ_DEBUG_LOG("[HF_026_005]--- close_sco g_source_remove ---");
		hf->sco_id = 0;
	}
	BLUEZ_DEBUG_INFO("[HFP] close_sco() End.");
}

static const char *state2str(handsfree_state_t state)
{
	BLUEZ_DEBUG_INFO("[HFP] state2str() Start.");
	BLUEZ_DEBUG_LOG("[HFP] state2str() state = %d", state);
	switch (state) {
	case HANDSFREE_STATE_DISCONNECTED:
		BLUEZ_DEBUG_LOG("[HF_027_001]--- state2str return DISCONNECTED ---");
		return "DISCONNECTED";
	case HANDSFREE_STATE_CONNECTING:
		BLUEZ_DEBUG_LOG("[HF_027_002]--- state2str return connecting ---");
		return "connecting";
	case HANDSFREE_STATE_CONNECTED:
	case HANDSFREE_STATE_PLAY_IN_PROGRESS:
		BLUEZ_DEBUG_LOG("[HF_027_004]--- state2str return CONNECTED ---");
		return "CONNECTED";
	case HANDSFREE_STATE_PLAYING:
		BLUEZ_DEBUG_LOG("[HF_027_005]--- state2str return playing ---");
		return "playing";
	case HANDSFREE_STATE_CONNECT_AFTER_SDP:
		BLUEZ_DEBUG_LOG("[HF_027_007]--- state2str return CONNECT_AFTER_SDP ---");
		return "CONNECT_AFTER_SDP";
	default:
		break;
	}
	BLUEZ_DEBUG_LOG("[HF_027_006]--- state2str return NULL ---");
	BLUEZ_DEBUG_INFO("[HFP] state2str() NULL End.");

	return NULL;
}

handsfree_state_t handsfree_get_state(struct audio_device *dev)
{
	struct handsfree *hf = dev->handsfree;

	BLUEZ_DEBUG_INFO("[HFP] handsfree_get_state() Start.");

	BLUEZ_DEBUG_LOG("[HF_017_001]--- handsfree_get_state hf->state = %d ---", hf->state);
	BLUEZ_DEBUG_INFO("[HFP] handsfree_get_state() End.");
	return hf->state;
}

static void sco_connect_cb(GIOChannel *chan, GError *err, gpointer user_data)
{
	int sk;
	struct audio_device *dev = user_data;
	struct handsfree *hf = dev->handsfree;
	struct pending_connect *p = hf->pending;

	BLUEZ_DEBUG_INFO("[HFP] sco_connect_cb() Start.");
	EVENT_DRC_BLUEZ_CB(FORMAT_ID_BT_CB, CB_SCO_CONNECT);
	
	if (err) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] ERROR = [ %s ]", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SCO_CONNECT_CB, err->code, 0);
		
		pending_connect_finalize(dev);
		
		handsfree_set_state(dev, HANDSFREE_STATE_CONNECTED);
		
		if (hf->rfcomm)
		{
			close_sco(dev);
		}
		return;
	}

	BLUEZ_DEBUG_LOG("[HFP] SCO socket opened for handsfree %s", dev->path);

	sk = g_io_channel_unix_get_fd(chan);

	BLUEZ_DEBUG_LOG("[HFP] SCO fd = %d", sk);

	if (p) {
		pending_connect_finalize(dev);
	}

	fcntl(sk, F_SETFL, 0);

	handsfree_set_state(dev, HANDSFREE_STATE_PLAYING);
}

int handsfree_connect_sco(struct audio_device *dev, GIOChannel *io)
{
	struct handsfree *hf = dev->handsfree;

	BLUEZ_DEBUG_INFO("[HFP] handsfree_connect_sco() Start.");

	if (hf->sco){
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] sco == NULL : -EISCONN");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_SCO, 0, 0);
		return -EISCONN;
	}

	hf->sco = g_io_channel_ref(io);
	BLUEZ_DEBUG_LOG("[HF_024_002]--- handsfree_connect_sco hf->sco = %p ---", hf->sco);

	BLUEZ_DEBUG_INFO("[HFP] handsfree_connect_sco() End.");
	return 0;
}

void handsfree_set_state(struct audio_device *dev, handsfree_state_t state)
{
	struct handsfree *hf = dev->handsfree;
	const char *state_str;
	handsfree_state_t old_state = hf->state;

	BLUEZ_DEBUG_INFO("[HFP] handsfree_set_state() Start.");

	if (old_state == state)
	{
		BLUEZ_DEBUG_LOG("[HF_018_001]--- handsfree_set_state old_state == state ---");
		return;
	}

	state_str = state2str(state);
	BLUEZ_DEBUG_LOG("[HF_018_002]--- handsfree_set_state state_str = %s ---", state_str);

	switch (state) {
	case HANDSFREE_STATE_DISCONNECTED:
		BLUEZ_DEBUG_LOG("[HF_018_011]--- handsfree_set_state HANDSFREE_STATE_DISCONNECTED ---");
		close_sco(dev);
		handsfree_close_rfcomm(dev);
		connection_initiator = FALSE;	//接続方向の初期化
		hf->sdp_flg = FALSE;				//SDP取得情報初期化
		g_hf_rfcommCh = 0xFF;			//相手チャンネル初期化
		break;
	case HANDSFREE_STATE_CONNECTING:
		BLUEZ_DEBUG_LOG("[HF_018_011]--- handsfree_set_state HANDSFREE_STATE_CONNECTING ---");
		break;
	case HANDSFREE_STATE_CONNECT_AFTER_SDP:
		BLUEZ_DEBUG_LOG("[HF_018_0XX]--- handsfree_set_state HANDSFREE_STATE_CONNECT_AFTER_SDP ---");
		get_records_after_connect_did(dev);
		break;
	case HANDSFREE_STATE_CONNECTED:
		BLUEZ_DEBUG_LOG("[HFP] handsfree_set_state() HANDSFREE_STATE_CONNECTED");
		close_sco(dev);
		BLUEZ_DEBUG_LOG("[HF_018_012]--- handsfree_set_state close_sco ---");
		break;
	case HANDSFREE_STATE_PLAY_IN_PROGRESS:
		BLUEZ_DEBUG_LOG("[HF_018_014]--- handsfree_set_state HANDSFREE_STATE_PLAY_IN_PROGRESS ---");
		break;
	case HANDSFREE_STATE_PLAYING:
		/* Do not watch HUP since we need to know when the link is
		   really disconnected */
		hf->sco_id = g_io_add_watch(hf->sco,
					G_IO_ERR | G_IO_NVAL,
					(GIOFunc) sco_cb, (void *)dev);
		BLUEZ_DEBUG_LOG("[HF_018_015]--- handsfree_set_state hf->sco_id = %d ---", hf->sco_id);
		break;
	default:
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] INVALID STATE");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SET_STATE, state, 0);
		break;
	}

	hf->state = state;
	EVENT_DRC_BLUEZ_STATE(FORMAT_ID_STATE, STATE_HFP, &dev->dst, state);
	btd_manager_set_state_info( &dev->dst, MNG_CON_HANDSFREE, state );

	BLUEZ_DEBUG_LOG("[HFP] State changed %s: %s -> %s", dev->path, str_state[old_state],
		str_state[state]);

	BLUEZ_DEBUG_INFO("[HFP] handsfree_set_state() End.");
}

//コールバック
void handsfree_connect_cb(GIOChannel *chan, GError *err, gpointer user_data)
{
	struct audio_device *dev = user_data;
	struct handsfree *hf = dev->handsfree;
	char hf_address[18];
	int32_t value;

	BLUEZ_DEBUG_INFO("[HFP] handsfree_connect_cb() Start.");
	EVENT_DRC_BLUEZ_CONNECT_CB(FORMAT_ID_CONNECT_CB, CB_HFP_CONNECT, connection_initiator);

	if (err) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] chan = %p err = [%s]", chan, err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_CB_ERR, err->code, 0);
		goto failed;
	}

	hf->rfcomm = hf->tmp_rfcomm;
	hf->tmp_rfcomm = NULL;

	ba2str(&dev->dst, hf_address);

	g_io_add_watch(chan, G_IO_IN | G_IO_ERR | G_IO_HUP| G_IO_NVAL,
			(GIOFunc) datareceive_hf_cb, (void *)dev);
	BLUEZ_DEBUG_LOG("[HF_011_004]--- handsfree_connect_cb g_io_add_watch ---");

	BLUEZ_DEBUG_LOG("[HFP] %s: Connected to %s", dev->path, hf_address);

	if( TRUE == connection_initiator && hf->sdp_flg == TRUE ) {

		//CFMの場合
		value = TRUE;
		send_signal_connectcfm( value, hf->features, hf->network, ( uint8_t )hf->rfcomm_ch, &dev->dst );

		BLUEZ_DEBUG_LOG("[HF_011_007]--- handsfree_connect_cb g_dbus_emit_signal ---");
		handsfree_set_state(dev, HANDSFREE_STATE_CONNECTED);
		BLUEZ_DEBUG_LOG("[HF_011_008]--- handsfree_connect_cb handsfree_set_state ---");

		pending_connect_finalize(dev);
		BLUEZ_DEBUG_LOG("[HF_011_0011]--- handsfree_connect_cb pending_connect_finalize ---");

		btd_manager_connect_profile( &dev->dst, MNG_CON_HANDSFREE, dev->handsfree->rfcomm_ch, 0 );

	} else {
		pending_connect_finalize(dev);
		BLUEZ_DEBUG_LOG("[HF_011_0011]--- handsfree_connect_cb pending_connect_finalize ---");

		handsfree_set_state(dev, HANDSFREE_STATE_CONNECT_AFTER_SDP);
	}
	BLUEZ_DEBUG_INFO("[HFP] handsfree_connect_cb() End.");

	return;

failed:
	BLUEZ_DEBUG_INFO("[HFP] handsfree_connect_cb() failed Start.");
	pending_connect_finalize(dev);
	BLUEZ_DEBUG_LOG("[HF_011_0013]--- handsfree_connect_cb pending_connect_finalize ---");
	if (TRUE == connection_initiator){
		//CFMの場合
		value = FALSE;
		send_signal_connectcfm( value, HF_SDP_DEFAULT_FEATURES, HF_SDP_DEFAULT_NETWORK, 0, &dev->dst );
		BLUEZ_DEBUG_LOG("[HF_011_0014]--- handsfree_connect_cb g_dbus_emit_signal ---");
		EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HFP_CONNECT_CFM, value);

		handsfree_set_state(dev, HANDSFREE_STATE_DISCONNECTED);
		BLUEZ_DEBUG_LOG("[HF_011_0015]--- handsfree_connect_cb handsfree_set_state ---");
	}
	else{
		handsfree_set_state(dev, HANDSFREE_STATE_DISCONNECTED);
		BLUEZ_DEBUG_LOG("[HF_011_0016]--- handsfree_connect_cb handsfree_set_state ---");
	}
	BLUEZ_DEBUG_INFO("[HFP] handsfree_connect_cb() failed End.");
}

static gboolean hs_preauth_cb(GIOChannel *chan, GIOCondition cond,
							gpointer user_data)
{
	struct audio_device *device = user_data;

	BLUEZ_DEBUG_INFO("[HFP] hs_preauth_cb() Start.");

	BLUEZ_DEBUG_LOG("[HFP] Handsfree disconnected during authorization");

	handsfree_set_state(device, HANDSFREE_STATE_DISCONNECTED);
	BLUEZ_DEBUG_LOG("[HF_031_002]--- hs_preauth_cb handsfree_set_state ---");

	device->hs_preauth_id = 0;
	BLUEZ_DEBUG_INFO("[HFP] hs_preauth_cb() End.");
	return FALSE;
}

//接続待受けコールバック
static void ag_confirm(GIOChannel *chan, gpointer data)
{
	struct audio_device *device;
	bdaddr_t src, dst;
	int perr;
	GError *err = NULL;
	uint8_t ch;

	BLUEZ_DEBUG_INFO("[HFP] ag_confirm() Start.");
	EVENT_DRC_BLUEZ_CB(FORMAT_ID_BT_CB, CB_HFP_REGISTER);

	bt_io_get(chan, BT_IO_RFCOMM, &err,
			BT_IO_OPT_SOURCE_BDADDR, &src,
			BT_IO_OPT_DEST_BDADDR, &dst,
			BT_IO_OPT_CHANNEL, &ch,
			BT_IO_OPT_INVALID);
	BLUEZ_DEBUG_LOG("[HF_030_001]--- ag_confirm bt_io_get ---");
	if (err) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] bt_io_get() ERROR [ %s ]", err->message);
		g_error_free(err);
		BLUEZ_DEBUG_LOG("[HF_030_002]--- ag_confirm g_error_free-->goto drop; ---");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_IO_GET, 0, 0);
		goto drop;
	}

	BLUEZ_DEBUG_WARN("[HFP] Confirm from [ %02x:%02x:%02x:%02x:%02x:%02x ]", dst.b[5], dst.b[4], dst.b[3], dst.b[2], dst.b[1], dst.b[0]);
	EVENT_DRC_BLUEZ_CONFIRM(FORMAT_ID_CONFIRM, CONFIRM_HFP, &dst);

	device = manager_get_device(&src, &dst, TRUE);
	BLUEZ_DEBUG_LOG("[HF_030_003]--- ag_confirm device = %p ---", device);
	if (!device)
	{
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] device == NULL --> goto drop");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_GET_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_GET_DEV, __LINE__, &dst);
		goto drop;
	}

	if (!manager_allow_handsfree_connection(device)) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] Refusing handsfree: too many existing connections");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_REFUSING, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_REFUSING, __LINE__, &dst);
		goto drop;
	}

	if( btd_manager_isconnect( MNG_CON_HANDSFREE ) == TRUE ) {
		/* 切断中 */
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] Handsfree is still under connection");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_UNDER_CONNECTION, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_UNDER_CONNECTION, __LINE__, &dst);
		goto drop;
	}

	if (!device->handsfree) {
		btd_device_add_uuid(device->btd_dev, HFP_AG_UUID);
		BLUEZ_DEBUG_LOG("[HF_030_006]--- ag_confirm btd_device_add_uuid ---");
		if (!device->handsfree) {
			BLUEZ_DEBUG_ERROR("[Bluez - HFP] device->handsfree is nothing --> goto drop");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_ADD_UUID, 0, 0);
			EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_ADD_UUID, __LINE__, &dst);
			goto drop;
		}
	}

	if (handsfree_get_state(device) > HANDSFREE_STATE_DISCONNECTED) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] Refusing new connection since one already exists");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_EXISTS, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_EXISTS, __LINE__, &dst);
		goto drop;
	}

	set_handsfree_active(device, TRUE);
	BLUEZ_DEBUG_LOG("[HF_030_008]--- ag_confirm set_handsfree_active ---");

	device->handsfree->rfcomm_ch = ch;

	if (handsfree_connect_rfcomm(device, chan) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] handsfree_connect_rfcomm() failed");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_CONNECT_RFCOMM, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONFIRM_CB_CONNECT_RFCOMM, __LINE__, &dst);
		goto drop;
	}

	handsfree_set_state(device, HANDSFREE_STATE_CONNECTING);
	BLUEZ_DEBUG_LOG("[HF_030_010]--- ag_confirm handsfree_set_state ---");

	if ( !bt_io_accept(chan, handsfree_connect_cb, device, NULL, &err)) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP]  bt_io_accept() ERROR = [ %s ]", err->message);
		g_error_free(err);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_AUTHORIZATION, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_AUTHORIZATION, __LINE__, &dst);
		goto drop;
	}

	//FALSE 相手からの接続 
	connection_initiator = FALSE;
	disconnect_initiator = FALSE;

	device->hs_preauth_id = g_io_add_watch(chan,
					G_IO_NVAL | G_IO_HUP | G_IO_ERR,
					hs_preauth_cb, (void *)device);
	BLUEZ_DEBUG_LOG("[HF_030_013]--- ag_confirm device->hs_preauth_id = %d ---", device->hs_preauth_id);

	BLUEZ_DEBUG_INFO("[HFP] ag_confirm() End.");
	return;

drop:
	g_io_channel_shutdown(chan, TRUE, NULL);
	BLUEZ_DEBUG_LOG("[HFP] ag_confirm g_io_channel_shutdown");
}

static int handsfree_set_channel(struct handsfree *handsfree,
				const sdp_record_t *record, uint16_t svc)
{
	int ch;
	sdp_list_t *protos;

	BLUEZ_DEBUG_INFO("[HFP] handsfree_set_channel() Start.");

	if (sdp_get_access_protos(record, &protos) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] Unable to get access protos from handsfree record");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SET_CHANNEL, 0, 0);
		return -1;
	}

	ch = sdp_get_proto_port(protos, RFCOMM_UUID);
	BLUEZ_DEBUG_LOG("[HF_035_002]--- handsfree_set_channel ch = %d ---", ch);

	sdp_list_foreach(protos, (sdp_list_func_t) sdp_list_free, NULL);
	BLUEZ_DEBUG_LOG("[HF_035_003]--- handsfree_set_channel sdp_list_foreach ---");
	sdp_list_free(protos, NULL);
	BLUEZ_DEBUG_LOG("[HF_035_004]--- handsfree_set_channel sdp_list_free ---");

	if (ch <= 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] Unable to get RFCOMM channel from handsfree record");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SET_CHANNEL, 0, 0);
		return -1;
	}

	handsfree->rfcomm_ch = ch;
	g_hf_rfcommCh = ch;

	handsfree->hfp_handle = record->handle;
	BLUEZ_DEBUG_LOG("[HFP] Discovered Handsfree service on channel %d", ch);
	BLUEZ_DEBUG_LOG("[HF_035_006]--- handsfree_set_channel handsfree->hfp_handle = %d ---", handsfree->hfp_handle);

	BLUEZ_DEBUG_INFO("[HFP] handsfree_set_channel() End.");
	return 0;
}

static void get_record_cb(sdp_list_t *recs, int err, gpointer user_data)
{
	struct audio_device *dev = user_data;
	struct handsfree *hf = dev->handsfree;
	struct pending_connect *p = hf->pending;
	sdp_record_t *record = NULL;
	sdp_list_t *r;
	uuid_t uuid;
	int32_t value = FALSE;
	int rfChannel;
	int linkinfo = 0;
	int network = HF_SDP_DEFAULT_NETWORK;
	int features = HF_SDP_DEFAULT_FEATURES;

	BLUEZ_DEBUG_INFO("[HFP] get_record_cb(). Start.");
	EVENT_DRC_BLUEZ_CB(FORMAT_ID_BT_CB, CB_HFP_GET_RECORD);
	

	if( hf->pending == NULL ){
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] assert hf->pending == NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
		goto failed;
	}

	if (err < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] Unable to get service record: %s (%d)", strerror(-err), -err);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, err, 0);
		goto failed;
	}

	if (!recs || !recs->data) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] No records found");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
		goto failed;
	}

	sdp_uuid16_create(&uuid, p->svclass);
	BLUEZ_DEBUG_LOG("[HF_034_004]--- get_record_cb sdp_uuid16_create; ---");

	for (r = recs; r != NULL; r = r->next) {
		sdp_list_t *classes;
		uuid_t class;

		record = r->data;

		if (sdp_get_service_classes(record, &classes) < 0) {
			BLUEZ_DEBUG_LOG("[HFP] Unable to get service classes from record");
			BLUEZ_DEBUG_LOG("[HF_034_005]--- get_record_cb continue; ---");
			continue;
		}

		if( classes == NULL ){
			BLUEZ_DEBUG_ERROR("[Bluez - HFP] sdp_get_service_classes() is failed");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
			goto failed;
		}

		memcpy(&class, classes->data, sizeof(uuid));

		sdp_list_free(classes, free);
		BLUEZ_DEBUG_LOG("[HF_034_006]--- get_record_cb sdp_list_free; ---");


		if (sdp_uuid_cmp(&class, &uuid) == 0)
		{
			sdp_get_int_attr(record, SDP_ATTR_EXTERNAL_NETWORK, &network);
			sdp_get_int_attr(record, SDP_ATTR_SUPPORTED_FEATURES, &features);
			BLUEZ_DEBUG_LOG("[HF_034_007]--- get_record_cb break; ---");
			break;
		}
	}

	if (r == NULL) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] No record found with UUID 0x%04x", p->svclass);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, p->svclass, 0);
		goto failed;
	}

	if (handsfree_set_channel(hf, record, p->svclass) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] Unable to extract RFCOMM channel from service record");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
		goto failed;
	}

	/* Set svclass to 0 so we can easily check that SDP is no-longer
	 * going on (to know if bt_cancel_discovery2 needs to be called) */
	p->svclass = 0;
	hf->sdp_flg = TRUE;
	hf->features = ( uint16_t )features;
	hf->network = ( uint8_t )network;

	err = get_records_did(dev);
	if (err < 0) {
		err = rfcomm_connect(hf->rfcomm_ch, dev);
		BLUEZ_DEBUG_LOG("[HF_034_010]--- get_record_cb err = %d ---", err);
		if (err < 0) {
			BLUEZ_DEBUG_ERROR("[Bluez - HFP] Unable to connect: %s (%d)", strerror(-err), -err);
			ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, err, 0);
			goto failed;
		}
		btd_adapter_signal_did( FALSE, 0, 0, 0 );
	}
	BLUEZ_DEBUG_INFO("[HFP] get_record_cb(). End.");
	return;

failed:
	linkinfo = btd_manager_find_connect_info_by_bdaddr( &dev->dst );
	if( linkinfo == MNG_ACL_MAX ) {
		if( ECONNABORTED == -err && 0 == btd_manager_get_acl_guard() ) {
			btd_manager_fake_signal_conn_complete( &dev->dst );
			value = 0xFF;
		} else {
			if( btd_manager_get_disconnect_info( &dev->dst, MNG_CON_HANDSFREE ) == TRUE ) {
				value = FALSE;
			} else {
				value = 0xFF;
			}
		}
	}
	if( p != NULL ) {
		p->svclass = 0;
	}
	pending_connect_finalize(dev);
	BLUEZ_DEBUG_LOG("[HF_034_015]--- get_record_cb pending_connect_finalize ---");

	handsfree_close_rfcomm(dev);

	send_signal_connectcfm( value, HF_SDP_DEFAULT_FEATURES, HF_SDP_DEFAULT_NETWORK, 0, &dev->dst );

	handsfree_set_state(dev, HANDSFREE_STATE_DISCONNECTED);

	BLUEZ_DEBUG_LOG("[HFP] get_record_cb handsfree_set_state");
}

static int get_records(struct audio_device *device)
{
	struct handsfree *hf = device->handsfree;
	uint16_t svclass;
	uuid_t uuid;
	int err;

	BLUEZ_DEBUG_INFO("[HFP] get_records() Start.");

	svclass = HANDSFREE_AGW_SVCLASS_ID;

	sdp_uuid16_create(&uuid, svclass);
	BLUEZ_DEBUG_LOG("[HF_033_003]--- get_records sdp_uuid16_create ---");

	BLUEZ_DEBUG_LOG("[HFP] svclass = %x", svclass);

	err = bt_search_service2(&device->src, &device->dst, &uuid,
						get_record_cb, device, NULL);
	BLUEZ_DEBUG_LOG("[HF_033_004]--- get_records err = %d ---", err);

	if (err < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] bt_search_service2() err = %d.", err);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORDS, err, 0);
		return err;
	}

	BLUEZ_DEBUG_LOG("[HFP] hf = %p. hf->pending = %p.", hf, hf->pending);
	if (hf->pending) {
		hf->pending->svclass = svclass;
		BLUEZ_DEBUG_LOG("[HFP] hf->pending->svclass = %x.", (int)hf->pending->svclass);
		BLUEZ_DEBUG_LOG("[HF_033_006]--- get_records return 0 ---");
		return 0;
	}

	handsfree_set_state(device, HANDSFREE_STATE_CONNECTING);
	BLUEZ_DEBUG_LOG("[HF_033_007]--- get_records handsfree_set_state ---");

	pending_connect_init(hf, HANDSFREE_STATE_CONNECTED);
	BLUEZ_DEBUG_LOG("[HF_033_008]--- get_records pending_connect_init ---");

	if (hf->pending) {
		hf->pending->svclass = svclass;
	}

	return 0;
}

static void get_records_did_cb(sdp_list_t *recs, int err, gpointer user_data)
{
	struct audio_device *dev = user_data;
	struct handsfree *hf = dev->handsfree;
	sdp_record_t *record = NULL;
	sdp_list_t *r;
	uuid_t uuid;
	int32_t value = FALSE;
	int result = FALSE;
	int linkinfo = 0;
	uint16_t VendorID = 0;
	uint16_t ProductID = 0;
	uint16_t Version = 0;
	sdp_list_t *classes;
	uuid_t class;
	sdp_data_t *pdlist;

	BLUEZ_DEBUG_INFO("[HFP] Start.");

	if (err < 0) {
		BLUEZ_DEBUG_WARN("[HFP] DID - Unable to get service record: %s (%d)", strerror(-err), -err);
		goto done;
	}

	if (!recs || !recs->data) {
		BLUEZ_DEBUG_WARN("[HFP] DID - No records found");
		goto done;
	}

	sdp_uuid16_create(&uuid, hf->svclass_did);

	for (r = recs; r != NULL; r = r->next) {

		record = r->data;

		if (sdp_get_service_classes(record, &classes) < 0) {
			BLUEZ_DEBUG_LOG("[HFP] DID -  Unable to get service classes from record");
			continue;
		}

		if( classes == NULL ){
			BLUEZ_DEBUG_WARN("[HFP] DID - sdp_get_service_classes is failed");
			goto done;
		}

		memcpy(&class, classes->data, sizeof(uuid));

		sdp_list_free(classes, free);

		if (sdp_uuid_cmp(&class, &uuid) == 0) {
			pdlist = sdp_data_get(record, SDP_ATTR_VENDOR_ID);
			VendorID = ( uint16_t )( pdlist ? pdlist->val.uint16 : 0x0000 );

			pdlist = sdp_data_get(record, SDP_ATTR_PRODUCT_ID);
			ProductID = ( uint16_t )( pdlist ? pdlist->val.uint16 : 0x0000 );

			pdlist = sdp_data_get(record, SDP_ATTR_VERSION);
			Version = ( uint16_t )( pdlist ? pdlist->val.uint16 : 0x0000 );
			break;
		}
	}

	if (r == NULL) {
		BLUEZ_DEBUG_WARN("[HFP] DID - No record found with UUID 0x%04x", PNP_INFO_SVCLASS_ID);
		goto done;
	}

	result = TRUE;

done:

	hf->svclass_did = 0;

	err = rfcomm_connect(hf->rfcomm_ch, dev);
	if (err < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] Unable to connect: %s (%d)", strerror(-err), -err);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, err, 0);
		goto failed;
	}

	btd_adapter_signal_did( result, VendorID, ProductID, Version );

	BLUEZ_DEBUG_INFO("[HFP] End.");
	return;

failed:
	linkinfo = btd_manager_find_connect_info_by_bdaddr( &dev->dst );
	if( linkinfo == MNG_ACL_MAX ) {
		value = 0xFF;
	}

	handsfree_close_rfcomm(dev);

	send_signal_connectcfm( value, HF_SDP_DEFAULT_FEATURES, HF_SDP_DEFAULT_NETWORK, 0, &dev->dst );

	handsfree_set_state(dev, HANDSFREE_STATE_DISCONNECTED);

	BLUEZ_DEBUG_INFO("[HFP] Error End.");
	return;
}

static int get_records_did(struct audio_device *device)
{
	struct handsfree *hf = device->handsfree;
	uint16_t svclass;
	uuid_t uuid;
	int err;

	BLUEZ_DEBUG_INFO("[HFP] Start.");

	svclass = PNP_INFO_SVCLASS_ID;
	sdp_uuid16_create(&uuid, svclass);

	err = bt_search_service2(&device->src, &device->dst, &uuid,
						get_records_did_cb, device, NULL);

	if (err < 0) {
		BLUEZ_DEBUG_WARN("[HFP] DID - bt_search_service2() err = %d.", err);
		return err;
	}
	
	hf->svclass_did = svclass;

	BLUEZ_DEBUG_INFO("[HFP] End.");

	return 0;
}

/* RFCOMMの接続 */
int rfcomm_connect(int rfChannel, struct audio_device *dev)
{
	struct handsfree *hf = dev->handsfree;
	GError *err = NULL;
	char address[18];
	char dstAddress[18];

	BLUEZ_DEBUG_INFO("[HFP] rfcomm_connect() Start.");

	ba2str(&dev->src, address);
	ba2str(&dev->dst, dstAddress);

	BLUEZ_DEBUG_LOG("[HFP] rfcomm_connect()  dev->dst= [%s]. dev->src = [%s].",dstAddress,address);

	if (rfChannel < 0){
		BLUEZ_DEBUG_LOG("[HFP] rfcomm_connect()  rfChannel = %d.", rfChannel);
		BLUEZ_DEBUG_LOG("[HF_004_002]--- rfcomm_connect get_records ---");
		return get_records(dev);
	}

	/*接続開始*/
	hf->tmp_rfcomm = bt_io_connect(BT_IO_RFCOMM, handsfree_connect_cb, dev,
					NULL, &err,
					BT_IO_OPT_SOURCE_BDADDR, &dev->src,
					BT_IO_OPT_DEST_BDADDR, &dev->dst,
					BT_IO_OPT_CHANNEL, rfChannel,
					BT_IO_OPT_INVALID);

	BLUEZ_DEBUG_LOG("[HF_004_003]--- rfcomm_connect hf->tmp_rfcomm = %p ---", hf->tmp_rfcomm);

	if (!hf->tmp_rfcomm) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] bt_io_connect() ERROR = [ %s ]", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_IO_CONNECT, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_IO_CONNECT, __LINE__, &dev->dst);
		g_error_free(err);
		return -EIO;
	}

	hf->hfp_active = hf->hfp_handle != 0 ? TRUE : FALSE;

	handsfree_set_state(dev, HANDSFREE_STATE_CONNECTING);
	BLUEZ_DEBUG_LOG("[HF_004_005]--- rfcomm_connect handsfree_set_state END ---");
	pending_connect_init(hf, HANDSFREE_STATE_CONNECTED);
	BLUEZ_DEBUG_LOG("[HF_004_006]--- rfcomm_connect pending_connect_init END ---");

	BLUEZ_DEBUG_INFO("[HFP] rfcomm_connect() End.");
	return 0;
}

//ソケット生成・接続
static DBusMessage *connect_hf(DBusConnection *conn, DBusMessage *msg, void *data)
{
	const char *clientAddr;
	int rfChannel;
	int status;
	uint32_t len;
	bdaddr_t dst;
	struct audio_device *device = NULL;
	struct handsfree *hf = NULL;
	int32_t value = TRUE;
	int linkinfo = 0;

	BLUEZ_DEBUG_INFO("[HFP] connect_hf() Start.");
	BLUEZ_DEBUG_LOG("[HF_002_001]--- connect_hf Call---");

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_INT32, &rfChannel,
						DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
						&clientAddr, &len,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_WARN("[HFP] connect_hf() rfChannel = %d clientAddd = [ %02x:%02x:%02x:%02x:%02x:%02x ]",rfChannel,clientAddr[5],clientAddr[4],clientAddr[3],clientAddr[2],clientAddr[1],clientAddr[0]);
	BLUEZ_DEBUG_LOG("[HF_003_002]--- connect_hf rfChannel = %d ---", rfChannel);
	BLUEZ_DEBUG_LOG("[HF_003_003]--- connect_hf clientAddd = %02x%02x%02x%02x%02x%02x ---", clientAddr[5],clientAddr[4],clientAddr[3],clientAddr[2],clientAddr[1],clientAddr[0]);
	BLUEZ_DEBUG_LOG("[HF_003_004]--- connect_hf len = %d ---", len);
	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_HFP_CONNECT, clientAddr);

	memcpy( &dst, clientAddr, sizeof(bdaddr_t) );

	device = manager_find_device(NULL, NULL, &dst, AUDIO_HANDSFREE_INTERFACE, FALSE);
	if ( device == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] device is NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_FIND_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_FIND_DEV, __LINE__, &dst);
		goto failed;
	}

	BLUEZ_DEBUG_LOG("[HF_003_005]--- connect_hf device = %p ---", device);

	if( btd_manager_isconnect( MNG_CON_HANDSFREE ) == TRUE ) {
		/* 切断中 */
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] Handsfree is still under connection");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_UNDER_CONNECTION, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_UNDER_CONNECTION, __LINE__, &dst);
		goto failed;
	}

	if (!manager_allow_handsfree_connection(device)) {
		/* 他デバイスに接続中 */
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] Handsfree is connecting to other device");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_REFUSING, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_REFUSING, __LINE__, &dst);
		goto failed;
	}

	if (handsfree_get_state(device) > HANDSFREE_STATE_DISCONNECTED) {
		/* 相手からの接続中 */
		BLUEZ_DEBUG_WARN("[HFP] Handsfree is connecting");
		/* 自分から接続（相手からの接続シーケンスで接続を完了させる） */
		connection_initiator = TRUE;
		disconnect_initiator = FALSE;
		EVENT_DRC_BLUEZ_CHGMSG(FORMAT_ID_CHGMSG, CHANGE_MESSAGE_HFP, &dst);
		return dbus_message_new_method_return(msg);
	}

	hf = device->handsfree;

	status = rfcomm_connect(rfChannel, device);
	BLUEZ_DEBUG_LOG("[HFP] rfcomm_connect() rfChannel = %d",rfChannel);
	BLUEZ_DEBUG_LOG("[HF_003_006]--- connect_hf status = %d ---", status);
	
	if (status < 0)
	{
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] rfcomm_connect() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_RFCOMM_CONNECT, status, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_RFCOMM_CONNECT, __LINE__, &dst);
		goto failed;
	}
	
	// 自分から接続
	connection_initiator = TRUE;
	disconnect_initiator = FALSE;

	BLUEZ_DEBUG_LOG("[HF_003_008]--- connect_hf dbus_message_new_method_return ---");
	BLUEZ_DEBUG_INFO("[HFP] connect_hf() End.");
	return dbus_message_new_method_return(msg);

failed:
	linkinfo = btd_manager_find_connect_info_by_bdaddr( &dst );
	if( linkinfo == MNG_ACL_MAX ) {
		value = 0xFF;
		btd_manager_fake_signal_conn_complete( &dst );
	} else {
		value = FALSE;
	}
	send_signal_connectcfm( value, HF_SDP_DEFAULT_FEATURES, HF_SDP_DEFAULT_NETWORK, 0, &dst );

	return dbus_message_new_method_return(msg);
}

//ソケット切断
static DBusMessage *disconnect_hf(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	const char *clientAddr;
	uint32_t len;
	bdaddr_t dst;
	struct audio_device *device = NULL;

	BLUEZ_DEBUG_INFO("[HFP] disconnect_hf() Start.");
	BLUEZ_DEBUG_LOG("[HF_002_002]--- disconnect_hf Call ---");

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
						&clientAddr, &len,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[HFP] disconnect_hf() clientAddd = [ %02x:%02x:%02x:%02x:%02x:%02x ]",clientAddr[5],clientAddr[4],clientAddr[3],clientAddr[2],clientAddr[1],clientAddr[0]);
	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_HFP_DISCONNECT, clientAddr);
	memcpy( &dst, clientAddr, sizeof(bdaddr_t) );

	device = manager_find_device(NULL, NULL, &dst, AUDIO_HANDSFREE_INTERFACE, FALSE);
	if( device == NULL || handsfree_get_state( device ) <= HANDSFREE_STATE_DISCONNECTED ) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] handsfree already disconnected !!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_FIND_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DISCONNECT_FIND_DEV, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}

	handsfree_shutdown(device);
	BLUEZ_DEBUG_LOG("[HF_005_001]--- disconnect_hf handsfree_shutdown END ---");
	BLUEZ_DEBUG_LOG("[HF_005_002]--- disconnect_hf dbus_message_new_method_retur ---");

	handsfree_set_state(device, HANDSFREE_STATE_DISCONNECTED);

	disconnect_initiator = TRUE;

	/* 既に切断通知をあげているか判定 */
	if( btd_manager_isconnect( MNG_CON_HANDSFREE ) == FALSE ) {
		BLUEZ_DEBUG_LOG("[HFP] handsfree already disconnected");
		btd_manager_handsfree_signal_disconnect_cfm( FALSE );
		disconnect_initiator = FALSE;
	}

	BLUEZ_DEBUG_INFO("[HFP] disconnect_hf() End.");
	return dbus_message_new_method_return(msg);
}

void handsfree_shutdown(struct audio_device *dev)
{
	struct handsfree *hf = dev->handsfree;
	struct pending_connect *p = hf->pending;

	BLUEZ_DEBUG_INFO("[HFP] handsfree_shutdown() Start.");

	pending_connect_finalize(dev);
	BLUEZ_DEBUG_LOG("[HF_006_002]--- handsfree_shutdown pending_connect_finalize END ---");
	BLUEZ_DEBUG_INFO("[HFP] handsfree_shutdown() End.");
}

//ソケット送信
static DBusMessage *datasend_hf(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	struct audio_device *device;
	struct handsfree *hf;
	const char *senddata;
	int status;
	uint32_t len;
	const char *clientAddr;
	uint32_t addrlen;
	bdaddr_t dst;

	BLUEZ_DEBUG_INFO("[HFP] datasend_hf() Start.");
	BLUEZ_DEBUG_LOG("[HF_002_003]--- datasend_hf Call ---");

	if (!dbus_message_get_args(msg, NULL,
							   DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
							   &senddata, &len,
							   DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
							   &clientAddr, &addrlen,
							   DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[HFP] datasend_hf() clientAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", clientAddr[5], clientAddr[4], clientAddr[3], clientAddr[2], clientAddr[1], clientAddr[0]);

	memcpy( &dst, clientAddr, sizeof(bdaddr_t) );
	device = manager_find_device(NULL, NULL, &dst, AUDIO_HANDSFREE_INTERFACE, FALSE);
	if ( device == NULL || device->handsfree == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] device = NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_FIND_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DATASEND_FIND_DEV, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}
	hf = device->handsfree;
	BLUEZ_DEBUG_LOG("[HF_007_003]--- datasend_hf len = %d ---", len);

	status = rfcomm_send(hf, len, senddata);
	BLUEZ_DEBUG_LOG("[HF_007_004]--- datasend_hf status = %d ---", status);
	if( status < 0 ) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] rfcomm_send() Err(%d)", status);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_RFCOMM_SEND, status, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DATASEND_RFCOMM_SEND, __LINE__, &dst);
	}

	BLUEZ_DEBUG_LOG("[HF_007_005]--- datasend_hf dbus_message_new_method_return ---");
	BLUEZ_DEBUG_INFO("[HFP] datasend_hf() End.");
	return dbus_message_new_method_return(msg);
}

int rfcomm_send(struct handsfree *hf, int len, const char *sendData, ...)
{
	BLUEZ_DEBUG_INFO("[HFP] rfcomm_send() Start.");
	va_list ap;
	int ret;

	va_start(ap, sendData);
	BLUEZ_DEBUG_LOG("[HF_008_001]--- rfcomm_send va_start ---");
	ret = handsfree_send_atcmd(hf, len, sendData, ap);
	BLUEZ_DEBUG_LOG("[HF_008_002]--- rfcomm_send ret = %d ---", ret);
	va_end(ap);
	BLUEZ_DEBUG_LOG("[HF_008_003]--- rfcomm_send va_end END ---");

	BLUEZ_DEBUG_INFO("[HFP] rfcomm_send() End.");
	return ret;
}

int rfcomm_send2(struct handsfree *hf, int len, const char *sendData, ...)
{
	BLUEZ_DEBUG_INFO("[HFP] rfcomm_send2() Start.");
	va_list ap;
	int ret;

	va_start(ap, sendData);
	BLUEZ_DEBUG_LOG("[HF_008_001]--- rfcomm_send2 va_start ---");
	ret = handsfree_send_event(hf, len, sendData, ap);
	BLUEZ_DEBUG_LOG("[HF_008_002]--- rfcomm_send2 ret = %d ---", ret);
	va_end(ap);
	BLUEZ_DEBUG_LOG("[HF_008_003]--- rfcomm_send2 va_end END ---");

	BLUEZ_DEBUG_INFO("[HFP] rfcomm_send2() End.");
	return ret;
}

//接続待受を開始
static DBusMessage *register_hf(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	int rfcommCh;
	int rfcommCh2;
	gboolean master = TRUE;
	GIOChannel *io;
	GError *err = NULL;
	GError *err2 = NULL;
	int32_t value = TRUE;
	bdaddr_t src;
	sdp_record_t *record;

	BLUEZ_DEBUG_INFO("[HFP] register_hf() Start.");
	BLUEZ_DEBUG_LOG("[HF_002_004]--- register_hf Call ---");

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_HFP_REGISTER, NULL);

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_INT32, &rfcommCh,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[HFP] register_hf() rfcommCh = %x",rfcommCh);
	BLUEZ_DEBUG_LOG("[HF_009_002]--- register_hf rfcommCh = %d ---", rfcommCh);


	if( g_hf_rfcomm_io != NULL ) {
		BLUEZ_DEBUG_LOG("[HFP] already registerd");
		value = TRUE;
		goto hf_registered;
	}

	bluezutil_get_bdaddr( &src );

 	BLUEZ_DEBUG_LOG("[HFP] register_hf() src = [ %02x:%02x:%02x:%02x:%02x:%02x ]",src.b[5],src.b[4],src.b[3],src.b[2],src.b[1],src.b[0]);

	if (!g_hf_rfcomm_io) {
		io = bt_io_listen(BT_IO_RFCOMM, NULL, ag_confirm, NULL, NULL, &err,
					BT_IO_OPT_SOURCE_BDADDR, &src,
					BT_IO_OPT_CHANNEL, rfcommCh,
					BT_IO_OPT_SEC_LEVEL, BT_IO_SEC_MEDIUM,
					BT_IO_OPT_MASTER, master,
					BT_IO_OPT_INVALID);

		if (!io) {
			BLUEZ_DEBUG_ERROR("[Bluez - HFP] bt_io_listen() ERROR = [ %s ]", err->message);
			ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_IO_LISTEN, err->code, 0);
			g_error_free(err);
			value = FALSE;
			goto hf_registered;
		}

		BLUEZ_DEBUG_LOG("[HFP] register_hf rfcommCh = %d io = %p", rfcommCh, io);
		g_hf_rfcomm_io = io;

		if( g_rfcomm_event == NULL ) {
			rfcommCh2 = 30;
			g_rfcomm_event = bt_io_listen_rfcomm2(rfcomm_confirm, &err2,
													BT_IO_OPT_SOURCE_BDADDR, &src,
													BT_IO_OPT_CHANNEL, rfcommCh2,
													BT_IO_OPT_SEC_LEVEL, BT_IO_SEC_MEDIUM,
													BT_IO_OPT_MASTER, master,
													BT_IO_OPT_INVALID);
			if (!g_rfcomm_event) {
				BLUEZ_DEBUG_ERROR("[Bluez - HFP] bt_io_listen_rfcomm2() ERROR = [ %s ]", err2->message);
				ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_IO_LISTEN, err2->code, 0);
				g_io_channel_shutdown(io, TRUE, NULL);
				g_io_channel_unref(io);
				g_error_free(err2);
				value = FALSE;
				goto hf_registered;
			}
		}
		BLUEZ_DEBUG_INFO("[HFP] register_hf() End.");
	}

	if(g_hf_record_id == 0) {
		record = hfp_sdp_record((uint8_t)rfcommCh);
		if (record == NULL) {
			BLUEZ_DEBUG_ERROR("[Bluez - HFP] SDP ERROR");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_SDP_RECORD, 0, 0);
			g_io_channel_unref(g_hf_rfcomm_io);
			g_hf_rfcomm_io = NULL;
			value = FALSE;
			goto hf_registered;
		}

		if (add_record_to_server(&src, record) < 0) {
			BLUEZ_DEBUG_ERROR("[Bluez - HFP] Unable to register HFP HS service record");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_ADD_RECORD, 0, 0);
			sdp_record_free(record);
			g_io_channel_unref(g_hf_rfcomm_io);
			g_hf_rfcomm_io = NULL;
			value = FALSE;
			goto hf_registered;
		}
		g_hf_record_id = record->handle;
		g_hf_sdp_rfcommCh = rfcommCh;
	}

hf_registered:

	g_dbus_emit_signal(g_hf_connection, "/",
					AUDIO_HANDSFREE_INTERFACE,
					"REGISTER_CFM",
					DBUS_TYPE_INT32, &value,			//接続待受開始通知の成功/失敗
					DBUS_TYPE_INT32, &rfcommCh, 		//HFPサーバーチャンネル
					DBUS_TYPE_INVALID);
	BLUEZ_DEBUG_LOG("[HF_009_006]--- register_hf g_dbus_emit_signal ---");

	BLUEZ_DEBUG_LOG("[HF_009_007]--- register_hf dbus_message_new_method_return  ---");

	BLUEZ_DEBUG_INFO("[HFP] register_hf() End.");

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HFP_REGISTER_CFM, value);

	return dbus_message_new_method_return(msg);
}

//接続待受を停止
static DBusMessage *deregister_hf(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	int sock;
	int32_t value = TRUE;

	BLUEZ_DEBUG_INFO("[HFP] deregister_hf() Start.");
	BLUEZ_DEBUG_LOG("[HF_002_005]--- register_hf Call ---");

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_HFP_DEREGISTER, NULL);

	g_dbus_emit_signal(g_hf_connection, "/",
					AUDIO_HANDSFREE_INTERFACE,
					"DEREGISTER_CFM",
					DBUS_TYPE_INT32, &value,			//接続待受停止通知の成功/失敗
					DBUS_TYPE_INVALID);
	BLUEZ_DEBUG_LOG("[HF_010_002]--- deregister_hf g_dbus_emit_signal ---");

	BLUEZ_DEBUG_LOG("[HF_010_003]--- deregister_hf dbus_message_new_method_return ---");
	BLUEZ_DEBUG_INFO("[HFP] deregister_hf() End.");

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HFP_DEREGISTER_CFM, value);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *connect_sco(DBusConnection *conn, DBusMessage *msg, void *data)
{
	const char *clientAddr;
	struct handsfree *hf;
	uint32_t len;
	bdaddr_t dst;
	GIOChannel *io;
	GError *err = NULL;

	BLUEZ_DEBUG_INFO("[HFP] connect_sco() Start.");
	BLUEZ_DEBUG_LOG("[HF_002_001]--- connect_sco Call---");

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
						&clientAddr, &len,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_SCO, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[HFP] connect_sco() clientAddd = [ %02x:%02x:%02x:%02x:%02x:%02x ]",clientAddr[5],clientAddr[4],clientAddr[3],clientAddr[2],clientAddr[1],clientAddr[0]);
	BLUEZ_DEBUG_LOG("[HF_003_003]--- connect_sco clientAddd = %02x%02x%02x%02x%02x%02x ---", clientAddr[5],clientAddr[4],clientAddr[3],clientAddr[2],clientAddr[1],clientAddr[0]);
	BLUEZ_DEBUG_LOG("[HF_003_004]--- connect_sco len = %d ---", len);

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_SCO_CONNECT, clientAddr);

	memcpy( &dst, clientAddr, sizeof(bdaddr_t) );

	struct audio_device *dev = manager_find_device(NULL, NULL, &dst, AUDIO_HANDSFREE_INTERFACE, FALSE);
	if ( dev == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP]  device = NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_SCO, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_SCO, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}
	
	hf = dev->handsfree;
	
	BLUEZ_DEBUG_LOG("[HF_003_005]--- connect_sco device  = %p ---", dev);
	
	if (hf->state != HANDSFREE_STATE_CONNECTED)
	{
		BLUEZ_DEBUG_LOG("[Bluez - HFP] connect_sco() hf->state != HANDSFREE_STATE_CONNECTED ---");
		return dbus_message_new_method_return(msg);
	}
	
	io = bt_io_connect(BT_IO_SCO, sco_connect_cb, dev, NULL, &err,
				BT_IO_OPT_SOURCE_BDADDR, &dev->src,
				BT_IO_OPT_DEST_BDADDR, &dev->dst,
				BT_IO_OPT_INVALID);
	if (!io) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] bt_io_connect() ERROR = [ %s ]", err->message);
		g_error_free(err);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_SCO, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_SCO, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}

	hf->sco = io;

	handsfree_set_state(dev, HANDSFREE_STATE_PLAY_IN_PROGRESS);

	pending_connect_init(hf, HANDSFREE_STATE_PLAYING);

	BLUEZ_DEBUG_INFO("[HFP] connect_sco() End.");
	return dbus_message_new_method_return(msg);
}

static DBusMessage *connect_esco(DBusConnection *conn, DBusMessage *msg, void *data)
{
	const char *clientAddr;
	struct handsfree *hf;
	uint32_t len;
	bdaddr_t dst;
	GIOChannel *io;
	GError *err = NULL;

	BLUEZ_DEBUG_INFO("[HFP] connect_esco() Start.");
	BLUEZ_DEBUG_LOG("[HF_002_001]--- connect_esco Call---");

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
						&clientAddr, &len,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_SCO, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[HFP] connect_esco() clientAddd = [ %02x:%02x:%02x:%02x:%02x:%02x ]",clientAddr[5],clientAddr[4],clientAddr[3],clientAddr[2],clientAddr[1],clientAddr[0]);
	BLUEZ_DEBUG_LOG("[HF_003_003]--- connect_esco clientAddd = %02x%02x%02x%02x%02x%02x ---", clientAddr[5],clientAddr[4],clientAddr[3],clientAddr[2],clientAddr[1],clientAddr[0]);
	BLUEZ_DEBUG_LOG("[HF_003_004]--- connect_esco len = %d ---", len);

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_SCO_CONNECT, clientAddr);

	memcpy( &dst, clientAddr, sizeof(bdaddr_t) );

	struct audio_device *dev = manager_find_device(NULL, NULL, &dst, AUDIO_HANDSFREE_INTERFACE, FALSE);
	if ( dev == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] device = NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_SCO, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_SCO, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}
	
	hf = dev->handsfree;
	
	BLUEZ_DEBUG_LOG("[HF_003_005]--- connect_esco device  = %p ---", dev);
	
	if (hf->state != HANDSFREE_STATE_CONNECTED)
	{
		BLUEZ_DEBUG_LOG("[Bluez - HFP] connect_esco() hf->state != HANDSFREE_STATE_CONNECTED ---");
		return dbus_message_new_method_return(msg);
	}
	
	io = bt_io_connect(BT_IO_ESCO, sco_connect_cb, dev, NULL, &err,
				BT_IO_OPT_SOURCE_BDADDR, &dev->src,
				BT_IO_OPT_DEST_BDADDR, &dev->dst,
				BT_IO_OPT_INVALID);
	if (!io) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] bt_io_connect() ERROR = [ %s ]", err->message);
		g_error_free(err);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_SCO, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_SCO, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}

	hf->sco = io;

	handsfree_set_state(dev, HANDSFREE_STATE_PLAY_IN_PROGRESS);

	pending_connect_init(hf, HANDSFREE_STATE_PLAYING);

	BLUEZ_DEBUG_INFO("[HFP] connect_esco() End.");
	return dbus_message_new_method_return(msg);
}

static DBusMessage *disconnect_sco(DBusConnection *conn, DBusMessage *msg, void *data)
{
	const char *clientAddr;
	struct handsfree *hf;
	uint32_t len;
	bdaddr_t dst;
	int sock;
	struct audio_device *dev = NULL;

	BLUEZ_DEBUG_INFO("[HFP] disconnect_sco() Start.");

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
						&clientAddr, &len,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_SCO, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[HFP] disconnect_sco() clientAddd = [ %02x:%02x:%02x:%02x:%02x:%02x ]",clientAddr[5],clientAddr[4],clientAddr[3],clientAddr[2],clientAddr[1],clientAddr[0]);

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_SCO_DISCONNECT, clientAddr);

	memcpy( &dst, clientAddr, sizeof(bdaddr_t) );

	dev = manager_find_device(NULL, NULL, &dst, AUDIO_HANDSFREE_INTERFACE, FALSE);
	if( dev == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] handsfree already disconnected !!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_SCO, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DISCONNECT_SCO, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}

	hf = dev->handsfree;

	if (hf->state == HANDSFREE_STATE_DISCONNECTED ||
				hf->state == HANDSFREE_STATE_CONNECTING)
	{
		return dbus_message_new_method_return(msg);
	}

	if (hf->sco) {
		sock = g_io_channel_unix_get_fd(hf->sco);
		/* shutdown but leave the socket open and wait for hup */
		shutdown(sock, SHUT_RDWR);
	} else {
		handsfree_set_state(dev, HANDSFREE_STATE_CONNECTED);
	}

	BLUEZ_DEBUG_INFO("[HFP] disconnect_sco() End.");
	return dbus_message_new_method_return(msg);
}

//ソケット送信
static DBusMessage *eventsend_hf(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	const char *senddata;
	int status;
	uint32_t len;
	const char *clientAddr;
	uint32_t addrlen;

	BLUEZ_DEBUG_INFO("[HFP] eventsend_hf() Start.");
	BLUEZ_DEBUG_LOG("[HF_002_003]--- eventsend_hf Call ---");

	if (!dbus_message_get_args(msg, NULL,
							   DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
							   &senddata, &len,
							   DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
							   &clientAddr, &addrlen,
							   DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[HFP] eventsend_hf() clientAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", clientAddr[5], clientAddr[4], clientAddr[3], clientAddr[2], clientAddr[1], clientAddr[0]);
	BLUEZ_DEBUG_LOG("[HF_007_003]--- eventsend_hf len = %d ---", len);

	status = rfcomm_send2(NULL, len, senddata);
	BLUEZ_DEBUG_LOG("[HF_007_004]--- eventsend_hf status = %d ---", status);
	if( status < 0 ) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP]  rfcomm_send2() Err(%d)", status);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_RFCOMM_SEND, status, 0);
	}

	BLUEZ_DEBUG_LOG("[HF_007_005]--- eventsend_hf dbus_message_new_method_return ---");
	BLUEZ_DEBUG_INFO("[HFP] eventsend_hf() End.");
	return dbus_message_new_method_return(msg);
}

static sdp_record_t *hfp_sdp_record(uint8_t ch)
{
	sdp_list_t *svclass_id, *pfseq, *apseq, *root;
	uuid_t root_uuid, svclass_uuid, ga_svclass_uuid;
	uuid_t l2cap_uuid, rfcomm_uuid;
	sdp_profile_desc_t profile;
	sdp_list_t *aproto, *proto[2];
	sdp_record_t *record;
	sdp_data_t *channel, *features;
	uint8_t netid = 0x01;
	uint16_t sdpfeat;
	sdp_data_t *network;

	record = sdp_record_alloc();
	if (!record){
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] sdp_record_alloc() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SDP_RECORD, 0, 0);
		return NULL;
	}

	network = sdp_data_alloc(SDP_UINT8, &netid);
	if (!network) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] sdp_data_alloc() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SDP_RECORD, 0, 0);
		sdp_record_free(record);
		return NULL;
	}

	sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
	root = sdp_list_append(0, &root_uuid);
	sdp_set_browse_groups(record, root);

	//HANDSFREE_SVCLASS_ID	0x111e
	sdp_uuid16_create(&svclass_uuid, HANDSFREE_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &svclass_uuid);
	//GENERIC_AUDIO_SVCLASS_ID	0x1203
	sdp_uuid16_create(&ga_svclass_uuid, GENERIC_AUDIO_SVCLASS_ID);
	svclass_id = sdp_list_append(svclass_id, &ga_svclass_uuid);
	sdp_set_service_classes(record, svclass_id);

	//HANDSFREE_PROFILE_ID		0x111e
	sdp_uuid16_create(&profile.uuid, HANDSFREE_PROFILE_ID);
	profile.version = 0x0106;
	pfseq = sdp_list_append(0, &profile);
	sdp_set_profile_descs(record, pfseq);

	//L2CAP_UUID	0x0100
	sdp_uuid16_create(&l2cap_uuid, L2CAP_UUID);
	proto[0] = sdp_list_append(0, &l2cap_uuid);
	apseq = sdp_list_append(0, proto[0]);

	//RFCOMM_UUID	0x0003
	sdp_uuid16_create(&rfcomm_uuid, RFCOMM_UUID);
	proto[1] = sdp_list_append(0, &rfcomm_uuid);
	//ch	1
	channel = sdp_data_alloc(SDP_UINT8, &ch);
	proto[1] = sdp_list_append(proto[1], channel);
	apseq = sdp_list_append(apseq, proto[1]);

	//SDP_ATTR_SUPPORTED_FEATURES		0x0311
	// 1 Wide-band speech: Supported
	// 0 Remote volume control: NOT Supported
	// 1 Voice recognition capability: Supported
	// 1 CLI presentation capability: Supported
	// 1 Call Waiting and three way calling: Supported
	// 1 EC and/or NR function: Supported
	// 101111 = 0x2F
	sdpfeat = (uint16_t) 0x002F;
	features = sdp_data_alloc(SDP_UINT16, &sdpfeat);
	sdp_attr_add(record, SDP_ATTR_SUPPORTED_FEATURES, features);

	aproto = sdp_list_append(0, apseq);
	sdp_set_access_protos(record, aproto);

	sdp_set_info_attr(record, "Hands-Free unit", 0, 0);

	sdp_data_free(channel);
	sdp_data_free(network);
	sdp_list_free(proto[0], 0);
	sdp_list_free(proto[1], 0);
	sdp_list_free(apseq, 0);
	sdp_list_free(pfseq, 0);
	sdp_list_free(aproto, 0);
	sdp_list_free(root, 0);
	sdp_list_free(svclass_id, 0);

	return record;
}
static void get_records_after_connect_cb(sdp_list_t *recs, int err, gpointer user_data)
{
	struct audio_device *dev = user_data;
	struct handsfree *hf = dev->handsfree;
	struct pending_connect *p = hf->pending;
	sdp_record_t *record = NULL;
	sdp_list_t *r;
	uuid_t uuid;
	int network = HF_SDP_DEFAULT_NETWORK;
	int features = HF_SDP_DEFAULT_FEATURES;
	int32_t value = TRUE;

	BLUEZ_DEBUG_INFO("[HFP] get_records_after_connect_cb(). Start.");

	if (hf->sdp_timer) {
		g_source_remove(hf->sdp_timer);
		hf->sdp_timer = 0;
	}

	if( hf->pending == NULL ){
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] assert hf->pending == NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORDS_AFTER_CONNECT_CB, 0, 0);
		goto failed;
	}

	if (err < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] Unable to get service record: %s (%d)",strerror(-err), -err);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORDS_AFTER_CONNECT_CB, err, 0);
		goto failed;
	}

	if (!recs || !recs->data) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] No records found");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORDS_AFTER_CONNECT_CB, 0, 0);
		goto failed;
	}

	sdp_uuid16_create(&uuid, p->svclass);

	for (r = recs; r != NULL; r = r->next) {
		sdp_list_t *classes;
		uuid_t class;

		record = r->data;

		if (sdp_get_service_classes(record, &classes) < 0) {
			BLUEZ_DEBUG_LOG("[HFP] Unable to get service classes from record");
			continue;
		}

		if( classes == NULL ){
			BLUEZ_DEBUG_ERROR("[Bluez - HFP] sdp_get_service_classes() is failed");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
			goto failed;
		}

		memcpy(&class, classes->data, sizeof(uuid));

		sdp_list_free(classes, free);

		if (sdp_uuid_cmp(&class, &uuid) == 0)
			sdp_get_int_attr(record, SDP_ATTR_EXTERNAL_NETWORK, &network);
			sdp_get_int_attr(record, SDP_ATTR_SUPPORTED_FEATURES, &features);
			break;
	}

	if (r == NULL) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] No record found with UUID 0x%04x", p->svclass);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORDS_AFTER_CONNECT_CB, p->svclass, 0);
		goto failed;
	}

	/* Set svclass to 0 so we can easily check that SDP is no-longer
	 * going on (to know if bt_cancel_discovery2 needs to be called) */
	p->svclass = 0;

	//相手接続後のSDP完了
	hf->sdp_flg = TRUE;
	hf->features = ( uint16_t )features;
	hf->network = ( uint8_t )network;

	if( TRUE == connection_initiator ) {
		send_signal_connectcfm( value, ( uint16_t )features, ( uint8_t )network, ( uint8_t )hf->rfcomm_ch, &dev->dst );
	} else {
		send_signal_connected( value, ( uint16_t )features, ( uint8_t )network, ( uint8_t )hf->rfcomm_ch, &dev->dst );
	}

	pending_connect_finalize(dev);

	handsfree_set_state(dev, HANDSFREE_STATE_CONNECTED);

	btd_manager_connect_profile( &dev->dst, MNG_CON_HANDSFREE, dev->handsfree->rfcomm_ch, 0 );

	BLUEZ_DEBUG_INFO("[HFP] get_records_after_connect_cb(). End.");

	return;

failed:
	if( p != NULL ){
		p->svclass = 0;
	}
	pending_connect_finalize(dev);
	handsfree_set_state(dev, HANDSFREE_STATE_DISCONNECTED);
}

static gboolean sdp_timer_cb(gpointer data)
{
	struct audio_device *device = data;
	struct handsfree *hf = device->handsfree;
	int32_t value = TRUE;

	BLUEZ_DEBUG_INFO("[HFP] sdp_timer_cb(). Start.");

	g_source_remove(hf->sdp_timer);
	hf->sdp_timer = 0;

	if( handsfree_get_state(device) != HANDSFREE_STATE_CONNECT_AFTER_SDP ) {
		BLUEZ_DEBUG_LOG("[HFP] alredy connected or disconnected");
		return TRUE;
	}

	pending_connect_finalize(device);

	BLUEZ_DEBUG_WARN("[HFP] SDP Time Out Connected by Default Infomation");
	ERR_DRC_BLUEZ(ERROR_BLUEZ_SDP_TIMEOUT, 0, 0);

	//相手接続後のSDPタイムアウト
	if( TRUE == connection_initiator ) {
		send_signal_connectcfm( value, HF_SDP_DEFAULT_FEATURES, HF_SDP_DEFAULT_NETWORK, ( uint8_t )hf->rfcomm_ch, &device->dst );
	} else {
		send_signal_connected( value, HF_SDP_DEFAULT_FEATURES, HF_SDP_DEFAULT_NETWORK, ( uint8_t )hf->rfcomm_ch, &device->dst );
	}
	hf->sdp_flg = TRUE;
	hf->features = ( uint16_t )HF_SDP_DEFAULT_NETWORK;
	hf->network = ( uint8_t )HF_SDP_DEFAULT_NETWORK;

	btd_manager_connect_profile( &device->dst, MNG_CON_HANDSFREE, (uint8_t)hf->rfcomm_ch, 0 );
	handsfree_set_state(device, HANDSFREE_STATE_CONNECTED);

	BLUEZ_DEBUG_INFO("[HFP] sdp_timer_cb(). End.");

	return TRUE;
}

static gboolean sdp_timer_did_cb(gpointer data)
{
	struct audio_device *device = data;
	struct handsfree *hf = device->handsfree;

	BLUEZ_DEBUG_INFO("[HFP] Start.");

	g_source_remove(hf->sdp_timer);
	hf->sdp_timer = 0;

	if( handsfree_get_state(device) != HANDSFREE_STATE_CONNECT_AFTER_SDP ) {
		BLUEZ_DEBUG_LOG("[HFP] alredy connected or disconnected");
		return TRUE;
	}

	BLUEZ_DEBUG_WARN("[HFP] DID - SDP Time Out Connected by Default Infomation");
	btd_adapter_signal_did( FALSE, 0, 0, 0 );

	get_records_after_connect(device);

	BLUEZ_DEBUG_INFO("[HFP] End.");

	return TRUE;
}

static int get_records_after_connect(struct audio_device *device)
{
	struct handsfree *hf = device->handsfree;
	uuid_t uuid;
	int err;

	BLUEZ_DEBUG_INFO("[HFP] get_records_after_connect() Start.");

	BLUEZ_DEBUG_LOG("[HFP] get_records_after_connect(). hf->pending = %p.",hf->pending);

	sdp_uuid16_create(&uuid, HANDSFREE_AGW_SVCLASS_ID);

	err = bt_search_service2(&device->src, &device->dst, &uuid,
						get_records_after_connect_cb, device, NULL);

	if (err < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] bt_search_service2() ERROR = %d.", err);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORDS_AFTER_CONNECT, err, 0);
		return err;
	}

	if ( hf->pending ) {
		BLUEZ_DEBUG_LOG("[HFP] get_records_after_connect(). hf->pending = %p.", hf->pending);
		hf->pending->svclass = HANDSFREE_AGW_SVCLASS_ID;
		BLUEZ_DEBUG_LOG("[HFP] get_records_after_connect(). hf->pending->svclass = %x.", hf->pending->svclass);
		return 0;
	}

	pending_connect_init(hf, HANDSFREE_STATE_CONNECTED);

	if (hf->pending) {
		hf->pending->svclass = HANDSFREE_AGW_SVCLASS_ID;
	}

	hf->sdp_timer = g_timeout_add_seconds(HF_SDP_TIMER, sdp_timer_cb, device);

	BLUEZ_DEBUG_INFO("[HFP] get_records_after_connect() End.");

	return 0;
}

static void get_records_after_connect_did_cb(sdp_list_t *recs, int err, gpointer user_data)
{
	struct audio_device *device = user_data;
	struct handsfree *hf = device->handsfree;
	sdp_record_t *record = NULL;
	sdp_list_t *r;
	uuid_t uuid;
	int result = FALSE;
	uint16_t VendorID = 0;
	uint16_t ProductID = 0;
	uint16_t Version = 0;
	sdp_list_t *classes;
	uuid_t class;
	sdp_data_t *pdlist;

	BLUEZ_DEBUG_INFO("[HFP] Start.");

	g_source_remove(hf->sdp_timer);
	hf->sdp_timer = 0;

	if( handsfree_get_state(device) != HANDSFREE_STATE_CONNECT_AFTER_SDP ) {
		BLUEZ_DEBUG_LOG("[HFP] alredy connected or disconnected");
		return;
	}

	if (err < 0) {
		BLUEZ_DEBUG_WARN("[HFP] DID - Unable to get service record: %s (%d)", strerror(-err), -err);
		goto done;
	}

	if (!recs || !recs->data) {
		BLUEZ_DEBUG_WARN("[HFP] DID - No records found");
		goto done;
	}

	sdp_uuid16_create(&uuid, hf->svclass_did);

	for (r = recs; r != NULL; r = r->next) {

		record = r->data;

		if (sdp_get_service_classes(record, &classes) < 0) {
			BLUEZ_DEBUG_LOG("[HFP] DID -  Unable to get service classes from record");
			continue;
		}

		if( classes == NULL ){
			BLUEZ_DEBUG_WARN("[HFP] DID - sdp_get_service_classes is failed");
			goto done;
		}

		memcpy(&class, classes->data, sizeof(uuid));

		sdp_list_free(classes, free);

		if (sdp_uuid_cmp(&class, &uuid) == 0) {
			pdlist = sdp_data_get(record, SDP_ATTR_VENDOR_ID);
			VendorID = ( uint16_t )( pdlist ? pdlist->val.uint16 : 0x0000 );

			pdlist = sdp_data_get(record, SDP_ATTR_PRODUCT_ID);
			ProductID = ( uint16_t )( pdlist ? pdlist->val.uint16 : 0x0000 );

			pdlist = sdp_data_get(record, SDP_ATTR_VERSION);
			Version = ( uint16_t )( pdlist ? pdlist->val.uint16 : 0x0000 );
			break;
		}
	}

	if (r == NULL) {
		BLUEZ_DEBUG_WARN("[HFP] DID - No record found with UUID 0x%04x", PNP_INFO_SVCLASS_ID);
		goto done;
	}

	result = TRUE;

done:

	hf->svclass_did = 0;
	
	get_records_after_connect(device);

	btd_adapter_signal_did( result, VendorID, ProductID, Version );

	BLUEZ_DEBUG_INFO("[HFP] End.");
	return;
}

static int get_records_after_connect_did(struct audio_device *device)
{
	struct handsfree *hf = device->handsfree;
	uint16_t svclass;
	uuid_t uuid;
	int err;

	BLUEZ_DEBUG_INFO("[HFP] Start.");

	svclass = PNP_INFO_SVCLASS_ID;
	sdp_uuid16_create(&uuid, svclass);

	err = bt_search_service2(&device->src, &device->dst, &uuid,
						get_records_after_connect_did_cb, device, NULL);

	if (err < 0) {
		BLUEZ_DEBUG_WARN("[HFP] DID - bt_search_service2() ERROR = %d.", err);
		return err;
	}

	hf->sdp_timer = g_timeout_add_seconds(HF_SDP_TIMER, sdp_timer_did_cb, device);

	hf->svclass_did = svclass;

	BLUEZ_DEBUG_INFO("[HFP] End.");

	return 0;
}

gboolean get_handsfree_active(struct audio_device *dev)
{
	struct handsfree *hf = dev->handsfree;

	BLUEZ_DEBUG_LOG("[HFP] get_handsfree_active() hfp_active = %d", hf->hfp_active);

	return hf->hfp_active;
}

void set_handsfree_active(struct audio_device *dev, gboolean active)
{
	struct handsfree *hf = dev->handsfree;

	hf->hfp_active = active;
}

static DBusMessage *cancel_ACL_hf(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	const char *clientAddr;
	uint32_t len;
	bdaddr_t dst;
	struct audio_device *device = NULL;
	int32_t value = 0xFF;

	BLUEZ_DEBUG_INFO("[HFP] cancel_ACL_hf() Start.");

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
						&clientAddr, &len,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CANCEL_ACL, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[HFP] cancel_ACL_hf() clientAddd = [ %02x:%02x:%02x:%02x:%02x:%02x ]",clientAddr[5],clientAddr[4],clientAddr[3],clientAddr[2],clientAddr[1],clientAddr[0]);
	memcpy( &dst, clientAddr, sizeof(bdaddr_t) );

	device = manager_find_device(NULL, NULL, &dst, AUDIO_HANDSFREE_INTERFACE, FALSE);
	if( device == NULL || handsfree_get_state( device ) <= HANDSFREE_STATE_DISCONNECTED ) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] already disconnected !!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CANCEL_ACL, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CANCEL_ACL, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}

	handsfree_shutdown(device);

	handsfree_set_state(device, HANDSFREE_STATE_DISCONNECTED);

	send_signal_connectcfm( value, HF_SDP_DEFAULT_FEATURES, HF_SDP_DEFAULT_NETWORK, 0, &dst );

	BLUEZ_DEBUG_INFO("[HFP] cancel_ACL_hf() End.");

	return dbus_message_new_method_return(msg);
}


static GDBusMethodTable handsfree_methods[] = {
	{ "CONNECT",			"iau", 	"", connect_hf,			0,	0	},	//HFの接続開始を行う。
	{ "DISCONNECT",			"ay",	"", disconnect_hf,		0,	0	},	//HFの切断開始を行う。
	{ "DATA_SND",			"ayay",	"", datasend_hf,		0,	0 	},	//HFのデータ送信を行う。
	{ "REGISTER",			"i",	"", register_hf,		0,	0 	},	//HFの接続待受を開始する。
	{ "DEREGISTER",			"", 	"", deregister_hf,		0,	0	},	//HFの接続待受を停止する。
	{ "CONNECT_SCO",		"au",	"", connect_sco,		0,	0	},	//SCOの接続開始を行う。
	{ "CONNECT_ESCO",		"au",	"", connect_esco,		0,	0	},	//ESCOの接続開始を行う。
	{ "DISCONNECT_SCO", 	"au",	"", disconnect_sco,		0,	0	},	//SCOの切断開始を行う。
	{ "EVENT_SND", 			"ayay",	"", eventsend_hf,		0,	0 	},	//イベントの送信を行う。
	{ "CANCEL_ACL_HF",		"ay",	"", cancel_ACL_hf,		0,	0	},	//ACLの接続キャンセルを行う。
	{ NULL, NULL, NULL, NULL, 0, 0 }
};

static GDBusSignalTable handsfree_signals[] = {
	{ "REGISTER_CFM",		"ii",		G_DBUS_SIGNAL_FLAG_DEPRECATED },	//HFの接続待受開始通知
	{ "DEREGISTER_CFM", 	"i",		G_DBUS_SIGNAL_FLAG_DEPRECATED },	//HFの接続待受停止通知
	{ "CONNECT_CFM",		"iqyy",		G_DBUS_SIGNAL_FLAG_DEPRECATED },	//HFの接続完了の通知
	{ "DISCONNECT_CFM", 	"q",		G_DBUS_SIGNAL_FLAG_DEPRECATED },	//HFの切断完了の通知
	{ "CONNECTED",			"iqyyay",	G_DBUS_SIGNAL_FLAG_DEPRECATED },	//リモートデバイスから接続の通知
	{ "DISCONNECTED",		"q",		G_DBUS_SIGNAL_FLAG_DEPRECATED },	//リモートデバイスからの切断の通知
	{ "DATA_RCV",			"qay",		G_DBUS_SIGNAL_FLAG_DEPRECATED },	//HFのデータ受信の通知
	{ NULL, NULL, G_DBUS_SIGNAL_FLAG_DEPRECATED }
};

static void path_unregister(void *data)
{
	BLUEZ_DEBUG_LOG("Unregistered interface %s on path %s",
		AUDIO_HANDSFREE_INTERFACE, "/");
}

void handsfree_unregister(struct audio_device *dev)
{
	
	struct handsfree *hf = dev->handsfree;
	int linkinfo = 0;
	int32_t value = 0xFF;

	BLUEZ_DEBUG_LOG("[HFP] handsfree unregister Start.");

	if (hf->state > HANDSFREE_STATE_DISCONNECTED) {
		BLUEZ_DEBUG_LOG("[HFP] Handsfree unregistered while device was connected!");
		handsfree_shutdown(dev);

		/* 自分から接続 */
		if( connection_initiator == TRUE && hf->state != HANDSFREE_STATE_CONNECTED ) {
			/* 接続が終っておらず、ACL が張られていない場合、接続完了通知（失敗）をあげる必要がある */
			linkinfo = btd_manager_find_connect_info_by_bdaddr( &dev->dst );
			if( linkinfo == MNG_ACL_MAX ) {
				if( btd_manager_get_disconnect_info( &dev->dst, MNG_CON_HANDSFREE ) == TRUE ) {
					value = FALSE;
				} else {
					value = 0xFF;
				}
				BLUEZ_DEBUG_WARN("[HFP] Unregister occurred : connect_cfm return NG(0x%02x)", value);
				send_signal_connectcfm( value, HF_SDP_DEFAULT_FEATURES, HF_SDP_DEFAULT_NETWORK, 0, &dev->dst );
			}
		}
		handsfree_set_state(dev, HANDSFREE_STATE_DISCONNECTED);
	} else {
		close_sco(dev);
		handsfree_close_rfcomm(dev);
		connection_initiator = FALSE;
	}

	g_free(hf);
	dev->handsfree = NULL;
}

struct handsfree  *handsfree_init(struct audio_device *dev)
{
	BLUEZ_DEBUG_INFO("[HFP] handsfree_init() Start.");
	struct handsfree *handsfree;
	EVENT_DRC_BLUEZ_INIT(FORMAT_ID_BT_INIT, INIT_HFP);

	BLUEZ_DEBUG_LOG("[HF_001_001]--- handsfree_init ---");

	handsfree = g_new0(struct handsfree, 1);
	BLUEZ_DEBUG_LOG("[HF_001_002]--- handsfree_init handsfree = %p ---", handsfree);
	handsfree->dev = dev;

	BLUEZ_DEBUG_INFO("[HFP] handsfree_init() End.");

	return handsfree;
}

int handsfree_dbus_init(void)
{
	BLUEZ_DEBUG_INFO("[HFP] handsfree_dbus_init() Start.");
	EVENT_DRC_BLUEZ_INIT(FORMAT_ID_BT_INIT, INIT_DBUS_HFP);

	if (g_hf_connection != NULL  ) {
		g_dbus_unregister_interface(g_hf_connection, "/", AUDIO_HANDSFREE_INTERFACE);
		g_hf_connection = NULL;
	}

	g_hf_connection = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
	if (g_dbus_register_interface(g_hf_connection, "/",
					AUDIO_HANDSFREE_INTERFACE,
					handsfree_methods, handsfree_signals,
					NULL, NULL, path_unregister) == FALSE) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP]  g_dbus_register_interface().ERR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DBUS_INIT, 0, 0);
		return -1;
	}
	btd_manager_set_connection_hfp( g_hf_connection );
	BLUEZ_DEBUG_INFO("[HFP] handsfree_dbus_init() End.");
	return 0;
}

// RFCOMM 切断イベント受信関連
static gboolean datareceive_rfcomm_cb(GIOChannel *chan, GIOCondition cond, void *user_data)
{
	unsigned char buf[BUF_SIZE];
	unsigned char *pbuf = buf;
	ssize_t bytes_read;
	int buf_len;
	int fd;
	int cnt;
	struct rfcomm_event rcvdata;
	char address[18] = { 0 };
	struct audio_device *device = NULL;
	uint8_t rf_chan;
	uint8_t rf_dir;
	uint16_t profile;
	struct audio_device *hfp_dev = NULL;
	int pbap_chan;
	bdaddr_t dst;

	if (cond & G_IO_NVAL)
	{
		BLUEZ_DEBUG_ERROR("[Bluez - RFCOMM] datareceive_rfcomm_cb G_IO_NVAL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_REV_RFCOMM_CB, 0, 0);
		return FALSE;
	}

	BLUEZ_DEBUG_INFO("[RFCOMM] datareceive_rfcomm_cb() Start.");

	if (cond & (G_IO_ERR | G_IO_HUP)) {
		BLUEZ_DEBUG_ERROR("[Bluez - RFCOMM] ERR or HUP on RFCOMM socket");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_REV_RFCOMM_CB, cond, 0);
		return FALSE;
	}

	fd = g_io_channel_unix_get_fd(chan);

	bytes_read = read(fd, buf, sizeof(buf) - 1);

	buf_len = bytes_read;

	BLUEZ_DEBUG_LOG("[RFCOMM] datareceive_rfcomm_cb() buf_len(%d)", buf_len, buf);

	if ( bytes_read < 0 || bytes_read < ( int )sizeof( struct rfcomm_event ) )
	{
		BLUEZ_DEBUG_ERROR("[Bluez - RFCOMM] Socket Data ERROR bytes_read = %d", bytes_read);
		return TRUE;
	}

	memcpy( &rcvdata, buf, sizeof( rcvdata ) );

	ba2str(&rcvdata.bdaddr, address);

	rf_dir = __dir(rcvdata.dlci);
	rf_chan = __srv_channel(rcvdata.dlci);

	hfp_dev = manager_find_device(NULL, NULL, &rcvdata.bdaddr, AUDIO_HANDSFREE_INTERFACE, FALSE);
	btd_manager_get_pbap_rfcommCh( &dst, &pbap_chan );

	BLUEZ_DEBUG_WARN("[RFCOMM] rf_dir = %d chan = 0x%02x evt = 0x%02x bdaddr = [ %s ]", rf_dir, rf_chan, rcvdata.evt, address );

	/* イベント解析 */
	switch( rcvdata.evt ) {
		/* チャンネルの接続応答を受取った */
		case MNG_CONNECT_CFM:
			/* HFP、PBAP 以外は不明として扱う */
			if( rf_chan ==g_hf_rfcommCh && hfp_dev != NULL && memcmp( &rcvdata.bdaddr, &hfp_dev->dst, sizeof( bdaddr_t ) ) == 0 ) {
				BLUEZ_DEBUG_WARN("[RFCOMM] MNG_CONNECT_CFM : Handsfree chan = 0x%02x", rf_chan);
			} else if( rf_chan == pbap_chan && memcmp( &rcvdata.bdaddr, &dst, sizeof( bdaddr_t ) ) == 0 ) {
				BLUEZ_DEBUG_WARN("[RFCOMM] MNG_CONNECT_CFM : Pbap  chan = 0x%02x", rf_chan);
			} else {
				btd_manager_connect_unknown_profile_rfcomm( &rcvdata.bdaddr, rf_chan );
			}
			break;

		/* チャンネルの接続指示を受取った */
		case MNG_CONNECTED:
			/* HFP 以外は不明として扱う（PBAP は相手からの接続は無い） */
			if( rf_chan == 0 ) {
				/* 制御チャンネルは来ない */
				BLUEZ_DEBUG_ERROR("[Bluez - RFCOMM] Control Channel is received chan = 0x%02x", rf_chan);
			} else if( rf_chan != g_hf_sdp_rfcommCh ) {
				btd_manager_connect_unknown_profile_rfcomm( &rcvdata.bdaddr, rf_chan );
			} else {
				/* Handsfree Channel : 1 */
				BLUEZ_DEBUG_WARN("[RFCOMM] MNG_CONNECTED: Handsfree chan = 0x%02x", rf_chan);
			}
			break;

		/* チャンネルの切断応答を受取った */
		case MNG_DISCONNECT_CFM:
			/* HFP or PBAP 判定 */
			profile = btd_manager_judgment_rfcomm( &rcvdata.bdaddr, rf_chan, rf_dir );
			if( profile == MNG_CON_HANDSFREE ) {
				if ( disconnect_initiator != TRUE ) {
					btd_manager_disconnected_profile_rfcomm( &rcvdata.bdaddr, TRUE, rf_chan, MNG_CON_HANDSFREE  );
				} else {
					btd_manager_disconnect_cfm_profile_rfcomm( &rcvdata.bdaddr, TRUE, rf_chan, MNG_CON_HANDSFREE );
				}
				disconnect_initiator = FALSE;
			} else if( profile == MNG_CON_PBAP ) {
				if( pbap_get_disconnect_initiator() != TRUE ) {
					btd_manager_disconnected_profile_rfcomm( &rcvdata.bdaddr, TRUE, rf_chan, MNG_CON_PBAP );
				} else {
					btd_manager_disconnect_cfm_profile_rfcomm( &rcvdata.bdaddr, TRUE, rf_chan, MNG_CON_PBAP );
				}
				pbap_set_disconnect_initiator( FALSE );
			} else if( rf_chan != 0 ) {
				/* 不明なチャンネルの切断 */
				btd_manager_disconnect_unknown_profile_rfcomm( &rcvdata.bdaddr, rf_chan );
			} else {
				BLUEZ_DEBUG_WARN("[RFCOMM] Control Channel disconected chan = 0x%02x", rf_chan);
			}
			break;

		/* チャンネルの切断指示を受取った */
		case MNG_DISCONNECTED:
			/* HFP or PBAP 判定 */
			profile = btd_manager_judgment_rfcomm( &rcvdata.bdaddr, rf_chan, rf_dir );
			if( profile == MNG_CON_HANDSFREE ) {
				btd_manager_disconnected_profile_rfcomm( &rcvdata.bdaddr, TRUE, rf_chan, MNG_CON_HANDSFREE );
				disconnect_initiator = FALSE;
			} else if( profile == MNG_CON_PBAP ) {
				btd_manager_disconnected_profile_rfcomm( &rcvdata.bdaddr, TRUE, rf_chan, MNG_CON_PBAP );
				pbap_set_disconnect_initiator( FALSE );
			} else if( rf_chan != 0 ) {
				/* 不明なチャンネルの切断 */
				btd_manager_disconnect_unknown_profile_rfcomm( &rcvdata.bdaddr, rf_chan );
			} else {
				BLUEZ_DEBUG_WARN("[RFCOMM] Control Channel disconected chan = 0x%02x", rf_chan);
			}
			break;

		default:
			break;
	}

	return TRUE;
}

void rfcomm_accept_cb(GIOChannel *chan, GError *err, gpointer user_data)
{
	BLUEZ_DEBUG_INFO("[HFP]rfcomm_accept_cb() Start.");

	g_io_add_watch(chan, G_IO_IN | G_IO_ERR | G_IO_HUP| G_IO_NVAL,
			(GIOFunc) datareceive_rfcomm_cb, NULL);

	return;
}

static void rfcomm_confirm(GIOChannel *chan, gpointer data)
{
	GError *err = NULL;
	gboolean ret;

	BLUEZ_DEBUG_INFO("[HFP]rfcomm_confirm() Start.");

	g_rfcomm_event_io = g_io_channel_ref(chan);
	
	ret = bt_io_accept(g_rfcomm_event_io, rfcomm_accept_cb, NULL, NULL, &err);
	if ( ret != TRUE ) {
		BLUEZ_DEBUG_ERROR("[Bluez - HFP] bt_io_accept() : %s", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONFIRM_CB_IO_ACCEPT, 0, 0);
		g_error_free(err);
	}
	BLUEZ_DEBUG_INFO("[HFP] rfcomm_confirm() End.");

	return;
}
