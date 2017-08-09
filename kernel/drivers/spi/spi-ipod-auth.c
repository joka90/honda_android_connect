/*--------------------------------------------------------------------------*/
/* COPYRIGHT(C) FUJITSU TEN LIMITED 2012-2014                               */
/*--------------------------------------------------------------------------*/
/****************************************************************************
*																			*
*		�V���[�Y����	: A-DA												*
*		�}�C�R������	: tegra3											*
*		�I�[�_�[����	: 													*
*		�u���b�N����	: kernel driver										*
*		DESCRIPTION		: iPod�F�؃h���C�o									*
*		�t�@�C������	: spi-ipod-auth.c									*
*																			*
*		Copyright(C) 2012 FUJITSU TEN LIMITED								*
*																			*
*		�w�G���g���[�v���O�����x											*
*			fs_ipa_init ............ �h���C�o������							*
*			fs_ipa_exit ............ �h���C�o�I��							*
*			fs_ipa_probe ........... �h���C�o����							*
*			fs_ipa_remove........... �h���C�o�폜							*
*			fs_ipa_open ............ �h���C�o�I�[�v��						*
*			fs_ipa_release.......... �h���C�o�N���[�Y						*
*			fs_ipa_suspend.......... �h���C�o�T�X�y���h						*
*			fs_ipa_resume .......... �h���C�o���W���[��						*
*			fs_ipa_compat_ioctl..... �h���C�oIOCONTROL						*
*			fs_ipa_ioctl ........... �h���C�oIOCONTROL						*
*			fs_ipa_sudden_device_poweroff..... �����I���G���g��				*
*			fs_ipa_reinit .......... �����N���G���g��						*
*		�wSPI����M�x														*
*			fc_ipa_syncRead	.........SPI��M								*
*			fc_ipa_syncWrite.........SPI���M								*
*			fc_ipa_sync..............SPI����M								*
*			fc_ipa_complete..........��M����								*
*		�w�^�C�}�[�x														*
*			fc_ipa_timer_init.......�^�C�}������							*
*			fc_ipa_timer_callback...�^�C�}�R�[���o�b�N						*
*			fc_ipa_timer_start......�^�C�}�X�^�[�g							*
*			fc_ipa_timer_cancel.....�^�C�}�L�����Z��						*
*		�w���̑��̃T�u���[�`���x											*
*			fc_ipa_show_adapter_name..�A�_�v�^�l�[������					*
*			fc_ipa_concreate..........��������								*
*			fc_ipa_decreate...........�I���㏈��							*
*			fc_ipa_workThread........ ���[�J�[�X���b�h						*
*			fc_ipa_Send.............. IPOD�F�ؑ��M							*
*			fc_ipa_Recv.............. IPOD�F�؎�M							*
*			fc_ipa_SendCmdAdr........ �R�}���h���M							*
*			fc_ipa_prepare_communication.. �ʐM���O����						*
*			fc_ipa_init.............. �|�[�g���Z�b�g						*
*			fc_ipa_set_sfio...........SFIO�ݒ菈��							*
*			fc_ipa_setCS............. CS#����								*
*			fc_ipa_checkPreSuspend... �T�X�y���h����						*
*			fc_ipa_set_gpio_output... GPIO OUT�ݒ�							*
*			fc_ipa_gpio_request_str.. gpio�o�^�����񐶐�					*
* 	�yNote�z 																*
* 		1S�ł�SW_BUDET#�̊��荞�݂��������Ă�����2S�ȍ~�������Ȃ��Ȃ��� 	*
* 		2S���SW_BUDET#��H/W�ύX�������Ă���Bkernel�ɑ΂��č����N���Ή� 	*
* 		���s���邪�A�����͕s���B 											*
* 		SW_BUDET#��GPIO INPUT�ɂēǂݍ���ł���肵��LOW���擾���ꔻ�f�ł��Ȃ�
* 		SW_BUDET#��GPIO INPUT�ɐݒ肷���Ipod�F�؃h���C�o�A�N�Z�X����i2C�� 	*
* 		Warnning����ʂɔ������AIpod�F�؃`�b�v�̓ǂݏ��������j�Q 			*
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
#define IF_GPIO			1	/* GPIO���� */
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
#define IPD_TIMER_SOMI_READY		1000	/* 50us->1ms iPod����d�l(03��)*/
#define IPD_TIMER_SOMI_RELEASE		1000	/* 50us->1ms iPod����d�l(03��)*/
#define IPD_TIMER_nSS_DEASSERT		1000	/* 300us->1ms iPod����d�l(03��)*/
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

