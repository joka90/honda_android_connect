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


#include <ada/log.h>
#include <cutils/log.h>


int _ERR_DRC_BLUEZ(int errorId, int srcId, int line, int para1, int para2);
int EVENT_DRC_BLUEZ_METHOD(int formatId, int param, const char *bdaddr);
int EVENT_DRC_BLUEZ_SIGNAL(int formatId, int param, int result);
int EVENT_DRC_BLUEZ_CONNECT_CB(int formatId, int callback, int connection);
int EVENT_DRC_BLUEZ_A2DP_CONNECT_CB(int formatId, int connection, int first, int second);
int EVENT_DRC_BLUEZ_CB(int formatId, int callback);
int EVENT_DRC_BLUEZ_HCI_METHOD(int formatId, int hciComm);
int EVENT_DRC_BLUEZ_HCI_SIGNAL(int formatId, int signal, unsigned char link_type, unsigned char status );
int EVENT_DRC_BLUEZ_DISCOVERY(int formatId, int event, int param);
int EVENT_DRC_BLUEZ_PAIRING(int formatId, int event, unsigned char status, bdaddr_t *bdaddr);
int EVENT_DRC_BLUEZ_INIT(int formatId, int event);
int EVENT_DRC_BLUEZ_MANAGER(int formatId, int event, bdaddr_t *bdaddr, int param);
int EVENT_DRC_BLUEZ_CONFIRM(int formatId, int event, bdaddr_t *bdaddr);
int EVENT_DRC_BLUEZ_STATE(int formatId, int event, bdaddr_t *bdaddr, int state);
int EVENT_DRC_BLUEZ_ADAPTER(int formatId, int event, int param);
int EVENT_DRC_BLUEZ_DID(int formatId, int event, int result, int vendorID, int productID, int version);
int EVENT_DRC_BLUEZ_BDADDR(int formatId, int event, int errId, int line, bdaddr_t *bdaddr);
int EVENT_DRC_BLUEZ_CHGMSG(int formatId, int event, bdaddr_t *bdaddr);

// Source file ID
#define BLUEZ_SRC_ID_AUDIO_A2DP_C		0x01
#define BLUEZ_SRC_ID_AUDIO_A2DPAD_C		0x02
#define BLUEZ_SRC_ID_AUDIO_AVDTP_C		0x03
#define BLUEZ_SRC_ID_AUDIO_AVRCPAD_C	0x04
#define BLUEZ_SRC_ID_AUDIO_BROWSING_C	0x05
#define BLUEZ_SRC_ID_AUDIO_CONTROL_C	0x06
#define BLUEZ_SRC_ID_AUDIO_DEVICE_C		0x07
#define BLUEZ_SRC_ID_AUDIO_HANDSFREE_C	0x08
#define BLUEZ_SRC_ID_AUDIO_MAIN_C		0x09
#define BLUEZ_SRC_ID_AUDIO_MANAGER_C	0x0a
#define BLUEZ_SRC_ID_AUDIO_PBAP_C		0x0b
#define BLUEZ_SRC_ID_BTIO_BTIO_C		0x0c
#define BLUEZ_SRC_ID_GDBUS_OBJECT_C		0x0d
#define BLUEZ_SRC_ID_LIB_BLUETOOTH_C	0x0e
#define BLUEZ_SRC_ID_LIB_HCI_C			0x0f
#define BLUEZ_SRC_ID_LIB_SDP_C			0x10
#define BLUEZ_SRC_ID_PLUGINS_HCIOPS_C	0x11
#define BLUEZ_SRC_ID_SRC_ADAPTER_C		0x12
#define BLUEZ_SRC_ID_SRC_HCICOMMAND_C	0x13
#define BLUEZ_SRC_ID_SRC_MAIN_C			0x14
#define BLUEZ_SRC_ID_SRC_MANAGER_C		0x15
#define BLUEZ_SRC_ID_SRC_RECORDER_C		0x16
#define BLUEZ_SRC_ID_SRC_STORAGE_C		0x17
#define BLUEZ_SRC_ID_SRC_TEXTFILE_C		0x18

#define ERR_DRC_BLUEZ( errorId, para1, para2 )	{	\
	_ERR_DRC_BLUEZ( errorId, BLUEZ_SRC_ID, __LINE__, \
	para1, para2 ); }

//フォーマットID
/** エラーログのフォーマットID */
#define FORMAT_ID_ERROR_LOG 1
/** BT DBUSメソッドを示す */
#define FORMAT_ID_DBUS_METHOD 		2010
/** BT DBUSシグナルを示す */
#define FORMAT_ID_DBUS_SIGNAL 		2020
/** BT HCI COMMANDを示す */
#define FORMAT_ID_HCI_COMMAND 		2030
/** BT HCI COMMAND SIGNALを示す */
#define FORMAT_ID_DBUS_HCI_SIGNAL 	2040
/** BT Connect callbackを示す */
#define FORMAT_ID_CONNECT_CB 		2050
/** BT A2DP Connect callbackを示す */
#define FORMAT_ID_A2DP_CONNECT_CB 	2060
/** BT 一般callbackを示す */
#define FORMAT_ID_BT_CB 			2070
/** BT DISCOVERY動作を示す */
#define FORMAT_ID_BT_DISCOVERY 		2080
/** BT PAIRING動作を示す */
#define FORMAT_ID_BT_PAIRING 		2090
/** BLUEZ 初期化動作を示す */
#define FORMAT_ID_BT_INIT 			2100

