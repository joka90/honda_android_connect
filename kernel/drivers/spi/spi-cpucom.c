/*
 * Copyright(C) 2012-2013 FUJITSU TEN LIMITED
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
//---------------------------------------------------------------------------
// REVISION
//---------------------------------------------------------------------------
// v14 2013/02/21 �d���ϓ������s��Ή��FRe-Open�ɂĒʐM�s��
//					��ԕs�������N����
//					relase�͖������ɁAOpen�ɂđS�ăN���A����
// v13 2013/02/15 �d���ϓ������s��Ή��FRe-Open�ɂĒʐM�s��
//					fs_ccd_reinit�֐��폜
// v12 2012/12/19 ISR��INT������C��
// v11 2012/12/17 DR�w�E���f
//		�ETX�������̏�ʂւ̒ʒm�|�C���g��TIDLE��֕ύX
// v10 2012/12/17 DR�w�E���f
//		�Ewrite�֐��ł̊J���R��
//		�ERX�������̊J���R��
// v9 2012/12/16 read/write���d�Ăяo���h�~MUTEX�ǉ�
// v8 2012/12/15 INT#L����t���s��C��
// v7 2012/12/15 TLOCK�^�C�}T.O.�Ώ�
// v6 2012/12/14 re-open�Ή�
// v5 SUSPEND kernel panic �Ή�
// v3 ��e�ʑ��̃��O�ǉ� �폜 BSC����֒�
// v2 �s��v���O����
// 		��e�ʑ��̃��O�ǉ�
/****************************************************************************
*																			*
*		�V���[�Y����	: A-DA												*
*		�}�C�R������	: tegra3											*
*		�I�[�_�[����	: 													*
*		�u���b�N����	: kernel driver										*
*		DESCRIPTION		: cpu�ԒʐM�h���C�o									*
*		�t�@�C������	: spi-cpucom.c										*
*																			*
*																			*
*		�w�G���g���[�v���O�����x											*
*			fs_ccd_init ............ �h���C�o������							*
*			fs_ccd_exit ............ �h���C�o�I��							*
*			fs_ccd_probe ........... �h���C�o����							*
*			fs_ccd_remove........... �h���C�o�폜							*
*			fs_ccd_open ............ �h���C�o�I�[�v��						*
*			fs_ccd_release.......... �h���C�o�N���[�Y						*
*			fs_ccd_suspend.......... �h���C�o�T�X�y���h						*
*			fs_ccd_resume .......... �h���C�o���W���[��						*
*			fs_ccd_read............. �h���C�o���[�h							*
*			fs_ccd_write ........... �h���C�o���C�g							*
*			fs_ccd_sudden_device_poweroff..... �����I���G���g��				*
*		�w��ԑJ�ڊ֐��x													*
*			fc_ccd_fsm_common .............. ���ʏ���						*
*			fc_ccd_fsm_run ................. ���s							*
*			fc_ccd_fsm_state_rx_wait_inth_err..�f�[�^��M�G���[INT#H�҂�	*
*			fc_ccd_fsm_state_rx_data ....... �f�[�^��M������				*
*			fc_ccd_fsm_state_rx_wait_data .. �f�[�^��M�҂�������			*
*			fc_ccd_fsm_state_rx_wait_len  .. �����O�X��M�҂�������			*
*			fc_ccd_fsm_state_tx_wait_intl .. ���M���fINT#L�҂���ԏ���		*
*			fc_ccd_fsm_state_tx_wait_inth .. ���M���fINT#H�҂���ԏ���		*
*			fc_ccd_fsm_state_tx_comp ....... ���M������ԏ���				*
*			fc_ccd_fsm_state_tx_tx ......... ���M����ԏ���					*
*			fc_ccd_fsm_state_tx_wait ....... ���M�҂���ԏ���				*
*			fc_ccd_fsm_state_idle .......... �A�C�h����ԏ���				*
*			fc_ccd_fsm_state_pre_idle....... �A�C�h���O��ԏ���				*
*			fc_ccd_fsm_init ................ ����������						*
*		�wSPI����M�x														*
*			fc_ccd_spi_sync_read............ SPI��M						*
*			fc_ccd_spi_sync_write........... SPI���M						*
*			fc_ccd_spi_sync................. SPI����M						*
*			fc_ccd_spi_complete............. ��M����						*
*		�w�o�b�t�@����x													*
*			fc_ccd_buffer_destory........... �o�b�t�@�S�폜					*
*			fc_ccd_buffer_destory_rx........ ��M�o�b�t�@�폜				*
*			fc_ccd_buffer_destory_tx........ ���M�o�b�t�@�폜				*
*			fc_ccd_buffer_get_tail ......... �����o�b�t�@�擾				*
*			fc_ccd_buffer_size ............. ���X�g�̃T�C�Y�擾				*
*			fc_ccd_buffer_destoryList....... rx/tx�o�b�t�@�폜				*
*		�w�^�C�}�[�x														*
*			fc_ccd_timer_init............... �^�C�}������					*
*			fc_ccd_timer_callback........... �^�C�}�R�[���o�b�N				*
*			fc_ccd_timer_start.............. �^�C�}�X�^�[�g					*
*			fc_ccd_timer_cancel............. �^�C�}�L�����Z��				*
*		�w���̑��̃T�u���[�`���x											*
*			fc_ccd_show_adapter_name..�A�_�v�^�l�[������					*
*			fc_ccd_workThread........ ���[�J�[�X���b�h						*
*			fc_ccd_isr_int .......... ���荞�݃n���h��						*
*			fc_ccd_rx_error.......... ��M���G���[����						*
*			fc_ccd_tx_error.......... ���M�G���[����						*
*			fc_ccd_check_pre_suspend. �T�X�y���h����						*
*			fc_ccd_check_int......... INT(SREQ)#H/L�`�F�b�N					*
*			fc_ccd_gpio_request_str.. gpio�o�^�����񐶐�					*
*			fc_ccd_print_data........ ����M�f�[�^�v�����g�i�f�o�b�N�p�j	*
*			fc_ccd_setCS............. CS#����								*
****************************************************************************/
/*-----------------------------------------------------------------------------
	include
------------------------------------------------------------------------------*/
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
#define CPUCOM_MAJOR			major_no
#define N_SPI_MINORS			32	/* ... up to 256 */
#define DRV_NAME "spi-cpucom"

/*******************************/
/* for debug defiens           */
//#define CPUCOMDEBUG		
//#define CPUCOMDEBUG_PRINTDATA
//#define CPUCOMDEBUG_LOGCACHE


#define IF_GPIO			1	/* GPIO���� */
#define IF_SPI_TEGRA	2	/* SPI 		*/
#define INTERFACE		IF_GPIO

/* PIN assign */
#define CPU_COM_SREQ0	TEGRA_GPIO_PK5
#define CPU_COM_MREQ0	TEGRA_GPIO_PEE0
#define CPU_COM_MCLK0	TEGRA_GPIO_PO7
#define CPU_COM_MDT0	TEGRA_GPIO_PO5
#define CPU_COM_SDT0	TEGRA_GPIO_PO6
#define CPU_COM_SREQ1	TEGRA_GPIO_PW2
#define CPU_COM_MREQ1	TEGRA_GPIO_PEE1
#define CPU_COM_MCLK1	TEGRA_GPIO_PY2
#define CPU_COM_MDT1	TEGRA_GPIO_PY0
#define CPU_COM_SDT1	TEGRA_GPIO_PY1

#define ACTIVE		1
#define NEGATIVE	0
#define HIGH		1
#define LOW			0
#define TRUE		1
#define FALSE		0
#define ESUCCESS	0				/* return code */
#define MSG_WAIT	0				/* message que syncronize : waitting*/
#define MSG_DONE	1				/* message que syncronize : complete*/
#define CALL_TX		0x01
#define CALL_RX		0x02
#define CALL_WS		0x04
#define	CPUCOM_TX_RTRY	(16)		/* �}�X�^���M���g���C�J�E���^ */
#define	CPUCOM_FSM_LOOPMAX		50	/* FSM���[�v��� */
#define	POLLING_ON_ABORT_RX_MAX	20	/* �}�X�^��M�G���[INT��ԃ|�[�����O���g���C�J�E���^ */
#define	POLLING_INT_OBSERVE		12	/* INT#L����t�����o 5Sec�~12=60Sec		*/
#define	PRE_SUSPEND_RETVAL	0x7FFF	/* �����I�������^�[���R�[�h */
#if INTERFACE == IF_SPI_TEGRA	/* SPI-TEGRA�ł̒ʐM */
	#define	SPI_TRANSFER_UNIT		32	/* 32byte�Â��M */
#elif INTERFACE == IF_GPIO		/* GPIO����ł̒ʐM	 */
	#define	SPI_TRANSFER_UNIT		1	/* 1byte�Â��M */
#endif

#define LOCK_TX			mutex_lock(&ccd->tx_buffer_list_lock);
#define UNLOCK_TX		mutex_unlock(&ccd->tx_buffer_list_lock);
#define LOCK_RX			mutex_lock(&ccd->rx_buffer_list_lock);
#define UNLOCK_RX		mutex_unlock(&ccd->rx_buffer_list_lock);

/*--------------------------------------------------------------------------*/
/* 	macro 																	*/
/*--------------------------------------------------------------------------*/
#ifdef CPUCOMDEBUG	/* �f�o�b�N */
	#define LOGD(fmt, ...)	printk(KERN_INFO DRV_NAME pr_fmt(fmt),##__VA_ARGS__)
	#define LOG_CHANGESTATE(ccd,main,sub)	(printk(KERN_INFO DRV_NAME "%s:[change state] %s(%s) -> %s(%s)\n", ccd->id, \
											cpucom_fsm_main[ccd->cpucom_fsm.main].str,cpucom_fsm_sub[ccd->cpucom_fsm.sub].str, \
											cpucom_fsm_main[main].str,cpucom_fsm_sub[sub].str))
	#define LOGFSM(fmt, ...)	printk(KERN_INFO pr_fmt(fmt) , ##__VA_ARGS__)
	#define LOGT(fmt, ...)	printk(KERN_INFO pr_fmt(fmt) ,##__VA_ARGS__)
	#define PRINT_DATA(this,kind,data,len)	fc_ccd_print_data(this,kind,(unsigned char*)data,len);
#else
	#define LOGD(fmt, ...)
	#define LOG_CHANGESTATE(ccd,main,sub)
	#define LOGFSM(fmt, ...)
	#define LOGT(fmt, ...)	
	#define PRINT_DATA(this,kind,data,len)
#endif /* CPUCOMDEBUG */


#ifdef CPUCOMDEBUG_LOGCACHE		/* for debug */
	#undef LOGT
	#undef LOGFSM
	#undef LOGD
	#define LOGT(fmt, ...)	printk(KERN_INFO DRV_NAME pr_fmt(fmt), ##__VA_ARGS__);//ccd_cache(pr_fmt(fmt) ,##__VA_ARGS__)
//	#define LOGD(fmt, ...)	ccd_cache(pr_fmt(fmt) ,##__VA_ARGS__)
	#define LOGD(fmt, ...)	
	#define LOGFSM(fmt, ...)	ccd_cache(pr_fmt(fmt) ,##__VA_ARGS__)
//	#define LOGFSM(fmt, ...)	
//	#define LOGI(fmt, ...)	printk(KERN_INFO DRV_NAME pr_fmt(fmt), ##__VA_ARGS__);ccd_cache(pr_fmt(fmt) ,##__VA_ARGS__)
//	#define LOGW(fmt, ...)	printk(KERN_WARNING DRV_NAME pr_fmt(fmt), ##__VA_ARGS__);ccd_cache(pr_fmt(fmt) ,##__VA_ARGS__)
//	#define LOGE(fmt, ...)	printk(KERN_ERR DRV_NAME pr_fmt(fmt),##__VA_ARGS__);ccd_cache(pr_fmt(fmt) ,##__VA_ARGS__)
	#define LOGI(fmt, ...)	printk(KERN_INFO DRV_NAME pr_fmt(fmt), ##__VA_ARGS__);
	#define LOGW(fmt, ...)	printk(KERN_WARNING DRV_NAME pr_fmt(fmt), ##__VA_ARGS__);
	#define LOGE(fmt, ...)	printk(KERN_ERR DRV_NAME pr_fmt(fmt),##__VA_ARGS__);
#else
	#define LOGI(fmt, ...)	ADA_LOG_INFO(DRV_NAME, pr_fmt(fmt), ##__VA_ARGS__)
	#define LOGW(fmt, ...)	ADA_LOG_WARNING(DRV_NAME, pr_fmt(fmt), ##__VA_ARGS__)
	#define LOGE(fmt, ...)	ADA_LOG_ERR(DRV_NAME, pr_fmt(fmt),##__VA_ARGS__)
#endif

#define UNUSED(x) (void)(x)
#define MS2NS(x)	(x*1000*1000)
#define US2NS(x)	(x*1000)
#define MS2US(x)	(x*1000)
#define US2SEC(x)	(x/1000000)
#define SEC2US(x)	(x*1000000)

#define RANGE(min,x,max) ((min<x && x<max)?1:0)
#define ID_DEFINE(x) x , #x

#define SIZE_OF_FCC				4
#define SIZE_OF_TOTAL(x)		(x*4+1+SIZE_OF_FCC)
#define SIZE_OF_EXCLUDE_FCC(x)	(x*4+1)
#define CLK_DELAY				__delay(700)//__udelay(0)
/*--------------------------------------------------------------------------*/
/* 	grobal value															*/
/*--------------------------------------------------------------------------*/
static DECLARE_BITMAP(minors, N_SPI_MINORS);
static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);
static struct class *cpucom_class;
static int major_no=0;


/*---------------------------------------------------------*/
/*  �����N���Ή� - CPU�ԒʐM�h���C�o�����t�t���O   -     */
/*          TRUE  : �����t                               */
/*          FALSE : ���싑��                               */
/*---------------------------------------------------------*/
static bool	accept_flg=FALSE;

#if defined(CPUCOMDEBUG_PRINTDATA) || defined(CPUCOMDEBUG_LOGCACHE)
#define CASHE_SIZE		1024*384
static char* pCashe=NULL;
static char* pCashe_current=NULL;
#endif

#ifdef CPUCOMDEBUG_PRINTDATA
#define CASHE_WATERMARK	((CASHE_SIZE/10)*9)	/* 90% */
#define PRINT_DATA(this,kind,data,len)	fc_ccd_print_data(this,kind,(unsigned char*)data,len);
#endif

#ifdef CPUCOMDEBUG_LOGCACHE		/* for debug */
static char	CasheLock =FALSE;
#endif


/*--------------------------------------------------------------------------*/
/* 	enum 																	*/
/*--------------------------------------------------------------------------*/
/*== �^�C�}ID ==*/
enum timer_id{
	TID_TCLK =0,					/* T1(tclk:2ms)					*/
	TID_TCSH,                       /* T2(tcsh�F1ms)				*/
	TID_TCLK2,                      /* T3(tclk2�F2ms)				*/
	TID_TCLK3,                      /* T4(tclk3�F2ms)				*/
	TID_TLOCK,                      /* T5(tlock�F5s)				*/
	TID_ABORT_TX,                   /* T6(tlock�F10s)				*/
	TID_TFAIL_MAX,                  /* T7(tfail����l�F100ms)		*/
	TID_INT_POLLING_ON_IDLE,        /* T8(�K�莞�Ԍo��[tlock�ȓ�])	*/
	TID_INT_POLLING_ON_ABORT_RX,    /* T9(INT#����|�[�����O)		*/
	TID_TIDLE,  					/* T10(CS HIGH�ێ�����)			*/
	TID_INT_OBSERVE,				/* T11(INT#L����t�����o)		*/
	TID_MAX
};
/*== ���b�Z�[�WID ==*/
enum worker_message_type{
	WMID_INTERUPT_INT_ACTIVE=0,			/* 0 */
	WMID_INTERUPT_INT_NEGATIVE,			/* 1 */
	WMID_INTERUPT_RX_COMP,				/* 2 */
//WMID_INTERUPT_RX_ERR,
//	WMID_INTERUPT_TX_COMP,
//WMID_INTERUPT_TX_EMP,
//WMID_INTERUPT_TX_ERR,
	
	WMID_TIMER_BEGIN,					/* 3 */
	WMID_TIMER_TCLK=WMID_TIMER_BEGIN,	/* 3 */
	WMID_TIMER_TCSH,					/* 4 */
	WMID_TIMER_TCLK2,					/* 5 */
	WMID_TIMER_TCLK3,					/* 6 */
	WMID_TIMER_TLOCK,					/* 7 */
	WMID_TIMER_ABORT_TX,				/* 8 */
	WMID_TIMER_TFAIL_MAX,				/* 9 */
	WMID_TIMER_INT_POLLING_ON_IDLE,		/* 10 */
	WMID_TIMER_INT_POLLING_ON_ABORT_RX,	/* 11 */
	WMID_TIDLE,							/* 12 */
	WMID_INT_OBSERVE,					/* 13 */
	
	WMID_TX,							/* 14 */
	WMID_MAX
};
/*== ���C���X�e�[�g ==*/
enum cpucom_fsm_mainstate{
	CPUCOM_STATE_NONE = 0,	/* 0 */
	CPUCOM_STATE_IDLE,		/* 1 */
	CPUCOM_STATE_TX,		/* 2 */
	CPUCOM_STATE_RX,		/* 3 */
	CPUCOM_STATE_SUSPEND,	/* 4 */
	CPUCOM_STATE_RESUME,	/* 5 */
	CPUCOM_STATE_SLAVE_ERROR,/* 6 */
	CPUCOM_STATE_MAX
};
/*== �T�u�X�e�[�g ==*/
enum cpucom_fsm_substate{
	CPUCOM_SUB_STATE_NONE=0,
	CPUCOM_SUB_STATE_IDLE,				/* (1)	S1:�}�X�^�[�f�[�^���M�҂����		*/
	CPUCOM_SUB_STATE_WAIT_TX,			/* (2)	S2:�}�X�^�[�f�[�^���M���			*/
	CPUCOM_SUB_STATE_TX,				/* (3)	S2:�}�X�^�[�f�[�^���M�������		*/
	CPUCOM_SUB_STATE_TX_COMP,			/* (4)	S3:���M���~�ɂ��INT#H���A�҂����	*/
	CPUCOM_SUB_STATE_WAIT_INTH, 		/* (5)	S4:�X���[�u�đ�INT#L���A�҂����	*/
	CPUCOM_SUB_STATE_WAIT_INTL, 		/* (6)	R1:�X���[�uLEN�t���[����M�҂����	*/

