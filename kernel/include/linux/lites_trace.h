/**
 * Copyright (c) 2011-2013 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
/**
 * @file lites_trace.h
 *
 * @brief 高速ログドライバ/高速ログライブラリの定義
 */
#ifndef __LITES_TRACE_H
#define __LITES_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* FTEN用ビルド */
#define LITES_FTEN          /* FTEN用高速ログを有効にする */
/* MMダイレコ対応 */
#define LITES_FTEN_MM_DIRECO       /*  MMダイレコ対応を有効にする */

/* ADA用ビルド */
#define LITES_ADA_LOG

/** FUJITSU TEN:2012-11-13 add start */
enum LOG_LEVEL {
	EMERG = 0,
	ALERT,
	CRIT,
	ERROR,
	WARN,
	NOTICE,
	INFO,
	LITES_DEBUG,
};
/** FUJITSU TEN:2012-11-13 add end */

#ifdef LITES_FTEN
#ifdef LITES_FTEN_MM_DIRECO
#define LITES_MAJOR                245
#else /* LITES_FTEN_MM_DIRECO */
#define LITES_MAJOR                241
#endif /* LITES_FTEN_MM_DIRECO */
#define LITES_ACCESS_BYTE_MAX      1024 /** 8 byte is for header */
#define LITES_VAR_ACCESS_BYTE      (0xffffffff)
#define LITES_WRITE_BUF_SIZE       LITES_ACCESS_BYTE_MAX - 8 /** ログヘッダを除く最大サイズ*/
#define LITES_TRACE_WRITE_BUF_SIZE (LITES_ACCESS_BYTE_MAX - 8 - 5 * sizeof(unsigned int)) /** 付加するログの最大サイズ （引いてるのはログヘッダサイズとトレースヘッダ部(boot_id, seq_no, level, trace_id, trace_no)サイズ）*/
#define LITES_READ_BUF_SIZE        8192 /** BUFSIZ */
#define LITES_LEVEL_SIZE           3 /** <n> : n = 0..7 */
#define LITES_LEVEL_NONE           (0xffffffff)
#define LITES_PRINTK_REGION        0x80000000 /** printk出力先のリージョン番号 */
/* FUJITSU -TEN:2012.12.03 mod start. */
// #define LITES_DEVPATH              "/dev/lites/trace" /**  device path for trace. */
#define LITES_DEVPATH              "/dev/lites" /**  device path for trace. */
/* FUJITSU -TEN:2012.12.03 mod end. */
#define LITES_TRACE_REGIONS        (0xffffffff)
#else /* LITES_FTEN */
#define LITES_MAJOR                241
#define LITES_ACCESS_BYTE_MAX      1024 /** 10 byte is for header */
#define LITES_VAR_ACCESS_BYTE      (0xffffffff)
#define LITES_WRITE_BUF_SIZE       LITES_ACCESS_BYTE_MAX - 10
#define LITES_TRACE_WRITE_BUF_SIZE (LITES_ACCESS_BYTE_MAX - 10 - 2 * sizeof(unsigned int))
#define LITES_READ_BUF_SIZE        8192 /** BUFSIZ */
#define LITES_LEVEL_SIZE           3 /** <n> : n = 0..7 */
#define LITES_LEVEL_NONE           (0xffffffff)
#define LITES_PRINTK_REGION        0x80000000 /** printk出力先のリージョン番号 */
#endif /* LITES_FTEN */

#ifndef __KERNEL__

# include <linux/types.h>
# include <linux/ioctl.h> 
# include <sys/types.h>

/** ユーザランドとカーネルで構造体を共有するために型を定義 */
typedef __u64 u64;
typedef __u32 u32;

#endif /* __KERNEL__ */

/** ioctl用コマンドで使用するマジックナンバ */
#define LITES_IOC_MAGIC     LITES_MAJOR

/** ioctl用コマンド */
#define LITES_INIT          _IOR(LITES_IOC_MAGIC,  0, unsigned long)
#define LITES_GET_ATTR      _IOR(LITES_IOC_MAGIC,  1, unsigned long)
#define LITES_CHG_LVL       _IOR(LITES_IOC_MAGIC,  2, unsigned long)
#define LITES_CHG_LVL_ALL   _IOR(LITES_IOC_MAGIC,  3, unsigned long)
#define LITES_INQ_LVL       _IOR(LITES_IOC_MAGIC,  4, unsigned long)
#define LITES_NTFY_SIG      _IOR(LITES_IOC_MAGIC,  5, unsigned long)
#define LITES_STOP_SIG      _IOR(LITES_IOC_MAGIC,  6, unsigned long)
#define LITES_INQ_SIG       _IOR(LITES_IOC_MAGIC,  7, unsigned long)
#define LITES_CLR_SIG_TBL   _IOR(LITES_IOC_MAGIC,  8, unsigned long)
#define LITES_INQ_OVER      _IOR(LITES_IOC_MAGIC,  9, unsigned long)
#define LITES_CLR_DATA      _IOR(LITES_IOC_MAGIC, 10, unsigned long)
#define LITES_CLR_DATA_ZERO _IOR(LITES_IOC_MAGIC, 11, unsigned long)
#define LITES_WRAP          _IOR(LITES_IOC_MAGIC, 12, unsigned long)
#define LITES_BLOCK         _IOR(LITES_IOC_MAGIC, 13, unsigned long)
#define LITES_WRITE         _IOR(LITES_IOC_MAGIC, 14, unsigned long)
#define LITES_WRITE_MULTI   _IOR(LITES_IOC_MAGIC, 15, unsigned long)
#define LITES_DUMP          _IOR(LITES_IOC_MAGIC, 16, unsigned long)
#define LITES_READ          _IOR(LITES_IOC_MAGIC, 17, unsigned long)
#define LITES_READ_SQ       _IOR(LITES_IOC_MAGIC, 18, unsigned long)
#define SYSLOG_DUMP         _IOR(LITES_IOC_MAGIC, 19, unsigned long)
#define LITES_CHG_NR        _IOR(LITES_IOC_MAGIC, 20, unsigned long)
#define LITES_INIT_NG       _IOR(LITES_IOC_MAGIC, 21, unsigned long)
#ifdef LITES_FTEN
#define LITES_STOP          _IOR(LITES_IOC_MAGIC, 22, unsigned long)
#define LITES_RESTART       _IOR(LITES_IOC_MAGIC, 23, unsigned long)
#define MGMT_INFO_DUMP      _IOR(LITES_IOC_MAGIC, 24, unsigned long)
#define LITES_GET_R_RGN_N   _IOR(LITES_IOC_MAGIC, 25, unsigned long)
#define LITES_GET_W_RGN_N   _IOR(LITES_IOC_MAGIC, 26, unsigned long)
#define UBINUX_K_INFO_DUMP  _IOR(LITES_IOC_MAGIC, 27, unsigned long)
#ifdef LITES_FTEN_MM_DIRECO
#define LITES_READ_COMP     _IOR(LITES_IOC_MAGIC, 28, unsigned long)
#endif /* LITES_FTEN_MM_DIRECO */
#endif /* LITES_FTEN */

#define LITES_IOC_DIR  _IOC_DIR(LITES_INIT)
#define LITES_IOC_TYPE _IOC_TYPE(LITES_INIT)
#define LITES_IOC_SIZE _IOC_SIZE(LITES_INIT)