static bool	pre_suspend_flg=FALSE;	/* �����N���Ή�*/
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
	IPODAUTH_STATE_NONE = 0,			/* ��ԂȂ� 			*/
	IPODAUTH_STATE_INIT,				/* ��������� 			*/
	IPODAUTH_STATE_BEGIN,				/* �J�n 				*/
	IPODAUTH_STATE_IDLE,				/* �A�C�h��				*/
	IPODAUTH_STATE_RUN,					/* ���s 				*/
	IPODAUTH_STATE_MAX
};
enum ipodauth_retcode{
	IPODAUTH_NONE	= 0,				/* �Ȃ� 		*/
	IPODAUTH_COMP	= 1,				/* ���� 		*/
	IPODAUTH_ERR	=-1,				/* �G���[ 		*/
	IPODAUTH_PRE_SUSPEND=-4				/* �T�X�y���h 	*/
};

/*--------------------------------------------------------------------------*/
/* 	struct 																	*/
/*--------------------------------------------------------------------------*/
/* �^�C�} �R���t�B�O*/
typedef	struct{
	enum timer_id	id;
	char			str[32];
	int				ms;
}ST_IPA_TIMER_CONFIG;
/* �^�C�} */
typedef	struct{
	void*					pthis;
    int 					index;
	ST_IPA_TIMER_CONFIG		config;
    struct hrtimer 			timer;
}ST_IPA_TIMER_SET;
/* ��ԑJ�� */
typedef	struct{
	enum ipodauth_state state;
	char			str[32];
}ST_IPA_IPODAUTH_FSM;
/* �L���[ */
typedef	struct{
	wait_queue_head_t		waitQue;
	bool					waitting;
}ST_IPA_WAIT_QUE;
/* ���[�J�[�X���b�h */
typedef	struct{
	struct	work_struct 		workq;
	enum worker_message_type	Msgtype;
	void*						pthis;
}ST_IPA_WORKER_INFO;

