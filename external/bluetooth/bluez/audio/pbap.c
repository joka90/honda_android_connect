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
#define BLUEZ_SRC_ID BLUEZ_SRC_ID_AUDIO_PBAP_C

#define BUF_SIZE 2046
static GIOChannel *g_pbap_rfcomm_io = NULL;
static uint32_t g_pbap_record_id = 0;
static DBusConnection *g_pbap_connection = NULL;
static gboolean disconnect_initiator = FALSE; //FALSE 相手からの切断 

static char *str_state[] = {
	"PBAP_STATE_DISCONNECTED",
	"PBAP_STATE_CONNECTING",
	"PBAP_STATE_CONNECTED"
};

struct pending_connect {
	uint16_t svclass;
};
struct pbap {
	struct audio_device *dev;
	int rfcomm_ch;
	uint8_t supp_repo;
	GIOChannel *rfcomm;
	GIOChannel *tmp_rfcomm;
	pbap_state_t state;
	struct pending_connect *pending;
};
static sdp_record_t *pbap_sdp_record(void);

static int rfcomm_connect(int rfChannel, struct audio_device *dev);
static int rfcomm_send(struct pbap *pbap, int len, const char *sendData, ...);
static int get_records(struct audio_device *device);

static int pbap_send_obexpacket(struct pbap *pbap, int len, const char *format)
{
	char rsp[BUF_SIZE];
	ssize_t total_written, count;
	int fd, i;
	
	BLUEZ_DEBUG_INFO("[PBAP] pbap_send_obexpacket() Start.");
	
	memcpy( rsp, format, len );
	count = len;

	BLUEZ_DEBUG_LOG("[PBAP] OBEX Packet Len:%d", (int)count);
	
	if (count <= 0)
	{
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] Packet LEN = 0 -EINVAL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SEND_OBEXPACKET, count, 0);
		return -EINVAL;
	}

	if (!pbap->rfcomm) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] pbap is not connected");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SEND_OBEXPACKET, 0, 0);
		return -EIO;
	}

	total_written = 0;
	fd = g_io_channel_unix_get_fd(pbap->rfcomm);
	BLUEZ_DEBUG_LOG("[PBAP_022_004]--- pbap_send_obexpacket fd = %d ---", fd);

	while (total_written < (count)) {
		ssize_t written;
		written = write(fd, rsp + total_written,
				count - total_written);
		BLUEZ_DEBUG_LOG("[PBAP_022_005]--- pbap_send_obexpacket written = %d ---", (int)written);
		if (written < 0){
			BLUEZ_DEBUG_ERROR("[Bluez - PBAP] Error written < 0");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_SEND_OBEXPACKET, errno, 0);
			return -errno;
		}
		for(i=0; i<(count - total_written);i++)
		{
			BLUEZ_DEBUG_LOG("[PBAP] OBEX Packet:%02x", *(rsp + total_written + i));
		}
		BLUEZ_DEBUG_LOG("[PBAP] written:%d", (int)written);
		total_written += written;
		BLUEZ_DEBUG_LOG("[PBAP] total_written:%d", (int)total_written);
	}
	BLUEZ_DEBUG_INFO("[PBAP] pbap_send_obexpacket() End.");
	return 0;
}

//ソケット受信関数(コールバック)
static gboolean datareceive_pbap_cb(GIOChannel *chan, GIOCondition cond,
								struct audio_device *device)
{
	struct pbap *pbap;
	unsigned char buf[BUF_SIZE];
	unsigned char *pbuf = buf;
	ssize_t bytes_read;
	int buf_len;
	size_t free_space;
	int fd;
	int value = TRUE;

	BLUEZ_DEBUG_INFO("[PBAP] datareceive_pbap_cb() Start.");

	if (cond & G_IO_NVAL)
	{
		BLUEZ_DEBUG_WARN("[PBAP] PBAP NVAL on RFCOMM socket");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_IO, 0, 0);
		return FALSE;
	}

	pbap = device->pbap;

	if (cond & (G_IO_ERR | G_IO_HUP)) {
		BLUEZ_DEBUG_WARN("[ PBAP] ERR or HUP on RFCOMM socket");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_IO, cond, 0);
		goto failed;
	}

	fd = g_io_channel_unix_get_fd(chan);
	BLUEZ_DEBUG_LOG("[PBAP_012_003]--- datareceive_pbap_cb fd = %d ---", fd);


	bytes_read = read(fd, buf, sizeof(buf) - 1);
	BLUEZ_DEBUG_LOG("[PBAP_012_004]--- datareceive_pbap_cb bytes_read = %d ---", (int)bytes_read);

	buf_len = bytes_read;

	BLUEZ_DEBUG_LOG("[PBAP] datareceive_pbap_cb() buf_len(%d) buf:%s ---", buf_len, buf);

	if (bytes_read < 0)
	{
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] bytes_read < 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_READ, bytes_read, 0);
		return TRUE;
	}

	free_space = sizeof(buf) - 1;

	if (free_space < (size_t) bytes_read) {
		/* Very likely that the PBAP is sending us garbage so
		 * just ignore the data and disconnect */
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] Too much data to fit incomming buffer");
		BLUEZ_DEBUG_LOG("[PBAP_012_006]--- datareceive_pbap_cb goto failed; ---");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_RECEIVE_CB_OVER_BUF, free_space, bytes_read);
		goto failed;
	}

	g_dbus_emit_signal(g_pbap_connection, "/",
				OBEX_PBAP_INTERFACE,
				"DATA_RCV",
				DBUS_TYPE_INT32, &value,		//データ受信通知の成功/失敗
				DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
				&pbuf,buf_len,
				DBUS_TYPE_INVALID);
	BLUEZ_DEBUG_LOG("[PBAP_012_011]--- datareceive_pbap_cb g_dbus_emit_signal ---");
		
	BLUEZ_DEBUG_LOG("[PBAP_012_012]--- datareceive_pbap_cb Return TRUE---");
	BLUEZ_DEBUG_INFO("[PBAP] datareceive_pbap_cb() End.");
	return TRUE;