/* FUJITSU-TEN 2013-02-20: ADA_LOG ADD start */
#define LITES_APP_LAYER 1000	// アプリ層(apps)のブロック.
#define LITES_FWK_LAYER 2000	// フレームワーク層(fwk)のブロック.
#define LITES_LIB_LAYER 3000	// ライブラリ層(lib)のブロック.
#define LITES_KNL_LAYER 4000	// カーネル層(knl)のブロック.
#define LITES_BTL_LAYER 5000	// ブートローダ(btl).
#define LITES_TOL_LAYER 6000	// ツール(tol).
/* FUJITSU-TEN 2013-02-20: ADA_LOG ADD end */

//2012.10.31 FTEN ADD start.
#if 0
//リージョングループID定義.
enum REGION_NUM{
	REGION_NOT_USE              = 0, // 未使用.
	REGION_PRINTK               , // 既存printk用グループID.
	REGION_SYSLOG               , // syslog用グループID.
	REGION_EVENT_LS1            ,// LS1通信用グループID.
	REGION_EVENT_MODE           ,// モード管理用グループID.
	REGION_EVENT_TAB            ,// TAB通信用グループID.
	REGION_EVENT_CMDSW          ,// コマンドSW用グループID.
	REGION_EVENT_BT_BLOCK_IF    ,// BTブロックIF用グループID.
	REGION_EVENT_BT_MODULE_IF   ,// BTモジュールIF用グループID.
	REGION_EVENT_MAICON         ,// マイコンIF用グループID.
	REGION_EVENT_MMPIF          ,// MMPIF用グループID.
	REGION_EVENT_MMP_IPOD       ,// MMP(IPOD)用グループID.
	REGION_EVENT_MMP_USB        ,// MMP(USB)用グループID.
	REGION_EVENT_MMP_CDDB       ,// MMP(CDDB)用グループID.
	REGION_EVENT_APL_IF         ,// アプリ連携IF用グループID.
	REGION_EVENT_NAVI_IF        ,// ナビIF用グループID.
	REGION_EVENT_HMI_IF         ,// HMIIF用グループID.
	REGION_EVENT_CDP            ,// CDPデッキ間通信用グループID.
	REGION_EVENT_CAN            ,// CAN通信用グループID.
	REGION_EVENT_MISC           ,// MISC用グループID.
	REGION_TRACE_BATTELY        ,// 電源用グループID.
	REGION_TRACE_MODE_SIG       ,// モード管理/車両信号用グループID.
	REGION_TRACE_AVC_LAN        ,// AVC-LAN通信用グループID.
	REGION_TRACE_KEY            ,// キー用グループID.
	REGION_TRACE_ERROR          ,// エラー用グループID.
	REGION_TRACE_HARD_ERROR     ,// ハードエラー用グループID.
	REGION_TRACE_CAN            ,// CAN用グループID.
	REGION_TRACE_MAKER          ,// メーカ固有ログ用グループID.
	REGION_ANDROID_ADA_MAIN     ,// 車載用Android(Main)ログ用グループID.
	REGION_ANDROID_MAIN         ,// Android既存ログ用グループID.
	REGION_ADA_PRINTK           ,// 車載用kernelログ用グループID.
	REGION_ANDROID_SYSTEM       ,// systemログ用グループID.
	REGION_ANDROID_EVENT        ,// eventログ用グループID.
	REGION_ANDROID_RADIO        ,// radioログ用グループID.
	REGION_ANDROID_DEVICE       ,// deviceState用グループID.
	REGION_ANDROID_OPE_HIS      ,// 操作履歴用グループID.
	REGION_ANDROID_OS_UPDATE    ,// OSバージョンアップ履歴ログ用グループID.
	REGION_ANDROID_APL_INSTALL  ,// アプリインストール履歴用グループID.
	REGION_PROC_SELF_NET_DEV    ,// /proc/self/net/dev用グループID.
	REGION_PROC_SLABINFO        ,// /proc/slabinfo用グループID.
	REGION_PROC_VMSTAT          ,// /proc/vmstat用グループID.
	REGION_PROC_INTERRUPTS       // /proc/interrupts用グループID.
};
#endif
#if 0
/* FUJITSU-TEN 2012-12-04: ADA_LOG ADD start */
//リージョングループID定義.
enum REGION_NUM{
	REGION_NOT_USE              = 0, // 未使用.
	REGION_PRINTK               ,// 既存printk用グループID.
	REGION_ADA_PRINTK           ,// 車載用kernelログ用グループID.
	REGION_SYSLOG               ,// syslog用グループID.
	REGION_EVENT_LOG            ,// LS1通信用グループID.
	REGION_TRACE_BATTELY        ,// 電源用グループID.
	REGION_TRACE_MODE_SIG       ,// モード管理/車両信号用グループID.
	REGION_TRACE_AVC_LAN        ,// AVC-LAN通信用グループID.
	REGION_TRACE_KEY            ,// キー用グループID.
	REGION_TRACE_ERROR          ,// エラー用グループID.
	REGION_TRACE_HARD_ERROR     ,// ハードエラー用グループID.
	REGION_TRACE_CAN            ,// CAN用グループID.
	REGION_TRACE_MAKER          ,// メーカ固有ログ用グループID.
	REGION_TRACE_DEVICE         ,// deviceState用グループID.
	REGION_TRACE_OPE_HIS        ,// 操作履歴用グループID.
	REGION_TRACE_OS_UPDATE      ,// OSバージョンアップ履歴ログ用グループID.
	REGION_TRACE_APL_INSTALL    ,// アプリインストール履歴用グループID.
	REGION_ANDROID_ADA_MAIN     ,// 車載用Android(Main)ログ用グループID.
	REGION_ANDROID_MAIN         ,// Android既存ログ用グループID.
	REGION_ANDROID_SYSTEM       ,// systemログ用グループID.
	REGION_ANDROID_EVENT        ,// eventログ用グループID.
	REGION_ANDROID_RADIO        ,// radioログ用グループID.
	REGION_PROC_SELF_NET_DEV    ,// /proc/self/net/dev用グループID.
	REGION_PROC_SLABINFO        ,// /proc/slabinfo用グループID.
	REGION_PROC_VMSTAT          ,// /proc/vmstat用グループID.
	REGION_PROC_INTERRUPTS       // /proc/interrupts用グループID.
};
/* FUJITSU-TEN 2012-12-04: ADA_LOG ADD end */

