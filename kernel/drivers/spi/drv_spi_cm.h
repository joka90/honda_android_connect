/*--------------------------------------------------------------------------*/
/* COPYRIGHT(C) FUJITSU TEN LIMITED 2012-2014                               */
/*--------------------------------------------------------------------------*/
/****************************************************************************
*																			*
*		シリーズ名称	: １４ＭＹ											*
*		マイコン名称	: メインマイコン									*
*		オーダー名称	: １４ＭＹ　Ａｎｄｒｏｉｄ－ＤＡ					*
*		ブロック名称	: Ｌｉｎｕｘカーネルドライバ						*
*		DESCRIPTION		: ＣＰＵ間通信ドライバ定義							*
*																			*
*		ファイル名称	: DrvCpuCm.h										*
*		作　成　者		: Micware)ogura & segawa							*
*		作　成　日		: 2013/01/25	<Ver. 1.00>							*
*		改　訂　日		: yyyy/mm/dd	<Ver. x.xx>							*
*																			*
*																			*
****************************************************************************/
#ifndef __DrvCpuCm_H__
#define __DrvCpuCm_H__


/************************************************************************
*																		*
*			Ｃompile Ｓwitch Ｄeclare Ｓection							*
*																		*
************************************************************************/
/*==========================================*/
/*	ログ出力								*/
/*==========================================*/
#define CPUCOMDEBUG		0					/* 1=デバッグ出力					*/
#define CPUCOMMODULE	0					/* 1=MODULE	0=y						*/

/*==========================================*/
/*	ＳＰＩ／パタパタ切り替え				*/
/*==========================================*/
#define IF_GPIO			1					/* GPIO制御							*/
#define IF_SPI_TEGRA	2					/* SPI				 				*/
#define __INTERFACE__		IF_GPIO

/*==========================================*/
/*	割り込み禁止方法						*/
/*==========================================*/
#define	__CCD_IRQ_LOCK__	 0				/* 0=disable_irq 1=spin_lock_irq	*/


/*==========================================*/
/*	IOCTL									*/
/*==========================================*/
#define CPU_IOC_MAGIC			'c'
#define	CPU_IOC_ISSUSPEND		((CPU_IOC_MAGIC<<8) | 1)	/* bf_ccd_shut_down_flg状態を返す	*/
#define	CPU_IOC_FDCKECK			((CPU_IOC_MAGIC<<8) | 2)	/* open後のFDチェック状態を返す	*/
#define	CPU_IOC_EXIT			((CPU_IOC_MAGIC<<8) | 3)	/* read/write lock解除	*/

/*--------* Ｅnd of Ｃompile Ｓwitch Ｄeclare Ｓection *----------------*/

/************************************************************************
*																		*
*			Ｍacro Ｄeclare Ｓection									*
*																		*
************************************************************************/
#define		SIZE_OF_EXCLUDE_FCC(x)	(x*4+1)
#if CPUCOMMODULE==1
	#define		CLK_DELAY				cm_delay(200)//__delay(700)
#else
	#define		CLK_DELAY				__delay(700)
#endif

/*==========================================*/
/*	ログ出力マクロ							*/
/*==========================================*/
#if CPUCOMDEBUG	== 1
	#define LOGD(fmt, ...)	fc_ccd_drc_out( KERN_INFO		M_CCD_DRV_NAME	" %s(%d):"	pr_fmt(fmt),	__FUNCTION__,	__LINE__,	##__VA_ARGS__ )
	#define LOGT(fmt, ...)	fc_ccd_drc_out( KERN_INFO		pr_fmt(fmt),	##__VA_ARGS__ )
	#define LOGM(fmt, ...)	fc_ccd_drc_out( KERN_ERR		M_CCD_DRV_NAME	" %s(%d):"	pr_fmt(fmt),	__FUNCTION__,	__LINE__, 	##__VA_ARGS__ )
	#define LOGW(fmt, ...)	fc_ccd_drc_out( KERN_WARNING	M_CCD_DRV_NAME	" %s(%d):"	pr_fmt(fmt),	__FUNCTION__,	__LINE__, 	##__VA_ARGS__ )
	#define LOGE(fmt, ...)	fc_ccd_drc_out( KERN_ERR		M_CCD_DRV_NAME	" %s(%d):"	pr_fmt(fmt),	__FUNCTION__,	__LINE__, 	##__VA_ARGS__ )
