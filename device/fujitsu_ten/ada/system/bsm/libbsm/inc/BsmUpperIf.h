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

/*****************************************************************************/
/*                                                                           */
/*      シリーズ名称    : A-DA                                               */
/*      マイコン名称    : メインマイコン                                     */
/*      オーダー名称    : Bluetoothモジュール制御部                          */
/*      ブロック名称    : -                                                  */
/*      機能名称        : BTモジュール制御タスク                             */
/*                                                                           */
/*      ファイル名称    : BsmUpperIf.h                                       */
/*      作   成   者    : 東芝情報システム株式会社                           */
/*      作　 成　 日    : 2011/10/20    <Ver. 0.01>                          */
/*      改　 訂　 日    : 2012/07/11    <Ver. 0.10>                          */
/*****************************************************************************/
#ifndef __BSM_UPPER_IF_H__
#define __BSM_UPPER_IF_H__
/*****************************************************************************/
/* INCLUDE宣言                                                               */
/*****************************************************************************/

/*****************************************************************************/
/* マクロ宣言                                                                */
/*****************************************************************************/

/*****************************************************************************/
/* 定数                                                                      */
/*****************************************************************************/
/* テストモード種別定義 */
/* DUTモード */
#define BSM_D_TEST_DUT_MODE                         0x00
/* HFP簡易接続 */
#define BSM_D_TEST_HFP_CON                          0x01
/* AVP簡易接続 */
#define BSM_D_TEST_AVP_CON                          0x02
/* ローカルループバック */
#define BSM_D_TEST_LOCAL_LOOPBACK                   0x03
/* オーディオテスト出力(fs=44.1kHz) */
#define BSM_D_TEST_AUDIO_TEST_44_1                  0x04
/* オーディオテスト出力(fs=48kHz) */
#define BSM_D_TEST_AUDIO_TEST_48                    0x05
/* リモートループバック */
#define BSM_D_TEST_REMOTE_LOOPBACK                  0x06
/* マイクレベル測定 */
#define BSM_D_TEST_CHECK_MIC_LEVEL                  0x07
/* 無変調キャリア送信 */
#define BSM_D_TEST_NON_MODULATED_CARRIER            0xF1
/* 連続バースト送信（Hoppingあり）*/
#define BSM_D_TEST_CONTINUATION_BURST_HOPPING       0xF2
/* 連続バースト送信（固定周波数) */
#define BSM_D_TEST_CONTINUATION_BURST_NON_HOPPING   0xF3

/*  送信チャネル数  */
#define BSM_D_CHANNEL_LEN               12
#define BSM_D_TEST_DEV_NAME_MAX         30
#define BSM_D_COMMON_INIT_VALUE         0

/*****************************************************************************/
/* 型宣言                                                                    */
/*****************************************************************************/
typedef void ( *BSM_CB_FP )( VOID * user, INT dl, UCHAR* df );

/*****************************************************************************/
/*  構造体定義                                                               */
/*****************************************************************************/
/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      BSM_TEST_PARAM                                                       */
/*  [DESCRIPTION]                                                            */
/*      テストパラメータ                                                     */
/*  [NOTE]                                                                   */
/*                                                                           */
/*****************************************************************************/
typedef struct {
	ULONG        test_mode;                       /* テストモード種別 */
	ULONG        frequency;                       /* 送信周波数 */
	ULONG        packet_type;                     /* 送信パケットタイプ */
	UCHAR        channel[BSM_D_CHANNEL_LEN];      /* 送信チャネル */
	INT          device_name_len;                 /* Device Nameのデータ長 */
	UCHAR        device_name[BSM_D_TEST_DEV_NAME_MAX]; /* ローカルデバイスのデバイス名称 */
} BSM_TEST_PARAM;

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      BSM_RESULT                                                           */
/*  [DESCRIPTION]                                                            */
/*      BSM API処理結果定義                                                  */
/*  [NOTE]                                                                   */
/*                                                                           */
/*****************************************************************************/
typedef enum {
	BSM_OK          =  0,
	BSM_ERROR       =  1,
	BSM_NO_RESOURCE =  2,
} BSM_RESULT;

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      BSM_MODE                                                             */
/*  [DESCRIPTION]                                                            */
/*      BSM起動モード定義                                                    */
/*  [NOTE]                                                                   */
/*                                                                           */
/*****************************************************************************/
typedef enum {
	BSM_MODE_SHUTDOWN     =  -1,
	BSM_MODE_NORMAL       =  0,
	BSM_MODE_HCI          =  1,
	BSM_MODE_DUT          =  2,
	BSM_MODE_HF_TEST      =  3,
	BSM_MODE_AV_TEST      =  4,
	BSM_MODE_RF_LOGO_TEST =  5,
} BSM_MODE;

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      BSM_MODE_OLD                                                         */
/*  [DESCRIPTION]                                                            */
/*      BSM起動モード定義（旧）                                              */
/*  [NOTE]                                                                   */
/*                                                                           */
/*****************************************************************************/
typedef enum {
	BSM_MODE_OLD_NORMAL  =  0,
	BSM_MODE_OLD_HCI     =  1,
	BSM_MODE_OLD_TEST    =  2,
} BSM_MODE_OLD;

/*****************************************************************************/
/* 外部変数宣言                                                              */
/*****************************************************************************/
extern INT	gdwBsmMode;

/*****************************************************************************/
/*  外部関数プロトタイプ                                                     */
/*****************************************************************************/
extern INT Bsm_Start( INT bsm_mode, BSM_TEST_PARAM * test_param );
extern INT Bsm_Stop();
extern INT Bsm_Set_Callback( BSM_CB_FP callback_func, VOID * user );
extern INT Bsm_Send( INT dl, UCHAR * df );

extern INT fs_bsm_Start( INT bsm_mode, INT bTestParamLen, UCHAR * pbTestParam );
extern INT fs_bsm_Stop();
extern INT fs_bsm_SetCallback( BSM_CB_FP callback_func, VOID * user );
extern INT fs_bsm_Send( INT dl, UCHAR * df );
#endif /* #ifndef __BSM_UPPER_IF_H__ */

