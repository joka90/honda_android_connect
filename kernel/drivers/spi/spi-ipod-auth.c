/*--------------------------------------------------------------------------*/
/* COPYRIGHT(C) FUJITSU TEN LIMITED 2012-2014                               */
/*--------------------------------------------------------------------------*/
/****************************************************************************
*																			*
*		シリーズ名称	: A-DA												*
*		マイコン名称	: tegra3											*
*		オーダー名称	: 													*
*		ブロック名称	: kernel driver										*
*		DESCRIPTION		: iPod認証ドライバ									*
*		ファイル名称	: spi-ipod-auth.c									*
*																			*
*		Copyright(C) 2012 FUJITSU TEN LIMITED								*
*																			*
*		『エントリープログラム』											*
*			fs_ipa_init ............ ドライバ初期化							*
*			fs_ipa_exit ............ ドライバ終了							*
*			fs_ipa_probe ........... ドライバ生成							*
*			fs_ipa_remove........... ドライバ削除							*
*			fs_ipa_open ............ ドライバオープン						*
*			fs_ipa_release.......... ドライバクローズ						*
*			fs_ipa_suspend.......... ドライバサスペンド						*
*			fs_ipa_resume .......... ドライバリジューム						*
*			fs_ipa_compat_ioctl..... ドライバIOCONTROL						*
*			fs_ipa_ioctl ........... ドライバIOCONTROL						*
*			fs_ipa_sudden_device_poweroff..... 高速終了エントリ				*
*			fs_ipa_reinit .......... 高速起動エントリ						*
*		『SPI送受信』														*
*			fc_ipa_syncRead	.........SPI受信								*
*			fc_ipa_syncWrite.........SPI送信								*
*			fc_ipa_sync..............SPI送受信								*
*			fc_ipa_complete..........受信完了								*
*		『タイマー』														*
*			fc_ipa_timer_init.......タイマ初期化							*
*			fc_ipa_timer_callback...タイマコールバック						*
*			fc_ipa_timer_start......タイマスタート							*
*			fc_ipa_timer_cancel.....タイマキャンセル						*
*		『その他のサブルーチン』											*
*			fc_ipa_show_adapter_name..アダプタネーム生成					*
*			fc_ipa_concreate..........生成処理								*
*			fc_ipa_decreate...........終了後処理							*
*			fc_ipa_workThread........ ワーカースレッド						*
*			fc_ipa_Send.............. IPOD認証送信							*
*			fc_ipa_Recv.............. IPOD認証受信							*
*			fc_ipa_SendCmdAdr........ コマンド送信							*
*			fc_ipa_prepare_communication.. 通信事前処理						*
*			fc_ipa_init.............. ポートリセット						*
*			fc_ipa_set_sfio...........SFIO設定処理							*
*			fc_ipa_setCS............. CS#制御								*
*			fc_ipa_checkPreSuspend... サスペンド判定						*
*			fc_ipa_set_gpio_output... GPIO OUT設定							*
*			fc_ipa_gpio_request_str.. gpio登録文字列生成					*
* 	【Note】 																*
* 		1SではSW_BUDET#の割り込みが発生していたが2S以降発生しなくなった 	*
* 		2SよりSW_BUDET#のH/W変更が入っている。kernelに対して高速起動対応 	*
* 		が行われるが、原因は不明。 											*
* 		SW_BUDET#をGPIO INPUTにて読み込んでも一定してLOWが取得され判断できない
* 		SW_BUDET#をGPIO INPUTに設定するとIpod認証ドライバアクセス中にi2Cの 	*
* 		Warnningが大量に発生し、Ipod認証チップの読み書き動作を阻害 			*
****************************************************************************/
/*--------------------------------------------------------------------------*/
/* 	include 																*/
/*--------------------------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/pm_runtime.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_ipodauth.h>
#include <linux/spi-tegra.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <linux/compat.h>

#include <linux/device.h>
#include <mach/clk.h>
#include <mach/delay.h>
#include <asm/uaccess.h>
#include "../../arch/arm/mach-tegra/gpio-names.h"
#include <linux/earlydevice.h>
#include <linux/budet_interruptible.h>

/*--------------------------------------------------------------------------*/
/* 	define 																	*/
/*--------------------------------------------------------------------------*/
#define IPODAUTH_MAJOR			major_no
#define N_SPI_MINORS			32	/* ... up to 256 */
#define DRV_NAME "spi-ipod-auth"

//#define IPODAUTHDEBUG		/* for debug */
#define IF_GPIO			1	/* GPIO制御 */
#define IF_SPI_TEGRA	2	/* SPI 		*/
#define INTERFACE		IF_GPIO

/* PIN assign */
#define IPOD_CS			TEGRA_GPIO_PW0
#define IPOD_TXD		TEGRA_GPIO_PB2
#define IPOD_RXD		TEGRA_GPIO_PC6
#define IPOD_CLK		TEGRA_GPIO_PZ3
#define IPOD_RESET		TEGRA_GPIO_PD0

#define ACTIVE		1
#define INACTIVE	0
#define HIGH		1
#define LOW			0
#define TRUE		1
#define FALSE		0
/* return code */
#define ESUCCESS	0
/* message que syncronize */
#define MSG_WAIT	0
#define MSG_DONE	1

#define IPD_DATA_MAXLEN				160
#define IPD_COMMAND_SIZE			2
#define IPD_TIMER_SOMI_READY		1000	/* 50us->1ms iPod制御仕様(03版)*/
#define IPD_TIMER_SOMI_RELEASE		1000	/* 50us->1ms iPod制御仕様(03版)*/
#define IPD_TIMER_nSS_DEASSERT		1000	/* 300us->1ms iPod制御仕様(03版)*/
#define IPD_TIMER_WARM_RESET		1000 	/* 1ms */
#define IPD_CLKDELAY				__delay(5800)	/* (1/75K)=0.0000133/2=6.6us =6.8*/

/*--------------------------------------------------------------------------*/
/* 	macro 																	*/
/*--------------------------------------------------------------------------*/
#ifdef IPODAUTHDEBUG
	#define LOGD(fmt,x...)	printk(KERN_INFO DRV_NAME ":" fmt, ##x)
	#define LOG_CHANGESTATE(iad,state)		(printk(KERN_INFO DRV_NAME "%s :[change state] %s -> %s\n", iad->id, \
											ipodauth_fsm[iad->fsm.state].str,ipodauth_fsm[state].str))
	#define LOGT(fmt,x...)	printk(KERN_INFO DRV_NAME ":" fmt, ##x)
	#define LOGI(fmt,x...)	printk(KERN_INFO DRV_NAME ":" fmt, ##x)
	#define LOGW(fmt,x...)	printk(KERN_WARNING DRV_NAME ":" fmt, ##x)
	#define LOGE(fmt,x...)	printk(KERN_ERR DRV_NAME ":" fmt, ##x)
#else
	#define LOGD(fmt,x...)
	#define LOGT(fmt,x...)
	#define LOG_CHANGESTATE(iad,state)
	#define LOGI(fmt,x...)	
	#define LOGW(fmt,x...)	ADA_LOG_WARNING(DRV_NAME, " %s(%d):" fmt,	__FUNCTION__,	__LINE__, 	##x )
	#define LOGE(fmt,x...)	ADA_LOG_ERR(DRV_NAME, " %s(%d):" fmt,	__FUNCTION__,	__LINE__, 	##x )
#endif /* IPODAUTHDEBUG */


#define UNUSED(x) (void)(x)
#define MS2NS(x)	(x*1000*1000)
#define RANGE(min,x,max) ((min<x && x<max)?1:0)
#define ID_DEFINE(x) x , #x