/** BT MANAGER 関連動作を示す */
#define FORMAT_ID_BT_MANAGER		2110
/** PROFILE CONFIRM 動作を示す */
#define FORMAT_ID_CONFIRM			2120
/** PROFILE STATE を示す */
#define FORMAT_ID_STATE				2130
/** BT ADAPTER 関連動作を示す  */
#define FORMAT_ID_BT_ADAPTER		2140
/** BT DID 関連動作を示す  */
#define FORMAT_ID_DID				2150
/** BLUEZ エラー時BDADDRを示す */
#define FORMAT_ID_BT_ERR_BDADDR		2160
/** 接続通知の付け替えを示す */
#define FORMAT_ID_CHGMSG			2170


//ログレベル
#define LOG_LEVEL_EVENT   3 //informational
#define LOG_LEVEL_ERROR   3 //error conditions

#if 0
//エラー箇所ID
//共通
#define TYPE_COMMON 	0
//HFP
#define TYPE_HFP 		1
//PBAP
#define TYPE_PBAP 		2
//A2DP
#define TYPE_A2DP 		3
//AVRCP
#define TYPE_AVRCP 		4
//BROWSING
#define TYPE_BROWSING   5
//HCI COMMAND
#define TYPE_HCICMD 	6
#endif

//付加情報
//BLUEZ 初期化動作
/** HCI CMDのDBUS初期化イベントを示す. */
#define INIT_DBUS_HCICMD			0x0001
/** HFPのDBUS初期化イベントを示す. */
#define INIT_DBUS_HFP				0x0002
/** HFPの初期化イベントを示す. */
#define INIT_HFP					0x0003
/** PBAPのDBUS初期化イベントを示す. */
#define INIT_DBUS_PBAP				0x0004
/** PBAPの初期化イベントを示す. */
#define INIT_PBAP					0x0005
/** A2DPのDBUS初期化イベントを示す. */
#define INIT_DBUS_A2DP				0x0006
/** A2DPの初期化イベントを示す. */
#define INIT_A2DP					0x0007
/** AVRCPのDBUS初期化イベントを示す. */
#define INIT_DBUS_AVRCP				0x0008
/** AVRCPの初期化イベントを示す. */
#define INIT_AVRCP					0x0009
/** BROWSINGのDBUS初期化イベントを示す. */
#define INIT_DBUS_BROWSING			0x000A
/** BROWSINGの初期化イベントを示す. */
#define INIT_BROWSING				0x000B
/** BLUEZ初期化を示す. */
#define INIT_BLUEZ					0x000C

/** HFの接続待受開始メソッドを示す. */
#define METHOD_HFP_REGISTER				0x0011
/** HFの接続待受停止メソッドを示す. */
#define METHOD_HFP_DEREGISTER			0x0012
/** HFP接続開始メソッドを示す. */
#define METHOD_HFP_CONNECT 				0x0013
/** HFP切断開始メソッドを示す. */
#define METHOD_HFP_DISCONNECT 			0x0014
/** SCO接続開始メソッドを示す. */
#define METHOD_SCO_CONNECT 				0x0015
/** SCO切断開始メソッドを示す. */
#define METHOD_SCO_DISCONNECT 			0x0016
/** HFP接続可否通知メソッドを示す. */
#define METHOD_HFP_AUTHORIZATION_RES 	0x0017

/** A2DPの接続待受開始メソッドを示す. */
#define METHOD_A2DP_REGISTER 			0x0021
/** A2DPの接続待受停止メソッドを示す. */
#define METHOD_A2DP_DEREGISTER 			0x0022
/** A2DP接続開始メソッドを示す. */
#define METHOD_A2DP_CONNECT 			0x0023
/** A2DP切断開始メソッドを示す. */
#define METHOD_A2DP_DISCONNECT 			0x0024
/** A2DP接続可否通知メソッドを示す. */
#define METHOD_A2DP_AUTHORIZATION_RES 	0x0025

/** AVRCPの接続待受開始メソッドを示す. */
#define METHOD_AVRCP_REGISTER 			0x0031
/** AVRCPの接続待受停止メソッドを示す. */
#define METHOD_AVRCP_DEREGISTER 		0x0032
/** AVRCP接続開始メソッドを示す. */
#define METHOD_AVRCP_CONNECT 			0x0033
/** AVRCP切断開始メソッドを示す. */
#define METHOD_AVRCP_DISCONNECT 		0x0034
/** AVRCP接続可否通知メソッドを示す. */
#define METHOD_AVRCP_AUTHORIZATION_RES 	0x0035