/* �C���X�^���X */
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
					/* 2013/03/20 �����]�T���������Ăق�����FTEN�a���˗� +5ms */
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
*		DESCRIPTION	: �^�C�}�[�L�����Z��									*
*																			*
*		PARAMETER	: IN  : *iad : �R���e�L�X�g								*
*																			*
*					  IN  : �^�C�}ID										*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ipa_timer_cancel(ST_IPA_DATA* iad,enum timer_id id)
{
	/*--------------------------------------*/
	/*  �^�C�}�L�����Z��					*/
	/*--------------------------------------*/
	hrtimer_cancel(&iad->timer_set[id].timer);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_timer_start									*
*																			*
*		DESCRIPTION	: �^�C�}�[�X�^�[�g										*
*																			*
*		PARAMETER	: IN  : *iad : �R���e�L�X�g								*
*																			*
*					  IN  : �^�C�}ID										*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ipa_timer_start(ST_IPA_DATA* iad,enum timer_id id)
{
	/*--------------------------------------*/
	/*  �^�C�}�X�^�[�g						*/
	/*--------------------------------------*/
	 hrtimer_start(&iad->timer_set[id].timer, 
	 				ktime_set( 0, MS2NS(iad->timer_set[id].config.ms)), 
	 				HRTIMER_MODE_REL);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_timer_callback										*
*																			*
*		DESCRIPTION	: �^�C�}�[�R�[���o�b�N									*
*																			*
*		PARAMETER	: IN  : *timer : �^�C�}�f�[�^							*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static enum hrtimer_restart fc_ipa_timer_callback(struct hrtimer *timer)
{
	enum hrtimer_restart ret= HRTIMER_NORESTART;
	ST_IPA_TIMER_SET* ts;
	
	/*--------------------------------------*/
	/*  �^�C�}�f�[�^�擾					*/
	/*--------------------------------------*/
	ts = container_of(timer, ST_IPA_TIMER_SET, timer);
	if(ts){
		ST_IPA_DATA* iad=(ST_IPA_DATA*)ts->pthis;	/* Instance�擾 */
		/*--------------------------------------*/
		/*  ���[�J�[�X���b�h��Timer�C�x���g���M	*/
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
*		DESCRIPTION	: �^�C�}����������										*
*																			*
*		PARAMETER	: IN  : *iad : �R���e�L�X�g								*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static int fc_ipa_timer_init(ST_IPA_DATA* iad)
{
	int status=ESUCCESS;
	int i;
	/*--------------------------------------*/
	/*  �^�C�}������						*/
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
*		DESCRIPTION	: gpio�o�^�����񐶐�									*
*																			*
*		PARAMETER	: IN  : *iad : �R���e�L�X�g								*
*																			*
*					  OUT : *out : �A���f�[�^�i�[��							*
*																			*
*					  IN  : *str : ���j�[�N������							*
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
*		DESCRIPTION	: GPIO OUT�ݒ�											*
*																			*
*		PARAMETER	: IN  : *iad : �R���e�L�X�g								*
*																			*
*					  IN  : ballname : �|�[�g��								*
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
*		DESCRIPTION	: �T�X�y���h����i�����N���Ή��j						*
*																			*
*		PARAMETER	: IN  : NONE											*
*																			*
*					  RET : TRUE[SUSPEND��]	/ FALSE[SUSPEND���łȂ�]		*
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
*		DESCRIPTION	: �T�X�y���h����i�����N���Ή��j						*
*																			*
*		PARAMETER	: IN  : NONE											*
*																			*
*					  RET : TRUE[SUSPEND��]	/ FALSE[SUSPEND���łȂ�]		*
*																			*
****************************************************************************/
static inline bool fc_ipa_checkPreSuspend(void)
{
	int retval = 0;
	/*---------------------------------------------------------*/
	/*  BU-DET/ACC���荞�݃`�F�b�N                             */
	/*    BU-DET ���荞�݂���re-init���O�܂ł𔻒�             */
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
*		DESCRIPTION	: ��ԑJ��												*
*																			*
*		PARAMETER	: IN  : *iad : �R���e�L�X�g								*
*																			*
*					  IN  : state	:	�ύX��X�e�[�g						*
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
*		DESCRIPTION	: CS#����												*
*																			*
*		PARAMETER	: IN  : *iad : �R���e�L�X�g								*
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
*		DESCRIPTION	: SFIO(Special Function IO)�֐ݒ�						*
*																			*
*		PARAMETER	: IN  : *iad : �R���e�L�X�g								*
*																			*
*					  IN  : ballname	:	�|�[�g��						*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline void fc_ipa_set_sfio(ST_IPA_DATA *iad,unsigned long ballname)
{
#if INTERFACE == IF_SPI_TEGRA	/* SPI-TEGRA�ɂ��ʐM */
	tegra_gpio_disable(ballname);
#else							/* GPIO����ɂ��ʐM */
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
*		DESCRIPTION	: ����������											*
*																			*
*		PARAMETER	: IN  : *iad : �R���e�L�X�g								*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ipa_init(ST_IPA_DATA *iad)
{
	char 	request_string[128]={0};
	/*--------------------------------------*/
	/*  GPIO�������ݒ�						*/
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
	/*  �^�C�}������						*/
	/*--------------------------------------*/
	fc_ipa_timer_init(iad);
	/*--------------------------------------*/
	/*  �X�e�[�g������						*/
	/*--------------------------------------*/
	fc_ipa_changeState(iad,IPODAUTH_STATE_INIT);
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_signalreset									*
*																			*
*		DESCRIPTION	: �|�[�g���Z�b�g										*
*																			*
*		PARAMETER	: IN  : *iad : �R���e�L�X�g								*
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
*		DESCRIPTION	: �ʐM���O����											*
*																			*
*		PARAMETER	: IN  : *iad : �R���e�L�X�g								*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static void fc_ipa_prepare_communication(ST_IPA_DATA *iad)
{
	/*--------------------------------------*/
	/*  SFIO�ɐݒ�							*/
	/*--------------------------------------*/
	fc_ipa_set_sfio(iad,IPOD_TXD);
	fc_ipa_set_sfio(iad,IPOD_RXD);
	fc_ipa_set_sfio(iad,IPOD_CLK);
	/*--------------------------------------*/
	/*  IDLE�֑J��							*/
	/*--------------------------------------*/
	fc_ipa_changeState(iad,IPODAUTH_STATE_IDLE);
	/*--------------------------------------*/
	/*  �҂���ԉ���						*/
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
*		DESCRIPTION	: SPI����M�����R�[���o�b�N								*
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
*		DESCRIPTION	: SPI�����ʐM											*
*																			*
*		PARAMETER	: IN  : *iad : 	�R���e�L�X�g							*
*																			*
*					: IN  : *message : ���b�Z�[�W							*
*																			*
*					  RET : �T�C�Y	/	���^�[���R�[�h						*
*																			*
****************************************************************************/
static ssize_t fc_ipa_sync(ST_IPA_DATA *iad, struct spi_message *message)
{
	int status;
	DECLARE_COMPLETION_ONSTACK(done);

	/*--------------------------------------*/
	/*  ���b�Z�[�W����						*/
	/*--------------------------------------*/
	message->complete = fc_ipa_complete;
	message->context = &done;

	/*--------------------------------------*/
	/*  master�֑��M						*/
	/*--------------------------------------*/
	spin_lock_irq(&iad->spi_lock);
	if (iad->spi == NULL)
		status = -ESHUTDOWN;
	else
		status = spi_async(iad->spi, message);
	spin_unlock_irq(&iad->spi_lock);

	/*--------------------------------------*/
	/*  �҂�����							*/
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
*		DESCRIPTION	: SPI���M												*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  IN  : *buf   : �f�[�^									*
*																			*
*					  IN  : length : �����O�X								*
*																			*
*					  RET : �T�C�Y / ���^�[���R�[�h							*
*																			*
****************************************************************************/
static inline ssize_t fc_ipa_syncWrite(ST_IPA_DATA *iad,char* buf,int length)
{
	ssize_t ret = 0;
	ssize_t spi_ret=0;
	volatile unsigned int	j;
	volatile unsigned int	i;

	/*--------------------------------------*/
	/*  �����O�X�����M	1�o�C�g�Â��M		*/
	/*--------------------------------------*/
	for(j=0;j<length;++j){
#if INTERFACE == IF_SPI_TEGRA	/* SPI-TEGRA�ɂ��ʐM */
		UNUSED(i);
		struct spi_transfer	t = {
				.tx_buf		= buf+j,
				.len		= 1,//length,
			};
		struct spi_message	m;

		/*--------------------------------------*/
		/*  ���b�Z�[�W�̐����Ƒ��M				*/
		/*--------------------------------------*/
		spi_message_init(&m);
		spi_message_add_tail(&t, &m);
		spi_ret = fc_ipa_sync(iad, &m);
#elif INTERFACE == IF_GPIO		/* GPIO����ɂ��ʐM */
		/*--------------------------------------*/
		/*  GPIO����ɂđ��M					*/
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
			__delay(700);	/* 2013/03/20	TX�Z�b�g�A�b�v���Ԃ�0.089us�̌v�����ʂɂ�NG 1us�J���� */
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
		if(0 == spi_ret)				/* �����O�X�[�� */
			ret = 0;
		if(TRUE == fc_ipa_checkPreSuspend())	/* �T�X�y���h 	*/
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
*		DESCRIPTION	: SPI��M												*
*																			*
*		PARAMETER	: IN  : *iad : �R���e�L�X�g								*
*																			*
*					  IN  : *b   	: �f�[�^								*
*																			*
*					  IN  : length  : �����O�X								*
*																			*
*					  RET : �T�C�Y / ���^�[���R�[�h							*
*																			*
****************************************************************************/
static inline ssize_t fc_ipa_syncRead(ST_IPA_DATA *iad,char* tx_buf,char* rx_buf,int length)
{
	ssize_t ret = 0;
	ssize_t spi_ret=0;

	volatile unsigned int	j;
	volatile unsigned int	i;
	for(j=0;j<length;++j){
#if INTERFACE == IF_SPI_TEGRA		/* SPI-TEGRA�ɂ��ʐM */
		UNUSED(i);
		struct spi_transfer	t = {
				.rx_buf		= rx_buf+j,
				.tx_buf		= tx_buf+j,
				.len		= 1,//length,
			};
		struct spi_message	m;

		/*--------------------------------------*/
		/*  ���b�Z�[�W�̐����Ǝ�M				*/
		/*--------------------------------------*/
		spi_message_init(&m);
		spi_message_add_tail(&t, &m);
		spi_ret=fc_ipa_sync(iad, &m);
#elif INTERFACE == IF_GPIO			/* GPIO����ɂ��ʐM */
		/*--------------------------------------*/
		/*  GPIO����ɂđ��M					*/
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
		if(0 == spi_ret)				/* �����O�X�[�� */
			ret = 0;
		if(TRUE == fc_ipa_checkPreSuspend())	/* �T�X�y���h */
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
*		DESCRIPTION	: �R�}���h���M											*
*																			*
*		PARAMETER	: IN  : *iad : �R���e�L�X�g								*
*																			*
*					  IN  : *pucSnd_data  : �f�[�^							*
*																			*
*					  RET : �T�C�Y / ���^�[���R�[�h							*
*																			*
****************************************************************************/
static ssize_t fc_ipa_SendCmdAdr(ST_IPA_DATA *iad,unsigned char *pucSnd_data)
{
	/*--------------------------------------*/
	/*  CS#H �l�K�e�B�u						*/
	/*--------------------------------------*/
	fc_ipa_setCS(iad,INACTIVE);
	__udelay(IPD_TIMER_nSS_DEASSERT);
	/*--------------------------------------*/
	/*  CS#L �A�N�e�B�u						*/
	/*--------------------------------------*/
	fc_ipa_setCS(iad,ACTIVE);
	/*--------------------------------------*/
	/*  MISO��GPIO�ݒ��					*/
	/*--------------------------------------*/
	fc_ipa_set_gpio_output(iad,IPOD_RXD,HIGH);
	__udelay(IPD_TIMER_SOMI_READY);
	/*--------------------------------------*/
	/*  MISO��SFIO�ݒ�ɖ߂�				*/
	/*--------------------------------------*/
	fc_ipa_set_sfio(iad,IPOD_RXD);
	return fc_ipa_syncWrite(iad,pucSnd_data,IPD_COMMAND_SIZE);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_Recv											*
*																			*
*		DESCRIPTION	: IPOD�F�؎�M											*
*																			*
*		PARAMETER	: IN  : *iad : �R���e�L�X�g								*
*																			*
*					  IN  : pucSnd_data	: ���M�o�b�t�@						*
*																			*
*					  IN  : ulSndLength	: ���M�����O�X						*
*																			*
*					  IN  : ulRcvLength	: ��M�����O�X						*
*																			*
*					  IN  : pucRcv_data	: ��M�o�b�t�@						*
*																			*
*					  OUT : pulOutlength: ��M�����O�X						*
*																			*
*					  RET : �T�C�Y / ���^�[���R�[�h							*
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
	/*  �R�}���h���M						*/
	/*--------------------------------------*/
	ssize_t ret= fc_ipa_SendCmdAdr(iad,pucSnd_data);
	if(IPD_COMMAND_SIZE==ret){	/* �T�C�Y�m�F */
		fc_ipa_set_gpio_output(iad,IPOD_RXD,LOW);
		__udelay(IPD_TIMER_SOMI_READY);
		fc_ipa_set_sfio(iad,IPOD_RXD);
		ret = ESUCCESS;
		/*--------------------------------------*/
		/*  MISO HIGH�҂�						*/
		/*--------------------------------------*/
		for(i=-1;i;--i){/* ����LOOP�h�~ */
			ret=fc_ipa_checkPreSuspend()?IPD_PRE_SUSPEND_RETVAL:ESUCCESS;
			if(gpio_get_value(IPOD_RXD) || ret!=ESUCCESS)
				break;
		}
		if(0==i)
			ret = -EFAULT;
		if(ret==ESUCCESS){
			/*--------------------------------------*/
			/*  �ǂݍ���							*/
			/*--------------------------------------*/
			fc_ipa_set_sfio(iad,IPOD_RXD);
			ret=fc_ipa_syncRead(iad,pucSnd_data+IPD_COMMAND_SIZE,pucRcv_data,ulRcvLength);
			if(0 < ret
			&& ret == ulRcvLength){
				*pulOutlength=ret;
			}else
				*pulOutlength=0;
			/*--------------------------------------*/
			/*  �㏈��								*/
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
*		DESCRIPTION	: IPOD�F�ؑ��M											*
*																			*
*		PARAMETER	: IN  : *iad : �R���e�L�X�g								*
*																			*
*					  OUT : *buf  : �f�[�^									*
*																			*
*					  OUT : length: �����O�X								*
*																			*
*					  RET : �T�C�Y / ���^�[���R�[�h							*
*																			*
****************************************************************************/
static ssize_t fc_ipa_Send(ST_IPA_DATA *iad,char* buf,int length)
{
	signed long i;
	/*--------------------------------------*/
	/*  �R�}���h���M						*/
	/*--------------------------------------*/
	ssize_t ret = fc_ipa_SendCmdAdr(iad,buf);
	if(IPD_COMMAND_SIZE==ret){
		fc_ipa_set_gpio_output(iad,IPOD_RXD,LOW);
		__udelay(IPD_TIMER_SOMI_READY);
		fc_ipa_set_sfio(iad,IPOD_RXD);
		ret = ESUCCESS;
		/*--------------------------------------*/
		/*  MISO HIGH�҂�						*/
		/*--------------------------------------*/
		for(i=-1;i;--i){/* ����LOOP�h�~ */
			ret=fc_ipa_checkPreSuspend()?IPD_PRE_SUSPEND_RETVAL:ESUCCESS;
			if(gpio_get_value(IPOD_RXD) || ret!=ESUCCESS)
				break;
		}
		if(0 == i)
			ret = -1;
		if(ESUCCESS == ret){
			/*--------------------------------------*/
			/*  ��������							*/
			/*--------------------------------------*/
			fc_ipa_set_sfio(iad,IPOD_RXD);
			ret=fc_ipa_syncWrite(iad,buf+IPD_COMMAND_SIZE,length-IPD_COMMAND_SIZE);
			/*--------------------------------------*/
			/*  �㏈��								*/
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
*		DESCRIPTION	: ���[�J�[�X���b�h										*
*																			*
*		PARAMETER	: IN  : work : ���[�N�f�[�^								*
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
	/*  �������							*/
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
	/*  �J�n���							*/
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
	/*  �A�C�h�����						*/
	/*--------------------------------------*/
	case IPODAUTH_STATE_IDLE:
		switch(wi->Msgtype){
		default:
			LOGW("IPODAUTH_STATE_IDLE default Msgtype=%d\n",wi->Msgtype);
			break;
		}
		break;
	/*--------------------------------------*/
	/*  ���s���							*/
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
*		DESCRIPTION	: �㏈��												*
*																			*
*		PARAMETER	: IN  : iad : �R���e�L�X�g								*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline void fc_ipa_decreate(ST_IPA_DATA *iad)
{
	int i;
	/*--------------------------------------*/
	/*  ���[�J�[�X���b�h�폜				*/
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
	/*  �`�b�v���Z�b�g						*/
	/*--------------------------------------*/
	fc_ipa_signalreset(iad);
	gpio_set_value(IPOD_RESET, LOW);
	/*--------------------------------------*/
	/*  GPIO�J��							*/
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
*		DESCRIPTION	: ��������												*
*																			*
*		PARAMETER	: IN  : iad : �R���e�L�X�g								*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
static inline int fc_ipa_concreate(ST_IPA_DATA *iad)
{
	int	status=0;
	fc_ipa_init(iad);
	/*--------------------------------------*/
	/*  ���[�J�[�X���b�h����				*/
	/*--------------------------------------*/
	iad->workqueue = create_singlethread_workqueue("spi-ipodauth-work");
	if (iad->workqueue){
		int i;
		/*--------------------------------------*/
		/*  ���[�J�[�X���b�h��񐶐�			*/
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
			/*  SPI-TEGRA�̐ݒ�						*/
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
*		PARAMETER	: IN  : filp : �t�@�C��									*
*																			*
*					: IN  : cmd  : �R�}���h									*
*																			*
*					: IN  : arg												*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static long fs_ipa_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int			err = 0;
	int			retval = ESUCCESS;
	
	ST_IPA_DATA	*iad;
	ST_KDV_IPD_IOC_WR	stWrData;
	ST_KDV_IPD_IOC_RD	stRdData;
	unsigned char		ucWRSndData[IPD_DATA_MAXLEN]={0};	/* ���M�f�[�^(��������)			*/
	unsigned char		ucRDSndData[IPD_DATA_MAXLEN]={0};	/* ���M�f�[�^(�ǂݍ���)			*/
	unsigned char		ucRcvData[IPD_DATA_MAXLEN]={0};		/* ��M�f�[�^					*/
	unsigned long		ulOutLength=0;						/* ��M�T�C�Y					*/

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
	/*  IDLE��ԂłȂ��Ȃ珀���҂�����		*/
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
	/*  �ǂݏ�������						*/
	/*--------------------------------------*/
	if(ESUCCESS == retval){
		/* �J�n�C�x���g���M */
		fc_ipa_changeState(iad,IPODAUTH_STATE_RUN);
		switch (cmd) {
		/*--------------------------------------*/
		/*  ��������							*/
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
		/*  �ǂݍ���							*/
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
#ifdef IPODAUTHDEBUG	/* �f�o�b�N�p */
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
		/* �I���C�x���g���M */
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
*		PARAMETER	: IN  : filp : �t�@�C��									*
*																			*
*					: IN  : cmd  : �R�}���h									*
*																			*
*					: IN  : arg												*
*																			*
*					  RET : ���^�[���R�[�h									*
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
*		DESCRIPTION	: Resume���O�ʒm										*
*																			*
*		PARAMETER	: IN  : dev : �f�o�C�X�t�@�C��							*
*																			*
*					  RET : ���^�[���R�[�h									*
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
*		DESCRIPTION	: Suspend���O�ʒm										*
*																			*
*		PARAMETER	: IN  : dev : �f�o�C�X�t�@�C��							*
*																			*
*					  RET : ���^�[���R�[�h									*
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
*		PARAMETER	: IN  : *inode : �m�[�h									*
*																			*
*					  IN  : *filp  : �t�@�C��								*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static int fs_ipa_open(struct inode *inode, struct file *filp)
{
	ST_IPA_DATA	*iad;
	int		status = -EFAULT;
	
	mutex_lock(&device_list_lock);
	/*--------------------------------------*/
	/*  �C���X�^���X�擾					*/
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
			/*  �X�e�[�g������						*/
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
*		PARAMETER	: IN  : *inode : �m�[�h									*
*																			*
*					  IN  : *filp  : �t�@�C��								*
*																			*
*					  RET : ���^�[���R�[�h									*
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
		/*	���[�U���`�F�b�N								*/
		/*--------------------------------------------------*/
		if(iad->users > 0) {
			iad->users --;
		}
		if(iad->users <= 0){
			iad->users= 0;
			/*--------------------------------------------------*/
			/*	�h���C�o��񏉊���								*/
			/*--------------------------------------------------*/
			filp->private_data = NULL;
			/*--------------------------------------*/
			/*  �^�C�}�L�����Z��					*/
			/*--------------------------------------*/
			for(i=0;i<TIMER_ID_MAX;++i)
				fc_ipa_timer_cancel(iad,i);
			/*--------------------------------------*/
			/*  ������Ԃ֑J�ڂ��I������			*/
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
*		PARAMETER	: IN  : *spi : spi�f�[�^								*
*																			*
*					  IN  : mesg : ���b�Z�[�W								*
*																			*
*					  RET : ���^�[���R�[�h									*
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
	/*  �`�b�v���Z�b�g						*/
	/*--------------------------------------*/
	fc_ipa_signalreset(iad);
	gpio_set_value(IPOD_RESET, LOW);
	/*--------------------------------------*/
	/*  Timer cancel						*/
	/*--------------------------------------*/
	for(i=0;i<TIMER_ID_MAX;++i)
		fc_ipa_timer_cancel(iad,i);
	/*--------------------------------------*/
	/*  �X�e�[�g������						*/
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
*		PARAMETER	: IN  : *spi : spi�f�[�^								*
*																			*
*					  RET : ���^�[���R�[�h									*
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
	/*  �`�b�v���Z�b�g						*/
	/*--------------------------------------*/
	fc_ipa_signalreset(iad);
	gpio_set_value(IPOD_RESET, LOW);
	/*--------------------------------------*/
	/*  Timer cancel						*/
	/*--------------------------------------*/
	for(i=0;i<TIMER_ID_MAX;++i)
		fc_ipa_timer_cancel(iad,i);
	/*--------------------------------------*/
	/*  �X�e�[�g������						*/
	/*--------------------------------------*/
	fc_ipa_changeState(iad,IPODAUTH_STATE_INIT);
	return 0;
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ipa_show_adapter_name								*
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
*		PARAMETER	: IN  : spi�f�[�^										*
*																			*
*					  RET : ���^�[���R�[�h									*
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
	/*  �f�[�^������						*/
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
		/*  �f�o�C�X�t�@�C������				*/
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
		/*  �����N���R�[���o�b�N�o�^			*/
		/*--------------------------------------*/
		iad->ed.reinit = NULL;//reiint�͏������Ă��Ȃ��̂�NULL�ݒ�
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
*		PARAMETER	: IN  : spi�f�[�^										*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static int __devexit fs_ipa_remove(struct spi_device *spi)
{
	ST_IPA_DATA	*iad = spi_get_drvdata(spi);
	LOGT("%s:(%d)\n",__FUNCTION__,__LINE__);

	spin_lock_irq(&iad->spi_lock);
	iad->spi = NULL;
	spi_set_drvdata(spi, NULL);					/*  SPI�o�^����					*/
	spin_unlock_irq(&iad->spi_lock);

	mutex_lock(&device_list_lock);
	early_device_unregister(&iad->ed);			/*  ���O�ʒm�o�^����			*/
	list_del(&iad->device_entry);			
	device_destroy(ipodauth_class, iad->devt);	/*  �N���X�폜					*/
	clear_bit(MINOR(iad->devt), minors);
	fc_ipa_decreate(iad);						/*  �㏈��						*/
	kfree(iad);									/*  �C���X�^���X�폜			*/
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
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static int __init fs_ipa_init(void)
{
	int status;
	
	printk(KERN_INFO "%s %s:(%d)\n",DRV_NAME,__FUNCTION__,__LINE__);

	/*--------------------------------------*/
	/*  chardev�o�^							*/
	/*--------------------------------------*/
	IPODAUTH_MAJOR = register_chrdev(IPODAUTH_MAJOR, DRV_NAME, &ipodauth_fops);
	if (IPODAUTH_MAJOR < 0){
		return IPODAUTH_MAJOR;
	}
	/*--------------------------------------*/
	/*  �N���X�쐬							*/
	/*--------------------------------------*/
	ipodauth_class = class_create(THIS_MODULE, DRV_NAME);
	if (IS_ERR(ipodauth_class)) {
		unregister_chrdev(IPODAUTH_MAJOR, spi_ipodauth_driver.driver.name);
		return PTR_ERR(ipodauth_class);
	}
	
	/*--------------------------------------*/
	/*  SPI�o�^								*/
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
