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

#include <fcntl.h>
#include <sys/stat.h>
#include <bluetooth/bluetooth.h>

#include "log.h"
#include "recorder.h"
#include "include/linux/lites_trace.h"

/* Source file ID */
#define BLUEZ_SRC_ID BLUEZ_SRC_ID_SRC_RECORDER_C

//グループID
/** BTブロックIF用 */
#define GROUP_ID_EVENT_LOG		REGION_EVENT_LOG
/** ソフトエラー用 */
#define GROUP_ID_TRACE_ERROR   REGION_TRACE_ERROR

//トレースID
/** BTブロックIF用 */
#define TRACE_ID_BT_BLOCK_IF  REGION_EVENT_BT_BLOCK_IF
/** ソフトエラー用 */
#define TRACE_ID_ERROR			0

//トレース番号
#define TRACE_NO_LIB_BLUEZ	(LITES_LIB_LAYER + 33)

static int errorRecorderBluez(int traceNo, int errorId, int srcId, int line, int para1, int para2);
static int eventRecorderBluez(int traceNo, int formatId, int size, void *logBuf);

/*-----------------------------------------------------------
* [Name]		ERR_DRC_BLUEZ
* [errorId] 	エラーID
* [srcId]		ソースID
* [line]		行数
* [para1]		付加情報1
* [para2]		付加情報2
* [Note]		BLUEZのエラーログをドラレコに出力
*-------------------------------------------------------------
*/
int _ERR_DRC_BLUEZ(int errorId, int srcId, int line, int para1, int para2)
{
	BLUEZ_DEBUG_LOG("traceNo=%d, errorId=%d, srcId=%d, line=%d, para1=%d, para2=%d", 
		TRACE_NO_LIB_BLUEZ, errorId, srcId, line, para1, para2);

	return errorRecorderBluez(TRACE_NO_LIB_BLUEZ, errorId, srcId, line, para1, para2);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_METHOD
* [formatId] 	ログフォーマットID
* [method]		DBUSメソッド種別
* [bdaddr]		BTデバイスアドレス
* [Note] 		BLUEZのDBUSメソッドイベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_METHOD(int formatId, int method, const char *bdaddr)
{	
	struct drc_bt_method_event_log	logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.method = (unsigned int)method;
	if(bdaddr != NULL){
		memcpy(logBuf.bdaddr, bdaddr, sizeof(logBuf.bdaddr));
	}
	size = sizeof(logBuf);
	
	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_METHOD() traceNo=%d, formatId=%d, method=%d, size=%d, clientAddd = [ %02x:%02x:%02x:%02x:%02x:%02x ]", TRACE_NO_LIB_BLUEZ, formatId, logBuf.method,
 size, logBuf.bdaddr[5],logBuf.bdaddr[4],logBuf.bdaddr[3],logBuf.bdaddr[2],logBuf.bdaddr[1],logBuf.bdaddr[0] );

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_SIGNAL
* [formatId] 	ログフォーマットID
* [signal]		DBUSシグナル種別
* [result]		シグナルで通知するBT動作の結果
* [Note] 		BLUEZのDBUSシグナルイベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_SIGNAL(int formatId, int signal, int result)
{	
	struct drc_bt_signal_event_log	logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.signal = (unsigned int)signal;
	logBuf.result = (unsigned int)result;
	size = sizeof(logBuf);

	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_SIGNAL() traceNo=%d, formatId=%d, signal=%d, result = %d, size=%d", TRACE_NO_LIB_BLUEZ, formatId, logBuf.signal, logBuf.result, size);

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_CONNECT_CB
* [formatId] 	ログフォーマットID
* [callback]	Callback種別
* [connection]	接続方向
* [Note] 		BLUEZのCONNECT_CBイベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_CONNECT_CB(int formatId, int callback, int connection)
{	
	struct drc_bt_connect_cb_event_log	logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.callback = (unsigned int)callback;
	logBuf.connection = (unsigned int)connection;
	size = sizeof(logBuf);

	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_CONNECT_CB() traceNo=%d, formatId=%d, callback=%d,  connection=%d, size=%d", TRACE_NO_LIB_BLUEZ, formatId, logBuf.callback, logBuf.connection, size);

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_A2DP_CONNECT_CB
* [formatId] 	ログフォーマットID
* [connection]	接続方向
* [first]		一本目接続
* [second]		二本目接続
* [Note] 		A2DPのCONNECT_CBイベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_A2DP_CONNECT_CB(int formatId, int connection, int first, int second)
{	
	struct drc_bt_a2dp_connect_cb_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.connection = (unsigned int)connection;
	logBuf.first = (unsigned int)first;
	logBuf.second = (unsigned int)second;
	size = sizeof(logBuf);

	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_A2DP_CONNECT_CB() traceNo=%d, formatId=%d, connection=%d,  first=%d, second=%d, size=%d", TRACE_NO_LIB_BLUEZ, 
					formatId, logBuf.connection, logBuf.first, logBuf.second, size);

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_CB
* [formatId] 	ログフォーマットID
* [callback]	Callback種別
* [Note] 		BLUEZの一般Callbackイベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_CB(int formatId, int callback)
{	
	struct drc_bt_cb_event_log logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.callback = (unsigned int)callback;
	size = sizeof(logBuf);

	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_CB() traceNo=%d, formatId=%d, callback=%d, size=%d", TRACE_NO_LIB_BLUEZ, formatId, logBuf.callback, size);

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_HCI
* [formatId] 	ログフォーマットID
* [hciComm]		hciComm種類
* [Note] 		BLUEZのHCI DBUSメソッドイベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_HCI_METHOD(int formatId, int hciComm)
{	
	struct drc_bt_hci_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.hci_comm = (unsigned int)hciComm;
	size = sizeof(logBuf);

	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_HCI_METHOD() traceNo=%d, formatId=%d, hciComm=%d, size=%d", 
		TRACE_NO_LIB_BLUEZ, formatId, logBuf.hci_comm, size);

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_HCI_SIGNAL
* [formatId] 	ログフォーマットID
* [signal]		HCI DBUSシグナル種類
* [link_type]	リンクタイプ
* [status]		状態
* [Note] 		BLUEZのHCI DBUSシグナルイベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_HCI_SIGNAL(int formatId, int signal, unsigned char link_type, unsigned char status )
{
	struct drc_bt_mng_hci_signal_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.signal = (unsigned int)signal;
	logBuf.link_type = (unsigned char)link_type;
	logBuf.status = (unsigned char)status;
	size = sizeof(logBuf);

	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_HCI_SIGNAL() traceNo=%d, formatId=%d, signal=%d, link_type=0x%02x, status=0x%02x, size=%d", 
		TRACE_NO_LIB_BLUEZ, formatId, logBuf.signal, logBuf.link_type, logBuf.status, size);

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
	
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_DISCOVERY
* [formatId] 	ログフォーマットID
* [event]	 	イベント種別
* [param]		パラメータ
* [Note] 		端末検索イベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_DISCOVERY(int formatId, int event, int param)
{	
	struct drc_bt_discovery_event_log logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.event = (unsigned int)event;
	logBuf.param = (unsigned int)param;
	size = sizeof(logBuf);

	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_DISCOVERY() traceNo=%d, formatId=%d, event=%d,  param=%d, size=%d", TRACE_NO_LIB_BLUEZ, formatId, logBuf.event, logBuf.param, size);

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_PAIRING
* [formatId] 	ログフォーマットID
* [event]		イベント種別
* [status]		状態(完了)/ペアリング数(接続要求)/io_cap(開始)
* [bdaddr]		BTデバイスアドレス
* [Note] 		ペアリングイベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_PAIRING(int formatId, int event, unsigned char status, bdaddr_t *bdaddr)
{	
	struct drc_bt_pairing_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.event = (unsigned int)event;
	logBuf.status = (unsigned int)status;
	if(bdaddr != NULL){
		memcpy(logBuf.bdaddr, bdaddr, sizeof(logBuf.bdaddr));
	}
	size = sizeof(logBuf);
	
	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_PAIRING() traceNo=%d, formatId=%d, event=%d, status=%d, size=%d, bdaddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", TRACE_NO_LIB_BLUEZ, formatId, logBuf.event, 
	logBuf.status, size, logBuf.bdaddr[5],logBuf.bdaddr[4],logBuf.bdaddr[3],logBuf.bdaddr[2],logBuf.bdaddr[1],logBuf.bdaddr[0] );

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_INIT
* [formatId] 	ログフォーマットID
* [event]		イベント種類
* [Note] 		BLUEZの初期化イベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_INIT(int formatId, int event)
{	
	struct drc_bt_init_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.event = (unsigned int)event;

	size = sizeof(logBuf);

	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_INIT() traceNo=%d, formatId=%d, event=%d, size=%d", 
		TRACE_NO_LIB_BLUEZ, formatId, logBuf.event, size);

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_MANAGER
* [formatId] 	ログフォーマットID
* [event]		イベント種類
* [bdaddr]		BTデバイスアドレス
* [param]		状態等
* [Note] 		BT MANAGER 関連動作を示すログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_MANAGER(int formatId, int event, bdaddr_t *bdaddr, int param)
{
	struct drc_bt_manager_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;

	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.event = (unsigned int)event;
	logBuf.param = (unsigned int)param;
	if(bdaddr != NULL){
		memcpy(logBuf.bdaddr, bdaddr, sizeof(logBuf.bdaddr));
	}
	size = sizeof(logBuf);

	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_MANAGER() traceNo=%d, formatId=%d, event=%d, size=%d, clientAddd = [ %02x:%02x:%02x:%02x:%02x:%02x ] param=%d", TRACE_NO_LIB_BLUEZ, formatId, logBuf.event, 
					size, logBuf.bdaddr[5],logBuf.bdaddr[4],logBuf.bdaddr[3],logBuf.bdaddr[2],logBuf.bdaddr[1],logBuf.bdaddr[0], logBuf.param );

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_CONFIRM
* [formatId] 	ログフォーマットID
* [event]		イベント種類
* [bdaddr]		BTデバイスアドレス
* [Note] 		PROFILE CONFIRM 動作を示すログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_CONFIRM(int formatId, int event, bdaddr_t *bdaddr)
{
	struct drc_bt_confirm_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;

	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.event = (unsigned int)event;
	if(bdaddr != NULL){
		memcpy(logBuf.bdaddr, bdaddr, sizeof(logBuf.bdaddr));
	}
	size = sizeof(logBuf);

	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_CONFIRM() traceNo=%d, formatId=%d, event=%d, size=%d, clientAddd = [ %02x:%02x:%02x:%02x:%02x:%02x ]", TRACE_NO_LIB_BLUEZ, formatId, logBuf.event, 
					size, logBuf.bdaddr[5],logBuf.bdaddr[4],logBuf.bdaddr[3],logBuf.bdaddr[2],logBuf.bdaddr[1],logBuf.bdaddr[0]);

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_STATE
* [formatId] 	ログフォーマットID
* [event]		イベント種類
* [bdaddr]		BTデバイスアドレス
* [state]		状態
* [Note] 		PROFILE STATE を示すログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_STATE(int formatId, int event, bdaddr_t *bdaddr, int state)
{
	struct drc_bt_state_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;

	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.event = (unsigned int)event;
	logBuf.state = (unsigned int)state;
	if(bdaddr != NULL){
		memcpy(logBuf.bdaddr, bdaddr, sizeof(logBuf.bdaddr));
	}
	size = sizeof(logBuf);

	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_STATE() traceNo=%d, formatId=%d, event=%d, size=%d, clientAddd = [ %02x:%02x:%02x:%02x:%02x:%02x ] state=%d", TRACE_NO_LIB_BLUEZ, formatId, logBuf.event, 
					size, logBuf.bdaddr[5],logBuf.bdaddr[4],logBuf.bdaddr[3],logBuf.bdaddr[2],logBuf.bdaddr[1],logBuf.bdaddr[0], logBuf.state );

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_ADAPTER
* [formatId] 	ログフォーマットID
* [event]		イベント種類
* [param]		DBUSメソッド種別
* [Note] 		BT ADAPTER 関連動作を示すログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_ADAPTER(int formatId, int event, int param)
{
	struct drc_bt_adapter_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;

	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.event = (unsigned int)event;
	logBuf.param = (unsigned int)param;
	size = sizeof(logBuf);

	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_ADAPTER() traceNo=%d, formatId=%d, event=%d, size=%d param=%d", TRACE_NO_LIB_BLUEZ, formatId, logBuf.event, size, logBuf.param);

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_DID
* [formatId] 	ログフォーマットID
* [event]		イベント種類
* [result]		結果
* [vendorID]	ベンダーID
* [productID]	プロダクトID
* [version]		バージョン
* [Note] 		BT DID 関連動作を示すログをドラレコに出力
*-------------------------------------------------------------
*/

int EVENT_DRC_BLUEZ_DID(int formatId, int event, int result, int vendorID, int productID, int version)
{
	struct drc_bt_did_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;

	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.event = (unsigned int)event;
	logBuf.result = (unsigned int)result;
	logBuf.vendorID = (unsigned int)vendorID;
	logBuf.productID = (unsigned int)productID;
	logBuf.version = (unsigned int)version;
	size = sizeof(logBuf);

	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_DID() traceNo=%d, formatId=%d, event=%d, size=%d result=%d vendorID=%d productID=%d version=%d", TRACE_NO_LIB_BLUEZ, formatId, logBuf.event, size, logBuf.result, logBuf.vendorID, logBuf.productID, logBuf.version);

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_BDADDR
* [formatId] 	ログフォーマットID
* [event]		イベント種類
* [errId]		エラー時のエラーID
* [line]		行数
* [bdaddr]		BTデバイスアドレス
* [Note] 		エラー時のBTアドレス表示を示すログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_BDADDR(int formatId, int event, int errId, int line, bdaddr_t *bdaddr)
{
	struct drc_bt_bdaddr_event_log	logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.event  = (unsigned int)event;
	logBuf.errId = (unsigned int)errId;
	logBuf.line   = (unsigned int)line;
	if(bdaddr != NULL){
		memcpy(logBuf.bdaddr, bdaddr, sizeof(logBuf.bdaddr));
	}

	size = sizeof(logBuf);
	
	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_BDADDR() traceNo=%d, formatId=%d, event=%d, size=%d, errId=%d, line=%d, bdAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", TRACE_NO_LIB_BLUEZ, formatId, logBuf.event, 
	size, logBuf.errId, logBuf.line, logBuf.bdaddr[5],logBuf.bdaddr[4],logBuf.bdaddr[3],logBuf.bdaddr[2],logBuf.bdaddr[1],logBuf.bdaddr[0] );

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
	
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BLUEZ_CHGMSG
* [formatId] 	ログフォーマットID
* [event]		イベント種類
* [bdaddr]		BTデバイスアドレス
* [Note] 		接続通知の付け替えを示すログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BLUEZ_CHGMSG(int formatId, int event, bdaddr_t *bdaddr)
{
	struct drc_bt_chgmsg_event_log	logBuf;	// 機能毎ログ格納バッファ
	int size;

	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.event  = (unsigned int)event;
	if(bdaddr != NULL){
		memcpy(logBuf.bdaddr, bdaddr, sizeof(logBuf.bdaddr));
	}
	size = sizeof(logBuf);

	BLUEZ_DEBUG_LOG("EVENT_DRC_BLUEZ_CHGMSG() traceNo=%d, formatId=%d, event=%d, size=%d, bdAddr = [ %02x:%02x:%02x:%02x:%02x:%02x ]", TRACE_NO_LIB_BLUEZ, formatId, logBuf.event, 
	size, logBuf.bdaddr[5],logBuf.bdaddr[4],logBuf.bdaddr[3],logBuf.bdaddr[2],logBuf.bdaddr[1],logBuf.bdaddr[0] );

	return eventRecorderBluez(TRACE_NO_LIB_BLUEZ, formatId, size, &logBuf);
	
}

/** BLUEZブロックイベントログ出力 */
int eventRecorderBluez(int traceNo, int formatId, int size, void *logBuf)
{
	int fd; 
	int group_id = GROUP_ID_EVENT_LOG;		// 出力先グループID
	int ret;

	struct lites_trace_header trc_hdr;		// ドラレコ用ヘッダー
 	struct lites_trace_format trc_fmt;		// ドラレコ用フォーマット
	memset(&trc_fmt, 0x0, sizeof(trc_fmt));
	memset(&trc_hdr, 0x0, sizeof(trc_hdr));

	fd = open("/dev/lites", O_RDWR);		// <-libipasのAPIを使用するために必須
	if(fd < 0){
		BLUEZ_DEBUG_ERROR("[Bluez - RECORDER] open() error. fd = %d", fd);
		return -1;
	}

	trc_fmt.trc_header = &trc_hdr;							// ドラレコ用ヘッダー格納
	trc_fmt.count = size;									// 保存ログバッファのサイズ
	trc_fmt.buf = logBuf;									// 保存ログバッファ
	trc_fmt.trc_header->level = LOG_LEVEL_EVENT;			// ログレベル
	trc_fmt.trc_header->trace_no = traceNo ;				// トレース番号(星取表のNo)
	trc_fmt.trc_header->trace_id = TRACE_ID_BT_BLOCK_IF;	// トレースID(機能毎の値)
	trc_fmt.trc_header->format_id = formatId;				// フォーマットID (bufのフォーマット識別子)

	ret = lites_trace_write(fd, group_id, &trc_fmt);	// ログ出力
	if (ret < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - RECORDER] lites_trace_write() error ret = %d", ret);
	}

	close(fd);

	return ret;
}

/** BLUEZブロックソフトエラー用ログ出力 */
int errorRecorderBluez(int traceNo, int errorId, int srcId, int line, int para1, int para2)
{
	int fd; 
	int group_id = GROUP_ID_TRACE_ERROR;	// 出力先グループID
	int ret;
 
	struct lites_trace_header trc_hdr;		// ドラレコ用ヘッダー
 	struct lites_trace_format trc_fmt;		// ドラレコ用フォーマット
	struct drc_bt_error_log  logBuf;		// エラーログ格納バッファ
	memset(&trc_fmt, 0x0, sizeof(trc_fmt));
	memset(&trc_hdr, 0x0, sizeof(trc_hdr));
	memset(&logBuf, 0x0, sizeof(logBuf));

	fd = open("/dev/lites", O_RDWR);		// <-libipasのAPIを使用するために必須
	if(fd < 0){
		BLUEZ_DEBUG_ERROR("[Bluez - RECORDER] open() error. fd = %d", fd);
		return -1;
	}

	logBuf.err_id = (unsigned int)errorId;
	logBuf.src_id	= (unsigned int)srcId;
	logBuf.line		= (unsigned int)line;
	logBuf.param1	= (unsigned int)para1;
	logBuf.param2	= (unsigned int)para2;

	trc_fmt.trc_header = &trc_hdr;							// ドラレコ用ヘッダー格納
	trc_fmt.count = sizeof(logBuf);							// 保存ログバッファのサイズ
	trc_fmt.buf = &logBuf;									// 保存ログバッファ
	trc_fmt.trc_header->level = LOG_LEVEL_ERROR;			// エラーログレベル
	trc_fmt.trc_header->trace_no = traceNo;					// トレース番号(星取表のNo)
	trc_fmt.trc_header->trace_id = TRACE_ID_ERROR;			// トレースID(機能毎の値)
	trc_fmt.trc_header->format_id = FORMAT_ID_ERROR_LOG;	// フォーマットID (bufのフォーマット識別子)

	ret = lites_trace_write(fd, group_id, &trc_fmt);		// ログ出力
	if (ret < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - RECORDER] lites_trace_write() error ret = %d", ret);
	}

	close(fd);

	return ret;
}