/** BROWSING接続開始メソッドを示す. */
#define METHOD_BROWSING_CONNECT 		0x0041
/** BROWSING切断開始メソッドを示す. */
#define METHOD_BROWSING_DISCONNECT 		0x0042
/** BROWSINGの接続待受開始メソッドを示す. */
#define METHOD_BROWSING_REGISTER 		0x0043
/** BROWSINGの接続待受停止メソッドを示す. */
#define METHOD_BROWSING_DEREGISTER 		0x0044

/** PBAPの接続待受開始メソッドを示す. */
#define METHOD_PBAP_REGISTER 			0x0051
/** PBAPの接続待受停止メソッドを示す. */
#define METHOD_PBAP_DEREGISTER 			0x0052
/** PBAP接続開始メソッドを示す. */
#define METHOD_PBAP_CONNECT 			0x0053
/** PBAP切断開始メソッドを示す. */
#define METHOD_PBAP_DISCONNECT 			0x0054

/** HFP接続待受開始通知を示す. */
#define SIGNAL_HFP_REGISTER_CFM 		0x0061
/** HFP接続待受停止通知を示す. */
#define SIGNAL_HFP_DEREGISTER_CFM 		0x0062
/** HFPの接続完了の通知を示す. */
#define SIGNAL_HFP_CONNECT_CFM 			0x0063
/** HFPの切断完了の通知を示す. */
#define SIGNAL_HFP_DISCONNECT_CFM 		0x0064
/** リモートデバイスからHFP接続の通知を示す. */
#define SIGNAL_HFP_CONNECTED 			0x0065
/** リモートデバイスからのHFP切断の通知を示す. */
#define SIGNAL_HFP_DISCONNECTED 		0x0066
/** HFPの接続可否の通知を示す. */
#define SIGNAL_HFP_AUTHORIZATION 		0x0067

/** A2DP接続待受開始通知を示す. */
#define SIGNAL_A2DP_REGISTER_CFM 		0x0071
/** A2DP接続待受停止通知を示す. */
#define SIGNAL_A2DP_DEREGISTER_CFM 		0x0072
/** A2DPの接続完了の通知を示す. */
#define SIGNAL_A2DP_CONNECT_CFM 		0x0073
/** A2DPの切断完了の通知を示す. */
#define SIGNAL_A2DP_DISCONNECT_CFM 		0x0074
/** リモートデバイスからA2DP接続の通知を示す. */
#define SIGNAL_A2DP_CONNECTED 			0x0075
/** リモートデバイスからのA2DP切断の通知を示す. */
#define SIGNAL_A2DP_DISCONNECTED 		0x0076
/** A2DPの接続可否の通知を示す. */
#define SIGNAL_A2DP_AUTHORIZATION 		0x0077

/** AVRCP接続待受開始通知を示す. */
#define SIGNAL_AVRCP_REGISTER_CFM 		0x0081
/** AVRCP接続待受停止通知を示す. */
#define SIGNAL_AVRCP_DEREGISTER_CFM 	0x0082
/** AVRCPの接続完了の通知を示す. */
#define SIGNAL_AVRCP_CONNECT_CFM 		0x0083
/** AVRCPの切断完了の通知を示す. */
#define SIGNAL_AVRCP_DISCONNECT_CFM 	0x0084
/** リモートデバイスからAVRCP接続の通知を示す. */
#define SIGNAL_AVRCP_CONNECTED 			0x0085
/** リモートデバイスからのAVRCP切断の通知を示す. */
#define SIGNAL_AVRCP_DISCONNECTED 		0x0086
/** AVRCPの接続可否の通知を示す. */
#define SIGNAL_AVRCP_AUTHORIZATION 		0x0087

/** BROWSINGの接続完了の通知を示す. */
#define SIGNAL_BROWSING_CONNECT_CFM 	0x0091
/** BROWSINGの切断完了の通知を示す. */
#define SIGNAL_BROWSING_DISCONNECT_CFM 	0x0092
/** リモートデバイスからのBROWSING切断の通知を示す. */
#define SIGNAL_BROWSING_DISCONNECTED 	0x0093
/** リモートデバイスからのBROWSING接続の通知を示す. */
#define SIGNAL_BROWSING_CONNECTED 	0x0094

/** PBAP接続待受開始通知を示す. */
#define SIGNAL_PBAP_REGISTER_CFM 		0x00A1
/** PBAP接続待受停止通知を示す. */
#define SIGNAL_PBAP_DEREGISTER_CFM 		0x00A2
/** PBAPの接続完了の通知を示す. */
#define SIGNAL_PBAP_CONNECT_CFM 		0x00A3
/** PBAPの切断完了の通知を示す. */
#define SIGNAL_PBAP_DISCONNECT_CFM 		0x00A4
/** リモートデバイスからのAVRCP切断の通知を示す. */
#define SIGNAL_PBAP_DISCONNECTED 		0x00A5
/** 不明のRFCOMM接続の通知を示す. */
#define SIGNAL_UNKNOWN_RFCOMM_CONNECT 		0x00A5
/** 不明のRFCOMM切断の通知を示す. */
#define SIGNAL_UNKNOWN_RFCOMM_DISCONNECT	0x00A6
/** 不明のL2CAP接続の通知を示す. */
#define SIGNAL_UNKNOWN_L2CAP_CONNECT 		0x00A7
/** 不明のL2CAP切断の通知を示す. */
#define SIGNAL_UNKNOWN_L2CAP_DISCONNECT 	0x00A8

