/*--------------------------------------------------------------------------*/
/* COPYRIGHT(C) FUJITSU TEN LIMITED 2012-2014                               */
/*--------------------------------------------------------------------------*/
/****************************************************************************
*																			*
*		�V���[�Y����	: �P�S�l�x											*
*		�}�C�R������	: ���C���}�C�R��									*
*		�I�[�_�[����	: �P�S�l�x�@�`�������������|�c�`					*
*		�u���b�N����	: �k���������J�[�l���h���C�o						*
*		DESCRIPTION		: �b�o�t�ԒʐM�h���C�o��`							*
*																			*
*		�t�@�C������	: DrvCpuCm.h										*
*		��@���@��		: Micware)ogura & segawa							*
*		��@���@��		: 2013/01/25	<Ver. 1.00>							*
*		���@���@��		: yyyy/mm/dd	<Ver. x.xx>							*
*																			*
*																			*
****************************************************************************/
#ifndef __DrvCpuCm_H__
#define __DrvCpuCm_H__


/************************************************************************
*																		*
*			�bompile �rwitch �ceclare �rection							*
*																		*
************************************************************************/
/*==========================================*/
/*	���O�o��								*/
/*==========================================*/
#define CPUCOMDEBUG		0					/* 1=�f�o�b�O�o��					*/
#define CPUCOMMODULE	0					/* 1=MODULE	0=y						*/

/*==========================================*/
/*	�r�o�h�^�p�^�p�^�؂�ւ�				*/
/*==========================================*/
#define IF_GPIO			1					/* GPIO����							*/
#define IF_SPI_TEGRA	2					/* SPI				 				*/
#define __INTERFACE__		IF_GPIO

/*==========================================*/
/*	���荞�݋֎~���@						*/
/*==========================================*/
#define	__CCD_IRQ_LOCK__	 0				/* 0=disable_irq 1=spin_lock_irq	*/


/*==========================================*/
/*	IOCTL									*/
/*==========================================*/
#define CPU_IOC_MAGIC			'c'
#define	CPU_IOC_ISSUSPEND		((CPU_IOC_MAGIC<<8) | 1)	/* bf_ccd_shut_down_flg��Ԃ�Ԃ�	*/
#define	CPU_IOC_FDCKECK			((CPU_IOC_MAGIC<<8) | 2)	/* open���FD�`�F�b�N��Ԃ�Ԃ�	*/
#define	CPU_IOC_EXIT			((CPU_IOC_MAGIC<<8) | 3)	/* read/write lock����	*/

/*--------* �dnd of �bompile �rwitch �ceclare �rection *----------------*/

/************************************************************************
*																		*
*			�lacro �ceclare �rection									*
*																		*
************************************************************************/
#define		SIZE_OF_EXCLUDE_FCC(x)	(x*4+1)
#if CPUCOMMODULE==1
	#define		CLK_DELAY				cm_delay(200)//__delay(700)
#else
	#define		CLK_DELAY				__delay(700)
#endif

/*==========================================*/
/*	���O�o�̓}�N��							*/
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
/*	���ԕϊ��}�N��							*/
/*==========================================*/
#define		M_CCD_MCR_MS2NS(x)		(x*1000*1000 )
#define		M_CCD_MCR_US2NS(x)		(x*1000      )
#define		M_CCD_MCR_MS2US(x)		(x*1000      )
#define		M_CCD_MCR_US2SEC(x)		(x/1000000   )
#define		M_CCD_MCR_SEC2US(x)		(x*1000000   )
#define		M_CCD_MCR_SEC2NS(x)		(x*1000000000)

/*--------* �dnd of �lacro �ceclare �rection *--------------------------*/

/************************************************************************
*																		*
*			�cefine �ceclare �rection									*
*																		*
************************************************************************/
/*==========================================*/
/*	���ʒ�`								*/
/*==========================================*/
#define		ACTIVE					1
#define		NEGATIVE				0

#define		ESUCCESS				0		/* return code						*/
#define		CPUCOM_TX_RTRY	(16)			/* �}�X�^���M���g���C�J�E���^		*/
#define		PRE_SUSPEND_RETVAL	0x7FFF		/* �����I�������^�[���R�[�h			*/

#define 	N_SPI_MINORS	32				/* ... up to 256 					*/
#define 	M_CCD_DRV_NAME	"spi-cpucom"