/* FUJITSU-TEN 2012-12-04: ADA_LOG ADD start */
enum REGION_EVENT_NUM{
	REGION_EVENT_LS1            =  1,// LS1通信用TトレースID.
	REGION_EVENT_MODE           ,// モード管理用トレースID.
	REGION_EVENT_TAB            ,// TAB通信用トレースID.
	REGION_EVENT_CMDSW          ,// コマンドSW用トレースID.
	REGION_EVENT_BT_BLOCK_IF    ,// BTブロックIF用トレースID.
	REGION_EVENT_BT_MODULE_IF   ,// BTモジュールIF用トレースID.
	REGION_EVENT_MAICON         ,// マイコンIF用トレースID.
	REGION_EVENT_MMPIF          ,// MMPIF用トレースID.
	REGION_EVENT_MMP_IPOD       ,// MMP(IPOD)用トレースID.
	REGION_EVENT_MMP_USB        ,// MMP(USB)用トレースID.
	REGION_EVENT_MMP_CDDB       ,// MMP(CDDB)用トレースID.
	REGION_EVENT_APL_IF         ,// アプリ連携IF用トレースID.
	REGION_EVENT_NAVI_IF        ,// ナビIF用トレースID.
	REGION_EVENT_HMI_IF         ,// HMIIF用トレースID.
	REGION_EVENT_CDP            ,// CDPデッキ間通信用トレースID.
	REGION_EVENT_CAN            ,// CAN通信用トレースID.
	REGION_EVENT_MISC            // MISC用トレースID.
};
/* FUJITSU-TEN 2012-12-04: ADA_LOG ADD end */
#endif
/* FUJITSU-TEN 2013-02-04: ADA_LOG ADD start */
#if 1
//リージョングループID定義.
enum REGION_NUM{
	REGION_NOT_USE              = 0, // 未使用.
	REGION_PRINTK               ,// 既存printk用グループID.
	REGION_ADA_PRINTK           ,// 車載用kernelログ用グループID.
	REGION_SYSLOG               ,// syslog用グループID.
	REGION_EVENT_LOG            ,// イベント用グループID.
	REGION_TRACE_BATTELY        ,// 電源用グループID.
	REGION_TRACE_MODE_SIG       ,// モード管理/車両信号用グループID.
	REGION_TRACE_AVC_LAN        ,// AVC-LAN通信用グループID.
	REGION_TRACE_KEY            ,// キー用グループID.
	REGION_TRACE_ERROR          ,// ソフトエラー用グループID.
	REGION_TRACE_DIAG_ERROR     ,// ダイアグ用ソフトエラー用グループID.
	REGION_TRACE_HARD_ERROR     ,// ハードエラー用グループID.
	REGION_TRACE_DIAG_HARD_ERROR,// ダイアグ用ハードエラー用グループID.
	REGION_TRACE_APL_INSTALL    ,// アプリインストール履歴用グループID.
	REGION_TRACE_OS_UPDATE      ,// OSバージョンアップ履歴ログ用グループID.
	REGION_TRACE_DRIVER         ,// ドライバトレース用グループID.
	REGION_TRACE_MAKER          ,// メーカ固有ログ用グループID.
	REGION_TRACE_WHITELIST      ,// ホワイトリスト用グループID.
	REGION_ANDROID_ADA_MAIN     ,// 車載用Android(Main)ログ用グループID.
	REGION_ANDROID_MAIN         ,// Android既存ログ用グループID.
	REGION_ANDROID_SYSTEM       ,// systemログ用グループID.
	REGION_ANDROID_EVENT        ,// eventログ用グループID.
	REGION_ANDROID_RADIO        ,// radioログ用グループID.
	REGION_PROC_SELF_NET_DEV    ,// /proc/self/net/dev用グループID.
	REGION_PROC_SLABINFO        ,// /proc/slabinfo用グループID.
	REGION_PROC_VMSTAT          ,// /proc/vmstat用グループID.
	REGION_PROC_INTERRUPTS      ,// /proc/interrupts用グループID.
	REGION_PROC_DISK             // eMMC容量トレース用グループID.
};
/* FUJITSU-TEN 2013-02-04: ADA_LOG ADD end */

/* FUJITSU-TEN 2013-02-04: ADA_LOG ADD start */
enum REGION_EVENT_NUM{
	REGION_EVENT_MODE           = 1,// モード管理用トレースID.
	REGION_EVENT_CMDSW          ,// コマンドSW用トレースID.
	REGION_EVENT_BT_BLOCK_IF    ,// BTブロックIF用トレースID.
	REGION_EVENT_MAICON         ,// 外部デバイスIF用トレースID.
	REGION_EVENT_MMPIF          ,// MMPIF用トレースID.
	REGION_EVENT_MMP_IPOD       ,// MMP(IPOD)用トレースID.
	REGION_EVENT_MMP_USB        ,// MMP(USB)用トレースID.
	REGION_EVENT_APL_IF         ,// アプリ連携IF用トレースID.
	REGION_EVENT_NAVI_IF        ,// ナビIF用トレースID.
	REGION_EVENT_HMI_IF         ,// HMIIF用トレースID.
	REGION_EVENT_VOICETAG       ,// VoiceTagログ用トレースID.
	REGION_EVENT_MIRRORLINK     ,// Mirorlink用トレースID.
	REGION_EVENT_APP_MODE       ,// AppMode用トレースID.
	REGION_EVENT_DUET           ,// duetログ用トレースID.
	REGION_EVENT_HTC            ,// HTCモードログ用トレースID.
	REGION_EVENT_REAL_VNC       ,// ReadVNCログ用トレースID.
	REGION_EVENT_GYRO           ,// Gyroセンサ用トレースID.
	REGION_EVENT_PMU_THERMAL    ,// PMU温度センサ用トレースID.
	REGION_EVENT_THERMAL        ,// サーマルセンサ用トレースID.
	REGION_EVENT_GPS_CLOCK      ,// GPS時計用トレースID.
	REGION_EVENT_IOCTL_TRACE    ,// IOCTLコマンド用トレースID.
	REGION_EVENT_USER_DATA      ,// ユーザデータ操作ログ用トレースID.
	REGION_EVENT_WIFI           ,// Wifi通信用トレースID.
	REGION_EVENT_HANDS_FREE     ,// ハンズフリー用トレースID.
	REGION_EVENT_LPA            ,// LPA用トレースID.
	REGION_EVENT_AUDIO          ,// Audio機能用トレースID.
	REGION_EVENT_INSPECTION     ,// 工程検査用トレースID.
	REGION_EVENT_SIRI           ,// Siri用トレースID.
	REGION_EVENT_STSW           ,// ステアリングスイッチ用トレースID.
	REGION_EVENT_TASKMGR        ,// タスクマネージャー用トレースID.
	REGION_EVENT_DIAG           ,// ダイアグ用トレースID.
	REGION_EVENT_CAMERA         ,// CAMERA用トレースID.
	REGION_EVENT_DISC           ,// DISC用トレースID.
	REGION_EVENT_TUNER          ,// TUNER用トレースID.
	REGION_EVENT_HDMI           ,// HDMI用トレースID.
	REGION_EVENT_STATEMGR       ,// 状態管理サービス用トレースID.
	REGION_EVENT_TEMPERATURE    ,// 温度センサ用トレースID.
	REGION_EVENT_EXMOUNT        ,// ExMountService用トレースID.
	REGION_EVENT_BLUETOOTH_DRV  ,// Bluetoothドライバログ種別ID
	REGION_EVENT_USB_DRV        ,// USBドライバログ種別ID
	REGION_EVENT_BTAS           ,// BluetoothAudioServiceログ種別ID
	REGION_EVENT_BCONTACTS      ,// BluetoothContactsServiceログ種別ID
	REGION_EVENT_BSETTING       ,// BluetoothSettingログ種別ID
	REGION_EVENT_STATUSBAR      ,// StatusBarログ種別ID
	REGION_EVENT_GSCREEN        ,// ガードスクリーン/画面OFFログ種別ID
	REGION_EVENT_AUTOMOTIVE     ,// 車両信号ログ種別ID
	REGION_EVENT_COLOR          ,// カラバリログ種別ID
	REGION_EVENT_OPENING        ,// OpeningScreenServiceログ種別ID
	REGION_EVENT_POPUP          ,// PopupServiceログ種別ID
	REGION_EVENT_PRIORITY       ,// アプリ優先度、OOM優先度 ログ種別ID
	REGION_EVENT_DASETTING      ,// A-DAセッティングログ種別ID
	REGION_EVENT_KEYOFFTIMER    ,// キーオフタイマ ログ種別ID
	REGION_EVENT_ANDROID        ,// A-DA Android標準変更機能ログ種別ID
	REGION_EVENT_VOICETAGNATIVE ,// VoiceTagNativeログ種別ID

};
/* FUJITSU-TEN 2013-02-04: ADA_LOG ADD end */
#endif
//2012.10.31 FTEN ADD end.

