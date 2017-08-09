/*--------------------------------------------------------------------------*/
/* COPYRIGHT(C) FUJITSU TEN LIMITED 2012-2014                               */
/*--------------------------------------------------------------------------*/
/****************************************************************************
*																			*
*		シリーズ名称	: １４ＭＹ											*
*		マイコン名称	: メインマイコン									*
*		オーダー名称	: １４ＭＹ　Ａｎｄｒｏｉｄ－ＤＡ					*
*		ブロック名称	: Ｌｉｎｕｘカーネルドライバ						*
*		DESCRIPTION		: ＣＰＵ間通信ドライバ								*
*																			*
*		ファイル名称	: drv_spi_cm.c										*
*		作　成　者		: Micware)ogura & segawa							*
*		作　成　日		: 2013/01/25	<Ver. 1.00>							*
*		改　訂　日		: yyyy/mm/dd	<Ver. x.xx>							*
*																			*
*		『システムＩＦルーチン』											*
*			module_info_def ...........	Ｌｉｎｕｘドライバ登録定義			*
*			fs_ccd_sys_init ...........	ドライバ初期化処理					*
*			fs_ccd_sys_exit ...........	ドライバ終了処理					*
*			fs_ccd_sys_probe ..........	ドライバ生成処理					*
*			fs_ccd_sys_remove..........	ドライバ削除処理					*
*			fs_ccd_sys_open ...........	ドライバオープン処理				*
*			fs_ccd_sys_close ..........	ドライバクローズ処理				*
*			fs_ccd_sys_suspend.........	ドライバサスペンド処理				*
*			fs_ccd_sys_resume .........	ドライバリジューム処理				*
*			fs_ccd_sys_read............	ドライバリード処理					*
*			fs_ccd_sys_write ..........	ドライバライト処理					*
*																			*
*		『高速起動ＩＦルーチン』											*
*			fs_ccd_fb_sdn_sdwn.........	高速起動・終了通知処理				*
*			fc_ccd_fb_chk_sus .........	高速起動・サスペンド確認処理		*
*																			*
*		『割り込みＩＦルーチン』											*
*			fs_ccd_intr_int_sig .......	ＩＮＴ信号割り込み処理				*
*																			*
*		『タイマＩＦルーチン』												*
*			fs_ccd_timeout_cb .........	タイムアウトコールバック			*
*																			*
*		『スレッドルーチン』												*
*			fc_ccd_wthrd_main .........	ワーカースレッドメイン処理			*
*																			*
*		『状態遷移ルーチン』												*
*			fc_ccd_fsm_snd_req ........	状態遷移処理「送信要求」			*
*			fc_ccd_fsm_snd_cmplt ......	状態遷移処理「送信完了」			*
*			fc_ccd_fsm_rcv_cmplt ......	状態遷移処理「受信完了」			*
*			fc_ccd_fsm_int_l ..........	状態遷移処理「ＩＮＴ－Ｌ割込み」	*
*			fc_ccd_fsm_int_h ..........	状態遷移処理「ＩＮＴ－Ｈ割込み」	*
*			fc_ccd_fsm_int_pulse ......	状態遷移処理「ＩＮＴパルス割込み」	*
*			fc_ccd_fsm_tout_clk1 ......	状態遷移処理「ｔｃｌｋ１　Ｔ．Ｏ．」*
*			fc_ccd_fsm_tout_csh .......	状態遷移処理「ｔｃｓｈ　Ｔ．Ｏ．」	*
*			fc_ccd_fsm_tout_idle ......	状態遷移処理「ｔｉｄｌｅ　Ｔ．Ｏ．」*
*			fc_ccd_fsm_tout_fail1 .....	状態遷移処理「ｔｆａｉｌ1 Ｔ．Ｏ．」*
*			fc_ccd_fsm_tout_comn ......	状態遷移処理「汎用タイマ　Ｔ．Ｏ．」*
*			fc_ccd_fsm_timout .........	状態遷移タイムアウト処理			*
*																			*
*		『サブルーチン』													*
*			fc_ccd_init_instance ......	インスタンス初期化処理				*
*			fc_ccd_make_gpio_str ......	ＧＰＩＯ登録文字列生成				*
*			fc_ccd_calc_fcc ...........	ＦＣＣ計算処理						*
*			fc_ccd_snd_dat ............	データ送信処理						*
*			fc_ccd_rcv_dat ............	データ受信処理						*
*			fc_ccd_snd_byte ...........	１バイト送信処理（ポート制御）		*
*			fc_ccd_rcv_byte ...........	１バイト受信処理（ポート制御）		*
*			fc_ccd_snd_spi_res ........	送信処理（シリアルリソース制御）	*
*			fc_ccd_rcv_spi_res ........	受信処理（シリアルリソース制御）	*
*			fc_ccd_spicomm_ctrl .......	ＳＰＩ送受信制御処理				*
*			fc_ccd_spicomm_cb .........	ＳＰＩ転送完了コールバック			*
*			fc_ccd_tim_init ...........	タイマ初期化						*
*			fc_ccd_tim_start ..........	タイマスタート						*
*			fc_ccd_tim_stop ...........	タイマ停止							*
*			fc_ccd_tim_all_stop .......	全タイマ停止						*
*			fc_ccd_set_sig_cs .........	ＣＳ信号制御処理					*
*			fc_ccd_get_sig_int ........	ＩＮＴ信号取得処理					*
*			fc_ccd_rel_res ............	リソース開放処理					*
*			fc_ccd_fsm_clr ............	状態遷移クリア処理					*
*			fc_ccd_init_drv_inf .......	ドライバ情報初期化処理				*
*			fc_ccd_chk_rcv_dat ........	受信データ確認処理					*
*			fc_ccd_set_rcv_lst ........	受信リスト格納処理					*
*			fc_ccd_chg_fsm_mod ........	状態遷移状態変更処理				*
*			fc_ccd_rdy_shtdwn .........	シャットダウン準備処理				*
*			fc_ccd_disable_irq ........	ドライバ割込み禁止処理				*
*			fc_ccd_enable_irq .........	ドライバ割込み許可処理				*
*																			*
****************************************************************************/
/*==========================================*/
/*	Includes								*/
/*==========================================*/
#include <linux/kernel.h>
#include <linux/jiffies.h>
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
#include <linux/spi-tegra.h>
#include <linux/gpio.h>
#include <linux/time.h>
#include <linux/compat.h>
#include <linux/device.h>

#include <mach/clk.h>
#include <mach/delay.h>
#include <asm/uaccess.h>

#include <linux/earlydevice.h>
#include <linux/budet_interruptible.h>
#include <linux/lites_trace.h>

#include "../../arch/arm/mach-tegra/gpio-names.h"
#include "StdGType.h"
#include "drv_spi_cm.h"


/*==========================================*/
/*	Buffers									*/
/*==========================================*/
DECLARE_BITMAP	(bf_ccd_minor_num, N_SPI_MINORS);
LIST_HEAD		(bf_ccd_device_list);
DEFINE_MUTEX	(bf_ccd_mutx_drv_com);

struct class	*bf_ccd_dev_clas;
INT				bf_ccm_major_num = 0;
BYTE			bf_ccd_shut_down_flg;

/*==========================================*/
/*	Tables									*/
/*==========================================*/
ULONG	const tb_ccd_timer_tim[M_CCD_TIM_MAX] = {
	M_CCD_MCR_MS2US(  2),					/* No.0：ｔｃｌｋ１		2ms			*/
///仕様変更	M_CCD_MCR_MS2US(  1),					/* No.1：ｔｃｓｈ		1ms			*/
	500,									/* No.1：ｔｃｓｈ		500us			*/
	M_CCD_MCR_MS2US(  3),					/* No.2：ｔｉｄｌｅ		3ms			*/
	M_CCD_MCR_MS2US(100),					/* No.3：ｔｆａｉｌ１	100ms		*/
	M_CCD_MCR_MS2US(M_CCD_COMTIM_TOUT_MS),	/* No.4：汎用			50ms		*/
	M_CCD_MCR_MS2US(  1),					/* No.5：INT FALESAFE	1ms			*/	
};

#if CPUCOMDEBUG	== 1
#define ID_DEFINE(x) x , #x
struct debug_string_table{
	int				id;
	char			str[128];
};

static const struct debug_string_table	timerTBL[]={
	{ID_DEFINE(M_CCD_TIM_TCLK1)},	//0
	{ID_DEFINE(M_CCD_TIM_TCSH)},	//1
	{ID_DEFINE(M_CCD_TIM_TIDLE)},	//2
	{ID_DEFINE(M_CCD_TIM_TFAIL1)},	//3
	{ID_DEFINE(M_CCD_TIM_COMON)},	//4
	{ID_DEFINE(M_CCD_TIM_INTFAIL)},	//5
	{ID_DEFINE(M_CCD_TIM_MAX)}		//6
};

static const struct debug_string_table	stateTBL[]={
	{ID_DEFINE(M_CCD_MODE_C0_INIT)},	
	{ID_DEFINE(M_CCD_MODE_C1_IDLE)},	
	{ID_DEFINE(M_CCD_MODE_C2_KPID)},	
	{ID_DEFINE(M_CCD_MODE_S0_SNDW)},	
	{ID_DEFINE(M_CCD_MODE_S1_SNDC)},	
	{ID_DEFINE(M_CCD_MODE_S2_CSHW)},	
	{ID_DEFINE(M_CCD_MODE_S3_CSHK)},	
	{ID_DEFINE(M_CCD_MODE_S4_ITHW)},	
	{ID_DEFINE(M_CCD_MODE_S5_ITLW)},	
	{ID_DEFINE(M_CCD_MODE_R0_ITHW)},	
	{ID_DEFINE(M_CCD_MODE_R1_RCVC)},	
	{ID_DEFINE(M_CCD_MODE_E0_INTE)}		
};
#endif
/*==========================================*/
/*	Prototype								*/
/*==========================================*/
INT		__init		fs_ccd_sys_init(VOID);
VOID	__exit		fs_ccd_sys_exit(VOID);
INT		__devinit	fs_ccd_sys_probe(struct spi_device *spi);
INT		__devexit	fs_ccd_sys_remove(struct spi_device *spi);
INT		fs_ccd_sys_open(struct inode *inode, struct file *filp);
INT		fs_ccd_sys_close(struct inode *inode, struct file *filp);
INT		fs_ccd_sys_suspend(struct spi_device *spi, pm_message_t mesg);
INT		fs_ccd_sys_resume(struct spi_device *spi);
ssize_t	fs_ccd_sys_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
long fs_ccd_sys_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
ssize_t	fs_ccd_sys_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
VOID	fs_ccd_fb_sdn_sdwn(struct device *dev);
INT		fc_ccd_fb_chk_sus(bool flag, void* param);
inline	irqreturn_t	fs_ccd_intr_int_sig(int irq, void *context_data);
enum hrtimer_restart	fs_ccd_timeout_cb(struct hrtimer *timer);
VOID	fc_ccd_wthrd_main(struct work_struct *work);

SHORT	fc_ccd_fsm_snd_req(ST_CCD_MNG_INFO *ccd);
VOID	fc_ccd_fsm_snd_cmplt(ST_CCD_MNG_INFO *ccd, SHORT sts);
VOID	fc_ccd_fsm_rcv_cmplt(ST_CCD_MNG_INFO *ccd, SHORT sts);
VOID	fc_ccd_fsm_int_l(ST_CCD_MNG_INFO *ccd);
VOID	fc_ccd_fsm_int_h(ST_CCD_MNG_INFO *ccd);
VOID	fc_ccd_fsm_int_pulse(ST_CCD_MNG_INFO *ccd, BYTE sig);
VOID	fc_ccd_fsm_tout_clk1(ST_CCD_MNG_INFO *ccd);
VOID	fc_ccd_fsm_tout_csh(ST_CCD_MNG_INFO *ccd);
VOID	fc_ccd_fsm_tout_idle(ST_CCD_MNG_INFO *ccd);
VOID	fc_ccd_fsm_tout_fail1(ST_CCD_MNG_INFO *ccd);
VOID	fc_ccd_fsm_tout_intfale(ST_CCD_MNG_INFO *ccd);
VOID	fc_ccd_fsm_tout_comn(ST_CCD_MNG_INFO *ccd);
VOID	fc_ccd_fsm_timout(ST_CCD_MNG_INFO *ccd, INT tim_num);

INT	fc_ccd_init_instance(ST_CCD_MNG_INFO *ccd);
inline	CHAR*	fc_ccd_make_gpio_str(CHAR *idn, CHAR *out, CHAR *str);
inline	ULONG	fc_ccd_calc_fcc(BYTE *buf, ULONG len);
SHORT	fc_ccd_snd_dat(ST_CCD_MNG_INFO *ccd,BYTE *sdat, USHORT len);
SHORT	fc_ccd_rcv_dat(ST_CCD_MNG_INFO *ccd,BYTE *rdat, USHORT len);
VOID	fc_ccd_snd_byte(ST_CCD_MNG_INFO *ccd, BYTE dat);
BYTE	fc_ccd_rcv_byte(ST_CCD_MNG_INFO *ccd);
SHORT	fc_ccd_snd_spi_res(ST_CCD_MNG_INFO *ccd, BYTE *sdat, USHORT len);
SHORT	fc_ccd_rcv_spi_res(ST_CCD_MNG_INFO *ccd, BYTE *rdat, USHORT len);
ssize_t	fc_ccd_spicomm_ctrl(ST_CCD_MNG_INFO *ccd, struct spi_message *msg);
VOID	fc_ccd_spicomm_cb(void *arg);
VOID	fc_ccd_tim_init(ST_CCD_MNG_INFO *ccd);
VOID	fc_ccd_tim_start(ST_CCD_MNG_INFO *ccd, USHORT timid);
VOID	fc_ccd_tim_stop(ST_CCD_MNG_INFO *ccd, USHORT timid);
VOID	fc_ccd_tim_all_stop(ST_CCD_MNG_INFO *ccd);
inline	VOID	fc_ccd_set_sig_cs(ST_CCD_MNG_INFO *ccd, INT act);
inline	BYTE	fc_ccd_get_sig_int(ST_CCD_MNG_INFO *ccd);
VOID	fc_ccd_rel_res(ST_CCD_MNG_INFO *ccd);
VOID	fc_ccd_fsm_clr(ST_CCD_MNG_INFO *ccd);
VOID	fc_ccd_init_drv_inf(ST_CCD_MNG_INFO *ccd);
SHORT	fc_ccd_chk_rcv_dat(ST_CCD_TXRX_BUF *rcv);
VOID	fc_ccd_set_rcv_lst(ST_CCD_MNG_INFO *ccd, ST_CCD_TXRX_BUF *rcv);
VOID	fc_ccd_chg_fsm_mod(ST_CCD_MNG_INFO *ccd, BYTE mode);
VOID	fc_ccd_rdy_shtdwn(ST_CCD_MNG_INFO *ccd);
inline	VOID	fc_ccd_disable_irq(ST_CCD_MNG_INFO *ccd);
inline	VOID	fc_ccd_enable_irq(ST_CCD_MNG_INFO *ccd);


/****************************************************************************
*																			*
*		SYMBOL		: module_info_def										*
*																			*
*		DESCRIPTION	: Ｌｉｎｕｘドライバ登録定義							*
*																			*
*		PARAMETER	: IN  : NONE											*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
/* module_info_def */
/*--------------------------------------------------*/
/*	登録/削除関数登録								*/
/*--------------------------------------------------*/
module_init(fs_ccd_sys_init);
module_exit(fs_ccd_sys_exit);

/*--------------------------------------------------*/
/*	ライセンス登録									*/
/*--------------------------------------------------*/
MODULE_LICENSE("GPL");

/*--------------------------------------------------*/
/*	モジュール名定義								*/
/*--------------------------------------------------*/
MODULE_ALIAS("spi:cpucom");

/*--------------------------------------------------*/
/*	Ｌｉｎｕｘシステムインターフェース登録			*/
/*--------------------------------------------------*/
const struct file_operations tb_ccd_CpuComDrvFops = {
	.owner =	THIS_MODULE,
	.write		=	fs_ccd_sys_write,
	.read		=	fs_ccd_sys_read,
	.unlocked_ioctl = fs_ccd_sys_ioctl,
	.open		=	fs_ccd_sys_open,
	.release	=	fs_ccd_sys_close,
	.llseek		=	no_llseek,
};
struct spi_driver tb_ccd_CpuComDrvInf = {
	.probe		=	fs_ccd_sys_probe,
	.remove		=	__devexit_p(fs_ccd_sys_remove),
	.suspend	=	fs_ccd_sys_suspend,
	.resume		=	fs_ccd_sys_resume,
	.driver = {
		.name	=	M_CCD_DRV_NAME,
		.owner	=	THIS_MODULE,
	},
};