/** HCI class command 実行開始を示す. */
#define METHOD_HCI_CLASSCOMM	    	0x00B1
/** HCI evtmask command 実行開始を示す. */
#define METHOD_HCI_EVTMASKCOMM 	    	0x00B2
/** HCI name command 実行開始を示す. */
#define METHOD_HCI_NAMECOMM 	    	0x00B3
/** HCI version command 実行開始を示す. */
#define METHOD_HCI_VERSIONCOMM 	    	0x00B4
/** HCI sspmode command 実行開始を示す. */
#define METHOD_HCI_SSPMODECOMM 	    	0x00B5
/** HCI lp command 実行開始を示す. */
#define METHOD_HCI_LPCOMM 	    		0x00B6
/** HCI addr command 実行開始を示す. */
#define METHOD_HCI_ADDRCOMM 	    	0x00B7
/** HCI dc command 実行開始を示す. */
#define METHOD_HCI_DCCOMM 	    		0x00B8
/** HCI vs codec config command 実行開始を示す. */
#define METHOD_HCI_VSCODECCOMM 	    	0x00B9
/** HCI vs wbs disassociate command 実行開始を示す. */
#define METHOD_HCI_VSWBSDISASSOCIATE   	0x00BA

/** HCI class command 実行完了を示す. */
#define SIGNAL_HCI_CLASSCOMM 	    	0x00C1
/** HCI evtmask command 実行完了を示す. */
#define SIGNAL_HCI_EVTMASKCOMM 	    	0x00C2
/** HCI name command実行完了を示す. */
#define SIGNAL_HCI_NAMECOMM	    		0x00C3
/** HCI version command 実行完了を示す. */
#define SIGNAL_HCI_VERSIONCOMM 	    	0x00C4
/** HCI sspmode command 実行完了を示す. */
#define SIGNAL_HCI_SSPMODECOMM 	    	0x00C5
/** HCI lp command 実行完了を示す. */
#define SIGNAL_HCI_LPCOMM 	    		0x00C6
/** HCI addr command 実行完了を示す. */
#define SIGNAL_HCI_ADDRCOMM 	    	0x00C7
/** HCI dc command 実行完了を示す. */
#define SIGNAL_HCI_DCCOMM 	    		0x00C8
/** HCI vs codec config command 実行完了を示す. */
#define SIGNAL_HCI_VSCODECCOMM 	    	0x00C9
/** HCI_CMD_STATUS シグナル通知を示す. */
#define SIGNAL_HCI_CMD_STATUS			0x00CA
/** HCI_CONNECT_REQ シグナル通知を示す. */
#define SIGNAL_HCI_CONNECT_REQ			0x00CB
/** HCI_CONNECT_COMP シグナル通知を示す. */
#define SIGNAL_HCI_CONNECT_COMP			0x00CC
/** HCI_DISCONNECT_COMP シグナル通知を示す. */
#define SIGNAL_HCI_DISCONNECT_COMP 		0x00CD
/** HCI_SYNC_CONNECT_COMP シグナル通知を示す. */
#define SIGNAL_HCI_SYNC_CONNECT_COMP 	0x00CE
/** HCI_SYNC_CONNECT_CHNG シグナル通知を示す. */
#define SIGNAL_HCI_SYNC_CONNECT_CHNG 	0x00CF
/** HCI vs wbs disassociate 実行完了を示す. */
#define SIGNAL_HCI_VSWBSDISASSOCIATE   	0x00C0

/** HFP接続Callbackを示す. */
#define CB_HFP_CONNECT 					0x00D1
/** PBAP接続Callbackを示す. */
#define CB_PBAP_CONNECT 				0x00D2
/** A2DP接続Callbackを示す. */
#define CB_A2DP_CONNECT 				0x00D3
/** AVRCP接続Callbackを示す. */
#define CB_AVRCP_CONNECT 				0x00D4
/** BROWSING接続Callbackを示す. */
#define CB_BROWSING_CONNECT 			0x00D5

/** HFP REGISTER Callbackを示す. */
#define CB_HFP_REGISTER 				0x00E1
/** A2DP REGISTER Callbackを示す. */
#define CB_A2DP_REGISTER 				0x00E2
/** AVRCP REGISTER Callbackを示す. */
#define CB_AVRCP_REGISTER 			    0x00E3
/** HFP GET RECORD Callbackを示す. */
#define CB_HFP_GET_RECORD				0x00E4
/** PBAP GET RECORD Callbackを示す. */
#define CB_PBAP_GET_RECORD 			    0x00E5
/** AVRCP GET RECORD Callbackを示す. */
#define CB_AVRCP_GET_RECORD 			0x00E6
/** SCO Connect Callbackを示す. */
#define CB_SCO_CONNECT 			   		0x00E7
/** BROWSING REGISTER Callbackを示すを示す. */
#define CB_BROWSING_REGISTER 			0x00E8

