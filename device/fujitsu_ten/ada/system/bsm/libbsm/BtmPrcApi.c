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

/****************************************************************************
*																			*
*		シリーズ名称	: ＦＴ－Ａ－ＤＡシリーズ							*
*		マイコン名称	: 共通												*
*		ブロック名称	: データ管理部										*
*		DESCRIPTION		: ＢＴＭプロセスＡＰＩ								*
*																			*
*		ファイル名称	: BtmPrcApi.c										*
*		作	 成	  者	: □□	□											*
*		作　 成　 日	: 2012/05/12	<Ver. 0.01.00>						*
*		改　 訂　 日	: 2012/07/11	<Ver. 0.10.00>						*
*																			*
*		『システムコール』													*
*			fs_bsm_Start	  .......	処理								*
*			tsk_btm_IpcRcvMsg .......	通知設定							*
*			fs_bsm_Send		  .......	データサービス情報取得処理			*
*																			*
****************************************************************************/
#define LOG_TAG		"libbsm"

/********************************
*		Include Files			*
********************************/
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<sched.h>
#include	<signal.h>
#include	<errno.h>
#include	<pthread.h>
#include	<semaphore.h>
#include	<limits.h>
#include	<stdint.h>
#include	<sys/socket.h>
#include	<sys/un.h>

#include	<ada/log.h>
#include	<cutils/log.h>

#include	"cmn_types.h"
#include	"linux_if.h"
#include	"BsmUpperIf.h"
#include	"PfIfMsg.h"
#include 	"include/linux/lites_trace.h"

/********************************
*		Local Defines			*
********************************/
#define		THREAD_PRIORITY		11			/* スレッド優先度				*/

/********************************
*		Local Buffers			*
********************************/
ULONG		stk_Tsk[1024*4/sizeof(ULONG)];	/* BTM I/Fタスク スタック領域	*/
static const UCHAR gBsmErrorData[2] = {0x00, 0x1C};		/* BSM 機能停止通知	*/

/********************************
*		Prototype				*
********************************/
static	BSM_CB_FP			gpfCallBack	= NULL;
static	VOID *				gpUser		= NULL;
static	BOOL8				gBsmStarted	= FALSE;
static	int					gSocketFd	= 0;
static	BSM_SOCKET_HEADER	gSocketHdr;
void	tsk_btm_IpcRcvMsg(void);
INT		bsmIfCtlPrcInit(INT, INT, UCHAR *);

/********************************
*		Macro					*
********************************/
#define ADA_BT_LOGE(fmt, ...) \
		(void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_ERROR, LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)
#define ADA_BT_LOGW(fmt, ...) \
		(void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_WARN, LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)
#define ADA_BT_LOGI(fmt, ...) \
		(void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_INFO, LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)
#define ADA_BT_LOGD(fmt, ...) \
		(void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_DEBUG, LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)
#define ADA_BT_LOGV(fmt, ...) \
		(void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_VERBOSE, LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)

//ドラレコ対応
#define GROUP_ID_EVENT_LOG 	   REGION_EVENT_LOG
#define GROUP_ID_TRACE_ERROR   REGION_TRACE_ERROR
#define TRACE_ID_BT_BLOCK_IF   REGION_EVENT_BT_BLOCK_IF
#define TRACE_ID_ERROR         0
#define TRACE_NO_LIB_HFP	  (LITES_LIB_LAYER + 36)
#define LOG_LEVEL_EVENT   	   3
#define LOG_LEVEL_ERROR   	   3
#define FORMAT_ID_ERROR_LOG    1
#define FORMAT_ID_BTM_EVENT    1200

//エラーID
//BTMエラー
#define ERROR_BSM_BTM		   0x0F01

//付加情報
/** BSM、BT スタックを起動を示す. */
#define BSM_BTM_START				0x11
/** BSM、BT スタックを停止を示す. */
#define BSM_BTM_STOP				0x12
/** BSM コマンド送信を示す. */
#define BSM_BTM_SEND 				0x13
/** BSM コールバック設定を示す. */
#define BSM_BTM_SETCB 				0x14