/*--------------------------------------------------------------------------*/
/* 	grobal value															*/
/*--------------------------------------------------------------------------*/
static DECLARE_BITMAP(minors, N_SPI_MINORS);
static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);
static DEFINE_MUTEX(device_if_lock);
static struct class *ipodauth_class;
static int major_no=0;

static bool	pre_suspend_flg=FALSE;	/* 高速起動対応*/
/*--------------------------------------------------------------------------*/
/* 	enum 																	*/
/*--------------------------------------------------------------------------*/
enum timer_id{
	TIMER_ID_T1 =0,						/* T1(60ms)				*/
	TIMER_ID_T2,  						/* T2(30ms)				*/
	TIMER_ID_T3,						/* T3(1ms)				*/
	TIMER_ID_MAX
};
enum worker_message_type{
	WMID_BUDET_HIGH,					/* SW_BUDET 			*/
	WMID_TIMER_T1,						/* TIMER1 				*/
	WMID_TIMER_T2,						/* TIMER2				*/
	WMID_TIMER_T3,						/* TIMER3				*/
	WMID_MAX
};

enum ipodauth_state{
	IPODAUTH_STATE_NONE = 0,			/* 状態なし 			*/
	IPODAUTH_STATE_INIT,				/* 初期化状態 			*/
	IPODAUTH_STATE_BEGIN,				/* 開始 				*/
	IPODAUTH_STATE_IDLE,				/* アイドル				*/
	IPODAUTH_STATE_RUN,					/* 実行 				*/
	IPODAUTH_STATE_MAX
};
enum ipodauth_retcode{
	IPODAUTH_NONE	= 0,				/* なし 		*/
	IPODAUTH_COMP	= 1,				/* 完了 		*/
	IPODAUTH_ERR	=-1,				/* エラー 		*/
	IPODAUTH_PRE_SUSPEND=-4				/* サスペンド 	*/
};

/*--------------------------------------------------------------------------*/
/* 	struct 																	*/
/*--------------------------------------------------------------------------*/
/* タイマ コンフィグ*/
typedef	struct{
	enum timer_id	id;
	char			str[32];
	int				ms;
}ST_IPA_TIMER_CONFIG;
/* タイマ */
typedef	struct{
	void*					pthis;
    int 					index;
	ST_IPA_TIMER_CONFIG		config;
    struct hrtimer 			timer;
}ST_IPA_TIMER_SET;
/* 状態遷移 */
typedef	struct{
	enum ipodauth_state state;
	char			str[32];
}ST_IPA_IPODAUTH_FSM;
/* キュー */
typedef	struct{
	wait_queue_head_t		waitQue;
	bool					waitting;
}ST_IPA_WAIT_QUE;
/* ワーカースレッド */
typedef	struct{
	struct	work_struct 		workq;
	enum worker_message_type	Msgtype;
	void*						pthis;
}ST_IPA_WORKER_INFO;

/* インスタンス */
typedef	struct{
	dev_t					devt;
	spinlock_t				spi_lock;
	struct spi_device		*spi;
	struct list_head		device_entry;
	struct tegra_spi_device_controller_data cdata;

	ST_IPA_WAIT_QUE			waitRunQue;
	ST_IPA_WAIT_QUE			waitProcessQue;
	struct workqueue_struct *workqueue;
	ST_IPA_WORKER_INFO*		worker_info[WMID_MAX];
	
	ST_IPA_TIMER_SET		timer_set[TIMER_ID_MAX];
	char					id[8];
	ST_IPA_IPODAUTH_FSM		fsm;
	struct early_device 	ed;
	unsigned char			users;
}ST_IPA_DATA;

/*--------------------------------------------------------------------------*/
/* 	table 																	*/
/*--------------------------------------------------------------------------*/
static const ST_IPA_TIMER_CONFIG timer_table[]={		  /*					min typ	max	(ms)	*/
	{ID_DEFINE(TIMER_ID_T1)				,60		},/* T1(60ms)					60	-	-			*/
	{ID_DEFINE(TIMER_ID_T2)				,35		},/* T2(35ms)					30	-	-			*/
					/* 2013/03/20 少し余裕を持たせてほしいとFTEN殿より依頼 +5ms */
	{ID_DEFINE(TIMER_ID_T3)				,1		} /* T3(1ms)					1	0	0			*/
};
static const ST_IPA_IPODAUTH_FSM ipodauth_fsm[]={
	{ID_DEFINE(IPODAUTH_STATE_NONE)},
	{ID_DEFINE(IPODAUTH_STATE_INIT)},
	{ID_DEFINE(IPODAUTH_STATE_BEGIN)},
	{ID_DEFINE(IPODAUTH_STATE_IDLE)},
	{ID_DEFINE(IPODAUTH_STATE_RUN)},
	{ID_DEFINE(IPODAUTH_STATE_MAX)}
};