failed:
	pbap_set_state(device, PBAP_STATE_DISCONNECTED);
	return FALSE;
}

static void pending_connect_init(struct pbap *pbap, pbap_state_t target_state)
{
	BLUEZ_DEBUG_INFO("[PBAP] pending_connect_init() Start.");
	pbap->pending = g_new0(struct pending_connect, 1);
	BLUEZ_DEBUG_LOG("[PBAP_014_002]--- pending_connect_init pbap->pending = %p ---", pbap->pending);
	BLUEZ_DEBUG_INFO("[PBAP] pending_connect_init() End.");
}

static void pending_connect_finalize(struct audio_device *dev)
{
	struct pbap *pbap = dev->pbap;
	struct pending_connect *p = pbap->pending;

	BLUEZ_DEBUG_INFO("[PBAP] pending_connect_finalize() Start.");

	if (p == NULL)
	{
		BLUEZ_DEBUG_LOG("[PBAP_015_001]--- pending_connect_finalize p=NULL ---");
		return;
	}

	if (p->svclass)
	{
		bt_cancel_discovery2(&dev->src, &dev->dst, p->svclass);
		BLUEZ_DEBUG_LOG("[PBAP_015_002]--- pending_connect_finalize bt_cancel_discovery2 ---");
	}
	g_free(p);
	BLUEZ_DEBUG_LOG("[PBAP_015_011]--- pending_connect_finalize g_free ---");

	pbap->pending = NULL;
	BLUEZ_DEBUG_INFO("[PBAP] pending_connect_finalize() End.");
}

GIOChannel *pbap_get_rfcomm(struct audio_device *dev)
{
	struct pbap *pbap = dev->pbap;

	BLUEZ_DEBUG_INFO("[PBAP] pbap_get_rfcomm() Start.");

	BLUEZ_DEBUG_LOG("[PBAP_029_001]--- pbap_get_rfcomm pbap->tmp_rfcomm = %p ---", pbap->tmp_rfcomm);
	return pbap->tmp_rfcomm;
}

int pbap_connect_rfcomm(struct audio_device *dev, GIOChannel *io)
{
	struct pbap *pbap = dev->pbap;

	BLUEZ_DEBUG_INFO("[PBAP] pbap_connect_rfcomm() Start.");

	if (pbap->tmp_rfcomm)
	{
		BLUEZ_DEBUG_LOG("[PBAP_032_001]--- pbap_connect_rfcomm -EALREADY ---");
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] pbap->tmp_rfcomm is not NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_RFCOMM_CONNECTED, 0, 0);
		return -EALREADY;
	}

	pbap->tmp_rfcomm = g_io_channel_ref(io);
	BLUEZ_DEBUG_LOG("[PBAP_032_002]--- pbap_connect_rfcomm pbap->tmp_rfcomm = %p ---", pbap->tmp_rfcomm);
	BLUEZ_DEBUG_INFO("[PBAP] pbap_connect_rfcomm() End.");
	return 0;
}

static int pbap_close_rfcomm(struct audio_device *dev)
{
	struct pbap *pbap = dev->pbap;
	GIOChannel *rfcomm = pbap->tmp_rfcomm ? pbap->tmp_rfcomm : pbap->rfcomm;

	BLUEZ_DEBUG_INFO("[PBAP] pbap_close_rfcomm() Start.");

	if (rfcomm) {
		g_io_channel_shutdown(rfcomm, TRUE, NULL);
		BLUEZ_DEBUG_LOG("[PBAP_019_001]--- pbap_close_rfcomm g_io_channel_shutdown ---");
		g_io_channel_unref(rfcomm);
		BLUEZ_DEBUG_LOG("[PBAP_019_002]--- pbap_close_rfcomm g_io_channel_unref ---");
		pbap->tmp_rfcomm = NULL;
		pbap->rfcomm = NULL;
	}

	BLUEZ_DEBUG_INFO("[PBAP] pbap_close_rfcomm() End.");

	return 0;
}

static const char *state2str(pbap_state_t state)
{
	BLUEZ_DEBUG_INFO("[PBAP] state2str() Start.");
	BLUEZ_DEBUG_LOG("[PBAP] state2str() state = %d ---", state);
	switch (state) {
	case PBAP_STATE_DISCONNECTED:
		BLUEZ_DEBUG_LOG("[PBAP_027_001]--- state2str return DISCONNECTED ---");
		return "DISCONNECTED";
	case PBAP_STATE_CONNECTING:
		BLUEZ_DEBUG_LOG("[PBAP_027_002]--- state2str return connecting ---");
		return "CONNECTING";
	case PBAP_STATE_CONNECTED:
		BLUEZ_DEBUG_LOG("[PBAP_027_003]--- state2str return CONNECTED ---");
		return "CONNECTED";
	default:
		break;
	}
	BLUEZ_DEBUG_LOG("[PBAP_027_006]--- state2str return NULL ---");
	BLUEZ_DEBUG_INFO("[PBAP] state2str() NULL End.");

	return NULL;
}
pbap_state_t pbap_get_state(struct audio_device *dev)
{
	BLUEZ_DEBUG_INFO("[PBAP] pbap_get_state() Start.");
	struct pbap *pbap = dev->pbap;

	if( dev->pbap == NULL ){
		return PBAP_STATE_DISCONNECTED;
	}

	BLUEZ_DEBUG_LOG("[PBAP_017_001]--- pbap_get_state pbap->state = %d ---", pbap->state);
	BLUEZ_DEBUG_INFO("[PBAP] pbap_get_state() End.");
	return pbap->state;
}