#else
	#define LOGM(fmt, ...)	ADA_LOG_DEBUG	(M_CCD_DRV_NAME, " %s(%d):" pr_fmt(fmt),	__FUNCTION__,	__LINE__, 	##__VA_ARGS__ )
	#define LOGD(fmt, ...)	ADA_LOG_DEBUG	(M_CCD_DRV_NAME, " %s(%d):" pr_fmt(fmt),	__FUNCTION__,	__LINE__, 	##__VA_ARGS__ )
	#define LOGT(fmt, ...)	ADA_LOG_DEBUG	(M_CCD_DRV_NAME, pr_fmt(fmt),	##__VA_ARGS__ )
	#define LOGW(fmt, ...)	ADA_LOG_WARNING	(M_CCD_DRV_NAME, " %s(%d):" pr_fmt(fmt),	__FUNCTION__,	__LINE__, 	##__VA_ARGS__ )
	#define LOGE(fmt, ...)	ADA_LOG_ERR		(M_CCD_DRV_NAME, " %s(%d):" pr_fmt(fmt),	__FUNCTION__,	__LINE__, 	##__VA_ARGS__ )
#endif /* CPUCOMDEBUG */

/*==========================================*/
/*	時間変換マクロ							*/
/*==========================================*/
#define		M_CCD_MCR_MS2NS(x)		(x*1000*1000 )
#define		M_CCD_MCR_US2NS(x)		(x*1000      )
#define		M_CCD_MCR_MS2US(x)		(x*1000      )
#define		M_CCD_MCR_US2SEC(x)		(x/1000000   )
#define		M_CCD_MCR_SEC2US(x)		(x*1000000   )
#define		M_CCD_MCR_SEC2NS(x)		(x*1000000000)

/*--------* Ｅnd of Ｍacro Ｄeclare Ｓection *--------------------------*/

/************************************************************************
*																		*
*			Ｄefine Ｄeclare Ｓection									*
*																		*
************************************************************************/
/*==========================================*/
/*	共通定義								*/
/*==========================================*/
#define		ACTIVE					1
#define		NEGATIVE				0

#define		ESUCCESS				0		/* return code						*/
#define		CPUCOM_TX_RTRY	(16)			/* マスタ送信リトライカウンタ		*/
#define		PRE_SUSPEND_RETVAL	0x7FFF		/* 高速終了時リターンコード			*/

#define 	N_SPI_MINORS	32				/* ... up to 256 					*/
#define 	M_CCD_DRV_NAME	"spi-cpucom"

#define		M_CCD_ENABLE			0		/* 許可								*/
#define		M_CCD_DISABLE			1		/* 禁止								*/
#define 	UNUSED(x) 				(void)(x)/* パラメータ未使用				*/
#define 	CHANNEL					(ccd?ccd->dev.spi.sdev->master->bus_num:-1)
#define 	MODE					(ccd?ccd->mng.mode:-1)

/*==========================================*/
/*	共通ステータス定義						*/
/*==========================================*/
#define		M_CCD_CSTS_OK			0		/* ＯＫ								*/
#define		M_CCD_CSTS_NG			1		/* ＮＧ								*/
#define		M_CCD_CSTS_SUS			2		/* ＳＵＳＰＥＮＤ					*/

#define		M_CCD_ISTS_NONE			0		/* 無し								*/
#define		M_CCD_ISTS_READY		1		/* 準備中							*/
#define		M_CCD_ISTS_CMPLT		2		/* 完了（正常）						*/
#define		M_CCD_ISTS_STOP			3		/* 中断								*/
#define		M_CCD_ISTS_SUSPEND		4		/* サスペンド発生					*/
#define		M_CCD_ISTS_ERR			5		/* エラー発生						*/

