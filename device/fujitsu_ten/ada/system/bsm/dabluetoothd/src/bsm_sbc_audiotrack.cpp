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

#include <media/AudioTrack.h>
#include <ada/log.h>
#include <cutils/log.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "include/linux/lites_trace.h"

#define USE_STREAMING_MODE
//#define STREAM_TYPE     AUDIO_STREAM_MUSIC
#define STREAM_TYPE     AUDIO_STREAM_ADA_NORMAL
#define AUDIO_FORMAT    AUDIO_FORMAT_PCM_16_BIT
#define CHANNEL_MASK    3
//#define FRAME_COUNT     (64000 * sizeof(unsigned char))
#define FRAME_COUNT     (0)

#define SBC_DEBUG_ERROR(fmt, ...) \
		((void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_ERROR, "bsm_sbc", __FUNCTION__, fmt, ##__VA_ARGS__))
#define SBC_DEBUG_WARNING(fmt, ...) \
		((void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_WARN, "bsm_sbc", __FUNCTION__, fmt, ##__VA_ARGS__))
#define SBC_DEBUG_LOGD(fmt, ...) \
		((void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_DEBUG, "bsm_sbc", __FUNCTION__, fmt, ##__VA_ARGS__))

//ドラレコ対応
#define GROUP_ID_EVENT_LOG     REGION_EVENT_LOG
#define GROUP_ID_TRACE_ERROR   REGION_TRACE_ERROR
#define TRACE_ID_BT_BLOCK_IF   REGION_EVENT_BT_BLOCK_IF
#define TRACE_ID_ERROR         0
#define TRACE_NO_LIB_A2DP	  (LITES_LIB_LAYER + 34)
#define LOG_LEVEL_EVENT   	   3
#define LOG_LEVEL_ERROR   	   3
#define FORMAT_ID_ERROR_LOG    1
#define FORMAT_ID_BSM_SBC_AUDIOTRACK 1210
//エラーID
//BTMエラー
#define ERROR_BSM_SBC_AUDIOTRACK	0x3F01

//付加情報
/** AUDIOTRACK OPENを示す. */
#define BSM_SBC_AUDIOTRACK_OPEN		0x11
/** AUDIOTRACK STOPを示す. */
#define BSM_SBC_AUDIOTRACK_STOP		0x12
/** AUDIOTRACK PAUSEを示す. */
#define BSM_SBC_AUDIOTRACK_PAUSE	0x13
/** AUDIOTRACK CLOSEを示す. */
#define BSM_SBC_AUDIOTRACK_CLOSE	0x14
/** AUDIOTRACK OPEN(INTERRUPT)を示す. */
#define BSM_SBC_AUDIOTRACK__INTERRUPTOPEN	0x15
/** AUDIOTRACK NEWを示す. */
#define BSM_SBC_AUDIOTRACK_NEW		0x16


typedef struct drc_btm_audiotrack_event_log  {
	unsigned int event;     /** event 種別 */
} DRC_BTM_AUDIOTRACK_EVENT_LOG;

typedef struct drc_btm_error_log  {
	unsigned int err_id;     /** エラーＩＤ */
	unsigned int param1;     /** 付加情報１ */
	unsigned int param2;     /** 付加情報2	*/
	unsigned int param3;     /** 付加情報3 	*/
	unsigned int param4;     /** 付加情報4 	*/
} DRC_BTM_ERROR_LOG;


namespace android {

AudioTrack* lpTrack;

/** BSM_AudioTrackイベントログ出力 */
static int EVENT_DRC_BSM_SBC_AUDIOTRACK(int formatId, int event)
{
	int fd; 
	int group_id = GROUP_ID_EVENT_LOG;				//  出力先グループID
    int ret;
	struct drc_btm_audiotrack_event_log  logBuf;	// 機能毎ログ格納バッファ

    struct lites_trace_header trc_hdr;				// ドラレコ用ヘッダー
 	struct lites_trace_format trc_fmt;				// ドラレコ用フォーマット
	memset(&trc_fmt, 0x0, sizeof(trc_fmt));
	memset(&trc_hdr, 0x0, sizeof(trc_hdr));
	memset(&logBuf, 0x0, sizeof(logBuf));

	fd = open("/dev/lites", O_RDWR);	// <-libipasのAPIを使用するために必須
	if(fd < 0){
		SBC_DEBUG_ERROR("open()[EVENT_DRC_BSM_SBC_AUDIOTRACK] error. fd=%d", fd);
		return -1;
	}

	logBuf.event = (unsigned int)event;
    trc_fmt.trc_header = &trc_hdr;				// ドラレコ用ヘッダー格納
	trc_fmt.count = sizeof(logBuf);				// 保存ログバッファのサイズ
	trc_fmt.buf = &logBuf;						// 保存ログバッファ
    trc_fmt.trc_header->level = LOG_LEVEL_EVENT;	// ログレベル
    trc_fmt.trc_header->trace_no = TRACE_NO_LIB_A2DP;		// トレース番号(星取表のNo)
    trc_fmt.trc_header->trace_id = TRACE_ID_BT_BLOCK_IF;	// トレースID(機能毎の値)
    trc_fmt.trc_header->format_id = formatId;				// フォーマットID (bufのフォーマット識別子)

	SBC_DEBUG_LOGD("EVENT_DRC_BSM_SBC_AUDIOTRACK() traceNo=%d, formatId=%d, event=%d", TRACE_NO_LIB_A2DP, formatId, event);
    ret = lites_trace_write(fd, group_id, &trc_fmt);	//  ログ出力
	if (ret < 0) {
		SBC_DEBUG_ERROR("lites_trace_write()[EVENT_DRC_BSM_SBC_AUDIOTRACK] error ret=%d", ret);
	}

    close(fd);

    return ret;
}

/** BSM AudioTrackソフトエラー用ログ出力 */
static int ERR_DRC_BSM_SBC_AUDIOTRACK(int errorId, int para1, int para2, int para3, int para4)
{
	int fd; 
	int group_id = GROUP_ID_TRACE_ERROR;	//  出力先グループID
	int ret;
 
    struct lites_trace_header trc_hdr;		// ドラレコ用ヘッダー
 	struct lites_trace_format trc_fmt;		// ドラレコ用フォーマット
	struct drc_btm_error_log  logBuf;	    // エラーログ格納バッファ
	memset(&trc_fmt, 0x0, sizeof(trc_fmt));
	memset(&trc_hdr, 0x0, sizeof(trc_hdr));
	memset(&logBuf, 0x0, sizeof(logBuf));

	fd = open("/dev/lites", O_RDWR);	    // <-libipasのAPIを使用するために必須
	if(fd < 0){
		SBC_DEBUG_ERROR("open()[ERR_DRC_BSM_SBC_AUDIOTRACK] error. fd=%d", fd);
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
    trc_fmt.trc_header->trace_no = TRACE_NO_LIB_A2DP;					// トレース番号(星取表のNo)
    trc_fmt.trc_header->trace_id = TRACE_ID_ERROR;	    	// トレースID(機能毎の値)
    trc_fmt.trc_header->format_id = FORMAT_ID_ERROR_LOG;	// フォーマットID (bufのフォーマット識別子)

	SBC_DEBUG_LOGD("ERR_DRC_BSM_SBC_AUDIOTRACK() traceNo=%d, errorId=%d, para1=%d, para2=%d, para3=%d, para4=%d", 
		TRACE_NO_LIB_A2DP, errorId, para1, para2, para3, para4);

    ret = lites_trace_write(fd, group_id, &trc_fmt);	    // ログ出力
	if (ret < 0) {
		SBC_DEBUG_ERROR("lites_trace_write()[ERR_DRC_BSM_SBC_AUDIOTRACK] error ret=%d", ret);
	}

    close(fd);

    return ret;
}


#ifdef USE_STREAMING_MODE
AudioTrack* playbackInit(uint32_t sampleRate, uint32_t channels, unsigned char streamingMode)
{
    //First we create an instance of AudioTrack class
    if(lpTrack == NULL){
    	EVENT_DRC_BSM_SBC_AUDIOTRACK(FORMAT_ID_BSM_SBC_AUDIOTRACK, BSM_SBC_AUDIOTRACK_NEW);
	    lpTrack = new AudioTrack();
   	 	if(lpTrack == NULL){
        	// Can not create AudioTrack Instance
			SBC_DEBUG_ERROR("new AudioTrack(). ERROR");
			ERR_DRC_BSM_SBC_AUDIOTRACK(ERROR_BSM_SBC_AUDIOTRACK, 0, 0, 0, __LINE__);
   	 		return NULL;
   	 	}
        
    	//Successfully created an instance
    	status_t status = lpTrack->set(streamingMode,
                          sampleRate,
                          AUDIO_FORMAT,
                          CHANNEL_MASK,
                          FRAME_COUNT,
                          0, //uint32_t flags
                          0, //callback_t cbf
                          0, //user
                          0, //notificationFrames
                          0, //sp<IMemory>& sharedBuffer
                          false,0); //threadCanCallJava

		if(status != NO_ERROR){
			SBC_DEBUG_ERROR("lpTrack->set(). ERROR");
			ERR_DRC_BSM_SBC_AUDIOTRACK(ERROR_BSM_SBC_AUDIOTRACK, status, 0, 0, __LINE__);
 			return NULL;		
		}
	}
		
    lpTrack->start();
    return lpTrack;
}


extern "C" int openAudioTrack(uint32_t sampleRate, uint32_t channels, unsigned char streamingMode)
{
	if( streamingMode )
	{
		SBC_DEBUG_WARNING("openAudioTrack(). AUDIO_STREAM_ADA_INTERRUPT");
		EVENT_DRC_BSM_SBC_AUDIOTRACK(FORMAT_ID_BSM_SBC_AUDIOTRACK, BSM_SBC_AUDIOTRACK__INTERRUPTOPEN);
	    lpTrack = playbackInit(sampleRate,channels,AUDIO_STREAM_ADA_INTERRUPT);
	}
	else
	{
		SBC_DEBUG_WARNING("openAudioTrack(). AUDIO_STREAM_ADA_NORMAL");
		EVENT_DRC_BSM_SBC_AUDIOTRACK(FORMAT_ID_BSM_SBC_AUDIOTRACK, BSM_SBC_AUDIOTRACK_OPEN);
	    lpTrack = playbackInit(sampleRate,channels,AUDIO_STREAM_ADA_NORMAL);
	}
    if(lpTrack == NULL){
        // Init AudioTrack failed
        return 1;
    }
    return 0;
}
#else  /* USE_STREAMING_MODE */
AudioTrack* playbackInit(uint32_t sampleRate, uint32_t channels)
{
    //First we create an instance of AudioTrack class
    if(lpTrack == NULL){
    	EVENT_DRC_BSM_SBC_AUDIOTRACK(FORMAT_ID_BSM_SBC_AUDIOTRACK, BSM_SBC_AUDIOTRACK_NEW);
	    lpTrack = new AudioTrack();
   	 	if(lpTrack == NULL){
        	// Can not create AudioTrack Instance
			SBC_DEBUG_ERROR("new AudioTrack(). ERROR");
			ERR_DRC_BSM_SBC_AUDIOTRACK(ERROR_BSM_SBC_AUDIOTRACK, 0, 0, 0, __LINE__);
   	 		return NULL;
   	 	}
        
    	//Successfully created an instance
    	status_t status = lpTrack->set(STREAM_TYPE,
                          sampleRate,
                          AUDIO_FORMAT,
                          CHANNEL_MASK,
                          FRAME_COUNT,
                          0, //uint32_t flags
                          0, //callback_t cbf
                          0, //user
                          0, //notificationFrames
                          0, //sp<IMemory>& sharedBuffer
                          false,0); //threadCanCallJava

		if(status != NO_ERROR){
			SBC_DEBUG_ERROR("lpTrack->set(). ERROR");
			ERR_DRC_BSM_SBC_AUDIOTRACK(ERROR_BSM_SBC_AUDIOTRACK, status, 0, 0, __LINE__);
 			return NULL;		
		}
	}
		
    lpTrack->start();
    return lpTrack;
}


extern "C" int openAudioTrack(uint32_t sampleRate, uint32_t channels)
{
	EVENT_DRC_BSM_SBC_AUDIOTRACK(FORMAT_ID_BSM_SBC_AUDIOTRACK, BSM_SBC_AUDIOTRACK_OPEN);
    lpTrack = playbackInit(sampleRate,channels);
    if(lpTrack == NULL){
        // Init AudioTrack failed
        return 1;
    }
    return 0;
}
#endif /* USE_STREAMING_MODE */


extern "C" void stopAudioTrack()
{
	EVENT_DRC_BSM_SBC_AUDIOTRACK(FORMAT_ID_BSM_SBC_AUDIOTRACK, BSM_SBC_AUDIOTRACK_STOP);
	if(lpTrack != NULL){
		lpTrack->stop();
	}else{
		ERR_DRC_BSM_SBC_AUDIOTRACK(ERROR_BSM_SBC_AUDIOTRACK, 0, 0, 0, __LINE__);
	}
}


extern "C" void pauseAudioTrack()
{
	EVENT_DRC_BSM_SBC_AUDIOTRACK(FORMAT_ID_BSM_SBC_AUDIOTRACK, BSM_SBC_AUDIOTRACK_PAUSE);
	if(lpTrack != NULL){
    	lpTrack->pause();
	}else{
		ERR_DRC_BSM_SBC_AUDIOTRACK(ERROR_BSM_SBC_AUDIOTRACK, 0, 0, 0, __LINE__);
	}
}


extern "C" void closeAudioTrack()
{
	EVENT_DRC_BSM_SBC_AUDIOTRACK(FORMAT_ID_BSM_SBC_AUDIOTRACK, BSM_SBC_AUDIOTRACK_CLOSE);
    if(lpTrack != NULL){
    	lpTrack->stop();
        delete lpTrack;
        lpTrack = NULL;
    }else{
		ERR_DRC_BSM_SBC_AUDIOTRACK(ERROR_BSM_SBC_AUDIOTRACK, 0, 0, 0, __LINE__);
	}
}


extern "C" int writeAudioTrack(unsigned char* outputBuffer,int nSamples)
{
	int sts = 0;
	if(lpTrack != NULL){
    	sts = lpTrack->write(outputBuffer, nSamples * sizeof(unsigned char));
	}else{
		ERR_DRC_BSM_SBC_AUDIOTRACK(ERROR_BSM_SBC_AUDIOTRACK, 0, 0, 0, __LINE__);
	}
    return sts;
}

}