void pbap_set_state(struct audio_device *dev, pbap_state_t state)
{
	struct pbap *pbap = dev->pbap;
	const char *state_str;
	pbap_state_t old_state = pbap->state;
	bdaddr_t dst;

	BLUEZ_DEBUG_INFO("[PBAP] pbap_set_state() Start.");

	if (old_state == state)
	{
		BLUEZ_DEBUG_LOG("[PBAP_018_001]--- pbap_set_state old_state == state ---");
		return;
	}

	state_str = state2str(state);
	BLUEZ_DEBUG_LOG("[PBAP_018_002]--- pbap_set_state state_str = %s ---", state_str);

	switch (state) {
	case PBAP_STATE_DISCONNECTED:
		BLUEZ_DEBUG_LOG("[PBAP_018_0XX]--- pbap_set_state PBAP_STATE_DISCONNECTED ---");
		pbap_close_rfcomm(dev);
		memset( &dst, 0x00, sizeof( bdaddr_t ) );
		btd_manager_set_pbap_rfcommCh( &dst,0xFF );
		break;
	case PBAP_STATE_CONNECTING:
		BLUEZ_DEBUG_LOG("[PBAP_018_011]--- pbap_set_state PBAP_STATE_CONNECTING ---");
		break;
	case PBAP_STATE_CONNECTED:
		BLUEZ_DEBUG_LOG("[PBAP] pbap_set_state() PBAP_STATE_CONNECTED ---");
		break;
	default:
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] INVALID STATE");;
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SET_STATE, state, 0);
	}

	pbap->state = state;
	EVENT_DRC_BLUEZ_STATE(FORMAT_ID_STATE, STATE_PBAP, &dev->dst, state);
	btd_manager_set_state_info( &dev->dst, MNG_CON_PBAP, state );

	BLUEZ_DEBUG_LOG("State changed %s: %s -> %s", dev->path, str_state[old_state],
		str_state[state]);

	BLUEZ_DEBUG_INFO("[PBAP] pbap_set_state() End.");
}

//コールバック
void pbap_connect_cb(GIOChannel *chan, GError *err, gpointer user_data)
{
	struct audio_device *dev = user_data;
	struct pbap *pbap = dev->pbap;
	struct pending_connect *p = pbap->pending;
	char pbab_address[18];
	int value;

	BLUEZ_DEBUG_INFO("[PBAP] pbap_connect_cb() Start.");
	EVENT_DRC_BLUEZ_CONNECT_CB(FORMAT_ID_CONNECT_CB, CB_PBAP_CONNECT, TRUE);

	if (err) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] ERROR = [ %s ]", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_CB_ERR, err->code, 0);
		goto failed;
	}

	pbap->rfcomm = pbap->tmp_rfcomm;
	pbap->tmp_rfcomm = NULL;

	ba2str(&dev->dst, pbab_address);

	g_io_add_watch(chan, G_IO_IN | G_IO_ERR | G_IO_HUP| G_IO_NVAL,
			(GIOFunc) datareceive_pbap_cb, (void *)dev);
	BLUEZ_DEBUG_LOG("[PBAP_011_004]--- pbap_connect_cb g_io_add_watch ---");

	BLUEZ_DEBUG_LOG("%s: Connected to %s", dev->path, pbab_address);

	//CFMの場合
	value = TRUE;
	g_dbus_emit_signal(g_pbap_connection, "/",
			OBEX_PBAP_INTERFACE,
			"CONNECT_CFM",
			DBUS_TYPE_INT32, &value,			//接続完了通知の成功/失敗
			DBUS_TYPE_UINT32,  &(pbap->supp_repo),
			DBUS_TYPE_INVALID);

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_PBAP_CONNECT_CFM, value);

	BLUEZ_DEBUG_LOG("[PBAP_011_007]--- pbap_connect_cb g_dbus_emit_signal ---");
	pbap_set_state(dev, PBAP_STATE_CONNECTED);
	BLUEZ_DEBUG_LOG("[PBAP_011_008]--- pbap_connect_cb pbap_set_state ---");

	pending_connect_finalize(dev);
	BLUEZ_DEBUG_LOG("[PBAP_011_0011]--- pbap_connect_cb pending_connect_finalize ---");
	BLUEZ_DEBUG_INFO("[PBAP] pbap_connect_cb() End.");

	btd_manager_connect_profile( &dev->dst, MNG_CON_PBAP, dev->pbap->rfcomm_ch, 0 );

	return;

failed:
	BLUEZ_DEBUG_INFO("[PBAP] pbap_connect_cb() failed Start.");
	pending_connect_finalize(dev);
	BLUEZ_DEBUG_LOG("[PBAP_011_0013]--- pbap_connect_cb pending_connect_finalize ---");

	pbap_close_rfcomm(dev);
	value = FALSE;
	g_dbus_emit_signal(g_pbap_connection, "/",
			OBEX_PBAP_INTERFACE,
			"CONNECT_CFM",
			DBUS_TYPE_INT32, &value,			//接続完了通知の成功/失敗
			DBUS_TYPE_UINT32,  &(pbap->supp_repo),
			DBUS_TYPE_INVALID);
	BLUEZ_DEBUG_LOG("[PBAP_011_0014]--- pbap_connect_cb g_dbus_emit_signal ---");
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_PBAP_CONNECT_CFM, value);

	pbap_set_state(dev, PBAP_STATE_DISCONNECTED);
	BLUEZ_DEBUG_LOG("[PBAP_011_0015]--- pbap_connect_cb pbap_set_state ---");

	BLUEZ_DEBUG_INFO("[PBAP] pbap_connect_cb() failed End.");
}