/* FUJITSU-TEN 2013-05-20: ADA_LOG ADD start */
#define REGION_TRACE_HEADER   -1  // トレースログヘッダ情報操作用グループID.
enum LITES_HEADER_SET {
	SET_HEADER_TSTMP = 1, // unix時刻記録用ID.
	SET_SUSPEND_SQ_S,     // サスペンド処理開始記録用ID.
	SET_SUSPEND_SQ_E,     // サスペンド処理終了記録用ID.
};
/* FUJITSU-TEN 2013-05-20: ADA_LOG ADD end */

/**
 * @struct region_attribute
 * @brief リージョンの属性値を定義
 */
typedef struct region_attribute {
	unsigned int size;             /**< リージョンのサイズ */
	unsigned int region_addr_high; /**< リージョンの開始アドレス（上位32bit） */
	unsigned int region_addr_low;  /**< リージョンの開始アドレス（下位32bit） */
	unsigned int level;            /**< 登録レベル */
	unsigned short wrap;           /**< 上書き有効/無効を示すフラグ */
	unsigned int access_byte;      /**< 単一ログの最大サイズ */
} LITES_REGION_ATTR;

/**
 * @struct zone_attribute
 * @brief LITES_GET_ATTRコマンドで使用するインターフェース
 */
typedef struct zone_attribute {
	unsigned int total_regions;           /**< リージョン属性値の配列のサイズ */
	struct region_attribute *region_attr; /**< リージョン属性値の配列 */
} LITES_ZONE_ATTR;

/**
 * @struct change_level
 * @brief LITES_CHG_LVLコマンドで使用するインターフェース
 */
typedef struct change_level {
	unsigned int region; /**< リージョン番号 */
	unsigned int level;  /**< 登録レベル */
} LITES_CHANGE_LEVEL;

/** LITES_CHG_LVL_ALL */
/** nothing */

/**
 * @struct get_level
 * @brief LITES_INQ_LVLコマンドで使用するインターフェース
 */
typedef struct get_level {
	unsigned int region; /**< リージョン番号 */
	unsigned int *level; /**< 登録レベル格納用領域 */
} LITES_GET_LEVEL;

/**
 * @struct notify_signal
 * @brief LITES_NTFY_SIGで使用するインターフェース
 */
#define LITES_TRGR_LVL_OFF 0
typedef struct notify_signal {
	unsigned int region;   /**< リージョン番号 */
	pid_t pid;             /**< シグナル通知先のPID */
	int sig_no;            /**< シグナル番号 */
	unsigned int trgr_lvl; /**< 残容量のしきい値（書き込み可能な領域がこのサ
				* イズを下回った場合にpidへsig_noのシグナルを通
				* 知する */
} LITES_NOTIFY_SIGNAL;

/** LITES_STOP_SIG */
/** nothing */

/**
 * @struct inq_signal_region
 * @brief LITES_INQ_SIGコマンドで使用するインターフェース
 */
#define LITES_CLR_ON  1 /** クリア有効 */
#define LITES_CLR_OFF 0 /** クリア無効 */
typedef struct inq_signal_region {
	unsigned long num;    /**< リージョン番号を格納する配列のサイズ */
	unsigned int *region; /**< リージョン番号を格納する配列 */
	unsigned int tbl_clr; /**< sig_timesをクリアするかしないかを示すフラグ */
} LITES_INQ_SIGNAL_REGION;

/** LITES_CLR_SIG_TBL */
/** nothing */

/**
 * @struct overflow_detail
 * @brief LITES_INQ_OVERコマンドで使用するインターフェース
 */
typedef struct overflow_detail {
	unsigned int region; /**< リージョン番号を格納する配列のサイズ */
	unsigned int *num;   /**< リージョン番号を格納する配列 */
	unsigned int clr;    /**< cancel_timesをクリアするかしないかを示すフラグ */
} LITES_OVERFLOW_DETAIL;

/**
 * @struct clear_region
 * @brief LITES_CLR_DATAコマンドで使用するインターフェース
 */
typedef struct clear_region {
	unsigned int num;     /**< 配列のサイズ */
	unsigned int *region; /**< リージョン番号の配列 */
} LITES_CLEAR_REGION;

/** LITES_CLR_DATA_ZERO */
/** nothing */

/**
 * @struct wrap_condition
 * @brief LITES_WRAPコマンドで使用するインターフェース
 */
#define LITES_WRAP_ON  1 /** 上書き禁止 */
#define LITES_WRAP_OFF 0 /** 上書き可能 */
typedef struct wrap_condition {
	unsigned int region; /**< リージョン番号 */
	unsigned short cond; /**< 上書き禁止/上書き可能を示すフラグ */
} LITES_WRAP_CONDITION;

/**
 * @struct block_condition
 * @brief LITES_BLOCKコマンドで使用するインターフェース
 */
#define LITES_BLK_ON  1 /** ブロックリード有効 */
#define LITES_BLK_OFF 0 /** ブロックリード無効 */
typedef struct block_condition {
	unsigned int region; /**< リージョン番号 */
	unsigned short cond; /**< ? */
} LITES_BLOCK_CONDITION;

/**
 * @struct lites_trace_header
 * @brief lites_trace_write関数とlites_trace_write_multi関数で使用するログのヘッ
 * ダ
 */
typedef struct lites_trace_header {
#ifdef LITES_FTEN
	unsigned int unixtime;     /**< unix時刻記録領域 */
	unsigned int seq_no;       /**< シーケンス番号 */
#endif /* LITES_FTEN */
	unsigned short level;      /**< 登録レベル */
	unsigned char  i_state;    /**< 内部状態 */
	unsigned char  rsv;        /**< リザーブ領域  */
	unsigned int   trace_no;   /**< トレース番号 */
	unsigned short trace_id;   /**< トレースID */
	unsigned short format_id;  /**< フォーマットID  */
} LITES_TRACE_HEADER;

/**
 * @struct lites_trace_format
 * @brief lites_trace_write関数とlites_trace_write_multi関数経由の場合に使用され
 * るログのフォーマット
 */
typedef struct lites_trace_format {
	struct lites_trace_header *trc_header; /**< ログのヘッダ */
	size_t count;                          /**< 文字列のサイズ */
	const void *buf;                       /**< 書き込む文字列 */
} LITES_TRACE_FORMAT;


#ifdef LITES_FTEN
/** タイムスタンプの領域サイズ定義 */
#define LITES_TIMESTAMP_SIZE       6
#else
/** タイムスタンプの領域サイズ定義 */
#define LITES_TIMESTAMP_SIZE       sizeof(u64)
#endif /* LITES_FTEN */


#ifdef LITES_FTEN
/**
 * @struct lites_log_common_header
 * @brief ログ共通ヘッダ
 */