	CPUCOM_SUB_STATE_WAIT_RX_LEN,		/* (7)	R2:�X���[�u�f�[�^��M�҂����		*/
	CPUCOM_SUB_STATE_WAIT_RX_DATA, 		/* (8)	R3:�X���[�u�f�[�^�t���[����M���	*/
	CPUCOM_SUB_STATE_RX_DATA,			/* (9)	E1:INT#H���A�҂����				*/
	CPUCOM_SUB_STATE_WAIT_INTH_ERR, 	/* (10)					*/

	CPUCOM_SUB_STATE_MAX
};
/*== ���^�[���l�F��ԊǗ� ==*/
enum cpucom_fsm_retcode{
	CPUCOM_RETCODE_NONE = 0,
	CPUCOM_RETCODE_DONE,
	CPUCOM_RETCODE_NOTOPERATION,
	CPUCOM_RETCODE_COMMON,
	CPUCOM_RETCODE_RERUN,
	CPUCOM_RETCODE_MAX
};
/*== ���^�[���l�F����M ==*/
enum cpucom_txrx_retcode{
	CPUCOM_RET_NONE			= 0,
	CPUCOM_RET_COMP			= 1,
	CPUCOM_RET_ERR			=-1,
	CPUCOM_RET_STOP			=-2,
	CPUCOM_RET_PRE_SUSPEND	=-4
};

/*--------------------------------------------------------------------------*/
/* 	struct 																	*/
/*--------------------------------------------------------------------------*/
/*------------------------------------*/
/*== �e�[�u�� 						==*/
/*------------------------------------*/
/* �^�C�} */
typedef	struct{
	enum timer_id	id;
	char			str[64];
	unsigned long	us;
}ST_CCD_TIMER_CONFIG;

/* �X�e�[�g�}�V��-���C�� */
typedef	struct{
	enum cpucom_fsm_mainstate state;
	char			str[64];
}ST_CCD_CPUCOM_FSM_MAIN;

/* �X�e�[�g�}�V��-�T�u */
typedef	struct{
	enum cpucom_fsm_substate state;
	char			str[64];
}ST_CCD_CPUCOM_FSM_SUB;

/* ���b�Z�[�W */
typedef	struct{
	enum worker_message_type msg;
	char			str[64];
}ST_CCD_CPUCOM_MESSAGE;
/*------------------------------------*/
/*== �^�C�} 						==*/
/*------------------------------------*/
typedef	struct{
	void*					pthis;
    int 					index;
	ST_CCD_TIMER_CONFIG		config;
    struct hrtimer 			timer;
}ST_CCD_TIMER_SET;

/*------------------------------------*/
/*== �C�x���g 						==*/
/*------------------------------------*/
typedef	struct{
	struct	work_struct 			workq;
	enum worker_message_type		Msgtype;
	void*							pthis;
}ST_CCD_WORKER_INFO;

/*------------------------------------*/
/*== �o�b�t�@ 						==*/
/*------------------------------------*/
typedef	struct{
	struct list_head list;
	char*			buf;
	unsigned short	length;
	unsigned short	offset;
	enum cpucom_txrx_retcode	err;
}ST_CCD_BUFFER;

/*== ��ԊǗ� ==*/
typedef	struct{
	enum cpucom_fsm_mainstate	main;
	enum cpucom_fsm_substate	sub;
}ST_CCD_CPUCOM_FSM;


/*------------------------------------*/
/*== instance 						==*/
/*------------------------------------*/
typedef	struct{
	dev_t			devt;
	spinlock_t		spi_lock;
	struct spi_device	*spi;
	struct list_head	device_entry;
	struct tegra_spi_device_controller_data cdata;

	ST_CCD_BUFFER* 		tx_buffer;
	ST_CCD_BUFFER* 		rx_buffer;
	struct list_head 	rx_buffer_list;

	struct mutex	 	rx_buffer_list_lock;
	struct mutex	 	tx_buffer_list_lock;
	struct mutex	 	write_interface_lock;
	struct mutex	 	read_interface_lock;
	struct mutex	 	workerthread_lock;

	wait_queue_head_t	txDataQue;
	wait_queue_head_t	rxDataQue;
	bool				rx_complete;
	bool				tx_complete;

	struct workqueue_struct 	*workqueue;
	ST_CCD_WORKER_INFO*	worker_info[WMID_MAX];
	
	ST_CCD_TIMER_SET	timer_set[TID_MAX];
	char				id[8];
	struct _pin{
		int		MCLK;	/* CLK  */
		int		MREQ;	/* CS   */
		int		SREQ;	/* INT  */
		int		SDAT;	/* MISO */
		int		MDAT;	/* MOSI */
	}pin;
	int irq;
	ST_CCD_CPUCOM_FSM cpucom_fsm;
	ST_CCD_CPUCOM_FSM pre_state;
	struct early_device ed;
	int users;
	int polling_on_abort_rx_cnt;
	int polling_int_observe_cnt;
	bool irq_status;
	bool disable_if;
	int calling;
	int wc;
#ifdef CPUCOMDEBUG_PRINTDATA
	unsigned long command_sn;/* for debug */
#endif
}ST_CCD_CPUCOM_DATA;

/*--------------------------------------------------------------------------*/
/* 	table 																	*/
/*--------------------------------------------------------------------------*/
/*------------------------------------*/
/*== �^�C�}��`�e�[�u�� 			==*/
/*------------------------------------*/
static const ST_CCD_TIMER_CONFIG timer_table[]={		 	 /*								min typ	max	(ms)	*/
	{ID_DEFINE(TID_TCLK)					,MS2US(2)		},/* T1(tclk :2ms)				2	-	-			*/
	{ID_DEFINE(TID_TCSH)					,MS2US(1)		},/* T2(tcsh :1ms)				1	-	3			*/
	{ID_DEFINE(TID_TCLK2)					,0				},/* T3(tclk2:1ms)				-	0	2			*/
	{ID_DEFINE(TID_TCLK3)					,0				},/* T4(tclk3:1ms)				-	0	2			*/
	{ID_DEFINE(TID_TLOCK)					,MS2US(5000)	},/* T5(tlock:5s )				5S	-	-			*/
	{ID_DEFINE(TID_ABORT_TX)				,MS2US(10000)	},/* T6(tlock:10s)				10S	-	-			*/
	{ID_DEFINE(TID_TFAIL_MAX)				,MS2US(100)		},/* T7(tfail����l�F100ms)		2	-	100			*/
	{ID_DEFINE(TID_INT_POLLING_ON_IDLE)		,MS2US(5000)	},/* T8(�K�莞�Ԍo��[tlock�ȓ�])*/
	{ID_DEFINE(TID_INT_POLLING_ON_ABORT_RX)	,MS2US(100)		},/* T9(INT#����|�[�����O)		*/
	{ID_DEFINE(TID_TIDLE)					,MS2US(3)		},/* T10(CS H�ێ�����)			3	-	-			*/
	{ID_DEFINE(TID_INT_OBSERVE)				,MS2US(5000)	} /* T11(IN#L�Ď�)*/
};
///�^�C�}0�̎���Replace���Ē�Call����悤�ɂ���	�ǂ�ʃC���p�N�g�H

/*------------------------------------*/
/*== ���C����ԃe�[�u���FforDebug 	==*/
/*------------------------------------*/
static const ST_CCD_CPUCOM_FSM_MAIN cpucom_fsm_main[]={
	{ID_DEFINE(CPUCOM_STATE_NONE)},
	{ID_DEFINE(CPUCOM_STATE_IDLE)},
	{ID_DEFINE(CPUCOM_STATE_TX)},
	{ID_DEFINE(CPUCOM_STATE_RX)},
	{ID_DEFINE(CPUCOM_STATE_SUSPEND)},
	{ID_DEFINE(CPUCOM_STATE_RESUME)},
	{ID_DEFINE(CPUCOM_STATE_SLAVE_ERROR)},
	{ID_DEFINE(CPUCOM_STATE_MAX)}
};
/*------------------------------------*/
/*== �T�u��ԃe�[�u���FforDebug 	==*/
/*------------------------------------*/
static const ST_CCD_CPUCOM_FSM_SUB cpucom_fsm_sub[]={
	{ID_DEFINE(CPUCOM_SUB_STATE_NONE)},
	{ID_DEFINE(CPUCOM_SUB_STATE_IDLE)},
	{ID_DEFINE(CPUCOM_SUB_STATE_WAIT_TX)},
	{ID_DEFINE(CPUCOM_SUB_STATE_TX)},
	{ID_DEFINE(CPUCOM_SUB_STATE_TX_COMP)},
	{ID_DEFINE(CPUCOM_SUB_STATE_WAIT_INTH)},
	{ID_DEFINE(CPUCOM_SUB_STATE_WAIT_INTL)},
	{ID_DEFINE(CPUCOM_SUB_STATE_WAIT_RX_LEN)},
	{ID_DEFINE(CPUCOM_SUB_STATE_WAIT_RX_DATA)},
	{ID_DEFINE(CPUCOM_SUB_STATE_RX_DATA)},
	{ID_DEFINE(CPUCOM_SUB_STATE_WAIT_INTH_ERR)},
	{ID_DEFINE(CPUCOM_SUB_STATE_MAX)}
};
/*------------------------------------*/
/*== ���b�Z�[�W�e�[�u���FforDebug   ==*/
/*------------------------------------*/
static const ST_CCD_CPUCOM_MESSAGE cpucom_message[]={
	{ID_DEFINE(WMID_INTERUPT_INT_ACTIVE)},
	{ID_DEFINE(WMID_INTERUPT_INT_NEGATIVE)},
	{ID_DEFINE(WMID_INTERUPT_RX_COMP)},
//	{ID_DEFINE(WMID_INTERUPT_RX_ERR)},
//	{ID_DEFINE(WMID_INTERUPT_TX_COMP)},
//	{ID_DEFINE(WMID_INTERUPT_TX_EMP)},
//	{ID_DEFINE(WMID_INTERUPT_TX_ERR)},
	{ID_DEFINE(WMID_TIMER_TCLK)},
	{ID_DEFINE(WMID_TIMER_TCSH)},
	{ID_DEFINE(WMID_TIMER_TCLK2)},
	{ID_DEFINE(WMID_TIMER_TCLK3)},
	{ID_DEFINE(WMID_TIMER_TLOCK)},
	{ID_DEFINE(WMID_TIMER_ABORT_TX)},
	{ID_DEFINE(WMID_TIMER_TFAIL_MAX)},
	{ID_DEFINE(WMID_TIMER_INT_POLLING_ON_IDLE)},
	{ID_DEFINE(WMID_TIMER_INT_POLLING_ON_ABORT_RX)},
	{ID_DEFINE(WMID_TIDLE)},
	{ID_DEFINE(WMID_INT_OBSERVE)},
	{ID_DEFINE(WMID_TX)},
	{ID_DEFINE(WMID_MAX)}
};