static int pbap_set_channel(struct pbap *pbap,
				const sdp_record_t *record, uint16_t svc)
{
	int ch;
	sdp_list_t *protos;
	BLUEZ_DEBUG_INFO("[PBAP] pbap_set_channel() Start.");

	if (sdp_get_access_protos(record, &protos) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] Unable to get access protos from pbap record");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SET_CHANNEL, 0, 0);
		return -1;
	}

	ch = sdp_get_proto_port(protos, RFCOMM_UUID);
	BLUEZ_DEBUG_LOG("[PBAP_035_002]--- pbap_set_channel ch = %d ---", ch);

	sdp_list_foreach(protos, (sdp_list_func_t) sdp_list_free, NULL);
	BLUEZ_DEBUG_LOG("[PBAP_035_003]--- pbap_set_channel sdp_list_foreach ---");
	sdp_list_free(protos, NULL);
	BLUEZ_DEBUG_LOG("[PBAP_035_004]--- pbap_set_channel sdp_list_free ---");

	if (ch <= 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] Unable to get RFCOMM channel from pbap record");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SET_CHANNEL, 0, 0);
		return -1;
	}

	pbap->rfcomm_ch = ch;
	btd_manager_set_pbap_rfcommCh( &pbap->dev->dst, ch );

	BLUEZ_DEBUG_LOG("[PBAP] Discovered Pbap service on channel %d", ch);

	BLUEZ_DEBUG_INFO("[PBAP] pbap_set_channel() End.");
	return 0;
}

static void get_record_cb(sdp_list_t *recs, int err, gpointer user_data)
{
	struct audio_device *dev = user_data;
	struct pbap *pbap = dev->pbap;
	struct pending_connect *p = pbap->pending;
	sdp_record_t *record = NULL;
	sdp_list_t *r;
	uuid_t uuid;
	int value;
	int rfChannel;
	int status;

	BLUEZ_DEBUG_INFO("[PBAP] get_record_cb(). Start.");
	EVENT_DRC_BLUEZ_CB(FORMAT_ID_BT_CB, CB_PBAP_GET_RECORD);

	if( pbap->pending == NULL ){
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] assert pbap->pending == NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
		goto failed;
	}

	if (err < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] Unable to get service record: %s (%d)", strerror(-err), -err);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, err, 0);
		goto failed;
	}

	if (!recs || !recs->data) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] No records found");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
		goto failed;
	}

	sdp_uuid16_create(&uuid, p->svclass);
	BLUEZ_DEBUG_LOG("[PBAP_034_004]--- get_record_cb sdp_uuid16_create; ---");

	for (r = recs; r != NULL; r = r->next) {
		sdp_list_t *classes;
		uuid_t class;

		record = r->data;

		if (sdp_get_service_classes(record, &classes) < 0) {
			BLUEZ_DEBUG_LOG("[PBAP] Unable to get service classes from record");
			continue;
		}

		if( classes == NULL ){
			BLUEZ_DEBUG_ERROR("[Bluez - PBAP] sdp_get_service_classes() is failed");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
			goto failed;
		}

		memcpy(&class, classes->data, sizeof(uuid));

		sdp_list_free(classes, free);
		BLUEZ_DEBUG_LOG("[PBAP_034_006]--- get_record_cb sdp_list_free; ---");


		if (sdp_uuid_cmp(&class, &uuid) == 0)
		{
			BLUEZ_DEBUG_LOG("[PBAP_034_007]--- get_record_cb break; ---");
			break;
		}
	}

	if (r == NULL) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] No record found with UUID 0x%04x", p->svclass);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, p->svclass, 0);
		goto failed;
	}

	if (pbap_set_channel(pbap, record, p->svclass) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] Unable to extract RFCOMM channel from service record");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, 0, 0);
		goto failed;
	}
	
	pbap->supp_repo = sdp_get_supp_repo(record);
	
	BLUEZ_DEBUG_LOG("[PBAP] sdp_get_supp_repo SUPP_REPO 0x%02x", pbap->supp_repo);

	/* Set svclass to 0 so we can easily check that SDP is no-longer
	 * going on (to know if bt_cancel_discovery2 needs to be called) */
	p->svclass = 0;

	err = rfcomm_connect(pbap->rfcomm_ch, dev);
	BLUEZ_DEBUG_LOG("[PBAP_034_010]--- get_record_cb err = %d ---", err);
	if (err < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] Unable to connect: %s (%d)", strerror(-err), -err);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORD_CB, err, 0);
		goto failed;
	}
	
	BLUEZ_DEBUG_INFO("[PBAP] get_record_cb(). End. ---");
	return;

failed:
	if (p != NULL) {
		p->svclass = 0;
	}
	pending_connect_finalize(dev);
	BLUEZ_DEBUG_LOG("[PBAP_034_015]--- get_record_cb pending_connect_finalize ---");

	pbap_close_rfcomm(dev);

	value = FALSE;
	g_dbus_emit_signal(g_pbap_connection, "/",
			OBEX_PBAP_INTERFACE,
			"CONNECT_CFM",
			DBUS_TYPE_INT32, &value,			//接続完了通知の成功/失敗
			DBUS_TYPE_UINT32,  &(pbap->supp_repo),
			DBUS_TYPE_INVALID);
	
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_PBAP_CONNECT_CFM, value);

	pbap_set_state(dev, PBAP_STATE_DISCONNECTED);

	BLUEZ_DEBUG_LOG("[PBAP_034_016]--- get_record_cb pbap_set_state ---");
}