typedef struct lites_log_common_header {
	unsigned short size;            /**< ログサイズ */
	unsigned char timestamp[LITES_TIMESTAMP_SIZE];  /**< タイプスタンプ jiffies64値への変換はlites_trace_get_jiffies64関数を用いること */
} LITES_LOG_COMMON_HEADER;


/**
 * @struct lites_log_header
 * @brief ログヘッダ
 */
typedef struct lites_log_header {
	unsigned int boot_id;   /**< ブートID */
	unsigned int seq_no;    /**< シーケンス番号 */
	unsigned short level;     /**< トレース登録レベル */
} LITES_LOG_HEADER;


/**
 * @struct lites_trace_record_header
 * @brief トレースのレコードヘッダ
 */
typedef struct lites_trace_record_header {
	struct lites_log_common_header log_common_header;   /**< ログ共通ヘッダ */
	struct lites_trace_header      trace_header;        /**< トレースヘッダ */
} LITES_TRACE_RECORD_HEADER;

/**
 * @struct lites_log_record_header
 * @brief ログ(printk/システムコール)のレコードヘッダ
 */
typedef struct lites_log_record_header {
	struct lites_log_common_header log_common_header;   /**< ログ共通ヘッダ */
	struct lites_log_header        log_header;          /**< ログヘッダ */
} LITES_LOG_RECORD_HEADER;


/**
 * @struct lites_trace_region_header
 * @brief トレースリージョンのヘッダ
 */
typedef struct lites_trace_region_header {
	unsigned int region;        /**< リージョン番号 */
	unsigned int size;          /**< リージョンのサイズ */
	unsigned long long addr;	/**< リージョンの開始物理アドレス */
	unsigned int level;         /**< 登録レベル */
	unsigned int access_byte;   /**< 単一ログのサイズ */
	unsigned int write;         /**< ライトポインタ */
	unsigned int read_sq;       /**< 逐次リードポインタ（参照用リードポインタ */
	unsigned int read;          /**< 一括リードポインタ */
	unsigned int pid;           /**< PID */
	unsigned int sig_no;        /**< シグナル番号 */
	unsigned int trgr_lvl;      /**< 残容量 */
	unsigned int sig_times;     /**< シグナル発行回数 */
	unsigned short block_rd;    /**< ブロックフラグ */
	unsigned short wrap;        /**< 上書き禁止フラグ */
	unsigned int cancel_times;  /**< 上書きキャンセル回数 */
	unsigned int flags;         /**< 各種フラグ */
} LITES_TRACE_REGION_HEADER;


/** リージョン数の定義 */
/** グループID増減時に修正必須 **/
/** FUJITSU TEN 2013-02-04 ADA LOG start */
/** FUJITSU TEN 2013-10-29 ADA LOG mod */
/** グループID数 27 (MAX = 27グループ * 3リージョン + 1(管理用) + 1(旧panic用)) **/
#define LITES_REGIONS_NR_MAX   83
/** FUJITSU TEN 2013-10-29 ADA LOG end */
/** FUJITSU TEN 2013-02-04 ADA LOG end */

/** トレースリージョン最大数の定義
 *  LITES_REGIONS_NR_MAXからprintkリージョンとsyslogリージョンを引いた値
 */
#define LITES_TRACE_REGION_MAX_NUM   LITES_REGIONS_NR_MAX - 2   /** トレースリージョン最大数 */

/** 各種フラグの値 */
#define LITES_FLAG_READ    0x00000001 /** readで読むべきデータが存在する */
#define LITES_FLAG_READ_SQ 0x00000002 /** read_sqで読むべきデータが存在する */

/** syslogリージョンのリージョン番号 */
#define LITES_SYSLOG_REGION          0x80000001 /** syslogリージョンのリージョン番号 */

/** 管理情報関連の定義 */
#define LITES_MGMT_MAGIC_NO        0x53534152   /** ダンプした結果が"RASS" */
/** FUJITSU TEN 2012-11-07 ADA LOG start */
// #define LITES_MGMT_INFO_SIZE       0x00000400   /** リージョン32個分として領域算出 */
#define LITES_MGMT_INFO_SIZE       0x00001000   /** リージョン119個分として領域算出 */
/** FUJITSU TEN 2012-11-07 ADA LOG end */
#define LITES_MGMT_TAIL_MAGIC_NO   0x45534152   /** ダンプした結果が"RASE" */

/**
 * @brief リージョン番号有効判定マクロ
 *
 * printkリージョン 0x80000000           index:0
 * syslogリージョン 0x80000001           index:1
 * traceリージョン  0x0～（登録したトレースリージョン数に依存）
 *
 */
#define LITES_CHECK_REGION_NO(region_no, trace_region_num)	\
	(	(region_no == LITES_PRINTK_REGION ||				\
		 region_no == LITES_SYSLOG_REGION ||				\
		 region_no < trace_region_num		 )	? 1 : 0 )

/**
 * @brief 管理情報内にあるregion_attr_listへ
 * アクセスするためのindex値を求めるマクロ
 *
 * 管理情報内に格納されるリージョン属性は、
 * 以下のindexのところに格納される
 * printkリージョン 0x80000000           index:0
 * syslogリージョン 0x80000001           index:1
 * traceリージョン  0x0～                index:2～
 *
 */
#define LITES_MGMT_LIST_ACCESS_INDEX(region_no)				\
	(	(region_no >= LITES_PRINTK_REGION) ?				\
		(region_no & 0x0000000F) : (region_no + 2)	)


#ifdef LITES_FTEN_MM_DIRECO
/**
 * @struct lites_trace_group_info
 * @brief トレースグループの管理情報を定義
 */
typedef struct lites_trace_group_info {
	u32 region_num;             /**< リージョン数 */
	u32 write_trace_region_no;  /**< 書き込み先トレースリージョン番号 */
	u32 head_region_no;         /**< 先頭リージョン番号 */
	u32 tail_region_no;         /**< 末尾リージョン番号 */
} LITES_TRACE_GROUP_INFO;

/** FUJITSU TEN 2013-02-04 ADA LOG start */
// #define LITES_TRACE_GROUP_MAX		10
#define LITES_TRACE_GROUP_MAX		REGION_PROC_DISK
/** FUJITSU TEN 2013-02-04 ADA LOG end */

/**
 * @brief 管理情報内にあるgroup_listへ
 * アクセスするためのindex値を求めるマクロ
 *
 * group_idの範囲チェックは上位で実施するものとする。
 */
#define LITES_MGMT_GROUP_LIST_ACCESS_INDEX(group_id)	(group_id - 1)


/** 管理情報構造体（956=24+24*32+16*10+4）*/
typedef struct lites_management_info {
	u32 magic_no;               /**< マジックナンバー */
	u32 boot_id;                /**< ブートID */
	u32 seq_no;                 /**< シーケンス番号 */
	u32 trace_region_num;       /**< トレースリージョン数 */
	u32 trace_group_num;        /**< トレースグループ数 */
	u32 region_num;             /**< リージョン数 */
	struct region_attribute region_attr_list[LITES_REGIONS_NR_MAX]; /**< リージョン情報 */
	struct lites_trace_group_info group_list[LITES_TRACE_GROUP_MAX]; /**< トレースグループ情報 */
	u32 tail_magic_no;  /**< 末尾マジックナンバー */
} LITES_MGMT_INFO;

#else /* LITES_FTEN_MM_DIRECO */