/** BT 検索開始を示す. */
#define BT_START_DISCOVERY 				0x00F1
/** BT 検索終了を示す. */
#define BT_STOP_DISCOVERY 				0x00F2
/** BT ペアリング開始を示す. */
#define BT_START_PAIRING				0x00F3
/** BT ペアリングキャンセルを示す. */
#define BT_CANCEL_PAIRING				0x00F4
/** BT BONDING 完了を示す. */
#define BT_BONDING_COMP					0x00F5
/** BT CONFIRM REQUEST を示す. */
#define BT_CONFIRM_REQ					0x00F6
/** BT PIN REQUEST を示す. */
#define BT_PIN_REQ						0x00F7

/** CONNECTION COMPLET の無視を示す. */
#define	MANAGER_COMP_IGNORE				0x0101
/** CONNECTION REQUEST の拒否を示す. */
#define	MANAGER_CONNREQ_REJECT			0x0102
/** CONNECTION REQUEST の許可を示す. */
#define	MANAGER_ACL_ACCEPT				0x0103
/** CONNECTION REQUEST の拒否を示す. */
#define	MANAGER_ACL_REJECT				0x0104
/** 不明な CHANNEL の切断要求を示す. */
#define	MANAGER_DISCONNECTED_CHAN		0x0105
/** 不明な CHANNEL の切断指示を示す. */
#define	MANAGER_DISC_CFM_CHAN			0x0106
/** 不明な L2CAP の切断要求を示す. */
#define	MANAGER_DISCONNECTED_L2CAP		0x0107
/** 不明な L2CAP の切断指示を示す. */
#define	MANAGER_DISC_CFM_LCAP			0x0108
/** ペアリングデバイスの追加を示す. */
#define	MANAGER_CREATE_PAIRDEVICE		0x0109
/** 相手端末からの HFP 接続要求を示す. */

#define	CONFIRM_HFP						0x0111
/** 相手端末からの A2DP 接続要求を示す. */
#define	CONFIRM_A2DP					0x0112
/** 相手端末からの AVRCP 接続要求を示す. */
#define	CONFIRM_AVRCP					0x0113
/** 相手端末からの BROWSING 接続要求を示す. */
#define	CONFIRM_BROWSING				0x0114

/** HFP の状態変化を示す. */
#define	STATE_HFP						0x0121
/** PBAP の状態変化を示す. */
#define	STATE_PBAP						0x0122
/** A2DP の状態変化を示す. */
#define	STATE_A2DP						0x0123
/** AVRCP の状態変化を示す. */
#define	STATE_AVRCP						0x0124
/** BROWSING の状態変化を示す. */
#define	STATE_BROWSING					0x0125

/** PROPERTY 変化（Pairable）を示す. */
#define	ADP_CHG_PAIRABLE				0x0131
/** PROPERTY 変化（DiscoverableTimeout）を示す. */
#define	ADP_CHG_DISCOVERABLETIMEOUT		0x0132
/** PROPERTY 変化（PairableTimeout）を示す. */
#define	ADP_CHG_PAIRABLETIMEOUT			0x0133
/** PROPERTY 変化（Class）を示す. */
#define	ADP_CHG_CLASS					0x0134
/** PROPERTY 変化（Name）を示す. */
#define	ADP_CHG_NAME					0x0135
/** PROPERTY 変化（Powered）を示す. */
#define	ADP_CHG_POWERED					0x0136
/** PROPERTY 変化（Discoverable）を示す. */
#define	ADP_CHG_DISCOVERABLE			0x0137
/** PROPERTY 変化（Discovering）を示す. */
#define	ADP_CHG_DISCOVERING				0x0138

/** DID SDP の取得を示す. */
#define	SIGNAL_DEVICEIDENTIFICATION		0x0141
/** DID SDP のTIMEOUTを示す. */
#define	SIGNAL_DID_SDP_TIMEOUT			0x0142

/** エラー時のBTアドレス表示を示す. */
#define BLUEZ_SRC_ID_BDADDR				0x0151

/** HFP接続通知の方向変更を示す. */
#define CHANGE_MESSAGE_HFP				0x0161
/** A2DP一本目接続通知の方向変更を示す. */
#define CHANGE_MESSAGE_A2DP_1ST			0x0162
/** A2DP二本目接続通知の方向変更を示す. */
#define CHANGE_MESSAGE_A2DP_2ND			0x0163
/** AVRCP接続通知の方向変更を示す. */
#define CHANGE_MESSAGE_AVRCP			0x0164
/** BROWSING接続通知の方向変更を示す. */
#define CHANGE_MESSAGE_BROWSING			0x0165
/** BROWSING接続通知の方向変更を示す. */
#define CHANGE_MESSAGE_BROWSING_CROSS	0x0165


