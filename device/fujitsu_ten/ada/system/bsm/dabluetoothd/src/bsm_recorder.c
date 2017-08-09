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
#include <ada/log.h>
#include <cutils/log.h>

#include "include/linux/lites_trace.h"

#define BSM_RECORDER_LOGE(fmt, ...) \
		(void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_ERROR, "libbsm", __FUNCTION__, fmt, ##__VA_ARGS__)
#if 0
#define BSM_RECORDER_LOGD(fmt, ...) \
		(void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_DEBUG, "libbsm", __FUNCTION__, fmt, ##__VA_ARGS__)
#else
#define BSM_RECORDER_LOGD(fmt, ...) 
#endif

//グループID
/** BTブロックIF用 */
#define GROUP_ID_EVENT_LOG     REGION_EVENT_LOG
/** ソフトエラー用 */
#define GROUP_ID_TRACE_ERROR   REGION_TRACE_ERROR

//トレースID
/** BTブロックIF用 */
#define TRACE_ID_BT_BLOCK_IF  REGION_EVENT_BT_BLOCK_IF
/** ソフトエラー用 */
#define TRACE_ID_ERROR        0

//トレース番号
/* ライブラリ層＆A2DP */
#define TRACE_NO_LIB_BSM_A2DP	(LITES_LIB_LAYER + 34)
/* ライブラリ層＆AVRCP */
#define TRACE_NO_LIB_BSM_AVRCP	(LITES_LIB_LAYER + 35)
/* ライブラリ層＆HFP */
#define TRACE_NO_LIB_BSM_HFP	(LITES_LIB_LAYER + 36)
/* ライブラリ層＆PBAP */
#define TRACE_NO_LIB_BSM_PBAP	(LITES_LIB_LAYER + 38)

//フォーマットID
/** エラーログのフォーマットID */
#define FORMAT_ID_ERROR_LOG 1
#define FORMAT_ID_ERROR_MSG_LOG 2

//ログレベル
#define LOG_LEVEL_EVENT   3 //informational
#define LOG_LEVEL_ERROR   3 //error conditions


typedef struct drc_bsm_send_method_event_log  {
	unsigned int method;     /** BT DBUSメソッド種別 */
	char btAddr[6];			/** BTアドレス */
} DRC_BSM_SEND_METHOD_EVENT_LOG;

	
typedef struct drc_bsm_get_signal_event_log  {
	unsigned int signal;     /** BT DBUSシグナル種別 */
	int result;            /** 通知結果 */
} DRC_BSM_GET_SIGNAL_EVENT_LOG;

typedef struct drc_bsm_hci_event_log  {
	unsigned int hci_comm;     /** HCIコマンド種別 */
    int result;                /** 実行結果 */
    unsigned char status; 	   /** HCIコマン状態 */
} DRC_BSM_HCI_EVENT_LOG;

typedef struct drc_bsm_action_event_log  {
	unsigned int param;     /** 付加情報 */
	int result;              /** 処理結果 */
} DRC_BSM_ACTION_EVENT_LOG;

typedef struct drc_bsm_error_log  {
	unsigned int err_id;     /** エラーＩＤ */
	unsigned int param1;     /** 付加情報1  */
	unsigned int param2;     /** 付加情報2  */
	unsigned int param3;     /** 付加情報3  */
	unsigned int param4;     /** 付加情報4  */
} DRC_BSM_ERROR_LOG;

#if 0
typedef struct drc_bsm_error_msg_log  {
	unsigned int err_id;     /** エラーＩＤ */
	unsigned char *msg;       /** メッセージ */
	unsigned int param1;     /** 付加情報1  */
	unsigned int param2;     /** 付加情報2  */
} DRC_BSM_ERROR_MSG_LOG;
#endif
static int errorRecorderBsm(int traceNo, int errorId, int para1, int para2, int para3, int para4);
//static int errorMessageRecorderBsm(int traceNo, int errorId, const char *msg, int para1, int para2);
static int eventRecorderBsm(int traceNo, int formatId, int size, void *logBuf);

/*-----------------------------------------------------------
* [Name] 		ERR_DRC_BSM_A2DP
* [errorId] 	エラーID
* [para1]       付加情報1
* [para1]		付加情報2
* [para1]		付加情報3
* [para1]		付加情報4
* [Note] 		BSM_A2DPのエラーログをドラレコに出力
*-------------------------------------------------------------
*/
int ERR_DRC_BSM_A2DP(int errorId, int para1, int para2, int para3, int para4)
{
	BSM_RECORDER_LOGD("ERR_DRC_BSM_A2DP() traceNo=%d, errorId=%d, para1=%d, para2=%d, para3=%d, para4=%d", 
		TRACE_NO_LIB_BSM_A2DP, errorId, para1, para2, para3, para4);

	return errorRecorderBsm(TRACE_NO_LIB_BSM_A2DP, errorId, para1, para2, para3, para4);
}

/*-----------------------------------------------------------
* [Name] 		ERR_DRC_BSM_AVRCP
* [errorId] 	エラーID
* [para1]       付加情報1
* [para1]		付加情報2
* [para1]		付加情報3
* [para1]		付加情報4
* [Note] 		BSM_AVRCPのエラーログをドラレコに出力
*-------------------------------------------------------------
*/
int ERR_DRC_BSM_AVRCP(int errorId, int para1, int para2, int para3, int para4)
{
	BSM_RECORDER_LOGD("ERR_DRC_BSM_AVRCP() traceNo=%d, errorId=%d, para1=%d, para2=%d, para3=%d, para4=%d", 
		TRACE_NO_LIB_BSM_AVRCP, errorId, para1, para2, para3, para4);

	return errorRecorderBsm(TRACE_NO_LIB_BSM_AVRCP, errorId, para1, para2, para3, para4);
}


/*-----------------------------------------------------------
* [Name] 		TRACE_NO_LIB_BSM_HFP
* [errorId] 	エラーID
* [para1]       付加情報1
* [para1]		付加情報2
* [para1]		付加情報3
* [para1]		付加情報4
* [Note] 		BSM_HFPのエラーログをドラレコに出力
*-------------------------------------------------------------
*/
int ERR_DRC_BSM_HFP(int errorId, int para1, int para2, int para3, int para4)
{
	BSM_RECORDER_LOGD("ERR_DRC_BSM_HFP() traceNo=%d, errorId=%d, para1=%d, para2=%d, para3=%d, para4=%d",
		TRACE_NO_LIB_BSM_HFP, errorId, para1, para2, para3, para4);

	return errorRecorderBsm(TRACE_NO_LIB_BSM_HFP, errorId, para1, para2, para3, para4);
}

/*-----------------------------------------------------------
* [Name] 		TRACE_NO_LIB_BSM_PBAP
* [errorId] 	エラーID
* [para1]       付加情報1
* [para1]		付加情報2
* [para1]		付加情報3
* [para1]		付加情報4
* [Note] 		BSM_A2DPのエラーログをドラレコに出力
*-------------------------------------------------------------
*/
int ERR_DRC_BSM_PBAP(int errorId, int para1, int para2, int para3, int para4)
{
	BSM_RECORDER_LOGD("ERR_DRC_BSM_PBAP() traceNo=%d, errorId=%d, para1=%d, para2=%d, para3=%d, para4=%d", 
		TRACE_NO_LIB_BSM_PBAP, errorId, para1, para2, para3, para4);

	return errorRecorderBsm(TRACE_NO_LIB_BSM_PBAP, errorId, para1, para2, para3, para4);
}


/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BSM_SEND_METHOD_A2DP
* [formatId] 	ログフォーマットID
* [method]      DBUSメソッド種別
* [btAddr]		BTアドレス
* [Note] 		BSMのDBUSメソッドを送るイベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BSM_SEND_METHOD_A2DP(int formatId, int method, const char *btAddr)
{	
	struct drc_bsm_send_method_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.method = (unsigned int)method;
	if(btAddr != NULL){
		memcpy(logBuf.btAddr, btAddr, 6);
	}
	size = sizeof(logBuf);
	
	BSM_RECORDER_LOGD("EVENT_DRC_BSM_SEND_METHOD_A2DP() traceNo=%d, formatId=%d, event=%d, size=%d, clientAddd = %02x%02x%02x%02x%02x%02x", TRACE_NO_LIB_BSM_A2DP, formatId, logBuf.method,
 size, logBuf.btAddr[0],logBuf.btAddr[1],logBuf.btAddr[2],logBuf.btAddr[3],logBuf.btAddr[4],logBuf.btAddr[5] );

	return eventRecorderBsm(TRACE_NO_LIB_BSM_A2DP, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BSM_SEND_METHOD_AVRCP
* [formatId] 	ログフォーマットID
* [method]      DBUSメソッド種別
* [btAddr]		BTアドレス
* [Note] 		BSMのDBUSメソッドを送るイベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BSM_SEND_METHOD_AVRCP(int formatId, int method, const char *btAddr)
{	
	struct drc_bsm_send_method_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.method = (unsigned int)method;
	if(btAddr != NULL){
		memcpy(logBuf.btAddr, btAddr, 6);
	}
	size = sizeof(logBuf);
	
	BSM_RECORDER_LOGD("EVENT_DRC_BSM_SEND_METHOD_AVRCP() traceNo=%d, formatId=%d, event=%d, size=%d, clientAddd = %02x%02x%02x%02x%02x%02x", TRACE_NO_LIB_BSM_AVRCP, formatId, logBuf.method,
 size, logBuf.btAddr[0],logBuf.btAddr[1],logBuf.btAddr[2],logBuf.btAddr[3],logBuf.btAddr[4],logBuf.btAddr[5] );

	return eventRecorderBsm(TRACE_NO_LIB_BSM_AVRCP, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BSM_SEND_METHOD_HFP
* [formatId] 	ログフォーマットID
* [method]      DBUSメソッド種別
* [btAddr]		BTアドレス
* [Note] 		BSMのDBUSメソッドを送るイベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BSM_SEND_METHOD_HFP(int formatId, int method, const char *btAddr)
{	
	struct drc_bsm_send_method_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.method = (unsigned int)method;
	if(btAddr != NULL){
		memcpy(logBuf.btAddr, btAddr, 6);
	}
	size = sizeof(logBuf);
	
	BSM_RECORDER_LOGD("EVENT_DRC_BSM_SEND_METHOD_HFP() traceNo=%d, formatId=%d, event=%d, size=%d, clientAddd = %02x%02x%02x%02x%02x%02x", TRACE_NO_LIB_BSM_HFP, formatId, logBuf.method,
 size, logBuf.btAddr[0],logBuf.btAddr[1],logBuf.btAddr[2],logBuf.btAddr[3],logBuf.btAddr[4],logBuf.btAddr[5] );

	return eventRecorderBsm(TRACE_NO_LIB_BSM_HFP, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BSM_SEND_METHOD_PBAP
* [formatId] 	ログフォーマットID
* [method]      DBUSメソッド種別
* [btAddr]		BTアドレス
* [Note] 		BSMのDBUSメソッドを送るイベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BSM_SEND_METHOD_PBAP(int formatId, int method, const char *btAddr)
{	
	struct drc_bsm_send_method_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.method = (unsigned int)method;
	if(btAddr != NULL){
		memcpy(logBuf.btAddr, btAddr, 6);
	}
	size = sizeof(logBuf);
	
	BSM_RECORDER_LOGD("EVENT_DRC_BSM_SEND_METHOD_PBAP() traceNo=%d, formatId=%d, event=%d, size=%d, clientAddd = %02x%02x%02x%02x%02x%02x", TRACE_NO_LIB_BSM_PBAP, formatId, logBuf.method,
 size, logBuf.btAddr[0],logBuf.btAddr[1],logBuf.btAddr[2],logBuf.btAddr[3],logBuf.btAddr[4],logBuf.btAddr[5] );

	return eventRecorderBsm(TRACE_NO_LIB_BSM_PBAP, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_A2DP_LOG
* [formatId] 	ログフォーマットID
* [param]		パラメータ情報
* [Note] 		A2DPの通常ログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BSM_A2DP_LOG(int formatId, int param, int result){
	struct drc_bsm_action_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.param = (unsigned int)param;
	logBuf.result = result;
	size = sizeof(logBuf);
	
	BSM_RECORDER_LOGD("EVENT_DRC_A2DP_LOG() traceNo=%d, formatId=%d, event=%d, size=%d", TRACE_NO_LIB_BSM_A2DP, formatId, logBuf.param,size);
	
	return eventRecorderBsm(TRACE_NO_LIB_BSM_A2DP, formatId, size, &logBuf);
}
/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_AVRCP_LOG
* [formatId] 	ログフォーマットID
* [param]		パラメータ情報
* [Note] 		AVRCPの通常ログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BSM_AVRCP_LOG(int formatId, int param, int result){
	struct drc_bsm_action_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.param = (unsigned int)param;
	logBuf.result = result;
	size = sizeof(logBuf);
	
	BSM_RECORDER_LOGD("EVENT_DRC_AVRCP_LOG() traceNo=%d, formatId=%d, event=%d, size=%d", TRACE_NO_LIB_BSM_AVRCP, formatId, logBuf.param,size);
	
	return eventRecorderBsm(TRACE_NO_LIB_BSM_AVRCP, formatId, size, &logBuf);
}
/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_HFP_LOG
* [formatId] 	ログフォーマットID
* [param]		パラメータ情報
* [Note] 		HFPの通常ログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BSM_HFP_LOG(int formatId, int param, int result){
	struct drc_bsm_action_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.param = (unsigned int)param;
	logBuf.result = result;
	size = sizeof(logBuf);
	
	BSM_RECORDER_LOGD("EVENT_DRC_HFP_LOG() traceNo=%d, formatId=%d, event=%d, size=%d", TRACE_NO_LIB_BSM_HFP, formatId, logBuf.param,size);
	
	return eventRecorderBsm(TRACE_NO_LIB_BSM_HFP, formatId, size, &logBuf);
}
/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_PBAP_LOG
* [formatId] 	ログフォーマットID
* [param]		パラメータ情報
* [Note] 		PBAPの通常ログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BSM_PBAP_LOG(int formatId, int param, int result){
	struct drc_bsm_action_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.param = (unsigned int)param;
	logBuf.result = result;
	size = sizeof(logBuf);
	
	BSM_RECORDER_LOGD("EVENT_DRC_PBAP_LOG() traceNo=%d, formatId=%d, event=%d, size=%d", TRACE_NO_LIB_BSM_PBAP, formatId, logBuf.param,size);
	
	return eventRecorderBsm(TRACE_NO_LIB_BSM_PBAP, formatId, size, &logBuf);
}


/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BSM_GET_SIGNAL_A2DP
* [formatId] 	ログフォーマットID
* [signal]      DBUSシグナル種別
* [result]		シグナルで通知するBT動作の結果
* [Note] 		BSMのDBUSシグナルを取得イベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BSM_GET_SIGNAL_A2DP(int formatId, int signal, int result)
{	
	struct drc_bsm_get_signal_event_log logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.signal = (unsigned int)signal;
	logBuf.result = result;
	size = sizeof(logBuf);

	BSM_RECORDER_LOGD("EVENT_DRC_BSM_GET_SIGNAL_A2DP() traceNo=%d, formatId=%d, event=%d, size=%d, result = %d", TRACE_NO_LIB_BSM_A2DP, formatId, logBuf.signal, size, result);

	return eventRecorderBsm(TRACE_NO_LIB_BSM_A2DP, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BSM_GET_SIGNAL_AVRCP
* [formatId] 	ログフォーマットID
* [signal]      DBUSシグナル種別
* [result]		シグナルで通知するBT動作の結果
* [Note] 		BSMのDBUSシグナルを取得イベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BSM_GET_SIGNAL_AVRCP(int formatId, int signal, int result)
{	
	struct drc_bsm_get_signal_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.signal = (unsigned int)signal;
	logBuf.result = result;
	size = sizeof(logBuf);

	BSM_RECORDER_LOGD("EVENT_DRC_BSM_GET_SIGNAL_AVRCP() traceNo=%d, formatId=%d, event=%d, size=%d, result = %d", TRACE_NO_LIB_BSM_AVRCP, formatId, logBuf.signal, size, result);

	return eventRecorderBsm(TRACE_NO_LIB_BSM_AVRCP, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BSM_GET_SIGNAL_HFP
* [formatId] 	ログフォーマットID
* [signal]      DBUSシグナル種別
* [result]		シグナルで通知するBT動作の結果
* [Note] 		BSMのDBUSシグナルを取得イベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BSM_GET_SIGNAL_HFP(int formatId, int signal, int result)
{	
	struct drc_bsm_get_signal_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.signal = (unsigned int)signal;
	logBuf.result = result;
	size = sizeof(logBuf);

	BSM_RECORDER_LOGD("EVENT_DRC_BSM_GET_SIGNAL_HFP() traceNo=%d, formatId=%d, event=%d, size=%d, result = %d", TRACE_NO_LIB_BSM_HFP, formatId, logBuf.signal, size, result);

	return eventRecorderBsm(TRACE_NO_LIB_BSM_HFP, formatId, size, &logBuf);
}

/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BSM_GET_SIGNAL_PBAP
* [formatId] 	ログフォーマットID
* [signal]      DBUSシグナル種別
* [result]		シグナルで通知するBT動作の結果
* [Note] 		BSMのDBUSシグナルを取得イベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BSM_GET_SIGNAL_PBAP(int formatId, int signal, int result)
{	
	struct drc_bsm_get_signal_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.signal = (unsigned int)signal;
	logBuf.result = result;
	size = sizeof(logBuf);

	BSM_RECORDER_LOGD("EVENT_DRC_BSM_GET_SIGNAL_PBAP() traceNo=%d, formatId=%d, event=%d, size=%d, result = %d", TRACE_NO_LIB_BSM_PBAP, formatId, logBuf.signal, size, result);

	return eventRecorderBsm(TRACE_NO_LIB_BSM_PBAP, formatId, size, &logBuf);
}


/*-----------------------------------------------------------
* [Name] 		EVENT_DRC_BSM_GET_SIGNAL_HCI
* [formatId] 	ログフォーマットID
* [signal]      DBUSシグナル種別
* [result]		シグナルで通知するBT動作の結果
* [Note] 		BSMのDBUSシグナルを取得イベントログをドラレコに出力
*-------------------------------------------------------------
*/
int EVENT_DRC_BSM_GET_SIGNAL_HCI(int formatId, int hciComm, int result, unsigned char status)
{	
	struct drc_bsm_hci_event_log  logBuf;	// 機能毎ログ格納バッファ
	int size;
	
	memset(&logBuf, 0x0, sizeof(logBuf));
	logBuf.hci_comm = (unsigned int)hciComm;
	logBuf.result = result;
	logBuf.status = (unsigned char)status;
	size = sizeof(logBuf);

	BSM_RECORDER_LOGD("EVENT_DRC_BSM_GET_SIGNAL_HCI() traceNo=%d, formatId=%d, size=%d, hciComm=%d, result = %d,  status=%x, ", 
		TRACE_NO_LIB_BSM_HFP, formatId, size, logBuf.hci_comm, logBuf.result, logBuf.status);

	return eventRecorderBsm(TRACE_NO_LIB_BSM_HFP, formatId, size, &logBuf);
}

/** Bluezブロックイベントログ出力 */
static int eventRecorderBsm(int traceNo, int formatId, int size, void *logBuf)
{
	int fd; 
	int group_id = GROUP_ID_EVENT_LOG;	//  出力先グループID
    int ret;

    struct lites_trace_header trc_hdr;	// ドラレコ用ヘッダー
 	struct lites_trace_format trc_fmt;	// ドラレコ用フォーマット
	memset(&trc_fmt, 0x0, sizeof(trc_fmt));
	memset(&trc_hdr, 0x0, sizeof(trc_hdr));

	fd = open("/dev/lites", O_RDWR);	// <-libipasのAPIを使用するために必須
	if(fd < 0){
		BSM_RECORDER_LOGE("open()[eventRecorderBluez] error. fd=%d", fd);
		return -1;
	}

    trc_fmt.trc_header = &trc_hdr;		// ドラレコ用ヘッダー格納
	trc_fmt.count = size;				// 保存ログバッファのサイズ
	trc_fmt.buf = logBuf;				// 保存ログバッファ
    trc_fmt.trc_header->level = LOG_LEVEL_EVENT;	// ログレベル
    trc_fmt.trc_header->trace_no = traceNo ;		// トレース番号(星取表のNo)
    trc_fmt.trc_header->trace_id = TRACE_ID_BT_BLOCK_IF;	// トレースID(機能毎の値)
    trc_fmt.trc_header->format_id = formatId;	// フォーマットID (bufのフォーマット識別子)

    ret = lites_trace_write(fd, group_id, &trc_fmt);	// <-ログ出力時に必須
	if (ret < 0) {
		BSM_RECORDER_LOGE("lites_trace_write()[eventRecorderBluez] error ret=%d", ret);
	}

    close(fd);

    return ret;
}

/** Bluezブロックソフトエラー用ログ出力 */
static int errorRecorderBsm(int traceNo, int errorId, int para1, int para2, int para3, int para4)
{
	int fd; 
	int group_id = GROUP_ID_TRACE_ERROR;	//  出力先グループID
	int ret;
 
    struct lites_trace_header trc_hdr;		// ドラレコ用ヘッダー
 	struct lites_trace_format trc_fmt;		// ドラレコ用フォーマット
	struct drc_bsm_error_log  logBuf;	    // エラーログ格納バッファ
	memset(&trc_fmt, 0x0, sizeof(trc_fmt));
	memset(&trc_hdr, 0x0, sizeof(trc_hdr));
	memset(&logBuf, 0x0, sizeof(logBuf));

	fd = open("/dev/lites", O_RDWR);	    // <-libipasのAPIを使用するために必須
	if(fd < 0){
		BSM_RECORDER_LOGE("open()[errorRecorderBluez] error. fd=%d", fd);
		return -1;
	}

	logBuf.err_id = (unsigned int)errorId;
	logBuf.param1 = (unsigned int)para1;
	logBuf.param2 = (unsigned int)para2;
	logBuf.param3 = (unsigned int)para3;
	logBuf.param4 = (unsigned int)para4;

    trc_fmt.trc_header = &trc_hdr;		// ドラレコ用ヘッダー格納
	trc_fmt.count = sizeof(logBuf);		// 保存ログバッファのサイズ
	trc_fmt.buf = &logBuf;				// 保存ログバッファ
    trc_fmt.trc_header->level = LOG_LEVEL_ERROR;			// エラーログレベル
    trc_fmt.trc_header->trace_no = traceNo;					// トレース番号(星取表のNo)
    trc_fmt.trc_header->trace_id = TRACE_ID_ERROR;	    	// トレースID(機能毎の値)
    trc_fmt.trc_header->format_id = FORMAT_ID_ERROR_LOG;	// フォーマットID (bufのフォーマット識別子)

    ret = lites_trace_write(fd, group_id, &trc_fmt);	    // <-ログ出力時に必須
	if (ret < 0) {
		BSM_RECORDER_LOGE("lites_trace_write()[errorRecorderBluez] error ret=%d", ret);
	}

    close(fd);

    return ret;
}
#if 0
int ERR_DRC_BSM_A2DP_MSG(int errorId, const char *msg, int para1, int para2){
	BSM_RECORDER_LOGD("ERR_DRC_BSM_A2DP() traceNo=%d, errorId=%d, msg=%s, para1=%d, para2=%d", 
		TRACE_NO_LIB_BSM_A2DP, errorId, msg, para1, para2);
	return errorMessageRecorderBsm(TRACE_NO_LIB_BSM_A2DP, errorId, msg, para1, para2);
}
int ERR_DRC_BSM_AVRCP_MSG(int errorId, const char *msg, int para1, int para2){
	BSM_RECORDER_LOGD("ERR_DRC_BSM_AVRCP_MSG() traceNo=%d, errorId=%d, msg=%s, para1=%d, para2=%d", 
		TRACE_NO_LIB_BSM_AVRCP, errorId, msg, para1, para2);
	return errorMessageRecorderBsm(TRACE_NO_LIB_BSM_AVRCP, errorId, msg, para1, para2);
}
int ERR_DRC_BSM_HFP_MSG(int errorId, const char *msg, int para1, int para2){
	BSM_RECORDER_LOGD("ERR_DRC_BSM_HFP_MSG() traceNo=%d, errorId=%d, msg=%s, para1=%d, para2=%d", 
		TRACE_NO_LIB_BSM_HFP, errorId, msg, para1, para2);
	return errorMessageRecorderBsm(TRACE_NO_LIB_BSM_HFP, errorId, msg, para1, para2);
}
int ERR_DRC_BSM_PBAP_MSG(int errorId, const char *msg, int para1, int para2){
	BSM_RECORDER_LOGD("ERR_DRC_BSM_PBAP_MSG() traceNo=%d, errorId=%d, msg=%s, para1=%d, para2=%d", 
		TRACE_NO_LIB_BSM_PBAP, errorId, msg, para1, para2);
	return errorMessageRecorderBsm(TRACE_NO_LIB_BSM_PBAP, errorId, msg, para1, para2);
}
#endif
/** BSMブロックソフトエラーメッセージ用ログ出力 */
#if 0
static int errorMessageRecorderBsm(int traceNo, int errorId, const char *msg, int para1, int para2)
{
	int fd; 
	int group_id = GROUP_ID_TRACE_ERROR;	//  出力先グループID
	int ret;
 
    struct lites_trace_header trc_hdr;		// ドラレコ用ヘッダー
 	struct lites_trace_format trc_fmt;		// ドラレコ用フォーマット
	struct drc_bsm_error_msg_log  logBuf;	    // エラーログ格納バッファ
	memset(&trc_fmt, 0x0, sizeof(trc_fmt));
	memset(&trc_hdr, 0x0, sizeof(trc_hdr));

	fd = open("/dev/lites", O_RDWR);	    // <-libipasのAPIを使用するために必須
	if(fd < 0){
		BSM_RECORDER_LOGE("open()[errorRecorderMassageBSM_LIB] error. fd=%d", fd);
		return -1;
	}

	logBuf.err_id = (unsigned int)errorId;
	logBuf.msg    = (unsigned char *)msg;
	logBuf.param1 = (unsigned int)para1;
	logBuf.param2 = (unsigned int)para2;

    trc_fmt.trc_header = &trc_hdr;		// ドラレコ用ヘッダー格納
	trc_fmt.count = sizeof(logBuf);		// 保存ログバッファのサイズ
	trc_fmt.buf = &logBuf;				// 保存ログバッファ
    trc_fmt.trc_header->level = LOG_LEVEL_ERROR;			// エラーログレベル
    trc_fmt.trc_header->trace_no = traceNo;					// トレース番号(星取表のNo)
    trc_fmt.trc_header->trace_id = TRACE_ID_ERROR;	    	// トレースID(機能毎の値)
    trc_fmt.trc_header->format_id = FORMAT_ID_ERROR_MSG_LOG;	// フォーマットID (bufのフォーマット識別子)

    ret = lites_trace_write(fd, group_id, &trc_fmt);	    // <-ログ出力時に必須
	if (ret < 0) {
		BSM_RECORDER_LOGE("lites_trace_write()[errorRecorderMassageBSM_LIB] error ret=%d", ret);
	}

    close(fd);

    return ret;
}
#endif