/** 管理情報構造体（796=24+24*32+4）*/
typedef struct lites_management_info {
	u32 magic_no;               /**< マジックナンバー */
	u32 boot_id;                /**< ブートID */
	u32 seq_no;                 /**< シーケンス番号 */
	u32 trace_region_num;       /**< トレースリージョン数 */
	u32 write_trace_region_no;  /**< 書き込み先トレースリージョン番号 */
	u32 region_num;             /**< リージョン数 */
	struct region_attribute region_attr_list[LITES_REGIONS_NR_MAX]; /**< リージョン情報 */
	u32 tail_magic_no;  /**< 末尾マジックナンバー */
} LITES_MGMT_INFO;

#endif /* LITES_FTEN_MM_DIRECO */


/**
 * @struct lites_management_info_dump_param
 * @brief MGMT_INFO_DUMPコマンドで使用するインターフェース
 */
typedef struct lites_management_info_dump_param {
	void *buf;           /**< ダンプ用のバッファ */
	size_t bufsize;      /**< バッファのサイズ */
} LITES_MGMT_INFO_DUMP_PARAM;


#ifdef LITES_FTEN_MM_DIRECO
/**
 * @struct lites_read_region_info
 * @brief 退避可能になったリージョン情報
 */
typedef struct lites_read_region_info {
	unsigned int group_id;      /**< 退避可能になったリージョンのグループID */
	unsigned int region_no;     /**< 退避可能になったリージョンのリージョン番号 */
} LITES_READ_REGION_INFO;
#endif /* LITES_FTEN_MM_DIRECO */

/**
 * @struct lites_read_region_number_param
 * @brief LITES_GET_R_RGN_Nコマンドで使用するインターフェース
 */
typedef struct lites_read_region_number_param {
#ifdef LITES_FTEN_MM_DIRECO
	unsigned int *num;                          /**< 退避可能になったリージョン情報の格納先配列のサイズ */
	struct lites_read_region_info *info_list;   /**< 退避可能になったリージョン情報の格納先配列 */
#else /* LITES_FTEN_MM_DIRECO */
	unsigned int *num;                   /**< リージョン番号の格納先配列のサイズ */
	unsigned int *region;                /**< リージョン番号の格納先配列 */
#endif /* LITES_FTEN_MM_DIRECO */
} LITES_READ_REGION_NUMBER_PARAM;

/**
 * @struct lites_write_region_number_param
 * @brief LITES_GET_W_RGN_Nコマンドで使用するインターフェース
 */
typedef struct lites_write_region_number_param {
#ifdef LITES_FTEN_MM_DIRECO
	unsigned int group_id;               /**< グループID */
#endif /* LITES_FTEN_MM_DIRECO */
	unsigned int *region;                /**< リージョン番号の格納先 */
} LITES_WRITE_REGION_NUMBER_PARAM;

/**
 * @struct ubinux_kernel_panic_dump_param
 * @brief UBINUX_K_INFO_DUMPコマンドで使用するインターフェース
 */
typedef struct ubinux_kernel_panic_dump_param {
	off_t offset;           /**< ダンプ開始位置のオフセット値 */
	void *buf;              /**< kernel panic情報を格納するバッファ */
	size_t bufsize;         /**< バッファのサイズ */
	unsigned int panicclr;  /**< ダンプ後にクリアするかしないかを示すフラグ */
} UBINUX_KERNEL_PANIC_DUMP_PARAM;


/**
 * @brief 可変長ログかどうかチェックする
 */
#define is_lites_var_log(access_byte)	((access_byte == LITES_VAR_ACCESS_BYTE))

/**
 * @brief ログのレコードサイズが正しいかどうか判別する
 */
#define lites_check_log_record_size(size)	(size <= LITES_ACCESS_BYTE_MAX)


#endif /* LITES_FTEN */


#ifdef LITES_FTEN
/** ログ共通ヘッダのヘッダサイズ定義 */
#define LITES_COMMON_LOG_HEADER_STRUCT_SIZE	sizeof(struct lites_log_common_header)	/* ログ共通ヘッダのサイズ */
#else
/** ログ共通ヘッダのヘッダサイズ定義 */
#define LITES_COMMON_LOG_HEADER_STRUCT_SIZE	(sizeof(u16) + sizeof(u64))				/* ログ共通ヘッダのサイズ */
#endif /* LITES_FTEN */

/**
 * @struct misc_attribute
 * @brief リージョンの属性値
 */
typedef struct misc_attribute {
	unsigned int size;             /**< リージョンのサイズ */
	unsigned int region_addr_high; /**< リージョンの開始物理アドレス（上位32bit） */
	unsigned int region_addr_low;  /**< リージョンの開始物理アドレス（下位32bit） */
	unsigned int level;            /**< 登録レベル */
	unsigned int access_byte;      /**< 単一ログの最大サイズ */
	pid_t pid;                     /**< シグナル通知先のPID */
	int sig_no;                    /**< シグナル番号 */
	unsigned int trgr_lvl;         /**< 残容量のしきい値 */
	unsigned short block_rd;       /**< ブロックリード有効/無効を示すフラグ */
	unsigned short wrap;           /**< 上書き可能/禁止を示すフラグ */
	unsigned int cancel_no;        /**< 書き込みがキャンセルされた回数 */
} LITES_MISC_ATTRIBUTE;

/**
 * @struct current_attribute
 * @brief LITES_GET_ATTRコマンドのインターフェースで使用するインターフェース
 */
typedef struct current_attribute {
	unsigned int region_no;           /**< リージョン番号 */
	struct misc_attribute *misc_attr; /**< リージョンの属性値 */
} LITES_CURRENT_ATTRIBUTE;

/**
 * @struct lites_write_param
 * @brief LITES_WRITEコマンドで使用するインターフェース
 */
typedef struct lites_write_param {
#ifdef LITES_FTEN_MM_DIRECO
	unsigned int group_id;              /**< グループID */
#else /* LITES_FTEN_MM_DIRECO */
	unsigned int region;                /**< リージョン番号 */
#endif /* LITES_FTEN_MM_DIRECO */
	struct lites_trace_format *trc_fmt; /**< ログのフォーマット */
} LITES_WRITE_PARAM;


/**
 * @struct lites_write_multi_param
 * @brief LITES_WRITE_MULTIコマンドで使用するインターフェース
 */
typedef struct lites_write_multi_param {
	unsigned int num;                   /**< リージョン番号の配列のサイズ */
	unsigned int *pos;                  /**< リージョン番号の配列 */
	struct lites_trace_format *trc_fmt; /**< ログのフォーマットの配列 */
} LITES_WRITE_MULTI_PARAM;

/**
 * @struct lites_dump_param
 * @brief LITES_DUMPコマンドで使用するインターフェース
 */
typedef struct lites_dump_param {
	unsigned int region; /**< リージョン番号 */
	off_t offset;        /**< ダンプ位置のオフセット値（リージョンの先頭がベー
			      *   ス） */
	void *buf;           /**< ダンプ用のバッファ */
	size_t count;        /**< バッファのサイズ */
} LITES_DUMP_PARAM;

/**
 * @struct lites_read_param
 * @brief LITES_READコマンドで使用するインターフェース
 */