static int get_records(struct audio_device *device)
{
	struct pbap *pbap = device->pbap;
	uint16_t svclass;
	uuid_t uuid;
	int err;

	BLUEZ_DEBUG_INFO("[PBAP] get_records() Start.");

	svclass = PBAP_PSE_SVCLASS_ID;

	BLUEZ_DEBUG_LOG("[PBAP_033_002]--- get_records svclass = %d ---", svclass);

	sdp_uuid16_create(&uuid, svclass);
	BLUEZ_DEBUG_LOG("[PBAP_033_003]--- get_records sdp_uuid16_create ---");

	BLUEZ_DEBUG_LOG("[PBAP] svclass = %x ---", svclass);

	err = bt_search_service2(&device->src, &device->dst, &uuid,
						get_record_cb, device, NULL);
	BLUEZ_DEBUG_LOG("[PBAP_033_004]--- get_records err = %d ---", err);

	BLUEZ_DEBUG_LOG("[PBAP] get_records(). err = %d.", err);
	if (err < 0)
	{
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] bt_search_service2() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_GET_RECORDS, err, 0);
		return err;
	}

	BLUEZ_DEBUG_LOG("[PBAP] get_records() 2. pbap = %p. ---", pbap);
	BLUEZ_DEBUG_LOG("[PBAP] get_records() 2. pbap->pending = %p. ---", pbap->pending);
	if (pbap->pending) {
		BLUEZ_DEBUG_LOG("[PBAP] get_records(). pbap->pending = %p. ---", pbap->pending);
		pbap->pending->svclass = svclass;
		BLUEZ_DEBUG_LOG("[PBAP] get_records(). pbap->pending->svclass = %x. ---",pbap->pending->svclass);
		BLUEZ_DEBUG_LOG("[PBAP_033_006]--- get_records return 0 ---");
		return 0;
	}

	BLUEZ_DEBUG_LOG("[PBAP] get_records(). 1. ---");
	pbap_set_state(device, PBAP_STATE_CONNECTING);
	BLUEZ_DEBUG_LOG("[PBAP_033_007]--- get_records pbap_set_state ---");

	pending_connect_init(pbap, PBAP_STATE_CONNECTED);
	BLUEZ_DEBUG_LOG("[PBAP_033_008]--- get_records pending_connect_init ---");

	if(pbap->pending) {
		pbap->pending->svclass = svclass;
	}

	return 0;
}

/* RFCOMMの接続 */
static int rfcomm_connect(int rfChannel, struct audio_device *dev)
{
	struct pbap *pbap = dev->pbap;
	GError *err = NULL;
	char address[18];
	char dstAddress[18];

	BLUEZ_DEBUG_INFO("[PBAP] rfcomm_connect() Start.");

	ba2str(&dev->src, address);
	ba2str(&dev->dst, dstAddress);

	BLUEZ_DEBUG_LOG("[PBAP] rfcomm_connect()  dev->dst= [%s]. dev->src = [%s]. ---",dstAddress,address);

	if (!manager_allow_pbap_connection(dev)) {
		/* 接続が拒否された場合 */
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] manager_allow_pbap_connection() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_REFUSING, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_REFUSING, __LINE__, &dev->dst);
		return -ECONNREFUSED;
	}

	if (rfChannel < 0){
		BLUEZ_DEBUG_LOG("[PBAP] rfcomm_connect()  rfChannel = %d. ---",rfChannel);
		BLUEZ_DEBUG_LOG("[PBAP_004_002]--- rfcomm_connect get_records ---");
		return get_records(dev);
	}

	/*接続開始*/
	pbap->tmp_rfcomm = bt_io_connect(BT_IO_RFCOMM, pbap_connect_cb, dev,
					NULL, &err,
					BT_IO_OPT_SOURCE_BDADDR, &dev->src,
					BT_IO_OPT_DEST_BDADDR, &dev->dst,
					BT_IO_OPT_CHANNEL, rfChannel,
					BT_IO_OPT_INVALID);

	BLUEZ_DEBUG_LOG("[PBAP_004_003]--- rfcomm_connect pbap->tmp_rfcomm = %p ---", pbap->tmp_rfcomm);

	if (!pbap->tmp_rfcomm) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] bt_io_connect() ERROR = [ %s ]", err->message);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_IO_CONNECT, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_IO_CONNECT, __LINE__, &dev->dst);
		g_error_free(err);
		return -EIO;
	}

	pbap_set_state(dev, PBAP_STATE_CONNECTING);
	BLUEZ_DEBUG_LOG("[PBAP_004_005]--- rfcomm_connect pbap_set_state End.");
	pending_connect_init(pbap, PBAP_STATE_CONNECTED);
	BLUEZ_DEBUG_LOG("[PBAP_004_006]--- rfcomm_connect pending_connect_init End.");

	BLUEZ_DEBUG_INFO("[PBAP] rfcomm_connect() End.");
	return 0;
}


//ソケット生成・接続
static DBusMessage *connect_pbap(DBusConnection *conn, DBusMessage *msg, void *data)
{
	const char *clientAddr;
	int rfChannel;
	int status;
	uint32_t len;
	bdaddr_t dst;
	struct audio_device *device = NULL;
	uint32_t supp_repo = 0;
	struct pbap *pbap = NULL;
	int value = TRUE;

	BLUEZ_DEBUG_INFO("[PBAP] connect_pbap() Start.");
	BLUEZ_DEBUG_LOG("[PBAP_002_001]--- connect_pbap Call---");

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_INT32, &rfChannel,
						DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
						&clientAddr, &len,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_WARN("[PBAP] connect_pbap() rfChannel = %d clientAddd = [ %02x:%02x:%02x:%02x:%02x:%02x ]",rfChannel,clientAddr[5],clientAddr[4],clientAddr[3],clientAddr[2],clientAddr[1],clientAddr[0]);
	BLUEZ_DEBUG_LOG("[PBAP_003_002]--- connect_pbap rfChannel = %d ---", rfChannel);
	BLUEZ_DEBUG_LOG("[PBAP_003_003]--- connect_pbap clientAddd = %02x%02x%02x%02x%02x%02x ---", clientAddr[5],clientAddr[4],clientAddr[3],clientAddr[2],clientAddr[1],clientAddr[0]);
	BLUEZ_DEBUG_LOG("[PBAP_003_004]--- connect_pbap len = %d ---", len);

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_PBAP_CONNECT, clientAddr);

	memcpy( &dst, clientAddr, sizeof(bdaddr_t) );

	device = manager_find_device(NULL, NULL, &dst, OBEX_PBAP_INTERFACE, FALSE);
	if ( device == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] device is nothing");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_FIND_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_FIND_DEV, __LINE__, &dst);
		goto failed;
	}

	BLUEZ_DEBUG_LOG("[PBAP_003_005]--- connect_pbap device = %p ---", device);

	if( btd_manager_isconnect( MNG_CON_PBAP ) == TRUE ) {
		/* 切断中 */
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] PBAP is still under connection");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_UNDER_CONNECTION, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_UNDER_CONNECTION, __LINE__, &dst);
		goto failed;
	}

	// RFCOMMの接続(チャンネル、デバイス)
	status = rfcomm_connect(rfChannel, device);
	BLUEZ_DEBUG_LOG("[PBAP] rfcomm_connect() rfChannel = %d ---",rfChannel);
	BLUEZ_DEBUG_LOG("[PBAP_003_006]--- connect_pbap status = %d ---", status);

	if (status < 0)
	{
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] rfcomm_connect() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_CONNECT_RFCOMM_CONNECT, status, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_CONNECT_RFCOMM_CONNECT, __LINE__, &dst);
		goto failed;
	}
	/* 切断方向初期化 */
	disconnect_initiator = FALSE;

	BLUEZ_DEBUG_LOG("[PBAP_003_008]--- connect_pbap dbus_message_new_method_return ---");
	BLUEZ_DEBUG_INFO("[PBAP] connect_pbap() End.");
	return dbus_message_new_method_return(msg);