#define		M_CCD_ENABLE			0		/* ����								*/
#define		M_CCD_DISABLE			1		/* �֎~								*/
#define 	UNUSED(x) 				(void)(x)/* �p�����[�^���g�p				*/
#define 	CHANNEL					(ccd?ccd->dev.spi.sdev->master->bus_num:-1)
#define 	MODE					(ccd?ccd->mng.mode:-1)

/*==========================================*/
/*	���ʃX�e�[�^�X��`						*/
/*==========================================*/
#define		M_CCD_CSTS_OK			0		/* �n�j								*/
#define		M_CCD_CSTS_NG			1		/* �m�f								*/
#define		M_CCD_CSTS_SUS			2		/* �r�t�r�o�d�m�c					*/

#define		M_CCD_ISTS_NONE			0		/* ����								*/
#define		M_CCD_ISTS_READY		1		/* ������							*/
#define		M_CCD_ISTS_CMPLT		2		/* �����i����j						*/
#define		M_CCD_ISTS_STOP			3		/* ���f								*/
#define		M_CCD_ISTS_SUSPEND		4		/* �T�X�y���h����					*/
#define		M_CCD_ISTS_ERR			5		/* �G���[����						*/

/*==========================================*/
/*	���									*/
/*==========================================*/
#define		M_CCD_MODE_C0_INIT		0		/* ���ʁF�������҂��i�����l�j		*/
#define		M_CCD_MODE_C1_IDLE		1		/* ���ʁF�A�C�h��					*/
#define		M_CCD_MODE_C2_KPID		2		/* ���ʁF�A�C�h���ێ�����			*/
#define		M_CCD_MODE_S0_SNDW		3		/* ���M�F���M�҂�					*/
#define		M_CCD_MODE_S1_SNDC		4		/* ���M�F���M�����҂�				*/
#define		M_CCD_MODE_S2_CSHW		5		/* ���M�F�b�r�@�g�҂�				*/
#define		M_CCD_MODE_S3_CSHK		6		/* ���M�F���M��@�b�r�g�ێ�����		*/
#define		M_CCD_MODE_S4_ITHW		7		/* ���M�F��M�ڍs���@�h�m�s�g�҂�	*/
#define		M_CCD_MODE_S5_ITLW		8		/* ���M�F��M�ڍs���@�h�m�s�k�҂�	*/
#define		M_CCD_MODE_R0_ITHW		9		/* ��M�F�h�m�s�@�g�҂�				*/
#define		M_CCD_MODE_R1_RCVC		10		/* ��M�F��M�����҂�				*/
#define		M_CCD_MODE_E0_INTE		11		/* �ُ�F�h�m�s�ُ�					*/

/*==========================================*/
/*	�|�[�g���ʎq							*/
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
/*	�ʐM��`								*/
/*==========================================*/
#if	__INTERFACE__ == IF_SPI_TEGRA
#define		M_CCD_COMM_PACKET		32		/* �P�񂠂���̑���M��				*/
#else /* __INTERFACE__ == IF_GPIO */
#define		M_CCD_COMM_PACKET		4		/* �P�񂠂���̑���M��				*/
#endif
#define		M_CCD_TRNS_CNT_MAX		1024	/* �ʐM�f�[�^�T�C�YMAX				*/
#define		M_CCD_LEN_SIZE			1		/* LEN�f�[�^��						*/
#define		M_CCD_FCC_SIZE			4		/* FCC�f�[�^��						*/
#define		M_CCD_TRNS_IF_MAX		(M_CCD_TRNS_CNT_MAX - M_CCD_FCC_SIZE)
											/* ��ʑw�Ƃ�IF�T�C�YMAX			*/

/*==========================================*/
/*	�^�C�}���ʎq							*/
/*==========================================*/
#define		M_CCD_TIM_TCLK1		0			/* No.0�F���������P					*/
#define		M_CCD_TIM_TCSH		1			/* No.1�F��������					*/
#define		M_CCD_TIM_TIDLE		2			/* No.2�F����������					*/
#define		M_CCD_TIM_TFAIL1	3			/* No.3�F�����������P				*/
#define		M_CCD_TIM_COMON		4			/* No.4�F�ėp						*/
#define		M_CCD_TIM_INTFAIL	5			/* No.5�FINT FAILSAFE				*/

#define		M_CCD_TIM_MAX		6			/* �^�C�}�ő�l						*/