/*==========================================*/
/*	状態									*/
/*==========================================*/
#define		M_CCD_MODE_C0_INIT		0		/* 共通：初期化待ち（初期値）		*/
#define		M_CCD_MODE_C1_IDLE		1		/* 共通：アイドル					*/
#define		M_CCD_MODE_C2_KPID		2		/* 共通：アイドル保持期間			*/
#define		M_CCD_MODE_S0_SNDW		3		/* 送信：送信待ち					*/
#define		M_CCD_MODE_S1_SNDC		4		/* 送信：送信完了待ち				*/
#define		M_CCD_MODE_S2_CSHW		5		/* 送信：ＣＳ　Ｈ待ち				*/
#define		M_CCD_MODE_S3_CSHK		6		/* 送信：送信後　ＣＳＨ保持期間		*/
#define		M_CCD_MODE_S4_ITHW		7		/* 送信：受信移行中　ＩＮＴＨ待ち	*/
#define		M_CCD_MODE_S5_ITLW		8		/* 送信：受信移行中　ＩＮＴＬ待ち	*/
#define		M_CCD_MODE_R0_ITHW		9		/* 受信：ＩＮＴ　Ｈ待ち				*/
#define		M_CCD_MODE_R1_RCVC		10		/* 受信：受信完了待ち				*/
#define		M_CCD_MODE_E0_INTE		11		/* 異常：ＩＮＴ異常					*/

/*==========================================*/
/*	ポート識別子							*/
/*==========================================*/
#define		CPU_COM_SREQ0		TEGRA_GPIO_PK5
#define		CPU_COM_MREQ0		TEGRA_GPIO_PEE0
#define		CPU_COM_MCLK0		TEGRA_GPIO_PO7
#define		CPU_COM_MDT0		TEGRA_GPIO_PO5
#define		CPU_COM_SDT0		TEGRA_GPIO_PO6
#define		CPU_COM_SREQ1		TEGRA_GPIO_PW2
#define		CPU_COM_MREQ1		TEGRA_GPIO_PEE1
#define		CPU_COM_MCLK1		TEGRA_GPIO_PY2
#define		CPU_COM_MDT1		TEGRA_GPIO_PY0
#define		CPU_COM_SDT1		TEGRA_GPIO_PY1

/*==========================================*/
/*	通信定義								*/
/*==========================================*/
#if	__INTERFACE__ == IF_SPI_TEGRA
#define		M_CCD_COMM_PACKET		32		/* １回あたりの送受信数				*/
#else /* __INTERFACE__ == IF_GPIO */
#define		M_CCD_COMM_PACKET		4		/* １回あたりの送受信数				*/
#endif
#define		M_CCD_TRNS_CNT_MAX		1024	/* 通信データサイズMAX				*/
#define		M_CCD_LEN_SIZE			1		/* LENデータ数						*/
#define		M_CCD_FCC_SIZE			4		/* FCCデータ数						*/
#define		M_CCD_TRNS_IF_MAX		(M_CCD_TRNS_CNT_MAX - M_CCD_FCC_SIZE)
											/* 上位層とのIFサイズMAX			*/

/*==========================================*/
/*	タイマ識別子							*/
/*==========================================*/
#define		M_CCD_TIM_TCLK1		0			/* No.0：ｔｃｌｋ１					*/
#define		M_CCD_TIM_TCSH		1			/* No.1：ｔｃｓｈ					*/
#define		M_CCD_TIM_TIDLE		2			/* No.2：ｔｉｄｌｅ					*/
#define		M_CCD_TIM_TFAIL1	3			/* No.3：ｔｆａｉｌ１				*/
#define		M_CCD_TIM_COMON		4			/* No.4：汎用						*/
#define		M_CCD_TIM_INTFAIL	5			/* No.5：INT FAILSAFE				*/

#define		M_CCD_TIM_MAX		6			/* タイマ最大値						*/

/*==========================================*/
/*	タイマフラグ							*/
/*==========================================*/
#define		M_CCD_TFG_TCLK1		0x00000001	/* No.0：ｔｃｌｋ１					*/
#define		M_CCD_TFG_TCSH		0x00000002	/* No.1：ｔｃｓｈ					*/
#define		M_CCD_TFG_TIDLE		0x00000004	/* No.2：ｔｉｄｌｅ					*/
#define		M_CCD_TFG_TFAIL1	0x00000008	/* No.3：ｔｆａｉｌ１				*/
#define		M_CCD_TFG_COMON		0x00000010	/* No.4：汎用						*/
#define		M_CCD_TFG_INTFAIL	0x00000020	/* No.5：INT FAILSAFE				*/