failed:
	value = FALSE;
	g_dbus_emit_signal(g_pbap_connection, "/",
			OBEX_PBAP_INTERFACE,
			"CONNECT_CFM",
			DBUS_TYPE_INT32, &value,			//接続完了通知の成功/失敗
			DBUS_TYPE_UINT32,  &supp_repo,
			DBUS_TYPE_INVALID);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_PBAP_CONNECT_CFM, value);

	return dbus_message_new_method_return(msg);
}

//ソケット切断
static DBusMessage *disconnect_pbap(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	const char *clientAddr;
	uint32_t len;
	bdaddr_t dst;
	struct audio_device *device = NULL;

	BLUEZ_DEBUG_INFO("[PBAP] disconnect_pbap() Start.");
	BLUEZ_DEBUG_LOG("[PBAP_002_002]--- disconnect_pbap Call ---");

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
						&clientAddr, &len,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[PBAP] disconnect_pbap() clientAddd = [ %02x:%02x:%02x:%02x:%02x:%02x ]",clientAddr[5],clientAddr[4],clientAddr[3],clientAddr[2],clientAddr[1],clientAddr[0]);

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_PBAP_DISCONNECT, clientAddr);

	memcpy( &dst, clientAddr, sizeof(bdaddr_t) );

	device = manager_find_device(NULL, NULL, &dst, OBEX_PBAP_INTERFACE, FALSE);

	if( device == NULL || pbap_get_state( device ) <= PBAP_STATE_DISCONNECTED ) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] pbap already disconnected !!");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DISCONNECT_FIND_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DISCONNECT_FIND_DEV, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}

	pbap_shutdown(device);
	BLUEZ_DEBUG_LOG("[PBAP_005_001]--- disconnect_pbap pbap_shutdown End.");
	pbap_set_state(device, PBAP_STATE_DISCONNECTED);
	BLUEZ_DEBUG_LOG("[PBAP_005_002]--- disconnect_pbap dbus_message_new_method_return ---");
	BLUEZ_DEBUG_INFO("[PBAP] disconnect_pbap() End.");

	disconnect_initiator = TRUE;

	/* 既に切断通知をあげているか判定 */
	if( btd_manager_isconnect( MNG_CON_PBAP ) == FALSE ) {
		BLUEZ_DEBUG_LOG("[PBAP] pbap already disconnected");
		btd_manager_pbap_signal_disconnect_cfm( FALSE );
		disconnect_initiator = FALSE;
	}

	return dbus_message_new_method_return(msg);
}

void pbap_shutdown(struct audio_device *dev)
{
	struct pbap *pbap = dev->pbap;
	struct pending_connect *p = pbap->pending;

	BLUEZ_DEBUG_INFO("[PBAP] pbap_shutdown() Start.");

	pending_connect_finalize(dev);
	BLUEZ_DEBUG_LOG("[PBAP_006_002]--- pbap_shutdown pending_connect_finalize End.");
	BLUEZ_DEBUG_INFO("[PBAP] pbap_shutdown() End.");
}

//ソケット送信
static DBusMessage *datasend_pbap(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	struct audio_device *device;
	struct pbap *pbap;
	const char *senddata;
	int status;
	uint32_t len;
	const char *clientAddr;
	uint32_t addrlen;
	bdaddr_t dst;

	BLUEZ_DEBUG_INFO("[PBAP] datasend_pbap() Start---");
	BLUEZ_DEBUG_LOG("[PBAP_002_003]--- datasend_pbap Call ---");

	if (!dbus_message_get_args(msg, NULL,
							   DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
							   &senddata, &len,
							   DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
							   &clientAddr, &addrlen,
							   DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}

	BLUEZ_DEBUG_LOG("[PBAP] datasend_pbap() clientAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", clientAddr[5], clientAddr[4], clientAddr[3], clientAddr[2], clientAddr[1], clientAddr[0]);

	memcpy( &dst, clientAddr, sizeof(bdaddr_t) );
	device = manager_find_device(NULL, NULL, &dst, OBEX_PBAP_INTERFACE, FALSE);
	if ( device == NULL || device->pbap == NULL ) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] device = NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_FIND_DEV, 0, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DATASEND_FIND_DEV, __LINE__, &dst);
		return dbus_message_new_method_return(msg);
	}
	pbap = device->pbap;

	BLUEZ_DEBUG_LOG("[PBAP_007_003]--- datasend_pbap len = %d ---", len);

	status = rfcomm_send(pbap, len, senddata);
	BLUEZ_DEBUG_LOG("[PBAP_007_004]--- datasend_pbap status = %d ---", status);
	if( status < 0 ) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] rfcomm_send() Err(%d)", status);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DATASEND_RFCOMM_SEND, status, 0);
		EVENT_DRC_BLUEZ_BDADDR(FORMAT_ID_BT_ERR_BDADDR, BLUEZ_SRC_ID_BDADDR, ERROR_BLUEZ_DATASEND_RFCOMM_SEND, __LINE__, &dst);
	}

	BLUEZ_DEBUG_LOG("[PBAP_007_005]--- datasend_pbap dbus_message_new_method_return ---");
	BLUEZ_DEBUG_INFO("[PBAP] datasend_pbap() End.");
	return dbus_message_new_method_return(msg);
}