//エラーID
//接続エラー(Dbusから接続情報取得失敗)
#define ERROR_BLUEZ_CONNECT_DBUS_GET_ARGS			0x5001
//接続エラー(対向機情報取得失敗)
#define ERROR_BLUEZ_CONNECT_FIND_DEV				0x5002
//接続エラー(RFCOMM接続失敗) /status:
#define ERROR_BLUEZ_CONNECT_RFCOMM_CONNECT			0x5003
//接続エラー(切断中)
#define ERROR_BLUEZ_CONNECT_UNDER_CONNECTION		0x5004
//接続エラー(接続拒否)
#define ERROR_BLUEZ_CONNECT_REFUSING				0x5005
//接続エラー(IO接続エラー)
#define ERROR_BLUEZ_CONNECT_IO_CONNECT				0x5006
//接続エラー(L2CAP接続失敗)
#define	ERROR_BLUEZ_CONNECT_L2CAP_CONNECT			0x5007
//接続Callbackエラー(callbackエラー)
#define ERROR_BLUEZ_CONNECT_CB_ERR					0x5101
//接続Callbackエラー(IO_GET失敗)
#define	ERROR_BLUEZ_CONNECT_CB_IO_GET				0x5102
//接続Callbackエラー(ソケットオプション取得失敗)
#define	ERROR_BLUEZ_CONNECT_CB_GET_SOCKOPT			0x5103
//接続Callbackエラー(既に接続済み)
#define	ERROR_BLUEZ_CONNECT_CB_CONNECTED			0x5104

//切断エラー(Dbusから切断情報取得失敗)
#define ERROR_BLUEZ_DISCONNECT_DBUS_GET_ARGS		0x5201
//切断エラー(既に切断済み)
#define ERROR_BLUEZ_DISCONNECT_FIND_DEV				0x5202
//切断エラー(接続が存在しない)
#define	ERROR_BLUEZ_DISCONNECT_NULL					0x5203
//切断エラー(その他エラー)
#define	ERROR_BLUEZ_DISCONNECT_ERROR				0x5204

//REGISTERエラー(Dbusから情報取得失敗)
#define ERROR_BLUEZ_REGISTER_DBUS_GET_ARGS			0x5301
//REGISTERエラー(ソケットlistenエラー)
#define ERROR_BLUEZ_REGISTER_IO_LISTEN				0x5302
//REGISTERエラー(SDPレコード取得エラー)
#define ERROR_BLUEZ_REGISTER_SDP_RECORD				0x5303
//REGISTERエラー(SDPレコード追加エラー)
#define ERROR_BLUEZ_REGISTER_ADD_RECORD				0x5304
//REGISTERエラー(既にREGISTER済み)
#define	ERROR_BLUEZ_REGISTER_REGISTERED				0x5305
//DEREGISTERエラー
#define	ERROR_BLUEZ_DEREGISTER						0x5306
//Callbackエラー(IO GETエラー)
#define ERROR_BLUEZ_CONFIRM_CB_IO_GET				0x5401
//Callbackエラー(デバイス取得失敗)
#define ERROR_BLUEZ_CONFIRM_CB_GET_DEV				0x5402
//Callbackエラー(接続拒否)
#define ERROR_BLUEZ_CONFIRM_CB_REFUSING				0x5403
//Callbackエラー(切断中)
#define ERROR_BLUEZ_CONFIRM_CB_UNDER_CONNECTION		0x5404
//Callbackエラー(UUID追加失敗)
#define ERROR_BLUEZ_CONFIRM_CB_ADD_UUID				0x5405
//Callbackエラー(接続が既に存在する)
#define ERROR_BLUEZ_CONFIRM_CB_EXISTS				0x5406
//Callbackエラー(RFCOMM接続エラー)
#define ERROR_BLUEZ_CONFIRM_CB_CONNECT_RFCOMM		0x5407
//Callbackエラー(ソケットacceptエラー)
#define	ERROR_BLUEZ_CONFIRM_CB_IO_ACCEPT			0x5408
//Callbackエラー(RFCOMM既に存在する)
#define ERROR_BLUEZ_CONNECT_RFCOMM_CONNECTED		0x5409

//データ送信エラー(Dbusから送信情報取得失敗)
#define ERROR_BLUEZ_DATASEND_DBUS_GET_ARGS 			0x5501
//データ送信エラー(デバイス取得失敗)
#define ERROR_BLUEZ_DATASEND_FIND_DEV 				0x5502
//データ送信エラー(RFCOMM送信失敗)
#define ERROR_BLUEZ_DATASEND_RFCOMM_SEND 			0x5503
//データ送信エラー(ソケット送信失敗)
#define ERROR_BLUEZ_DATASEND_WRITE 					0x5504
//データ受信エラー(G_IOエラー)
#define ERROR_BLUEZ_RECEIVE_CB_IO					0x5601
//データ受信エラー(切断状態)
#define ERROR_BLUEZ_RECEIVE_CB_DISCONNECTED			0x5602
//データ受信エラー(ソケットREAD失敗)
#define ERROR_BLUEZ_RECEIVE_CB_READ					0x5603
//データ受信エラー(空きバッファ不足)
#define ERROR_BLUEZ_RECEIVE_CB_OVER_BUF				0x5604
//データ受信エラー(IO_GETエラー)
#define	ERROR_BLUEZ_RECEIVE_CB_IO_GET				0x5605