/*==========================================*/
/*	汎用タイマ定義							*/
/*==========================================*/
#define		M_CCD_COMTIM_TOUT_MS	50
#define		M_CCD_COMTIM_100MS		(       100/M_CCD_COMTIM_TOUT_MS)
#define		M_CCD_COMTIM_300MS		(       300/M_CCD_COMTIM_TOUT_MS)
#define		M_CCD_COMTIM_500MS		(       500/M_CCD_COMTIM_TOUT_MS)
#define		M_CCD_COMTIM_1SEC		(    1*1000/M_CCD_COMTIM_TOUT_MS)
#define		M_CCD_COMTIM_5SEC		(    5*1000/M_CCD_COMTIM_TOUT_MS)
#define		M_CCD_COMTIM_1MIN		( 1*60*1000/M_CCD_COMTIM_TOUT_MS)
#define		M_CCD_COMTIM_MAX		( 2*60*1000/M_CCD_COMTIM_TOUT_MS)

/*==========================================*/
/*	ワーカースレッド定義					*/
/*==========================================*/
#define		M_CCD_WINF_SND			0		/* 送信イベント						*/
#define		M_CCD_WINF_RCV			1		/* 受信イベント						*/
#define		M_CCD_WINF_CNT			2		/* ワーカースレッドイベント数		*/

/*==========================================*/
/*	ドラレコログ出力						*/
/*==========================================*/
#define DRIVERECODER_CPUCOMDRIVER_NO	13			/*	CPU間通信ドライバ：knl0013：星取表より */

/*--------* Ｅnd of Ｄefine Ｄeclare Ｓection *-------------------------*/

/************************************************************************
*																		*
*			Ｓtruct Ｄeclare Ｓection									*
*																		*
************************************************************************/
/*==========================================*/
/*	ワーカスレッド情報						*/
/*==========================================*/
typedef	struct _TAG_CCD_WORKER_INFO {
	struct	work_struct 	workq;			/* ワーカスレッド管理情報			*/
	BYTE					eve_num;		/* 起動イベント番号					*/
	void					*inst_p;		/* インスタンスバッファポインタ		*/
} ST_CCD_WORKER_INFO;


/*==========================================*/
/*	送受信情報								*/
/*==========================================*/
typedef	struct _TAG_CCD_TXRX_BUF {
	struct list_head	list;				/* リスト管理情報（標準関数用）		*/
	BYTE				dat[M_CCD_TRNS_CNT_MAX + 16];
											/* 送受信データ(+16はﾌｪｰﾙ）			*/
	USHORT				len;				/* データ長							*/
	BYTE				rcv_len;			/* データ長受信データ				*/
	SHORT				sts;				/* ステータス情報					*/
} ST_CCD_TXRX_BUF;

/*==========================================*/
/*	タイマ情報								*/
/*==========================================*/
typedef	struct _TAG_CCD_TIMER_INFO {
	void				*inst_p;			/* インスタンスバッファポインタ		*/
    INT 				tim_num;			/* タイマ番号						*/
	ULONG				usec;				/* タイムアウト値					*/
    struct hrtimer 		hndr;				/* タイマハンドラ					*/
} ST_CCD_TIMER_INFO;