static int rfcomm_send(struct pbap *pbap, int len, const  char *sendData, ...)
{
	int ret;

	BLUEZ_DEBUG_INFO("[PBAP] rfcomm_send() Start.");

	BLUEZ_DEBUG_LOG("[PBAP_008_001]--- rfcomm_send va_Start.");
	ret = pbap_send_obexpacket(pbap, len, sendData);
	BLUEZ_DEBUG_LOG("[PBAP_008_002]--- rfcomm_send ret = %d ---", ret);
	BLUEZ_DEBUG_LOG("[PBAP_008_003]--- rfcomm_send va_end End.");

	BLUEZ_DEBUG_INFO("[PBAP] rfcomm_send() End.");
	return ret;
}

//接続待受を開始
static DBusMessage *register_pbap(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	GError *err = NULL;
	int value = TRUE;
	bdaddr_t src;
	sdp_record_t *record;

	BLUEZ_DEBUG_INFO("[PBAP] register_pbap() Start.");
	BLUEZ_DEBUG_LOG("[PBAP_002_004]--- register_pbap Call ---");

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_PBAP_REGISTER, NULL);

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_DBUS_GET_ARGS, 0, 0);
		return NULL;
	}

	bluezutil_get_bdaddr( &src );
	
	if (!g_pbap_record_id) {
		record = pbap_sdp_record();
		if (record == NULL) {
			BLUEZ_DEBUG_ERROR("[Bluez - PBAP] pbap_sdp_record() SDP ERROR");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_SDP_RECORD, 0, 0);
			return NULL;
		}

		if (add_record_to_server(&src, record) < 0) {
			BLUEZ_DEBUG_ERROR("[Bluez - PBAP] Unable to register OBEX Pbap service record");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_REGISTER_ADD_RECORD, 0, 0);
			sdp_record_free(record);
			return NULL;
		}
		
		g_pbap_record_id = record->handle;
	}

	g_dbus_emit_signal(g_pbap_connection, "/",
					OBEX_PBAP_INTERFACE,
					"REGISTER_CFM",
					DBUS_TYPE_INT32, &value,			//接続待受開始通知の成功/失敗
					DBUS_TYPE_INVALID);
	BLUEZ_DEBUG_LOG("[PBAP_009_006]--- register_pbap g_dbus_emit_signal ---");

	BLUEZ_DEBUG_LOG("[PBAP_009_007]--- register_pbap dbus_message_new_method_return  ---");

	BLUEZ_DEBUG_INFO("[PBAP] register_pbap() End.");

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_PBAP_REGISTER_CFM, value);		

	return dbus_message_new_method_return(msg);
}

//接続待受を停止
static DBusMessage *deregister_pbap(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	int sock;
	int value = TRUE;

	BLUEZ_DEBUG_INFO("[PBAP] deregister_pbap() Start---");
	BLUEZ_DEBUG_LOG("[PBAP_002_005]--- register_pbap Call ---");

	EVENT_DRC_BLUEZ_METHOD(FORMAT_ID_DBUS_METHOD, METHOD_PBAP_DEREGISTER, NULL);

	g_dbus_emit_signal(g_pbap_connection, "/",
					OBEX_PBAP_INTERFACE,
					"DEREGISTER_CFM",
					DBUS_TYPE_INT32, &value,			//接続待受停止通知の成功/失敗
					DBUS_TYPE_INVALID);
	BLUEZ_DEBUG_LOG("[PBAP_010_002]--- deregister_pbap g_dbus_emit_signal ---");

	BLUEZ_DEBUG_LOG("[PBAP_010_003]--- deregister_pbap dbus_message_new_method_return ---");
	BLUEZ_DEBUG_INFO("[PBAP] deregister_pbap() End---");

	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_PBAP_DEREGISTER_CFM, value);

	return dbus_message_new_method_return(msg);
}

static sdp_record_t *pbap_sdp_record(void)
{
	sdp_list_t *svclass_id, *pfseq;
	uuid_t svclass_uuid;
	sdp_profile_desc_t profile;
	sdp_record_t *record;

	BLUEZ_DEBUG_LOG("[PBAP] pbap_sdp_record() Start.");
	
	record = sdp_record_alloc();
	if (!record) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] sdp_record_alloc() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_SDP_RECORD, 0, 0);
		return NULL;
	}

	BLUEZ_DEBUG_LOG("[PBAP] pbap_sdp_record() Phonebook Access - PCE UUID (0x112E) ---");
	//Phonebook Access - PCE UUID (0x112E)
	sdp_uuid16_create(&svclass_uuid, PBAP_PCE_SVCLASS_ID);
	svclass_id = sdp_list_append(0, &svclass_uuid);
	sdp_set_service_classes(record, svclass_id);

	BLUEZ_DEBUG_LOG("[PBAP] pbap_sdp_record() Phonebook Access Version 1.1 ---");
	sdp_uuid16_create(&profile.uuid, PBAP_SVCLASS_ID);
	profile.version = 0x0101;
	pfseq = sdp_list_append(0, &profile);
	sdp_set_profile_descs(record, pfseq);

	BLUEZ_DEBUG_LOG("[PBAP] pbap_sdp_record() Phonebook Access Name : PhonebookAccess PCE ---");
	sdp_set_info_attr(record, "PhonebookAccess PCE", 0, 0);

	sdp_list_free(pfseq, 0);
	sdp_list_free(svclass_id, 0);

	BLUEZ_DEBUG_LOG("[PBAP] pbap_sdp_record() End.");
	
	return record;
}