//HCI
//class commandエラー
#define ERROR_BLUEZ_HCI_CMD_CLASS					0x5701
//evtmask commandエラー
#define ERROR_BLUEZ_HCI_CMD_EVTMASK					0x5702
//name commandエラー
#define ERROR_BLUEZ_HCI_CMD_NAME					0x5703
//version commandエラー
#define ERROR_BLUEZ_HCI_CMD_VERSION					0x5704
//sspmode commandエラー
#define ERROR_BLUEZ_HCI_CMD_SSPMODE					0x5705
//lp commandエラー
#define ERROR_BLUEZ_HCI_CMD_LP						0x5706
//addr commandエラー
#define ERROR_BLUEZ_HCI_CMD_ADDR					0x5707
//dc commandエラー
#define ERROR_BLUEZ_HCI_CMD_DC						0x5708
//vs codec config commandエラー
#define ERROR_BLUEZ_HCI_CMD_VSCODEC					0x5709
//LIB HCI実行エラー
#define ERROR_BLUEZ_LIB_HCI							0x570A
//vs wbs disassociateエラー
#define ERROR_BLUEZ_HCI_CMD_VSWBSDISASSOCIATE		0x570B
//get roleエラー
#define ERROR_BLUEZ_HCI_CMD_GETROLE					0x570C
//accept conn reqエラー
#define ERROR_BLUEZ_HCI_CMD_ACCON					0x570D
//reject conn reqエラー
#define ERROR_BLUEZ_HCI_CMD_RJCON					0x570E

//SDPレコードエラー
#define ERROR_BLUEZ_SDP_RECORD						0x5801
//GET_RECORDエラー(レコード取得エラー)
#define ERROR_BLUEZ_GET_RECORDS						0x5802
//GET_RECORD Callbackエラー
#define	ERROR_BLUEZ_GET_RECORD_CB					0x5803
//GET_RECORDエラー(レコード取得エラー)
#define ERROR_BLUEZ_GET_RECORDS_AFTER_CONNECT		0x5804
//GET_RECORD Callbackエラー
#define	ERROR_BLUEZ_GET_RECORDS_AFTER_CONNECT_CB	0x5805

//HFP
//SCO接続エラー
#define ERROR_BLUEZ_CONNECT_SCO						0x5901
//SCO切断エラー
#define ERROR_BLUEZ_DISCONNECT_SCO					0x5902
//SCO Callbackエラー
#define ERROR_BLUEZ_SCO_CB							0x5903
//SCO接続Callbackエラー
#define ERROR_BLUEZ_SCO_CONNECT_CB					0x5904
//AT COMMAND送信エラー
#define ERROR_BLUEZ_SEND_ATCMD						0x5905
//RFCOMMデータ受信Callbackエラー
#define	ERROR_BLUEZ_REV_RFCOMM_CB					0x5906

//A2DP
//SIGNALING終了エラー
#define	ERROR_BLUEZ_CLOSE_SIGNALING					0x5A01
//STREAM終了エラー
#define	ERROR_BLUEZ_CLOSE_STREAM					0x5A02
//BUFサイズ設定エラー
#define	ERROR_BLUEZ_SET_BUFSIZE						0x5A03
//BUFサイズ取得エラー
#define	ERROR_BLUEZ_GET_BUFSIZE						0x5A04
//TRANSPORT接続エラー
#define	ERROR_BLUEZ_TRANSPORT_CONNECT				0x5A05
//状態変更エラー
#define	ERROR_BLUEZ_CHANGE_STATE					0x5A06
//L2CAPデータ受信Callbackエラー
#define	ERROR_BLUEZ_REV_L2CAP_CB					0x5A07


//その他
//Dbus初期化エラー
#define ERROR_BLUEZ_DBUS_INIT						0x5B01
//AVRCPデータ設定エラー
#define	ERROR_BLUEZ_AVRCP_SET_DATA					0x5B02
//obexpacket送信エラー
#define	ERROR_BLUEZ_SEND_OBEXPACKET					0x5B03
//状態設定エラー
#define ERROR_BLUEZ_SET_STATE						0x5B04
//チャンネル設定エラー
#define	ERROR_BLUEZ_SET_CHANNEL						0x5B05
//ACLキャンセルエラー
#define	ERROR_BLUEZ_CANCEL_ACL						0x5B06
//認証エラー
#define ERROR_BLUEZ_AUTHORIZATION					0x5B07
//SCO SERVER Callbackエラー
#define	ERROR_BLUEZ_SCO_SERVER_CB					0x5B08
//RFCOMM BTIOエラー
#define	ERROR_BLUEZ_BTIO_RFCOMM						0x5B09
//L2CAP BTIOエラー
#define	ERROR_BLUEZ_BTIO_L2CAP						0x5B0A
//OOM 設定エラー
#define ERROR_BLUEZ_SET_OOM							0x5B0B
//MNGエラー
#define ERROR_BLUEZ_SRC_MNG							0x5B0C
//ペアリングエラー
#define ERROR_BLUEZ_PAIRING							0x5B0D
//HCIイベントフィルター設定エラー
#define ERROR_BLUEZ_HCI_FILTER						0x5B0E
//SDPタイムアウト
#define ERROR_BLUEZ_SDP_TIMEOUT						0x5B0F
//ADAPTERエラー
#define ERROR_BLUEZ_SRC_ADAPTER						0x5B10