#define LITES_ERASE_OFF 0 /** 消去無効 */
#define LITES_ERASE_ON  1 /** 消去有効 */
typedef struct lites_read_param {
	unsigned int region; /**< リージョン番号 */
	off_t offset;        /**< 読み込み位置のオフセット値（readがベース） */
	void *buf;           /**< 読み込み用のバッファ */
	size_t count;        /**< バッファのサイズ */
	unsigned char erase; /**< 読み込み後にログを消去するかを示すフラグ */
} LITES_READ_PARAM;

/**
 * @struct lites_read_sq_param
 * @brief LITES_READ_SQコマンドで使用するインターフェース
 */
typedef struct lites_read_sq_param {
	unsigned int region; /**< リージョン番号 */
	off_t offset;        /**< 読み込み位置のオフセット値（ */
	void *buf;           /**< 読み込み用のバッファ */
	size_t count;        /**< バッファのサイズ */
} LITES_READ_SQ_PARAM;

/**
 * @struct region_attribute_ng
 * @brief リージョンの属性値を定義
 */
typedef struct region_attribute_ng {
	unsigned int region;      /**< リージョン番号 */
	unsigned int size;        /**< リージョンのサイズ */
	u64 addr;                 /**< 物理アドレス */
	unsigned int level;       /**< 登録レベル */
	unsigned int access_byte; /**< 単一ログのサイズ */
	unsigned short wrap;      /**< 上書き可能/禁止を示すフラグ */
	unsigned short block_rd;  /**< ブロックリード有効/無効を示すフラグ */
} LITES_REGION_ATTR_NG;

#ifdef __KERNEL__

#ifdef LITES_FTEN_MM_DIRECO
extern int lites_trace_write(unsigned int group_id, struct lites_trace_format *trc_fmt);
#else /* LITES_FTEN_MM_DIRECO */
extern int lites_trace_write(unsigned int region, struct lites_trace_format *trc_fmt);
#endif /* LITES_FTEN_MM_DIRECO */
extern int lites_trace_write_multi(unsigned int num, unsigned int *pos, struct lites_trace_format *trc_fmt);

/** vprintk呼び出し用マクロ */
# define call_vprintk(fmt, args...)        vprintk_caller[vprintk_caller_index].vprintk(fmt, ##args)
# define call_lites_vprintk(fmt, args...)  vprintk_caller[1].vprintk(fmt, ##args)
# define call_native_vprintk(fmt, args...) vprintk_caller[0].vprintk(fmt, ##args)

/**
 * @struct vprintk_caller_t
 * @brief vprintk用構造体
 */
struct vprintk_caller_t {
        asmlinkage int (*vprintk)(const char *fmt, va_list args);
};

/** vprintkの配列 */
extern struct vprintk_caller_t vprintk_caller[];
extern int vprintk_caller_index;

#else /* __KERNEL__ */
/** ライブラリの戻り値 */
# define LITES_NORMAL 0
# define LITES_ERROR  -1

#ifdef LITES_FTEN
/** ライブラリが提供するAPIのプロトタイプ宣言 */

/**
 * @brief jiffies64値取得関数
 *        lites_log_common_header構造体内のtimestampからjiffies64値を算出する
 *
 * @param[in]  log        lites_log_common_header構造体のアドレス
 *
 * @return jiffies64値
 */
extern unsigned long long lites_trace_get_jiffies64(struct lites_log_common_header *log);

/**
 * @brief トレースリージョン初期化関数
 *
 * @param[in]  fd         /dev/lites/traceのファイルディスクリプタ
 * @param[in]  zone_attr  zone_attribute構造体
 *
 * @return LITES_NORMAL トレースリージョン初期化に成功した
 * @retval LITES_ERROR  トレースリージョン初期化に失敗した
 * 初期化に失敗
 */
extern int lites_trace_init(int fd, struct zone_attribute *zone_attr);

/**
 * @brief システム内のロギング停止関数
 *
 * @param[in]  fd      /dev/lites/traceのファイルディスクリプタ
 *
 * @retval LITES_NORMAL ロギング停止に成功した
 * @retval LITES_ERROR  ロギング停止に失敗した
 */
extern int lites_trace_stop(int fd);

/**
 * @brief システム内のロギング再開関数
 *
 * @param[in]  fd      /dev/lites/traceのファイルディスクリプタ
 *
 * @return LITES_NORMAL ロギング再開に成功した
 * @retval LITES_ERROR  ロギング再開に失敗した
 */
extern int lites_trace_restart(int fd);

#ifdef LITES_FTEN_MM_DIRECO
/**
 * @brief トレースログ書き込み関数
 *
 * @param[in]  fd       /dev/lites/traceのファイルディスクリプタ
 * @param[in]  group_id 書き込み先トレースグループのID
 * @param[in]  trc_fmt  lites_trace_fotmat構造体
 *
 * @retval 0以上        書き込んだトレースログサイズ（ログ共通ヘッダのサイズは除く）
 * @retval LITES_ERROR  トレースが開始されていない、書き込みに失敗した
 */
extern int lites_trace_write(int fd, unsigned int group_id, struct lites_trace_format *trc_fmt);
#else /* LITES_FTEN_MM_DIRECO */
/**
 * @brief トレースログ書き込み関数
 *
 * @param[in]  fd      /dev/lites/traceのファイルディスクリプタ
 * @param[in]  trc_fmt lites_trace_fotmat構造体
 *
 * @retval 0以上        書き込んだトレースログサイズ（ログ共通ヘッダのサイズは除く）
 * @retval LITES_ERROR  トレースが開始されていない、書き込みに失敗した
 */
extern int lites_trace_write(int fd, struct lites_trace_format *trc_fmt);
#endif /* LITES_FTEN_MM_DIRECO */

/**
 * @brief リージョンのデータクリア関数（デバッグ用）
 *
 * @param[in]  fd      /dev/lites/traceのファイルディスクリプタ
 * @param[in]  region  リージョン番号
 *
 * @retval LITES_NORMAL コマンドが成功した
 * @retval LITES_ERROR  不正なアドレスを指定した
 * 指定したリージョン番号に対応するリージョンが存在しない
 */
extern int lites_trace_clear(int fd, unsigned int region);

/**
 * @brief リージョンのデータゼロクリア関数（デバッグ用）
 *
 * @param[in]  fd      /dev/lites/traceのファイルディスクリプタ
 * @param[in]  region  リージョン番号
 *
 * @retval LITES_NORMAL コマンドが成功した
 * @retval LITES_ERROR  不正なアドレスを指定した
 * 指定したリージョン番号に対応するリージョンが存在しない
 */
extern int lites_trace_clear_zero(int fd, unsigned int region);

/**
 * @brief リージョンのレベル変更関数
 *
 * @param[in]  fd      /dev/lites/traceのファイルディスクリプタ
 * @param[in]  region  リージョン番号
 * @param[in]  level   レベル
 *
 * @retval LITES_NORMAL コマンドが成功した
 * @retval LITES_ERROR  レベルの値が不正である、不正なアドレスを指定した
 * 指定したリージョン番号に対応するリージョンが存在しない
 */
extern int lites_trace_change_level(int fd, unsigned int region, unsigned int level);

#ifdef LITES_FTEN_MM_DIRECO
/**
 * @brief 退避（読み出し）可能なリージョン番号の取得関数
 *
 * @param[in]       fd      /dev/lites/traceのファイルディスクリプタ
 * @param[in/out]   num     退避可能になったリージョン情報を格納する配列のサイズ/格納数
 * @param[in/out]   info    退避可能になったリージョン情報を格納する配列
 *
 * @retval LITES_NORMAL 取得が成功した
 * @retval LITES_ERROR  異常が発生した
 */