/*==========================================*/
/*	�^�C�}�t���O							*/
/*==========================================*/
#define		M_CCD_TFG_TCLK1		0x00000001	/* No.0�F���������P					*/
#define		M_CCD_TFG_TCSH		0x00000002	/* No.1�F��������					*/
#define		M_CCD_TFG_TIDLE		0x00000004	/* No.2�F����������					*/
#define		M_CCD_TFG_TFAIL1	0x00000008	/* No.3�F�����������P				*/
#define		M_CCD_TFG_COMON		0x00000010	/* No.4�F�ėp						*/
#define		M_CCD_TFG_INTFAIL	0x00000020	/* No.5�FINT FAILSAFE				*/

/*==========================================*/
/*	�ėp�^�C�}��`							*/
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
/*	���[�J�[�X���b�h��`					*/
/*==========================================*/
#define		M_CCD_WINF_SND			0		/* ���M�C�x���g						*/
#define		M_CCD_WINF_RCV			1		/* ��M�C�x���g						*/
#define		M_CCD_WINF_CNT			2		/* ���[�J�[�X���b�h�C�x���g��		*/

/*==========================================*/
/*	�h�����R���O�o��						*/
/*==========================================*/
#define DRIVERECODER_CPUCOMDRIVER_NO	13			/*	CPU�ԒʐM�h���C�o�Fknl0013�F����\��� */

/*--------* �dnd of �cefine �ceclare �rection *-------------------------*/

/************************************************************************
*																		*
*			�rtruct �ceclare �rection									*
*																		*
************************************************************************/
/*==========================================*/
/*	���[�J�X���b�h���						*/
/*==========================================*/
typedef	struct _TAG_CCD_WORKER_INFO {
	struct	work_struct 	workq;			/* ���[�J�X���b�h�Ǘ����			*/
	BYTE					eve_num;		/* �N���C�x���g�ԍ�					*/
	void					*inst_p;		/* �C���X�^���X�o�b�t�@�|�C���^		*/
} ST_CCD_WORKER_INFO;


/*==========================================*/
/*	����M���								*/
/*==========================================*/
typedef	struct _TAG_CCD_TXRX_BUF {
	struct list_head	list;				/* ���X�g�Ǘ����i�W���֐��p�j		*/
	BYTE				dat[M_CCD_TRNS_CNT_MAX + 16];
											/* ����M�f�[�^(+16��̪�فj			*/
	USHORT				len;				/* �f�[�^��							*/
	BYTE				rcv_len;			/* �f�[�^����M�f�[�^				*/
	SHORT				sts;				/* �X�e�[�^�X���					*/
} ST_CCD_TXRX_BUF;

/*==========================================*/
/*	�^�C�}���								*/
/*==========================================*/
typedef	struct _TAG_CCD_TIMER_INFO {
	void				*inst_p;			/* �C���X�^���X�o�b�t�@�|�C���^		*/
    INT 				tim_num;			/* �^�C�}�ԍ�						*/
	ULONG				usec;				/* �^�C���A�E�g�l					*/
    struct hrtimer 		hndr;				/* �^�C�}�n���h��					*/
} ST_CCD_TIMER_INFO;