/*==========================================*/
/*	ＣＰＵ間通信ドライバ管理バッファ		*/
/*==========================================*/
typedef	struct _TAG_CCD_MNG_INFO {
	struct {	/*---* デバイスドライバ情報 *-----------------------------------*/
		dev_t				devt;			/* 登録デバイス番号					*/
		struct list_head	dev_ent;		/* デバイスリスト登録用情報			*/
		struct {	/*---* ＳＰＩデバイスドライバ情報 *-------------------------*/
			struct spi_device	*sdev;		/* ＳＰＩデバイス制御ポインタ		*/
			struct tegra_spi_device_controller_data cnfg;
											/* ＳＰＩ設定データ					*/
			char				id_name[8];	/* ＳＰＩバス識別情報名				*/
			spinlock_t			lockt;		/* ＳＰＩ排他制御					*/
		} spi;
		INT	 					users;		/* OPEN数							*/
	} dev;
	struct {	/*---* ＣＰＵリソース情報 *-------------------------------------*/
		struct {	/*---* ＧＰＩＯ－ＰＩＮ *-----------------------------------*/
			INT		MCLK;					/* ＭＣＬＫ（ＣＬＫ）				*/
			INT		MREQ;					/* ＭＲＥＱ（ＣＳ）					*/
			INT		SREQ;					/* ＳＲＥＱ（ＩＮＴ）				*/
			INT		SDAT;					/* ＳＤＡＴ（ＳＩ）					*/
			INT		MDAT;					/* ＭＤＡＴ（ＳＯ）					*/
		} pin;
		struct {	/*---* 割り込み *-------------------------------------------*/
			INT			sreq_num;			/* ＳＲＥＱ（ＩＮＴ）割り込み番号	*/
			BYTE		sreq_sts;			/* ＳＲＥＱ（ＩＮＴ）信号取得状態	*/
			BYTE 		sreq_selfsts;		/* ＳＲＥＱ（ＩＮＴ）自己設定状態	*/
			BYTE		ena_sts;			/* 割込み許可状態					*/
			ULONG		irq_flags;			/* 割込み状態保存フラグ				*/
			ULONG		tim_res;			/* タイマ割込み保留フラグ			*/
			spinlock_t	irq_ctl;			/* 割込み制御排他					*/
		} irq;
		struct {	/*---* ポート *---------------------------------------------*/
			BYTE		cs_sts;				/* ＣＳ信号設定状態					*/
			ktime_t		cs_tim;				/* インアクティブ時間				*/
		} prt;
	} cpu;
	struct {	/*---* 管理情報 *-----------------------------------------------*/
		BYTE	previous_mode;				/* 前モード							*/
		BYTE	mode;						/* モード							*/
		struct workqueue_struct *wkr_thrd;	/* ワーカースレッド					*/
		ST_CCD_WORKER_INFO		wkr_inf[M_CCD_WINF_CNT];
											/* ワーカスレッドＩＦ情報			*/
	} mng;
	struct {	/*---* タイマ情報 *---------------------------------------------*/
		ST_CCD_TIMER_INFO		tmr_inf[M_CCD_TIM_MAX];
											/* タイマ情報						*/
		ULONG					tcnt;		/* 汎用タイマカウンタ				*/
		ULONG					mchk;		/* モード固着監視カウンタ			*/
	} tim;
	struct {	/*---* 通信情報 *-----------------------------------------------*/
		ST_CCD_TXRX_BUF 	tx_buf;			/* 送信バッファ						*/
		ST_CCD_TXRX_BUF 	rx_buf;			/* 受信バッファ						*/
		struct list_head 	rx_que;			/* 受信キュー						*/
	} comm;
	struct {	/*---* システム制御情報 *---------------------------------------*/
		struct {	/*---* イベント制御 *---------------------------------------*/
			wait_queue_head_t	tx_evq;		/* 送信待ちＷＡＩＴＱＵＥ			*/
			wait_queue_head_t	rx_evq;		/* 受信待ちＷＡＩＴＱＵＥ			*/
			INT					tx_flg;		/* 送信待ちフラグ					*/
			INT					rx_flg;		/* 受信待ちフラグ					*/
		} event;
		struct {	/*---* 排他制御 *-------------------------------------------*/
			struct mutex	 	instnc;		/* インスタンス排他					*/
			struct mutex	 	wt_if;		/* ＷＲＩＴＥインターフェース排他	*/
			struct mutex	 	rd_if;		/* ＲＥＡＤインターフェース排他		*/
		} mutex;
	} sys;
	struct early_device		ed_inf;			/* 高速起動イベント情報				*/
	BYTE			bf_ccd_acpt_flg;		/*	受付拒否						*/
	BYTE			bf_ccd_resume_not_yet;	/*	Resume	未発生					*/
} ST_CCD_MNG_INFO;

/*--------* Ｅnd of Ｓtruct Ｄeclare Ｓection *-------------------------*/

/************************************************************************
*																		*
*			Ｅxternal Ｄeclare Ｓection									*
*																		*
************************************************************************/
/*--------* Ｅnd of Ｅxternal Ｄeclare Ｓection *-----------------------*/
#endif	/* __CcmPrcMn_H__ */
/****************************************************************************
*				DrvSpiCm.h	END												*
****************************************************************************/