//BTIOエラー(BTIOで使用済み)
#define ERROR_BLUEZ_BTIO							 0x5F01

typedef struct drc_bt_method_event_log  {
	unsigned int method;     /** BT DBUSメソッド種別 */
	char bdaddr[6];			 /** BTデバイスアドレス */
} DRC_BT_METHOD_EVENT_LOG;

	
typedef struct drc_bt_signal_event_log  {
	unsigned int signal;     /** BT DBUSシグナル種別 */
	unsigned int result;     /** 通知結果 */
} DRC_BT_SIGNAL_EVENT_LOG;

typedef struct drc_bt_hci_event_log  {
	unsigned int hci_comm;     /** HCIコマンド種別 */
} DRC_BT_HCI_EVENT_LOG;
	
typedef struct drc_bt_mng_hci_signal_log  {
	unsigned int signal;     	/** シグナル種別 */
	unsigned char link_type;    /** リンク種別 */
	unsigned char status;     	/** 状態 */
} DRC_BT_MNG_HCI_SIGNAL_LOG;

typedef struct drc_bt_error_log  {
	unsigned int err_id;     /** エラーＩＤ */
	unsigned int src_id;      /** ソースID	*/
	unsigned int line;       /** 行数		*/
	unsigned int param1;     /** 付加情報1	*/
	unsigned int param2;     /** 付加情報2 	*/
} DRC_BT_ERROR_LOG;

typedef struct drc_bt_connect_cb_event_log  {
	unsigned int callback;     /** BT Connect Callback種別 */
	unsigned int connection;   /** 接続方向 */
} DRC_BT_CONNECT_CB_EVENT_LOG;

typedef struct drc_bt_a2dp_connect_cb_event_log  {
	unsigned int connection;    /** 接続方向*/
	unsigned int first;   		/** 一本目接続 */
	unsigned int second;   		/** 二本目接続 */
} DRC_BT_A2DP_CONNECT_CB_EVENT_LOG;

typedef struct drc_bt_cb_event_log  {
	unsigned int callback;     /** BT  Callback種別 */
} DRC_BT_CB_EVENT_LOG;

typedef struct drc_bt_discovery_event_log  {
	unsigned int event;     /** BT 検索動作 */
	unsigned int param;     /** 検索パラメータ */
} DRC_BT_DISCOVERY_EVENT_LOG;

typedef struct drc_bt_pairing_event_log  {
	unsigned int event;		/** BT ペアリング動作 */
	unsigned char status;	/** 状態 */
	char bdaddr[6];			/** BTデバイスアドレス */
} DRC_BT_PAIRING_EVENT_LOG;

typedef struct drc_bt_init_event_log  {
	unsigned int event;     /** BLUEZ 初期化イベント種別 */
} DRC_BT_INIT_EVENT_LOG;

typedef struct drc_bt_manager_event_log  {
	unsigned int event;		/** BT MANAGER 動作種別 */
	char bdaddr[6];			/** BTデバイスアドレス */
	unsigned int param;		/** 状態等 */
} DRC_BT_MANAGER_EVENT_LOG;

typedef struct drc_bt_confirm_event_log  {
	unsigned int event;		/** Profile 種別 */
	char bdaddr[6];			/** BTデバイスアドレス */
} DRC_BT_CONFIRM_EVENT_LOG;

typedef struct drc_bt_state_event_log  {
	unsigned int event;		/** Profile 種別 */
	char bdaddr[6];			/** BTデバイスアドレス */
	unsigned int state;		/** 状態 */
} DRC_BT_STATE_EVENT_LOG;

typedef struct drc_bt_adapter_event_log  {
	unsigned int event;		/** BT ADAPTER 動作種別 */
	unsigned int param;		/** パラメータ */
} DRC_BT_ADAPTER_EVENT_LOG;

typedef struct drc_bt_did_event_log  {
	unsigned int event;			/** DID 動作種別 */
	unsigned int result;		/** 結果 */
	unsigned int vendorID;		/** ベンダーID */
	unsigned int productID;		/** プロダクトID */
	unsigned int version;		/** バージョン */
} DRC_BT_DID_EVENT_LOG;

typedef struct drc_bt_bdaddr_event_log  {
	unsigned int event;     /** BT動作 */
	unsigned int errId;     /** エラー時のエラーID */
	unsigned int line;      /** ログ出力行 */
	char bdaddr[6];			/** BTデバイスアドレス */
} DRC_BT_BDADDR_EVENT_LOG;

typedef struct drc_bt_chgmsg_event_log  {
	unsigned int event;     /** BT動作 */
	char bdaddr[6];			/** BTデバイスアドレス */
} DRC_BT_CHGMSG_EVENT_LOG;