#if CPUCOMMODULE==1
inline void cm_delay(int cnt){
	do{
		__asm__ __volatile__("nop");
	}while(--cnt);
}
#endif
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_drc_out										*
*																			*
*		DESCRIPTION	: ドラレコログ出力(デバック用)							*
*																			*
*		PARAMETER	: IN  : format											*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
static void fc_ccd_drc_out(char* format, ...){
	struct lites_trace_header trc_hdr;	/* ドラレコ用ヘッダー 		*/
	struct lites_trace_format trc_fmt;	/* ドラレコ用フォーマット 	*/
    va_list arg;
	char buf[256]={0};
    va_start(arg, format);
    vsprintf(buf, format, arg);
    va_end(arg);
	
	memset(&trc_fmt, 0x0, sizeof(trc_fmt));
	memset(&trc_hdr, 0x0, sizeof(trc_hdr));

	trc_fmt.buf			= buf;
	trc_fmt.count		= strnlen(buf,sizeof(buf)-1)+1;
	trc_fmt.trc_header	= &trc_hdr;     /* ドラレコ用ヘッダー格納 	*/
	trc_fmt.trc_header->level = 2;    	/* ログレベル 0が最優先		*/
	trc_fmt.trc_header->trace_id  = 0;
										/* 未使用枠：レングス入れる	*/
	trc_fmt.trc_header->format_id= (unsigned short)0xFF;	
										/*種別を格納  				*/
	trc_fmt.trc_header->trace_no = LITES_KNL_LAYER+DRIVERECODER_CPUCOMDRIVER_NO;
										/* CPU間通信ドライバを指定 	*/
	lites_trace_write(REGION_TRACE_AVC_LAN, &trc_fmt);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_drc_error										*
*																			*
*		DESCRIPTION	: ドライバ・エラー保存処理								*
*																			*
*		PARAMETER	: IN  : err_id	エラーID								*
*					: IN  : param1	パラメータ1								*
*					: IN  : param2	パラメータ2								*
*					: IN  : param3	パラメータ3								*
*					: IN  : param4	パラメータ4								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NULL											*
*																			*
****************************************************************************/
void inline fc_ccd_drc_error(	unsigned int err_id,
								unsigned int param1,
								unsigned int param2,
								unsigned int param3,
								unsigned int param4	)
{
	struct lites_trace_header trc_hdr;	/* ドラレコ用ヘッダー 		*/
	struct lites_trace_format trc_fmt;	/* ドラレコ用フォーマット 	*/
	struct driverecorder_error_data{
		unsigned int	err_id;	/* エラーＩＤ */
		unsigned int	param1;	/* 付加情報１ */
		unsigned int	param2;	/* 付加情報２ */
		unsigned int	param3;	/* 付加情報３ */
		unsigned int	param4;	/* 付加情報４ */
	}err={
		.err_id = err_id,
		.param1 = param1,
		.param2 = param2,
		.param3 = param3,
		.param4 = param4,
	};
	memset(&trc_fmt, 0x0, sizeof(trc_fmt));
	memset(&trc_hdr, 0x0, sizeof(trc_hdr));
	
	/*--------------------------------------------------*/
	/* データ作成してlitesドライバ書き込み				*/
	/*--------------------------------------------------*/

	trc_fmt.buf		=  &err;
	trc_fmt.count	=  sizeof(err);		
	trc_fmt.trc_header	= &trc_hdr;     /* ドラレコ用ヘッダー格納 	*/
	trc_fmt.trc_header->level = 2;    	/* ログレベル 0が最優先		*/
	trc_fmt.trc_header->trace_id  = 0;	/* 未使用 					*/
	trc_fmt.trc_header->format_id= (unsigned short)0;	/* 固定 	*/
	trc_fmt.trc_header->trace_no = LITES_KNL_LAYER+DRIVERECODER_CPUCOMDRIVER_NO;
										/* CPU間通信ドライバを指定 	*/
										
	/* 両方に書き込む */
	lites_trace_write(REGION_TRACE_ERROR, &trc_fmt);
	lites_trace_write(REGION_TRACE_AVC_LAN, &trc_fmt);
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_drc_event										*
*																			*
*		DESCRIPTION	: ドライバ・event保存処理								*
*																			*
*		PARAMETER	: IN  : err_id	ID										*
*					: IN  : param1	パラメータ1								*
*					: IN  : param2	パラメータ2								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NULL											*
*																			*
****************************************************************************/
enum ccd_event_formatid{
	CCD_EVENT_FORMATID_MIN = 0,
	CCD_EVENT_FORMATID_IF,		//1
	CCD_EVENT_FORMATID_WAKEUP,	//2
	CCD_EVENT_FORMATID_MAX
};
enum ccd_event_id{
	CCD_EVENT_ID_MIN = 0,
	CCD_EVENT_ID_INIT,		//1
	CCD_EVENT_ID_EXIT,		//2
	CCD_EVENT_ID_PROBE,		//3
	CCD_EVENT_ID_REMOVE,	//4
	CCD_EVENT_ID_OPEN,		//5
	CCD_EVENT_ID_REOPEN,	//6
	CCD_EVENT_ID_CLOSE,		//7
	CCD_EVENT_ID_SUSPEND,	//8
	CCD_EVENT_ID_RESUME,	//9
	CCD_EVENT_ID_READ,		//A
	CCD_EVENT_ID_WRITE,		//B
	CCD_EVENT_ID_IOCTL,		//C
	CCD_EVENT_ID_SHUTDOWN,	//D
	CCD_EVENT_ID_FLGCHECK,	//E
	CCD_EVENT_ID_WAKEUP_RX,	//F
	CCD_EVENT_ID_WAKEUP_TX,	//10
	CCD_EVENT_ID_ISSUSPEND,	//11
	CCD_EVENT_ID_FDCKECK,	//12
	CCD_EVENT_ID_FORCEEXIT,	//13
	CCD_EVENT_ID_MAX
};

void inline fc_ccd_drc_event(	unsigned int event_id,
								unsigned short format_id,
								unsigned int param1,
								unsigned int param2
							)
{
	struct lites_trace_header trc_hdr;	/* ドラレコ用ヘッダー 		*/
	struct lites_trace_format trc_fmt;	/* ドラレコ用フォーマット 	*/
	struct driverecorder_error_data{
		unsigned int	event_id;	/* エラーＩＤ */
		unsigned int	param1;	/* 付加情報１ */
		unsigned int	param2;	/* 付加情報２ */
	}err={
		.event_id = event_id,
		.param1 = param1,
		.param2 = param2,
	};
	memset(&trc_fmt, 0x0, sizeof(trc_fmt));
	memset(&trc_hdr, 0x0, sizeof(trc_hdr));
	
	/*--------------------------------------------------*/
	/* データ作成してlitesドライバ書き込み				*/
	/*--------------------------------------------------*/

	trc_fmt.buf		=  &err;
	trc_fmt.count	=  sizeof(err);		
	trc_fmt.trc_header	= &trc_hdr;     /* ドラレコ用ヘッダー格納 	*/
	trc_fmt.trc_header->level = 2;    	/* ログレベル 0が最優先		*/
	trc_fmt.trc_header->trace_id  = 0;	/* 未使用 					*/
	trc_fmt.trc_header->format_id= (unsigned short)format_id;
	trc_fmt.trc_header->trace_no = LITES_KNL_LAYER+DRIVERECODER_CPUCOMDRIVER_NO;
										/* CPU間通信ドライバを指定 	*/
										
	lites_trace_write(REGION_TRACE_DRIVER, &trc_fmt);
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_init										*
*																			*
*		DESCRIPTION	: ドライバ初期化処理									*
*																			*
*		PARAMETER	: IN  : NONE											*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
int __init	fs_ccd_sys_init(void)
{
	INT		sts;
	LONG	ret;

	/*--------------------------------------------------*/
	/*  キャラクターデバイス登録						*/
	/*--------------------------------------------------*/
	bf_ccm_major_num = register_chrdev(bf_ccm_major_num, M_CCD_DRV_NAME, &tb_ccd_CpuComDrvFops);
	if(bf_ccm_major_num < 0){
		LOGE("ERR0000:bf_ccm_major_num=%d", bf_ccm_major_num);
		fc_ccd_drc_error(0,-1,bf_ccm_major_num,0,-1);
		return	bf_ccm_major_num;
	}

	/*--------------------------------------------------*/
	/*  クラス作成										*/
	/*--------------------------------------------------*/
	bf_ccd_dev_clas = class_create(THIS_MODULE, M_CCD_DRV_NAME);
	ret = IS_ERR(bf_ccd_dev_clas);			/* エラー確認						*/
	if(ret != 0) {
		unregister_chrdev(bf_ccm_major_num, tb_ccd_CpuComDrvInf.driver.name);
		LOGE("ERR0001:ret=%d", (int)ret);
		fc_ccd_drc_error(1,-1,ret,0,-1);
		return	PTR_ERR(bf_ccd_dev_clas);
	}

	/*--------------------------------------------------*/
	/*  作成クラスをＳＰＩへ登録						*/
	/*--------------------------------------------------*/
	sts = spi_register_driver(&tb_ccd_CpuComDrvInf);
	if(sts < 0) {							/* 登録失敗？						*/
		LOGE("ERR0002:sts=%d", bf_ccm_major_num);
		fc_ccd_drc_error(2,-1,sts,0,-1);
		class_destroy(bf_ccd_dev_clas);		/* 登録削除							*/
		unregister_chrdev(bf_ccm_major_num, tb_ccd_CpuComDrvInf.driver.name);
	}
	fc_ccd_drc_event(CCD_EVENT_ID_INIT,CCD_EVENT_FORMATID_IF,sts,0);
	return	sts;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_exit										*
*																			*
*		DESCRIPTION	: ドライバ終了処理										*
*																			*
*		PARAMETER	: IN  : NONE											*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID __exit	fs_ccd_sys_exit(void)
{
	/*--------------------------------------------------*/
	/*  SPIデバイス登録解除								*/
	/*--------------------------------------------------*/
	spi_unregister_driver(&tb_ccd_CpuComDrvInf);
	/*--------------------------------------------------*/
	/*	Class登録削除									*/
	/*--------------------------------------------------*/
	class_destroy(bf_ccd_dev_clas);
	fc_ccd_drc_event(CCD_EVENT_ID_EXIT,CCD_EVENT_FORMATID_IF,0,0);
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_show_adapter_name								*
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
static ssize_t fc_ccd_show_adapter_name(struct device *dev, struct device_attribute *attr, char *buf)
{
	int no;
	UNUSED(attr);
	/*--------------------------------------------------*/
	/*	パラメータチェック								*/
	/*--------------------------------------------------*/
	if(NULL==dev
	|| NULL ==buf)
		return 0;
	no  = MINOR(dev->devt);
	return sprintf(buf, "spi-tegra-cpucom-%d", no);
}
static DEVICE_ATTR(name, S_IRUGO, fc_ccd_show_adapter_name, NULL);

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_probe										*
*																			*
*		DESCRIPTION	: ドライバ生成処理										*
*																			*
*		PARAMETER	: IN  : spiデータ										*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
INT __devinit	fs_ccd_sys_probe(struct spi_device *spi)
{
	ST_CCD_MNG_INFO		*ccd			= NULL;
	struct device		*dev			= NULL;
	CHAR 				req_str[128]	= { 0 };
	INT					status			= 0;
	ULONG				minor_num;

	/*--------------------------------------------------*/
	/*	インスタンス(管理バッファ)生成					*/
	/*--------------------------------------------------*/
	ccd = kzalloc(sizeof(*ccd), GFP_KERNEL);
											/* インスタンスメモリ確保			*/
	if(ccd == NULL) {						/* メモリ確保失敗					*/
		LOGE("[%s]ERR0003:ccd=NULL!!",ccd->dev.spi.id_name);
		fc_ccd_drc_error(3,-1,0,0,MODE);
		return	-ENOMEM;					/* エラー終了						*/
	}

	memset(ccd, 0, sizeof(*ccd));			/* 管理バッファクリア				*/
	ccd->dev.spi.sdev = spi;				/* SPI設定							*/
	spin_lock_init(&ccd->dev.spi.lockt);

	/*--------------------------------------------------*/
	/*	リスト初期化									*/
	/*--------------------------------------------------*/
	INIT_LIST_HEAD(&ccd->dev.dev_ent);
	INIT_LIST_HEAD(&ccd->comm.rx_que);

	/*--------------------------------------------------*/
	/*	ドライバ内共通データ排他	開始				*/
	/*--------------------------------------------------*/
	mutex_lock(&bf_ccd_mutx_drv_com);

	/*--------------------------------------------------*/
	/*	デバイスファイル生成							*/
	/*--------------------------------------------------*/
	minor_num = find_first_zero_bit(bf_ccd_minor_num, N_SPI_MINORS);
											/* マイナーＮｏ．空き検索			*/
	if(minor_num >= N_SPI_MINORS) {			/* 空き無し！						*/
		LOGE("[%d.%d]ERR0004:minor_num=%d",spi->master->bus_num, spi->chip_select, (int)minor_num);
		fc_ccd_drc_error(4,spi->master->bus_num,minor_num,0,MODE);
		mutex_unlock(&bf_ccd_mutx_drv_com);
		kfree(ccd);
		return -ENODEV;
	}
	ccd->dev.devt = MKDEV(bf_ccm_major_num, minor_num);
											/* デバイス番号生成					*/
	dev = device_create(bf_ccd_dev_clas,
						 &spi->dev,
						 ccd->dev.devt,
						 ccd,
						 "cpucom%d.%d",
						 spi->master->bus_num,
						 spi->chip_select);
							 				/* デバイス生成						*/
	memset(ccd->dev.spi.id_name, 0, sizeof(ccd->dev.spi.id_name));
	sprintf(ccd->dev.spi.id_name, "%d.%d", spi->master->bus_num, spi->chip_select);
	status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
	if(status != 0) {						/* 登録失敗？						*/
		/* ERROR TRACE */
		LOGE("[%s]ERR0005:status=%d",ccd->dev.spi.id_name, status);
		fc_ccd_drc_error(5,spi->master->bus_num,status,0,MODE);
		device_destroy(bf_ccd_dev_clas, ccd->dev.devt);
											/*  デバイス削除*/
		mutex_unlock(&bf_ccd_mutx_drv_com);
		kfree(ccd);
		return status;
	}
	status = device_create_file(dev, &dev_attr_name);
							 				/* デバイスファイル生成				*/

	if(status != 0) {						/* デバイスファイル生成失敗			*/
		/* ERROR TRACE */
		LOGE("[%s]ERR0006:status=%d",ccd->dev.spi.id_name, status);
		fc_ccd_drc_error(6,spi->master->bus_num,status,0,MODE);
		device_remove_file(dev, &dev_attr_name);
											/*  デバイスファイル削除			*/
		device_destroy(bf_ccd_dev_clas, ccd->dev.devt);	
											/*  デバイス削除					*/
		mutex_unlock(&bf_ccd_mutx_drv_com);
		kfree(ccd);
		return status;
	}
	set_bit(minor_num, bf_ccd_minor_num);	/* マイナーＮｏ．登録				*/

	/*--------------------------------------------------*/
	/*	デバイスファイルリスト登録						*/
	/*--------------------------------------------------*/
	list_add(&ccd->dev.dev_ent, &bf_ccd_device_list);

	/*--------------------------------------------------*/
	/*	ドライバ内共通データ排他	終了				*/
	/*--------------------------------------------------*/
	mutex_unlock(&bf_ccd_mutx_drv_com);

	/*--------------------------------------------------*/
	/*  インスタンス初期化処理							*/
	/*--------------------------------------------------*/
	status = fc_ccd_init_instance(ccd);
	if(status != 0) {						/* 登録失敗							*/
		/* ERROR TRACE */
		LOGE("[%s]ERR0007:status=%d",ccd->dev.spi.id_name, status);
		fc_ccd_drc_error(7,spi->master->bus_num,status,0,MODE);
		device_remove_file(dev, &dev_attr_name);
											/*  デバイスファイル削除			*/
		device_destroy(bf_ccd_dev_clas, ccd->dev.devt);	
											/*  デバイス削除					*/
		clear_bit(MINOR(ccd->dev.devt), bf_ccd_minor_num);
											/*  マイナーNo削除				*/
		list_del(&ccd->dev.dev_ent);		/* デバイスリスト削除				*/
		kfree(ccd);
		return status;
	}

	/*--------------------------------------------------*/
	/*  高速起動コールバック登録						*/
	/*--------------------------------------------------*/
	ccd->ed_inf.sudden_device_poweroff = fs_ccd_fb_sdn_sdwn;
	ccd->ed_inf.dev = dev;
	status = early_device_register(&ccd->ed_inf);
											/* early デバイス登録				*/
	if(status != 0) {						/* 登録失敗							*/
		/* ERROR TRACE */
		LOGE("[%s]ERR0008:status=%d",ccd->dev.spi.id_name, status);
		fc_ccd_drc_error(7,spi->master->bus_num,status,0,MODE);
		device_remove_file(dev, &dev_attr_name);
											/*  デバイスファイル削除			*/
		device_destroy(bf_ccd_dev_clas, ccd->dev.devt);	
											/*  デバイス削除					*/
		clear_bit(MINOR(ccd->dev.devt), bf_ccd_minor_num);
											/*  マイナーNo削除					*/
		list_del(&ccd->dev.dev_ent);		/* デバイスリスト削除				*/
		early_device_unregister(&ccd->ed_inf);
											/* early デバイス登録解除			*/
		destroy_workqueue(ccd->mng.wkr_thrd);
											/* ワークキュー削除					*/
		kfree(ccd);
		return status;
	}

	/*--------------------------------------------------*/
	/*  SPI登録											*/
	/*--------------------------------------------------*/
	spi_set_drvdata(spi, ccd);

	/*--------------------------------------------------*/
	/*  ポート設定										*/
	/*--------------------------------------------------*/
	gpio_request(ccd->cpu.pin.MREQ, fc_ccd_make_gpio_str(ccd->dev.spi.id_name, req_str, "MREQ"));
	gpio_request(ccd->cpu.pin.SREQ, fc_ccd_make_gpio_str(ccd->dev.spi.id_name, req_str, "SREQ"));

	tegra_gpio_enable(ccd->cpu.pin.SREQ);
	tegra_gpio_enable(ccd->cpu.pin.MREQ);

	gpio_direction_input(ccd->cpu.pin.SREQ);
	gpio_direction_output(ccd->cpu.pin.MREQ, HIGH);

	/*--------------------------------------------------*/
	/*	パタパタ制御の場合はＳＰＩもＧＰＩＯ設定する	*/
	/*--------------------------------------------------*/
#if __INTERFACE__ == IF_GPIO	/* GPIO設定の場合はすべて設定する */
	gpio_request(ccd->cpu.pin.SDAT, fc_ccd_make_gpio_str(ccd->dev.spi.id_name, req_str, "SDAT"));
	gpio_request(ccd->cpu.pin.MCLK, fc_ccd_make_gpio_str(ccd->dev.spi.id_name, req_str, "MCLK"));
	gpio_request(ccd->cpu.pin.MDAT, fc_ccd_make_gpio_str(ccd->dev.spi.id_name, req_str, "MDAT"));

	tegra_gpio_enable(ccd->cpu.pin.SDAT);
	tegra_gpio_enable(ccd->cpu.pin.MDAT);
	tegra_gpio_enable(ccd->cpu.pin.MCLK);

	gpio_direction_input(ccd->cpu.pin.SDAT);
	gpio_direction_output(ccd->cpu.pin.MDAT, LOW);
	gpio_direction_output(ccd->cpu.pin.MCLK, HIGH);
#endif

	/*--------------------------------------------------*/
	/*  TG MUTE ON										*/
	/*--------------------------------------------------*/
	gpio_request(TEGRA_GPIO_PX6, "TEGRA_MUTE");
	tegra_gpio_enable(TEGRA_GPIO_PX6);
	gpio_direction_output(TEGRA_GPIO_PX6, LOW);
	gpio_set_value(TEGRA_GPIO_PX6, LOW);

	/*--------------------------------------------------*/
	/*  割り込み設定									*/
	/*--------------------------------------------------*/
	ccd->cpu.irq.sreq_num = gpio_to_irq(ccd->cpu.pin.SREQ);
	fc_ccd_make_gpio_str(ccd->dev.spi.id_name, req_str, "IRQ");
	status = request_irq(ccd->cpu.irq.sreq_num,
						  fs_ccd_intr_int_sig,
						  (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING),
						  req_str,
						  ccd);
	if(status == ESUCCESS) {
		fc_ccd_disable_irq(ccd);			/* 割り込み禁止						*/
	}
	else {
		/* ERROR TRACE */
		LOGE("[%s]ERR0009:status=%d",ccd->dev.spi.id_name, status);
		fc_ccd_drc_error(9,spi->master->bus_num,status,0,MODE);
		device_remove_file(dev, &dev_attr_name);
											/*  デバイスファイル削除			*/
		device_destroy(bf_ccd_dev_clas, ccd->dev.devt);	
											/*  デバイス削除					*/
		clear_bit(MINOR(ccd->dev.devt), bf_ccd_minor_num);
											/*  マイナーNo削除					*/
		list_del(&ccd->dev.dev_ent);		/* デバイスリスト削除				*/
		early_device_unregister(&ccd->ed_inf);
											/* early デバイス登録解除			*/
		destroy_workqueue(ccd->mng.wkr_thrd);
											/* ワークキュー削除					*/
		free_irq(ccd->cpu.irq.sreq_num, ccd);
											/* 割り込み登録削除 				*/
		kfree(ccd);
	}
	fc_ccd_drc_event(CCD_EVENT_ID_PROBE,CCD_EVENT_FORMATID_IF,status,ccd->dev.spi.id_name[0]);
	return status;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_remove										*
*																			*
*		DESCRIPTION	: ドライバ削除処理										*
*																			*
*		PARAMETER	: IN  : spiデータ										*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
INT __devexit	fs_ccd_sys_remove(struct spi_device *spi)
{
	ST_CCD_MNG_INFO	*ccd;

	ccd = spi_get_drvdata(spi);

	/*--------------------------------------------------*/
	/*	ドライバ内共通データ排他	開始				*/
	/*--------------------------------------------------*/
	mutex_lock(&bf_ccd_mutx_drv_com);

	/*--------------------------------------------------*/
	/*	ドライバデータ初期化実施						*/
	/*--------------------------------------------------*/
	fc_ccd_init_drv_inf(ccd);
	/*--------------------------------------------------*/
	/*  解除・開放										*/
	/*--------------------------------------------------*/
	spin_lock_irq(&ccd->dev.spi.lockt);		/* SPI割り込み禁止					*/
	ccd->dev.spi.sdev = NULL;				/* SPIデバイス情報ポインタクリア	*/
	spi_set_drvdata(spi, NULL);				/* SPI登録解除			 			*/
	spin_unlock_irq(&ccd->dev.spi.lockt);	/* SPI割り込み解除					*/

	/*--------------------------------------------------*/
	/*  ＧＰＩＯ開放									*/
	/*--------------------------------------------------*/
	gpio_free(ccd->cpu.pin.MREQ);
	gpio_free(ccd->cpu.pin.SREQ);
#if __INTERFACE__ == IF_GPIO
	gpio_free(ccd->cpu.pin.MCLK);
	gpio_free(ccd->cpu.pin.SDAT);
	gpio_free(ccd->cpu.pin.MDAT);
#endif

	/*--------------------------------------------------*/
	/*	高速起動登録解除								*/
	/*--------------------------------------------------*/
	early_device_unregister(&ccd->ed_inf);	/* early デバイス登録解除			*/

	/*--------------------------------------------------*/
	/*	デバイスリスト削除								*/
	/*--------------------------------------------------*/
	device_remove_file(ccd->ed_inf.dev, &dev_attr_name);
											/*  デバイスファイル削除			*/
	list_del(&ccd->dev.dev_ent);			/* デバイスリスト削除				*/
	device_destroy(bf_ccd_dev_clas, ccd->dev.devt);
											/* デバイスクラス削除				*/
	clear_bit(MINOR(ccd->dev.devt), bf_ccd_minor_num);
											/* マイナーNo削除					*/

	/*--------------------------------------------------*/
	/*	送受信リソース削除								*/
	/*--------------------------------------------------*/
	fc_ccd_rel_res(ccd);

	/*--------------------------------------------------*/
	/*	ワーカースレッド削除							*/
	/*--------------------------------------------------*/
	destroy_workqueue(ccd->mng.wkr_thrd);	/* ワークキュー削除					*/

	/*--------------------------------------------------*/
	/*	割り込み登録削除								*/
	/*--------------------------------------------------*/
	free_irq(ccd->cpu.irq.sreq_num, ccd);

	/*--------------------------------------------------*/
	/*	インスタンス開放								*/
	/*--------------------------------------------------*/
	kfree(ccd);								/* インスタンス削除					*/

	/*--------------------------------------------------*/
	/*	ドライバ内共通データ排他	終了				*/
	/*--------------------------------------------------*/
	mutex_unlock(&bf_ccd_mutx_drv_com);
	fc_ccd_drc_event(CCD_EVENT_ID_REMOVE,CCD_EVENT_FORMATID_IF,0,ccd->dev.spi.id_name[0]);
	return 0;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_open										*
*																			*
*		DESCRIPTION	: ドライバオープン処理									*
*																			*
*		PARAMETER	: IN  : *inode : ノード									*
*						  : *filp  : ファイル								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
INT		fs_ccd_sys_open(struct inode *inode, struct file *filp)
{
	ST_CCD_MNG_INFO		*ccd;
	INT					sts		= -EFAULT;

	/*--------------------------------------------------*/
	/*	ドライバ内共通データ排他	開始				*/
	/*--------------------------------------------------*/
	mutex_lock(&bf_ccd_mutx_drv_com);

	/*--------------------------------------------------*/
	/* 	デバイスリスト検索し、インスタンス取得			*/
	/*--------------------------------------------------*/
	list_for_each_entry(ccd, &bf_ccd_device_list, dev.dev_ent) {
		if(ccd->dev.devt == inode->i_rdev) {
			sts = ESUCCESS;
			break;
		}
	}
	if(sts != ESUCCESS) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0010:sts=%d",ccd->dev.spi.id_name, sts);
		fc_ccd_drc_error(10,-1,sts,0,MODE);
		mutex_unlock(&bf_ccd_mutx_drv_com);
		return	-EFAULT;
	}
	filp->private_data = ccd;
	nonseekable_open(inode, filp);

	/*--------------------------------------------------*/
	/*	インスタンス排他	開始						*/
	/*--------------------------------------------------*/
	mutex_lock(&ccd->sys.mutex.instnc);

	if(ccd->dev.users > 0){
		LOGD("[%s]RE-OPEN",ccd->dev.spi.id_name);
		fc_ccd_drc_event(CCD_EVENT_ID_REOPEN,CCD_EVENT_FORMATID_IF,ccd->dev.users,ccd->dev.spi.id_name[0]);
	}
	/*--------------------------------------------------*/
	/*	ドライバデータ初期化実施						*/
	/*--------------------------------------------------*/
	fc_ccd_init_drv_inf(ccd);

	/*--------------------------------------------------*/
	/*	ユーザ数加算									*/
	/*--------------------------------------------------*/
	ccd->dev.users++;

	/*--------------------------------------------------*/
	/*	受付許可										*/
	/*--------------------------------------------------*/
	ccd->bf_ccd_acpt_flg = TRUE;

	/*--------------------------------------------------*/
	/*	シャットダウンフラグＯＦＦ						*/
	/*--------------------------------------------------*/
	bf_ccd_shut_down_flg = OFF;

	/*--------------------------------------------------*/
	/*	状態遷移情報クリア（IDLEステートへ遷移）		*/
	/*--------------------------------------------------*/
	fc_ccd_fsm_clr(ccd);

	
	/*--------------------------------------------------*/
	/*	ドライバ割り込み許可							*/
	/*--------------------------------------------------*/
	fc_ccd_enable_irq(ccd);

	/*--------------------------------------------------*/
	/*	インスタンス排他	終了						*/
	/*--------------------------------------------------*/
	mutex_unlock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/*	ドライバ内共通データ排他	終了				*/
	/*--------------------------------------------------*/
	mutex_unlock(&bf_ccd_mutx_drv_com);
	LOGD("[%s]OPEN",ccd->dev.spi.id_name);
	fc_ccd_drc_event(CCD_EVENT_ID_OPEN,CCD_EVENT_FORMATID_IF,ccd->dev.users,ccd->dev.spi.id_name[0]);
	return sts;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_close										*
*																			*
*		DESCRIPTION	: ドライバクローズ処理									*
*																			*
*		PARAMETER	: IN  : *inode : ノード									*
*						  : *filp  : ファイル								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
INT		fs_ccd_sys_close(struct inode *inode, struct file *filp)
{
	ST_CCD_MNG_INFO		*ccd;
	ccd = filp->private_data;
	/*--------------------------------------------------*/
	/*	ドライバ内共通データ排他	開始				*/
	/*--------------------------------------------------*/
	mutex_lock(&bf_ccd_mutx_drv_com);
	if(ccd){
		LOGD("[%s]RELEASE",ccd->dev.spi.id_name);
		/*--------------------------------------------------*/
		/*	ユーザ数チェック								*/
		/*--------------------------------------------------*/
		if(ccd->dev.users > 0) {
			ccd->dev.users --;
		}
		if(ccd->dev.users <= 0){
			ccd->dev.users= 0;
			/*--------------------------------------------------*/
			/*	ドライバ情報初期化								*/
			/*--------------------------------------------------*/
			fc_ccd_init_drv_inf(ccd);
		}
	}
	/*--------------------------------------------------*/
	/*	ドライバ内共通データ排他	終了				*/
	/*--------------------------------------------------*/
	mutex_unlock(&bf_ccd_mutx_drv_com);
	fc_ccd_drc_event(CCD_EVENT_ID_CLOSE,CCD_EVENT_FORMATID_IF,ccd->dev.users,ccd->dev.spi.id_name[0]);
	return 0;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_suspend									*
*																			*
*		DESCRIPTION	: ドライバサスペンド処理								*
*																			*
*		PARAMETER	: IN  : *spi : spiデータ								*
*							mesg : メッセージ								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
INT		fs_ccd_sys_suspend(struct spi_device *spi, pm_message_t mesg)
{
	ST_CCD_MNG_INFO	*ccd;

	ccd = spi_get_drvdata(spi);
	LOGD("[%s]SUSPEND",ccd->dev.spi.id_name);

	/*--------------------------------------------------*/
	/*	ドライバ内共通データ排他	開始				*/
	/*--------------------------------------------------*/
	mutex_lock(&bf_ccd_mutx_drv_com);

	/*--------------------------------------------------*/
	/*	インスタンス排他	開始						*/
	/*--------------------------------------------------*/
	mutex_lock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/*	ドライバ情報初期化								*/
	/*--------------------------------------------------*/
	fc_ccd_init_drv_inf(ccd);

	/*--------------------------------------------------*/
	/*	シャットダウンフラグＯＮ						*/
	/*--------------------------------------------------*/
	bf_ccd_shut_down_flg = ON;

	ccd->bf_ccd_resume_not_yet = TRUE;

	/*--------------------------------------------------*/
	/*	送受信待ちの待ち解除							*/
	/*--------------------------------------------------*/
	if(ccd->sys.event.tx_flg == 0){
		ccd->sys.event.tx_flg = 1;
		fc_ccd_drc_event(CCD_EVENT_ID_WAKEUP_TX,CCD_EVENT_FORMATID_WAKEUP,CCD_EVENT_ID_SUSPEND,ccd->dev.spi.id_name[0]);
		wake_up_interruptible(&ccd->sys.event.tx_evq);

	}
	if(ccd->sys.event.rx_flg == 0){
		ccd->sys.event.rx_flg = 1;
		fc_ccd_drc_event(CCD_EVENT_ID_WAKEUP_RX,CCD_EVENT_FORMATID_WAKEUP,CCD_EVENT_ID_SUSPEND,ccd->dev.spi.id_name[0]);
		wake_up_interruptible(&ccd->sys.event.rx_evq);
	}

	/*--------------------------------------------------*/
	/*	インスタンス排他	終了						*/
	/*--------------------------------------------------*/
	mutex_unlock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/*	ドライバ内共通データ排他	終了				*/
	/*--------------------------------------------------*/
	mutex_unlock(&bf_ccd_mutx_drv_com);
	fc_ccd_drc_event(CCD_EVENT_ID_SUSPEND,CCD_EVENT_FORMATID_IF,bf_ccd_shut_down_flg,ccd->dev.spi.id_name[0]);
	return	0;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_resume										*
*																			*
*		DESCRIPTION	: ドライバリジューム処理								*
*																			*
*		PARAMETER	: IN  : *spi : spiデータ								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
INT		fs_ccd_sys_resume(struct spi_device *spi)
{
	ST_CCD_MNG_INFO	*ccd;

	ccd = spi_get_drvdata(spi);
	LOGD("[%s]RESUME",ccd->dev.spi.id_name);

	/*--------------------------------------------------*/
	/*	ドライバ内共通データ排他	開始				*/
	/*--------------------------------------------------*/
	mutex_lock(&bf_ccd_mutx_drv_com);

	/*--------------------------------------------------*/
	/*	インスタンス排他	開始						*/
	/*--------------------------------------------------*/
	mutex_lock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/*	ドライバ情報初期化								*/
	/*--------------------------------------------------*/
	fc_ccd_init_drv_inf(ccd);


	ccd->bf_ccd_resume_not_yet = FALSE;
	/*--------------------------------------------------*/
	/*	送受信待ちの待ち解除							*/
	/*--------------------------------------------------*/
	if(ccd->sys.event.tx_flg == 0){
		ccd->sys.event.tx_flg = 1;
		fc_ccd_drc_event(CCD_EVENT_ID_WAKEUP_TX,CCD_EVENT_FORMATID_WAKEUP,CCD_EVENT_ID_RESUME,ccd->dev.spi.id_name[0]);
		wake_up_interruptible(&ccd->sys.event.tx_evq);
	}
	if(ccd->sys.event.rx_flg == 0){
		ccd->sys.event.rx_flg = 1;
		fc_ccd_drc_event(CCD_EVENT_ID_WAKEUP_RX,CCD_EVENT_FORMATID_WAKEUP,CCD_EVENT_ID_RESUME,ccd->dev.spi.id_name[0]);
		wake_up_interruptible(&ccd->sys.event.rx_evq);
	}

	/*--------------------------------------------------*/
	/*	インスタンス排他	終了						*/
	/*--------------------------------------------------*/
	mutex_unlock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/*	ドライバ内共通データ排他	終了				*/
	/*--------------------------------------------------*/
	mutex_unlock(&bf_ccd_mutx_drv_com);
	fc_ccd_drc_event(CCD_EVENT_ID_RESUME,CCD_EVENT_FORMATID_IF,bf_ccd_shut_down_flg,ccd->dev.spi.id_name[0]);
	return	0;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_read										*
*																			*
*		DESCRIPTION	: ドライバリード処理									*
*																			*
*		PARAMETER	: IN  : *filp  : ファイル								*
*						  : *buf   : バッファ								*
*						  : count  : カウント								*
*						  : *f_pos : ポジション								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
ssize_t	fs_ccd_sys_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ST_CCD_MNG_INFO		*ccd;
	ST_CCD_TXRX_BUF		*rcv = NULL;
	INT					lst_sts;
	INT					wai_sts;
	INT					ent;
	BYTE				sts;
	ULONG				cpy_sts;
	ssize_t				read_length=0;

	/*--------------------------------------------------*/
	/*	コンテキストチェック							*/
	/*--------------------------------------------------*/
	ccd = (ST_CCD_MNG_INFO*)(filp->private_data);
	if(ccd == NULL) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0011:ccd=NULL",ccd->dev.spi.id_name);
		fc_ccd_drc_error(7,-1,0,0,MODE);

		return	-EFAULT;
	}
	LOGD("[%s]READ -->",ccd->dev.spi.id_name);

	sts = M_CCD_CSTS_OK;

	/*--------------------------------------------------*/
	/*	受信ＩＦ排他制御	開始						*/
	/*--------------------------------------------------*/
	mutex_lock(&ccd->sys.mutex.rd_if);

	/*--------------------------------------------------*/
	/*	インスタンス排他	開始						*/
	/*--------------------------------------------------*/
	mutex_lock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/* 	ドライバ割り込み禁止							*/
	/*--------------------------------------------------*/
	fc_ccd_disable_irq(ccd);

	/*--------------------------------------------------*/
	/*	状態チェック									*/
	/*--------------------------------------------------*/
	
	if((ccd->mng.mode == M_CCD_MODE_C0_INIT)
	 || (ccd->bf_ccd_acpt_flg == FALSE)){
		sts = M_CCD_CSTS_NG;
	}
	if(bf_ccd_shut_down_flg == ON) {
		sts = M_CCD_CSTS_SUS;
	}

	if(sts == M_CCD_CSTS_OK) {
		/*--------------------------------------------------*/
		/*	受信データが無い場合は受信を待つ				*/
		/*--------------------------------------------------*/
		lst_sts = list_empty(&ccd->comm.rx_que);
		if(lst_sts != 0) {					/* 受信リストが空？					*/
			/*--------------------------------------------------*/
			/*	受信バッファに”待ち”を設定					*/
			/*--------------------------------------------------*/
			ccd->comm.rx_buf.sts = M_CCD_ISTS_READY;

			/*--------------------------------------------------*/
			/* 	ドライバ割り込み許可							*/
			/*--------------------------------------------------*/
			fc_ccd_enable_irq(ccd);

			/*--------------------------------------------------*/
			/*	待ち中はインスタンス排他解除する				*/
			/*--------------------------------------------------*/
			mutex_unlock(&ccd->sys.mutex.instnc);

			/*--------------------------------------------------*/
			/*	受信待ち										*/
			/*--------------------------------------------------*/
			ccd->sys.event.rx_flg = 0;
			wai_sts = wait_event_interruptible(ccd->sys.event.rx_evq, ccd->sys.event.rx_flg);
											/* 受信待ち							*/

			/*--------------------------------------------------*/
			/*	待ち中のインスタンス排他を再度開始				*/
			/*--------------------------------------------------*/
			mutex_lock(&ccd->sys.mutex.instnc);

			/*--------------------------------------------------*/
			/* 	ドライバ割り込み禁止							*/
			/*--------------------------------------------------*/
			fc_ccd_disable_irq(ccd);
		}

		/*--------------------------------------------------*/
		/*	サスペンド中断で待ち解除の時はサスペンド終了	*/
		/*--------------------------------------------------*/
		if(bf_ccd_shut_down_flg == ON) {
			sts = M_CCD_CSTS_SUS;
		}
		else {
			/*--------------------------------------------------*/
			/*	受信データを取得								*/
			/*--------------------------------------------------*/
			rcv = NULL;
			ent = list_empty(&ccd->comm.rx_que);
			if(ent == 1) {					/* からっぽだ！						*/
				/* ERROR TRACE */
				LOGE("[%s]ERR0012:ent=1",ccd->dev.spi.id_name);
				fc_ccd_drc_error(12,CHANNEL,0,0,MODE);
			}
			else {
				rcv = list_first_entry(&ccd->comm.rx_que, ST_CCD_TXRX_BUF, list);
				list_del(&rcv->list);		/* リストから削除					*/
			}

			/*--------------------------------------------------*/
			/*	ユーザランドへコピー							*/
			/*--------------------------------------------------*/
			if(rcv != NULL) {
				/*--------------------------------------------------*/
				/*	受信終了ステータス確認							*/
				/*--------------------------------------------------*/
				if(rcv->sts == M_CCD_ISTS_CMPLT){
					cpy_sts = copy_to_user(buf, rcv->dat, rcv->len);
					if(cpy_sts != 0) {			/* コピー失敗？						*/
						/* ERROR TRACE */
						LOGE("[%s]ERR0013:cpy_sts=%d rcv_len=%d",ccd->dev.spi.id_name, (int)cpy_sts, (int)rcv->len);
						fc_ccd_drc_error(13,CHANNEL,cpy_sts,rcv->len,MODE);

						sts = M_CCD_CSTS_NG;	/* エラー値セット					*/
					}else{						/* コピー成功						*/
						/*--------------------------------------------------*/
						/*	受信レングス確定								*/
						/*--------------------------------------------------*/
						read_length=rcv->len;
					}
				}else
				if(rcv->sts == M_CCD_ISTS_SUSPEND){
					sts = M_CCD_CSTS_SUS;
				}else{
					sts = M_CCD_CSTS_NG;	/* エラー値セット					*/
					LOGW("[%s]WAR0000:sts=%d",ccd->dev.spi.id_name, ccd->comm.rx_buf.sts);
				}
				/*--------------------------------------------------*/
				/*	受信バッファのメモリ開放						*/
				/*--------------------------------------------------*/
				kfree(rcv);
			}
			else{
				/* ERROR TRACE */
				LOGE("[%s]ERR0014:rcv=NULL",ccd->dev.spi.id_name);
				fc_ccd_drc_error(14,CHANNEL,0,0,MODE);

				sts = M_CCD_CSTS_NG;
			}
		}
	}

	/*--------------------------------------------------*/
	/* 	ドライバ割り込み許可							*/
	/*--------------------------------------------------*/
	fc_ccd_enable_irq(ccd);

	/*--------------------------------------------------*/
	/*	インスタンス排他	終了						*/
	/*--------------------------------------------------*/
	mutex_unlock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/*	受信ＩＦ排他制御	終了						*/
	/*--------------------------------------------------*/
	mutex_unlock(&ccd->sys.mutex.rd_if);

	/*--------------------------------------------------*/
	/*	戻り値設定										*/
	/*--------------------------------------------------*/
	if(bf_ccd_shut_down_flg == ON)
		sts = M_CCD_CSTS_SUS;
	if(sts == M_CCD_CSTS_OK) {
		LOGD("[%s]READ <--(%d) ",ccd->dev.spi.id_name,read_length);
		return	read_length;					/* 正常終了							*/
	}
	else
	if(sts == M_CCD_CSTS_SUS) {
		LOGD("[%s]READ <--(%d) ",ccd->dev.spi.id_name,PRE_SUSPEND_RETVAL);
		fc_ccd_drc_event(CCD_EVENT_ID_READ,CCD_EVENT_FORMATID_IF,PRE_SUSPEND_RETVAL,ccd->dev.spi.id_name[0]);
		return	PRE_SUSPEND_RETVAL;
	}
	else {
		fc_ccd_drc_event(CCD_EVENT_ID_READ,CCD_EVENT_FORMATID_IF,-EFAULT,ccd->dev.spi.id_name[0]);
		LOGD("[%s]READ <--(%d) ",ccd->dev.spi.id_name,-EFAULT);
		return	-EFAULT;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_write										*
*																			*
*		DESCRIPTION	: ドライバライト処理									*
*																			*
*		PARAMETER	: IN  : *filp  : ファイル								*
*						  : *buf   : バッファ								*
*						  : *f_pos : ポジション								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
ssize_t	fs_ccd_sys_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	ST_CCD_MNG_INFO		*ccd;
	ST_CCD_TXRX_BUF		*snd;
	ssize_t				ret		= -EFAULT;
	USHORT				sts		= M_CCD_CSTS_OK;
	ULONG				cpy_sts	= 0;
	INT					wai_sts;

	ccd = (ST_CCD_MNG_INFO*)(filp->private_data);

	/*--------------------------------------------------*/
	/*	コンテキストチェック							*/
	/*--------------------------------------------------*/
	if(ccd == NULL) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0015:ccd=NULL",ccd->dev.spi.id_name);
		fc_ccd_drc_error(15,CHANNEL,0,0,MODE);
		return -EFAULT;
	}
	LOGD("[%s]WRITE -->",ccd->dev.spi.id_name);
	/*--------------------------------------------------*/
	/*	送信ＩＦ排他制御	開始						*/
	/*--------------------------------------------------*/
	mutex_lock(&ccd->sys.mutex.wt_if);

	/*--------------------------------------------------*/
	/*	インスタンス排他	開始						*/
	/*--------------------------------------------------*/
	mutex_lock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/* 	ドライバ割り込み禁止							*/
	/*--------------------------------------------------*/
	fc_ccd_disable_irq(ccd);

	snd = &ccd->comm.tx_buf;
	sts = M_CCD_CSTS_OK;

	/*--------------------------------------------------*/
	/*	パラメータチェック								*/
	/*--------------------------------------------------*/
	if(count == 0) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0016:count=0",ccd->dev.spi.id_name);
		fc_ccd_drc_error(16,CHANNEL,0,0,MODE);
		sts = M_CCD_CSTS_NG;
	}
	if(count > M_CCD_TRNS_IF_MAX) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0017:count=%d",ccd->dev.spi.id_name, count);
		fc_ccd_drc_error(17,CHANNEL,count,0,MODE);
		sts = M_CCD_CSTS_NG;
	}
	
	/*--------------------------------------------------*/
	/*	状態チェック									*/
	/*--------------------------------------------------*/
	if(bf_ccd_shut_down_flg == ON) {
		sts = M_CCD_CSTS_SUS;
	}
	if((ccd->mng.mode == M_CCD_MODE_C0_INIT)
	 || (ccd->bf_ccd_acpt_flg == FALSE)){
		sts = M_CCD_CSTS_NG;
	}

	/*--------------------------------------------------*/
	/*	パラメータデータコピー							*/
	/*	※0バイト目はLENを後で格納する					*/
	/*--------------------------------------------------*/
	if(sts == M_CCD_CSTS_OK) {
		cpy_sts	 = copy_from_user(&ccd->comm.tx_buf.dat[1], buf, count);
		if(cpy_sts != 0) {					/* コピー失敗？						*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0018:cpy_sts=%d, count=%d",ccd->dev.spi.id_name, (int)cpy_sts, (int)count);
			fc_ccd_drc_error(18,CHANNEL,cpy_sts,count,MODE);
			sts = M_CCD_CSTS_NG;
		}
	}

	/*--------------------------------------------------*/
	/*	送信用ＬＥＮデータ計算（4の倍数）				*/
	/*--------------------------------------------------*/
	if((count % 4) != 0) {
		/* WARNING TRACE */
		LOGW("[%s]WAR0001:count=%d",ccd->dev.spi.id_name, count);

		count = (count + 3) & 0xfffffffc;	/* ４の倍数に変換					*/
	}
	ccd->comm.tx_buf.dat[0]	= count / 4;
	ccd->comm.tx_buf.len	= count;

	/*--------------------------------------------------*/
	/*	状態遷移処理（送信要求）						*/
	/*--------------------------------------------------*/
	if(sts == M_CCD_CSTS_OK) {
		sts = fc_ccd_fsm_snd_req(ccd);
	}

	/*--------------------------------------------------*/
	/*	送信起動していれば送信完了を待つ				*/
	/*--------------------------------------------------*/
	if(sts == M_CCD_CSTS_OK) {
		/*--------------------------------------------------*/
		/* 	ドライバ割り込み許可							*/
		/*--------------------------------------------------*/
		fc_ccd_enable_irq(ccd);

		/*--------------------------------------------------*/
		/*	待ち中はインスタンス排他解除する				*/
		/*--------------------------------------------------*/
		mutex_unlock(&ccd->sys.mutex.instnc);

		/*--------------------------------------------------*/
		/*	送信完了待ち									*/
		/*--------------------------------------------------*/
		ccd->sys.event.tx_flg = 0;
		wai_sts = wait_event_interruptible(ccd->sys.event.tx_evq, ccd->sys.event.tx_flg);
											/* 送信完了待ち						*/

		/*--------------------------------------------------*/
		/*	待ち中のインスタンス排他を再度開始				*/
		/*--------------------------------------------------*/
		mutex_lock(&ccd->sys.mutex.instnc);

		/*--------------------------------------------------*/
		/* 	ドライバ割り込み禁止							*/
		/*--------------------------------------------------*/
		fc_ccd_disable_irq(ccd);
	}

	/*--------------------------------------------------*/
	/*	戻り値設定										*/
	/*--------------------------------------------------*/
	if(sts == M_CCD_CSTS_OK) {
		if(snd->sts == M_CCD_ISTS_CMPLT) {	/* 正常終了時は送信バイト数			*/
			ret = count;
		}
		else
		if((snd->sts == M_CCD_ISTS_SUSPEND)	/* サスペンドによる終了				*/
		 || (bf_ccd_shut_down_flg == ON)) {	/* サスペンド状態					*/
			ret = PRE_SUSPEND_RETVAL;
		}
		else {								/* その他 ＝ エラー終了				*/
			ret = -EFAULT;
		}
	}
	else
	if(sts == M_CCD_CSTS_SUS) {
		ret = PRE_SUSPEND_RETVAL;
	}
	else {
		if(count == 0) {					/* 元から０バイト指定の場合は正常	*/
			ret = 0;
		}
		else {
			ret = -EFAULT;
		}
	}
	snd->sts = M_CCD_ISTS_NONE;				/* 送信データ無し設定				*/

	/*--------------------------------------------------*/
	/* 	ドライバ割り込み許可							*/
	/*--------------------------------------------------*/
	fc_ccd_enable_irq(ccd);

	/*--------------------------------------------------*/
	/*	インスタンス排他	終了						*/
	/*--------------------------------------------------*/
	mutex_unlock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/*	送信ＩＦ排他制御	開放						*/
	/*--------------------------------------------------*/
	mutex_unlock(&ccd->sys.mutex.wt_if);
	if(bf_ccd_shut_down_flg == ON)
		ret = PRE_SUSPEND_RETVAL;
	LOGD("[%s]WRITE <--(%d)",ccd->dev.spi.id_name,ret);
	if(ret == PRE_SUSPEND_RETVAL || ret <= 0)
		fc_ccd_drc_event(CCD_EVENT_ID_WRITE,CCD_EVENT_FORMATID_IF,ret,ccd->dev.spi.id_name[0]);
	return	ret;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_ioctl										*
*																			*
*		DESCRIPTION	: 高速起動・終了通知処理								*
*																			*
*		PARAMETER	: IN  : *filp  : ファイル								*
*					: IN  : cmd    : コマンド								*
*					: IN  : arg	   : 引数									*
*					  RET : リターンコード									*
*																			*
*		CAUTION		: IOCTL													*
*																			*
****************************************************************************/
long fs_ccd_sys_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int			err = 0;
	long		ret = -EFAULT;
	ST_CCD_MNG_INFO		*ccd;
	unsigned long result = FALSE;
	/*--------------------------------------------------*/
	/*	マジックコード確認								*/
	/*--------------------------------------------------*/
	if (_IOC_TYPE(cmd) != CPU_IOC_MAGIC){
		LOGE("MAGIC CODE NOT MUTCH");
		return -ENOTTY;
	}
	/*--------------------------------------------------*/
	/*	アクセスパーミッション確認						*/
	/*--------------------------------------------------*/
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
				(void __user *)arg, _IOC_SIZE(cmd));
	if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
				(void __user *)arg, _IOC_SIZE(cmd));
	if (err){
		LOGE("permission err");
		return -EFAULT;
	}
	/*--------------------------------------------------*/
	/*	送信ＩＦ排他制御	開始						*/
	/*--------------------------------------------------*/
	ccd = filp->private_data;
	switch (cmd) {
	case CPU_IOC_ISSUSPEND:
		LOGD("ioctl:CPU_IOC_ISSUSPEND");
		if(bf_ccd_shut_down_flg
		|| TRUE == early_device_invoke_with_flag_irqsave(RNF_BUDET_ACCOFF_INTERRUPT_MASK,fc_ccd_fb_chk_sus, NULL)){
			result = TRUE;
		}else{
			result = FALSE;
		}
		if(!copy_to_user( (int __user*)arg,&result , sizeof(unsigned long)))
			ret=ESUCCESS;
		fc_ccd_drc_event(CCD_EVENT_ID_ISSUSPEND,CCD_EVENT_FORMATID_IF,result,ccd->dev.spi.id_name[0]);
		break;
	case CPU_IOC_FDCKECK:
		if(ccd->bf_ccd_resume_not_yet == TRUE){
			result = FALSE;
		}else{
			result = TRUE;
		}
		if(!copy_to_user( (int __user*)arg,&result , sizeof(unsigned long)))
			ret=ESUCCESS;
		fc_ccd_drc_event(CCD_EVENT_ID_FDCKECK,CCD_EVENT_FORMATID_IF,ccd->bf_ccd_acpt_flg,ccd->dev.spi.id_name[0]);
		break;
	case CPU_IOC_EXIT:
		/*--------------------------------------------------*/
		/*	送受信待ちの待ち解除							*/
		/*--------------------------------------------------*/
		if(ccd->sys.event.tx_flg == 0){
			ccd->sys.event.tx_flg = 1;
			fc_ccd_drc_event(CCD_EVENT_ID_WAKEUP_TX,CCD_EVENT_FORMATID_WAKEUP,CCD_EVENT_ID_SUSPEND,ccd->dev.spi.id_name[0]);
			wake_up_interruptible(&ccd->sys.event.tx_evq);

		}
		if(ccd->sys.event.rx_flg == 0){
			ccd->sys.event.rx_flg = 1;
			fc_ccd_drc_event(CCD_EVENT_ID_WAKEUP_RX,CCD_EVENT_FORMATID_WAKEUP,CCD_EVENT_ID_SUSPEND,ccd->dev.spi.id_name[0]);
			wake_up_interruptible(&ccd->sys.event.rx_evq);
		}
		/*--------------------------------------------------*/
		/*	ドライバ情報初期化								*/
		/*--------------------------------------------------*/
		fc_ccd_init_drv_inf(ccd);
		
		fc_ccd_drc_event(CCD_EVENT_ID_FORCEEXIT,CCD_EVENT_FORMATID_IF,ccd->bf_ccd_acpt_flg,ccd->dev.spi.id_name[0]);
		break;
	default:
		LOGW("[%s]WAR0001:cmd=0x%x",ccd->dev.spi.id_name, cmd);
		break;
	}
	/*--------------------------------------------------*/
	/*	送信ＩＦ排他制御	開放						*/
	/*--------------------------------------------------*/
	fc_ccd_drc_event(CCD_EVENT_ID_IOCTL,CCD_EVENT_FORMATID_IF,ret,ccd->dev.spi.id_name[0]);
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_fb_sdn_sdwn									*
*																			*
*		DESCRIPTION	: 高速起動・終了通知処理								*
*																			*
*		PARAMETER	: IN  : devデータ										*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : リターンコード									*
*																			*
*		CAUTION		: 高速起動のコールバックは時間のかかる処理は禁止！！	*
*																			*
****************************************************************************/
VOID	fs_ccd_fb_sdn_sdwn(struct device *dev)
{
	/*--------------------------------------------------*/
	/*	高速化の為、排他制御はしない					*/
	/*--------------------------------------------------*/
	LOGD("sudden_device_poweroff");
	/*--------------------------------------------------*/
	/*	シャットダウンフラグＯＮ						*/
	/*--------------------------------------------------*/
	bf_ccd_shut_down_flg = ON;

	fc_ccd_drc_event(CCD_EVENT_ID_SHUTDOWN,CCD_EVENT_FORMATID_IF,0,0);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fb_chk_sus										*
*																			*
*		DESCRIPTION	: 高速起動・サスペンド確認処理							*
*																			*
*		PARAMETER	: IN  : devデータ										*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
INT		fc_ccd_fb_chk_sus(bool flag, void* param)
{
	if(flag == 0) {
		return	FALSE;
	}
	else {
		return	TRUE;
	}
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_intr_int_sig									*
*																			*
*		DESCRIPTION	: 割り込みハンドラ										*
*																			*
*		PARAMETER	: IN  : irq  : 割り込み									*
*																			*
*					  IN  : *context_data  : コンテキスト					*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
inline	irqreturn_t	fs_ccd_intr_int_sig(int irq, void *context_data)
{
	ST_CCD_MNG_INFO		*ccd;
	BYTE				sig;

	/*--------------------------------------------------*/
	/*	インスタンス取得								*/
	/*--------------------------------------------------*/

	ccd = (ST_CCD_MNG_INFO *)context_data;
	if(ccd == NULL) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0019:ccd=NULL",ccd->dev.spi.id_name);
		fc_ccd_drc_error(19,CHANNEL,0,0,MODE);
		return IRQ_HANDLED;
	}

	/*--------------------------------------------------*/
	/*	ＩＮＴ信号状態取得	※できるだけ早く取得！		*/
	/*--------------------------------------------------*/
	sig = fc_ccd_get_sig_int(ccd);

	/*--------------------------------------------------*/
	/*	汎用タイマカウンタクリア						*/
	/*--------------------------------------------------*/
	ccd->tim.tcnt = 0;

	/*--------------------------------------------------*/
	/*	自己設定時割り込み無視処理						*/
	/*--------------------------------------------------*/
	if(ccd->cpu.irq.sreq_selfsts != 0xFF){
		if((BYTE)ccd->cpu.irq.sreq_selfsts == sig ){
			LOGD("[%s]ERR0045:interrupt ignore selfsetting sig[%d]",ccd->dev.spi.id_name,sig);
			return IRQ_HANDLED;
		}else{
			/* 違う割り込みが来たらクリア */
			ccd->cpu.irq.sreq_selfsts = 0xFF;
		}
	}
	/*--------------------------------------------------*/
	/*	変化毎に状態遷移処理呼び出し					*/
	/*--------------------------------------------------*/
	if(ccd->cpu.irq.sreq_sts != sig) {
		/*--------------------------------------------------*/
		/*	ちゃんとINT受信したら							*/
		/*	INT FALSE TIMER 停止	フラグも解除			*/
		/*--------------------------------------------------*/
		fc_ccd_tim_stop(ccd, M_CCD_TIM_INTFAIL);
		ccd->cpu.irq.tim_res &= ~M_CCD_TFG_INTFAIL;
		
		if(sig == ACTIVE) {
			fc_ccd_fsm_int_l(ccd);			/* INT L 割込み						*/
		}
		else {
			fc_ccd_fsm_int_h(ccd);			/* INT H 割込み						*/
		}
	}
	/*--------------------------------------------------*/
	/*	認識状態と同一のときは短時間でＨＬの可能性有り	*/
	/*--------------------------------------------------*/
	else {
		/* WARNING TRACE */
		//COLD BOOT ・Sus-Res時に出易い
		//Resume起動時間に関わるのでLOG表示しない
		//LOGW("[%s]WAR0002:sig=%d mode=%d",ccd->dev.spi.id_name, sig, ccd->mng.mode);

		fc_ccd_fsm_int_pulse(ccd, sig);
	}

	/*--------------------------------------------------*/
	/*	ＩＮＴ信号状態取得								*/
	/*--------------------------------------------------*/
	ccd->cpu.irq.sreq_sts = sig;

	return IRQ_HANDLED;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_timeout_cb										*
*																			*
*		DESCRIPTION	: タイムアウトコールバック								*
*																			*
*		PARAMETER	: IN  : *timer : タイマデータ							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
enum hrtimer_restart	fs_ccd_timeout_cb(struct hrtimer *timer)
{
	enum hrtimer_restart	ret = HRTIMER_NORESTART;
	ST_CCD_TIMER_INFO		*ts;
	ST_CCD_MNG_INFO			*ccd;

	/*--------------------------------------------------*/
	/*	タイマ情報インスタンス取得						*/
	/*--------------------------------------------------*/
	ts = container_of(timer, ST_CCD_TIMER_INFO, hndr);
	if(ts == 0) {
		/* ERROR TRACE */
		LOGE("ERR0020:ts=0");
		fc_ccd_drc_error(20,CHANNEL,0,0,MODE);
		return ret;
	}

	ccd = (ST_CCD_MNG_INFO*)(ts->inst_p);

	/*--------------------------------------------------*/
	/*	ドライバ割込禁止状態の場合は要因だけ残し、		*/
	/*	タイムアウト処理は割り込み解除処理の中で実行する*/
	/*--------------------------------------------------*/


	if(ccd->cpu.irq.ena_sts == M_CCD_DISABLE) {
LOGT("[%s]Timer Pending(%s)",ccd->dev.spi.id_name,stateTBL[ts->tim_num].str);
		switch(ts->tim_num) {
			case	M_CCD_TIM_TCLK1:	/*---* ｔｃｌｋ１*----------------------*/
				ccd->cpu.irq.tim_res |= M_CCD_TFG_TCLK1;
				break;

			case	M_CCD_TIM_TCSH:		/*---* ｔｃｓｈ*------------------------*/
				ccd->cpu.irq.tim_res |= M_CCD_TFG_TCSH;
				break;

			case	M_CCD_TIM_TIDLE:	/*---* ｔｉｄｌｅ*----------------------*/
				ccd->cpu.irq.tim_res |= M_CCD_TFG_TIDLE;
				break;

			case	M_CCD_TIM_TFAIL1:	/*---* ｔｆａｉｌ１*--------------------*/
				ccd->cpu.irq.tim_res |= M_CCD_TFG_TFAIL1;
				break;

			case	M_CCD_TIM_COMON:	/*---* 汎用タイマ *---------------------*/
				ccd->cpu.irq.tim_res |= M_CCD_TFG_COMON;
				break;

			case	M_CCD_TIM_INTFAIL:	/*---* INT FALESAFEタイマ *-------------*/
				ccd->cpu.irq.tim_res |= M_CCD_TFG_INTFAIL;
				break;

			default:					/*---* その他（エラー）	*---------------*/
				/* ERROR TRACE */
				LOGE("[%s]ERR0021:tim_num=%d",ccd->dev.spi.id_name, ts->tim_num);
				fc_ccd_drc_error(21,CHANNEL,ts->tim_num,0,MODE);
				break;
		}
	}
	/*--------------------------------------------------*/
	/*	ドライバ割込許可状態の場合はタイムアウト処理実行*/
	/*--------------------------------------------------*/
	else{
		/*--------------------------------------------------*/
		/* 	ドライバ割り込み禁止							*/
		/*--------------------------------------------------*/
		fc_ccd_disable_irq(ccd);

		/*--------------------------------------------------*/
		/*	タイムアウト種別毎に状態遷移処理実施			*/
		/*--------------------------------------------------*/
		fc_ccd_fsm_timout(ccd, ts->tim_num);

		/*--------------------------------------------------*/
		/* 	ドライバ割り込み許可							*/
		/*--------------------------------------------------*/
		fc_ccd_enable_irq(ccd);
	}

	return ret;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_wthrd_main										*
*																			*
*		DESCRIPTION	: ワーカースレッドメイン処理							*
*																			*
*		PARAMETER	: IN  : *work  :ワーカースレッドデータ					*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_wthrd_main(struct work_struct *work)
{
	ST_CCD_WORKER_INFO	*wkr_inf;
	ST_CCD_MNG_INFO		*ccd;
	SHORT				ret;

	/*--------------------------------------------------*/
	/*	インスタンス取得								*/
	/*--------------------------------------------------*/
	wkr_inf	= container_of(work, ST_CCD_WORKER_INFO, workq);
	ccd		= (ST_CCD_MNG_INFO*)(wkr_inf->inst_p);
	if(ccd == NULL) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0022:ccd=NULL",ccd->dev.spi.id_name);
		fc_ccd_drc_error(22,CHANNEL,0,0,MODE);
		return;								/* 中断								*/
	}

	/*--------------------------------------------------*/
	/*	送受信処理										*/
	/*--------------------------------------------------*/
	switch(wkr_inf->eve_num){
		case M_CCD_WINF_SND:	/*---* 送信イベント *-----------------------*/
			LOGM("[%s] M_CCD_WINF_SND",ccd->dev.spi.id_name);
			/*--------------------------------------------------*/
			/*	割り込み等で状態変更していれば何もしない		*/
			/*--------------------------------------------------*/
			if(ccd->mng.mode != M_CCD_MODE_S1_SNDC) {
				LOGT("[%s]%s(%d) NOT M_CCD_MODE_S1_SNDC(%s)",ccd->dev.spi.id_name,__FUNCTION__,__LINE__,stateTBL[ccd->mng.mode].str);
				break;
			}

			/*--------------------------------------------------*/
			/*	データ送信（※ＬＥＮ、ＦＣＣデータ長加味する）	*/
			/*--------------------------------------------------*/
			ret = fc_ccd_snd_dat(ccd,
								 ccd->comm.tx_buf.dat,
								 ccd->comm.tx_buf.len + M_CCD_LEN_SIZE + M_CCD_FCC_SIZE);
								 			/* 送信処理							*/
#if CPUCOMDEBUG	== 1
/*fc_ccd_drc_out( M_CCD_DRV_NAME	"[%s] %s(%d):Driver Tx data=0x%02X%02X%02X%02X%02X%02X%02X%02X",ccd->dev.spi.id_name,__FUNCTION__,	__LINE__,
											ccd->comm.tx_buf.dat[0],
											ccd->comm.tx_buf.dat[1],
											ccd->comm.tx_buf.dat[2],
											ccd->comm.tx_buf.dat[3],
											ccd->comm.tx_buf.dat[4],
											ccd->comm.tx_buf.dat[5],
											ccd->comm.tx_buf.dat[6],
											ccd->comm.tx_buf.dat[7]
											);
*/
#endif

			/*--------------------------------------------------*/
			/*	インスタンス排他	開始						*/
			/*--------------------------------------------------*/
			mutex_lock(&ccd->sys.mutex.instnc);

			/*--------------------------------------------------*/
			/* 	ドライバ割り込み禁止							*/
			/*--------------------------------------------------*/
			fc_ccd_disable_irq(ccd);

			/*--------------------------------------------------*/
			/*	状態遷移：送信完了								*/
			/*--------------------------------------------------*/
			fc_ccd_fsm_snd_cmplt(ccd, ret);	/* 送信完了処理						*/

			/*--------------------------------------------------*/
			/* 	ドライバ割り込み許可							*/
			/*--------------------------------------------------*/
			fc_ccd_enable_irq(ccd);

			/*--------------------------------------------------*/
			/*	インスタンス排他	終了						*/
			/*--------------------------------------------------*/
			mutex_unlock(&ccd->sys.mutex.instnc);
			break;

		case M_CCD_WINF_RCV:	/*---* 受信イベント *-----------------------*/
			LOGM("[%s] M_CCD_WINF_RCV",ccd->dev.spi.id_name);
			/*--------------------------------------------------*/
			/*	割り込み等で状態変更していれば何もしない		*/
			/*--------------------------------------------------*/
			if(ccd->mng.mode != M_CCD_MODE_R1_RCVC) {
				LOGT("[%s]%s(%d) NOT M_CCD_WINF_RCV(%s)",ccd->dev.spi.id_name,__FUNCTION__,__LINE__,stateTBL[ccd->mng.mode].str);
				break;
			}

			/*--------------------------------------------------*/
			/*	データ受信（※ＦＣＣデータ長加味する）			*/
			/*--------------------------------------------------*/
			ret = fc_ccd_rcv_dat(ccd,
								 ccd->comm.rx_buf.dat,
								 ccd->comm.rx_buf.len + M_CCD_FCC_SIZE);
									 		/* 受信処理							*/

			/*--------------------------------------------------*/
			/*	インスタンス排他	開始						*/
			/*--------------------------------------------------*/
			mutex_lock(&ccd->sys.mutex.instnc);

			/*--------------------------------------------------*/
			/* 	ドライバ割り込み禁止							*/
			/*--------------------------------------------------*/
			fc_ccd_disable_irq(ccd);

			/*--------------------------------------------------*/
			/*	状態遷移：受信完了								*/
			/*--------------------------------------------------*/
			fc_ccd_fsm_rcv_cmplt(ccd, ret);	/* 受信完了処理						*/

			/*--------------------------------------------------*/
			/* 	ドライバ割り込み許可							*/
			/*--------------------------------------------------*/
			fc_ccd_enable_irq(ccd);

			/*--------------------------------------------------*/
			/*	インスタンス排他	終了						*/
			/*--------------------------------------------------*/
			mutex_unlock(&ccd->sys.mutex.instnc);
			break;

		default:				/*---* その他（エラー）*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0023:wkr_inf->eve_num=%d",ccd->dev.spi.id_name, wkr_inf->eve_num);
			fc_ccd_drc_error(23,CHANNEL,wkr_inf->eve_num,0,MODE);
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_snd_req									*
*																			*
*		DESCRIPTION	: 状態遷移処理「送信要求」								*
*																			*
*		PARAMETER	: IN  : ccd		: インスタンス							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : 送信起動有無									*
*																			*
****************************************************************************/
SHORT	fc_ccd_fsm_snd_req(ST_CCD_MNG_INFO *ccd)
{
	ST_CCD_TXRX_BUF		*snd;
	ULONG				fcc;
	SHORT				ret;

	snd = &ccd->comm.tx_buf;
	ret = M_CCD_CSTS_NG;					/* デフォルト＝送信起動無し		*/

	/*--------------------------------------------------*/
	/*	状態チェック									*/
	/*--------------------------------------------------*/
	if(bf_ccd_shut_down_flg == ON) {
		return	M_CCD_CSTS_SUS;
	}
	if((ccd->mng.mode == M_CCD_MODE_C0_INIT)
	 || (ccd->bf_ccd_acpt_flg == FALSE)){
		return	M_CCD_CSTS_NG;
	}


	/*--------------------------------------------------*/
	/*	送信データが残っている場合(ありえない)はログ出力*/
	/*	※送信ロックを避ける為に上書きする				*/
	/*--------------------------------------------------*/
	if(snd->sts != M_CCD_ISTS_NONE) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0024:snd->stsm=%d",ccd->dev.spi.id_name, snd->sts);
		fc_ccd_drc_error(24,CHANNEL,snd->sts,0,MODE);
	}

	/*--------------------------------------------------*/
	/*	送信データ作成									*/
	/*--------------------------------------------------*/
	fcc = fc_ccd_calc_fcc(snd->dat, snd->len+1);
											/* FCC計算(LENデータも含める）		*/
	snd->dat[snd->len + 1] = (BYTE)((fcc & 0xFF000000) >> 24);
	snd->dat[snd->len + 2] = (BYTE)((fcc & 0x00FF0000) >> 16);
	snd->dat[snd->len + 3] = (BYTE)((fcc & 0x0000FF00) >>  8);
	snd->dat[snd->len + 4] = (BYTE)((fcc & 0x000000FF)      );
											/* FCC格納(格納位置はLENデータ加味)	*/
	snd->sts = M_CCD_ISTS_READY;			/* ステータスを準備中に設定			*/

	/*--------------------------------------------------*/
	/*	状態毎の対応実施								*/
	/*--------------------------------------------------*/
	switch(ccd->mng.mode) {
		case M_CCD_MODE_C1_IDLE:	/*---* 共通：アイドル *---------------------*/
			/*--------------------------------------------------*/
			/*	ＣＳ＝Ｌ出力（アクティブ）						*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, ACTIVE);

			/*--------------------------------------------------*/
			/*	ｔｃｌｋ１タイマ起動							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TCLK1);

			/*--------------------------------------------------*/
			/*	送信待ちへ遷移									*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S0_SNDW);
			ret = M_CCD_CSTS_OK;			/* 送信起動あり						*/
			break;

		case M_CCD_MODE_C2_KPID:	/*---* 共通：アイドル保持期間 *-------------*/
		case M_CCD_MODE_S4_ITHW:	/*---* 送信：受信移行中　ＩＮＴＨ待ち *-----*/
		case M_CCD_MODE_S5_ITLW:	/*---* 送信：受信移行中　ＩＮＴＬ待ち *-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* 受信：ＩＮＴ　Ｈ待ち *---------------*/
		case M_CCD_MODE_R1_RCVC:	/*---* 受信：受信完了待ち *-----------------*/
		case M_CCD_MODE_E0_INTE:	/*---* 異常：ＩＮＴ異常 *-------------------*/
///仕様変更
		case M_CCD_MODE_S3_CSHK:	/*---* 送信：送信後　ＣＳＨ保持期間 *-------*/
			/*--------------------------------------------------*/
			/*	アイドル後に送信を行うのを待つ					*/
			/*--------------------------------------------------*/
			ret = M_CCD_CSTS_OK;			/* 送信起動あり						*/
			break;

		/*--------------------------------------------------*/
		/*	送信中はイベント発生する事はない（エラー扱い）	*/
		/*--------------------------------------------------*/
		case M_CCD_MODE_C0_INIT:	/*---* 共通：初期化待ち（初期値）*----------*/
		case M_CCD_MODE_S0_SNDW:	/*---* 送信：送信待ち *---------------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* 送信：送信完了待ち *-----------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* 送信：ＣＳ　Ｈ待ち *-----------------*/
///仕様変更				case M_CCD_MODE_S3_CSHK:	/*---* 送信：送信後　ＣＳＨ保持期間 *-------*/
		default:					/*---* その他（エラー）*--------------------*/
			/*--------------------------------------------------*/
			/*	ｔｃｌｋ１タイマ停止	（動いていたら）		*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TCLK1].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TCLK1);
			/*--------------------------------------------------*/
			/*	ｔｃｓｈタイマ停止	（動いていたら）		*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TCSH].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TCSH);
			/*--------------------------------------------------*/
			/*	ｔｉｄｌｅタイマ停止	（動いていたら）		*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TIDLE].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TIDLE);
				
			/* ERROR TRACE */
			LOGE("[%s]ERR0025:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(25,CHANNEL,ccd->mng.mode,0,MODE);
			fc_ccd_fsm_clr(ccd);			/* 状態遷移情報クリア（フェール）	*/
			break;
	}

	return	ret;
}


/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_snd_cmplt									*
*																			*
*		DESCRIPTION	: 状態遷移処理：「送信完了」							*
*																			*
*		PARAMETER	: IN  : ccd		: インスタンス							*
*						  : sts		: 送信ステータス						*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_snd_cmplt(ST_CCD_MNG_INFO *ccd, SHORT sts)
{
	ST_CCD_TXRX_BUF		*snd;

	snd = &ccd->comm.tx_buf;

	/*--------------------------------------------------*/
	/*	状態毎の対応実施								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_S1_SNDC:	/*---* 送信：送信完了待ち *-----------------*/
			/*--------------------------------------------------*/
			/*	正常終了時は送信動作継続						*/
			/*--------------------------------------------------*/
			if(sts == M_CCD_ISTS_CMPLT) {
				LOGT("[%s]SEND M_CCD_ISTS_CMPLT",ccd->dev.spi.id_name);
///仕様変更
				/*--------------------------------------------------*/
				/*	ＣＳ＝Ｈ出力（インアクティブ）					*/
				/*--------------------------------------------------*/
				fc_ccd_set_sig_cs(ccd, NEGATIVE);
				/*--------------------------------------------------*/
				/*	ｔｃｓｈタイマ起動								*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_TCSH);

				/*--------------------------------------------------*/
				/*	ＣＳＨ待ちへ遷移								*/
				/*--------------------------------------------------*/
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S2_CSHW);

			}
			/*--------------------------------------------------*/
			/*	シャットダウンの場合は送信待ち解除し初期状態へ	*/
			/*--------------------------------------------------*/
			else
			if(sts == M_CCD_ISTS_SUSPEND) {
				LOGT("[%s]SEND M_CCD_ISTS_SUSPEND",ccd->dev.spi.id_name);
				/*--------------------------------------------------*/
				/*	送信ステータスをサスペンドに設定				*/
				/*--------------------------------------------------*/
				snd->sts = sts;		 		/* 終了ステータス設定				*/

				/*--------------------------------------------------*/
				/*	シャットダウン準備（初期状態へ遷移）			*/
				/*--------------------------------------------------*/
				fc_ccd_rdy_shtdwn(ccd);

			}
			/*--------------------------------------------------*/
			/*	中断（INT#L発生）は中断させ、その後リトライ	*/
			/*--------------------------------------------------*/
			else
			if(sts == M_CCD_ISTS_STOP) {
				LOGT("[%s]SEND M_CCD_ISTS_STOP",ccd->dev.spi.id_name);
				/*--------------------------------------------------*/
				/*	中断ステータスを設定する						*/
				/*--------------------------------------------------*/
				snd->sts = sts;				/* 終了ステータス設定				*/

				/*--------------------------------------------------*/
				/*	基本的には以降は何もしない→ＩＮＴ割込みでの処理を待つ	*/
				/*	ＩＮＴ FALE SAFE Timer							*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			}
			/*--------------------------------------------------*/
			/*	エラー終了の場合は中断ステータスを設定する		*/
			/*	※CS H保持完了後、STOPでのリトライを実施		*/
			/*--------------------------------------------------*/
			else {
				LOGT("[%s]SEND other",ccd->dev.spi.id_name);
				/*--------------------------------------------------*/
				/*	中断ステータスを設定する						*/
				/*--------------------------------------------------*/
				snd->sts = M_CCD_ISTS_STOP;	/* 終了ステータス=中断設定			*/

				/*--------------------------------------------------*/
				/*	ＣＳ信号をＨにする								*/
				/*--------------------------------------------------*/
				fc_ccd_set_sig_cs(ccd, NEGATIVE);

				/*--------------------------------------------------*/
				/*	ｔｉｄｌｅタイマ起動							*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_TIDLE);

				/*--------------------------------------------------*/
				/*	送信後ＣＳＨ保持期間へ移行						*/
				/*--------------------------------------------------*/
///仕様変更				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S3_CSHK);
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C2_KPID);
			}
			break;

		case M_CCD_MODE_C0_INIT:	/*---* 共通：初期化待ち（初期値）*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* 共通：アイドル *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* 共通：アイドル保持期間 *-------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* 送信：送信待ち *---------------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* 送信：ＣＳ　Ｈ待ち *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* 送信：送信後　ＣＳＨ保持期間 *-------*/
		case M_CCD_MODE_S4_ITHW:	/*---* 送信：受信移行中　ＩＮＴＨ待ち *-----*/
		case M_CCD_MODE_S5_ITLW:	/*---* 送信：受信移行中　ＩＮＴＬ待ち *-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* 受信：ＩＮＴ　Ｈ待ち *---------------*/
		case M_CCD_MODE_R1_RCVC:	/*---* 受信：受信完了待ち *-----------------*/
		case M_CCD_MODE_E0_INTE:	/*---* 異常：ＩＮＴ異常 *-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			break;

		default:					/*---* その他（エラー）*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0026:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(26,CHANNEL,ccd->mng.mode,0,MODE);
			fc_ccd_fsm_clr(ccd);			/* 状態遷移情報クリア（フェール）	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_rcv_cmplt									*
*																			*
*		DESCRIPTION	: 状態遷移処理：「受信完了」							*
*																			*
*		PARAMETER	: IN  : ccd		: インスタンス							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_rcv_cmplt(ST_CCD_MNG_INFO *ccd, SHORT sts)
{
	ST_CCD_TXRX_BUF		*rcv;
	USHORT				chk;

	rcv = &ccd->comm.rx_buf;

	chk = STS_OK;							/* デフォルトＯＫ					*/

	/*--------------------------------------------------*/
	/*	状態毎の対応実施								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_R1_RCVC:	/*---* 受信：受信完了待ち *-----------------*/
			/*--------------------------------------------------*/
			/*	シャットダウンの場合は送信待ち解除し初期状態へ	*/
			/*--------------------------------------------------*/
			if(sts == M_CCD_ISTS_SUSPEND) {
				LOGT("[%s]%s(%d) M_CCD_ISTS_SUSPEND",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);

				/*--------------------------------------------------*/
				/*	送信ステータスをサスペンドに設定				*/
				/*--------------------------------------------------*/
				rcv->sts = sts;		 		/* 終了ステータス設定				*/

				/*--------------------------------------------------*/
				/*	シャットダウン準備（初期状態へ遷移）			*/
				/*--------------------------------------------------*/
				fc_ccd_rdy_shtdwn(ccd);

				break;						/* 処理終了							*/
			}
			/*--------------------------------------------------*/
			/*	正常受信完了時はデータ確認実施					*/
			/*--------------------------------------------------*/
			else
			if(sts == M_CCD_ISTS_CMPLT) {
				LOGT("[%s]%s(%d) M_CCD_ISTS_CMPLT",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
				/*--------------------------------------------------*/
				/*	データ確認										*/
				/*--------------------------------------------------*/
				chk = fc_ccd_chk_rcv_dat(rcv);

				/*--------------------------------------------------*/
				/*	受信データＯＫならReadIF処理へ通知				*/
				/*--------------------------------------------------*/
				if(chk == M_CCD_CSTS_OK) {
					LOGT("[%s]%s(%d) M_CCD_CSTS_OK",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
					/*--------------------------------------------------*/
					/*	完了ステータス設定								*/
					/*--------------------------------------------------*/
					rcv->sts = M_CCD_ISTS_CMPLT;

					/*--------------------------------------------------*/
					/*	受信データをリストに格納						*/
					/*--------------------------------------------------*/
					fc_ccd_set_rcv_lst(ccd, rcv);
					rcv->sts = M_CCD_ISTS_NONE;

					/*--------------------------------------------------*/
					/*	受信待ちの待ち解除								*/
					/*--------------------------------------------------*/
					if(ccd->sys.event.rx_flg == 0) {
						ccd->sys.event.rx_flg = 1;
						LOGT("[%s]%s(%d) Wake Up Rx",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
						wake_up_interruptible(&ccd->sys.event.rx_evq);
					}
				}
				/*--------------------------------------------------*/
				/*	受信データＮＧなら何もしない（再度受信を待つ）	*/
				/*--------------------------------------------------*/
				else {
					LOGT("[%s]%s(%d) M_CCD_CSTS_NG",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
					/* WARNING TRACE */
					LOGW("[%s]WAR0003:chk=%d",ccd->dev.spi.id_name, chk);
				}
			}

			/*--------------------------------------------------*/
			/*	ＣＳ信号をＨにする								*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);

			/*--------------------------------------------------*/
			/*	ｔｉｄｌｅタイマ起動							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TIDLE);

			/*--------------------------------------------------*/
			/*	アイドル保持期間へ移行							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C2_KPID);
			break;

		case M_CCD_MODE_C0_INIT:	/*---* 共通：初期化待ち（初期値）*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* 共通：アイドル *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* 共通：アイドル保持期間 *-------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* 送信：送信待ち *---------------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* 送信：ＣＳ　Ｈ待ち *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* 送信：送信後　ＣＳＨ保持期間 *-------*/
		case M_CCD_MODE_S4_ITHW:	/*---* 送信：受信移行中　ＩＮＴＨ待ち *-----*/
		case M_CCD_MODE_S5_ITLW:	/*---* 送信：受信移行中　ＩＮＴＬ待ち *-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* 受信：ＩＮＴ　Ｈ待ち *---------------*/
		case M_CCD_MODE_E0_INTE:	/*---* 異常：ＩＮＴ異常 *-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			break;

		default:					/*---* その他（エラー）*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0027:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(27,CHANNEL,ccd->mng.mode,0,MODE);
			fc_ccd_fsm_clr(ccd);			/* 状態遷移情報クリア（フェール）	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_int_l										*
*																			*
*		DESCRIPTION	: 状態遷移処理「ＩＮＴ－Ｌ割込み」						*
*																			*
*		PARAMETER	: IN  : ccd		: インスタンス							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_int_l(ST_CCD_MNG_INFO *ccd)
{
	ST_CCD_TXRX_BUF		*rcv;
	SHORT				sts;
	BYTE				len;

	rcv = &ccd->comm.rx_buf;

	/*--------------------------------------------------*/
	/*	状態毎の対応実施								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_C1_IDLE:	/*---* 共通：アイドル *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* 共通：アイドル保持期間 *-------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* 送信：送信待ち *---------------------*/
		case M_CCD_MODE_S5_ITLW:	/*---* 送信：受信移行中　ＩＮＴＬ待ち *-----*/
			/*--------------------------------------------------*/
			/*	ｔｆａｉｌ１タイマ停止							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_stop(ccd, M_CCD_TIM_TFAIL1);
			/*--------------------------------------------------*/
			/*	ｔｃｌｋ１タイマ停止	（動いていたら）		*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TCLK1].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TCLK1);
			/*--------------------------------------------------*/
			/*	ｔｉｄｌｅタイマ停止	（動いていたら）		*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TIDLE].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TIDLE);

			/*--------------------------------------------------*/
			/*	ＣＳ＝Ｌ出力（アクティブ）						*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, ACTIVE);

			/*--------------------------------------------------*/
			/*	ＬＥＮ（１バイト）を受信						*/
			/*--------------------------------------------------*/
#if	__INTERFACE__ == IF_SPI_TEGRA		/* SPI-TEGRAでの通信 */
			sts = fc_ccd_rcv_spi_res(ccd, &len, 1);
#else /* __INTERFACE__ == IF_GPIO */	/* GPIO制御での通信 */
			len = fc_ccd_rcv_byte(ccd);
			/*◆ レングスチェック */
			if(len)	sts = M_CCD_CSTS_OK;
			else	sts = M_CCD_CSTS_NG;
#endif
			/*--------------------------------------------------*/
			/*	受信正常終了した場合、受信動作継続				*/
			/*--------------------------------------------------*/
			if(sts == M_CCD_CSTS_OK) {
				/*--------------------------------------------------*/
				/*	受信データ長格納（受信値の４倍）				*/
				/*--------------------------------------------------*/
				rcv->rcv_len = len;
				rcv->len = len * 4;

				/*--------------------------------------------------*/
				/*	ＩＮＴ　Ｈ待ちへ遷移							*/
				/*--------------------------------------------------*/
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_R0_ITHW);
				/*--------------------------------------------------*/
				/*	ＩＮＴ FALE SAFE Timer							*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			}
			/*--------------------------------------------------*/
			/*	受信異常終了した場合、受信動作キャンセル		*/
			/*--------------------------------------------------*/
			else {
				LOGT("[%s]%s(%d) RX length error",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
				/*--------------------------------------------------*/
				/*	ＣＳ＝Ｈ出力（インアクティブ）					*/
				/*--------------------------------------------------*/
				fc_ccd_set_sig_cs(ccd, NEGATIVE);

				/*--------------------------------------------------*/
				/*	ｔｉｄｌｅタイマ起動							*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_TIDLE);

				/*--------------------------------------------------*/
				/*	アイドル保持期間へ移行							*/
				/*--------------------------------------------------*/
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C2_KPID);
			}
			break;

		case M_CCD_MODE_S1_SNDC:	/*---* 送信：送信完了待ち *-----------------*/
			/*--------------------------------------------------*/
			/*	ＣＳ＝Ｈ出力（インアクティブ）					*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);

			/*--------------------------------------------------*/
			/*	ＩＮＴ　Ｈ待ちへ遷移							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S4_ITHW);
			/*--------------------------------------------------*/
			/*	ＩＮＴ FALE SAFE Timer							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			break;

		case M_CCD_MODE_S2_CSHW:	/*---* 送信：ＣＳ　Ｈ待ち *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* 送信：送信後　ＣＳＨ保持期間 *-------*/
			/*--------------------------------------------------*/
			/*	ｔｃｓｈ、ｔｉｄｌｅタイマ停止					*/
			/*--------------------------------------------------*/
			fc_ccd_tim_stop(ccd, M_CCD_TIM_TCSH);
			fc_ccd_tim_stop(ccd, M_CCD_TIM_TIDLE);

			/*--------------------------------------------------*/
			/*	ＣＳ＝Ｈ出力（インアクティブ）					*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);

			/*--------------------------------------------------*/
			/*	ＩＮＴ　Ｈ待ちへ遷移							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S4_ITHW);
			/*--------------------------------------------------*/
			/*	ＩＮＴ FALE SAFE Timer							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			break;

		case M_CCD_MODE_R1_RCVC:	/*---* 受信：受信完了待ち *-----------------*/
			/* WARNIG TRACE */
			LOGW("[%s]WAR0004:mode-event SH?",ccd->dev.spi.id_name);

			/*--------------------------------------------------*/
			/*	ＣＳ＝Ｈ出力（インアクティブ）					*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);

			/*--------------------------------------------------*/
			/*	ＩＮＴ　Ｈ待ちへ遷移							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S4_ITHW);
			/*--------------------------------------------------*/
			/*	ＩＮＴ FALE SAFE Timer							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			break;

		case M_CCD_MODE_C0_INIT:	/*---* 共通：初期化待ち（初期値）*----------*/
		case M_CCD_MODE_R0_ITHW:	/*---* 受信：ＩＮＴ　Ｈ待ち *---------------*/
		case M_CCD_MODE_S4_ITHW:	/*---* 送信：受信移行中　ＩＮＴＨ待ち *-----*/
		case M_CCD_MODE_E0_INTE:	/*---* 異常：ＩＮＴ異常 *-------------------*/
			/* WARNING TRACE */
			LOGW("[%s]WAR0007:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			break;

		default:					/*---* その他（エラー）*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0028:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(28,CHANNEL,ccd->mng.mode,0,MODE);
			fc_ccd_fsm_clr(ccd);			/* 状態遷移情報クリア（フェール）	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_int_h										*
*																			*
*		DESCRIPTION	: 状態遷移処理「ＩＮＴ－Ｈ割込み」						*
*																			*
*		PARAMETER	: IN  : ccd		: インスタンス							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_int_h(ST_CCD_MNG_INFO *ccd)
{
	/*--------------------------------------------------*/
	/*	状態毎の対応実施								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_S4_ITHW:	/*---* 送信：受信移行中　ＩＮＴＨ待ち *-----*/
			/*--------------------------------------------------*/
			/*	ｔｉｄｌｅタイマ停止	（動いていたら）		*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TIDLE].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TIDLE);
			/*--------------------------------------------------*/
			/*	ｔｃｌｋ１タイマ停止	（動いていたら）		*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TCLK1].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TCLK1);
			/*--------------------------------------------------*/
			/*	ｔｆａｉｌ１タイマ起動							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TFAIL1);

			/*--------------------------------------------------*/
			/*	ＩＮＴ　Ｌ待ちへ遷移							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S5_ITLW);
			/*--------------------------------------------------*/
			/*	ＩＮＴ FALE SAFE Timer							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			break;

		case M_CCD_MODE_R0_ITHW:	/*---* 受信：ＩＮＴ　Ｈ待ち *---------------*/
			/*--------------------------------------------------*/
			/*	受信完了待ちへ遷移								*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_R1_RCVC);
			/*--------------------------------------------------*/
			/*	受信起動：ワーカースレッド起動					*/
			/*--------------------------------------------------*/
			queue_work(ccd->mng.wkr_thrd,
						(struct work_struct*)&ccd->mng.wkr_inf[M_CCD_WINF_RCV]);
			break;

		case M_CCD_MODE_E0_INTE:	/*---* 異常：ＩＮＴ異常 *-------------------*/
			/*--------------------------------------------------*/
			/*	ｔｉｄｌｅタイマ起動							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TIDLE);

			/*--------------------------------------------------*/
			/*	アイドル保持期間期間へ遷移						*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C2_KPID);
			break;

		case M_CCD_MODE_C0_INIT:	/*---* 共通：初期化待ち（初期値）*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* 共通：アイドル *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* 共通：アイドル保持期間 *-------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* 送信：送信待ち *---------------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* 送信：送信完了待ち *-----------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* 送信：ＣＳ　Ｈ待ち *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* 送信：送信後　ＣＳＨ保持期間 *-------*/
		case M_CCD_MODE_S5_ITLW:	/*---* 送信：受信移行中　ＩＮＴＬ待ち *-----*/
		case M_CCD_MODE_R1_RCVC:	/*---* 受信：受信完了待ち *-----------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			/* WARNING TRACE */
			LOGW("[%s]WAR0008:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			break;

		default:					/*---* その他（エラー）*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0029:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(29,CHANNEL,ccd->mng.mode,0,MODE);
			fc_ccd_fsm_clr(ccd);			/* 状態遷移情報クリア（フェール）	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_int_pulse									*
*																			*
*		DESCRIPTION	: 状態遷移処理「ＩＮＴパルス割込み」					*
*																			*
*		PARAMETER	: IN  : ccd		: インスタンス							*
*						  : sig		: INT信号状態							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_int_pulse(ST_CCD_MNG_INFO *ccd, BYTE sig)
{
	/*--------------------------------------------------*/
	/*	状態毎の対応実施								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s <- %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str,sig == ACTIVE?"ACTIVE":"NEGATIVE");
	switch(ccd->mng.mode) {
		case M_CCD_MODE_C2_KPID:	/*---* 共通：アイドル保持期間 H*-------------*/
			/*--------------------------------------------------*/
			/*	ｔｉｄｌｅタイマ停止							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_stop(ccd, M_CCD_TIM_TIDLE);

			/*--------------------------------------------------*/
			/*	ｔｆａｉｌ１タイマ起動							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TFAIL1);

			/*--------------------------------------------------*/
			/*	ＩＮＴ　Ｌ待ちへ遷移							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S5_ITLW);
			/*--------------------------------------------------*/
			/*	ＩＮＴ FALE SAFE Timer							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			break;
		case M_CCD_MODE_S0_SNDW:	/*---* 送信：送信待ち H*---------------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* 送信：送信完了待ち H*-----------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* 送信：ＣＳ　Ｈ待ち H*-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* 送信：送信後　ＣＳＨ保持期間 H*-------*/

			/*--------------------------------------------------*/
			/*	ｔｃｌｋ１タイマ停止	（動いていたら）	*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TCLK1].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TCLK1);
			/*--------------------------------------------------*/
			/*	ｔｉｄｌｅタイマ停止	（動いていたら）	*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TIDLE].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TIDLE);
			/*--------------------------------------------------*/
			/*	ｔｃｓｈタイマ停止	（動いていたら）	*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TCSH].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TCSH);

			/*--------------------------------------------------*/
			/*	ＣＳ＝Ｈ出力（インアクティブ）					*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);
			/*--------------------------------------------------*/
			/*	ｔｆａｉｌ１タイマ起動							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TFAIL1);
			/*--------------------------------------------------*/
			/*	ＩＮＴ　Ｌ待ちへ遷移							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S5_ITLW);
			/*--------------------------------------------------*/
			/*	ＩＮＴ FALE SAFE Timer							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			break;

		case M_CCD_MODE_S5_ITLW:	/*---* 送信：受信移行中　ＩＮＴＬ待ち H*-----*/
			/*--------------------------------------------------*/
			/*	ｔｆａｉｌ１タイマ再起動						*/
			/*--------------------------------------------------*/
			fc_ccd_tim_stop(ccd, M_CCD_TIM_TFAIL1);
			fc_ccd_tim_start(ccd, M_CCD_TIM_TFAIL1);
			break;

		case M_CCD_MODE_R1_RCVC:	/*---* 受信：受信完了待ち H*-----------------*/
			/* WARNING TRACE */
			LOGW("[%s]WAR0005:mode-event SH?",ccd->dev.spi.id_name);

			/*--------------------------------------------------*/
			/*	ＣＳ＝Ｈ出力（インアクティブ）					*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);

			/*--------------------------------------------------*/
			/*	ｔｆａｉｌ１タイマ起動							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TFAIL1);

			/*--------------------------------------------------*/
			/*	ＩＮＴ　Ｌ待ちへ遷移							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S5_ITLW);
			/*--------------------------------------------------*/
			/*	ＩＮＴ FALE SAFE Timer							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			break;

		case M_CCD_MODE_C0_INIT:	/*---* 共通：初期化待ち（初期値）?*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* 共通：アイドル H*---------------------*/
		case M_CCD_MODE_S4_ITHW:	/*---* 送信：受信移行中　ＩＮＴＨ待ち L*-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* 受信：ＩＮＴ　Ｈ待ち L*---------------*/
		case M_CCD_MODE_E0_INTE:	/*---* 異常：ＩＮＴ異常 L*-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			/* 何もしない */
			break;

		default:					/*---* その他（エラー）*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0030:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(30,CHANNEL,ccd->mng.mode,0,MODE);
			fc_ccd_fsm_clr(ccd);			/* 状態遷移情報クリア（フェール）	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_tout_clk1									*
*																			*
*		DESCRIPTION	: 状態遷移処理「ｔｃｌｋ１　Ｔ．Ｏ．」					*
*																			*
*		PARAMETER	: IN  : ccd		: インスタンス							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_tout_clk1(ST_CCD_MNG_INFO *ccd)
{
	/*--------------------------------------------------*/
	/*	状態毎の対応実施								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_S0_SNDW:	/*---* 送信：送信待ち *---------------------*/
			/*--------------------------------------------------*/
			/*	送信完了待ちへ遷移								*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S1_SNDC);
			/*--------------------------------------------------*/
			/*	送信起動：ワーカースレッド起動					*/
			/*--------------------------------------------------*/
			queue_work(ccd->mng.wkr_thrd,
						(struct work_struct*)&ccd->mng.wkr_inf[M_CCD_WINF_SND]);

			break;

		case M_CCD_MODE_C0_INIT:	/*---* 共通：初期化待ち（初期値）*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* 共通：アイドル *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* 共通：アイドル保持期間 *-------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* 送信：送信完了待ち *-----------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* 送信：ＣＳ　Ｈ待ち *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* 送信：送信後　ＣＳＨ保持期間 *-------*/
		case M_CCD_MODE_S4_ITHW:	/*---* 送信：受信移行中　ＩＮＴＨ待ち *-----*/
		case M_CCD_MODE_S5_ITLW:	/*---* 送信：受信移行中　ＩＮＴＬ待ち *-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* 受信：ＩＮＴ　Ｈ待ち *---------------*/
		case M_CCD_MODE_R1_RCVC:	/*---* 受信：受信完了待ち *-----------------*/
		case M_CCD_MODE_E0_INTE:	/*---* 異常：ＩＮＴ異常 *-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			/* 何もしない */
			break;

		default:					/*---* その他（エラー）*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0031:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(31,CHANNEL,ccd->mng.mode,0,MODE);

			fc_ccd_fsm_clr(ccd);			/* 状態遷移情報クリア（フェール）	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_tout_csh									*
*																			*
*		DESCRIPTION	: 状態遷移処理「ｔｃｓｈ　Ｔ．Ｏ．」					*
*																			*
*		PARAMETER	: IN  : ccd		: インスタンス							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_tout_csh(ST_CCD_MNG_INFO *ccd)
{
	/*--------------------------------------------------*/
	/*	状態毎の対応実施								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_S2_CSHW:	/*---* 送信：ＣＳ　Ｈ待ち *-----------------*/
///仕様変更
			/*--------------------------------------------------*/
			/*	送信完了イベント発行（WRITE待ち解除）			*/
			/*--------------------------------------------------*/
			if(ccd->sys.event.tx_flg == 0){
				if(ccd->comm.tx_buf.sts == M_CCD_ISTS_READY) {
					ccd->comm.tx_buf.sts = M_CCD_ISTS_CMPLT;
												/* 送信完了設定						*/
					ccd->sys.event.tx_flg = 1;
					LOGT("[%s]%s(%d) Wake Up Tx",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
					wake_up_interruptible(&ccd->sys.event.tx_evq);
												/* イベント発行						*/
												/* アイドル状態へ遷移				*/
				}
			}
			/*--------------------------------------------------*/
			/*	ＣＳ＝Ｈ出力（インアクティブ）					*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);

			/*--------------------------------------------------*/
			/*	ｔｉｄｌｅタイマ起動							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TIDLE);

			/*--------------------------------------------------*/
			/*	ＣＳＨ保持期間へ遷移							*/
			/*--------------------------------------------------*/
///仕様変更				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S3_CSHK);
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C2_KPID);
			break;

		case M_CCD_MODE_C0_INIT:	/*---* 共通：初期化待ち（初期値）*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* 共通：アイドル *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* 共通：アイドル保持期間 *-------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* 送信：送信待ち *---------------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* 送信：送信完了待ち *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* 送信：送信後　ＣＳＨ保持期間 *-------*/
		case M_CCD_MODE_S4_ITHW:	/*---* 送信：受信移行中　ＩＮＴＨ待ち *-----*/
		case M_CCD_MODE_S5_ITLW:	/*---* 送信：受信移行中　ＩＮＴＬ待ち *-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* 受信：ＩＮＴ　Ｈ待ち *---------------*/
		case M_CCD_MODE_R1_RCVC:	/*---* 受信：受信完了待ち *-----------------*/
		case M_CCD_MODE_E0_INTE:	/*---* 異常：ＩＮＴ異常 *-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			/* 何もしない */
			break;

		default:					/*---* その他（エラー）*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0032:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(32,CHANNEL,ccd->mng.mode,0,MODE);

			fc_ccd_fsm_clr(ccd);			/* 状態遷移情報クリア（フェール）	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_tout_idle									*
*																			*
*		DESCRIPTION	: 状態遷移処理「ｔｉｄｌｅ　Ｔ．Ｏ．」					*
*																			*
*		PARAMETER	: IN  : ccd		: インスタンス							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_tout_idle(ST_CCD_MNG_INFO *ccd)
{
	/*--------------------------------------------------*/
	/*	状態毎の対応実施								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_C2_KPID:	/*---* 共通：アイドル保持期間 *-------------*/
			/*--------------------------------------------------*/
			/*	送信要求がある場合は送信起動					*/
			/*--------------------------------------------------*/
			if((ccd->comm.tx_buf.sts == M_CCD_ISTS_READY)
			 || (ccd->comm.tx_buf.sts == M_CCD_ISTS_STOP)) {
				ccd->comm.tx_buf.sts =M_CCD_ISTS_READY;			/* ステータスを準備中に戻す			*/
				/*--------------------------------------------------*/
				/*	ＣＳ＝Ｌ出力（アクティブ）						*/
				/*--------------------------------------------------*/
				fc_ccd_set_sig_cs(ccd, ACTIVE);

				/*--------------------------------------------------*/
				/*	ｔｃｌｋ１タイマ起動							*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_TCLK1);

				/*--------------------------------------------------*/
				/*	送信待ちへ遷移									*/
				/*--------------------------------------------------*/
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S0_SNDW);
			}
			/*--------------------------------------------------*/
			/*	送信要求ない場合はアイドル状態へ遷移			*/
			/*--------------------------------------------------*/
			else {
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C1_IDLE);
			}
			break;

		case M_CCD_MODE_S3_CSHK:	/*---* 送信：送信後　ＣＳＨ保持期間 *-------*/
///仕様変更
///			/*--------------------------------------------------*/
///			/*	送信完了イベント発行（WRITE待ち解除）			*/
///			/*--------------------------------------------------*/
///			if(ccd->comm.tx_buf.sts == M_CCD_ISTS_READY) {
///				ccd->comm.tx_buf.sts = M_CCD_ISTS_CMPLT;
///											/* 送信完了設定						*/
///				ccd->sys.event.tx_flg = 1;
///				LOGT("[%s]%s(%d) Wake Up Tx",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
///				wake_up_interruptible(&ccd->sys.event.tx_evq);
///											/* イベント発行						*/
///											/* アイドル状態へ遷移				*/
///			}
			/*--------------------------------------------------*/
			/*	送信完了できていない場合は送信起動（リトライ）	*/
			/*--------------------------------------------------*/
			if(ccd->comm.tx_buf.sts == M_CCD_ISTS_STOP) {
				ccd->comm.tx_buf.sts =M_CCD_ISTS_READY;			/* ステータスを準備中に戻す				*/
				/*--------------------------------------------------*/
				/*	ＣＳ＝Ｌ出力（アクティブ）						*/
				/*--------------------------------------------------*/
				fc_ccd_set_sig_cs(ccd, ACTIVE);

				/*--------------------------------------------------*/
				/*	ｔｃｌｋ１タイマ起動							*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_TCLK1);

				/*--------------------------------------------------*/
				/*	送信待ちへ遷移									*/
				/*--------------------------------------------------*/
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S0_SNDW);
			}
			/*--------------------------------------------------*/
			/*	送信要求ない場合はアイドル状態へ遷移			*/
			/*--------------------------------------------------*/
			else {
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C1_IDLE);
			}
			break;

		case M_CCD_MODE_C0_INIT:	/*---* 共通：初期化待ち（初期値）*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* 共通：アイドル *---------------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* 送信：送信待ち *---------------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* 送信：送信完了待ち *-----------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* 送信：ＣＳ　Ｈ待ち *-----------------*/
		case M_CCD_MODE_S4_ITHW:	/*---* 送信：受信移行中　ＩＮＴＨ待ち *-----*/
		case M_CCD_MODE_S5_ITLW:	/*---* 送信：受信移行中　ＩＮＴＬ待ち *-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* 受信：ＩＮＴ　Ｈ待ち *---------------*/
		case M_CCD_MODE_R1_RCVC:	/*---* 受信：受信完了待ち *-----------------*/
		case M_CCD_MODE_E0_INTE:	/*---* 異常：ＩＮＴ異常 *-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			/* 何もしない */
			break;

		default:					/*---* その他（エラー）*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0033:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(33,CHANNEL,ccd->mng.mode,0,MODE);

			fc_ccd_fsm_clr(ccd);			/* 状態遷移情報クリア（フェール）	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_tout_fail1									*
*																			*
*		DESCRIPTION	: 状態遷移処理「ｔｆａｉｌ1 Ｔ．Ｏ．」					*
*																			*
*		PARAMETER	: IN  : ccd		: インスタンス							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_tout_fail1(ST_CCD_MNG_INFO *ccd)
{
	/*--------------------------------------------------*/
	/*	状態毎の対応実施								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_S5_ITLW:	/*---* 送信：受信移行中　ＩＮＴＬ待ち *-----*/
			/*--------------------------------------------------*/
			/*	送信要求がある場合は送信起動					*/
			/*--------------------------------------------------*/
			if((ccd->comm.tx_buf.sts == M_CCD_ISTS_READY)
			 || (ccd->comm.tx_buf.sts == M_CCD_ISTS_STOP)) {
				ccd->comm.tx_buf.sts =M_CCD_ISTS_READY;			/* ステータスを準備中に戻す			*/
				/*--------------------------------------------------*/
				/*	ＣＳ＝Ｌ出力（アクティブ）						*/
				/*--------------------------------------------------*/
				fc_ccd_set_sig_cs(ccd, ACTIVE);

				/*--------------------------------------------------*/
				/*	ｔｃｌｋ１タイマ起動							*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_TCLK1);

				/*--------------------------------------------------*/
				/*	送信待ちへ遷移									*/
				/*--------------------------------------------------*/
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S0_SNDW);
			}
			/*--------------------------------------------------*/
			/*	送信要求ない場合はアイドル状態へ遷移			*/
			/*--------------------------------------------------*/
			else {
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C1_IDLE);
			}
			break;

		case M_CCD_MODE_C0_INIT:	/*---* 共通：初期化待ち（初期値）*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* 共通：アイドル *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* 共通：アイドル保持期間 *-------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* 送信：送信待ち *---------------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* 送信：送信完了待ち *-----------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* 送信：ＣＳ　Ｈ待ち *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* 送信：送信後　ＣＳＨ保持期間 *-------*/
		case M_CCD_MODE_S4_ITHW:	/*---* 送信：受信移行中　ＩＮＴＨ待ち *-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* 受信：ＩＮＴ　Ｈ待ち *---------------*/
		case M_CCD_MODE_R1_RCVC:	/*---* 受信：受信完了待ち *-----------------*/
		case M_CCD_MODE_E0_INTE:	/*---* 異常：ＩＮＴ異常 *-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			break;

		default:					/*---* その他（エラー）*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0034:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(34,CHANNEL,ccd->mng.mode,0,MODE);

			fc_ccd_fsm_clr(ccd);			/* 状態遷移情報クリア（フェール）	*/
			break;
	}
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_tout_intfale								*
*																			*
*		DESCRIPTION	: 状態遷移処理「INT FALESAFE タイマＴ．Ｏ．」			*
*																			*
*		PARAMETER	: IN  : ccd		: インスタンス							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_tout_intfale(ST_CCD_MNG_INFO *ccd)
{
	BYTE sig = fc_ccd_get_sig_int(ccd);	/* INT 状態取得 */
	/*--------------------------------------------------*/
	/*	状態毎の対応実施								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_R0_ITHW:	/*---* 受信：ＩＮＴ　Ｈ待ち *---------------*/
			if(sig == NEGATIVE){
				/*--------------------------------------------------*/
				/*	前状態毎の対応実施								*/
				/*--------------------------------------------------*/
				switch(ccd->mng.previous_mode){
				case M_CCD_MODE_C1_IDLE:	/*---* 共通：アイドル *---------------------*/
				case M_CCD_MODE_C2_KPID:	/*---* 共通：アイドル保持期間 *-------------*/
				case M_CCD_MODE_S0_SNDW:	/*---* 送信：送信待ち *---------------------*/
				case M_CCD_MODE_S5_ITLW:	/*---* 送信：受信移行中　ＩＮＴＬ待ち *-----*/
					/*--------------------------------------------------*/
					/*	INT状態管理更新									*/
					/*--------------------------------------------------*/
					ccd->cpu.irq.sreq_sts = sig;
					ccd->cpu.irq.sreq_selfsts= sig;
					LOGD("[%s]ERR1000:mode=%d sreq_selfsts=%d",ccd->dev.spi.id_name, ccd->mng.mode,ccd->cpu.irq.sreq_selfsts);
					fc_ccd_drc_error(1000,CHANNEL,ccd->mng.mode,0,MODE);
					/*--------------------------------------------------*/
					/*	受信完了待ちへ遷移								*/
					/*--------------------------------------------------*/
					fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_R1_RCVC);
					/*--------------------------------------------------*/
					/*	受信起動：ワーカースレッド起動					*/
					/*--------------------------------------------------*/
					queue_work(ccd->mng.wkr_thrd,
								(struct work_struct*)&ccd->mng.wkr_inf[M_CCD_WINF_RCV]);
					break;
				case M_CCD_MODE_S1_SNDC:	/*---* 送信：送信完了待ち *-----------------*/
				case M_CCD_MODE_S2_CSHW:	/*---* 送信：ＣＳ　Ｈ待ち *-----------------*/
				case M_CCD_MODE_S3_CSHK:	/*---* 送信：送信後　ＣＳＨ保持期間 *-------*/
				case M_CCD_MODE_R1_RCVC:	/*---* 受信：受信完了待ち *-----------------*/
					/*--------------------------------------------------*/
					/*	INT状態管理更新									*/
					/*--------------------------------------------------*/
					ccd->cpu.irq.sreq_sts = sig;
					ccd->cpu.irq.sreq_selfsts= sig;
					LOGD("[%s]ERR1001:mode=%d sreq_selfsts=%d",ccd->dev.spi.id_name, ccd->mng.mode,ccd->cpu.irq.sreq_selfsts);
					fc_ccd_drc_error(1001,CHANNEL,ccd->mng.mode,0,MODE);
					/*--------------------------------------------------*/
					/*	INT#H処理実行									*/
					/*--------------------------------------------------*/
					fc_ccd_fsm_int_h(ccd);
					break;
				}
			}
			break;
		case M_CCD_MODE_S1_SNDC:	/*---* 送信：送信完了待ち *-----------------*/
			if(sig == ACTIVE){
				/*--------------------------------------------------*/
				/*	INT状態管理更新									*/
				/*--------------------------------------------------*/
				ccd->cpu.irq.sreq_sts = sig;
				ccd->cpu.irq.sreq_selfsts= sig;
				LOGD("[%s]ERR1002:mode=%d sreq_selfsts=%d",ccd->dev.spi.id_name, ccd->mng.mode,ccd->cpu.irq.sreq_selfsts);
				fc_ccd_drc_error(1002,CHANNEL,ccd->mng.mode,0,MODE);
				/*--------------------------------------------------*/
				/*	INT#L処理実行									*/
				/*--------------------------------------------------*/
				fc_ccd_fsm_int_l(ccd);
			}
			break;
		case M_CCD_MODE_S4_ITHW:	/*---* 送信：受信移行中　ＩＮＴＨ待ち *-----*/
			if(sig == NEGATIVE){
				/*--------------------------------------------------*/
				/*	INT状態管理更新									*/
				/*--------------------------------------------------*/
				ccd->cpu.irq.sreq_sts = sig;
				ccd->cpu.irq.sreq_selfsts= sig;
				LOGD("[%s]ERR1003:mode=%d sreq_selfsts=%d",ccd->dev.spi.id_name, ccd->mng.mode,ccd->cpu.irq.sreq_selfsts);
				fc_ccd_drc_error(1003,CHANNEL,ccd->mng.mode,0,MODE);
				/*--------------------------------------------------*/
				/*	INT#L処理実行									*/
				/*--------------------------------------------------*/
				fc_ccd_fsm_int_l(ccd);
			}
			break;
		case M_CCD_MODE_S5_ITLW:	/*---* 送信：受信移行中　ＩＮＴＬ待ち *-----*/
			if(sig == ACTIVE){
				/*--------------------------------------------------*/
				/*	前状態毎の対応実施								*/
				/*--------------------------------------------------*/
				switch(ccd->mng.previous_mode){
					case M_CCD_MODE_C2_KPID:	/*---* 共通：アイドル保持期間 H*-------------*/
					case M_CCD_MODE_S0_SNDW:	/*---* 送信：送信待ち H*---------------------*/
					case M_CCD_MODE_S1_SNDC:	/*---* 送信：送信完了待ち H*-----------------*/
					case M_CCD_MODE_S2_CSHW:	/*---* 送信：ＣＳ　Ｈ待ち H*-----------------*/
					case M_CCD_MODE_S3_CSHK:	/*---* 送信：送信後　ＣＳＨ保持期間 H*-------*/
					case M_CCD_MODE_R1_RCVC:	/*---* 受信：受信完了待ち H*-----------------*/
					/*--------------------------------------------------*/
					/*	INT状態管理更新									*/
					/*--------------------------------------------------*/
					ccd->cpu.irq.sreq_sts = sig;
					ccd->cpu.irq.sreq_selfsts= sig;
					LOGD("[%s]ERR1004:mode=%d sreq_selfsts=%d",ccd->dev.spi.id_name, ccd->mng.mode,ccd->cpu.irq.sreq_selfsts);
					fc_ccd_drc_error(1004,CHANNEL,ccd->mng.mode,0,MODE);
					/*--------------------------------------------------*/
					/*	INT#L処理実行									*/
					/*--------------------------------------------------*/
					fc_ccd_fsm_int_l(ccd);
					break;
				}
			}
			break;
		case M_CCD_MODE_C0_INIT:	/*---* 共通：初期化待ち（初期値）*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* 共通：アイドル *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* 共通：アイドル保持期間 *-------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* 送信：ＣＳ　Ｈ待ち *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* 送信：送信後　ＣＳＨ保持期間 *-------*/
		case M_CCD_MODE_R1_RCVC:	/*---* 受信：受信完了待ち *-----------------*/
		case M_CCD_MODE_E0_INTE:	/*---* 異常：ＩＮＴ異常 *-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			break;

		default:					/*---* その他（エラー）*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0034:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(34,CHANNEL,ccd->mng.mode,0,MODE);

			fc_ccd_fsm_clr(ccd);			/* 状態遷移情報クリア（フェール）	*/
			break;
	}
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_tout_comn									*
*																			*
*		DESCRIPTION	: 状態遷移処理「汎用タイマ　Ｔ．Ｏ．」					*
*																			*
*		PARAMETER	: IN  : ccd		: インスタンス							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_tout_comn(ST_CCD_MNG_INFO *ccd)
{
	BYTE	sig;
	INT		sus;

	/*--------------------------------------------------*/
	/*  サスペンド状態ポーリング確認					*/
	/*--------------------------------------------------*/
	sus  = early_device_invoke_with_flag_irqsave(RNF_BUDET_ACCOFF_INTERRUPT_MASK,
													fc_ccd_fb_chk_sus, NULL);
	if(sus == TRUE){
		LOGT("[%s]%s(%d) bf_ccd_shut_down_flg ON !!!",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
		bf_ccd_shut_down_flg = ON;
	}

	/*--------------------------------------------------*/
	/*	シャットダウン時はＲＷ待ち開放					*/
	/*--------------------------------------------------*/
	if(bf_ccd_shut_down_flg == ON) {
		fc_ccd_drc_event(CCD_EVENT_ID_FLGCHECK,CCD_EVENT_FORMATID_IF,bf_ccd_shut_down_flg,ccd->dev.spi.id_name[0]);
		
		if(ccd->sys.event.tx_flg==0){
			if((ccd->comm.tx_buf.sts == M_CCD_ISTS_READY)
			|| (ccd->comm.tx_buf.sts == M_CCD_ISTS_STOP )) {
				LOGT("[%s]%s(%d) Wake Up Tx",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
				fc_ccd_drc_event(CCD_EVENT_ID_WAKEUP_TX,CCD_EVENT_FORMATID_WAKEUP,-1,ccd->dev.spi.id_name[0]);

				ccd->comm.tx_buf.sts	= M_CCD_ISTS_ERR;
				ccd->sys.event.tx_flg	= 1;
				wake_up_interruptible(&ccd->sys.event.tx_evq);
			}
		}
		if(ccd->sys.event.rx_flg == 0){
			if(ccd->comm.rx_buf.sts == M_CCD_ISTS_READY) {
				LOGT("[%s]%s(%d) Wake Up Rx",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
				fc_ccd_drc_event(CCD_EVENT_ID_WAKEUP_RX,CCD_EVENT_FORMATID_WAKEUP,-1,ccd->dev.spi.id_name[0]);
				ccd->comm.rx_buf.sts	= M_CCD_ISTS_ERR;
				ccd->sys.event.rx_flg	= 1;
				wake_up_interruptible(&ccd->sys.event.rx_evq);
			}
		}

		/*--------------------------------------------------*/
		/*	初期化待ちへ遷移								*/
		/*--------------------------------------------------*/
		fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C0_INIT);
		return;								/* 処理中断							*/
	}

	/*--------------------------------------------------*/
	/*	状態固着カウンタ　カウントアップ				*/
	/*--------------------------------------------------*/
	if(ccd->tim.mchk < M_CCD_COMTIM_MAX) {
		ccd->tim.mchk++;
	}

	/*--------------------------------------------------*/
	/*	ＩＮＴ信号状態取得								*/
	/*--------------------------------------------------*/
	sig = fc_ccd_get_sig_int(ccd);

	/*--------------------------------------------------*/
	/*	状態固着確認（ＩＮＴネガティブ時のみ）			*/
	/*--------------------------------------------------*/
	if(sig == NEGATIVE) {
		/*--------------------------------------------------*/
		/*	固着発生していたら状態クリア（アイドル）する	*/
		/*--------------------------------------------------*/
		if((ccd->tim.mchk > M_CCD_COMTIM_300MS)
		 && (ccd->mng.mode != M_CCD_MODE_C0_INIT)
		 && (ccd->mng.mode != M_CCD_MODE_C1_IDLE)
		 && (ccd->mng.mode != M_CCD_MODE_E0_INTE)) {
			/* ERROR TRACE */
			LOGE("[%s]ERR0035:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(35,CHANNEL,ccd->mng.mode,0,MODE);

			fc_ccd_fsm_clr(ccd);			/* 状態遷移情報クリア				*/
			return;							/* 処理中断							*/
		}
	}

	/*--------------------------------------------------*/
	/*	ＩＮＴがＬのときのみカウントアップ				*/
	/*--------------------------------------------------*/
	if(sig == ACTIVE) {
		if(ccd->tim.tcnt < M_CCD_COMTIM_MAX) {
			ccd->tim.tcnt++;
		}
	}
	else {
		ccd->tim.tcnt = 0;
	}

	/*--------------------------------------------------*/
	/*	状態毎の対応実施								*/
	/*--------------------------------------------------*/
	///LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_C0_INIT:	/*---* 共通：初期化待ち（初期値）*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* 共通：アイドル *---------------------*/
			/*--------------------------------------------------*/
			/*	ＣＳ信号をＨにする（フェールセーフ）			*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);
			break;

		case M_CCD_MODE_R0_ITHW:	/*---* 受信：ＩＮＴ　Ｈ待ち *---------------*/
		case M_CCD_MODE_S4_ITHW:	/*---* 送信：受信移行中　ＩＮＴＨ待ち *-----*/
			/*--------------------------------------------------*/
			/*	１秒(暫定値)を超えていたら送信は捨て始める		*/
			/*--------------------------------------------------*/
			if(ccd->tim.tcnt > M_CCD_COMTIM_1SEC) {
				LOGW("[%s]%s(%d) 1SEC Over",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
				if(ccd->sys.event.tx_flg == 0){
					if((ccd->comm.tx_buf.sts == M_CCD_ISTS_READY)
					 || (ccd->comm.tx_buf.sts == M_CCD_ISTS_STOP)) {
						LOGT("[%s]%s(%d) Wake Up Tx",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
						fc_ccd_drc_event(CCD_EVENT_ID_WAKEUP_TX,CCD_EVENT_FORMATID_WAKEUP,-2,ccd->dev.spi.id_name[0]);
						ccd->comm.tx_buf.sts	= M_CCD_ISTS_ERR;
						ccd->sys.event.tx_flg	= 1;
						wake_up_interruptible(&ccd->sys.event.tx_evq);
					}
				}
			}
			/*--------------------------------------------------*/
			/*	TOUT経過しても状態固着の場合はエラー状態へ遷移	*/
			/*--------------------------------------------------*/
			if(ccd->tim.tcnt == M_CCD_COMTIM_5SEC) {
				LOGW("[%s]%s(%d) 5SEC Over",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
				/*--------------------------------------------------*/
				/*	ＣＳ信号をＨにする								*/
				/*--------------------------------------------------*/
				fc_ccd_set_sig_cs(ccd, NEGATIVE);

				/*--------------------------------------------------*/
				/*	エラー状態へ遷移								*/
				/*--------------------------------------------------*/
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_E0_INTE);
			}
			break;

		case M_CCD_MODE_E0_INTE:	/*---* 異常：ＩＮＴ異常 *-------------------*/
			/*--------------------------------------------------*/
			/*	送信要求は捨てる								*/
			/*--------------------------------------------------*/
			if(ccd->sys.event.tx_flg == 0){
				if((ccd->comm.tx_buf.sts == M_CCD_ISTS_READY)
				 || (ccd->comm.tx_buf.sts == M_CCD_ISTS_STOP)) {
					LOGW("[%s]%s(%d) --E0_INTE-- Wake Up Tx",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
					fc_ccd_drc_event(CCD_EVENT_ID_WAKEUP_TX,CCD_EVENT_FORMATID_WAKEUP,-3,ccd->dev.spi.id_name[0]);
					ccd->comm.tx_buf.sts	= M_CCD_ISTS_ERR;
					ccd->sys.event.tx_flg	= 1;
					wake_up_interruptible(&ccd->sys.event.tx_evq);
				}
			}

			/*--------------------------------------------------*/
			/*	ＣＳ信号をＨにする（フェールセーフ）			*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);

			/*--------------------------------------------------*/
			/*	１分待って回復しない場合は記録（仕様）			*/
			/*--------------------------------------------------*/
			if(ccd->tim.tcnt == M_CCD_COMTIM_1MIN) {
				/* ERROR TRACE */
				LOGE("[%s]ERR0036:Cpu Communication Driver Error, Logged by Specification : 1min_Timeout",ccd->dev.spi.id_name);
				fc_ccd_drc_error(36,CHANNEL,0,0,MODE);
			}
			break;

		case M_CCD_MODE_C2_KPID:	/*---* 共通：アイドル保持期間 *-------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* 送信：送信待ち *---------------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* 送信：送信完了待ち *-----------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* 送信：ＣＳ　Ｈ待ち *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* 送信：送信後　ＣＳＨ保持期間 *-------*/
		case M_CCD_MODE_S5_ITLW:	/*---* 送信：受信移行中　ＩＮＴＬ待ち *-----*/
		case M_CCD_MODE_R1_RCVC:	/*---* 受信：受信完了待ち *-----------------*/
//			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			break;

		default:					/*---* その他（エラー）*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0037:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(37,CHANNEL,ccd->mng.mode,0,MODE);

			fc_ccd_fsm_clr(ccd);			/* 状態遷移情報クリア（フェール）	*/
			break;
	}

	/*--------------------------------------------------*/
	/*	タイマ再起動									*/
	/*--------------------------------------------------*/
	fc_ccd_tim_start(ccd, M_CCD_TIM_COMON);

}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_timout										*
*																			*
*		DESCRIPTION	: 状態遷移タイムアウト処理								*
*																			*
*		PARAMETER	: IN  : *ccd : コンテキスト								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_timout(ST_CCD_MNG_INFO *ccd, INT tim_num)
{
	/*--------------------------------------------------*/
	/*	タイムアウト種別毎に状態遷移処理実施			*/
	/*--------------------------------------------------*/
	switch(tim_num) {
		case	M_CCD_TIM_TCLK1:	/*---* ｔｃｌｋ１*--------------------------*/
			fc_ccd_fsm_tout_clk1(ccd);
			break;

		case	M_CCD_TIM_TCSH:		/*---* ｔｃｓｈ*----------------------------*/
			fc_ccd_fsm_tout_csh(ccd);
			break;

		case	M_CCD_TIM_TIDLE:	/*---* ｔｉｄｌｅ*--------------------------*/
			fc_ccd_fsm_tout_idle(ccd);
			break;

		case	M_CCD_TIM_TFAIL1:	/*---* ｔｆａｉｌ１*------------------------*/
			fc_ccd_fsm_tout_fail1(ccd);
			break;

		case	M_CCD_TIM_COMON:	/*---* 汎用タイマ *-------------------------*/
			fc_ccd_fsm_tout_comn(ccd);
			break;

		case	M_CCD_TIM_INTFAIL:	/*---*INT FALESAFEタイマ *------------------*/
			fc_ccd_fsm_tout_intfale(ccd);
			break;

		default:					/*---* その他（エラー）	*-------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0038:tim_num=%d",ccd->dev.spi.id_name, tim_num);
			fc_ccd_drc_error(38,CHANNEL,tim_num,0,MODE);
			break;
	}

}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_init_instance									*
*																			*
*		DESCRIPTION	: インスタンス初期化処理								*
*																			*
*		PARAMETER	: IN  : *ccd : コンテキスト								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
INT	fc_ccd_init_instance(ST_CCD_MNG_INFO *ccd)
{
	BYTE	i;

	/*--------------------------------------------------*/
	/*  受付拒否										*/
	/*--------------------------------------------------*/
	ccd->bf_ccd_acpt_flg = FALSE;

	/*--------------------------------------------------*/
	/*  ポート設定										*/
	/*--------------------------------------------------*/
	if(ccd->dev.spi.sdev->master->bus_num == 0){
											/* SPI1 = CPU_COM1 ：大容量通信*/
		ccd->cpu.pin.MCLK = CPU_COM_MCLK1;
		ccd->cpu.pin.MREQ = CPU_COM_MREQ1;
		ccd->cpu.pin.SREQ = CPU_COM_SREQ1;
		ccd->cpu.pin.SDAT = CPU_COM_SDT1;
		ccd->cpu.pin.MDAT = CPU_COM_MDT1;
	}
	else
/*	if(ccd->dev.spi.sdev->master->bus_num == 1) */ {
											 /* SPI2 = CPU_COM0 : コマンド通信*/
		ccd->cpu.pin.MCLK = CPU_COM_MCLK0;
		ccd->cpu.pin.MREQ = CPU_COM_MREQ0;
		ccd->cpu.pin.SREQ = CPU_COM_SREQ0;
		ccd->cpu.pin.SDAT = CPU_COM_SDT0;
		ccd->cpu.pin.MDAT = CPU_COM_MDT0;
	}

	/*--------------------------------------------------*/
	/*  wait que初期化									*/
	/*--------------------------------------------------*/
	ccd->sys.event.tx_flg = 1;
	ccd->sys.event.rx_flg = 1;
	init_waitqueue_head(&ccd->sys.event.tx_evq);
	init_waitqueue_head(&ccd->sys.event.rx_evq);

	/*--------------------------------------------------*/
	/*  Mutex que初期化									*/
	/*--------------------------------------------------*/
	mutex_init(&ccd->sys.mutex.instnc);
	mutex_init(&ccd->sys.mutex.wt_if);
	mutex_init(&ccd->sys.mutex.rd_if);

	/*--------------------------------------------------*/
	/*  ワーカースレッド生成							*/
	/*--------------------------------------------------*/
	ccd->mng.wkr_thrd = create_singlethread_workqueue("spi-tegra-cpucom-work");
	if(ccd->mng.wkr_thrd == 0) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0039:ccd->mng.wkr_thrd=0",ccd->dev.spi.id_name);
		fc_ccd_drc_error(39,CHANNEL,0,0,MODE);
		return	-ENOMEM;
	}

	/*--------------------------------------------------*/
	/*  ワーカスレッドＩＦ用イベント情報生成			*/
	/*--------------------------------------------------*/
	for(i=0; i<M_CCD_WINF_CNT; i++){
		memset((void*)&ccd->mng.wkr_inf[i], 0, sizeof(ccd->mng.wkr_inf[i]));
		INIT_WORK((struct work_struct*)&ccd->mng.wkr_inf[i], fc_ccd_wthrd_main);
		ccd->mng.wkr_inf[i].eve_num = i;
		ccd->mng.wkr_inf[i].inst_p	= ccd;
	}

	/*--------------------------------------------------*/
	/*  SPI-TEGRA設定									*/
	/*--------------------------------------------------*/
	ccd->dev.spi.cnfg.is_hw_based_cs = 1;
	ccd->dev.spi.cnfg.cs_setup_clk_count =0;
	ccd->dev.spi.cnfg.cs_hold_clk_count  =0;

	ccd->dev.spi.sdev->controller_data = (void*)&ccd->dev.spi.cnfg;
	ccd->dev.spi.sdev->bits_per_word=8;

	spi_setup(ccd->dev.spi.sdev);

	/*--------------------------------------------------*/
	/*  タイマー初期化									*/
	/*--------------------------------------------------*/
	fc_ccd_tim_init(ccd);

	/*--------------------------------------------------*/
	/*	割り込み制御用排他初期化						*/
	/*--------------------------------------------------*/
	spin_lock_init(&ccd->cpu.irq.irq_ctl);

	return	ESUCCESS;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_make_gpio_str									*
*																			*
*		DESCRIPTION	: ＧＰＩＯ登録文字列生成								*
*																			*
*		PARAMETER	: IN  : *ccd : コンテキスト								*
*					      : *str : ユニーク文字列							*
*																			*
*					  OUT : *out : 連結データ格納先							*
*																			*
*					  RET : ACTIVE	/ NEGATIVE								*
*																			*
****************************************************************************/
inline	CHAR*	fc_ccd_make_gpio_str(CHAR *idn, CHAR *out, CHAR *str)
{
	sprintf(out,"%s%s%s", M_CCD_DRV_NAME, idn, str);

	return out;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_calc_fcc										*
*																			*
*		DESCRIPTION	: ＦＣＣ計算処理										*
*																			*
*		PARAMETER	: IN  : buf  : 計算データ								*
*						  : len  : データ長									*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : FCC値											*
*																			*
****************************************************************************/
inline	ULONG	fc_ccd_calc_fcc(BYTE *buf, ULONG len)
{
	ULONG	result;
	ULONG	i;

	result = 0;
	for(i=0; i<len; i++) {
		result += buf[i];
	}

	return result;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_snd_dat										*
*																			*
*		DESCRIPTION	: データ送信処理										*
*																			*
*		PARAMETER	: IN  : *ccd	: コンテキスト							*
*						  : sdat	: 送信データ							*
*						  : len		: データ長								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : 成功／失敗										*
*																			*
*		CAUTION		: インスタンス排他されていないので注意！！				*
*					  インスタンスデータ参照以外禁止！						*
*																			*
****************************************************************************/
SHORT	fc_ccd_snd_dat(ST_CCD_MNG_INFO *ccd, BYTE *sdat, USHORT len)
{
	USHORT	i, j;
	USHORT	cnt, mod;
	SHORT	ret;
	BYTE	sig;

	/*--------------------------------------------------*/
	/*	４バイトずつ送信するので回数計算				*/
	/*--------------------------------------------------*/
	cnt = len / M_CCD_COMM_PACKET;		/* 送信起動回数						*/
	mod = len % M_CCD_COMM_PACKET;		/* あまりの送信データ数				*/

	/*--------------------------------------------------*/
	/*	４バイトずつ送信								*/
	/*--------------------------------------------------*/
	ret = M_CCD_CSTS_OK;				/* デフォルト＝ＯＫ					*/
	for(i=0; i<cnt; i++) {

#if	__INTERFACE__ == IF_SPI_TEGRA		/* SPI-TEGRAでの通信 				*/
		ret = fc_ccd_snd_spi_res(ccd, &sdat[i*4], 4);
#else /* INTERFACE == IF_GPIO */		/* GPIO制御での通信 				*/
		for(j=0; j<4; j++) {
			fc_ccd_snd_byte(ccd, sdat[i*4+j]);
		}
#endif

		/*--------------------------------------------------*/
		/*	中断確認										*/
		/*--------------------------------------------------*/
		sig = fc_ccd_get_sig_int(ccd);
		if(sig == ACTIVE) {
			return	M_CCD_ISTS_STOP;
		}
		if(bf_ccd_shut_down_flg == ON) {
			LOGT("[%s]%s(%d) bf_ccd_shut_down_flg ON !!!",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
			return	M_CCD_ISTS_SUSPEND;
		}
		if(ret != M_CCD_CSTS_OK) {
			LOGT("[%s]%s(%d) ERROR",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
			return	M_CCD_ISTS_ERR;
		}
	}

	/*--------------------------------------------------*/
	/*	残りのデータを送信								*/
	/*--------------------------------------------------*/
#if	__INTERFACE__ == IF_SPI_TEGRA			/* SPI-TEGRAでの通信 */
	ret = fc_ccd_snd_spi_res(ccd, &sdat[i*4], mod);
#else /* INTERFACE == IF_GPIO */			/* GPIO制御での通信 */
	for(j=0; j<mod; j++) {
		fc_ccd_snd_byte(ccd, sdat[i*4+j]);
	}
#endif

	/*--------------------------------------------------*/
	/*	中断確認										*/
	/*--------------------------------------------------*/
	sig = fc_ccd_get_sig_int(ccd);
	if(sig == ACTIVE) {
		LOGT("[%s]%s(%d) INT#L",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
		return	M_CCD_ISTS_STOP;
	}
	if(bf_ccd_shut_down_flg == ON) {
		LOGT("[%s]%s(%d) bf_ccd_shut_down_flg ON !!!",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
		return	M_CCD_ISTS_SUSPEND;
	}
	if(ret != M_CCD_CSTS_OK) {
		LOGT("[%s]%s(%d) ERROR",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
		return	M_CCD_ISTS_ERR;
	}

	return M_CCD_ISTS_CMPLT;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_rcv_dat										*
*																			*
*		DESCRIPTION	: データ受信処理										*
*																			*
*		PARAMETER	: IN  : *ccd	: コンテキスト							*
						  : len		: データ長								*
*																			*
*					  OUT : rdat	: 受信データ							*
*																			*
*					  RET : 成功／失敗										*
*																			*
*		CAUTION		: インスタンス排他されていないので注意！！				*
*					  受信バッファ格納以外インスタンスデータ変更禁止！		*
*																			*
****************************************************************************/
SHORT	fc_ccd_rcv_dat(ST_CCD_MNG_INFO *ccd, BYTE *rdat, USHORT len)
{
	USHORT	i, j;
	USHORT	cnt, mod;
	SHORT	ret;
	BYTE	sig;

	/*--------------------------------------------------*/
	/*	設定バイトずつ受信するので回数計算				*/
	/*--------------------------------------------------*/
	cnt = len / M_CCD_COMM_PACKET;			/* 送信起動回数						*/
	mod = len % M_CCD_COMM_PACKET;			/* あまりの受信データ数				*/

	/*--------------------------------------------------*/
	/*	設定バイトずつ送信								*/
	/*--------------------------------------------------*/
	ret = M_CCD_CSTS_OK;					/* デフォルト＝ＯＫ					*/
	for(i=0; i<cnt; i++) {
		BYTE sig;
#if	__INTERFACE__ == IF_SPI_TEGRA		/* SPI-TEGRAでの通信 */
		ret = fc_ccd_rcv_spi_res(ccd, &rdat[i*M_CCD_COMM_PACKET], M_CCD_COMM_PACKET);
#else /* __INTERFACE__ == IF_GPIO */	/* GPIO制御での通信 */
		for(j=0; j<M_CCD_COMM_PACKET; j++) {
			rdat[i*M_CCD_COMM_PACKET+j] = fc_ccd_rcv_byte(ccd);
		}
#endif

		/*--------------------------------------------------*/
		/*	中断確認										*/
		/*--------------------------------------------------*/
		sig = fc_ccd_get_sig_int(ccd);
		if(sig == ACTIVE) {
			LOGT("[%s]%s(%d) INT#L",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
			return	M_CCD_ISTS_STOP;
		}
		if(bf_ccd_shut_down_flg == ON) {
			LOGT("[%s]%s(%d) bf_ccd_shut_down_flg ON !!!",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
			return	M_CCD_ISTS_SUSPEND;
		}
		if(ret != M_CCD_CSTS_OK) {
			LOGT("[%s]%s(%d) ERROR",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
			return	M_CCD_ISTS_ERR;
		}
	}

	/*--------------------------------------------------*/
	/*	残りのデータを送信								*/
	/*--------------------------------------------------*/
#if	__INTERFACE__ == IF_SPI_TEGRA			/* SPI-TEGRAでの通信 */
	ret = fc_ccd_rcv_spi_res(ccd, &rdat[i*M_CCD_COMM_PACKET], mod);
#else /* __INTERFACE__ == IF_GPIO */		/* GPIO制御での通信 */
	for(j=0; j<mod; j++) {
		rdat[i*M_CCD_COMM_PACKET+j] = fc_ccd_rcv_byte(ccd);
	}
#endif

	/*--------------------------------------------------*/
	/*	中断確認										*/
	/*--------------------------------------------------*/
	sig = fc_ccd_get_sig_int(ccd);
	if(sig == ACTIVE) {
		LOGT("[%s]%s(%d) INT#L",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
		return	M_CCD_ISTS_STOP;
	}
	if(bf_ccd_shut_down_flg == ON) {
		LOGT("[%s]%s(%d) bf_ccd_shut_down_flg ON !!!",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
		return	M_CCD_ISTS_SUSPEND;
	}
	if(ret != M_CCD_CSTS_OK) {
		LOGT("[%s]%s(%d) ERROR",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
		return	M_CCD_ISTS_ERR;
	}
	return M_CCD_ISTS_CMPLT;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_snd_byte										*
*																			*
*		DESCRIPTION	: １バイト送信処理（ポート制御）						*
*																			*
*		PARAMETER	: IN  : *ccd	: コンテキスト							*
*						  : dat		: 送信データ							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_snd_byte(ST_CCD_MNG_INFO *ccd, BYTE dat)
{
	INT		i;

	gpio_set_value(ccd->cpu.pin.MCLK, HIGH);

	for(i=0; i<8; i++){
		gpio_set_value(ccd->cpu.pin.MCLK, LOW);
		if((dat & (0x01<<i)) != 0) {
			gpio_set_value(ccd->cpu.pin.MDAT, HIGH);
		}
		else {
			gpio_set_value(ccd->cpu.pin.MDAT, LOW);
		}
		CLK_DELAY;
		gpio_set_value(ccd->cpu.pin.MCLK, HIGH);
		CLK_DELAY;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_rcv_byte										*
*																			*
*		DESCRIPTION	: １バイト受信処理（ポート制御）						*
*																			*
*		PARAMETER	: IN  : *ccd	: コンテキスト							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : dat		: 受信データ							*
*																			*
****************************************************************************/
BYTE	fc_ccd_rcv_byte(ST_CCD_MNG_INFO *ccd)
{
	INT		i;
	BYTE	dat;

	gpio_set_value(ccd->cpu.pin.MCLK, HIGH);

	dat = 0;
	for(i=0; i<8; i++){
		gpio_set_value(ccd->cpu.pin.MCLK, LOW);
		CLK_DELAY;
		gpio_set_value(ccd->cpu.pin.MCLK, HIGH);
		if(gpio_get_value(ccd->cpu.pin.SDAT) == HIGH) {
			dat |= (BYTE)(0x01 << i);
		}
		CLK_DELAY;
	}

	return	dat;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_snd_spi_res									*
*																			*
*		DESCRIPTION	: 送信処理（シリアルリソース制御）						*
*																			*
*		PARAMETER	: IN  : *ccd	: コンテキスト							*
*						  : sdat	: 送信データ							*
*						  : len		: データ長								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : 成功／失敗										*
*																			*
****************************************************************************/
#if	__INTERFACE__ == IF_SPI_TEGRA			/* SPI-TEGRAでの通信 */
SHORT	fc_ccd_snd_spi_res(ST_CCD_MNG_INFO *ccd, BYTE *sdat, USHORT len)
{
	struct spi_transfer		spi_set;
	struct spi_message		msg;
	ssize_t					ret = 0;
	/*--------------------------------------------------*/
	/*	ＳＰＩへの送信指示メッセージ作成				*/
	/*--------------------------------------------------*/
	memset(&spi_set, 0, sizeof(spi_set));
	spi_set.tx_buf	= sdat;
	spi_set.len		= len;

	spi_message_init(&msg);
	spi_message_add_tail(&spi_set, &msg);

	/*--------------------------------------------------*/
	/*	ＳＰＩ転送制御実行								*/
	/*--------------------------------------------------*/
	ret = fc_ccd_spicomm_ctrl(ccd, &msg);

	/*--------------------------------------------------*/
	/*	転送数が指示通りで正常終了						*/
	/*--------------------------------------------------*/
	if(ret != len) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0040:ret=%d len=%d",ccd->dev.spi.id_name, ret, len);
		fc_ccd_drc_error(40,CHANNEL,ret,len,MODE);

		return	M_CCD_CSTS_NG;
	}
	else {
		return	M_CCD_CSTS_OK;
	}
}

#endif
#if	__INTERFACE__ == IF_SPI_TEGRA			/* SPI-TEGRAでの通信 */
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_rcv_spi_res									*
*																			*
*		DESCRIPTION	: 受信処理（シリアルリソース制御）						*
*																			*
*		PARAMETER	: IN  : *ccd	: コンテキスト							*
						  : len		: データ長								*
*																			*
*					  OUT : rdat	: 受信データ							*
*																			*
*					  RET : 成功／失敗										*
*																			*
****************************************************************************/
SHORT	fc_ccd_rcv_spi_res(ST_CCD_MNG_INFO *ccd, BYTE *rdat, USHORT len)
{
	struct spi_transfer		spi_set;
	struct spi_message		msg;
	ssize_t					ret = 0;
	/*--------------------------------------------------*/
	/*	ＳＰＩへの受信指示メッセージ作成				*/
	/*--------------------------------------------------*/
	memset(&spi_set, 0, sizeof(spi_set));
	spi_set.rx_buf	= rdat;
	spi_set.len		= len;

	spi_message_init(&msg);
	spi_message_add_tail(&spi_set, &msg);

	/*--------------------------------------------------*/
	/*	ＳＰＩ転送制御実行								*/
	/*--------------------------------------------------*/
	ret = fc_ccd_spicomm_ctrl(ccd, &msg);

	/*--------------------------------------------------*/
	/*	転送数が指示通りで正常終了						*/
	/*--------------------------------------------------*/
	if(ret != len) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0041:ret=%d len=%d",ccd->dev.spi.id_name, ret, len);
		fc_ccd_drc_error(41,CHANNEL,ret,len,MODE);
		return	M_CCD_CSTS_NG;
	}
	else {
		return	M_CCD_CSTS_OK;
	}
}
#endif
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_spicomm_ctrl									*
*																			*
*		DESCRIPTION	: ＳＰＩ送受信制御処理									*
*																			*
*		PARAMETER	: IN  : *ccd	: コンテキスト							*
*						  : *msg	: ＳＰＩメッセージ						*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : サイズ / リターンコード							*
*																			*
****************************************************************************/
#if __INTERFACE__ == IF_SPI_TEGRA	/* SPI-TEGRAでの通信 */
ssize_t	fc_ccd_spicomm_ctrl(ST_CCD_MNG_INFO *ccd, struct spi_message *msg)
{
	DECLARE_COMPLETION_ONSTACK(done);
	INT		status;

	msg->complete	= fc_ccd_spicomm_cb;
	msg->context	= &done;

	spin_lock_irq(&ccd->dev.spi.lockt);

	/*--------------------------------------------------*/
	/*	ＳＰＩへ送受信指示メッセージ送信				*/
	/*--------------------------------------------------*/
	if(ccd->dev.spi.sdev == NULL) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0042:sdev=NULL",ccd->dev.spi.id_name);
		fc_ccd_drc_error(42,CHANNEL,0,0,MODE);
		status = -ESHUTDOWN;
	}else{
		status = spi_async(ccd->spi.sdev, msg);
											/* メッセージ送信					*/
	}

	spin_unlock_irq(&ccd->dev.spi.lockt);

	if(status != 0) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0043:sdev=NULL",ccd->dev.spi.id_name);
		fc_ccd_drc_error(43,CHANNEL,0,0,MODE);

		return status;						/* 異常終了							*/
	}

	/*--------------------------------------------------*/
	/*	ＳＰＩ転送完了待ち								*/
	/*--------------------------------------------------*/
	wait_for_completion_interruptible(&done);
	status = msg->status;

	/*--------------------------------------------------*/
	/*	正常終了時は転送バイト数を返り値とする			*/
	/*--------------------------------------------------*/
	if(status == 0) {
		status = msg->actual_length;
	}

	return	status;
}
#endif

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_spicomm_cb										*
*																			*
*		DESCRIPTION	: ＳＰＩ転送完了コールバック							*
*																			*
*		PARAMETER	: IN  : *arg 	: 未使用								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
#if __INTERFACE__ == IF_SPI_TEGRA	/* SPI-TEGRAでの通信 */
VOID	fc_ccd_spicomm_cb(void *arg)
{
	/*--------------------------------------------------*/
	/*	転送処理の待ちを起こす							*/
	/*--------------------------------------------------*/
	complete(arg);
}
#endif

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_tim_init										*
*																			*
*		DESCRIPTION	: タイマ初期化											*
*																			*
*		PARAMETER	: IN  : *ccd	: コンテキスト							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : リターンコード									*
*																			*
****************************************************************************/
VOID	fc_ccd_tim_init(ST_CCD_MNG_INFO *ccd)
{
	INT		i;

	/*--------------------------------------------------*/
	/*	各タイマ初期設定								*/
	/*--------------------------------------------------*/
	for(i=0; i<M_CCD_TIM_MAX; i++) {
		ccd->tim.tmr_inf[i].inst_p			= (void*)ccd;
		ccd->tim.tmr_inf[i].tim_num			= i;
		ccd->tim.tmr_inf[i].usec			= tb_ccd_timer_tim[i];

		hrtimer_init(&ccd->tim.tmr_inf[i].hndr, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

		/*--------------------------------------------------*/
		/*	コールバック登録はＩＮＩＴの後に設定する		*/
		/*--------------------------------------------------*/
		ccd->tim.tmr_inf[i].hndr.function	= fs_ccd_timeout_cb;
	}

	return;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_tim_start										*
*																			*
*		DESCRIPTION	: タイマースタート										*
*																			*
*		PARAMETER	: IN  : *ccd	: コンテキスト							*
*							timid	: タイマID								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_tim_start(ST_CCD_MNG_INFO *ccd, USHORT timid)
{
	ktime_t		time;
	LONG		sec;
	ULONG		nano;

	/*--------------------------------------------------*/
	/*	起動していたら一旦止める						*/
	/*--------------------------------------------------*/
	if(hrtimer_active(&ccd->tim.tmr_inf[timid].hndr)) {
		fc_ccd_tim_stop(ccd, timid);
	}

	/*--------------------------------------------------*/
	/*	タイマ時間設定									*/
	/*--------------------------------------------------*/
	sec  = M_CCD_MCR_US2SEC(ccd->tim.tmr_inf[timid].usec);
	nano = M_CCD_MCR_US2NS (ccd->tim.tmr_inf[timid].usec % M_CCD_MCR_SEC2US(1));
	time = ktime_set(sec, nano);

	/*--------------------------------------------------*/
	/*	タイマ起動										*/
	/*--------------------------------------------------*/
	hrtimer_start(&ccd->tim.tmr_inf[timid].hndr, time, HRTIMER_MODE_REL);
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_tim_stop										*
*																			*
*		DESCRIPTION	: タイマー停止											*
*																			*
*		PARAMETER	: IN  : *ccd	: コンテキスト							*
*							timid	: タイマID								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_tim_stop(ST_CCD_MNG_INFO *ccd, USHORT timid)
{
	/*--------------------------------------------------*/
	/*	タイマーキャンセル								*/
	/*--------------------------------------------------*/
	hrtimer_try_to_cancel(&ccd->tim.tmr_inf[timid].hndr);
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_tim_all_stop									*
*																			*
*		DESCRIPTION	: 全タイマー停止										*
*																			*
*		PARAMETER	: IN  : *ccd	: コンテキスト							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_tim_all_stop(ST_CCD_MNG_INFO *ccd)
{
	INT		i;

	/*--------------------------------------------------*/
	/*	各タイマ初期設定								*/
	/*--------------------------------------------------*/
	for(i=0; i<M_CCD_TIM_MAX; i++) {
		hrtimer_try_to_cancel(&ccd->tim.tmr_inf[i].hndr);
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_set_sig_cs										*
*																			*
*		DESCRIPTION	: ＣＳ信号制御処理										*
*																			*
*		PARAMETER	: IN  : *ccd	: コンテキスト							*
*						  : act		: ACTIVE or NEGATIVE					*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
inline	VOID	fc_ccd_set_sig_cs(ST_CCD_MNG_INFO *ccd, INT act)
{
	USHORT	i;
	LONG	usec;

	/*--------------------------------------------------*/
	/*	設定状態に変化がないときはそのまま設定			*/
	/*--------------------------------------------------*/
	if(ccd->cpu.prt.cs_sts == act) {
		if(act == ACTIVE) {
			gpio_set_value(ccd->cpu.pin.MREQ, LOW);
		}
		else {
			gpio_set_value(ccd->cpu.pin.MREQ, HIGH);
		}
	}

	/*--------------------------------------------------*/
	/*	設定状態に変化があるときはＬ時に時間確保する	*/
	/*	ＳＨマイコンの取り逃し対策						*/
	/*--------------------------------------------------*/
	else {
		/*--------------------------------------------------*/
		/*	ＣＳＬ時は最低時間（200us）保持する				*/
		/*--------------------------------------------------*/
		if(act == ACTIVE) {
			for(i=0; i<500; i++) {
				usec = ktime_us_delta(ktime_get(), ccd->cpu.prt.cs_tim);
				/*--------------------------------------------------*/
				/*	200us経過or時間オーバフローでＣＳセットする		*/
				/*--------------------------------------------------*/
				if((usec < 0) || (usec > 200)) {
					break;
				}
				/*--------------------------------------------------*/
				/*	インループにて少し待つ							*/
				/*--------------------------------------------------*/
#if CPUCOMMODULE==1
				cm_delay(100);
#else
				__delay(500);				/* 1usecちょいぐらい・				*/
#endif
			}
			gpio_set_value(ccd->cpu.pin.MREQ, LOW);
		}
		/*--------------------------------------------------*/
		/*	ＣＳＨは時間取得してそのまま設定				*/
		/*--------------------------------------------------*/
		else {
			ccd->cpu.prt.cs_tim = ktime_get();
			gpio_set_value(ccd->cpu.pin.MREQ, HIGH);
		}
		/*--------------------------------------------------*/
		/*	ＣＳに信号出力設定								*/
		/*--------------------------------------------------*/
		if(act == ACTIVE) {
			gpio_set_value(ccd->cpu.pin.MREQ, LOW);
		}
		else {
			gpio_set_value(ccd->cpu.pin.MREQ, HIGH);
		}
	}
	ccd->cpu.prt.cs_sts = act;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_get_sig_int									*
*																			*
*		DESCRIPTION	: ＩＮＴ信号取得処理									*
*																			*
*		PARAMETER	: IN  : *ccd : コンテキスト								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : active (ACTIVE or NEGATIVE)						*
*																			*
****************************************************************************/
BYTE	fc_ccd_get_sig_int(ST_CCD_MNG_INFO *ccd)
{
	int		sig;

	sig = gpio_get_value(ccd->cpu.pin.SREQ);

	if(sig == LOW)
			return ACTIVE;
	else	return NEGATIVE;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_rel_res										*
*																			*
*		DESCRIPTION	: リソース開放処理										*
*																			*
*		PARAMETER	: IN  : *ccd : コンテキスト								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_rel_res(ST_CCD_MNG_INFO *ccd)
{
	ST_CCD_TXRX_BUF		*del;
	INT					ret;
	LONG				i;

	/*--------------------------------------------------*/
	/*	送受信バッファクリア							*/
	/*--------------------------------------------------*/
	memset(&(ccd->comm.tx_buf), 0, sizeof(ccd->comm.tx_buf));
	memset(&(ccd->comm.rx_buf), 0, sizeof(ccd->comm.rx_buf));

	/*--------------------------------------------------*/
	/*	受信キュー初期化＆開放							*/
	/*--------------------------------------------------*/
	for(i=0; i<1000; i++) {
		ret = list_empty(&ccd->comm.rx_que);
		if(ret == 0) {						/* リストが空でない					*/
			del = list_first_entry(&ccd->comm.rx_que, ST_CCD_TXRX_BUF, list);
											/* リスト取得						*/
			list_del(&del->list);			/* リストから削除					*/
			kfree(del);						/* メモリ開放						*/
		}
		else {								/* リストが空						*/
			break;							/* リスト削除完了					*/
		}
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_clr										*
*																			*
*		DESCRIPTION	: 状態遷移クリア処理									*
*																			*
*		PARAMETER	: IN  : *ccd : コンテキスト								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_clr(ST_CCD_MNG_INFO *ccd)
{
	BYTE		sig;
	/*--------------------------------------------------*/
	/*	送受信待ちの待ち解除							*/
	/*--------------------------------------------------*/
	if(ccd->sys.event.tx_flg == 0){
		ccd->sys.event.tx_flg = 1;
		fc_ccd_drc_event(CCD_EVENT_ID_WAKEUP_TX,CCD_EVENT_FORMATID_WAKEUP,-4,ccd->dev.spi.id_name[0]);
		wake_up_interruptible(&ccd->sys.event.tx_evq);
	}
	if(ccd->sys.event.rx_flg == 0){
		ccd->sys.event.rx_flg = 1;
		fc_ccd_drc_event(CCD_EVENT_ID_WAKEUP_RX,CCD_EVENT_FORMATID_WAKEUP,-4,ccd->dev.spi.id_name[0]);
		wake_up_interruptible(&ccd->sys.event.rx_evq);
	}
	/*--------------------------------------------------*/
	/*	ＣＳ信号インアクティブ							*/
	/*--------------------------------------------------*/
	fc_ccd_set_sig_cs(ccd, NEGATIVE);

	/*--------------------------------------------------*/
	/*	送受信バッファクリア							*/
	/*--------------------------------------------------*/
	memset(&ccd->comm.tx_buf, 0, sizeof(ccd->comm.tx_buf));
	memset(&ccd->comm.rx_buf, 0, sizeof(ccd->comm.rx_buf));

	/*--------------------------------------------------*/
	/*	全タイマ停止									*/
	/*--------------------------------------------------*/
	fc_ccd_tim_all_stop(ccd);

	/*--------------------------------------------------*/
	/*	汎用タイマ起動									*/
	/*--------------------------------------------------*/
	fc_ccd_tim_start(ccd, M_CCD_TIM_COMON);
											/* タイマ起動					*/
	ccd->tim.tcnt = 0;						/* タイマカウンタクリア			*/

	/*--------------------------------------------------*/
	/*	状態をアイドルへ変更							*/
	/*--------------------------------------------------*/
	fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C1_IDLE);

	/*--------------------------------------------------*/
	/*	ＩＮＴ信号がアクティブの場合は一旦、			*/
	/*	ＩＮＴＨ待ち状態へ遷移させＩＮＴネガティブを待つ*/
	/*--------------------------------------------------*/
	sig = fc_ccd_get_sig_int(ccd);
	if(sig == ACTIVE) {
		fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_E0_INTE);
											/* ＩＮＴエラー状態へ遷移			*/
	}
	ccd->cpu.irq.sreq_sts = sig;
	ccd->cpu.irq.sreq_selfsts = 0xFF;

}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_init_drv_inf									*
*																			*
*		DESCRIPTION	: ドライバ情報初期化処理								*
*																			*
*		PARAMETER	: IN  : *ccd : コンテキスト								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_init_drv_inf(ST_CCD_MNG_INFO *ccd)
{
	/*--------------------------------------------------*/
	/*	ＣＳ信号インアクティブ							*/
	/*--------------------------------------------------*/
	fc_ccd_set_sig_cs(ccd, NEGATIVE);

	/*--------------------------------------------------*/
	/*	割り込み禁止									*/
	/*--------------------------------------------------*/
	fc_ccd_disable_irq(ccd);

	/*--------------------------------------------------*/
	/*	全タイマ停止									*/
	/*--------------------------------------------------*/
	fc_ccd_tim_all_stop(ccd);

	/*--------------------------------------------------*/
	/*	リソース開放									*/
	/*--------------------------------------------------*/
	fc_ccd_rel_res(ccd);

	/*--------------------------------------------------*/
	/*	状態を初期化前へ変更							*/
	/*--------------------------------------------------*/
	ccd->mng.mode = M_CCD_MODE_C0_INIT;

	/*--------------------------------------------------*/
	/*	管理情報クリア									*/
	/*--------------------------------------------------*/
	ccd->tim.tcnt = 0;
	ccd->tim.mchk = 0;

	/*--------------------------------------------------*/
	/*	受付拒否										*/
	/*--------------------------------------------------*/
	ccd->bf_ccd_acpt_flg = FALSE;
	/*--------------------------------------------------*/
	/*	割り込み自己設定								*/
	/*--------------------------------------------------*/
	ccd->cpu.irq.sreq_selfsts = 0xFF;

}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_chk_rcv_dat									*
*																			*
*		DESCRIPTION	: 受信データ確認処理									*
*																			*
*		PARAMETER	: IN  : *rcv : 受信データ								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ＯＫ／ＮＧ										*
*																			*
****************************************************************************/
SHORT	fc_ccd_chk_rcv_dat(ST_CCD_TXRX_BUF *rcv)
{
	ULONG	clc_fcc, rcv_fcc;

	/*--------------------------------------------------*/
	/*	ＦＣＣ取得										*/
	/*--------------------------------------------------*/
	rcv_fcc  = 0;
	rcv_fcc |= (ULONG)((((ULONG)(rcv->dat[rcv->len+0]) << 24) & 0xFF000000));
	rcv_fcc |= (ULONG)((((ULONG)(rcv->dat[rcv->len+1]) << 16) & 0x00FF0000));
	rcv_fcc |= (ULONG)((((ULONG)(rcv->dat[rcv->len+2]) <<  8) & 0x0000FF00));
	rcv_fcc |= (ULONG)((((ULONG)(rcv->dat[rcv->len+3])      ) & 0x000000FF));

	/*--------------------------------------------------*/
	/*	ＦＣＣ計算										*/
	/*--------------------------------------------------*/
	clc_fcc = fc_ccd_calc_fcc(rcv->dat, rcv->len);
											/* FCC計算							*/
	clc_fcc = clc_fcc + rcv->rcv_len;		/* LENデータを足す					*/

	/*--------------------------------------------------*/
	/*	ＦＣＣ確認										*/
	/*--------------------------------------------------*/
	if(clc_fcc != rcv_fcc) {
		/* WARNING TRACE */
		LOGW("WAR0006:clc_fcc=%08X rcv_fcc=%08X",(int)clc_fcc, (int)rcv_fcc);
		return	M_CCD_CSTS_NG;
	}
	else {
		return	M_CCD_CSTS_OK;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_set_rcv_lst									*
*																			*
*		DESCRIPTION	: 受信リスト格納処理									*
*																			*
*		PARAMETER	: IN  : *ccd : コンテキスト								*
*						  : *rcv : 受信データ								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_set_rcv_lst(ST_CCD_MNG_INFO *ccd, ST_CCD_TXRX_BUF *rcv)
{
	ST_CCD_TXRX_BUF		*ent;

	ent = NULL;

	/*--------------------------------------------------*/
	/*	メモリ確保して受信データをコピー				*/
	/*--------------------------------------------------*/
	ent = kmalloc(sizeof(*ent), GFP_KERNEL);
	if(ent == NULL) {						/* 確保失敗！！						*/
		/* ERROR TRACE */
		LOGE("[%s]ERR0044:ent=NULL",ccd->dev.spi.id_name);
		fc_ccd_drc_error(44,CHANNEL,0,0,MODE);
		return;
	}
	memcpy(ent, rcv, sizeof(*ent));			/* 登録用データにコピー				*/
	INIT_LIST_HEAD(&ent->list);

	/*--------------------------------------------------*/
	/*	受信キューリストに格納							*/
	/*--------------------------------------------------*/
	list_add_tail(&ent->list, &ccd->comm.rx_que);
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_chg_fsm_mod									*
*																			*
*		DESCRIPTION	: 状態遷移状態変更処理									*
*																			*
*		PARAMETER	: IN  : *ccd : コンテキスト								*
*						  : mode : 変更先モード								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_chg_fsm_mod(ST_CCD_MNG_INFO *ccd, BYTE mode)
{
	/*--------------------------------------------------*/
	/*	モード変化するときは固着監視カウンタクリア		*/
	/*--------------------------------------------------*/
	if(ccd->mng.mode != mode) {
		ccd->tim.mchk = 0;
	}

	/*--------------------------------------------------*/
	/*	モード変更										*/
	/*--------------------------------------------------*/
	LOGT("[%s]MODE CHANGE EVENT [%s  ->  %s]",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str,stateTBL[mode].str);
	ccd->mng.previous_mode = ccd->mng.mode;
	ccd->mng.mode = mode;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_rdy_shtdwn										*
*																			*
*		DESCRIPTION	: シャットダウン準備処理								*
*																			*
*		PARAMETER	: IN  : *ccd : コンテキスト								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_rdy_shtdwn(ST_CCD_MNG_INFO *ccd)
{
	/*--------------------------------------------------*/
	/*	ＣＳ＝Ｈ出力（インアクティブ）					*/
	/*--------------------------------------------------*/
	fc_ccd_set_sig_cs(ccd, NEGATIVE);

	/*--------------------------------------------------*/
	/*	状態遷移情報クリア								*/
	/*--------------------------------------------------*/
	fc_ccd_fsm_clr(ccd);

	/*--------------------------------------------------*/
	/*	送受信待ちの待ち解除							*/
	/*--------------------------------------------------*/
	if(ccd->sys.event.tx_flg == 0){
		LOGT("[%s]%s(%d) Wake Up Tx",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
		fc_ccd_drc_event(CCD_EVENT_ID_WAKEUP_TX,CCD_EVENT_FORMATID_WAKEUP,CCD_EVENT_ID_SHUTDOWN,ccd->dev.spi.id_name[0]);
		ccd->sys.event.tx_flg = 1;
		wake_up_interruptible(&ccd->sys.event.tx_evq);
	}
	if(ccd->sys.event.rx_flg == 0){
		LOGT("[%s]%s(%d) Wake Up Rx",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
		fc_ccd_drc_event(CCD_EVENT_ID_WAKEUP_RX,CCD_EVENT_FORMATID_WAKEUP,CCD_EVENT_ID_SHUTDOWN,ccd->dev.spi.id_name[0]);
		ccd->sys.event.rx_flg = 1;
		wake_up_interruptible(&ccd->sys.event.rx_evq);
	}
	/*--------------------------------------------------*/
	/*	初期化待ちへ遷移								*/
	/*--------------------------------------------------*/
	fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C0_INIT);
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_disable_irq									*
*																			*
*		DESCRIPTION	: 割り込み割込み禁止処理								*
*																			*
*		PARAMETER	: IN  : *ccd : コンテキスト								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
inline	VOID	fc_ccd_disable_irq(ST_CCD_MNG_INFO *ccd)
{
	ULONG	flags;

	/*--------------------------------------------------*/
	/*	ＳＰＩＮＬＯＣＫでの排他　開始					*/
	/*--------------------------------------------------*/
	spin_lock_irqsave(&ccd->cpu.irq.irq_ctl,flags);

	/*--------------------------------------------------*/
	/*	ＩＮＴ信号割込み制御							*/
	/*--------------------------------------------------*/
	if(ccd->cpu.irq.ena_sts == M_CCD_ENABLE) {
		disable_irq(ccd->cpu.irq.sreq_num);
	}

	/*--------------------------------------------------*/
	/*	割り込み状態設定								*/
	/*--------------------------------------------------*/
	ccd->cpu.irq.ena_sts = M_CCD_DISABLE;

	/*--------------------------------------------------*/
	/*	ＳＰＩＮＬＯＣＫでの排他　終了					*/
	/*--------------------------------------------------*/
	spin_unlock_irqrestore(&ccd->cpu.irq.irq_ctl,flags);
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_enable_irq										*
*																			*
*		DESCRIPTION	: ドライバ割り込み許可処理								*
*																			*
*		PARAMETER	: IN  : *ccd : コンテキスト								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
inline	VOID	fc_ccd_enable_irq(ST_CCD_MNG_INFO *ccd)
{
	ULONG	flags;

	/*--------------------------------------------------*/
	/*	割り込み禁止期間中にタイマ割込みが				*/
	/*	発生していたら処理する							*/
	/*--------------------------------------------------*/
	if(ccd->cpu.irq.tim_res != 0) {
		if((ccd->cpu.irq.tim_res & M_CCD_TFG_TCLK1) != 0) {
			fc_ccd_fsm_timout(ccd, M_CCD_TIM_TCLK1);
		}
		if((ccd->cpu.irq.tim_res & M_CCD_TFG_TCSH) != 0) {
			fc_ccd_fsm_timout(ccd, M_CCD_TIM_TCSH);
		}
		if((ccd->cpu.irq.tim_res & M_CCD_TFG_TIDLE) != 0) {
			fc_ccd_fsm_timout(ccd, M_CCD_TIM_TIDLE);
		}
		if((ccd->cpu.irq.tim_res & M_CCD_TFG_TFAIL1) != 0) {
			fc_ccd_fsm_timout(ccd, M_CCD_TIM_TFAIL1);
		}
		if((ccd->cpu.irq.tim_res & M_CCD_TFG_COMON) != 0) {
			fc_ccd_fsm_timout(ccd, M_CCD_TIM_COMON);
		}
		if((ccd->cpu.irq.tim_res & M_CCD_TFG_INTFAIL) != 0) {
			fc_ccd_fsm_timout(ccd, M_CCD_TFG_INTFAIL);
		}
		ccd->cpu.irq.tim_res = 0;
	}

	/*--------------------------------------------------*/
	/*	ＳＰＩＮＬＯＣＫでの排他　開始					*/
	/*--------------------------------------------------*/
	spin_lock_irqsave(&ccd->cpu.irq.irq_ctl,flags);

	/*--------------------------------------------------*/
	/*	ＩＮＴ信号割込み制御							*/
	/*--------------------------------------------------*/
	if(ccd->cpu.irq.ena_sts == M_CCD_DISABLE) {
		enable_irq(ccd->cpu.irq.sreq_num);
	}

	/*--------------------------------------------------*/
	/*	割り込み状態設定								*/
	/*--------------------------------------------------*/
	ccd->cpu.irq.ena_sts = M_CCD_ENABLE;

	/*--------------------------------------------------*/
	/*	ＳＰＩＮＬＯＣＫでの排他　終了					*/
	/*--------------------------------------------------*/
	spin_unlock_irqrestore(&ccd->cpu.irq.irq_ctl,flags);
}
/****************************************************************************
*				drv_spi_cm.c	END											*
****************************************************************************/