/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_timer_cancel									*
*																			*
*		DESCRIPTION	: タイマーキャンセル									*
*																			*
*		PARAMETER	: IN  : *iad : コンテキスト								*
*																			*
*					  IN  : タイマID										*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ipa_timer_cancel(ST_IPA_DATA* iad,enum timer_id id)
{
	/*--------------------------------------*/
	/*  タイマキャンセル					*/
	/*--------------------------------------*/
	hrtimer_cancel(&iad->timer_set[id].timer);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_timer_start									*
*																			*
*		DESCRIPTION	: タイマースタート										*
*																			*
*		PARAMETER	: IN  : *iad : コンテキスト								*
*																			*
*					  IN  : タイマID										*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ipa_timer_start(ST_IPA_DATA* iad,enum timer_id id)
{
	/*--------------------------------------*/
	/*  タイマスタート						*/
	/*--------------------------------------*/
	 hrtimer_start(&iad->timer_set[id].timer, 
	 				ktime_set( 0, MS2NS(iad->timer_set[id].config.ms)), 
	 				HRTIMER_MODE_REL);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_timer_callback										*
*																			*
*		DESCRIPTION	: タイマーコールバック									*
*																			*
*		PARAMETER	: IN  : *timer : タイマデータ							*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
static enum hrtimer_restart fc_ipa_timer_callback(struct hrtimer *timer)
{
	enum hrtimer_restart ret= HRTIMER_NORESTART;
	ST_IPA_TIMER_SET* ts;
	
	/*--------------------------------------*/
	/*  タイマデータ取得					*/
	/*--------------------------------------*/
	ts = container_of(timer, ST_IPA_TIMER_SET, timer);
	if(ts){
		ST_IPA_DATA* iad=(ST_IPA_DATA*)ts->pthis;	/* Instance取得 */
		/*--------------------------------------*/
		/*  ワーカースレッドへTimerイベント送信	*/
		/*--------------------------------------*/
		LOGD("[%d] %s(%d ms) this 0x%08x",ts->index,ts->config.str,ts->config.ms,ts->pthis);
		queue_work(iad->workqueue, (struct work_struct*)iad->worker_info[WMID_TIMER_T1+ts->config.id]);
	}else
		LOGE("%s:(%d) : cannot get object\n",__FUNCTION__,__LINE__);
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_timer_init										*
*																			*
*		DESCRIPTION	: タイマ初期化処理										*
*																			*
*		PARAMETER	: IN  : *iad : コンテキスト								*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
static int fc_ipa_timer_init(ST_IPA_DATA* iad)
{
	int status=ESUCCESS;
	int i;
	/*--------------------------------------*/
	/*  タイマ初期化						*/
	/*--------------------------------------*/
	for(i=0;i<ARRAY_SIZE(iad->timer_set);++i){
		iad->timer_set[i].pthis = (void*)iad;
		iad->timer_set[i].index = i;
		iad->timer_set[i].config.id=timer_table[i].id;
		iad->timer_set[i].config.ms=timer_table[i].ms;
		hrtimer_init(&iad->timer_set[i].timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		iad->timer_set[i].timer.function = fc_ipa_timer_callback;
	}
	return status;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_gpio_request_str								*
*																			*
*		DESCRIPTION	: gpio登録文字列生成									*
*																			*
*		PARAMETER	: IN  : *iad : コンテキスト								*
*																			*
*					  OUT : *out : 連結データ格納先							*
*																			*
*					  IN  : *str : ユニーク文字列							*
*																			*
*					  RET : ACTIVE	/ INACTIVE								*
*																			*
****************************************************************************/
static inline char* fc_ipa_gpio_request_str(ST_IPA_DATA *iad,char* out,char* str)
{
	sprintf(out,"%s%s%s",DRV_NAME,iad->id,str);
	return out;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_set_gpio_output								*
*																			*
*		DESCRIPTION	: GPIO OUT設定											*
*																			*
*		PARAMETER	: IN  : *iad : コンテキスト								*
*																			*
*					  IN  : ballname : ポート名								*
*																			*
*					  IN  : hl : High / low									*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline void fc_ipa_set_gpio_output(ST_IPA_DATA *iad,unsigned long ballname,unsigned long hl)
{
	tegra_gpio_enable(ballname);
	gpio_direction_output(ballname, hl);
	gpio_set_value(ballname, hl);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_check_irqflag									*
*																			*
*		DESCRIPTION	: サスペンド判定（高速起動対応）						*
*																			*
*		PARAMETER	: IN  : NONE											*
*																			*
*					  RET : TRUE[SUSPEND中]	/ FALSE[SUSPEND中でない]		*
*																			*
****************************************************************************/
static int fc_ipa_check_irqflag(bool flag, void* param) {
	int retval = -EINTR;
	if (!flag) {
		retval = 0;
	}
	return retval;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_checkPreSuspend								*
*																			*
*		DESCRIPTION	: サスペンド判定（高速起動対応）						*
*																			*
*		PARAMETER	: IN  : NONE											*
*																			*
*					  RET : TRUE[SUSPEND中]	/ FALSE[SUSPEND中でない]		*
*																			*
****************************************************************************/
static inline bool fc_ipa_checkPreSuspend(void)
{
	int retval = 0;
	/*---------------------------------------------------------*/
	/*  BU-DET/ACC割り込みチェック                             */
	/*    BU-DET 割り込みからre-init直前までを判定             */
	/*---------------------------------------------------------*/
	retval = early_device_invoke_with_flag_irqsave(
		RNF_BUDET_ACCOFF_INTERRUPT_MASK, fc_ipa_check_irqflag, NULL);
	if(-EINTR == retval){
		LOGT("Budet Interrupted occured.");
	}
	return pre_suspend_flg|(-EINTR == retval);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_changeState									*
*																			*
*		DESCRIPTION	: 状態遷移												*
*																			*
*		PARAMETER	: IN  : *iad : コンテキスト								*
*																			*
*					  IN  : state	:	変更先ステート						*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline void fc_ipa_changeState(ST_IPA_DATA *iad,enum ipodauth_state state)
{
	if(state != iad->fsm.state){
		LOG_CHANGESTATE(iad,state);
		iad->fsm.state =state;
	}
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_setCS											*
*																			*
*		DESCRIPTION	: CS#制御												*
*																			*
*		PARAMETER	: IN  : *iad : コンテキスト								*
*																			*
*					  IN  : active (ACTIVE or INACTIVE)						*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline void fc_ipa_setCS(ST_IPA_DATA* iad,bool active)
{
	if(active)	gpio_set_value(IPOD_CS, LOW);
	else		gpio_set_value(IPOD_CS, HIGH);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_set_sfio										*
*																			*
*		DESCRIPTION	: SFIO(Special Function IO)へ設定						*
*																			*
*		PARAMETER	: IN  : *iad : コンテキスト								*
*																			*
*					  IN  : ballname	:	ポート名						*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline void fc_ipa_set_sfio(ST_IPA_DATA *iad,unsigned long ballname)
{
#if INTERFACE == IF_SPI_TEGRA	/* SPI-TEGRAによる通信 */
	tegra_gpio_disable(ballname);
#else							/* GPIO制御による通信 */
	if(IPOD_RXD == ballname){
		tegra_gpio_enable(ballname);
		gpio_direction_input(ballname);
	}
#endif
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_init											*
*																			*
*		DESCRIPTION	: 初期化処理											*
*																			*
*		PARAMETER	: IN  : *iad : コンテキスト								*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ipa_init(ST_IPA_DATA *iad)
{
	char 	request_string[128]={0};
	/*--------------------------------------*/
	/*  GPIO初期化設定						*/
	/*--------------------------------------*/
	gpio_request(IPOD_RESET		,fc_ipa_gpio_request_str(iad,request_string,"RESET"));
	gpio_request(IPOD_CS		,fc_ipa_gpio_request_str(iad,request_string,"CS"));
	gpio_request(IPOD_TXD		,fc_ipa_gpio_request_str(iad,request_string,"TXD"));
	gpio_request(IPOD_RXD		,fc_ipa_gpio_request_str(iad,request_string,"RXD"));
	gpio_request(IPOD_CLK		,fc_ipa_gpio_request_str(iad,request_string,"CLK"));

	fc_ipa_set_gpio_output(iad,IPOD_RESET,LOW);
	
	fc_ipa_set_gpio_output(iad,IPOD_CS,LOW);
	fc_ipa_set_gpio_output(iad,IPOD_TXD,LOW);
	fc_ipa_set_gpio_output(iad,IPOD_RXD,LOW);
	fc_ipa_set_gpio_output(iad,IPOD_CLK,LOW);

	/*--------------------------------------*/
	/*  タイマ初期化						*/
	/*--------------------------------------*/
	fc_ipa_timer_init(iad);
	/*--------------------------------------*/
	/*  ステート初期化						*/
	/*--------------------------------------*/
	fc_ipa_changeState(iad,IPODAUTH_STATE_INIT);
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_signalreset									*
*																			*
*		DESCRIPTION	: ポートリセット										*
*																			*
*		PARAMETER	: IN  : *iad : コンテキスト								*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ipa_signalreset(ST_IPA_DATA *iad)
{
	fc_ipa_set_gpio_output(iad,IPOD_CS,LOW);
	fc_ipa_set_gpio_output(iad,IPOD_TXD,LOW);
	fc_ipa_set_gpio_output(iad,IPOD_RXD,LOW);
	fc_ipa_set_gpio_output(iad,IPOD_CLK,LOW);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_prepare_communication							*
*																			*
*		DESCRIPTION	: 通信事前処理											*
*																			*
*		PARAMETER	: IN  : *iad : コンテキスト								*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ipa_prepare_communication(ST_IPA_DATA *iad)
{
	/*--------------------------------------*/
	/*  SFIOに設定							*/
	/*--------------------------------------*/
	fc_ipa_set_sfio(iad,IPOD_TXD);
	fc_ipa_set_sfio(iad,IPOD_RXD);
	fc_ipa_set_sfio(iad,IPOD_CLK);
	/*--------------------------------------*/
	/*  IDLEへ遷移							*/
	/*--------------------------------------*/
	fc_ipa_changeState(iad,IPODAUTH_STATE_IDLE);
	/*--------------------------------------*/
	/*  待ち状態解除						*/
	/*--------------------------------------*/
	LOGT("IOCTL wait unlock -->");
	iad->waitRunQue.waitting=MSG_DONE;
	wake_up_interruptible(&iad->waitRunQue.waitQue);
	LOGT("IOCTL wait unlock <--");
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_complete										*
*																			*
*		DESCRIPTION	: SPI送受信完了コールバック								*
*																			*
*		PARAMETER	: IN  : *arg : 											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ipa_complete(void *arg)
{
	complete(arg);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_sync											*
*																			*
*		DESCRIPTION	: SPI同期通信											*
*																			*
*		PARAMETER	: IN  : *iad : 	コンテキスト							*
*																			*
*					: IN  : *message : メッセージ							*
*																			*
*					  RET : サイズ	/	リターンコード						*
*																			*
****************************************************************************/
static ssize_t fc_ipa_sync(ST_IPA_DATA *iad, struct spi_message *message)
{
	int status;
	DECLARE_COMPLETION_ONSTACK(done);

	/*--------------------------------------*/
	/*  メッセージ生成						*/
	/*--------------------------------------*/
	message->complete = fc_ipa_complete;
	message->context = &done;

	/*--------------------------------------*/
	/*  masterへ送信						*/
	/*--------------------------------------*/
	spin_lock_irq(&iad->spi_lock);
	if (iad->spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_async(iad->spi, message);
	spin_unlock_irq(&iad->spi_lock);

	/*--------------------------------------*/
	/*  待ち処理							*/
	/*--------------------------------------*/
	if (status == 0) {
		wait_for_completion_interruptible(&done);//wait_for_completion(&done);
		status = message->status;
		if (status == 0)
			status = message->actual_length;
	}
	return status;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_syncWrite										*
*																			*
*		DESCRIPTION	: SPI送信												*
*																			*
*		PARAMETER	: IN  : *ccd : コンテキスト								*
*																			*
*					  IN  : *buf   : データ									*
*																			*
*					  IN  : length : レングス								*
*																			*
*					  RET : サイズ / リターンコード							*
*																			*
****************************************************************************/
static inline ssize_t fc_ipa_syncWrite(ST_IPA_DATA *iad,char* buf,int length)
{
	ssize_t ret = 0;
	ssize_t spi_ret=0;
	volatile unsigned int	j;
	volatile unsigned int	i;

	/*--------------------------------------*/
	/*  レングス分送信	1バイトづつ送信		*/
	/*--------------------------------------*/
	for(j=0;j<length;++j){
#if INTERFACE == IF_SPI_TEGRA	/* SPI-TEGRAによる通信 */
		UNUSED(i);
		struct spi_transfer	t = {
				.tx_buf		= buf+j,
				.len		= 1,//length,
			};
		struct spi_message	m;

		/*--------------------------------------*/
		/*  メッセージの生成と送信				*/
		/*--------------------------------------*/
		spi_message_init(&m);
		spi_message_add_tail(&t, &m);
		spi_ret = fc_ipa_sync(iad, &m);
#elif INTERFACE == IF_GPIO		/* GPIO制御による通信 */
		/*--------------------------------------*/
		/*  GPIO制御にて送信					*/
		/*--------------------------------------*/
		gpio_set_value(IPOD_CLK, LOW);
		gpio_set_value(IPOD_TXD, LOW);
		/*  The CP latches the state of SPI_SIMO on the falling edge of SPI_CLK
		 The accessory controller should update SPI_SIMO on the rising edge of SPI_CLK.*/
		for(i=0; i < 8; i++){
			gpio_set_value(IPOD_CLK, HIGH);
			IPD_CLKDELAY;
			if(*(buf+j) & (0x80 >> i))/* MSB first */
				gpio_set_value(IPOD_TXD, HIGH);
			else 
				gpio_set_value(IPOD_TXD, LOW);
			__delay(700);	/* 2013/03/20	TXセットアップ時間が0.089usの計測結果にてNG 1us開ける */
			gpio_set_value(IPOD_CLK, LOW);
			IPD_CLKDELAY;
		}
		// 
		gpio_set_value(IPOD_CLK, LOW);
		IPD_CLKDELAY;
		gpio_set_value(IPOD_CLK, LOW);

		spi_ret=1;
#endif
		ret+=spi_ret;
		if(0 == spi_ret)				/* レングスゼロ */
			ret = 0;
		if(TRUE == fc_ipa_checkPreSuspend())	/* サスペンド 	*/
			ret = IPD_PRE_SUSPEND_RETVAL;
		if(ret <= 0)
			break;
	}
	gpio_set_value(IPOD_TXD, LOW);
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_syncRead										*
*																			*
*		DESCRIPTION	: SPI受信												*
*																			*
*		PARAMETER	: IN  : *iad : コンテキスト								*
*																			*
*					  IN  : *b   	: データ								*
*																			*
*					  IN  : length  : レングス								*
*																			*
*					  RET : サイズ / リターンコード							*
*																			*
****************************************************************************/
static inline ssize_t fc_ipa_syncRead(ST_IPA_DATA *iad,char* tx_buf,char* rx_buf,int length)
{
	ssize_t ret = 0;
	ssize_t spi_ret=0;

	volatile unsigned int	j;
	volatile unsigned int	i;
	for(j=0;j<length;++j){
#if INTERFACE == IF_SPI_TEGRA		/* SPI-TEGRAによる通信 */
		UNUSED(i);
		struct spi_transfer	t = {
				.rx_buf		= rx_buf+j,
				.tx_buf		= tx_buf+j,
				.len		= 1,//length,
			};
		struct spi_message	m;

		/*--------------------------------------*/
		/*  メッセージの生成と受信				*/
		/*--------------------------------------*/
		spi_message_init(&m);
		spi_message_add_tail(&t, &m);
		spi_ret=fc_ipa_sync(iad, &m);
#elif INTERFACE == IF_GPIO			/* GPIO制御による通信 */
		/*--------------------------------------*/
		/*  GPIO制御にて送信					*/
		/*--------------------------------------*/
		/* When SPI_nSS is low, the CP updates SPI_SOMI on the rising edge of SPI_CLK. 
		The accessory controller should latch SPI_SOMI on the falling edge of SPI_CLK.*/
		gpio_set_value(IPOD_CLK, LOW);
		for(i=0; i < 8; i++){
			gpio_set_value(IPOD_CLK, HIGH);
			IPD_CLKDELAY;
			gpio_set_value(IPOD_CLK, LOW);
			if(gpio_get_value(IPOD_RXD))
				*(rx_buf+j) |= 0x80 >> i;/* MSB first */
			IPD_CLKDELAY;
		}

		gpio_set_value(IPOD_CLK, LOW);
		IPD_CLKDELAY;
		gpio_set_value(IPOD_CLK, LOW);

		spi_ret=1;
#endif
		ret+=spi_ret;
		if(0 == spi_ret)				/* レングスゼロ */
			ret = 0;
		if(TRUE == fc_ipa_checkPreSuspend())	/* サスペンド */
			ret = IPD_PRE_SUSPEND_RETVAL;
		if(ret <= 0)
			break;
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_SendCmdAdr										*
*																			*
*		DESCRIPTION	: コマンド送信											*
*																			*
*		PARAMETER	: IN  : *iad : コンテキスト								*
*																			*
*					  IN  : *pucSnd_data  : データ							*
*																			*
*					  RET : サイズ / リターンコード							*
*																			*
****************************************************************************/
static ssize_t fc_ipa_SendCmdAdr(ST_IPA_DATA *iad,unsigned char *pucSnd_data)
{
	/*--------------------------------------*/
	/*  CS#H ネガティブ						*/
	/*--------------------------------------*/
	fc_ipa_setCS(iad,INACTIVE);
	__udelay(IPD_TIMER_nSS_DEASSERT);
	/*--------------------------------------*/
	/*  CS#L アクティブ						*/
	/*--------------------------------------*/
	fc_ipa_setCS(iad,ACTIVE);
	/*--------------------------------------*/
	/*  MISOをGPIO設定に					*/
	/*--------------------------------------*/
	fc_ipa_set_gpio_output(iad,IPOD_RXD,HIGH);
	__udelay(IPD_TIMER_SOMI_READY);
	/*--------------------------------------*/
	/*  MISOをSFIO設定に戻す				*/
	/*--------------------------------------*/
	fc_ipa_set_sfio(iad,IPOD_RXD);
	return fc_ipa_syncWrite(iad,pucSnd_data,IPD_COMMAND_SIZE);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_Recv											*
*																			*
*		DESCRIPTION	: IPOD認証受信											*
*																			*
*		PARAMETER	: IN  : *iad : コンテキスト								*
*																			*
*					  IN  : pucSnd_data	: 送信バッファ						*
*																			*
*					  IN  : ulSndLength	: 送信レングス						*
*																			*
*					  IN  : ulRcvLength	: 受信レングス						*
*																			*
*					  IN  : pucRcv_data	: 受信バッファ						*
*																			*
*					  OUT : pulOutlength: 受信レングス						*
*																			*
*					  RET : サイズ / リターンコード							*
*																			*
****************************************************************************/
static ssize_t fc_ipa_Recv(ST_IPA_DATA *iad,
						unsigned char *pucSnd_data,
						unsigned long ulSndLength,
						unsigned long ulRcvLength,
						unsigned char *pucRcv_data,
						unsigned long *pulOutlength)
{
	signed long i;
	/*--------------------------------------*/
	/*  コマンド送信						*/
	/*--------------------------------------*/
	ssize_t ret= fc_ipa_SendCmdAdr(iad,pucSnd_data);
	if(IPD_COMMAND_SIZE==ret){	/* サイズ確認 */
		fc_ipa_set_gpio_output(iad,IPOD_RXD,LOW);
		__udelay(IPD_TIMER_SOMI_READY);
		fc_ipa_set_sfio(iad,IPOD_RXD);
		ret = ESUCCESS;
		/*--------------------------------------*/
		/*  MISO HIGH待ち						*/
		/*--------------------------------------*/
		for(i=-1;i;--i){/* 無限LOOP防止 */
			ret=fc_ipa_checkPreSuspend()?IPD_PRE_SUSPEND_RETVAL:ESUCCESS;
			if(gpio_get_value(IPOD_RXD) || ret!=ESUCCESS)
				break;
		}
		if(0==i)
			ret = -EFAULT;
		if(ret==ESUCCESS){
			/*--------------------------------------*/
			/*  読み込み							*/
			/*--------------------------------------*/
			fc_ipa_set_sfio(iad,IPOD_RXD);
			ret=fc_ipa_syncRead(iad,pucSnd_data+IPD_COMMAND_SIZE,pucRcv_data,ulRcvLength);
			if(0 < ret
			&& ret == ulRcvLength){
				*pulOutlength=ret;
			}else
				*pulOutlength=0;
			/*--------------------------------------*/
			/*  後処理								*/
			/*--------------------------------------*/
			__udelay(IPD_TIMER_SOMI_RELEASE);
			fc_ipa_set_sfio(iad,IPOD_RXD);
			fc_ipa_setCS(iad,INACTIVE);
		}
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_Send											*
*																			*
*		DESCRIPTION	: IPOD認証送信											*
*																			*
*		PARAMETER	: IN  : *iad : コンテキスト								*
*																			*
*					  OUT : *buf  : データ									*
*																			*
*					  OUT : length: レングス								*
*																			*
*					  RET : サイズ / リターンコード							*
*																			*
****************************************************************************/
static ssize_t fc_ipa_Send(ST_IPA_DATA *iad,char* buf,int length)
{
	signed long i;
	/*--------------------------------------*/
	/*  コマンド送信						*/
	/*--------------------------------------*/
	ssize_t ret = fc_ipa_SendCmdAdr(iad,buf);
	if(IPD_COMMAND_SIZE==ret){
		fc_ipa_set_gpio_output(iad,IPOD_RXD,LOW);
		__udelay(IPD_TIMER_SOMI_READY);
		fc_ipa_set_sfio(iad,IPOD_RXD);
		ret = ESUCCESS;
		/*--------------------------------------*/
		/*  MISO HIGH待ち						*/
		/*--------------------------------------*/
		for(i=-1;i;--i){/* 無限LOOP防止 */
			ret=fc_ipa_checkPreSuspend()?IPD_PRE_SUSPEND_RETVAL:ESUCCESS;
			if(gpio_get_value(IPOD_RXD) || ret!=ESUCCESS)
				break;
		}
		if(0 == i)
			ret = -1;
		if(ESUCCESS == ret){
			/*--------------------------------------*/
			/*  書き込み							*/
			/*--------------------------------------*/
			fc_ipa_set_sfio(iad,IPOD_RXD);
			ret=fc_ipa_syncWrite(iad,buf+IPD_COMMAND_SIZE,length-IPD_COMMAND_SIZE);
			/*--------------------------------------*/
			/*  後処理								*/
			/*--------------------------------------*/
			__udelay(IPD_TIMER_SOMI_RELEASE);
			fc_ipa_set_sfio(iad,IPOD_RXD);
			fc_ipa_setCS(iad,INACTIVE);
		}
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_workThread										*
*																			*
*		DESCRIPTION	: ワーカースレッド										*
*																			*
*		PARAMETER	: IN  : work : ワークデータ								*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ipa_workThread(struct work_struct *work)
{
	ST_IPA_WORKER_INFO *wi = container_of(work, ST_IPA_WORKER_INFO, workq);
	ST_IPA_DATA* iad = (ST_IPA_DATA*)wi->pthis;
	
	switch(iad->fsm.state){
	/*--------------------------------------*/
	/*  初期状態							*/
	/*--------------------------------------*/
	case IPODAUTH_STATE_INIT:
		switch(wi->Msgtype){
		case WMID_BUDET_HIGH:
			fc_ipa_signalreset(iad);
			fc_ipa_timer_start(iad,TIMER_ID_T1);	/* T1(60ms) */
			fc_ipa_changeState(iad,IPODAUTH_STATE_BEGIN);
			break;
		default:
			LOGW("IPODAUTH_STATE_INIT default Msgtype=%d\n",wi->Msgtype);
			break;
		}
		break;
	/*--------------------------------------*/
	/*  開始状態							*/
	/*--------------------------------------*/
	case IPODAUTH_STATE_BEGIN:
		switch(wi->Msgtype){
		case WMID_TIMER_T1:/* T1(60ms) */
		case WMID_TIMER_T3:/* T3(1ms) */
			fc_ipa_set_gpio_output(iad,IPOD_RESET,HIGH);
			fc_ipa_timer_start(iad,TIMER_ID_T2);	/* T2(30ms) */
			break;
		case WMID_TIMER_T2:/* T2(30ms) */
			fc_ipa_prepare_communication(iad);
			break;
		default:
			LOGW("IPODAUTH_STATE_BEGIN default Msgtype=%d\n",wi->Msgtype);
			break;
		}
		break;
	/*--------------------------------------*/
	/*  アイドル状態						*/
	/*--------------------------------------*/
	case IPODAUTH_STATE_IDLE:
		switch(wi->Msgtype){
		default:
			LOGW("IPODAUTH_STATE_IDLE default Msgtype=%d\n",wi->Msgtype);
			break;
		}
		break;
	/*--------------------------------------*/
	/*  実行状態							*/
	/*--------------------------------------*/
	case IPODAUTH_STATE_RUN:
		switch(wi->Msgtype){
		default:
			LOGW("IPODAUTH_STATE_RUN default Msgtype=%d\n",wi->Msgtype);
			break;
		}
		break;
	default:
		break;
	}
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_decreate										*
*																			*
*		DESCRIPTION	: 後処理												*
*																			*
*		PARAMETER	: IN  : iad : コンテキスト								*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline void fc_ipa_decreate(ST_IPA_DATA *iad)
{
	int i;
	/*--------------------------------------*/
	/*  ワーカースレッド削除				*/
	/*--------------------------------------*/
	for(i=0;i<ARRAY_SIZE(iad->worker_info);++i){
		if(iad->worker_info[i]){
			kfree(iad->worker_info[i]);
			iad->worker_info[i] =NULL;
		}
	}
	if(iad->workqueue){
		destroy_workqueue(iad->workqueue);
		iad->workqueue = NULL;
	}
	/*--------------------------------------*/
	/*  チップリセット						*/
	/*--------------------------------------*/
	fc_ipa_signalreset(iad);
	gpio_set_value(IPOD_RESET, LOW);
	/*--------------------------------------*/
	/*  GPIO開放							*/
	/*--------------------------------------*/
	gpio_free(IPOD_RESET);
	gpio_free(IPOD_CS);
	gpio_free(IPOD_TXD);
	gpio_free(IPOD_RXD);
	gpio_free(IPOD_CLK);

}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_concreate										*
*																			*
*		DESCRIPTION	: 生成処理												*
*																			*
*		PARAMETER	: IN  : iad : コンテキスト								*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline int fc_ipa_concreate(ST_IPA_DATA *iad)
{
	int	status=0;
	fc_ipa_init(iad);
	/*--------------------------------------*/
	/*  ワーカースレッド生成				*/
	/*--------------------------------------*/
	iad->workqueue = create_singlethread_workqueue("spi-ipodauth-work");
	if (iad->workqueue){
		int i;
		/*--------------------------------------*/
		/*  ワーカースレッド情報生成			*/
		/*--------------------------------------*/
		for(i=0;i<ARRAY_SIZE(iad->worker_info);++i){
			iad->worker_info[i]=(ST_IPA_WORKER_INFO*)kmalloc(sizeof(*iad->worker_info[i]), GFP_KERNEL);
			if(!iad->worker_info[i]){
				LOGE("cannot alloc  ST_IPA_WORKER_INFO\n");
				status = -ENOMEM;
				goto exit;
			}else{
				memset(iad->worker_info[i],0,sizeof(*iad->worker_info[i]));
				INIT_WORK((struct work_struct*)iad->worker_info[i], fc_ipa_workThread);
				iad->worker_info[i]->Msgtype = (enum worker_message_type)i;
				iad->worker_info[i]->pthis = iad;
			}
		}
		if(status == ESUCCESS){
			/*--------------------------------------*/
			/*  SPI-TEGRAの設定						*/
			/*--------------------------------------*/
			iad->cdata.is_hw_based_cs = 0;
			iad->cdata.cs_setup_clk_count =0;
			iad->cdata.cs_hold_clk_count  =0;
			iad->spi->controller_data = (void*)&iad->cdata;
			iad->spi->bits_per_word=8;
			spi_setup(iad->spi);
		}
	}else{
		status = -ENOMEM;
		LOGE("cannot alloc  create_singlethread_workqueue\n");
		goto exit;
	}
	return status;
exit:
	fc_ipa_decreate(iad);
	return status;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ipa_ioctl											*
*																			*
*		DESCRIPTION	: I/O control											*
*																			*
*		PARAMETER	: IN  : filp : ファイル									*
*																			*
*					: IN  : cmd  : コマンド									*
*																			*
*					: IN  : arg												*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
static long fs_ipa_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int			err = 0;
	int			retval = ESUCCESS;
	
	ST_IPA_DATA	*iad;
	ST_KDV_IPD_IOC_WR	stWrData;
	ST_KDV_IPD_IOC_RD	stRdData;
	unsigned char		ucWRSndData[IPD_DATA_MAXLEN]={0};	/* 送信データ(書き込み)			*/
	unsigned char		ucRDSndData[IPD_DATA_MAXLEN]={0};	/* 送信データ(読み込み)			*/
	unsigned char		ucRcvData[IPD_DATA_MAXLEN]={0};		/* 受信データ					*/
	unsigned long		ulOutLength=0;						/* 受信サイズ					*/

	LOGD("%s:(%d)\n",__FUNCTION__,__LINE__);
	/* Check type and command number */
	if (_IOC_TYPE(cmd) != IPD_IOC_MAGIC){
		return -ENOTTY;
	}

	/* Check access direction once here; don't repeat below.
	 * IOC_DIR is from the user perspective, while access_ok is
	 * from the kernel perspective; so they look reversed.
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
				(void __user *)arg, _IOC_SIZE(cmd));
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
				(void __user *)arg, _IOC_SIZE(cmd));
	if (err){
		LOGE("%s(%d):permission err",__FUNCTION__,__LINE__);
		return -EFAULT;
	}
	if(cmd==IPD_IOC_ISSUSPEND){
 		if(TRUE == fc_ipa_checkPreSuspend() ){
            return IPD_PRE_SUSPEND_RETVAL;
 		}else{
			return ESUCCESS;
		}
	}
	mutex_lock(&device_list_lock);
	memset(&stWrData,0,sizeof(stWrData));
	memset(&stRdData,0,sizeof(stRdData));

	iad = filp->private_data;
	if(TRUE == fc_ipa_checkPreSuspend()	){
		LOGT("%s(%d):suspend Guard",__FUNCTION__,__LINE__);
		mutex_unlock(&device_list_lock);
		return IPD_PRE_SUSPEND_RETVAL;
	}

	/*--------------------------------------*/
	/*  IDLE状態でないなら準備待ち処理		*/
	/*--------------------------------------*/
	if(IPODAUTH_STATE_IDLE!=iad->fsm.state){
		LOGT("IOCTL wait IDLE.......");
		iad->waitRunQue.waitting = MSG_WAIT;
		wait_event_interruptible( iad->waitRunQue.waitQue, iad->waitRunQue.waitting );
		LOGT("IOCTL wait IDLE out");
		if(TRUE == fc_ipa_checkPreSuspend()){
			LOGT("%s(%d):suspend Guard",__FUNCTION__,__LINE__);
			retval = IPD_PRE_SUSPEND_RETVAL;
		}
	}
	/*--------------------------------------*/
	/*  読み書き処理						*/
	/*--------------------------------------*/
	if(ESUCCESS == retval){
		/* 開始イベント送信 */
		fc_ipa_changeState(iad,IPODAUTH_STATE_RUN);
		switch (cmd) {
		/*--------------------------------------*/
		/*  書き込み							*/
		/*--------------------------------------*/
		case	IPD_IOC_WRITE:
			LOGT("IPD_IOC_WRITE\n");
			if(!copy_from_user( &stWrData, (int __user*)arg, sizeof(stWrData) )
			&& !copy_from_user( ucWRSndData, (int __user*)stWrData.pucSndData, stWrData.ulSndLength )){
				LOGT("stWrData.ulSndLength %d \n",(int)stWrData.ulSndLength);
				LOGT("ucWRSndData [0x%x , 0x%x, 0x%x] \n",ucWRSndData[0],ucWRSndData[1],ucWRSndData[2]);
				retval = fc_ipa_Send(iad,ucWRSndData, stWrData.ulSndLength);
				if(!retval)
					retval = -EFAULT;
				if(0 < retval)
					retval =ESUCCESS;
			}else
				retval = -EFAULT;
			break;
		/*--------------------------------------*/
		/*  読み込み							*/
		/*--------------------------------------*/
		case	IPD_IOC_READ:
			LOGT("IPD_IOC_READ\n");
			if(!copy_from_user( &stRdData, (int __user*)arg, sizeof(stRdData) )
			&& !copy_from_user( ucRDSndData, (int __user*)stRdData.pucSndData, stRdData.ulSndLength )
			) {
				LOGT("stRdData.pucSndData [0x%x , 0x%x] \n",ucRDSndData[0],ucRDSndData[1]);
				LOGT("stRdData.ulSndLength %d \n",(int)stRdData.ulSndLength);
				LOGT("stRdData.ulRcvLength %d \n",(int)stRdData.ulRcvLength);
				LOGT("stRdData.ucRcvData [0x%x , 0x%x] \n",ucRcvData[0],ucRcvData[1]);
				retval=fc_ipa_Recv(iad,
							ucRDSndData,
							stRdData.ulSndLength,
							stRdData.ulRcvLength,
							ucRcvData,
							&ulOutLength);
#ifdef IPODAUTHDEBUG	/* デバック用 */
				LOGT("read size %d",(int)ulOutLength);
				{
					int i;
					LOGT("receive data = ");
					for(i=0;i<stRdData.ulRcvLength;++i)
						printk("%02x ",ucRcvData[i]);
					printk("\n");
				}
#endif
				if(!retval)
					retval = -EFAULT;
				else
				if(ulOutLength){
					if(!copy_to_user( (int __user*)stRdData.pucRcvData, ucRcvData, ulOutLength )
					&& !copy_to_user( (int __user*)stRdData.pulOutLength, &ulOutLength,  sizeof(ulOutLength) )) {
						retval = ESUCCESS;
					}else
						retval = -EFAULT;
				}
			}else
				retval = -EFAULT;
			break;
		default:
			LOGW("unknown command (%d) \n",cmd);
			break;
		}
		/* 終了イベント送信 */
		LOGT("IOCTL goto END");
		fc_ipa_prepare_communication(iad);
	}

	if(TRUE == fc_ipa_checkPreSuspend()){
		LOGT("%s(%d):occur suspend",__FUNCTION__,__LINE__);
		retval = IPD_PRE_SUSPEND_RETVAL;
	}
	mutex_unlock(&device_list_lock);
	LOGD("%s : ret = %d\n",__FUNCTION__,retval);
	return retval;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ipa_compat_ioctl									*
*																			*
*		DESCRIPTION	: I/O control											*
*																			*
*		PARAMETER	: IN  : filp : ファイル									*
*																			*
*					: IN  : cmd  : コマンド									*
*																			*
*					: IN  : arg												*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
#ifdef CONFIG_COMPAT
static long fs_ipa_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return fs_ipa_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define fs_ipa_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

/****************************************************************************
*																			*
*		SYMBOL		: fs_ipa_reinit											*
*																			*
*		DESCRIPTION	: Resume事前通知										*
*																			*
*		PARAMETER	: IN  : dev : デバイスファイル							*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
static int fs_ipa_reinit(struct device *dev)
{
	UNUSED(dev);
//	printk(KERN_INFO DRV_NAME "%s",__FUNCTION__);
	return 0;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ipa_sudden_device_poweroff							*
*																			*
*		DESCRIPTION	: Suspend事前通知										*
*																			*
*		PARAMETER	: IN  : dev : デバイスファイル							*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
static void fs_ipa_sudden_device_poweroff(struct device *dev)
{
	UNUSED(dev);
//	printk(KERN_INFO DRV_NAME "%s",__FUNCTION__);
	pre_suspend_flg=TRUE;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ipa_open											*
*																			*
*		DESCRIPTION	: OPEN													*
*																			*
*		PARAMETER	: IN  : *inode : ノード									*
*																			*
*					  IN  : *filp  : ファイル								*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
static int fs_ipa_open(struct inode *inode, struct file *filp)
{
	ST_IPA_DATA	*iad;
	int		status = -EFAULT;
	
	mutex_lock(&device_list_lock);
	/*--------------------------------------*/
	/*  インスタンス取得					*/
	/*--------------------------------------*/
	list_for_each_entry(iad, &device_list, device_entry) {
		if (iad->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}
	LOGD("%s(%d)\n",__FUNCTION__,__LINE__);
	if (0 == status) {
		if(iad->users<=0){//one user
			iad->users++;
			filp->private_data = iad;
			nonseekable_open(inode, filp);
			/*--------------------------------------*/
			/*  ステート初期化						*/
			/*--------------------------------------*/
			fc_ipa_changeState(iad,IPODAUTH_STATE_INIT);
			/*--------------------------------------*/
			/*  SUSPEND FLG OFF						*/
			/*--------------------------------------*/
			pre_suspend_flg=FALSE;
			/*--------------------------------------*/
			/*  Chip COLD Start						*/
			/*--------------------------------------*/
			LOGD("Chip Cold Reset\n");
			queue_work(iad->workqueue, (struct work_struct*)iad->worker_info[WMID_BUDET_HIGH]);
		}else{
			LOGW("Multiple open !!!!\n");
			status = -EBUSY;
		}
	}
	mutex_unlock(&device_list_lock);
	LOGT("%s:ret=%d\n",__FUNCTION__,status);
	return status;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ipa_release										*
*																			*
*		DESCRIPTION	: RELEASE												*
*																			*
*		PARAMETER	: IN  : *inode : ノード									*
*																			*
*					  IN  : *filp  : ファイル								*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
static int fs_ipa_release(struct inode *inode, struct file *filp)
{
	ST_IPA_DATA	*iad;
	int i;

	iad = filp->private_data;
	LOGT("%s(%d)\n",__FUNCTION__,__LINE__);
	mutex_lock(&device_list_lock);
	if(iad){
		/*--------------------------------------------------*/
		/*	ユーザ数チェック								*/
		/*--------------------------------------------------*/
		if(iad->users > 0) {
			iad->users --;
		}
		if(iad->users <= 0){
			iad->users= 0;
			/*--------------------------------------------------*/
			/*	ドライバ情報初期化								*/
			/*--------------------------------------------------*/
			filp->private_data = NULL;
			/*--------------------------------------*/
			/*  タイマキャンセル					*/
			/*--------------------------------------*/
			for(i=0;i<TIMER_ID_MAX;++i)
				fc_ipa_timer_cancel(iad,i);
			/*--------------------------------------*/
			/*  初期状態へ遷移し終了処理			*/
			/*--------------------------------------*/
			fc_ipa_changeState(iad,IPODAUTH_STATE_INIT);
		}
	}
	mutex_unlock(&device_list_lock);
	LOGT("%s:ret=%d\n",__FUNCTION__,0);
	return 0;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ipa_suspend										*
*																			*
*		DESCRIPTION	: SUSPEND												*
*																			*
*		PARAMETER	: IN  : *spi : spiデータ								*
*																			*
*					  IN  : mesg : メッセージ								*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
static int fs_ipa_suspend(struct spi_device *spi, pm_message_t mesg)
{
	ST_IPA_DATA	*iad = spi_get_drvdata(spi);
	int i;
	LOGT("%s:(%d)\n",__FUNCTION__,__LINE__);

	/*--------------------------------------*/
	/*  SUSPEND FLG ON						*/
	/*--------------------------------------*/
	pre_suspend_flg=TRUE;
	/*--------------------------------------*/
	/*  チップリセット						*/
	/*--------------------------------------*/
	fc_ipa_signalreset(iad);
	gpio_set_value(IPOD_RESET, LOW);
	/*--------------------------------------*/
	/*  Timer cancel						*/
	/*--------------------------------------*/
	for(i=0;i<TIMER_ID_MAX;++i)
		fc_ipa_timer_cancel(iad,i);
	/*--------------------------------------*/
	/*  ステート初期化						*/
	/*--------------------------------------*/
	fc_ipa_changeState(iad,IPODAUTH_STATE_INIT);
	return 0;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ipa_resume											*
*																			*
*		DESCRIPTION	: RESUME												*
*																			*
*		PARAMETER	: IN  : *spi : spiデータ								*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
static int fs_ipa_resume(struct spi_device *spi)
{
	int i;
	ST_IPA_DATA	*iad = spi_get_drvdata(spi);
	LOGT("%s:(%d)\n",__FUNCTION__,__LINE__);
	/*--------------------------------------*/
	/*  SUSPEND FLG ON						*/
	/*--------------------------------------*/
	pre_suspend_flg=TRUE;
	/*--------------------------------------*/
	/*  チップリセット						*/
	/*--------------------------------------*/
	fc_ipa_signalreset(iad);
	gpio_set_value(IPOD_RESET, LOW);
	/*--------------------------------------*/
	/*  Timer cancel						*/
	/*--------------------------------------*/
	for(i=0;i<TIMER_ID_MAX;++i)
		fc_ipa_timer_cancel(iad,i);
	/*--------------------------------------*/
	/*  ステート初期化						*/
	/*--------------------------------------*/
	fc_ipa_changeState(iad,IPODAUTH_STATE_INIT);
	return 0;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_show_adapter_name								*
*																			*
*		DESCRIPTION	: アダプタネーム生成									*
*																			*
*		PARAMETER	: IN : *dev : devデータ									*
*																			*
*					  IN : *attr: 属性データ								*
*																			*
*					  OUT: *buf : 文字列生成バッファ						*
*																			*
*					  RET : 文字数											*
*																			*
****************************************************************************/
static ssize_t fc_ipa_show_adapter_name(struct device *dev, struct device_attribute *attr, char *buf)
{
	int no  = MINOR(dev->devt);
	return sprintf(buf, "spi-ipodauth-%d\n", no);
}
static DEVICE_ATTR(name, S_IRUGO, fc_ipa_show_adapter_name, NULL);

/****************************************************************************
*																			*
*		SYMBOL		: fs_ipa_probe											*
*																			*
*		DESCRIPTION	: probe													*
*																			*
*		PARAMETER	: IN  : spiデータ										*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
static int __devinit fs_ipa_probe(struct spi_device *spi)
{
	ST_IPA_DATA	*iad;
	int	status=0;
	unsigned long	minor;
	struct device *dev=NULL;

	/* Allocate driver data */
	iad = kzalloc(sizeof(*iad), GFP_KERNEL);
	if (NULL == iad){
		return -ENOMEM;
	}
	/*--------------------------------------*/
	/*  データ初期化						*/
	/*--------------------------------------*/
	memset(iad,0,sizeof(*iad));
	iad->spi = spi;
	spin_lock_init(&iad->spi_lock);
	INIT_LIST_HEAD(&iad->device_entry);
	init_waitqueue_head( &iad->waitRunQue.waitQue);
	init_waitqueue_head( &iad->waitProcessQue.waitQue);

	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		/*--------------------------------------*/
		/*  デバイスファイル生成				*/
		/*--------------------------------------*/
		iad->devt = MKDEV(IPODAUTH_MAJOR, minor);
		dev = device_create(ipodauth_class, &spi->dev, iad->devt,
				    iad, "ipodauth%d.%d",
				    spi->master->bus_num, spi->chip_select);
		memset(iad->id,0,sizeof(iad->id));
		sprintf(iad->id,"%d.%d",spi->master->bus_num, spi->chip_select);
		status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
		if (0 == status ){
			status = device_create_file(dev, &dev_attr_name);
			if (0 == status )
				status=fc_ipa_concreate(iad);
		}
	} else {
		dev_dbg(&spi->dev, "no minor number available!\n");
		status = -ENODEV;
	}
	if (0 == status) {
		set_bit(minor, minors);
		list_add(&iad->device_entry, &device_list);
	}
	mutex_unlock(&device_list_lock);
	if (status == 0){
		/*--------------------------------------*/
		/*  高速起動コールバック登録			*/
		/*--------------------------------------*/
		iad->ed.reinit = NULL;//reiintは処理していないのでNULL設定
		iad->ed.sudden_device_poweroff = fs_ipa_sudden_device_poweroff;
		iad->ed.dev = dev;
		if(0!=early_device_register(&iad->ed))
			early_device_unregister(&iad->ed);
		spi_set_drvdata(spi, iad);
		LOGT("probe SUCCESS !!!");
	}else{
		kfree(iad);
		LOGE("probe FAIL !!!");
	}
	return status;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ipa_remove											*
*																			*
*		DESCRIPTION	: remove												*
*																			*
*		PARAMETER	: IN  : spiデータ										*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
static int __devexit fs_ipa_remove(struct spi_device *spi)
{
	ST_IPA_DATA	*iad = spi_get_drvdata(spi);
	LOGT("%s:(%d)\n",__FUNCTION__,__LINE__);

	spin_lock_irq(&iad->spi_lock);
	iad->spi = NULL;
	spi_set_drvdata(spi, NULL);					/*  SPI登録解除					*/
	spin_unlock_irq(&iad->spi_lock);

	mutex_lock(&device_list_lock);
	early_device_unregister(&iad->ed);			/*  事前通知登録解除			*/
	list_del(&iad->device_entry);			
	device_destroy(ipodauth_class, iad->devt);	/*  クラス削除					*/
	clear_bit(MINOR(iad->devt), minors);
	fc_ipa_decreate(iad);						/*  後処理						*/
	kfree(iad);									/*  インスタンス削除			*/
	mutex_unlock(&device_list_lock);
	return 0;
}
static const struct file_operations ipodauth_fops = {
	.owner =	THIS_MODULE,
	/* REVISIT switch to aio primitives, so that userspace
	 * gets more complete API coverage.  It'll simplify things
	 * too, except for the locking.
	 */
	.open =		fs_ipa_open,
	.unlocked_ioctl = fs_ipa_ioctl,
	.compat_ioctl = fs_ipa_compat_ioctl,
	.release =	fs_ipa_release,
	.llseek =	no_llseek,
};

static struct spi_driver spi_ipodauth_driver = {
	.probe	= fs_ipa_probe,
	.remove = __devexit_p(fs_ipa_remove),
	.suspend =	fs_ipa_suspend,
	.resume =	fs_ipa_resume,
	.driver = {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	},
};

/****************************************************************************
*																			*
*		SYMBOL		: fs_ipa_init											*
*																			*
*		DESCRIPTION	: INIT													*
*																			*
*		PARAMETER	: None													*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
static int __init fs_ipa_init(void)
{
	int status;
	
	printk(KERN_INFO "%s %s:(%d)\n",DRV_NAME,__FUNCTION__,__LINE__);

	/*--------------------------------------*/
	/*  chardev登録							*/
	/*--------------------------------------*/
	IPODAUTH_MAJOR = register_chrdev(IPODAUTH_MAJOR, DRV_NAME, &ipodauth_fops);
	if (IPODAUTH_MAJOR < 0){
		return IPODAUTH_MAJOR;
	}
	/*--------------------------------------*/
	/*  クラス作成							*/
	/*--------------------------------------*/
	ipodauth_class = class_create(THIS_MODULE, DRV_NAME);
	if (IS_ERR(ipodauth_class)) {
		unregister_chrdev(IPODAUTH_MAJOR, spi_ipodauth_driver.driver.name);
		return PTR_ERR(ipodauth_class);
	}
	
	/*--------------------------------------*/
	/*  SPI登録								*/
	/*--------------------------------------*/
	if ((status = spi_register_driver(&spi_ipodauth_driver) )< 0) {
		class_destroy(ipodauth_class);
		unregister_chrdev(IPODAUTH_MAJOR, spi_ipodauth_driver.driver.name);
	}
	return status;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ipa_exit											*
*																			*
*		DESCRIPTION	: EXIT													*
*																			*
*		PARAMETER	: None													*
*																			*
*					  RET : None											*
*																			*
****************************************************************************/
static void __exit fs_ipa_exit(void)
{
	spi_unregister_driver(&spi_ipodauth_driver);
}

module_init(fs_ipa_init);
module_exit(fs_ipa_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:ipod-auth");