#ifdef CPUCOMDEBUG_LOGCACHE		/* for debug */
static void ccd_cache_out(){
	char* p;
	CasheLock =TRUE;
	printk(KERN_INFO "== BEGIN CACHE OUT ===========================================================================\n");
	for(p=pCashe;p!=pCashe_current;++p){
		printk("%c",*p);
		*p ='\0';
	}
	printk("@@@@@@ current point @@@@@@@\n");
	for(;p!=&pCashe[CASHE_SIZE];++p){
		printk("%c",*p);
		*p ='\0';
	}
	pCashe_current = pCashe;
	printk(KERN_INFO "== END CACHE OUT ===========================================================================\n");
	CasheLock =FALSE;
	
}
static void ccd_cache(char* format, ...){
	if(!CasheLock){
		char buf[1024]={0};
		char returncose[2]={'\n','\0'};
	    va_list arg;

	    va_start(arg, format);
	    vsprintf(buf, format, arg);
	    va_end(arg);
	    if(pCashe
	    && pCashe_current){
			if(pCashe+CASHE_SIZE <= pCashe_current+strlen(buf)){
				//ccd_cache_out();
				pCashe_current=pCashe;
			}
			*pCashe_current ='\0';
			strcat(pCashe_current,buf);
			pCashe_current+=strlen(buf);
			
			*pCashe_current ='\0';
			strcat(pCashe_current,returncose);
			pCashe_current+=strlen(returncose);
		}
	}
}
#endif

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_setCS											*
*																			*
*		DESCRIPTION	: CS#����												*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : active (ACTIVE or NEGATIVE)						*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline void fc_ccd_setCS(ST_CCD_CPUCOM_DATA* ccd,bool active)
{

	if(active)	gpio_set_value(ccd->pin.MREQ, LOW);
	else		gpio_set_value(ccd->pin.MREQ, HIGH);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_calc_FFC										*
*																			*
*		DESCRIPTION	: CS#����												*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : buf  : �o�b�t�@									*
*																			*
*					  IN  : len  : �����O�X									*
*																			*
*					  RET : FCC�l											*
*																			*
****************************************************************************/
static inline unsigned long fc_ccd_calc_fcc(ST_CCD_CPUCOM_DATA* ccd,char* buf,unsigned long len)
{
	unsigned long result= 0;
	int i;
	for( i = 0; i < len; i++ )
		result += buf[i];
	return result;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_timer_cancel									*
*																			*
*		DESCRIPTION	: �^�C�}�[�L�����Z��									*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : �^�C�}ID										*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ccd_timer_cancel(ST_CCD_CPUCOM_DATA* ccd,enum timer_id id)
{
	hrtimer_cancel(&ccd->timer_set[id].timer);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_timer_start									*
*																			*
*		DESCRIPTION	: �^�C�}�[�X�^�[�g										*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : �^�C�}ID										*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ccd_timer_start(ST_CCD_CPUCOM_DATA* ccd,enum timer_id id)
{
	ktime_t time;
	if(ccd->timer_set[id].config.us < SEC2US(1))	/* 1�b���� */
		time = ktime_set( 0, US2NS(ccd->timer_set[id].config.us));
	else											/* 1�b�ȏ� */
		time = ktime_set( US2SEC(ccd->timer_set[id].config.us), 0);
	hrtimer_start(&ccd->timer_set[id].timer,time , HRTIMER_MODE_REL);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_timer_callback									*
*																			*
*		DESCRIPTION	: �^�C�}�[�R�[���o�b�N									*
*																			*
*		PARAMETER	: IN  : *timer : �^�C�}�f�[�^							*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static enum hrtimer_restart fc_ccd_timer_callback(struct hrtimer *timer)
{
	enum hrtimer_restart ret= HRTIMER_NORESTART;
	ST_CCD_TIMER_SET* ts;
	
	ts = container_of(timer, ST_CCD_TIMER_SET, timer);
	if(ts){
		ST_CCD_CPUCOM_DATA* ccd=(ST_CCD_CPUCOM_DATA*)ts->pthis;
		/*--------------------------------------*/
		/* �^�C�}ID�ɑΉ��������b�Z�[�W�𑗐M	*/
		/*--------------------------------------*/
		queue_work(ccd->workqueue, (struct work_struct*)ccd->worker_info[WMID_TIMER_BEGIN+ts->config.id]);
	}else
		printk("%s(%d) : cannot get object",__FUNCTION__,__LINE__);
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_timer_init										*
*																			*
*		DESCRIPTION	: �^�C�}������											*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static int fc_ccd_timer_init(ST_CCD_CPUCOM_DATA* ccd)
{
	int status=ESUCCESS;
	int i;
	LOGD("%s: %s",ccd->id,__FUNCTION__);
	fc_ccd_setCS(ccd,NEGATIVE);
	for(i=0;i<ARRAY_SIZE(ccd->timer_set);++i){
		ccd->timer_set[i].pthis = (void*)ccd;
		ccd->timer_set[i].index = i;
		ccd->timer_set[i].config.id=timer_table[i].id;
		ccd->timer_set[i].config.us=timer_table[i].us;
		hrtimer_init(&ccd->timer_set[i].timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ccd->timer_set[i].timer.function = fc_ccd_timer_callback;
	}
	return status;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_print_data										*
*																			*
*		DESCRIPTION	: ����M�f�[�^�v�����g�i�f�o�b�N�p�j					*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : 1 = ��M	0 = ���M							*
*																			*
*					  IN  : *dat : �o�͕�����								*
*																			*
*					  IN  : len  : �o�̓����O�X								*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
#if 1
//defined(CPUCOMDEBUG) || defined(CPUCOMDEBUG_PRINTDATA)
static void fc_ccd_print_data(ST_CCD_CPUCOM_DATA *ccd, unsigned char kind, unsigned char* dat, unsigned short len )
{
#define ARTWORK
#ifdef ARTWORK
	if(0 == ccd->spi->master->bus_num){
		char	str[1024*2+128];
		short	i;
		for( i=0; i<len; i++ )
			sprintf( &str[i*2], "%02X", dat[i] );
		str[i*2] = '\0';
		LOGI("%s[%d]: %s", ccd->id,len,str );
	}
#else
	char	str[256];
	short	i;
	if(len > 128)
		len = 128;
	sprintf( str, "%d ",kind );
	for( i=0; i<len; i++ )
		sprintf( &str[2+i*3], "%02X ", dat[i] );
	str[(2+i*3)-1] = '\0';
#endif
#ifdef CPUCOMDEBUG					/* �f�o�b�N */
	LOGD("%s: %s", ccd->id,str );
#elif defined CPUCOMDEBUG_PRINTDATA			/* �f�o�b�N on cashe */
	if(pCashe){
		static bool lock=FALSE;
		char	strsn[32]={0};
		sprintf( strsn, "[%08d]", (int)++ccd->command_sn );
		if(FALSE == lock){
			if(pCashe+CASHE_WATERMARK <= pCashe_current){
				char* p=pCashe;
				lock=TRUE;
				/* printout */
				printk("\n== BEGIN ================================================================\n");
				LOGI("%s: CASHE PRINT OUT\n", ccd->id);
				++pCashe_current;
				do{
					printk("%c",*p);
					*p='\0';
				}while(++p!=pCashe_current);
				pCashe_current=pCashe;
				lock=FALSE;
				printk("%s%s",strsn,str);
				printk("\n=== END   ===============================================================\n");
			}else{
				strncpy(pCashe_current,strsn,strlen(strsn));
				pCashe_current+=strlen(strsn);

				strcpy(pCashe_current,str);
				pCashe_current+=strlen(str);
				*pCashe_current++ = '\n';
			}
		}
	}else
		LOGI("pCashe is NULL");
#endif
}
#endif
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_gpio_request_str								*
*																			*
*		DESCRIPTION	: gpio�o�^�����񐶐�									*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  OUT : *out : �A���f�[�^�i�[��							*
*																			*
*					  IN  : *str : ���j�[�N������							*
*																			*
*					  RET : ACTIVE	/ NEGATIVE								*
*																			*
****************************************************************************/
static inline char* fc_ccd_gpio_request_str(ST_CCD_CPUCOM_DATA *ccd,char* out,char* str)
{
	sprintf(out,"%s%s%s",DRV_NAME,ccd->id,str);
	return out;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_check_int										*
*																			*
*		DESCRIPTION	: INT(SREQ)#H/L�`�F�b�N									*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  RET : ACTIVE	/ NEGATIVE								*
*																			*
****************************************************************************/
static inline int fc_ccd_check_int(ST_CCD_CPUCOM_DATA* ccd)
{
	int ret = NEGATIVE;
	if(LOW == gpio_get_value(ccd->pin.SREQ))
		ret = ACTIVE;
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_check_irqflag									*
*																			*
*		DESCRIPTION	: �T�X�y���h����i�����N���Ή��j						*
*																			*
*		PARAMETER	: IN  : NONE											*
*																			*
*					  RET : TRUE[SUSPEND��]	/ FALSE[SUSPEND���łȂ�]		*
*																			*
****************************************************************************/
int fc_ccd_check_irqflag(bool flag, void* param) {
	int retval = -EINTR;
	if (!flag) {
		retval = 0;
	}
	return retval;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_check_pre_suspend								*
*																			*
*		DESCRIPTION	: �T�X�y���h����i�����N���Ή��j						*
*																			*
*		PARAMETER	: IN  : NONE											*
*																			*
*					  RET : TRUE[SUSPEND��]	/ FALSE[SUSPEND���łȂ�]		*
*																			*
****************************************************************************/
static inline bool fc_ccd_check_pre_suspend(void)
{
	int retval = 0;
	/*---------------------------------------------------------*/
	/*  BU-DET/ACC���荞�݃`�F�b�N                             */
	/*    BU-DET ���荞�݂���re-init���O�܂ł𔻒�             */
	/*---------------------------------------------------------*/
// occured panic
	retval = early_device_invoke_with_flag_irqsave(
		RNF_BUDET_ACCOFF_INTERRUPT_MASK, fc_ccd_check_irqflag, NULL);
//	if(-EINTR == retval){
//		LOGT("Budet Interrupted occured.");
//	}
	/*---------------------------------------------------------*/
	/*  ��t���� or BU-DET/ACC���荞�݂������Ă���             */
	/*---------------------------------------------------------*/
	return ((FALSE == accept_flg) | (-EINTR == retval));
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_sub_IsList											*
*																			*
*		DESCRIPTION	: ���X�g���ɑ��݃`�F�b�N								*
*																			*
*		PARAMETER	: IN  : *l   :  ���X�g�i��M or ���M�j					*
*																			*
*					  RET : �T�C�Y											*
*																			*
****************************************************************************/
static inline bool fc_ccd_buffer_islist(ST_CCD_BUFFER* target,struct list_head* l)
{
	bool ret=FALSE;
	if(!(target->list.next == LIST_POISON1 && target->list.prev == LIST_POISON2) 
	&& !list_empty(&target->list)){
		ST_CCD_BUFFER* r=NULL;
		list_for_each_entry(r,l,list) {
			if(r == target){
				ret = TRUE;
				break;
			}
		}
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_buffer_destory									*
*																			*
*		DESCRIPTION	: rx/tx�o�b�t�@�폜										*
*																			*
*		PARAMETER	: IN  : *target  :  ���X�g�i��M or ���M�j				*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline void fc_ccd_buffer_destory(ST_CCD_BUFFER* target)
{
	if(target){
		if(target->buf){
			kfree(target->buf);
			target->buf=NULL;
		}
		kfree(target);
	}
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_buffer_destoryList								*
*																			*
*		DESCRIPTION	: rx/tx�o�b�t�@�폜										*
*																			*
*		PARAMETER	: IN  : *target  :  ���X�g�i��M or ���M�j				*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline void fc_ccd_buffer_destoryList(ST_CCD_BUFFER* target,struct list_head* l)
{
	if(target){
		if(fc_ccd_buffer_islist(target,l)){
			list_del(&target->list);
			fc_ccd_buffer_destory(target);
		}
	}
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_buffer_size									*
*																			*
*		DESCRIPTION	: ���X�g�̃T�C�Y�擾									*
*																			*
*		PARAMETER	: IN  : *l   :  ���X�g�i��M or ���M�j					*
*																			*
*					  RET : �T�C�Y											*
*																			*
****************************************************************************/
static inline int fc_ccd_buffer_size(struct list_head* l)
{
	int ret=0;
	ST_CCD_BUFFER* b=NULL;
	if(!list_empty(l)){
		list_for_each_entry(b,l,list) {
			++ret;
			if(list_is_last(&b->list,l))
				break;
		}
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_buffer_get_tail								*
*																			*
*		DESCRIPTION	: �����o�b�t�@�擾										*
*																			*
*		PARAMETER	: IN  : *ccd : 	�R���e�L�X�g							*
*																			*
*					  IN  : *l   :  ���X�g�i��M or ���M�j					*
*																			*
*					  RET : ST_CCD_BUFFER* : �o�b�t�@					*
*																			*
****************************************************************************/
static ST_CCD_BUFFER* fc_ccd_buffer_get_tail(ST_CCD_CPUCOM_DATA *ccd,struct list_head* l)
{
	ST_CCD_BUFFER* ret=NULL;
	if(!list_empty(l)){
		list_for_each_entry(ret,l,list) {
			if(list_is_last(&ret->list,l))
				break;
		}
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_buffer_destory_tx								*
*																			*
*		DESCRIPTION	: ��M�o�b�t�@�폜										*
*																			*
*		PARAMETER	: IN  : *ccd : 	�R���e�L�X�g							*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ccd_buffer_destory_tx(ST_CCD_CPUCOM_DATA* ccd){
	LOCK_TX{
		fc_ccd_buffer_destory(ccd->tx_buffer);
		ccd->tx_buffer = NULL;
	}UNLOCK_TX
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_buffer_destory_tx								*
*																			*
*		DESCRIPTION	: ��M�o�b�t�@�폜										*
*																			*
*		PARAMETER	: IN  : *ccd : 	�R���e�L�X�g							*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ccd_buffer_destory_rx(ST_CCD_CPUCOM_DATA* ccd){
	LOCK_RX{
		fc_ccd_buffer_destory(ccd->rx_buffer);
		ccd->rx_buffer = NULL;
	}UNLOCK_RX
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_buffer_destory_rx								*
*																			*
*		DESCRIPTION	: ��M�o�b�t�@�폜										*
*																			*
*		PARAMETER	: IN  : *ccd : 	�R���e�L�X�g							*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ccd_buffer_destory_rx_list(ST_CCD_CPUCOM_DATA* ccd)
{
	unsigned int limit;
	LOCK_RX{
		for(limit = 0xFF ;limit; --limit){			/* ���255�ݒ� */
			if(!list_empty(&ccd->rx_buffer_list)){
				ST_CCD_BUFFER* b=list_first_entry(&ccd->rx_buffer_list,ST_CCD_BUFFER,list);
				fc_ccd_buffer_destoryList(b,&ccd->rx_buffer_list);	/* �o�b�t�@�폜		*/
			}else
				break;
			}
	}UNLOCK_RX
	fc_ccd_buffer_destory_rx(ccd);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_buffer_destory_all								*
*																			*
*		DESCRIPTION	: �o�b�t�@�S�폜										*
*																			*
*		PARAMETER	: IN  : *ccd : 	�R���e�L�X�g							*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ccd_buffer_destory_all(ST_CCD_CPUCOM_DATA* ccd)
{
	fc_ccd_buffer_destory_tx(ccd);
	fc_ccd_buffer_destory_rx_list(ccd);
}
/****************************************************************************/
/* 	SYMBOL		: 	 											*/
/* 	DESCRIPTION	: 			 								*/
/*  PARAMETER	: [IN]ccd	: context										*/
/*  RETVAL		: none														*/
/****************************************************************************/
static inline void fc_ccd_wakeup_rx(ST_CCD_CPUCOM_DATA* ccd)
{
	/* read �҂���Ԃ̏ꍇ��������V�O�i�����M*/
	LOCK_RX{
		if(MSG_WAIT == ccd->rx_complete){
			ccd->rx_complete = MSG_DONE;
			wake_up_interruptible(&ccd->rxDataQue);
		}
	}UNLOCK_RX
}
/****************************************************************************/
/* 	SYMBOL		: 	 											*/
/* 	DESCRIPTION	: 			 								*/
/*  PARAMETER	: [IN]ccd	: context										*/
/*  RETVAL		: none														*/
/****************************************************************************/
static inline void fc_ccd_wakeup_tx(ST_CCD_CPUCOM_DATA* ccd)
{
///�G���[�E�R�[�h�́H�H
	if(ccd->tx_buffer){
		bool waitting=FALSE;
		LOCK_TX{
			if(MSG_WAIT == ccd->tx_complete)
				waitting=TRUE;
			
		}UNLOCK_TX
		if(waitting){
			ccd->tx_complete = MSG_DONE;
			wake_up_interruptible(&ccd->txDataQue);
		}else{
			LOGW("tx_buffer exit but not waiiting");
		}
	}
}
/****************************************************************************/
/* 	SYMBOL		: 	 											*/
/* 	DESCRIPTION	: 			 								*/
/*  PARAMETER	: [IN]ccd	: context										*/
/*  RETVAL		: none														*/
/****************************************************************************/
static inline void fc_ccd_wakeup_rxtx(ST_CCD_CPUCOM_DATA* ccd)
{
	fc_ccd_wakeup_rx(ccd);
	fc_ccd_wakeup_tx(ccd);
}

/****************************************************************************/
/* 	SYMBOL		: 	 											*/
/* 	DESCRIPTION	: 			 								*/
/*  PARAMETER	: [IN]ccd	: context										*/
/*  RETVAL		: none														*/
/****************************************************************************/
static inline void fc_ccd_cleanup(ST_CCD_CPUCOM_DATA* ccd)
{
	unsigned int loop=0;
	ccd->disable_if=TRUE;
	/*-----------------------------------------*/
	/*  ���쒆�Ȃ�΁A�������甲����           */
	/*-----------------------------------------*/
	if(ccd->calling){
		/*-----------------------------------------*/
		/*  �u���b�L���O����                       */
		/*-----------------------------------------*/
		if(ccd->tx_buffer)
			ccd->tx_buffer->err = CPUCOM_RET_ERR;
		fc_ccd_wakeup_rxtx(ccd);
		/*-----------------------------------------*/
		/*  �I���҂�		                       */
		/*-----------------------------------------*/
		for(loop=0xFFFF;loop;--loop){
			if(ccd->calling){
				yield();/* schesule�֏�����Ԃ� */
				__udelay(1);
				LOGD("Zz");
			}else
				break;
		}
	}
	/*-----------------------------------------*/
	/*  ���������Ă�����J������               */
	/*-----------------------------------------*/
	if(0 == ccd->calling){
		fc_ccd_buffer_destory_all(ccd);
	}else
		LOGT("cleanup remain (%d)",ccd->calling);
	ccd->disable_if=FALSE;
}
/****************************************************************************/
/* 	SYMBOL		: 	 											*/
/* 	DESCRIPTION	: 			 								*/
/*  PARAMETER	: [IN]ccd	: context										*/
/*  RETVAL		: none														*/
/****************************************************************************/
static inline void fc_ccd_do_release(ST_CCD_CPUCOM_DATA *ccd)
{
	fc_ccd_cleanup(ccd);
	ccd->polling_on_abort_rx_cnt=0;
	ccd->polling_int_observe_cnt=0;
	ccd->rx_complete = MSG_DONE;
	ccd->tx_complete = MSG_DONE;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_spi_complete									*
*																			*
*		DESCRIPTION	: SPI����M����											*
*																			*
*		PARAMETER	: IN  : *arg : 											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
#if INTERFACE == IF_SPI_TEGRA	/* SPI-TEGRA�ł̒ʐM */
static void fc_ccd_spi_complete(void *arg)
{
	complete(arg);
}
#endif
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_spi_sync										*
*																			*
*		DESCRIPTION	: SPI����M����											*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *message : ���b�Z�[�W							*
*																			*
*					  RET : �T�C�Y / ���^�[���R�[�h							*
*																			*
****************************************************************************/
#if INTERFACE == IF_SPI_TEGRA	/* SPI-TEGRA�ł̒ʐM */
static ssize_t fc_ccd_spi_sync(ST_CCD_CPUCOM_DATA *ccd, struct spi_message *message)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int status;

	message->complete = fc_ccd_spi_complete;
	message->context = &done;

	spin_lock_irq(&ccd->spi_lock);
	if (NULL == ccd->spi){
		status = -ESHUTDOWN;
	}else{
		/*--------------------------------------*/
		/* SPI�փ��b�Z�[�W���M					*/
		/*--------------------------------------*/
		status = spi_async(ccd->spi, message);
	}
	spin_unlock_irq(&ccd->spi_lock);

	if (0 == status) {
		/*--------------------------------------*/
		/* SPI�����҂�							*/
		/*--------------------------------------*/
		wait_for_completion_interruptible(&done);
		status = message->status;
		if (0 == status)
			status = message->actual_length;
	}
	return status;
}
#endif
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_spi_sync_write									*
*																			*
*		DESCRIPTION	: SPI���M												*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *b   : �f�[�^									*
*																			*
*					  RET : �T�C�Y / ���^�[���R�[�h							*
*																			*
****************************************************************************/
static inline ssize_t fc_ccd_spi_sync_write(ST_CCD_CPUCOM_DATA *ccd,ST_CCD_BUFFER* b)
{
	ssize_t ret = 0;
	ssize_t ret_total = 0;
	volatile unsigned int	i;
	volatile unsigned int	cnt=0;
	char* p=b->buf;
	char* endpoint=b->buf+b->length;
	ssize_t transfer_length;
	b->err = CPUCOM_RET_NONE;
	/* �I�t�Z�b�g���V�t�g�iRx����Length�j */
	p+=b->offset;		/* �I�t�Z�b�g�t�� */
	/*--------------------------------------*/
	/* �f�[�^�I���܂łP�o�C�g�����M		*/
	/*--------------------------------------*/
	for(;p < endpoint ; p+=SPI_TRANSFER_UNIT,++cnt){
#if INTERFACE == IF_SPI_TEGRA	/* SPI-TEGRA�ł̒ʐM */
		struct spi_transfer	t = {
				.tx_buf		= p,
				.len		= p+SPI_TRANSFER_UNIT < endpoint ? SPI_TRANSFER_UNIT : endpoint-p,
			};
		transfer_length = t.len;
		struct spi_message	m;
		spi_message_init(&m);
		spi_message_add_tail(&t, &m);
		ret = fc_ccd_spi_sync(ccd, &m);
#elif INTERFACE == IF_GPIO		/* GPIO����ł̒ʐM	 */
		gpio_set_value(ccd->pin.MCLK, HIGH);
		for(i=0; i < 8; i++){
			gpio_set_value(ccd->pin.MCLK, LOW);
			gpio_set_value(ccd->pin.MDAT, *p & (0x01 << i));/* LSB first */
			CLK_DELAY;
			gpio_set_value(ccd->pin.MCLK, HIGH);
			CLK_DELAY;
		}
		transfer_length =SPI_TRANSFER_UNIT;
		ret=SPI_TRANSFER_UNIT;
#endif
		/*--------------------------------------*/
		/* �G���[�`�F�b�N						*/
		/*--------------------------------------*/
		if(ret<0){							/* ERROR case */
			b->err=CPUCOM_RET_ERR;
		}else{								/* SUCCESS case */
			ret_total+=ret;					/* �����O�X���Z */
			if(transfer_length != ret)				/* ���M�f�[�^������Ȃ� */
				b->err = CPUCOM_RET_ERR;
		}
		/*--------------------------------------*/
		/* ���荞�݃`�F�b�N						*/
		/*--------------------------------------*/
		if(TRUE == fc_ccd_check_pre_suspend())	/* SUSPEND���O�ʒm���� */
			b->err = CPUCOM_RET_PRE_SUSPEND;
		if(ccd->disable_if)
			b->err = CPUCOM_RET_ERR;
		if(ACTIVE == fc_ccd_check_int(ccd))		/* INT#���荞�� */
			b->err = CPUCOM_RET_STOP;
		/*--------------------------------------*/
		/* ���f����								*/
		/*--------------------------------------*/
		if(CPUCOM_RET_NONE != b->err)
			break;
	}
	gpio_set_value(ccd->pin.MDAT, LOW);
	return ret_total;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_spi_sync_read									*
*																			*
*		DESCRIPTION	: SPI��M												*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *b   : �f�[�^									*
*																			*
*					  RET : �T�C�Y / ���^�[���R�[�h							*
*																			*
****************************************************************************/
static inline ssize_t fc_ccd_spi_sync_read(ST_CCD_CPUCOM_DATA *ccd,ST_CCD_BUFFER* b)
{
	ssize_t ret_total = 0;
	ssize_t ret= 0;
	char* p = b->buf;
	char* endpoint=b->buf+b->length;
	volatile unsigned int	i;
	ssize_t transfer_length;
	p+=b->offset;					/* �I�t�Z�b�g�t�� */

	/*--------------------------------------*/
	/* �f�[�^��M							*/
	/*--------------------------------------*/
	for(;p < endpoint ; p+=SPI_TRANSFER_UNIT){
#if INTERFACE == IF_SPI_TEGRA		/* SPI-TEGRA�ł̒ʐM */
		struct spi_transfer	t = {
				.rx_buf		= p,
				.len		= p+SPI_TRANSFER_UNIT < endpoint ? SPI_TRANSFER_UNIT : endpoint-p,
			};
		transfer_length = t.len;
		struct spi_message	m;
		spi_message_init(&m);
		spi_message_add_tail(&t, &m);
		ret=fc_ccd_spi_sync(ccd, &m);
#elif INTERFACE == IF_GPIO			/* GPIO����ł̒ʐM	 */
		gpio_set_value(ccd->pin.MCLK, HIGH);
		for(i=0; i < 8; i++){
			gpio_set_value(ccd->pin.MCLK, LOW);
			CLK_DELAY;
			gpio_set_value(ccd->pin.MCLK, HIGH);
			if(HIGH == gpio_get_value(ccd->pin.SDAT))
				*p |= 0x01 << i;/* LSB first */
			CLK_DELAY;
		}
		transfer_length = SPI_TRANSFER_UNIT;
		ret=SPI_TRANSFER_UNIT;
#endif
		/*--------------------------------------*/
		/* �G���[�`�F�b�N						*/
		/*--------------------------------------*/
		if(ret<0){							/* ERROR case */
			b->err=CPUCOM_RET_ERR;
		}else{								/* SUCCESS case */
			ret_total+=ret;					/* �����O�X���Z */
			if(transfer_length != ret)				/* ���M�f�[�^������Ȃ� */
				b->err = CPUCOM_RET_ERR;
		}
		/*--------------------------------------*/
		/* ���荞�݃`�F�b�N						*/
		/*--------------------------------------*/
		if(TRUE == fc_ccd_check_pre_suspend())	/* SUSPEND���O�ʒm���� */
			b->err = CPUCOM_RET_PRE_SUSPEND;
		if(b->offset){//�f�[�^��M
			if(ACTIVE == fc_ccd_check_int(ccd) ){
				b->err = CPUCOM_RET_STOP;
			}
		}else{//�����O�X��M
//			if(NEGATIVE == fc_ccd_check_int(ccd) ){
//				b->err = CPUCOM_RET_STOP;
//			}
		}
		if(ccd->disable_if)
			b->err = CPUCOM_RET_ERR;
		/*--------------------------------------*/
		/* ���f����								*/
		/*--------------------------------------*/
		if(CPUCOM_RET_NONE != b->err)
			break;
	}
	return ret_total;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_isr_int										*
*																			*
*		DESCRIPTION	: ���荞�݃n���h��										*
*																			*
*		PARAMETER	: IN  : irq  : ���荞��									*
*																			*
*					  IN  : *context_data  : �R���e�L�X�g					*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline irqreturn_t fc_ccd_isr_int(int irq, void *context_data)
{
	ST_CCD_CPUCOM_DATA *ccd = (ST_CCD_CPUCOM_DATA *)context_data;
	if(ccd
	&& FALSE == fc_ccd_check_pre_suspend()				/* �T�X�y���h���O�ʒm�擾�O */
	&& FALSE == ccd->disable_if
	&& CPUCOM_STATE_SLAVE_ERROR != ccd->cpucom_fsm.main	/* �X���[�u�f�o�C�X���� 	*/
	&& CPUCOM_STATE_NONE != ccd->cpucom_fsm.main
	){
		int int_state = fc_ccd_check_int(ccd);
		if(ccd->wc++>1000){
			LOGT("write i/f lock (%d-%d)",ccd->cpucom_fsm.main,ccd->cpucom_fsm.sub);
			ccd->wc=0;
		}
		if(ACTIVE==int_state){
			queue_work(ccd->workqueue, (struct work_struct*)ccd->worker_info[WMID_INTERUPT_INT_ACTIVE]);

			/*----------------------------------------------------------------------*/
			/*	������CS����iWorkerThread�ł̏����ł͊Ԃɍ���Ȃ��P�[�X������j	*/
			/*----------------------------------------------------------------------*/
			if(CPUCOM_STATE_IDLE  == ccd->cpucom_fsm.main){
				/*--------------------------------------------------------------*/
				/* �A�C�h��:�A�C�h���̏ꍇ��CS��������							*/
				/*--------------------------------------------------------------*/
				if( ccd->cpucom_fsm.sub == CPUCOM_SUB_STATE_IDLE )
				{
					fc_ccd_setCS(ccd,ACTIVE);
				}
				else
				/*--------------------------------------------------------------*/
				/* �A�C�h��:�v���A�C�h���̏ꍇ�͑O���[�h����M�̏ꍇ�̂�CS����	*/
				/*--------------------------------------------------------------*/
				if( ccd->cpucom_fsm.sub == CPUCOM_SUB_STATE_NONE ){
					if(CPUCOM_STATE_RX == ccd->pre_state.main){
						fc_ccd_setCS(ccd,ACTIVE);
					}
				}
			}

		}else
		if(NEGATIVE==int_state){
			queue_work(ccd->workqueue, (struct work_struct*)ccd->worker_info[WMID_INTERUPT_INT_NEGATIVE]);
		}else
			LOGE("%s(%d) INT# stauts %d : INT# Unknown Status",__FUNCTION__,__LINE__,int_state);
	}
	return IRQ_HANDLED;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_replace_message							*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : �C�x���g����ւ�����					*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *wi  : ���[�J�[�X���b�h�C�x���g					*
*																			*
*					  IN  : Msgtype : �ϊ����b�Z�[�W						*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline void fc_ccd_fsm_replace_message(ST_CCD_CPUCOM_DATA* ccd,ST_CCD_WORKER_INFO *wi,enum worker_message_type Msgtype)
{
	wi->Msgtype = Msgtype;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_change_state								*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : ��ԑJ��								*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : main : �J�ڐ惁�C���X�e�[�g						*
*																			*
*					  IN  : sub  : �J�ڐ�T�u�X�e�[�g						*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline void fc_ccd_fsm_change_state(ST_CCD_CPUCOM_DATA *ccd,enum cpucom_fsm_mainstate main,enum cpucom_fsm_substate sub)
{
	LOG_CHANGESTATE(ccd,main,sub);
	ccd->pre_state = ccd->cpucom_fsm;
	if(CPUCOM_STATE_NONE != main
	&& main != ccd->cpucom_fsm.main){
		ccd->cpucom_fsm.main =main;
		sub = CPUCOM_SUB_STATE_NONE;
		/*--------------------------------------*/
		/* IDLE�ւ̑J�� 						*/
		/*--------------------------------------*/
		if(CPUCOM_STATE_IDLE == main){
			fc_ccd_setCS(ccd,NEGATIVE);					/* CS#H(NEGATIVE) */
			fc_ccd_timer_cancel(ccd,TID_TLOCK);			/* T5 */
			fc_ccd_timer_cancel(ccd,TID_TIDLE);			/* T5 */
			fc_ccd_timer_start(ccd,TID_TIDLE);			/* T11 */
			fc_ccd_timer_cancel(ccd,TID_INT_POLLING_ON_IDLE);			/* T5 */
			fc_ccd_timer_start(ccd,TID_INT_POLLING_ON_IDLE);/* T8 */
		}
		/*--------------------------------------*/
		/* IDLE����o��J��		 				*/
		/*--------------------------------------*/
		if(CPUCOM_STATE_IDLE == ccd->pre_state.main){
			fc_ccd_timer_cancel(ccd,TID_INT_POLLING_ON_IDLE);/* T8 */
			fc_ccd_timer_cancel(ccd,TID_TIDLE);				/* T11 */
		}
	}
	ccd->cpucom_fsm.sub = sub;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_goto_rxerror_wait_int_high					*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : ����������							*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline void fc_ccd_fsm_goto_rxerror_wait_int_high(ST_CCD_CPUCOM_DATA* ccd)
{
	fc_ccd_buffer_destory_rx(ccd);
	fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_WAIT_INTH_ERR);
	fc_ccd_setCS(ccd,NEGATIVE);
	fc_ccd_timer_start(ccd,TID_INT_POLLING_ON_ABORT_RX);/* T9 */
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_goto_idle									*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : ����������							*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline enum cpucom_fsm_retcode fc_ccd_fsm_goto_idle(ST_CCD_CPUCOM_DATA* ccd)
{
	bool go_to_idle=TRUE;
	enum cpucom_fsm_retcode ret = CPUCOM_RETCODE_DONE;

//IDLE�ւ��ǂ��ā@�Z���^�C�}���|���Ĕ��f�H
	/* INT#L�̏ꍇ�ɂǂ��ɑJ�ڂ�����̂� */
	switch(ccd->cpucom_fsm.main){
	case CPUCOM_STATE_TX:
		switch(ccd->cpucom_fsm.sub)	{
		case CPUCOM_SUB_STATE_WAIT_TX:
			if(ACTIVE == fc_ccd_check_int(ccd))	{
				fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_RX , CPUCOM_SUB_STATE_NONE);
				ret = CPUCOM_RETCODE_RERUN;
				go_to_idle=FALSE;
			}
			break;
		case CPUCOM_SUB_STATE_TX:
		case CPUCOM_SUB_STATE_TX_COMP:
			if(ACTIVE == fc_ccd_check_int(ccd))	{
				fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_WAIT_INTH);
				fc_ccd_setCS(ccd,NEGATIVE);
				fc_ccd_timer_start(ccd,TID_ABORT_TX);	/* T6 */
				ret = CPUCOM_RETCODE_DONE;
				go_to_idle=FALSE;
			}
			break;
		case CPUCOM_SUB_STATE_WAIT_INTH:
		case CPUCOM_SUB_STATE_WAIT_INTL:
		default:
			break;
		}
		break;
	case CPUCOM_STATE_RX:
	case CPUCOM_STATE_IDLE:
	default:
		break;
	}
	if(go_to_idle){
		fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_IDLE , CPUCOM_SUB_STATE_NONE);
		ret = CPUCOM_RETCODE_DONE;
	}
	return ret;
}


/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_init										*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : ����������							*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline void fc_ccd_fsm_init(ST_CCD_CPUCOM_DATA* ccd)
{
	int i;
	/*--------------------------------------*/
	/* CS#H					 				*/
	/*--------------------------------------*/
	fc_ccd_setCS(ccd,NEGATIVE);
	/*--------------------------------------*/
	/* �X�e�[�g�}�V�����������				*/
	/*--------------------------------------*/
	ccd->cpucom_fsm.main= CPUCOM_STATE_NONE;
	ccd->cpucom_fsm.sub= CPUCOM_SUB_STATE_NONE;
	/*--------------------------------------*/
	/* �^�C�}������							*/
	/*--------------------------------------*/
	for(i=0;i<TID_MAX;++i)
		fc_ccd_timer_cancel(ccd,i);
	/*--------------------------------------*/
	/* �o�b�t�@������						*/
	/*--------------------------------------*/
	fc_ccd_buffer_destory_all(ccd);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_state_pre_idle								*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : �A�C�h���O��ԏ���(CS#H�ێ�)			*
*  							���M->��M	3msCS#H�ێ� 						*
*  							���M->���M	3msCS#H�ێ� 						*
*  							��M->���M	3msCS#H�ێ� 						*
*  							��M->��M	NO TIME 							*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *wi	 : ���[�J�[�X���b�h���b�Z�[�W				*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static inline enum cpucom_fsm_retcode fc_ccd_fsm_state_pre_idle(ST_CCD_CPUCOM_DATA* ccd,ST_CCD_WORKER_INFO *wi)
{
	enum cpucom_fsm_retcode ret = CPUCOM_RETCODE_NOTOPERATION;

	switch(wi->Msgtype){
	/*--------------------------------------*/
	/* �A�C�h���ێ����ԃ^�C�} 				*/
	/*--------------------------------------*/
	case WMID_TIDLE:
		if(ACTIVE == fc_ccd_check_int(ccd)){
			/* INT#L�Ȃ�Ύ�M��Ԃ� */
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_RX , CPUCOM_SUB_STATE_NONE);
			fc_ccd_setCS(ccd,ACTIVE);
			ret = CPUCOM_RETCODE_RERUN;
		}else{
		/* �đ����� */
			/*--------------------------------------*/
			/* ���M�\���`�F�b�N					*/
			/*  	�҂���Ԃ�=���M�f�[�^����		*/
			/*  	INT#L�ł͂Ȃ���					*/
			/*--------------------------------------*/
			bool wait=FALSE;
			LOCK_TX{
				if(MSG_WAIT == ccd->tx_complete)
					wait = TRUE;
			}UNLOCK_TX
			if(wait){
				/*--------------------------------------*/
				/* ���M������	 						*/
				/*--------------------------------------*/
				if(CPUCOM_RET_COMP == ccd->tx_buffer->err){
					if(ccd->tx_buffer){
						/* ��ʂ֒ʒm */
						fc_ccd_wakeup_tx(ccd);
					}else{
						LOGE("%s: %s(%d) not exist buffer. go to idle",ccd->id,__FUNCTION__,__LINE__);
					}
					fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_IDLE);
					ret = CPUCOM_RETCODE_DONE;
				}
				/*--------------------------------------*/
				/* �đ�����		 						*/
				/*--------------------------------------*/
				else{
					fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_TX , CPUCOM_SUB_STATE_NONE);
					fc_ccd_fsm_replace_message(ccd,wi,WMID_TX);
					fc_ccd_setCS(ccd,ACTIVE);
					ret = CPUCOM_RETCODE_RERUN;
				}
			}else{
				fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_IDLE);
				ret = CPUCOM_RETCODE_DONE;
			}
		}
		break;
	/*--------------------------------------*/
	/* INT#L���荞�� 						*/
	/*--------------------------------------*/
	case WMID_INTERUPT_INT_ACTIVE:
		if(CPUCOM_STATE_RX == ccd->pre_state.main){
			if(ACTIVE == fc_ccd_check_int(ccd) ){
				fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_RX , CPUCOM_SUB_STATE_NONE);
				fc_ccd_setCS(ccd,ACTIVE);
				ret = CPUCOM_RETCODE_RERUN;
			}
		}
		if(CPUCOM_RETCODE_NOTOPERATION == ret){//��ԑJ�ڂ��Ȃ��ꍇ�͕ی��^�C�}���N�� �J�ڂ��Ȃ���Ԃ������
			if(!hrtimer_active(&ccd->timer_set[TID_TIDLE].timer))/* Timer expired TIDLE in que??*/
				fc_ccd_timer_start(ccd,TID_TIDLE);//�ی�
			ret = CPUCOM_RETCODE_DONE;
		}
		break;
	/*--------------------------------------*/
	/* CS#L����t���h�~�t�F�[���Z�[�t 		*/
	/*--------------------------------------*/
	case WMID_TIMER_INT_POLLING_ON_IDLE:
		fc_ccd_setCS(ccd,NEGATIVE);
		fc_ccd_timer_start(ccd,TID_INT_POLLING_ON_IDLE);
		ret = CPUCOM_RETCODE_DONE;
		break;
	default:
		break;
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_state_idle									*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : �A�C�h����ԏ���						*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *wi	 : ���[�J�[�X���b�h���b�Z�[�W				*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static inline enum cpucom_fsm_retcode fc_ccd_fsm_state_idle(ST_CCD_CPUCOM_DATA* ccd,ST_CCD_WORKER_INFO *wi)
{
	enum cpucom_fsm_retcode ret = CPUCOM_RETCODE_NOTOPERATION;

	switch(wi->Msgtype){
	/*--------------------------------------*/
	/* ���M�v�� 							*/
	/*--------------------------------------*/
	case WMID_TX:
		if(NEGATIVE == fc_ccd_check_int(ccd) ){
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_TX , CPUCOM_SUB_STATE_NONE);
			fc_ccd_fsm_replace_message(ccd,wi,WMID_TX);
			fc_ccd_setCS(ccd,ACTIVE);
			ret = CPUCOM_RETCODE_RERUN;
		}else{
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_RX , CPUCOM_SUB_STATE_NONE);
			fc_ccd_setCS(ccd,ACTIVE);
			ret = CPUCOM_RETCODE_RERUN;
		}
		break;
	/*--------------------------------------*/
	/* INT#L���荞�� 						*/
	/*--------------------------------------*/
	case WMID_INTERUPT_INT_ACTIVE:
		if(ACTIVE == fc_ccd_check_int(ccd) ){
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_RX , CPUCOM_SUB_STATE_NONE);
			fc_ccd_setCS(ccd,ACTIVE);
			ret = CPUCOM_RETCODE_RERUN;
		}else{
			LOGD("%s: %s(%d) not match INT# status <%d,%d,%d>",ccd->id,__FUNCTION__,__LINE__,ccd->cpucom_fsm.main,ccd->cpucom_fsm.sub,wi->Msgtype);
			fc_ccd_timer_cancel(ccd,TID_INT_POLLING_ON_IDLE);/* T8 */
			fc_ccd_timer_cancel(ccd,TID_TIDLE);				/* T11 */
			ret = fc_ccd_fsm_goto_idle(ccd);
		}
		break;
	/*--------------------------------------*/
	/* CS#L����t���h�~�t�F�[���Z�[�t 		*/
	/*--------------------------------------*/
	case WMID_TIMER_INT_POLLING_ON_IDLE:
		fc_ccd_setCS(ccd,NEGATIVE);
		fc_ccd_timer_start(ccd,TID_INT_POLLING_ON_IDLE);
		ret = CPUCOM_RETCODE_DONE;
		break;
	default:
		break;
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_state_tx_wait								*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : ���M�҂���ԏ���						*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *wi	 : ���[�J�[�X���b�h���b�Z�[�W				*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static inline enum cpucom_fsm_retcode fc_ccd_fsm_state_tx_wait(ST_CCD_CPUCOM_DATA* ccd,ST_CCD_WORKER_INFO *wi)
{
	enum cpucom_fsm_retcode ret = CPUCOM_RETCODE_NOTOPERATION;
	switch(wi->Msgtype){
	/*--------------------------------------*/
	/* �}�X�^���M�P�\���� 					*/
	/*--------------------------------------*/
	case WMID_TIMER_TCLK:/* T1 */
		if(NEGATIVE == fc_ccd_check_int(ccd) ){
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_TX);
			ret = CPUCOM_RETCODE_RERUN;
		}else{
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_RX , CPUCOM_SUB_STATE_NONE);
			ret = CPUCOM_RETCODE_RERUN;
		}
		break;
	/*--------------------------------------*/
	/* INT��L 								*/
	/*--------------------------------------*/
	case WMID_INTERUPT_INT_ACTIVE:
		if(ACTIVE == fc_ccd_check_int(ccd) ){
			fc_ccd_timer_cancel(ccd,TID_TCLK);	/* T1 */
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_RX , CPUCOM_SUB_STATE_NONE);
			ret = CPUCOM_RETCODE_RERUN;
		}else{
			LOGD("%s: %s(%d) not match INT# status <%d,%d,%d>",ccd->id,__FUNCTION__,__LINE__,ccd->cpucom_fsm.main,ccd->cpucom_fsm.sub,wi->Msgtype);
			ret = fc_ccd_fsm_goto_idle(ccd);
		}
		break;
	default:
		break;
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_state_tx_tx								*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : ���M����ԏ���						*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *wi	 : ���[�J�[�X���b�h���b�Z�[�W				*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static inline enum cpucom_fsm_retcode fc_ccd_fsm_state_tx_tx(ST_CCD_CPUCOM_DATA* ccd,ST_CCD_WORKER_INFO *wi)
{
	enum cpucom_fsm_retcode ret = CPUCOM_RETCODE_NOTOPERATION;
	switch(wi->Msgtype){
	/*--------------------------------------*/
	/* �}�X�^���M�P�\���� 					*/
	/*--------------------------------------*/
	case WMID_TIMER_TCLK:/* T1 */
		if(ACTIVE == fc_ccd_check_int(ccd) ){
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_WAIT_INTH);
			fc_ccd_setCS(ccd,NEGATIVE);
			fc_ccd_timer_start(ccd,TID_ABORT_TX);	/* T6 */
			ret = CPUCOM_RETCODE_DONE;
		}else{
			ST_CCD_BUFFER* b= ccd->tx_buffer;
			if(b){
				unsigned short length = 0;
				
				/* �������� */
				length = fc_ccd_spi_sync_write(ccd,b);
				/* �������ݐ��� */
				if(CPUCOM_RET_NONE == b->err){
					/* �����O�X�`�F�b�N */
					if(b->length == length){
						fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_TX_COMP);
						fc_ccd_timer_start(ccd,TID_TCSH);
						ret = CPUCOM_RETCODE_DONE;
					}else{
						LOGE("%s: %s(%d) TX error!!! not match length",ccd->id,__FUNCTION__,__LINE__);
						b->err = CPUCOM_RET_ERR;
						ret = fc_ccd_fsm_goto_idle(ccd);
					}
				}else{
				/* �������ݎ��s */
					if(CPUCOM_RET_ERR == b->err){
						LOGD("%s: %s(%d) TX error go to idle",ccd->id,__FUNCTION__,__LINE__);
						ret = fc_ccd_fsm_goto_idle(ccd);
					}else
					if(CPUCOM_RET_STOP == b->err){
						/* �������ݒ�INT#L���o */
						/* INT#L�C�x���g�����ŏ��� */
						fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_WAIT_INTH);
						fc_ccd_setCS(ccd,NEGATIVE);
						fc_ccd_timer_cancel(ccd,TID_TCLK);		/* T1 */
						fc_ccd_timer_start(ccd,TID_ABORT_TX);	/* T6 */
						ret = CPUCOM_RETCODE_DONE;
					}else
					if(CPUCOM_RET_PRE_SUSPEND == b->err){
						/* �������ݒ�SUSPEND���O�ʒm���o */
						/*--------------------------------------*/
						/*  �C�x���g�����҂�����				*/
						/*--------------------------------------*/
						fc_ccd_wakeup_tx(ccd);
						fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_SUSPEND , CPUCOM_SUB_STATE_NONE);
						ret = CPUCOM_RETCODE_DONE;
					}else{
						b->err = CPUCOM_RET_ERR;
						ret = fc_ccd_fsm_goto_idle(ccd);
					}
				}
			}else{
				LOGE("%s: %s(%d) not exist buffer. go to idle",ccd->id,__FUNCTION__,__LINE__);
///�V�O�i�� �グ��H
				ret = fc_ccd_fsm_goto_idle(ccd);
			}
		}
		break;
	/*--------------------------------------*/
	/* INT#L 								*/
	/*--------------------------------------*/
	case WMID_INTERUPT_INT_ACTIVE:
		fc_ccd_timer_cancel(ccd,TID_TCLK);		/* T1 */
		if(ACTIVE == fc_ccd_check_int(ccd) ){
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_WAIT_INTH);
			fc_ccd_setCS(ccd,NEGATIVE);
			fc_ccd_timer_start(ccd,TID_ABORT_TX);	/* T6 */
			ret = CPUCOM_RETCODE_DONE;
		}else{
			LOGD("%s: %s(%d) not match INT# status <%d,%d,%d>",ccd->id,__FUNCTION__,__LINE__,ccd->cpucom_fsm.main,ccd->cpucom_fsm.sub,wi->Msgtype);
			ret = fc_ccd_fsm_goto_idle(ccd);
		}
		break;
	default:
		break;
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_state_tx_comp								*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : ���M������ԏ���						*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *wi	 : ���[�J�[�X���b�h���b�Z�[�W				*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static inline enum cpucom_fsm_retcode fc_ccd_fsm_state_tx_comp(ST_CCD_CPUCOM_DATA* ccd,ST_CCD_WORKER_INFO *wi)
{
	enum cpucom_fsm_retcode ret = CPUCOM_RETCODE_NOTOPERATION;
	switch(wi->Msgtype){
	case WMID_TIMER_TCSH:			/* CS#H�܂ł̎��� 	T2					*/
		if(ACTIVE == fc_ccd_check_int(ccd) ){
			fc_ccd_timer_cancel(ccd,TID_TCSH);
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_WAIT_INTH);
			fc_ccd_setCS(ccd,NEGATIVE);
			fc_ccd_timer_start(ccd,TID_ABORT_TX);	/* T6 */
			ret = CPUCOM_RETCODE_DONE;
		}else{
			/*--------------------------------------*/
			/* ���M����	 							*/
			/*  	��ʂւ̒ʒm��TIDLE�o�ߌ�Ɏ��{	*/
			/*--------------------------------------*/
			ccd->tx_buffer->err = CPUCOM_RET_COMP;
			ret = fc_ccd_fsm_goto_idle(ccd);
		}
		break;
	case WMID_INTERUPT_INT_ACTIVE:	/* INT#L 								*/
		if(ACTIVE == fc_ccd_check_int(ccd) ){
			fc_ccd_timer_cancel(ccd,TID_TCSH);
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_WAIT_INTH);
			fc_ccd_setCS(ccd,NEGATIVE);
			fc_ccd_timer_start(ccd,TID_ABORT_TX);	/* T6 */
			ret = CPUCOM_RETCODE_DONE;
		}else{
			LOGD("%s: %s(%d) not match INT# status <%d,%d,%d>",ccd->id,__FUNCTION__,__LINE__,ccd->cpucom_fsm.main,ccd->cpucom_fsm.sub,wi->Msgtype);
//			ret = fc_ccd_fsm_goto_idle(ccd);
		}
		break;
	default:
		break;
	}
	return ret;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_state_tx_wait_inth							*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : ���M���fINT#H�҂���ԏ���			*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *wi	 : ���[�J�[�X���b�h���b�Z�[�W				*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static inline enum cpucom_fsm_retcode fc_ccd_fsm_state_tx_wait_inth(ST_CCD_CPUCOM_DATA* ccd,ST_CCD_WORKER_INFO *wi)
{
	enum cpucom_fsm_retcode ret = CPUCOM_RETCODE_NOTOPERATION;
	switch(wi->Msgtype){
	/*--------------------------------------*/
	/* INT#H 								*/
	/*--------------------------------------*/
	case WMID_INTERUPT_INT_NEGATIVE:
		fc_ccd_timer_cancel(ccd,TID_ABORT_TX);/* T6 */
		if(NEGATIVE == fc_ccd_check_int(ccd) ){
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_WAIT_INTL);
			fc_ccd_timer_start(ccd,TID_TFAIL_MAX);/* T7 */
			ret = CPUCOM_RETCODE_DONE;
		}else{
			LOGD("%s: %s(%d) not match INT# status <%d,%d,%d>",ccd->id,__FUNCTION__,__LINE__,ccd->cpucom_fsm.main,ccd->cpucom_fsm.sub,wi->Msgtype);
			ret = fc_ccd_fsm_goto_idle(ccd);
		}
		break;
	/*--------------------------------------*/
	/* ���M���f�^�C���A�E�gtlock*2 			*/
	/*--------------------------------------*/
	case WMID_TIMER_ABORT_TX:/* T6 */
		LOGW("%s: %s(%d) ABORT TX timer expired <%d,%d,%d>",ccd->id,__FUNCTION__,__LINE__,ccd->cpucom_fsm.main,ccd->cpucom_fsm.sub,wi->Msgtype);
		ret = fc_ccd_fsm_goto_idle(ccd);
		break;
	default:
		break;
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_state_tx_wait_intl							*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : ���M���fINT#L�҂���ԏ���			*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *wi	 : ���[�J�[�X���b�h���b�Z�[�W				*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static inline enum cpucom_fsm_retcode fc_ccd_fsm_state_tx_wait_intl(ST_CCD_CPUCOM_DATA* ccd,ST_CCD_WORKER_INFO *wi)
{
	enum cpucom_fsm_retcode ret = CPUCOM_RETCODE_NOTOPERATION;
	switch(wi->Msgtype){
	/*--------------------------------------*/
	/* INT#L	 							*/
	/*--------------------------------------*/
	case WMID_INTERUPT_INT_ACTIVE:
		fc_ccd_timer_cancel(ccd,TID_TFAIL_MAX);
		if(ACTIVE == fc_ccd_check_int(ccd) ){
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_RX , CPUCOM_SUB_STATE_NONE);
			ret = CPUCOM_RETCODE_RERUN;
		}else{
			LOGD("%s: %s(%d) not match INT# status <%d,%d,%d>",ccd->id,__FUNCTION__,__LINE__,ccd->cpucom_fsm.main,ccd->cpucom_fsm.sub,wi->Msgtype);
			ret = fc_ccd_fsm_goto_idle(ccd);
		}
		break;
	/*--------------------------------------*/
	/* ���M���f�^�C���A�E�g tfail 			*/
	/*--------------------------------------*/
	case WMID_TIMER_TFAIL_MAX:/* T7 */
		LOGE("%s: %s(%d) TFAIL MAX timer expired <%d,%d,%d>",ccd->id,__FUNCTION__,__LINE__,ccd->cpucom_fsm.main,ccd->cpucom_fsm.sub,wi->Msgtype);
		ret = fc_ccd_fsm_goto_idle(ccd);
		break;
	default:
		break;
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_state_rx_wait_len							*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : �����O�X��M�҂�������				*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *wi	 : ���[�J�[�X���b�h���b�Z�[�W				*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static inline enum cpucom_fsm_retcode fc_ccd_fsm_state_rx_wait_len(ST_CCD_CPUCOM_DATA* ccd,ST_CCD_WORKER_INFO *wi)
{
	enum cpucom_fsm_retcode ret = CPUCOM_RETCODE_NOTOPERATION;
	switch(wi->Msgtype){
	/*--------------------------------------*/
	/* �N���b�N���o���� */
	/*--------------------------------------*/
	case WMID_TIMER_TLOCK:/* T5 */
		{
			LOGW("%s: (%d)[RX] TLOCK(5sec)T.O",ccd->id,__LINE__);
			fc_ccd_fsm_goto_rxerror_wait_int_high(ccd);
			ret = CPUCOM_RETCODE_DONE;
		}
		break;
	/*--------------------------------------*/
	/* �N���b�N���o�J�n���ԁi�����O�X�j		*/
	/*--------------------------------------*/
	case WMID_TIMER_TCLK2:/* T2	 */
		if(ACTIVE == fc_ccd_check_int(ccd)){
			ST_CCD_BUFFER* b=NULL;
			fc_ccd_timer_cancel(ccd,TID_TLOCK);/* T5 */
			
			/*--------------------------------------*/
			/* ��M�o�b�t�@����						*/
			/*--------------------------------------*/
			b= kmalloc(sizeof(*b), GFP_KERNEL);
			if(b){
				char* temp = kmalloc(sizeof(*temp), GFP_KERNEL);
				memset(b,0,sizeof(*b));
				if(temp){
					ssize_t size=0;
					*temp=0;
					ccd->rx_buffer = b;
					INIT_LIST_HEAD(&b->list);

					/* �����O�X�̂ݓǂݍ��� */
					b->length =1;
					b->buf=temp;
					size=fc_ccd_spi_sync_read(ccd,b);
					/* ��M���� */
					if(CPUCOM_RET_NONE == b->err){
						if(1 == size
						&& 0 != *temp){
							/* �T�C�Y���̃o�b�t�@���m�� */
							b->length=(unsigned short)*temp;
							kfree(temp);
							b->buf= kmalloc(SIZE_OF_TOTAL(b->length), GFP_KERNEL);
							if(b->buf){
								memset(b->buf,0,SIZE_OF_TOTAL(b->length));
								b->buf[0] = (char)b->length;
								b->length = SIZE_OF_TOTAL(b->length);
								b->offset =1;
								b->err = CPUCOM_RET_NONE;
								
								fc_ccd_timer_start(ccd,TID_TLOCK);	/* T5 */
								fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_WAIT_RX_DATA);
								ret = CPUCOM_RETCODE_DONE;
							}else{
								LOGE("%s: %s(%d) : cannot allocate memory!!!",ccd->id,__FUNCTION__,__LINE__);
								fc_ccd_buffer_destory_rx(ccd);
								ret = fc_ccd_fsm_goto_idle(ccd);
							}
						}else{
							LOGD("%s: %s(%d) : RX length err (size=%d len=%d)",ccd->id,__FUNCTION__,__LINE__,size,*temp);
							fc_ccd_buffer_destory_rx(ccd);
							ret = fc_ccd_fsm_goto_idle(ccd);
						}
					}
					/* ��M���s */
					else{
						if(CPUCOM_RET_ERR == b->err){
							LOGW("%s: %s(%d) : RX return is err ",ccd->id,__FUNCTION__,__LINE__);
							fc_ccd_buffer_destory_rx(ccd);
							ret = fc_ccd_fsm_goto_idle(ccd);
						}else
						if(CPUCOM_RET_PRE_SUSPEND == b->err){
							/*--------------------------------------*/
							/*  ���X�g�ɒǉ�						*/
							/*--------------------------------------*/
							LOCK_RX{
								list_add_tail(&b->list, &ccd->rx_buffer_list);
							ccd->rx_buffer = NULL;
							}UNLOCK_RX
							/*--------------------------------------*/
							/*  �C�x���g�����҂�����				*/
							/*--------------------------------------*/
							fc_ccd_wakeup_rx(ccd);
							/*--------------------------------------*/
							/*  SUSPEND�֑J��						*/
							/*--------------------------------------*/
							fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_SUSPEND , CPUCOM_SUB_STATE_NONE);
							fc_ccd_fsm_replace_message(ccd,wi,WMID_TIMER_TCLK3);
							ret = CPUCOM_RETCODE_RERUN;
						}else
						if(CPUCOM_RET_STOP == b->err){
							/*--------------------------------------*/
							/*  INT#��ԕω�						*/
							/*--------------------------------------*/
							LOGW("%s: %s(%d) : INT#H while read the length",ccd->id,__FUNCTION__,__LINE__);
							fc_ccd_buffer_destory_rx(ccd);
							ret = fc_ccd_fsm_goto_idle(ccd);
						}else{
							LOGW("%s: %s(%d) : RX return is unkown err ",ccd->id,__FUNCTION__,__LINE__);
							fc_ccd_buffer_destory_rx(ccd);
							ret = fc_ccd_fsm_goto_idle(ccd);
						}
					}
				}else{
					fc_ccd_buffer_destory_rx(ccd);
					ret = fc_ccd_fsm_goto_idle(ccd);
				}
			}else{
				LOGE("%s: %s(%d) : cannot allocate memory!!!",ccd->id,__FUNCTION__,__LINE__);
				ret = fc_ccd_fsm_goto_idle(ccd);
			}
		}else{
			ret = fc_ccd_fsm_goto_idle(ccd);
		}
		break;
	default:
		break;
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_state_rx_wait_data							*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : �f�[�^��M�҂�������					*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *wi	 : ���[�J�[�X���b�h���b�Z�[�W				*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static inline enum cpucom_fsm_retcode fc_ccd_fsm_state_rx_wait_data(ST_CCD_CPUCOM_DATA* ccd,ST_CCD_WORKER_INFO *wi)
{
	enum cpucom_fsm_retcode ret = CPUCOM_RETCODE_NOTOPERATION;
	switch(wi->Msgtype){
	/*--------------------------------------*/
	/* �N���b�N���o���� 					*/
	/*--------------------------------------*/
	case WMID_TIMER_TCLK3:/* T4 */
		fc_ccd_timer_cancel(ccd,TID_TLOCK);/* T5 */
		fc_ccd_timer_start(ccd,TID_TLOCK);
		if(NEGATIVE == fc_ccd_check_int(ccd)){
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_RX_DATA);
			fc_ccd_fsm_replace_message(ccd,wi,WMID_TIMER_TCLK3);
			ret = CPUCOM_RETCODE_RERUN;
		}else
			ret = CPUCOM_RETCODE_DONE;
		break;
	case WMID_TIMER_TLOCK:/* T5 */
		fc_ccd_timer_cancel(ccd,TID_TLOCK);/* T5 */
		LOGW("%s: (%d)[RX]TLOCK(5sec)T.O",ccd->id,__LINE__);
		fc_ccd_fsm_goto_rxerror_wait_int_high(ccd);
		ret = CPUCOM_RETCODE_DONE;
		break;
	/*--------------------------------------*/
	/* INT#H 								*/
	/*--------------------------------------*/
	case WMID_INTERUPT_INT_NEGATIVE:
		if(NEGATIVE == fc_ccd_check_int(ccd) ){
			fc_ccd_timer_cancel(ccd,TID_TLOCK);	/* T5 */
			fc_ccd_timer_start(ccd,TID_TLOCK);	/* T5 */
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_RX_DATA);
			fc_ccd_fsm_replace_message(ccd,wi,WMID_TIMER_TCLK3);
			ret = CPUCOM_RETCODE_RERUN;
		}else{
			fc_ccd_timer_cancel(ccd,TID_TLOCK);/* T5 */
			LOGD("%s: %s(%d) not match INT# status <%d,%d,%d>",ccd->id,__FUNCTION__,__LINE__,ccd->cpucom_fsm.main,ccd->cpucom_fsm.sub,wi->Msgtype);
			fc_ccd_buffer_destory_rx(ccd);
			ret = fc_ccd_fsm_goto_idle(ccd);
		}
		break;
	default:
		break;
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_state_rx_data								*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : �f�[�^��M������						*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *wi	 : ���[�J�[�X���b�h���b�Z�[�W				*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static inline enum cpucom_fsm_retcode fc_ccd_fsm_state_rx_data(ST_CCD_CPUCOM_DATA* ccd,ST_CCD_WORKER_INFO *wi)
{
	enum cpucom_fsm_retcode ret = CPUCOM_RETCODE_NOTOPERATION;
	switch(wi->Msgtype){
	/*--------------------------------------*/
	/*  �N���b�N���o����					*/
	/*--------------------------------------*/
	case WMID_TIMER_TLOCK:/* T5 */
		{
			LOGW("%s: (%d)[RX]TLOCK(5sec)T.O",ccd->id,__LINE__);
			fc_ccd_fsm_goto_rxerror_wait_int_high(ccd);
			ret = CPUCOM_RETCODE_DONE;
		}
		break;
	/*--------------------------------------*/
	/*  ��M����							*/
	/*--------------------------------------*/
	case WMID_INTERUPT_RX_COMP:
		{
			unsigned long fcc_rcv= 0;
			unsigned long fcc_cal= 0;
			unsigned long all_len= 0;
			ST_CCD_BUFFER* b = ccd->rx_buffer;
			if(b){
				/*--------------------------------------*/
				/*  FCC�`�F�b�N							*/
				/*--------------------------------------*/
				all_len = b->length - SIZE_OF_FCC;
				fcc_cal = fc_ccd_calc_fcc(ccd,b->buf,all_len);
				fcc_rcv = ((ulong)b->buf[all_len    ] << 24)
						| ((ulong)b->buf[all_len + 1] << 16)
						| ((ulong)b->buf[all_len + 2] <<  8)
						| ((ulong)b->buf[all_len + 3]);
				if( fcc_cal == fcc_rcv ) {
					LOCK_RX{
						b->err = CPUCOM_RET_COMP;
						list_add_tail(&b->list, &ccd->rx_buffer_list);
					ccd->rx_buffer = NULL;	
					}UNLOCK_RX
					/*--------------------------------------*/
					/*  �C�x���g�����҂�����				*/
					/*--------------------------------------*/
					fc_ccd_wakeup_rx(ccd);
				}else{
					LOGE("%s: FCC error!! fcc 0x%08X / calc 0x%08X",ccd->id,(int)fcc_rcv,(int)fcc_cal);
					fc_ccd_buffer_destory_rx(ccd);
				}
				/*--------------------------------------*/
				/*  IDLE�֑J��							*/
				/*--------------------------------------*/
				ret = fc_ccd_fsm_goto_idle(ccd);
			}else{
				LOGE("%s: %s(%d) event=RX_COMP : rx buffer is emty",ccd->id,__FUNCTION__,__LINE__);
				ret = fc_ccd_fsm_goto_idle(ccd);
			}
		}
		break;
	/*--------------------------------------*/
	/*  �N���b�N���o�J�n���ԁi�f�[�^�j		*/
	/*--------------------------------------*/
	case WMID_TIMER_TCLK3:
		fc_ccd_timer_cancel(ccd,TID_TLOCK);/* T5 */
		if(NEGATIVE == fc_ccd_check_int(ccd) ){
			ST_CCD_BUFFER* b = ccd->rx_buffer;
			if(b){
				/* �f�[�^�ǂݍ��� */
				ssize_t size=0;
				size=fc_ccd_spi_sync_read(ccd,b);
				/* ��M���� */
				if(CPUCOM_RET_NONE == b->err){
					ssize_t data_size = b->length - b->offset;
					if(data_size != size){
						LOGE("%s: %s(%d) : cannot get RX data !!!",ccd->id,__FUNCTION__,__LINE__);
						fc_ccd_buffer_destory_rx(ccd);
						ret = fc_ccd_fsm_goto_idle(ccd);
					}else{
						fc_ccd_fsm_replace_message(ccd,wi,WMID_INTERUPT_RX_COMP);
						ret = CPUCOM_RETCODE_RERUN;
					}
				}
				/* ��M���s */
				else{
					if(CPUCOM_RET_ERR == b->err){
						LOGW("%s: %s(%d) : RX return is err ",ccd->id,__FUNCTION__,__LINE__);
						fc_ccd_buffer_destory_rx(ccd);
						ret = fc_ccd_fsm_goto_idle(ccd);
					}else
					if(CPUCOM_RET_PRE_SUSPEND == b->err){
						LOCK_RX{
							list_add_tail(&b->list, &ccd->rx_buffer_list);
							/*--------------------------------------*/
							/*  �C�x���g�����҂�����				*/
							/*--------------------------------------*/
						ccd->rx_buffer = NULL;
						}UNLOCK_RX
						fc_ccd_wakeup_rx(ccd);
						/*--------------------------------------*/
						/*  SUSPEND�֑J��						*/
						/*--------------------------------------*/
						fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_SUSPEND , CPUCOM_SUB_STATE_NONE);
						ret = CPUCOM_RETCODE_DONE;
					}else
					if(CPUCOM_RET_STOP == b->err){
						/*--------------------------------------*/
						/*  INT#��ԕω�						*/
						/*--------------------------------------*/
						LOGW("%s: %s(%d) : INT#L while read the data",ccd->id,__FUNCTION__,__LINE__);
						fc_ccd_buffer_destory_rx(ccd);
						ret = fc_ccd_fsm_goto_idle(ccd);
					}else{
						LOGE("%s: %s(%d) unknown err",ccd->id,__FUNCTION__,__LINE__);
						fc_ccd_buffer_destory_rx(ccd);
						ret = fc_ccd_fsm_goto_idle(ccd);
					}
				}
			}else{
				LOGE("%s: %s(%d) event=TIMER_TCLK3 : buffer is emty",ccd->id,__FUNCTION__,__LINE__);
				ret = fc_ccd_fsm_goto_idle(ccd);
			}
		}else{
			LOGD("%s: %s(%d) not match INT# status <%d,%d,%d>",ccd->id,__FUNCTION__,__LINE__,ccd->cpucom_fsm.main,ccd->cpucom_fsm.sub,wi->Msgtype);
			fc_ccd_buffer_destory_rx(ccd);
			ret = fc_ccd_fsm_goto_idle(ccd);
		}
		break;
	default:
		break;
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_state_rx_wait_inth_err						*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : �f�[�^��M�G���[INT#H�҂�������		*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *wi	 : ���[�J�[�X���b�h���b�Z�[�W				*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static inline enum cpucom_fsm_retcode fc_ccd_fsm_state_rx_wait_inth_err(ST_CCD_CPUCOM_DATA* ccd,ST_CCD_WORKER_INFO *wi)
{
	enum cpucom_fsm_retcode ret = CPUCOM_RETCODE_NOTOPERATION;
	switch(wi->Msgtype){
	/*--------------------------------------*/
	/*  INT#H�`�F�b�N�^�C�}					*/
	/*--------------------------------------*/
	case WMID_TIMER_INT_POLLING_ON_ABORT_RX:/* T9 */
		fc_ccd_setCS(ccd,NEGATIVE);
		/* INT#H���o */
		if(NEGATIVE == fc_ccd_check_int(ccd)){
			ccd->polling_on_abort_rx_cnt=0;
			ret = fc_ccd_fsm_goto_idle(ccd);
		}else{
			/* �J�E���g�I�[�o�[ */
			if(POLLING_ON_ABORT_RX_MAX <= ccd->polling_on_abort_rx_cnt++ ){
				ccd->polling_on_abort_rx_cnt=0;
				LOGD("%s: %s(%d) NOT DETECT INT#H",ccd->id,__FUNCTION__,__LINE__);
				/* IDLE�ւ͑J�ڂ��Ȃ� 							*/
				/* INT#L�̂܂܂������ꍇ�A����t��FailSafe����	*/
				//fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_IDLE , CPUCOM_SUB_STATE_NONE);
			}
			fc_ccd_timer_start(ccd,TID_INT_POLLING_ON_ABORT_RX);
			ret = CPUCOM_RETCODE_DONE;
		}
		break;
	/*--------------------------------------*/
	/*  INT#H								*/
	/*--------------------------------------*/
	case WMID_INTERUPT_INT_NEGATIVE:
		if(NEGATIVE == fc_ccd_check_int(ccd) ){
			fc_ccd_timer_cancel(ccd,TID_INT_POLLING_ON_ABORT_RX);
			ccd->polling_on_abort_rx_cnt=0;
			ret = fc_ccd_fsm_goto_idle(ccd);
		}else{
			LOGD("%s: %s(%d) not match INT# status <%d,%d,%d>",ccd->id,__FUNCTION__,__LINE__,ccd->cpucom_fsm.main,ccd->cpucom_fsm.sub,wi->Msgtype);
			ret = fc_ccd_fsm_goto_idle(ccd);
		}
		break;
	default:
		break;
	}
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_run										*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : ���s����								*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *wi	 : ���[�J�[�X���b�h���b�Z�[�W				*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static inline enum cpucom_fsm_retcode fc_ccd_fsm_run(ST_CCD_CPUCOM_DATA* ccd,ST_CCD_WORKER_INFO *wi)
{
	enum cpucom_fsm_retcode ret = CPUCOM_RETCODE_NOTOPERATION;
	LOGD("%s: %s(%d)",ccd->id,__FUNCTION__,__LINE__);
	LOGD("%s: %s[%s] <-- msg: %s"	,ccd->id,cpucom_fsm_main[ccd->cpucom_fsm.main].str
											,cpucom_fsm_sub[ccd->cpucom_fsm.sub].str
											,cpucom_message[wi->Msgtype].str );
	/*--------------------------------------*/
	/*  �T�X�y���h���O�ʒm����SUSPEND��		*/
	/*--------------------------------------*/
	if(TRUE == fc_ccd_check_pre_suspend())
		fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_SUSPEND , CPUCOM_SUB_STATE_NONE);

	switch(ccd->cpucom_fsm.main){
	/*--------------------------------------*/
	/*  �A�C�h��							*/
	/*--------------------------------------*/
	case CPUCOM_STATE_IDLE:
		LOGFSM("%s: IDLE(%d <-- (%d)",ccd->id,ccd->cpucom_fsm.sub,wi->Msgtype);
		switch(ccd->cpucom_fsm.sub){
		/*--------------------------------------*/
		/*  �A�C�h���������					*/
		/*--------------------------------------*/
		case CPUCOM_SUB_STATE_NONE:	
			ret = fc_ccd_fsm_state_pre_idle(ccd,wi);
			break;
		/*--------------------------------------*/
		/*  �A�C�h�����						*/
		/*--------------------------------------*/
		case CPUCOM_SUB_STATE_IDLE:	
			ret = fc_ccd_fsm_state_idle(ccd,wi);
			break;
		default:
			break;
		}
		break;
	/*--------------------------------------*/
	/*  ���M								*/
	/*--------------------------------------*/
	case CPUCOM_STATE_TX:
		LOGFSM("%s: TX (%d <-- %d)",ccd->id,ccd->cpucom_fsm.sub,wi->Msgtype);
		switch(ccd->cpucom_fsm.sub){
		/*--------------------------------------*/
		/*  ���M�������						*/
		/*--------------------------------------*/
		case CPUCOM_SUB_STATE_NONE:	
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_WAIT_TX);
			fc_ccd_setCS(ccd,ACTIVE);
			fc_ccd_timer_start(ccd,TID_TCLK);
			ret = CPUCOM_RETCODE_DONE;
			break;
		/*--------------------------------------*/
		/*  ���M�҂����						*/
		/*--------------------------------------*/
		case CPUCOM_SUB_STATE_WAIT_TX:
			ret = fc_ccd_fsm_state_tx_wait(ccd,wi);
			break;
		/*--------------------------------------*/
		/*  ���M�����							*/
		/*--------------------------------------*/
		case CPUCOM_SUB_STATE_TX:
			ret = fc_ccd_fsm_state_tx_tx(ccd,wi);
			break;
		/*--------------------------------------*/
		/*  ���M����							*/
		/*--------------------------------------*/
		case CPUCOM_SUB_STATE_TX_COMP:
			ret = fc_ccd_fsm_state_tx_comp(ccd,wi);
			break;
		/*--------------------------------------*/
		/*  ���M���fINT#H�҂�					*/
		/*--------------------------------------*/
		case CPUCOM_SUB_STATE_WAIT_INTH:
			ret = fc_ccd_fsm_state_tx_wait_inth(ccd,wi);
			break;
		/*--------------------------------------*/
		/*  ���M���fINT#L�҂�					*/
		/*--------------------------------------*/
		case CPUCOM_SUB_STATE_WAIT_INTL:
			ret = fc_ccd_fsm_state_tx_wait_intl(ccd,wi);
			break;
		default:
			break;
		}
		break;
	/*--------------------------------------*/
	/*  ��M								*/
	/*--------------------------------------*/
	case CPUCOM_STATE_RX:
		LOGFSM("%s: RX (%d <-- %d)",ccd->id,ccd->cpucom_fsm.sub,wi->Msgtype);
		switch(ccd->cpucom_fsm.sub){
		/*--------------------------------------*/
		/*  ��M�������						*/
		/*--------------------------------------*/
		case CPUCOM_SUB_STATE_NONE:
			fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_NONE , CPUCOM_SUB_STATE_WAIT_RX_LEN);
			fc_ccd_setCS(ccd,ACTIVE);
			fc_ccd_fsm_replace_message(ccd,wi,WMID_TIMER_TCLK2);
			ret = CPUCOM_RETCODE_RERUN;
			break;
		/*--------------------------------------*/
		/*  �����O�X��M�҂����				*/
		/*--------------------------------------*/
		case CPUCOM_SUB_STATE_WAIT_RX_LEN:
			ret = fc_ccd_fsm_state_rx_wait_len(ccd,wi);
			break;
		/*--------------------------------------*/
		/*  �f�[�^��M�҂����					*/
		/*--------------------------------------*/
		case CPUCOM_SUB_STATE_WAIT_RX_DATA:
			ret = fc_ccd_fsm_state_rx_wait_data(ccd,wi);
			break;
		/*--------------------------------------*/
		/*  �f�[�^��M���						*/
		/*--------------------------------------*/
		case CPUCOM_SUB_STATE_RX_DATA:
			ret = fc_ccd_fsm_state_rx_data(ccd,wi);
			break;
		/*--------------------------------------*/
		/*  �f�[�^��M���s						*/
		/*--------------------------------------*/
		case CPUCOM_SUB_STATE_WAIT_INTH_ERR:
			ret = fc_ccd_fsm_state_rx_wait_inth_err(ccd,wi);
			break;
		default:
			break;
		}
		break;
	/*--------------------------------------*/
	/*  �T�X�y���h							*/
	/*--------------------------------------*/
	case CPUCOM_STATE_SUSPEND:
		LOGFSM("%s: SUSPEND evt(%d)",ccd->id,wi->Msgtype);
		fc_ccd_wakeup_rxtx(ccd);
		break;
	/*--------------------------------------*/
	/*  ���W���[��							*/
	/*--------------------------------------*/
	case CPUCOM_STATE_RESUME:
		LOGFSM("%s: RESUME evt(%d)",ccd->id,wi->Msgtype);
		break;
	/*--------------------------------------*/
	/*  �X���[�u�f�o�C�X�ُ�				*/
	/*--------------------------------------*/
	case CPUCOM_STATE_SLAVE_ERROR:
		LOGFSM("%s: SLAVE DEVICE ERROR evt(%d)",ccd->id,wi->Msgtype);
		break;
	default:
		break;
	}
	LOGD("%s: %s:ret=%d",ccd->id,__FUNCTION__,ret);
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_common										*
*																			*
*		DESCRIPTION	: �X�e�[�g�}�V�� : ���ʏ���								*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *wi	 : ���[�J�[�X���b�h���b�Z�[�W				*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static inline enum cpucom_fsm_retcode fc_ccd_fsm_common(ST_CCD_CPUCOM_DATA* ccd,ST_CCD_WORKER_INFO *wi)
{
	enum cpucom_fsm_retcode ret = CPUCOM_RETCODE_NOTOPERATION;
	switch(wi->Msgtype){
	/*--------------------------------------*/
	/*  INT#L����t�����o�^�C�}	5s			*/
	/*--------------------------------------*/
	case WMID_INT_OBSERVE:/* T12 */
		/*--------------------------------------*/
		/*  INT#L�̏ꍇ�̂ݍX�V����				*/
		/*--------------------------------------*/
		if(ACTIVE==fc_ccd_check_int(ccd)){
			if(POLLING_INT_OBSERVE <= ++ccd->polling_int_observe_cnt){	/* 60Sec����t�����o */
				LOGE("%s: PERMANENTAL INT#L!! SLAVE DEVICE FAIL ERROR",ccd->id);
				fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_SLAVE_ERROR , CPUCOM_SUB_STATE_NONE);
			}else{
				/*--------------------------------------*/
				/* 10Sec���ɑ��M�f�[�^�̊J���������s��  */
				/*--------------------------------------*/
				if(0==ccd->polling_int_observe_cnt%2){					/* 10Sec������t�����o */
					  LOGW("%s: MAINTAIN INT#L %dSec",ccd->id,ccd->polling_int_observe_cnt*5);
					  if(ccd->tx_buffer){
						  ccd->tx_buffer->err = CPUCOM_RET_ERR;
						  fc_ccd_wakeup_tx(ccd);
					  }
					  /*--------------------------------------*/
					  /* �C���^�[�t�F�C�X���b�N				*/
					  /*--------------------------------------*/
					  ccd->disable_if=TRUE;
				}
				/*--------------------------------------*/
				/* INT#L����t�����o�^�C�}�ăX�^�[�g	*/
				/*--------------------------------------*/
				fc_ccd_timer_start(ccd,TID_INT_OBSERVE);	/* T12 */
			}
		}else{
			/*----------------------------------------------*/
			/* �J�E���^�������E�C���^�[�t�F�C�X���b�N����	*/
			/*----------------------------------------------*/
			ccd->polling_int_observe_cnt=0;
			ccd->disable_if=FALSE;
		}
		break;
	default:
		break;
	}
	return ret;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_workThread										*
*																			*
*		DESCRIPTION	: ���[�J�[�X���b�h										*
*																			*
*		PARAMETER	: IN  : *work  :���[�J�[�X���b�h�f�[�^					*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ccd_workThread(struct work_struct *work)
{
	int i;
	ST_CCD_WORKER_INFO *orig_wi = container_of(work, ST_CCD_WORKER_INFO, workq);
	ST_CCD_CPUCOM_DATA* ccd = (ST_CCD_CPUCOM_DATA*)orig_wi->pthis;

	/*--------------------------------------*/
	/*  �C�x���g�����[�J���ɃR�s�[			*/
	/*     �C�x���g����ւ������Ή�			*/
	/*--------------------------------------*/
	ST_CCD_WORKER_INFO work_wi = *orig_wi;
	mutex_lock(&ccd->workerthread_lock);

	ccd->calling|=CALL_WS;
	/*--------------------------------------*/
	/*  INT#����t�����o�^�C�}����			*/
	if(WMID_INTERUPT_INT_ACTIVE == orig_wi->Msgtype
	|| WMID_INTERUPT_INT_NEGATIVE == orig_wi->Msgtype){
		if(ACTIVE == fc_ccd_check_int(ccd) ){
			fc_ccd_timer_start(ccd,TID_INT_OBSERVE);	/* T12 */
		}else{
			fc_ccd_timer_cancel(ccd,TID_INT_OBSERVE);	/* T12 */
			ccd->polling_int_observe_cnt=0;				/* �J�E���^������ */
			ccd->disable_if=FALSE;
		}
	}
	if(FALSE==ccd->disable_if){
		/*--------------------------------------*/
		/*  �X�e�[�g�}�V�����s					*/
		/*--------------------------------------*/
		for(i=0;i<CPUCOM_FSM_LOOPMAX;++i){
			/* RERUN����FSM�ăX�^�[�g 		*/
			if(CPUCOM_RETCODE_RERUN != fc_ccd_fsm_run(ccd,&work_wi))
				break;
		}
	}
	/*--------------------------------------*/
	/*  ���ʏ���							*/
	/*--------------------------------------*/
	fc_ccd_fsm_common(ccd,&work_wi);		/* �������C�x���g�͋��ʏ�����  	*/
	ccd->calling&=~CALL_WS;
	mutex_unlock(&ccd->workerthread_lock);
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_read											*
*																			*
*		DESCRIPTION	: OPEN													*
*																			*
*		PARAMETER	: IN  : *filp  : �t�@�C��								*
*																			*
*					  IN  : *buf   : �o�b�t�@								*
*																			*
*					  IN  : count  : �J�E���g								*
*																			*
*					  IN  : *f_pos : �|�W�V����								*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static long fs_ccd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	ST_CCD_CPUCOM_DATA	*ccd = filp->private_data;
	unsigned char data[32+1]={0};
	copy_from_user( data, (int __user*)arg, cmd<32?cmd:32 );
	LOGT("[Middle]:%s",data);
	return 0;
	
}

static ssize_t fs_ccd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t			status = -EFAULT;
	ST_CCD_BUFFER* b=NULL;
	bool exist=FALSE;
	bool			done=FALSE;
	ST_CCD_CPUCOM_DATA	*ccd = filp->private_data;
	LOGD("%s: %s(%d)",ccd->id,__FUNCTION__,__LINE__);
//LOGT("%s:R->",ccd->id);

	/*--------------------------------------*/
	/*  �R���e�L�X�g�`�F�b�N				*/
	/*--------------------------------------*/
	if(NULL == ccd){
		LOGE("%s: %s(%d) context is NULL",ccd->id,__FUNCTION__,__LINE__);
		return status;
	}
	mutex_lock(&ccd->read_interface_lock);
	/*--------------------------------------*/
	/*  ��ԃ`�F�b�N						*/
	/*--------------------------------------*/
	if(FALSE == fc_ccd_check_pre_suspend()				/* �T�X�y���h���O�ʒm�擾�O */
	&& FALSE == ccd->disable_if							/* Interface����            */
	&& CPUCOM_STATE_SLAVE_ERROR != ccd->cpucom_fsm.main	/* �X���[�u�f�o�C�X���� 	*/
	&& CPUCOM_STATE_NONE != ccd->cpucom_fsm.main		/* �X�e�[�g�}�V��������� 	*/
	){
		ccd->calling|=CALL_RX;
		do{
			b=NULL;
			exist=FALSE;
			LOCK_RX{
				if(!list_empty(&ccd->rx_buffer_list)){
					exist=TRUE;
					b = list_first_entry(&ccd->rx_buffer_list,ST_CCD_BUFFER,list);
				}
			}UNLOCK_RX
			/*--------------------------------------*/
			/*  �o�b�t�@�f�[�^�`�F�b�N				*/
			/*--------------------------------------*/
			if(exist
			&& b){
				switch(b->err){
				case CPUCOM_RET_COMP:
				case CPUCOM_RET_PRE_SUSPEND:
					done = TRUE;
					break;
				default:
					LOGW("%s: %s(%d) read non expect return code(%d) ",ccd->id,__FUNCTION__,__LINE__,b->err);
					done = TRUE;
					break;
				}
			}else{
				/*--------------------------------------*/
				/*  �o�b�t�@�Ƀf�[�^�Ȃ�				*/
				/*--------------------------------------*/
				LOCK_RX{
					ccd->rx_complete = MSG_WAIT;
				}UNLOCK_RX
				/*--------------------------------------*/
				/*  ��M�҂�							*/
				/*--------------------------------------*/
				wait_event_interruptible(ccd->rxDataQue, ccd->rx_complete);
				LOCK_RX{
					ccd->rx_complete = MSG_DONE;
				}UNLOCK_RX
			}
			if(TRUE == fc_ccd_check_pre_suspend())
				done = TRUE;
			if(ccd->disable_if)
				done = TRUE;
		}while(!done);

		/*--------------------------------------*/
		/*  ���^�[���R�[�h����					*/
		/*--------------------------------------*/
		if(b){
			/*--------------------------------------*/
			/*  SUSPEND���f							*/
			/*--------------------------------------*/
			if(CPUCOM_RET_PRE_SUSPEND == b->err){
				status = PRE_SUSPEND_RETVAL;
			}
			/*--------------------------------------*/
			/*  ��M����							*/
			/*--------------------------------------*/
			else
			if(CPUCOM_RET_COMP == b->err){
				status = b->length-SIZE_OF_FCC;
				if(status == SIZE_OF_EXCLUDE_FCC(b->buf[0])){
					/*--------------------------------------*/
					/*  ���[�U�����h�փR�s�[				*/
					/*--------------------------------------*/
					if(!copy_to_user(&buf[0], b->buf, status )){
						PRINT_DATA(ccd,1,b->buf,SIZE_OF_TOTAL(b->buf[0]));
					}else{
						status = -EFAULT;
						LOGE("%s: %s : copy_to_user fail",ccd->id,__FUNCTION__);
					}
				}else{
					status = -EFAULT;
					LOGE("%s: Error size!!! status: %d  / buf[0] length %d",ccd->id,status,SIZE_OF_EXCLUDE_FCC(b->buf[0]));
				}
			}
			/*--------------------------------------*/
			/*  �o�b�t�@�J��						*/
			/*--------------------------------------*/
			LOCK_RX{
				fc_ccd_buffer_destoryList(b,&ccd->rx_buffer_list);
			}UNLOCK_RX
		}
		ccd->calling&=~CALL_RX;
	}else{
		LOGT("%s: read Guard(%d,%d(%d-%d)%d)",ccd->id,fc_ccd_check_pre_suspend(),ccd->disable_if,ccd->cpucom_fsm.main,ccd->cpucom_fsm.main,accept_flg);
	}
	if(TRUE == fc_ccd_check_pre_suspend())
		status = PRE_SUSPEND_RETVAL;
	LOGD("%s: %s:ret=%d",ccd->id,__FUNCTION__,status);
	mutex_unlock(&ccd->read_interface_lock);

//LOGT("<--R %s",ccd->id);
	return status;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_write											*
*																			*
*		DESCRIPTION	: OPEN													*
*																			*
*		PARAMETER	: IN  : *filp  : �t�@�C��								*
*																			*
*					  IN  : *buf   : �o�b�t�@								*
*																			*
*					  IN  : *f_pos : �|�W�V����								*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static ssize_t 	fs_ccd_write(struct file *filp, const char __user *buf,size_t count, loff_t *f_pos)
{
	
	ssize_t			status = -EFAULT;
	ST_CCD_CPUCOM_DATA	*ccd = filp->private_data;
	LOGD("%s: %s(%d)",ccd->id,__FUNCTION__,__LINE__);
	
	/*--------------------------------------*/
	/*  �R���e�L�X�g�`�F�b�N				*/
	/*--------------------------------------*/
	if(NULL == ccd){
		LOGE("%s: %s(%d) context is NULL",ccd->id,__FUNCTION__,__LINE__);
		return status;
	}
	mutex_lock(&ccd->write_interface_lock);
	/*--------------------------------------*/
	/*  ��ԃ`�F�b�N						*/
	/*--------------------------------------*/
	if(FALSE == fc_ccd_check_pre_suspend()				/* �T�X�y���h���O�ʒm�擾�O */
	&& FALSE == ccd->disable_if							/* Interface����            */
	&& CPUCOM_STATE_SLAVE_ERROR != ccd->cpucom_fsm.main	/* �X���[�u�f�o�C�X���� 	*/
	&& CPUCOM_STATE_NONE != ccd->cpucom_fsm.main		/* �X�e�[�g�}�V��������� 	*/
	){
		char length=0;
		ST_CCD_BUFFER* b=NULL;
#ifdef CPUCOMDEBUG_LOGCACHE		/* for debug */
		if(count <= 2){
			char cmd;
			copy_from_user(&cmd, buf, 1);
			printk(KERN_INFO "#### CMD %d ####",cmd);
			ccd_cache_out();
			return 0;
		}
#endif
		ccd->calling|=CALL_TX;
		/*--------------------------------------*/
		/*  ���M�o�b�t�@����					*/
		/*--------------------------------------*/
		b= kmalloc(sizeof(*b), GFP_KERNEL);
		if(b){
			ccd->tx_buffer=b;
			memset(b,0,sizeof(*b));
			INIT_LIST_HEAD(&b->list);
			
			if(!copy_from_user(&length, buf, 1)){
											/* 1�o�C�g�ڂ̓����O�X */
				b->length = SIZE_OF_TOTAL(length);
				b->buf= kmalloc(b->length, GFP_KERNEL);
				if(b->buf){
					memset(b->buf,0,b->length);
					if(!copy_from_user(b->buf, buf, b->length)){
											/* �����O�X���R�s�[ */
						/*--------------------------------------*/
						/*  FCC����								*/
						/*--------------------------------------*/
						unsigned long	fcc=fc_ccd_calc_fcc(ccd,b->buf,length * 4+1);
						b->buf[1 + (length * 4) + 0] = (unsigned char)((fcc & 0xFF000000) >> 24);
						b->buf[1 + (length * 4) + 1] = (unsigned char)((fcc & 0x00FF0000) >> 16);
						b->buf[1 + (length * 4) + 2] = (unsigned char)((fcc & 0x0000FF00) >>  8);
						b->buf[1 + (length * 4) + 3] = (unsigned char)((fcc & 0x000000FF));
						/*--------------------------------------*/
						/*  QUE�ɒǉ����ăC�x���g���s			*/
						/*--------------------------------------*/
						queue_work(ccd->workqueue, (struct work_struct*)ccd->worker_info[WMID_TX]);
						ccd->wc=0;
						/*--------------------------------------*/
						/*  ���M�����҂�						*/
						/*--------------------------------------*/
						LOCK_TX{
							ccd->tx_complete = MSG_WAIT;
						}UNLOCK_TX
						wait_event_interruptible( ccd->txDataQue, ccd->tx_complete );
						LOCK_TX{
							ccd->tx_complete = MSG_DONE;
						}UNLOCK_TX
						/*--------------------------------------*/
						/*  ���M����							*/
						/*--------------------------------------*/
						if(CPUCOM_RET_COMP == b->err){
							PRINT_DATA(ccd,0,b->buf,SIZE_OF_TOTAL(b->buf[0]));
							status = b->length;
						}
						/*--------------------------------------*/
						/*  �T�X�y���h���O�ʒm�ɂ�钆�f		*/
						/*--------------------------------------*/
						else
						if(CPUCOM_RET_PRE_SUSPEND == b->err){
							status = PRE_SUSPEND_RETVAL;
						}else
							status = -EFAULT;
					}else
						status = -ENOMEM;
				}else
					status = -ENOMEM;
			}
			/*--------------------------------------*/
			/*  �o�b�t�@�̊J��						*/
			/*--------------------------------------*/
			fc_ccd_buffer_destory_tx(ccd);
		}else{
			status = -ENOMEM;
		}
		ccd->calling&=~CALL_TX;
	}else{
		LOGT("%s: read Guard(%d,%d(%d-%d)%d)",ccd->id,fc_ccd_check_pre_suspend(),ccd->disable_if,ccd->cpucom_fsm.main,ccd->cpucom_fsm.main,accept_flg);
	}

	if(TRUE == fc_ccd_check_pre_suspend())
		status = PRE_SUSPEND_RETVAL;
	LOGD("%s: %s:ret=%d",ccd->id,__FUNCTION__,status);
	mutex_unlock(&ccd->write_interface_lock);

	return status;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_open											*
*																			*
*		DESCRIPTION	: OPEN													*
*																			*
*		PARAMETER	: IN  : *inode : �m�[�h									*
*																			*
*					  IN  : *filp  : �t�@�C��								*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static int fs_ccd_open(struct inode *inode, struct file *filp)
{
	int i;
	ST_CCD_CPUCOM_DATA	*ccd;
	int		status = -EFAULT;
	mutex_lock(&device_list_lock);
	/*--------------------------------------*/
	/*  �f�o�C�X���X�g����					*/
	/*--------------------------------------*/
	list_for_each_entry(ccd, &device_list, device_entry) {
		if (ccd->devt == inode->i_rdev) {
			status = ESUCCESS;
			break;
		}
	}
	if(status != ESUCCESS){
		LOGE("%s: CANNOT FIND my device",ccd->id);
		return -EFAULT;
	}
	filp->private_data = ccd;
	nonseekable_open(inode, filp);
	mutex_lock(&ccd->workerthread_lock);
	/*-----------------------------------------*/
	/*  �@�\��~	                           */
	/*-----------------------------------------*/
	ccd->disable_if=TRUE;
	ccd->cpucom_fsm.main= CPUCOM_STATE_NONE;
	ccd->cpucom_fsm.sub= CPUCOM_SUB_STATE_NONE;
	/*--------------------------------------*/
	/* �^�C�}������							*/
	/*--------------------------------------*/
	for(i=0;i<TID_MAX;++i)
		fc_ccd_timer_cancel(ccd,i);
	/*-----------------------------------------*/
	/*  ClenaUp				                  */
	/*-----------------------------------------*/
	if(0 < ccd->users){	/* re-open */
		LOGI("%s RE-OPEN",ccd->id);
	}
	fc_ccd_do_release(ccd);
	/*-----------------------------------------*/
	/*  ���[�U������                           */
	/*-----------------------------------------*/
	ccd->users++;
	/*-----------------------------------------*/
	/*  ��t����                               */
	/*-----------------------------------------*/
	accept_flg=TRUE;
	ccd->disable_if=FALSE;
	/*-----------------------------------------*/
	/*  IDLE�X�e�[�g�֑J��                     */
	/*-----------------------------------------*/
	ccd->cpucom_fsm.main= CPUCOM_STATE_NONE;
	ccd->cpucom_fsm.sub= CPUCOM_SUB_STATE_NONE;
	fc_ccd_fsm_change_state(ccd,CPUCOM_STATE_IDLE , CPUCOM_SUB_STATE_NONE);
	/*-----------------------------------------*/
	/*  ���荞�݋���                           */
	/*-----------------------------------------*/
	if(FALSE == ccd->irq_status){
		enable_irq(ccd->irq);
		ccd->irq_status =TRUE;
	}
	mutex_unlock(&ccd->workerthread_lock);
	mutex_unlock(&device_list_lock);
	LOGI("%s: oepn SUCCESS",ccd->id);
	return status;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_release										*
*																			*
*		DESCRIPTION	: RELEASE												*
*																			*
*		PARAMETER	: IN  : *inode : �m�[�h									*
*																			*
*					  IN  : *filp  : �t�@�C��								*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static int fs_ccd_release(struct inode *inode, struct file *filp)
{
	int		dofree;
	ST_CCD_CPUCOM_DATA	*ccd=NULL;

	ccd = filp->private_data;
	LOGT("%s: %s(%d)",ccd->id,__FUNCTION__,__LINE__);
	mutex_lock(&device_list_lock);
	/*-----------------------------------------*/
	/*  ���[�U������                           */
	/*-----------------------------------------*/
	--ccd->users;
	/*-----------------------------------------*/
	/*  spi��ԃ`�F�b�N                        */
	/*-----------------------------------------*/
	/* ... after we unbound from the underlying device? */
	spin_lock_irq(&ccd->spi_lock);
	dofree = (ccd->spi == NULL);
	spin_unlock_irq(&ccd->spi_lock);
	if (dofree){
		LOGE("%s: %s(%d) ccd->spi is NULL",ccd->id,__FUNCTION__,__LINE__);
		//kfree(ccd);
	}
	mutex_unlock(&device_list_lock);
	return 0;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_concreator										*
*																			*
*		DESCRIPTION	: ��������												*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static int fc_ccd_concreator(ST_CCD_CPUCOM_DATA* ccd)
{
	int status=ESUCCESS;
	LOGD("%s: %s(%d)",ccd->id,__FUNCTION__,__LINE__);
	/*-----------------------------------------*/
	/*  ��t����                               */
	/*-----------------------------------------*/
	accept_flg=FALSE;
	/*--------------------------------------*/
	/*  �|�[�g�ݒ�							*/
	/*--------------------------------------*/
	if(0 == ccd->spi->master->bus_num){/* SPI1 = CPU_COM1 �F��e�ʒʐM*/
		ccd->pin.MCLK = CPU_COM_MCLK1;
		ccd->pin.MREQ = CPU_COM_MREQ1;
		ccd->pin.SREQ = CPU_COM_SREQ1;
		ccd->pin.SDAT = CPU_COM_SDT1;
		ccd->pin.MDAT = CPU_COM_MDT1;
	}else
	if(1 == ccd->spi->master->bus_num){/* SPI2 = CPU_COM0 : �R�}���h�ʐM*/
		ccd->pin.MCLK = CPU_COM_MCLK0;
		ccd->pin.MREQ = CPU_COM_MREQ0;
		ccd->pin.SREQ = CPU_COM_SREQ0;
		ccd->pin.SDAT = CPU_COM_SDT0;
		ccd->pin.MDAT = CPU_COM_MDT0;
	}else{
		LOGE("%s: UNKNOWN bus %d",ccd->id,ccd->spi->master->bus_num);
		status = -EINVAL;
	}
	if(ESUCCESS == status){
		int i;
		/*--------------------------------------*/
		/*  wait que������						*/
		/*--------------------------------------*/
		init_waitqueue_head( &ccd->txDataQue );
		init_waitqueue_head( &ccd->rxDataQue );
		/*--------------------------------------*/
		/*  �C�x���g�R���f�B�V����������		*/
		/*--------------------------------------*/
		ccd->rx_complete = MSG_DONE;
		ccd->tx_complete = MSG_DONE;
		/*--------------------------------------*/
		/*  Mutex que������						*/
		/*--------------------------------------*/
		mutex_init(&ccd->tx_buffer_list_lock);
		mutex_init(&ccd->rx_buffer_list_lock);
		mutex_init(&ccd->write_interface_lock);
		mutex_init(&ccd->read_interface_lock);
		mutex_init(&ccd->workerthread_lock);
		/*--------------------------------------*/
		/*  ���[�J�[�X���b�h����				*/
		/*--------------------------------------*/
		ccd->workqueue = create_singlethread_workqueue("spi-tegra-cpucom-work");
		if (ccd->workqueue){
			/*--------------------------------------*/
			/*  �C�x���g����						*/
			/*--------------------------------------*/
			for(i=0;i<ARRAY_SIZE(ccd->worker_info);++i){
				ccd->worker_info[i]=(ST_CCD_WORKER_INFO*)kmalloc(sizeof(*ccd->worker_info[i]), GFP_KERNEL);
				if(!ccd->worker_info[i]){
					LOGE("%s: cannot alloc  ST_CCD_WORKER_INFO",ccd->id);
					status = -ENOMEM;
					break;
				}else{
					memset(ccd->worker_info[i],0,sizeof(*ccd->worker_info[i]));
					INIT_WORK((struct work_struct*)ccd->worker_info[i], fc_ccd_workThread);
					ccd->worker_info[i]->Msgtype = (enum worker_message_type)i;
					ccd->worker_info[i]->pthis =ccd;
				}
			}
			if(ESUCCESS == status){
				/*--------------------------------------*/
				/*  SPI-TEGRA�ݒ�						*/
				/*--------------------------------------*/
				ccd->cdata.is_hw_based_cs = 1;		/* CS��H/W�ɂĎ������� */
				ccd->cdata.cs_setup_clk_count =0;
				ccd->cdata.cs_hold_clk_count  =0;
				ccd->spi->controller_data = (void*)&ccd->cdata;
				ccd->spi->bits_per_word=8;
				spi_setup(ccd->spi);
				/*--------------------------------------*/
				/*  �^�C�}�[������						*/
				/*--------------------------------------*/
				status = fc_ccd_timer_init(ccd);
			}else{
				for(i=0;i<ARRAY_SIZE(ccd->worker_info);++i){
					if(ccd->worker_info[i])
						kfree(ccd->worker_info[i]);
				}
				destroy_workqueue(ccd->workqueue);
			}
		}else{
			status = -ENOMEM;
			LOGE("%s: cannot alloc  create_singlethread_workqueue",ccd->id);
		}
	}
	LOGD("%s: %s:ret=%d",ccd->id,__FUNCTION__,status);
	return status;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_suspend										*
*																			*
*		DESCRIPTION	: SUSPEND												*
*																			*
*		PARAMETER	: IN  : *spi : spi�f�[�^								*
*																			*
*					  IN  : mesg : ���b�Z�[�W								*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static int fs_ccd_suspend(struct spi_device *spi, pm_message_t mesg)
{
	int i;
	ST_CCD_CPUCOM_DATA	*ccd = spi_get_drvdata(spi);
	LOGT("%s: %s",ccd->id,__FUNCTION__);
	/*-----------------------------------------*/
	/*  ���荞�݋֎~                           */
	/*-----------------------------------------*/
	if(TRUE == ccd->irq_status){
		disable_irq(ccd->irq);
		ccd->irq_status =FALSE;
	}
	/*--------------------------------------*/
	/* �^�C�}������							*/
	/*--------------------------------------*/
	for(i=0;i<TID_MAX;++i)
		fc_ccd_timer_cancel(ccd,i);
	/*-----------------------------------------*/
	/*  ��t����                               */
	/*-----------------------------------------*/
	accept_flg=FALSE;
	ccd->cpucom_fsm.main= CPUCOM_STATE_NONE;
	ccd->cpucom_fsm.sub= CPUCOM_SUB_STATE_NONE;
	fc_ccd_do_release(ccd);
	return 0;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_resume											*
*																			*
*		DESCRIPTION	: RESUME												*
*																			*
*		PARAMETER	: IN  : *spi : spi�f�[�^								*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static int fs_ccd_resume(struct spi_device *spi)
{
	int i;
	ST_CCD_CPUCOM_DATA	*ccd = spi_get_drvdata(spi);
	LOGT("%s: %s",ccd->id,__FUNCTION__);
	/*--------------------------------------*/
	/* �^�C�}������							*/
	/*--------------------------------------*/
	for(i=0;i<TID_MAX;++i)
		fc_ccd_timer_cancel(ccd,i);
	/*-----------------------------------------*/
	/*  ���荞�݋֎~                           */
	/*-----------------------------------------*/
	if(TRUE == ccd->irq_status){
		disable_irq(ccd->irq);
		ccd->irq_status =FALSE;
	}
	/*-----------------------------------------*/
	/*  ��t����                               */
	/*-----------------------------------------*/
	accept_flg=FALSE;
	/*-----------------------------------------*/
	/*  CS#H(NEGATIVE)                         */
	/*-----------------------------------------*/
	fc_ccd_setCS(ccd,NEGATIVE);	
	/*-----------------------------------------*/
	/*  �����[�X����                           */
	/*-----------------------------------------*/
	ccd->cpucom_fsm.main= CPUCOM_STATE_NONE;
	ccd->cpucom_fsm.sub= CPUCOM_SUB_STATE_NONE;
	fc_ccd_do_release(ccd);
	return 0;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_show_adapter_name								*
*																			*
*		DESCRIPTION	: �A�_�v�^�l�[������									*
*																			*
*		PARAMETER	: IN : *dev : dev�f�[�^									*
*																			*
*					  IN : *attr: �����f�[�^								*
*																			*
*					  OUT: *buf : �����񐶐��o�b�t�@						*
*																			*
*					  RET : ������											*
*																			*
****************************************************************************/
static ssize_t fc_ccd_show_adapter_name(struct device *dev, struct device_attribute *attr, char *buf)
{
	int no  = MINOR(dev->devt);
	return sprintf(buf, "spi-tegra-cpucom-%d", no);
}
static DEVICE_ATTR(name, S_IRUGO, fc_ccd_show_adapter_name, NULL);
/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sudden_device_poweroff							*
*																			*
*		DESCRIPTION	: �����I�����O�ʒm�֐�									*
*																			*
*		PARAMETER	: IN  : dev�f�[�^										*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static void fs_ccd_sudden_device_poweroff(struct device *dev)
{
	ST_CCD_CPUCOM_DATA* ccd=NULL;
	UNUSED(dev);
	UNUSED(ccd);
	LOGT("fs_ccd_sudden_device_poweroff");
	/*-----------------------------------------*/
	/*  ��t����                               */
	/*-----------------------------------------*/
	accept_flg=FALSE;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_probe											*
*																			*
*		DESCRIPTION	: probe													*
*																			*
*		PARAMETER	: IN  : spi�f�[�^										*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static int __devinit fs_ccd_probe(struct spi_device *spi)
{
	ST_CCD_CPUCOM_DATA	*ccd;
	struct device *dev=NULL;
	char 	request_string[128]={0};
	int	status=0;
	unsigned long	minor;

	/*--------------------------------------*/
	/*  �C���X�^���X����					*/
	/*--------------------------------------*/
	ccd = kzalloc(sizeof(*ccd), GFP_KERNEL);
	if (NULL == ccd){
		LOGE("%s: %s(%d) kzalloc ERR!!",ccd->id,__FUNCTION__,__LINE__);
		return -ENOMEM;
	}

#if defined(CPUCOMDEBUG_PRINTDATA) || defined(CPUCOMDEBUG_LOGCACHE)
	pCashe=kzalloc(CASHE_SIZE, GFP_KERNEL);
	pCashe_current=pCashe;
	if (NULL == pCashe){
		LOGE("%s: %s(%d) kzalloc ERR!! pCashe is NULL",ccd->id,__FUNCTION__,__LINE__);
	}else{
		memset(pCashe,0,sizeof(*pCashe));
	}
#endif /*CPUCOMDEBUG_PRINTDATA*/

	memset(ccd,0,sizeof(*ccd));
	ccd->spi = spi;							/* SPI�ݒ�			 */
	spin_lock_init(&ccd->spi_lock);
	/*--------------------------------------*/
	/*  ���X�g������						*/
	/*--------------------------------------*/
	INIT_LIST_HEAD(&ccd->device_entry);
	INIT_LIST_HEAD(&ccd->rx_buffer_list);

	/*--------------------------------------*/
	/*  �f�o�C�X�t�@�C������				*/
	/*--------------------------------------*/
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		ccd->devt = MKDEV(CPUCOM_MAJOR, minor);
		dev = device_create(cpucom_class, &spi->dev, ccd->devt,
				    ccd, "cpucom%d.%d",
				    spi->master->bus_num, spi->chip_select);
		memset(ccd->id,0,sizeof(ccd->id));
		sprintf(ccd->id,"%d.%d",spi->master->bus_num, spi->chip_select);
		status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
		if (0 == status){
			status = device_create_file(dev, &dev_attr_name);
		}
	} else {
		dev_dbg(&spi->dev, "no minor number available!");
		status = -ENODEV;
	}
	/*--------------------------------------*/
	/*  �C���X�^���X����������				*/
	/*--------------------------------------*/
	if (0 == status) {
		set_bit(minor, minors);
		list_add(&ccd->device_entry, &device_list);
		status =fc_ccd_concreator(ccd);
	}
	mutex_unlock(&device_list_lock);
	if (0 == status){
		/*--------------------------------------*/
		/*  �����N���R�[���o�b�N�֐��ݒ�		*/
		/*--------------------------------------*/
		ccd->ed.reinit = NULL;
		ccd->ed.sudden_device_poweroff = fs_ccd_sudden_device_poweroff;
		ccd->ed.dev = dev;
		if(0 != early_device_register(&ccd->ed))		/* early �f�o�C�X�o�^*/
			early_device_unregister(&ccd->ed);			/* early �f�o�C�X�o�^����*/
		/*--------------------------------------*/
		/*  SPI�o�^								*/
		/*--------------------------------------*/
		spi_set_drvdata(spi, ccd);
		/*--------------------------------------*/
		/*  �|�[�g�ݒ�							*/
		/*--------------------------------------*/
		gpio_request(ccd->pin.MREQ,fc_ccd_gpio_request_str(ccd,request_string,"MREQ"));
		gpio_request(ccd->pin.SREQ,fc_ccd_gpio_request_str(ccd,request_string,"SREQ"));
#if INTERFACE == IF_GPIO	/* GPIO�ݒ�̏ꍇ�͂��ׂĐݒ肷�� */
		gpio_request(ccd->pin.MCLK,fc_ccd_gpio_request_str(ccd,request_string,"MCLK"));
		gpio_request(ccd->pin.SDAT,fc_ccd_gpio_request_str(ccd,request_string,"SDAT"));
		gpio_request(ccd->pin.MDAT,fc_ccd_gpio_request_str(ccd,request_string,"MDAT"));

		tegra_gpio_enable(ccd->pin.MDAT);
		gpio_direction_output(ccd->pin.MDAT, LOW);
		tegra_gpio_enable(ccd->pin.SDAT);
		gpio_direction_input(ccd->pin.SDAT);
		tegra_gpio_enable(ccd->pin.MCLK);
		gpio_direction_output(ccd->pin.MCLK, HIGH);
#endif
		tegra_gpio_enable(ccd->pin.SREQ);
		gpio_direction_input(ccd->pin.SREQ);
		tegra_gpio_enable(ccd->pin.MREQ);
		gpio_direction_output(ccd->pin.MREQ, HIGH);
		/*--------------------------------------*/
		/*  TG MUTE ON							*/
		/*--------------------------------------*/
		gpio_request(TEGRA_GPIO_PX6, "TEGRA_MUTE");
		tegra_gpio_enable(TEGRA_GPIO_PX6);
		gpio_direction_output(TEGRA_GPIO_PX6, LOW);
		gpio_set_value(TEGRA_GPIO_PX6, LOW);
	
		/*--------------------------------------*/
		/*  �X�e�[�g�}�V�������ݒ�				*/
		/*--------------------------------------*/
		fc_ccd_fsm_init(ccd);
		/*--------------------------------------*/
		/*  ���荞�ݐݒ�						*/
		/*--------------------------------------*/
		ccd->irq = gpio_to_irq(ccd->pin.SREQ);
		fc_ccd_gpio_request_str(ccd,request_string,"IRQ");
		status = request_irq(ccd->irq,
			fc_ccd_isr_int,
			IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING,
			request_string, ccd);
		if(ESUCCESS == status){
			LOGI("%s: probe SUCCESS  irq - %d",ccd->id,ccd->irq);
			disable_irq(ccd->irq);
		}else
			LOGE("%s: FAIL request_irq errcode(%d)",ccd->id,status);
	}
	if(status){
		LOGI("%s: probe FAIL",ccd->id);
		kfree(ccd);
	}
	return status;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_remove											*
*																			*
*		DESCRIPTION	: remove												*
*																			*
*		PARAMETER	: IN  : spi�f�[�^										*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static int __devexit fs_ccd_remove(struct spi_device *spi)
{
	int i;
	ST_CCD_CPUCOM_DATA	*ccd = spi_get_drvdata(spi);
	LOGD("%s: %s(%d)",ccd->id,__FUNCTION__,__LINE__);

	/*--------------------------------------*/
	/*  �����E�J��							*/
	/*--------------------------------------*/
	spin_lock_irq(&ccd->spi_lock);
	ccd->spi = NULL;
	spi_set_drvdata(spi, NULL);					/* SPI�o�^����			 */
	spin_unlock_irq(&ccd->spi_lock);

	mutex_lock(&device_list_lock);

	/*--------------------------------------*/
	/*  GPIO�J��							*/
	/*--------------------------------------*/
	gpio_free(ccd->pin.MREQ);
	gpio_free(ccd->pin.SREQ);
#if INTERFACE == IF_GPIO
	gpio_free(ccd->pin.MCLK);
	gpio_free(ccd->pin.SDAT);
	gpio_free(ccd->pin.MDAT);
#endif

	early_device_unregister(&ccd->ed);			/* early �f�o�C�X�o�^����*/
	list_del(&ccd->device_entry);				/* �f�o�C�X���X�g�폜	 */
	device_destroy(cpucom_class, ccd->devt);	/* �f�o�C�X�N���X�폜	 */
	clear_bit(MINOR(ccd->devt), minors);		/* �}�C�i�[No�폜		 */
	fc_ccd_buffer_destory_all(ccd);				/* �o�b�t�@�S�폜 		 */
	destroy_workqueue(ccd->workqueue);			/* ���[�N�L���[�폜 */
	
	for(i=0;i<ARRAY_SIZE(ccd->worker_info);++i){/* ���[�J�[�X���b�h�C�x���g�S�폜 */
		if(ccd->worker_info[i])
			kfree(ccd->worker_info[i]);
	}
	free_irq(ccd->irq,ccd);
	kfree(ccd);									/* �C���X�^���X�폜 */
#ifdef CPUCOMDEBUG_PRINTDATA					/* �f�o�b�N	�f�[�^�L���b�V�� */
	if (pCashe)
		kfree(pCashe);
#endif /*CPUCOMDEBUG_PRINTDATA*/

	mutex_unlock(&device_list_lock);

	return 0;
}
static const struct file_operations cpucom_fops = {
	.owner =	THIS_MODULE,
	/* REVISIT switch to aio primitives, so that userspace
	 * gets more complete API coverage.  It'll simplify things
	 * too, except for the locking.
	 */
	.write =	fs_ccd_write,
	.read =		fs_ccd_read,
	.open =		fs_ccd_open,
	.release =	fs_ccd_release,
	.llseek =	no_llseek,
	.unlocked_ioctl = fs_ccd_ioctl,
};

static struct spi_driver spi_cpucom_driver = {
	.probe	= fs_ccd_probe,
	.remove = __devexit_p(fs_ccd_remove),
	.suspend =	fs_ccd_suspend,
	.resume =	fs_ccd_resume,
	.driver = {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
	},
};
/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_init											*
*																			*
*		DESCRIPTION	: INIT													*
*																			*
*		PARAMETER	: None													*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static int __init fs_ccd_init(void)
{
	int status;
	ST_CCD_CPUCOM_DATA* ccd=NULL;
	UNUSED(ccd);
	/*--------------------------------------*/
	/*  �L�����N�^�[�f�o�C�X�o�^			*/
	/*--------------------------------------*/
	CPUCOM_MAJOR = register_chrdev(CPUCOM_MAJOR, DRV_NAME, &cpucom_fops);
	if (CPUCOM_MAJOR < 0){
		LOGE("%s(%d) register_chrdev ERR!!",__FUNCTION__,__LINE__);
		return CPUCOM_MAJOR;
	}
	/*--------------------------------------*/
	/*  �N���X�쐬�o�^						*/
	/*--------------------------------------*/
	cpucom_class = class_create(THIS_MODULE, DRV_NAME);
	if (IS_ERR(cpucom_class)) {
		unregister_chrdev(CPUCOM_MAJOR, spi_cpucom_driver.driver.name);
		LOGE("%s(%d) class_create ERR!!",__FUNCTION__,__LINE__);
		return PTR_ERR(cpucom_class);
	}
	/*--------------------------------------*/
	/*  SPI�ւ̓o�^							*/
	/*--------------------------------------*/
	if ((status = spi_register_driver(&spi_cpucom_driver) )< 0) {
		class_destroy(cpucom_class);         /* SPI�o�^  */
		unregister_chrdev(CPUCOM_MAJOR, spi_cpucom_driver.driver.name);
		LOGE("%s(%d) spi_register_driver ERR!!",__FUNCTION__,__LINE__);
	}										/* SPI�o�^  */
	LOGD("%s(%d) status=%d",__FUNCTION__,__LINE__,status);
	return status;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_exit											*
*																			*
*		DESCRIPTION	: EXIT													*
*																			*
*		PARAMETER	: None													*
*																			*
*					  RET : None											*
*																			*
****************************************************************************/
static void __exit fs_ccd_exit(void)
{
	/*----------------------------------*/
	/*  SPI�f�o�C�X�o�^����				*/
	/*----------------------------------*/
	spi_unregister_driver(&spi_cpucom_driver);
}

module_init(fs_ccd_init);
module_exit(fs_ccd_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:cpucom");