typedef struct drc_bt_btm_event_log  {
	unsigned int event;     /** event 種別 */
} DRC_BT_BTM_EVENT_LOG;

typedef struct drc_btm_error_log  {
	unsigned int err_id;     /** エラーＩＤ */
	unsigned int param1;     /** 付加情報１ */
	unsigned int param2;     /** 付加情報2	*/
	unsigned int param3;     /** 付加情報3 	*/
	unsigned int param4;     /** 付加情報4 	*/
} DRC_BTM_ERROR_LOG;

static int ERR_DRC_BSM_BTM(int errorId, int para1, int para2, int para3, int para4);
static int EVENT_DRC_BSM_BTM(int formatId, int param);

/*==========================================================================*/
/*							コールバック									*/
/*==========================================================================*/
INT fs_bsm_SetCallback( BSM_CB_FP callback_func, VOID * user )
{
	gpfCallBack = callback_func;
	gpUser		= user;

	EVENT_DRC_BSM_BTM(FORMAT_ID_BTM_EVENT, BSM_BTM_SETCB);
	
	return BSM_OK;
}

/*==========================================================================*/
/*							システムコール									*/
/*==========================================================================*/
/**********************************************************************************/
/*  [SYMBOL]                                                                      */
/*      fs_bsm_Start                                                              */
/*  [DESCRIPTION]                                                                 */
/*      上位ソフトより BSM、BT スタックを起動する。                               */
/*  [TYPE]                                                                        */
/*      INT                                                                       */
/*  [PARAMETER]                                                                   */
/*      iBsmMode       : I   動作モード                                           */
/*      bTestParamLen  : I   テストパラメタ長                                     */
/*      pbTestParam    : I   テストパラメタ                                       */
/*  [RETURN]                                                                      */
/*      BSM_OK:    正常終了                                                       */
/*      BSM_ERROR: 異常終了                                                       */
/*  [NOTE]                                                                        */
/*      本来BSMに実装されているI/Fを使って、                                      */
/*      BSM起動のためのIPC送信を行う。                                            */
/*      また、IPC受信用スレッドの生成もここで行う。                               */
/**********************************************************************************/
INT fs_bsm_Start( INT iBsmMode, INT bTestParamLen, UCHAR *pbTestParam )
{
	INT	res = BSM_OK;

	EVENT_DRC_BSM_BTM(FORMAT_ID_BTM_EVENT, BSM_BTM_START);
	
	/* modeパラメタチェック */
	if( iBsmMode > BSM_MODE_RF_LOGO_TEST
	||  (  iBsmMode <  BSM_MODE_NORMAL
		&& iBsmMode != BSM_MODE_SHUTDOWN ) )
	{
		ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
		return BSM_ERROR;
	}

	/* 簡易接続モードの場合はパラメタチェックする */
	if( iBsmMode == BSM_MODE_HF_TEST
	||  iBsmMode == BSM_MODE_AV_TEST )
	{
		if( bTestParamLen > BSM_D_TEST_DEV_NAME_MAX )
		{
			ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
			return BSM_ERROR;
		}
	}

	/* 初回の起動のみ実行 */
	if( gBsmStarted == FALSE )
	{
		/*======================================================================*/
		/*	スレッド起動														*/
		/*======================================================================*/
		pthread_t			tid;
		pthread_attr_t		attr;
		struct sched_param	param;
		int					ret;
		struct sockaddr_un	addr;

		/*----------------------------------------------------------------------*/
		/*	プロセス間通信用SOCKET生成											*/
		/*----------------------------------------------------------------------*/
		gSocketFd = socket(PF_UNIX, SOCK_STREAM, 0);
		if (gSocketFd < 0)
		{
			perror("socket");
			/* エラーログ出力 */
			ADA_BT_LOGE("socket create error!!");
			ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
			return BSM_ERROR;
		}

		/*----------------------------------------------------------------------*/
		/*	プロセス間通信用SERVER SOCKETに接続									*/
		/*----------------------------------------------------------------------*/
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strlcpy(addr.sun_path, BSM_SOCKET_PATH, sizeof(addr.sun_path));

		if (connect(gSocketFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		{
			perror("connect");
			close(gSocketFd);
			gSocketFd = 0;
			/* エラーログ出力 */
			ADA_BT_LOGE("socket connect error!!");
			ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
			return BSM_ERROR;
		}

		/*----------------------------------------------------------------------*/
		/*	スレッド属性初期化													*/
		/*----------------------------------------------------------------------*/
		pthread_attr_init(&attr);

		/*----------------------------------------------------------------------*/
		/*	スケジューリング・ポリシーを「ＦＩＦＯ」に設定						*/
		/*----------------------------------------------------------------------*/
		pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

		/*----------------------------------------------------------------------*/
		/*	静的優先度を設定													*/
		/*----------------------------------------------------------------------*/
		param.sched_priority = THREAD_PRIORITY;
		pthread_attr_setschedparam(&attr, &param);

		/*----------------------------------------------------------------------*/
		/*	スタック設定														*/
		/*----------------------------------------------------------------------*/
		ret = pthread_attr_setstack(&attr, &stk_Tsk[0], sizeof(stk_Tsk));
		if (ret < 0)
		{
			/* エラーログ出力 */
			ADA_BT_LOGE("pthread set stack error!! ret=%d", ret);
			ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
			pthread_attr_destroy(&attr);
			return BSM_ERROR;
		}

		/*----------------------------------------------------------------------*/
		/*	スレッド生成														*/
		/*----------------------------------------------------------------------*/
		ret = pthread_create(&tid, &attr, (void*)tsk_btm_IpcRcvMsg, NULL);
		/* TODO:tid保存＆スレッド削除を考える */

		/*----------------------------------------------------------------------*/
		/*	起動失敗															*/
		/*----------------------------------------------------------------------*/
		if (ret != 0)
		{
			close(gSocketFd);
			gSocketFd = 0;
			/* エラーログ出力 */
			ADA_BT_LOGE("pthread create error!! ret=%d", ret);
			ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
			pthread_attr_destroy(&attr);
			return BSM_ERROR;
		}
	}

	/* 初期化済みを設定 */
	gBsmStarted = TRUE;

	/*----------------------------------------------------------------------*/
	/*	初期化関数の呼び出し												*/
	/*----------------------------------------------------------------------*/
	res = bsmIfCtlPrcInit(iBsmMode, bTestParamLen, pbTestParam);

	return res;
}

/**********************************************************************************/
/*  [SYMBOL]                                                                      */
/*      fs_bsm_Stop                                                               */
/*  [DESCRIPTION]                                                                 */
/*      上位ソフトより BSM、BT スタックを停止する。                               */
/*  [TYPE]                                                                        */
/*      INT                                                                       */
/*  [PARAMETER]                                                                   */
/*      None                                                                      */
/*  [RETURN]                                                                      */
/*      BSM_OK:    正常終了                                                       */
/*      BSM_ERROR: 異常終了                                                       */
/*  [NOTE]                                                                        */
/*      本来BSMに実装されているI/Fを使って、                                      */
/*      BSM停止のためのIPC送信を行う。                                            */
/**********************************************************************************/
INT fs_bsm_Stop( )
{
	UCHAR				pbDf[sizeof(BSM_SOCKET_HEADER)];

	/*----------------------------------------------------------------------*/
	/*	プロセス間コマンドの作成											*/
	/*----------------------------------------------------------------------*/
	BSM_SOCKET_HEADER	*sockethdr;
	sockethdr = (BSM_SOCKET_HEADER *)pbDf;
	sockethdr->event = BTM_EVE_STOP_REQ;		/* イベントコード			*/
	sockethdr->size  = 0;						/* 付加データサイズ			*/

	EVENT_DRC_BSM_BTM(FORMAT_ID_BTM_EVENT, BSM_BTM_STOP);
	
	if (gSocketFd <= 0)
	{
		/* エラーログ出力 */
		ADA_BT_LOGE("pthread send STOP_REQ error!! non socket.");
		ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
		return BSM_ERROR;
	}

	/*----------------------------------------------------------------------*/
	/*	ＢＴＭ プロセスへコマンド送信										*/
	/*----------------------------------------------------------------------*/
	if (write(gSocketFd, pbDf, sizeof(BSM_SOCKET_HEADER)) < 0)
	{
		/* エラーログ出力 */
		ADA_BT_LOGE("pthread send STOP_REQ error!!");
		ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
		return BSM_ERROR;
	}

	return BSM_OK;
}

/****************************************************************************
*																			*
*		SYMBOL		: tsk_btm_IpcRcvMsg										*
*																			*
*		DESCRIPTION	: BTM I/Fタスクメイン処理								*
*																			*
*		PARAMETER	: IN  : None											*
*																			*
*					  OUT : None											*
*																			*
*					  RET : None											*
*																			*
****************************************************************************/
void tsk_btm_IpcRcvMsg( void )
{
	int					size;
	UCHAR *				pbData;

	while (1)
	{
		memset(&gSocketHdr, 0, sizeof(BSM_SOCKET_HEADER));

		/*------------------------------------------------------------------*/
		/*	メッセージ受信待ち（ソケットヘッダサイズ分読込）				*/
		/*------------------------------------------------------------------*/
		size = read(gSocketFd, &gSocketHdr, sizeof(BSM_SOCKET_HEADER));

		if ((size == sizeof(BSM_SOCKET_HEADER)) && (gSocketHdr.size > 0))
		{
			/*--------------------------------------------------------------*/
			/*	ソケットデータ用ヒープ取得									*/
			/*--------------------------------------------------------------*/
			pbData = malloc(gSocketHdr.size);
			if (pbData != NULL)
			{
				/*----------------------------------------------------------*/
				/*	メッセージ受信（ソケット本体サイズ分読込）				*/
				/*----------------------------------------------------------*/
				size = read(gSocketFd, pbData, gSocketHdr.size);

				if (size == gSocketHdr.size)
				{
					/*------------------------------------------------------*/
					/*	イベントIDで分岐									*/
					/*------------------------------------------------------*/
					switch(gSocketHdr.event)
					{
						case BTM_EVE_MESSAGE_RSP:		/* コマンド受信		*/
							if(gpfCallBack != NULL)
							{
								/* 登録コールバックにデータを引き渡し */
								gpfCallBack( gpUser, size, pbData );
							}
							break;

						default:						/* その他			*/
							/*----------------------------------------------*/
							/*	イベントIDエラー							*/
							/*----------------------------------------------*/
							/* エラーログ出力 */
							ADA_BT_LOGE("recive unknow event!! event=%d", gSocketHdr.event);
							ERR_DRC_BSM_BTM(ERROR_BSM_BTM, gSocketHdr.event, 0, 0, __LINE__);
							break;
					}
				}
				else
				{
					/* エラーログ出力 */
					ADA_BT_LOGE("pthread data socket read size error!! size=%d", size);
					ERR_DRC_BSM_BTM(ERROR_BSM_BTM, size, 0, 0, __LINE__);
				}

				/*----------------------------------------------------------*/
				/*	ソケットデータ用ヒープ解放								*/
				/*----------------------------------------------------------*/
				free(pbData);
				pbData = NULL;
			}
			else
			{
				/* エラーログ出力 */
				ADA_BT_LOGE("pthread cannot malloc memory for recive!!");
				ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
				break;
			}
		}
		else
		{
			/* エラーログ出力 */
			ADA_BT_LOGE("pthread header socket read size error!! size=%d", size);
			ERR_DRC_BSM_BTM(ERROR_BSM_BTM, size, 0, 0, __LINE__);
			break;
		}
	}

	/*----------------------------------------------------------------------*/
	/*	スレッドエラー終了処理												*/
	/*----------------------------------------------------------------------*/
	close(gSocketFd);
	gSocketFd = 0;
	gBsmStarted = FALSE;

	if(gpfCallBack != NULL)
	{
		/* 登録コールバックにBSM機能停止通知を渡す */
		gpfCallBack( gpUser, sizeof(gBsmErrorData), gBsmErrorData );
	}

	/* スレッドを終了 */
	pthread_exit(0);
}

/****************************************************************************
*																			*
*		SYMBOL		: bsmIfCtlPrcInit										*
*																			*
*		DESCRIPTION	: BSM起動要求											*
*																			*
*		PARAMETER	: IN  : iMode --- 起動モード							*
*																			*
*					  OUT : None											*
*																			*
*					  RET : ret --- 処理結果								*
*										BSM_OK		：正常終了				*
*										BSM_ERROR	：異常終了				*
****************************************************************************/
INT bsmIfCtlPrcInit( INT iMode, INT bTestParamLen, UCHAR *pbTestParam )
{
	UCHAR				pbDf[38+sizeof(BSM_SOCKET_HEADER)];

	/*----------------------------------------------------------------------*/
	/*	プロセス間コマンドの作成											*/
	/*----------------------------------------------------------------------*/
	BSM_SOCKET_HEADER	*sockethdr;
	sockethdr = (BSM_SOCKET_HEADER *)pbDf;
	sockethdr->event = BTM_EVE_INIT_REQ;		/* イベントコード			*/
	sockethdr->size  = 38;						/* 付加データサイズ			*/

	memcpy( &pbDf[0+sizeof(BSM_SOCKET_HEADER)], &iMode, sizeof(INT) );
	memcpy( &pbDf[4+sizeof(BSM_SOCKET_HEADER)], &bTestParamLen, sizeof(INT) );
	if( pbTestParam != NULL )
	{
		memcpy( &pbDf[8+sizeof(BSM_SOCKET_HEADER)], pbTestParam, BSM_D_TEST_DEV_NAME_MAX );
	}

	/*----------------------------------------------------------------------*/
	/*	ＢＴＭ プロセスへコマンド送信										*/
	/*----------------------------------------------------------------------*/
	if (write(gSocketFd, pbDf, 38+sizeof(BSM_SOCKET_HEADER)) < 0)
	{
		/* エラーログ出力 */
		ADA_BT_LOGE("pthread send INIT_REQ error!!");
		ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
		return BSM_ERROR;
	}

	return BSM_OK;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_bsm_Send											*
*																			*
*		DESCRIPTION	: BSM コマンド送信										*
*																			*
*		PARAMETER	: IN  : iDl --- 送信データ長							*
*							pDf --- 送信データポインタ						*
*																			*
*					  OUT : None											*
*																			*
*					  RET : ret --- 処理結果								*
*										BSM_OK		：正常終了				*
*										BSM_ERROR	：異常終了				*
****************************************************************************/
INT fs_bsm_Send( INT iDl, UCHAR* pDf )
{
	BSM_SOCKET_HEADER *	pSocketHdr;
	UCHAR *				pbDf;

	EVENT_DRC_BSM_BTM(FORMAT_ID_BTM_EVENT, BSM_BTM_SEND);
	
	/*----------------------------------------------------------------------*/
	/*	プロセス間コマンド送信バッファを生成								*/
	/*----------------------------------------------------------------------*/
	pbDf = malloc(iDl + sizeof(BSM_SOCKET_HEADER));
	if (pbDf == NULL)
	{
		/* エラーログ出力 */
		ADA_BT_LOGE("pthread cannot malloc memory at sending!!");
		ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
		return BSM_ERROR;
	}

	/*----------------------------------------------------------------------*/
	/*	プロセス間コマンドの作成											*/
	/*----------------------------------------------------------------------*/
	pSocketHdr = (BSM_SOCKET_HEADER *)pbDf;
	pSocketHdr->event = BTM_EVE_MESSAGE_REQ;	/* イベントコード			*/
	pSocketHdr->size  = iDl;					/* 付加データサイズ			*/
	memcpy(&pbDf[sizeof(BSM_SOCKET_HEADER)], pDf, iDl );

	if (gSocketFd <= 0)
	{
		free(pbDf);
		/* エラーログ出力 */
		ADA_BT_LOGE("pthread send error!! non socket.");
		ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
		return BSM_ERROR;
	}

	/*----------------------------------------------------------------------*/
	/*	ＢＴＭ プロセスへコマンド送信										*/
	/*----------------------------------------------------------------------*/
	if (write(gSocketFd, pbDf, iDl+sizeof(BSM_SOCKET_HEADER)) < 0)
	{
		free(pbDf);
		/* エラーログ出力 */
		ADA_BT_LOGE("pthread send error!!");
		ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
		return BSM_ERROR;
	}

	free(pbDf);

	return BSM_OK;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      Bsm_Start                                                            */
/*  [DESCRIPTION]                                                            */
/*      BSMタスク起動                                                        */
/*  [TYPE]                                                                   */
/*      INT                                                                  */
/*  [PARAMETER]                                                              */
/*      INT bsm_mode                                                         */
/*      BSM_TEST_PARAM * test_param                                          */
/*  [RETURN]                                                                 */
/*      タスク起動結果                                                       */
/*  [NOTE]                                                                   */
/*                                                                           */
/*****************************************************************************/
INT Bsm_Start( INT bsm_mode, BSM_TEST_PARAM * test_param )
{
	UCHAR	pbTestParam[BSM_D_TEST_DEV_NAME_MAX];
	INT		bTestParamLen;
	INT		bNewBsmMode;

	memset( pbTestParam, BSM_D_COMMON_INIT_VALUE, sizeof( pbTestParam ) );

	if( bsm_mode == BSM_MODE_OLD_NORMAL )
	{
		bNewBsmMode = BSM_MODE_NORMAL;
		bTestParamLen = 0;
	}
	else if( bsm_mode == BSM_MODE_OLD_HCI )
	{
		bNewBsmMode = BSM_MODE_HCI;
		bTestParamLen = 0;
	}
	else if( bsm_mode == BSM_MODE_OLD_TEST )
	{
		if( test_param == (BSM_TEST_PARAM*)NULL )
		{
			ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
			return BSM_ERROR;
		}

		/* テストモード種別がHFP簡易接続 */
		if( test_param->test_mode == BSM_D_TEST_HFP_CON )
		{
			bNewBsmMode = BSM_MODE_HF_TEST;
			bTestParamLen = test_param->device_name_len;
			memcpy( pbTestParam, test_param->device_name, bTestParamLen );
		}
		/* テストモード種別がAVP簡易接続 */
		else if( test_param->test_mode == BSM_D_TEST_AVP_CON )
		{
			bNewBsmMode = BSM_MODE_AV_TEST;
			bTestParamLen = test_param->device_name_len;
			memcpy( pbTestParam, test_param->device_name, bTestParamLen );
		}
		else
		{
			ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
			return BSM_ERROR;
		}
	}
	else
	{
		ERR_DRC_BSM_BTM(ERROR_BSM_BTM, 0, 0, 0, __LINE__);
		return BSM_ERROR;
	}

	return fs_bsm_Start( bNewBsmMode, bTestParamLen, pbTestParam );
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      Bsm_Stop                                                             */
/*  [DESCRIPTION]                                                            */
/*      BSMタスク停止                                                        */
/*  [TYPE]                                                                   */
/*      INT                                                                  */
/*  [PARAMETER]                                                              */
/*      None                                                                 */
/*  [RETURN]                                                                 */
/*      タスク停止結果                                                       */
/*  [NOTE]                                                                   */
/*                                                                           */
/*****************************************************************************/
INT Bsm_Stop( )
{
	return fs_bsm_Stop();
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      Bsm_Set_Callback                                                     */
/*  [DESCRIPTION]                                                            */
/*      BSMコールバック関数設定                                              */
/*  [TYPE]                                                                   */
/*      INT                                                                  */
/*  [PARAMETER]                                                              */
/*      BSM_CB_FP callback_func                                              */
/*      VOID * user                                                          */
/*  [RETURN]                                                                 */
/*      コールバック関数設定結果                                             */
/*  [NOTE]                                                                   */
/*                                                                           */
/*****************************************************************************/
INT Bsm_Set_Callback( BSM_CB_FP callback_func, VOID * user )
{
	return fs_bsm_SetCallback( callback_func, user );
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      Bsm_Send                                                             */
/*  [DESCRIPTION]                                                            */
/*      BSMコマンド送信                                                      */
/*  [TYPE]                                                                   */
/*      INT                                                                  */
/*  [PARAMETER]                                                              */
/*      INT dl                                                               */
/*      UCHAR * df                                                           */
/*  [RETURN]                                                                 */
/*      送信処理結果                                                         */
/*  [NOTE]                                                                   */
/*                                                                           */
/*****************************************************************************/
INT Bsm_Send( INT dl, UCHAR * df )
{
	return fs_bsm_Send( dl, df );
}


/** BSM BTMイベントログ出力 */
static int EVENT_DRC_BSM_BTM(int formatId, int event)
{
	int fd; 
	int group_id = GROUP_ID_EVENT_LOG;	//  出力先グループID
    int ret;
	struct drc_bt_btm_event_log  logBuf;	// 機能毎ログ格納バッファ

    struct lites_trace_header trc_hdr;	// ドラレコ用ヘッダー
 	struct lites_trace_format trc_fmt;	// ドラレコ用フォーマット
	memset(&trc_fmt, 0x0, sizeof(trc_fmt));
	memset(&trc_hdr, 0x0, sizeof(trc_hdr));
	memset(&logBuf, 0x0, sizeof(logBuf));

	fd = open("/dev/lites", O_RDWR);	// <-libipasのAPIを使用するために必須
	if(fd < 0){
		ADA_BT_LOGE("open()[EVENT_DRC_BSM_BTM] error. fd=%d", fd);
		return -1;
	}

	logBuf.event = (unsigned int)event;
    trc_fmt.trc_header = &trc_hdr;		// ドラレコ用ヘッダー格納
	trc_fmt.count = sizeof(logBuf);		// 保存ログバッファのサイズ
	trc_fmt.buf = &logBuf;				// 保存ログバッファ
    trc_fmt.trc_header->level = LOG_LEVEL_EVENT;			// ログレベル
    trc_fmt.trc_header->trace_no = TRACE_NO_LIB_HFP;		// トレース番号(星取表のNo)
    trc_fmt.trc_header->trace_id = TRACE_ID_BT_BLOCK_IF;	// トレースID(機能毎の値)
    trc_fmt.trc_header->format_id = formatId;				// フォーマットID (bufのフォーマット識別子)

    ret = lites_trace_write(fd, group_id, &trc_fmt);	// ログ出力
	if (ret < 0) {
		ADA_BT_LOGE("lites_trace_write()[EVENT_DRC_BSM_BTM] error ret=%d", ret);
	}

    close(fd);

    return ret;
}

/** BSM_BTMソフトエラー用ログ出力 */
static int ERR_DRC_BSM_BTM(int errorId, int para1, int para2, int para3, int para4)
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
		ADA_BT_LOGE("open()[ERR_DRC_BSM_BTM] error. fd=%d", fd);
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
    trc_fmt.trc_header->trace_no = TRACE_NO_LIB_HFP;		// トレース番号(星取表のNo)
    trc_fmt.trc_header->trace_id = TRACE_ID_ERROR;	    	// トレースID(機能毎の値)
    trc_fmt.trc_header->format_id = FORMAT_ID_ERROR_LOG;	// フォーマットID (bufのフォーマット識別子)

    ret = lites_trace_write(fd, group_id, &trc_fmt);	    // ログ出力
	if (ret < 0) {
		ADA_BT_LOGE("lites_trace_write()[ERR_DRC_BSM_BTM] error ret=%d", ret);
	}

    close(fd);

    return ret;
}