extern int lites_trace_get_read_region_number(int fd, unsigned int *num, struct lites_read_region_info *info);
#else /* LITES_FTEN_MM_DIRECO */
/**
 * @brief 退避（読み出し）可能なリージョン番号の取得関数
 *
 * @param[in]       fd      /dev/lites/traceのファイルディスクリプタ
 * @param[in/out]   num     退避可能になったリージョン番号を格納する配列のサイズ
 * @param[in/out]   region  退避可能になったリージョン番号を格納する配列
 *
 * @retval LITES_NORMAL 取得が成功した
 * @retval LITES_ERROR  異常が発生した
 */
extern int lites_trace_get_read_region_number(int fd, unsigned int *num, unsigned int *region);
#endif /* LITES_FTEN_MM_DIRECO */

/**
 * @brief ログの読み込み用関数
 *        【注意事項】
 *        トレースログ書き込み先のリージョンの読み出しを実施する場合は、
 *        読み出し中にログ出力側からの更新があったりするので、十分な注意が必要である。
 *        lites_trace_stop関数でロギングを停止してから、実施すること。
 *
 * @param[in]       fd      /dev/lites/traceのファイルディスクリプタ
 * @param[in]       region  読み込みするリージョン番号
 * @param[in]       offset  読み込み位置のオフセット値
 * @param[in/out]   buf     トレースを格納するバッファ
 * @param[in]       count   バッファのサイズ
 * @param[in]       erase   読み込み後にクリアするかしないかを示すフラグ
 *
 * @retval 0以上        読み込んだログのサイズ。0の場合は、リージョン内に読み込むログが無い。
 * @retval LITES_ERROR  ロギングが開始されていない、読み込みに失敗した
 */
extern ssize_t lites_trace_read(int fd, unsigned int region, off_t offset, void *buf, size_t count, unsigned char erase);

#ifdef LITES_FTEN_MM_DIRECO
/**
 * @brief 読み出し完了設定関数
 *
 * @param[in]       fd      /dev/lites/traceのファイルディスクリプタ
 * @param[in]       region  読み出し完了扱いとするリージョン番号
 *
 * @retval LITES_NORMAL 正常終了
 * @retval LITES_ERROR  ファイルディスクリプタ不正/指定したリージョンが存在しない
 */
extern int lites_trace_read_completed(int fd, unsigned int region);
#endif /* LITES_FTEN_MM_DIRECO */

#ifdef LITES_FTEN_MM_DIRECO
/**
 * @brief トレースログの書き込み先リージョン番号の取得関数
 *        【注意事項】
 *        トレースログ書き込み先のリージョンの読み出しを実施する場合は、
 *        読み出し中にログ出力側からの更新があったりするので、十分な注意が必要である。
 *        lites_trace_stop関数でロギングを停止してから、実施すること。
 *
 * @param[in]       fd        /dev/lites/traceのファイルディスクリプタ
 * @param[in]       group_id  グループのID
 * @param[in/out]   region    トレースログの書き込み先リージョン番号を格納先
 *
 * @retval LITES_NORMAL 取得が成功した
 * @retval LITES_ERROR  不正なアドレスを指定した
 */
extern int lites_trace_get_write_region_number(int fd, unsigned int group_id, unsigned int *region);
#else /* LITES_FTEN_MM_DIRECO */
/**
 * @brief トレースログの書き込み先リージョン番号の取得関数
 *        【注意事項】
 *        トレースログ書き込み先のリージョンの読み出しを実施する場合は、
 *        読み出し中にログ出力側からの更新があったりするので、十分な注意が必要である。
 *        lites_trace_stop関数でロギングを停止してから、実施すること。
 *
 * @param[in]       fd      /dev/lites/traceのファイルディスクリプタ
 * @param[in/out]   region  トレースログの書き込み先リージョン番号を格納先
 *
 * @retval LITES_NORMAL 取得が成功した
 * @retval LITES_ERROR  不正なアドレスを指定した
 */
extern int lites_trace_get_write_region_number(int fd, unsigned int *region);
#endif /* LITES_FTEN_MM_DIRECO */


// 以降、内部用
/**
 * @brief 各種コマンド呼び出し用関数（内部用）
 *
 * @param[in]  fd      /dev/lites/traceのファイルディスクリプタ
 * @param[in]  cmd     コマンド
 * @param[in]  ...     コマンドの引数（ひとつのアドレス）
 *
 * @retval LITES_NORMAL コマンドが成功した
 * @retval LITES_ERROR  トレースが開始されていない、非対応のコマンドが指定された、
 * コマンドが失敗した
 */
extern int lites_trace_ioctl(int fd, unsigned int cmd, ...);

/**
 * @brief リージョンのダンプ関数（内部用）
 *
 * @param[in]       fd      /dev/lites/traceのファイルディスクリプタ
 * @param[in]       region  ダンプするリージョン番号
 * @param[in]       offset  ダンプ開始位置のオフセット値
 * @param[in/out]   buf     格納するバッファ
 * @param[in]       count   バッファのサイズ
 *
 * @retval 0以上    ダンプが成功した
 * @retval -1       異常が発生した
 */
extern ssize_t lites_trace_dump(int fd, unsigned int region, off_t offset, void *buf, size_t count);

/**
 * @brief 管理情報のダンプ関数（内部用）
 *
 * @param[in]  fd         /dev/lites/traceのファイルディスクリプタ
 * @param[in]  mgmt_info  lites_management_info構造体
 *
 * @retval LITES_NORMAL コマンドが成功した
 * @retval LITES_ERROR  不正なアドレスを指定した
 * 指定したリージョン番号に対応するロギング領域が存在しない
 * 管理情報のダンプに失敗した
 */
extern int lites_management_info_dump(int fd, struct lites_management_info *mgmt_info);

/**
 * @brief 管理情報のダンプ関数（内部用）
 *
 * @param[in]  fd         /dev/lites/traceのファイルディスクリプタ
 * @param[in]  mgmt_info  lites_management_info構造体
 *
 * @retval LITES_NORMAL コマンドが成功した
 * @retval LITES_ERROR  不正なアドレスを指定した
 * 指定したリージョン番号に対応するロギング領域が存在しない
 * 管理情報のダンプに失敗した
 */
extern int lites_management_info_dump(int fd, struct lites_management_info *mgmt_info);

#else /* LITES_FTEN */
/** ライブラリが提供するAPIのプロトタイプ宣言 */
extern int lites_trace_start(void);
extern int lites_trace_stop(void);
extern int lites_trace_write(unsigned int region, struct lites_trace_format *trc_fmt);
extern int lites_trace_write_multi(unsigned int num, unsigned int *pos, struct lites_trace_format *trc_fmt);
extern ssize_t lites_trace_dump(unsigned int region, off_t offset, void *buf, size_t count);
extern ssize_t lites_trace_read(unsigned int region, off_t offset, void *buf, size_t count, unsigned char erase);
extern ssize_t lites_trace_read_sq(unsigned int region, off_t offset, void *buf, size_t count);
extern int lites_trace_ioctl(unsigned int cmd, ...);
#endif /* LITES_FTEN */

#endif /* __KERNEL__ */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LITES_TRACE_H */