/*==========================================*/
/*	�b�o�t�ԒʐM�h���C�o�Ǘ��o�b�t�@		*/
/*==========================================*/
typedef	struct _TAG_CCD_MNG_INFO {
	struct {	/*---* �f�o�C�X�h���C�o��� *-----------------------------------*/
		dev_t				devt;			/* �o�^�f�o�C�X�ԍ�					*/
		struct list_head	dev_ent;		/* �f�o�C�X���X�g�o�^�p���			*/
		struct {	/*---* �r�o�h�f�o�C�X�h���C�o��� *-------------------------*/
			struct spi_device	*sdev;		/* �r�o�h�f�o�C�X����|�C���^		*/
			struct tegra_spi_device_controller_data cnfg;
											/* �r�o�h�ݒ�f�[�^					*/
			char				id_name[8];	/* �r�o�h�o�X���ʏ��				*/
			spinlock_t			lockt;		/* �r�o�h�r������					*/
		} spi;
		INT	 					users;		/* OPEN��							*/
	} dev;
	struct {	/*---* �b�o�t���\�[�X��� *-------------------------------------*/
		struct {	/*---* �f�o�h�n�|�o�h�m *-----------------------------------*/
			INT		MCLK;					/* �l�b�k�j�i�b�k�j�j				*/
			INT		MREQ;					/* �l�q�d�p�i�b�r�j					*/
			INT		SREQ;					/* �r�q�d�p�i�h�m�s�j				*/
			INT		SDAT;					/* �r�c�`�s�i�r�h�j					*/
			INT		MDAT;					/* �l�c�`�s�i�r�n�j					*/
		} pin;
		struct {	/*---* ���荞�� *-------------------------------------------*/
			INT			sreq_num;			/* �r�q�d�p�i�h�m�s�j���荞�ݔԍ�	*/
			BYTE		sreq_sts;			/* �r�q�d�p�i�h�m�s�j�M���擾���	*/
			BYTE 		sreq_selfsts;		/* �r�q�d�p�i�h�m�s�j���Ȑݒ���	*/
			BYTE		ena_sts;			/* �����݋����					*/
			ULONG		irq_flags;			/* �����ݏ�ԕۑ��t���O				*/
			ULONG		tim_res;			/* �^�C�}�����ݕۗ��t���O			*/
			spinlock_t	irq_ctl;			/* �����ݐ���r��					*/
		} irq;
		struct {	/*---* �|�[�g *---------------------------------------------*/
			BYTE		cs_sts;				/* �b�r�M���ݒ���					*/
			ktime_t		cs_tim;				/* �C���A�N�e�B�u����				*/
		} prt;
	} cpu;
	struct {	/*---* �Ǘ���� *-----------------------------------------------*/
		BYTE	previous_mode;				/* �O���[�h							*/
		BYTE	mode;						/* ���[�h							*/
		struct workqueue_struct *wkr_thrd;	/* ���[�J�[�X���b�h					*/
		ST_CCD_WORKER_INFO		wkr_inf[M_CCD_WINF_CNT];
											/* ���[�J�X���b�h�h�e���			*/
	} mng;
	struct {	/*---* �^�C�}��� *---------------------------------------------*/
		ST_CCD_TIMER_INFO		tmr_inf[M_CCD_TIM_MAX];
											/* �^�C�}���						*/
		ULONG					tcnt;		/* �ėp�^�C�}�J�E���^				*/
		ULONG					mchk;		/* ���[�h�Œ��Ď��J�E���^			*/
	} tim;
	struct {	/*---* �ʐM��� *-----------------------------------------------*/
		ST_CCD_TXRX_BUF 	tx_buf;			/* ���M�o�b�t�@						*/
		ST_CCD_TXRX_BUF 	rx_buf;			/* ��M�o�b�t�@						*/
		struct list_head 	rx_que;			/* ��M�L���[						*/
	} comm;
	struct {	/*---* �V�X�e�������� *---------------------------------------*/
		struct {	/*---* �C�x���g���� *---------------------------------------*/
			wait_queue_head_t	tx_evq;		/* ���M�҂��v�`�h�s�p�t�d			*/
			wait_queue_head_t	rx_evq;		/* ��M�҂��v�`�h�s�p�t�d			*/
			INT					tx_flg;		/* ���M�҂��t���O					*/
			INT					rx_flg;		/* ��M�҂��t���O					*/
		} event;
		struct {	/*---* �r������ *-------------------------------------------*/
			struct mutex	 	instnc;		/* �C���X�^���X�r��					*/
			struct mutex	 	wt_if;		/* �v�q�h�s�d�C���^�[�t�F�[�X�r��	*/
			struct mutex	 	rd_if;		/* �q�d�`�c�C���^�[�t�F�[�X�r��		*/
		} mutex;
	} sys;
	struct early_device		ed_inf;			/* �����N���C�x���g���				*/
	BYTE			bf_ccd_acpt_flg;		/*	��t����						*/
	BYTE			bf_ccd_resume_not_yet;	/*	Resume	������					*/
} ST_CCD_MNG_INFO;

/*--------* �dnd of �rtruct �ceclare �rection *-------------------------*/

/************************************************************************
*																		*
*			�dxternal �ceclare �rection									*
*																		*
************************************************************************/
/*--------* �dnd of �dxternal �ceclare �rection *-----------------------*/
#endif	/* __CcmPrcMn_H__ */
/****************************************************************************
*				DrvSpiCm.h	END												*
****************************************************************************/