static GDBusMethodTable pbap_methods[] = {
	{ "CONNECT",	"iau",	"", connect_pbap,		0,	0	},	//PBAPの接続開始を行う。
	{ "DISCONNECT",	"au",	"", disconnect_pbap,	0,	0	},	//PBAPの切断開始を行う。
	{ "DATA_SND",	"auau",	"", datasend_pbap,		0,	0	},	//PBAPのデータ送信を行う。
	{ "REGISTER",	"",		"", register_pbap,		0,	0	},	//PBAPの接続待受を開始する。
	{ "DEREGISTER",	"", 	"", deregister_pbap,	0,	0	},	//PBAPの接続待受を停止する。
	{ NULL, NULL, NULL, NULL, 0, 0 }
};

static GDBusSignalTable pbap_signals[] = {
	{ "REGISTER_CFM",		"i",		G_DBUS_SIGNAL_FLAG_DEPRECATED },	//PBAPの接続待受開始通知
	{ "DEREGISTER_CFM", 	"i",		G_DBUS_SIGNAL_FLAG_DEPRECATED },	//PBAPの接続待受停止通知
	{ "CONNECT_CFM",		"iu",		G_DBUS_SIGNAL_FLAG_DEPRECATED },	//PBAPの接続完了の通知
	{ "DISCONNECT_CFM", 	"q",		G_DBUS_SIGNAL_FLAG_DEPRECATED },	//PBAPの切断完了の通知
	{ "DISCONNECTED",		"q",		G_DBUS_SIGNAL_FLAG_DEPRECATED },	//リモートデバイスからの切断の通知
	{ "DATA_RCV",			"iay",		G_DBUS_SIGNAL_FLAG_DEPRECATED },	//PBAPのデータ受信の通知
	{ NULL, NULL, G_DBUS_SIGNAL_FLAG_DEPRECATED }
};


static void path_unregister(void *data)
{

	BLUEZ_DEBUG_LOG("[PBAP] Unregistered interface %s on path %s",
		OBEX_PBAP_INTERFACE, "/");
}

void pbap_unregister(struct audio_device *dev)
{
	struct pbap *pbap = dev->pbap;
	int linkinfo = 0;
	int value = 0xFF;
	uint8_t supp_repo = 0;

	BLUEZ_DEBUG_LOG("[PBAP] pbap unregister");

	if (pbap->state > PBAP_STATE_DISCONNECTED) {
		BLUEZ_DEBUG_LOG("[PBAP] Pbap unregistered while device was connected!");
		pbap_shutdown(dev);
		if( pbap->state != PBAP_STATE_CONNECTED ) {
			/* 接続が終っておらず、ACL が張られていない場合、接続完了通知（失敗）をあげる必要がある */
			linkinfo = btd_manager_find_connect_info_by_bdaddr( &dev->dst );
			if( linkinfo == MNG_ACL_MAX ) {
				BLUEZ_DEBUG_WARN("[PBAP] Unregister occurred : connect_cfm return NG(0xFF)");
				g_dbus_emit_signal(g_pbap_connection, "/",
						OBEX_PBAP_INTERFACE,
						"CONNECT_CFM",
						DBUS_TYPE_INT32, &value,
						DBUS_TYPE_UINT32,  &supp_repo,
						DBUS_TYPE_INVALID);
				EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_PBAP_CONNECT_CFM, value);
			}
		}

		pbap_set_state(dev, PBAP_STATE_DISCONNECTED);
	} else {
		pbap_close_rfcomm(dev);
	}
	
	g_free(pbap);
	
	dev->pbap = NULL;
}

struct pbap  *pbap_init(struct audio_device *dev)
{
	BLUEZ_DEBUG_INFO("[PBAP] pbap_init() Start.");
	struct pbap *pbap;
	
	BLUEZ_DEBUG_LOG("[PBAP_001_001]--- pbap_init ---");
	EVENT_DRC_BLUEZ_INIT(FORMAT_ID_BT_INIT, INIT_PBAP);

	pbap = g_new0(struct pbap, 1);
	BLUEZ_DEBUG_LOG("[PBAP_001_002]--- pbap_init pbap = %p ---", pbap);
	pbap->dev = dev;
	BLUEZ_DEBUG_INFO("[PBAP] pbap_init() End.");

	return pbap;

}

int pbap_dbus_init(void)
{
	BLUEZ_DEBUG_INFO("[PBAP] pbap_dbus_init() Start.");
	EVENT_DRC_BLUEZ_INIT(FORMAT_ID_BT_INIT, INIT_DBUS_PBAP);

	if (g_pbap_connection != NULL  ) {
		g_dbus_unregister_interface(g_pbap_connection, "/", OBEX_PBAP_INTERFACE);
		g_pbap_connection = NULL;
	}

	g_pbap_connection = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
	if (g_dbus_register_interface(g_pbap_connection, "/",
					OBEX_PBAP_INTERFACE,
					pbap_methods, pbap_signals,
					NULL, NULL, path_unregister) == FALSE) {
		BLUEZ_DEBUG_ERROR("[Bluez - PBAP] g_dbus_register_interface(). ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DBUS_INIT, 0, 0);
		return -1;
	}
	btd_manager_set_connection_pbap( g_pbap_connection );

	BLUEZ_DEBUG_LOG("[PBAP] pbap_dbus_init()->g_dbus_register_interface().SUCCESS");
	BLUEZ_DEBUG_INFO("[PBAP] pbap_dbus_init() End.");
	return 0;
}

int pbap_get_channel(struct audio_device *dev)
{
	return dev->pbap->rfcomm_ch;
}

int pbap_get_disconnect_initiator(void)
{
	return disconnect_initiator;
}

void pbap_set_disconnect_initiator(int flg)
{
	disconnect_initiator = flg;
	return;
}
