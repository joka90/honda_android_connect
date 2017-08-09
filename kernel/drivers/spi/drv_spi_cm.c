/*--------------------------------------------------------------------------*/
/* COPYRIGHT(C) FUJITSU TEN LIMITED 2012-2014                               */
/*--------------------------------------------------------------------------*/
/****************************************************************************
*																			*
*		�V���[�Y����	: �P�S�l�x											*
*		�}�C�R������	: ���C���}�C�R��									*
*		�I�[�_�[����	: �P�S�l�x�@�`�������������|�c�`					*
*		�u���b�N����	: �k���������J�[�l���h���C�o						*
*		DESCRIPTION		: �b�o�t�ԒʐM�h���C�o								*
*																			*
*		�t�@�C������	: drv_spi_cm.c										*
*		��@���@��		: Micware)ogura & segawa							*
*		��@���@��		: 2013/01/25	<Ver. 1.00>							*
*		���@���@��		: yyyy/mm/dd	<Ver. x.xx>							*
*																			*
*		�w�V�X�e���h�e���[�`���x											*
*			module_info_def ...........	�k���������h���C�o�o�^��`			*
*			fs_ccd_sys_init ...........	�h���C�o����������					*
*			fs_ccd_sys_exit ...........	�h���C�o�I������					*
*			fs_ccd_sys_probe ..........	�h���C�o��������					*
*			fs_ccd_sys_remove..........	�h���C�o�폜����					*
*			fs_ccd_sys_open ...........	�h���C�o�I�[�v������				*
*			fs_ccd_sys_close ..........	�h���C�o�N���[�Y����				*
*			fs_ccd_sys_suspend.........	�h���C�o�T�X�y���h����				*
*			fs_ccd_sys_resume .........	�h���C�o���W���[������				*
*			fs_ccd_sys_read............	�h���C�o���[�h����					*
*			fs_ccd_sys_write ..........	�h���C�o���C�g����					*
*																			*
*		�w�����N���h�e���[�`���x											*
*			fs_ccd_fb_sdn_sdwn.........	�����N���E�I���ʒm����				*
*			fc_ccd_fb_chk_sus .........	�����N���E�T�X�y���h�m�F����		*
*																			*
*		�w���荞�݂h�e���[�`���x											*
*			fs_ccd_intr_int_sig .......	�h�m�s�M�����荞�ݏ���				*
*																			*
*		�w�^�C�}�h�e���[�`���x												*
*			fs_ccd_timeout_cb .........	�^�C���A�E�g�R�[���o�b�N			*
*																			*
*		�w�X���b�h���[�`���x												*
*			fc_ccd_wthrd_main .........	���[�J�[�X���b�h���C������			*
*																			*
*		�w��ԑJ�ڃ��[�`���x												*
*			fc_ccd_fsm_snd_req ........	��ԑJ�ڏ����u���M�v���v			*
*			fc_ccd_fsm_snd_cmplt ......	��ԑJ�ڏ����u���M�����v			*
*			fc_ccd_fsm_rcv_cmplt ......	��ԑJ�ڏ����u��M�����v			*
*			fc_ccd_fsm_int_l ..........	��ԑJ�ڏ����u�h�m�s�|�k�����݁v	*
*			fc_ccd_fsm_int_h ..........	��ԑJ�ڏ����u�h�m�s�|�g�����݁v	*
*			fc_ccd_fsm_int_pulse ......	��ԑJ�ڏ����u�h�m�s�p���X�����݁v	*
*			fc_ccd_fsm_tout_clk1 ......	��ԑJ�ڏ����u���������P�@�s�D�n�D�v*
*			fc_ccd_fsm_tout_csh .......	��ԑJ�ڏ����u���������@�s�D�n�D�v	*
*			fc_ccd_fsm_tout_idle ......	��ԑJ�ڏ����u�����������@�s�D�n�D�v*
*			fc_ccd_fsm_tout_fail1 .....	��ԑJ�ڏ����u����������1 �s�D�n�D�v*
*			fc_ccd_fsm_tout_comn ......	��ԑJ�ڏ����u�ėp�^�C�}�@�s�D�n�D�v*
*			fc_ccd_fsm_timout .........	��ԑJ�ڃ^�C���A�E�g����			*
*																			*
*		�w�T�u���[�`���x													*
*			fc_ccd_init_instance ......	�C���X�^���X����������				*
*			fc_ccd_make_gpio_str ......	�f�o�h�n�o�^�����񐶐�				*
*			fc_ccd_calc_fcc ...........	�e�b�b�v�Z����						*
*			fc_ccd_snd_dat ............	�f�[�^���M����						*
*			fc_ccd_rcv_dat ............	�f�[�^��M����						*
*			fc_ccd_snd_byte ...........	�P�o�C�g���M�����i�|�[�g����j		*
*			fc_ccd_rcv_byte ...........	�P�o�C�g��M�����i�|�[�g����j		*
*			fc_ccd_snd_spi_res ........	���M�����i�V���A�����\�[�X����j	*
*			fc_ccd_rcv_spi_res ........	��M�����i�V���A�����\�[�X����j	*
*			fc_ccd_spicomm_ctrl .......	�r�o�h����M���䏈��				*
*			fc_ccd_spicomm_cb .........	�r�o�h�]�������R�[���o�b�N			*
*			fc_ccd_tim_init ...........	�^�C�}������						*
*			fc_ccd_tim_start ..........	�^�C�}�X�^�[�g						*
*			fc_ccd_tim_stop ...........	�^�C�}��~							*
*			fc_ccd_tim_all_stop .......	�S�^�C�}��~						*
*			fc_ccd_set_sig_cs .........	�b�r�M�����䏈��					*
*			fc_ccd_get_sig_int ........	�h�m�s�M���擾����					*
*			fc_ccd_rel_res ............	���\�[�X�J������					*
*			fc_ccd_fsm_clr ............	��ԑJ�ڃN���A����					*
*			fc_ccd_init_drv_inf .......	�h���C�o��񏉊�������				*
*			fc_ccd_chk_rcv_dat ........	��M�f�[�^�m�F����					*
*			fc_ccd_set_rcv_lst ........	��M���X�g�i�[����					*
*			fc_ccd_chg_fsm_mod ........	��ԑJ�ڏ�ԕύX����				*
*			fc_ccd_rdy_shtdwn .........	�V���b�g�_�E����������				*
*			fc_ccd_disable_irq ........	�h���C�o�����݋֎~����				*
*			fc_ccd_enable_irq .........	�h���C�o�����݋�����				*
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
	M_CCD_MCR_MS2US(  2),					/* No.0�F���������P		2ms			*/
///�d�l�ύX	M_CCD_MCR_MS2US(  1),					/* No.1�F��������		1ms			*/
	500,									/* No.1�F��������		500us			*/
	M_CCD_MCR_MS2US(  3),					/* No.2�F����������		3ms			*/
	M_CCD_MCR_MS2US(100),					/* No.3�F�����������P	100ms		*/
	M_CCD_MCR_MS2US(M_CCD_COMTIM_TOUT_MS),	/* No.4�F�ėp			50ms		*/
	M_CCD_MCR_MS2US(  1),					/* No.5�FINT FALESAFE	1ms			*/	
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
*		DESCRIPTION	: �k���������h���C�o�o�^��`							*
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
/*	�o�^/�폜�֐��o�^								*/
/*--------------------------------------------------*/
module_init(fs_ccd_sys_init);
module_exit(fs_ccd_sys_exit);

/*--------------------------------------------------*/
/*	���C�Z���X�o�^									*/
/*--------------------------------------------------*/
MODULE_LICENSE("GPL");

/*--------------------------------------------------*/
/*	���W���[������`								*/
/*--------------------------------------------------*/
MODULE_ALIAS("spi:cpucom");

/*--------------------------------------------------*/
/*	�k���������V�X�e���C���^�[�t�F�[�X�o�^			*/
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
*		DESCRIPTION	: �h�����R���O�o��(�f�o�b�N�p)							*
*																			*
*		PARAMETER	: IN  : format											*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
static void fc_ccd_drc_out(char* format, ...){
	struct lites_trace_header trc_hdr;	/* �h�����R�p�w�b�_�[ 		*/
	struct lites_trace_format trc_fmt;	/* �h�����R�p�t�H�[�}�b�g 	*/
    va_list arg;
	char buf[256]={0};
    va_start(arg, format);
    vsprintf(buf, format, arg);
    va_end(arg);
	
	memset(&trc_fmt, 0x0, sizeof(trc_fmt));
	memset(&trc_hdr, 0x0, sizeof(trc_hdr));

	trc_fmt.buf			= buf;
	trc_fmt.count		= strnlen(buf,sizeof(buf)-1)+1;
	trc_fmt.trc_header	= &trc_hdr;     /* �h�����R�p�w�b�_�[�i�[ 	*/
	trc_fmt.trc_header->level = 2;    	/* ���O���x�� 0���ŗD��		*/
	trc_fmt.trc_header->trace_id  = 0;
										/* ���g�p�g�F�����O�X�����	*/
	trc_fmt.trc_header->format_id= (unsigned short)0xFF;	
										/*��ʂ��i�[  				*/
	trc_fmt.trc_header->trace_no = LITES_KNL_LAYER+DRIVERECODER_CPUCOMDRIVER_NO;
										/* CPU�ԒʐM�h���C�o���w�� 	*/
	lites_trace_write(REGION_TRACE_AVC_LAN, &trc_fmt);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_drc_error										*
*																			*
*		DESCRIPTION	: �h���C�o�E�G���[�ۑ�����								*
*																			*
*		PARAMETER	: IN  : err_id	�G���[ID								*
*					: IN  : param1	�p�����[�^1								*
*					: IN  : param2	�p�����[�^2								*
*					: IN  : param3	�p�����[�^3								*
*					: IN  : param4	�p�����[�^4								*
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
	struct lites_trace_header trc_hdr;	/* �h�����R�p�w�b�_�[ 		*/
	struct lites_trace_format trc_fmt;	/* �h�����R�p�t�H�[�}�b�g 	*/
	struct driverecorder_error_data{
		unsigned int	err_id;	/* �G���[�h�c */
		unsigned int	param1;	/* �t�����P */
		unsigned int	param2;	/* �t�����Q */
		unsigned int	param3;	/* �t�����R */
		unsigned int	param4;	/* �t�����S */
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
	/* �f�[�^�쐬����lites�h���C�o��������				*/
	/*--------------------------------------------------*/

	trc_fmt.buf		=  &err;
	trc_fmt.count	=  sizeof(err);		
	trc_fmt.trc_header	= &trc_hdr;     /* �h�����R�p�w�b�_�[�i�[ 	*/
	trc_fmt.trc_header->level = 2;    	/* ���O���x�� 0���ŗD��		*/
	trc_fmt.trc_header->trace_id  = 0;	/* ���g�p 					*/
	trc_fmt.trc_header->format_id= (unsigned short)0;	/* �Œ� 	*/
	trc_fmt.trc_header->trace_no = LITES_KNL_LAYER+DRIVERECODER_CPUCOMDRIVER_NO;
										/* CPU�ԒʐM�h���C�o���w�� 	*/
										
	/* �����ɏ������� */
	lites_trace_write(REGION_TRACE_ERROR, &trc_fmt);
	lites_trace_write(REGION_TRACE_AVC_LAN, &trc_fmt);
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_drc_event										*
*																			*
*		DESCRIPTION	: �h���C�o�Eevent�ۑ�����								*
*																			*
*		PARAMETER	: IN  : err_id	ID										*
*					: IN  : param1	�p�����[�^1								*
*					: IN  : param2	�p�����[�^2								*
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
	struct lites_trace_header trc_hdr;	/* �h�����R�p�w�b�_�[ 		*/
	struct lites_trace_format trc_fmt;	/* �h�����R�p�t�H�[�}�b�g 	*/
	struct driverecorder_error_data{
		unsigned int	event_id;	/* �G���[�h�c */
		unsigned int	param1;	/* �t�����P */
		unsigned int	param2;	/* �t�����Q */
	}err={
		.event_id = event_id,
		.param1 = param1,
		.param2 = param2,
	};
	memset(&trc_fmt, 0x0, sizeof(trc_fmt));
	memset(&trc_hdr, 0x0, sizeof(trc_hdr));
	
	/*--------------------------------------------------*/
	/* �f�[�^�쐬����lites�h���C�o��������				*/
	/*--------------------------------------------------*/

	trc_fmt.buf		=  &err;
	trc_fmt.count	=  sizeof(err);		
	trc_fmt.trc_header	= &trc_hdr;     /* �h�����R�p�w�b�_�[�i�[ 	*/
	trc_fmt.trc_header->level = 2;    	/* ���O���x�� 0���ŗD��		*/
	trc_fmt.trc_header->trace_id  = 0;	/* ���g�p 					*/
	trc_fmt.trc_header->format_id= (unsigned short)format_id;
	trc_fmt.trc_header->trace_no = LITES_KNL_LAYER+DRIVERECODER_CPUCOMDRIVER_NO;
										/* CPU�ԒʐM�h���C�o���w�� 	*/
										
	lites_trace_write(REGION_TRACE_DRIVER, &trc_fmt);
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_init										*
*																			*
*		DESCRIPTION	: �h���C�o����������									*
*																			*
*		PARAMETER	: IN  : NONE											*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
int __init	fs_ccd_sys_init(void)
{
	INT		sts;
	LONG	ret;

	/*--------------------------------------------------*/
	/*  �L�����N�^�[�f�o�C�X�o�^						*/
	/*--------------------------------------------------*/
	bf_ccm_major_num = register_chrdev(bf_ccm_major_num, M_CCD_DRV_NAME, &tb_ccd_CpuComDrvFops);
	if(bf_ccm_major_num < 0){
		LOGE("ERR0000:bf_ccm_major_num=%d", bf_ccm_major_num);
		fc_ccd_drc_error(0,-1,bf_ccm_major_num,0,-1);
		return	bf_ccm_major_num;
	}

	/*--------------------------------------------------*/
	/*  �N���X�쐬										*/
	/*--------------------------------------------------*/
	bf_ccd_dev_clas = class_create(THIS_MODULE, M_CCD_DRV_NAME);
	ret = IS_ERR(bf_ccd_dev_clas);			/* �G���[�m�F						*/
	if(ret != 0) {
		unregister_chrdev(bf_ccm_major_num, tb_ccd_CpuComDrvInf.driver.name);
		LOGE("ERR0001:ret=%d", (int)ret);
		fc_ccd_drc_error(1,-1,ret,0,-1);
		return	PTR_ERR(bf_ccd_dev_clas);
	}

	/*--------------------------------------------------*/
	/*  �쐬�N���X���r�o�h�֓o�^						*/
	/*--------------------------------------------------*/
	sts = spi_register_driver(&tb_ccd_CpuComDrvInf);
	if(sts < 0) {							/* �o�^���s�H						*/
		LOGE("ERR0002:sts=%d", bf_ccm_major_num);
		fc_ccd_drc_error(2,-1,sts,0,-1);
		class_destroy(bf_ccd_dev_clas);		/* �o�^�폜							*/
		unregister_chrdev(bf_ccm_major_num, tb_ccd_CpuComDrvInf.driver.name);
	}
	fc_ccd_drc_event(CCD_EVENT_ID_INIT,CCD_EVENT_FORMATID_IF,sts,0);
	return	sts;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_exit										*
*																			*
*		DESCRIPTION	: �h���C�o�I������										*
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
	/*  SPI�f�o�C�X�o�^����								*/
	/*--------------------------------------------------*/
	spi_unregister_driver(&tb_ccd_CpuComDrvInf);
	/*--------------------------------------------------*/
	/*	Class�o�^�폜									*/
	/*--------------------------------------------------*/
	class_destroy(bf_ccd_dev_clas);
	fc_ccd_drc_event(CCD_EVENT_ID_EXIT,CCD_EVENT_FORMATID_IF,0,0);
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
	int no;
	UNUSED(attr);
	/*--------------------------------------------------*/
	/*	�p�����[�^�`�F�b�N								*/
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
*		DESCRIPTION	: �h���C�o��������										*
*																			*
*		PARAMETER	: IN  : spi�f�[�^										*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���^�[���R�[�h									*
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
	/*	�C���X�^���X(�Ǘ��o�b�t�@)����					*/
	/*--------------------------------------------------*/
	ccd = kzalloc(sizeof(*ccd), GFP_KERNEL);
											/* �C���X�^���X�������m��			*/
	if(ccd == NULL) {						/* �������m�ێ��s					*/
		LOGE("[%s]ERR0003:ccd=NULL!!",ccd->dev.spi.id_name);
		fc_ccd_drc_error(3,-1,0,0,MODE);
		return	-ENOMEM;					/* �G���[�I��						*/
	}

	memset(ccd, 0, sizeof(*ccd));			/* �Ǘ��o�b�t�@�N���A				*/
	ccd->dev.spi.sdev = spi;				/* SPI�ݒ�							*/
	spin_lock_init(&ccd->dev.spi.lockt);

	/*--------------------------------------------------*/
	/*	���X�g������									*/
	/*--------------------------------------------------*/
	INIT_LIST_HEAD(&ccd->dev.dev_ent);
	INIT_LIST_HEAD(&ccd->comm.rx_que);

	/*--------------------------------------------------*/
	/*	�h���C�o�����ʃf�[�^�r��	�J�n				*/
	/*--------------------------------------------------*/
	mutex_lock(&bf_ccd_mutx_drv_com);

	/*--------------------------------------------------*/
	/*	�f�o�C�X�t�@�C������							*/
	/*--------------------------------------------------*/
	minor_num = find_first_zero_bit(bf_ccd_minor_num, N_SPI_MINORS);
											/* �}�C�i�[�m���D�󂫌���			*/
	if(minor_num >= N_SPI_MINORS) {			/* �󂫖����I						*/
		LOGE("[%d.%d]ERR0004:minor_num=%d",spi->master->bus_num, spi->chip_select, (int)minor_num);
		fc_ccd_drc_error(4,spi->master->bus_num,minor_num,0,MODE);
		mutex_unlock(&bf_ccd_mutx_drv_com);
		kfree(ccd);
		return -ENODEV;
	}
	ccd->dev.devt = MKDEV(bf_ccm_major_num, minor_num);
											/* �f�o�C�X�ԍ�����					*/
	dev = device_create(bf_ccd_dev_clas,
						 &spi->dev,
						 ccd->dev.devt,
						 ccd,
						 "cpucom%d.%d",
						 spi->master->bus_num,
						 spi->chip_select);
							 				/* �f�o�C�X����						*/
	memset(ccd->dev.spi.id_name, 0, sizeof(ccd->dev.spi.id_name));
	sprintf(ccd->dev.spi.id_name, "%d.%d", spi->master->bus_num, spi->chip_select);
	status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
	if(status != 0) {						/* �o�^���s�H						*/
		/* ERROR TRACE */
		LOGE("[%s]ERR0005:status=%d",ccd->dev.spi.id_name, status);
		fc_ccd_drc_error(5,spi->master->bus_num,status,0,MODE);
		device_destroy(bf_ccd_dev_clas, ccd->dev.devt);
											/*  �f�o�C�X�폜*/
		mutex_unlock(&bf_ccd_mutx_drv_com);
		kfree(ccd);
		return status;
	}
	status = device_create_file(dev, &dev_attr_name);
							 				/* �f�o�C�X�t�@�C������				*/

	if(status != 0) {						/* �f�o�C�X�t�@�C���������s			*/
		/* ERROR TRACE */
		LOGE("[%s]ERR0006:status=%d",ccd->dev.spi.id_name, status);
		fc_ccd_drc_error(6,spi->master->bus_num,status,0,MODE);
		device_remove_file(dev, &dev_attr_name);
											/*  �f�o�C�X�t�@�C���폜			*/
		device_destroy(bf_ccd_dev_clas, ccd->dev.devt);	
											/*  �f�o�C�X�폜					*/
		mutex_unlock(&bf_ccd_mutx_drv_com);
		kfree(ccd);
		return status;
	}
	set_bit(minor_num, bf_ccd_minor_num);	/* �}�C�i�[�m���D�o�^				*/

	/*--------------------------------------------------*/
	/*	�f�o�C�X�t�@�C�����X�g�o�^						*/
	/*--------------------------------------------------*/
	list_add(&ccd->dev.dev_ent, &bf_ccd_device_list);

	/*--------------------------------------------------*/
	/*	�h���C�o�����ʃf�[�^�r��	�I��				*/
	/*--------------------------------------------------*/
	mutex_unlock(&bf_ccd_mutx_drv_com);

	/*--------------------------------------------------*/
	/*  �C���X�^���X����������							*/
	/*--------------------------------------------------*/
	status = fc_ccd_init_instance(ccd);
	if(status != 0) {						/* �o�^���s							*/
		/* ERROR TRACE */
		LOGE("[%s]ERR0007:status=%d",ccd->dev.spi.id_name, status);
		fc_ccd_drc_error(7,spi->master->bus_num,status,0,MODE);
		device_remove_file(dev, &dev_attr_name);
											/*  �f�o�C�X�t�@�C���폜			*/
		device_destroy(bf_ccd_dev_clas, ccd->dev.devt);	
											/*  �f�o�C�X�폜					*/
		clear_bit(MINOR(ccd->dev.devt), bf_ccd_minor_num);
											/*  �}�C�i�[No�폜				*/
		list_del(&ccd->dev.dev_ent);		/* �f�o�C�X���X�g�폜				*/
		kfree(ccd);
		return status;
	}

	/*--------------------------------------------------*/
	/*  �����N���R�[���o�b�N�o�^						*/
	/*--------------------------------------------------*/
	ccd->ed_inf.sudden_device_poweroff = fs_ccd_fb_sdn_sdwn;
	ccd->ed_inf.dev = dev;
	status = early_device_register(&ccd->ed_inf);
											/* early �f�o�C�X�o�^				*/
	if(status != 0) {						/* �o�^���s							*/
		/* ERROR TRACE */
		LOGE("[%s]ERR0008:status=%d",ccd->dev.spi.id_name, status);
		fc_ccd_drc_error(7,spi->master->bus_num,status,0,MODE);
		device_remove_file(dev, &dev_attr_name);
											/*  �f�o�C�X�t�@�C���폜			*/
		device_destroy(bf_ccd_dev_clas, ccd->dev.devt);	
											/*  �f�o�C�X�폜					*/
		clear_bit(MINOR(ccd->dev.devt), bf_ccd_minor_num);
											/*  �}�C�i�[No�폜					*/
		list_del(&ccd->dev.dev_ent);		/* �f�o�C�X���X�g�폜				*/
		early_device_unregister(&ccd->ed_inf);
											/* early �f�o�C�X�o�^����			*/
		destroy_workqueue(ccd->mng.wkr_thrd);
											/* ���[�N�L���[�폜					*/
		kfree(ccd);
		return status;
	}

	/*--------------------------------------------------*/
	/*  SPI�o�^											*/
	/*--------------------------------------------------*/
	spi_set_drvdata(spi, ccd);

	/*--------------------------------------------------*/
	/*  �|�[�g�ݒ�										*/
	/*--------------------------------------------------*/
	gpio_request(ccd->cpu.pin.MREQ, fc_ccd_make_gpio_str(ccd->dev.spi.id_name, req_str, "MREQ"));
	gpio_request(ccd->cpu.pin.SREQ, fc_ccd_make_gpio_str(ccd->dev.spi.id_name, req_str, "SREQ"));

	tegra_gpio_enable(ccd->cpu.pin.SREQ);
	tegra_gpio_enable(ccd->cpu.pin.MREQ);

	gpio_direction_input(ccd->cpu.pin.SREQ);
	gpio_direction_output(ccd->cpu.pin.MREQ, HIGH);

	/*--------------------------------------------------*/
	/*	�p�^�p�^����̏ꍇ�͂r�o�h���f�o�h�n�ݒ肷��	*/
	/*--------------------------------------------------*/
#if __INTERFACE__ == IF_GPIO	/* GPIO�ݒ�̏ꍇ�͂��ׂĐݒ肷�� */
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
	/*  ���荞�ݐݒ�									*/
	/*--------------------------------------------------*/
	ccd->cpu.irq.sreq_num = gpio_to_irq(ccd->cpu.pin.SREQ);
	fc_ccd_make_gpio_str(ccd->dev.spi.id_name, req_str, "IRQ");
	status = request_irq(ccd->cpu.irq.sreq_num,
						  fs_ccd_intr_int_sig,
						  (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING),
						  req_str,
						  ccd);
	if(status == ESUCCESS) {
		fc_ccd_disable_irq(ccd);			/* ���荞�݋֎~						*/
	}
	else {
		/* ERROR TRACE */
		LOGE("[%s]ERR0009:status=%d",ccd->dev.spi.id_name, status);
		fc_ccd_drc_error(9,spi->master->bus_num,status,0,MODE);
		device_remove_file(dev, &dev_attr_name);
											/*  �f�o�C�X�t�@�C���폜			*/
		device_destroy(bf_ccd_dev_clas, ccd->dev.devt);	
											/*  �f�o�C�X�폜					*/
		clear_bit(MINOR(ccd->dev.devt), bf_ccd_minor_num);
											/*  �}�C�i�[No�폜					*/
		list_del(&ccd->dev.dev_ent);		/* �f�o�C�X���X�g�폜				*/
		early_device_unregister(&ccd->ed_inf);
											/* early �f�o�C�X�o�^����			*/
		destroy_workqueue(ccd->mng.wkr_thrd);
											/* ���[�N�L���[�폜					*/
		free_irq(ccd->cpu.irq.sreq_num, ccd);
											/* ���荞�ݓo�^�폜 				*/
		kfree(ccd);
	}
	fc_ccd_drc_event(CCD_EVENT_ID_PROBE,CCD_EVENT_FORMATID_IF,status,ccd->dev.spi.id_name[0]);
	return status;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_remove										*
*																			*
*		DESCRIPTION	: �h���C�o�폜����										*
*																			*
*		PARAMETER	: IN  : spi�f�[�^										*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
INT __devexit	fs_ccd_sys_remove(struct spi_device *spi)
{
	ST_CCD_MNG_INFO	*ccd;

	ccd = spi_get_drvdata(spi);

	/*--------------------------------------------------*/
	/*	�h���C�o�����ʃf�[�^�r��	�J�n				*/
	/*--------------------------------------------------*/
	mutex_lock(&bf_ccd_mutx_drv_com);

	/*--------------------------------------------------*/
	/*	�h���C�o�f�[�^���������{						*/
	/*--------------------------------------------------*/
	fc_ccd_init_drv_inf(ccd);
	/*--------------------------------------------------*/
	/*  �����E�J��										*/
	/*--------------------------------------------------*/
	spin_lock_irq(&ccd->dev.spi.lockt);		/* SPI���荞�݋֎~					*/
	ccd->dev.spi.sdev = NULL;				/* SPI�f�o�C�X���|�C���^�N���A	*/
	spi_set_drvdata(spi, NULL);				/* SPI�o�^����			 			*/
	spin_unlock_irq(&ccd->dev.spi.lockt);	/* SPI���荞�݉���					*/

	/*--------------------------------------------------*/
	/*  �f�o�h�n�J��									*/
	/*--------------------------------------------------*/
	gpio_free(ccd->cpu.pin.MREQ);
	gpio_free(ccd->cpu.pin.SREQ);
#if __INTERFACE__ == IF_GPIO
	gpio_free(ccd->cpu.pin.MCLK);
	gpio_free(ccd->cpu.pin.SDAT);
	gpio_free(ccd->cpu.pin.MDAT);
#endif

	/*--------------------------------------------------*/
	/*	�����N���o�^����								*/
	/*--------------------------------------------------*/
	early_device_unregister(&ccd->ed_inf);	/* early �f�o�C�X�o�^����			*/

	/*--------------------------------------------------*/
	/*	�f�o�C�X���X�g�폜								*/
	/*--------------------------------------------------*/
	device_remove_file(ccd->ed_inf.dev, &dev_attr_name);
											/*  �f�o�C�X�t�@�C���폜			*/
	list_del(&ccd->dev.dev_ent);			/* �f�o�C�X���X�g�폜				*/
	device_destroy(bf_ccd_dev_clas, ccd->dev.devt);
											/* �f�o�C�X�N���X�폜				*/
	clear_bit(MINOR(ccd->dev.devt), bf_ccd_minor_num);
											/* �}�C�i�[No�폜					*/

	/*--------------------------------------------------*/
	/*	����M���\�[�X�폜								*/
	/*--------------------------------------------------*/
	fc_ccd_rel_res(ccd);

	/*--------------------------------------------------*/
	/*	���[�J�[�X���b�h�폜							*/
	/*--------------------------------------------------*/
	destroy_workqueue(ccd->mng.wkr_thrd);	/* ���[�N�L���[�폜					*/

	/*--------------------------------------------------*/
	/*	���荞�ݓo�^�폜								*/
	/*--------------------------------------------------*/
	free_irq(ccd->cpu.irq.sreq_num, ccd);

	/*--------------------------------------------------*/
	/*	�C���X�^���X�J��								*/
	/*--------------------------------------------------*/
	kfree(ccd);								/* �C���X�^���X�폜					*/

	/*--------------------------------------------------*/
	/*	�h���C�o�����ʃf�[�^�r��	�I��				*/
	/*--------------------------------------------------*/
	mutex_unlock(&bf_ccd_mutx_drv_com);
	fc_ccd_drc_event(CCD_EVENT_ID_REMOVE,CCD_EVENT_FORMATID_IF,0,ccd->dev.spi.id_name[0]);
	return 0;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_open										*
*																			*
*		DESCRIPTION	: �h���C�o�I�[�v������									*
*																			*
*		PARAMETER	: IN  : *inode : �m�[�h									*
*						  : *filp  : �t�@�C��								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
INT		fs_ccd_sys_open(struct inode *inode, struct file *filp)
{
	ST_CCD_MNG_INFO		*ccd;
	INT					sts		= -EFAULT;

	/*--------------------------------------------------*/
	/*	�h���C�o�����ʃf�[�^�r��	�J�n				*/
	/*--------------------------------------------------*/
	mutex_lock(&bf_ccd_mutx_drv_com);

	/*--------------------------------------------------*/
	/* 	�f�o�C�X���X�g�������A�C���X�^���X�擾			*/
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
	/*	�C���X�^���X�r��	�J�n						*/
	/*--------------------------------------------------*/
	mutex_lock(&ccd->sys.mutex.instnc);

	if(ccd->dev.users > 0){
		LOGD("[%s]RE-OPEN",ccd->dev.spi.id_name);
		fc_ccd_drc_event(CCD_EVENT_ID_REOPEN,CCD_EVENT_FORMATID_IF,ccd->dev.users,ccd->dev.spi.id_name[0]);
	}
	/*--------------------------------------------------*/
	/*	�h���C�o�f�[�^���������{						*/
	/*--------------------------------------------------*/
	fc_ccd_init_drv_inf(ccd);

	/*--------------------------------------------------*/
	/*	���[�U�����Z									*/
	/*--------------------------------------------------*/
	ccd->dev.users++;

	/*--------------------------------------------------*/
	/*	��t����										*/
	/*--------------------------------------------------*/
	ccd->bf_ccd_acpt_flg = TRUE;

	/*--------------------------------------------------*/
	/*	�V���b�g�_�E���t���O�n�e�e						*/
	/*--------------------------------------------------*/
	bf_ccd_shut_down_flg = OFF;

	/*--------------------------------------------------*/
	/*	��ԑJ�ڏ��N���A�iIDLE�X�e�[�g�֑J�ځj		*/
	/*--------------------------------------------------*/
	fc_ccd_fsm_clr(ccd);

	
	/*--------------------------------------------------*/
	/*	�h���C�o���荞�݋���							*/
	/*--------------------------------------------------*/
	fc_ccd_enable_irq(ccd);

	/*--------------------------------------------------*/
	/*	�C���X�^���X�r��	�I��						*/
	/*--------------------------------------------------*/
	mutex_unlock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/*	�h���C�o�����ʃf�[�^�r��	�I��				*/
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
*		DESCRIPTION	: �h���C�o�N���[�Y����									*
*																			*
*		PARAMETER	: IN  : *inode : �m�[�h									*
*						  : *filp  : �t�@�C��								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
INT		fs_ccd_sys_close(struct inode *inode, struct file *filp)
{
	ST_CCD_MNG_INFO		*ccd;
	ccd = filp->private_data;
	/*--------------------------------------------------*/
	/*	�h���C�o�����ʃf�[�^�r��	�J�n				*/
	/*--------------------------------------------------*/
	mutex_lock(&bf_ccd_mutx_drv_com);
	if(ccd){
		LOGD("[%s]RELEASE",ccd->dev.spi.id_name);
		/*--------------------------------------------------*/
		/*	���[�U���`�F�b�N								*/
		/*--------------------------------------------------*/
		if(ccd->dev.users > 0) {
			ccd->dev.users --;
		}
		if(ccd->dev.users <= 0){
			ccd->dev.users= 0;
			/*--------------------------------------------------*/
			/*	�h���C�o��񏉊���								*/
			/*--------------------------------------------------*/
			fc_ccd_init_drv_inf(ccd);
		}
	}
	/*--------------------------------------------------*/
	/*	�h���C�o�����ʃf�[�^�r��	�I��				*/
	/*--------------------------------------------------*/
	mutex_unlock(&bf_ccd_mutx_drv_com);
	fc_ccd_drc_event(CCD_EVENT_ID_CLOSE,CCD_EVENT_FORMATID_IF,ccd->dev.users,ccd->dev.spi.id_name[0]);
	return 0;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_suspend									*
*																			*
*		DESCRIPTION	: �h���C�o�T�X�y���h����								*
*																			*
*		PARAMETER	: IN  : *spi : spi�f�[�^								*
*							mesg : ���b�Z�[�W								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
INT		fs_ccd_sys_suspend(struct spi_device *spi, pm_message_t mesg)
{
	ST_CCD_MNG_INFO	*ccd;

	ccd = spi_get_drvdata(spi);
	LOGD("[%s]SUSPEND",ccd->dev.spi.id_name);

	/*--------------------------------------------------*/
	/*	�h���C�o�����ʃf�[�^�r��	�J�n				*/
	/*--------------------------------------------------*/
	mutex_lock(&bf_ccd_mutx_drv_com);

	/*--------------------------------------------------*/
	/*	�C���X�^���X�r��	�J�n						*/
	/*--------------------------------------------------*/
	mutex_lock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/*	�h���C�o��񏉊���								*/
	/*--------------------------------------------------*/
	fc_ccd_init_drv_inf(ccd);

	/*--------------------------------------------------*/
	/*	�V���b�g�_�E���t���O�n�m						*/
	/*--------------------------------------------------*/
	bf_ccd_shut_down_flg = ON;

	ccd->bf_ccd_resume_not_yet = TRUE;

	/*--------------------------------------------------*/
	/*	����M�҂��̑҂�����							*/
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
	/*	�C���X�^���X�r��	�I��						*/
	/*--------------------------------------------------*/
	mutex_unlock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/*	�h���C�o�����ʃf�[�^�r��	�I��				*/
	/*--------------------------------------------------*/
	mutex_unlock(&bf_ccd_mutx_drv_com);
	fc_ccd_drc_event(CCD_EVENT_ID_SUSPEND,CCD_EVENT_FORMATID_IF,bf_ccd_shut_down_flg,ccd->dev.spi.id_name[0]);
	return	0;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_resume										*
*																			*
*		DESCRIPTION	: �h���C�o���W���[������								*
*																			*
*		PARAMETER	: IN  : *spi : spi�f�[�^								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
INT		fs_ccd_sys_resume(struct spi_device *spi)
{
	ST_CCD_MNG_INFO	*ccd;

	ccd = spi_get_drvdata(spi);
	LOGD("[%s]RESUME",ccd->dev.spi.id_name);

	/*--------------------------------------------------*/
	/*	�h���C�o�����ʃf�[�^�r��	�J�n				*/
	/*--------------------------------------------------*/
	mutex_lock(&bf_ccd_mutx_drv_com);

	/*--------------------------------------------------*/
	/*	�C���X�^���X�r��	�J�n						*/
	/*--------------------------------------------------*/
	mutex_lock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/*	�h���C�o��񏉊���								*/
	/*--------------------------------------------------*/
	fc_ccd_init_drv_inf(ccd);


	ccd->bf_ccd_resume_not_yet = FALSE;
	/*--------------------------------------------------*/
	/*	����M�҂��̑҂�����							*/
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
	/*	�C���X�^���X�r��	�I��						*/
	/*--------------------------------------------------*/
	mutex_unlock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/*	�h���C�o�����ʃf�[�^�r��	�I��				*/
	/*--------------------------------------------------*/
	mutex_unlock(&bf_ccd_mutx_drv_com);
	fc_ccd_drc_event(CCD_EVENT_ID_RESUME,CCD_EVENT_FORMATID_IF,bf_ccd_shut_down_flg,ccd->dev.spi.id_name[0]);
	return	0;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_sys_read										*
*																			*
*		DESCRIPTION	: �h���C�o���[�h����									*
*																			*
*		PARAMETER	: IN  : *filp  : �t�@�C��								*
*						  : *buf   : �o�b�t�@								*
*						  : count  : �J�E���g								*
*						  : *f_pos : �|�W�V����								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���^�[���R�[�h									*
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
	/*	�R���e�L�X�g�`�F�b�N							*/
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
	/*	��M�h�e�r������	�J�n						*/
	/*--------------------------------------------------*/
	mutex_lock(&ccd->sys.mutex.rd_if);

	/*--------------------------------------------------*/
	/*	�C���X�^���X�r��	�J�n						*/
	/*--------------------------------------------------*/
	mutex_lock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/* 	�h���C�o���荞�݋֎~							*/
	/*--------------------------------------------------*/
	fc_ccd_disable_irq(ccd);

	/*--------------------------------------------------*/
	/*	��ԃ`�F�b�N									*/
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
		/*	��M�f�[�^�������ꍇ�͎�M��҂�				*/
		/*--------------------------------------------------*/
		lst_sts = list_empty(&ccd->comm.rx_que);
		if(lst_sts != 0) {					/* ��M���X�g����H					*/
			/*--------------------------------------------------*/
			/*	��M�o�b�t�@�Ɂh�҂��h��ݒ�					*/
			/*--------------------------------------------------*/
			ccd->comm.rx_buf.sts = M_CCD_ISTS_READY;

			/*--------------------------------------------------*/
			/* 	�h���C�o���荞�݋���							*/
			/*--------------------------------------------------*/
			fc_ccd_enable_irq(ccd);

			/*--------------------------------------------------*/
			/*	�҂����̓C���X�^���X�r����������				*/
			/*--------------------------------------------------*/
			mutex_unlock(&ccd->sys.mutex.instnc);

			/*--------------------------------------------------*/
			/*	��M�҂�										*/
			/*--------------------------------------------------*/
			ccd->sys.event.rx_flg = 0;
			wai_sts = wait_event_interruptible(ccd->sys.event.rx_evq, ccd->sys.event.rx_flg);
											/* ��M�҂�							*/

			/*--------------------------------------------------*/
			/*	�҂����̃C���X�^���X�r�����ēx�J�n				*/
			/*--------------------------------------------------*/
			mutex_lock(&ccd->sys.mutex.instnc);

			/*--------------------------------------------------*/
			/* 	�h���C�o���荞�݋֎~							*/
			/*--------------------------------------------------*/
			fc_ccd_disable_irq(ccd);
		}

		/*--------------------------------------------------*/
		/*	�T�X�y���h���f�ő҂������̎��̓T�X�y���h�I��	*/
		/*--------------------------------------------------*/
		if(bf_ccd_shut_down_flg == ON) {
			sts = M_CCD_CSTS_SUS;
		}
		else {
			/*--------------------------------------------------*/
			/*	��M�f�[�^���擾								*/
			/*--------------------------------------------------*/
			rcv = NULL;
			ent = list_empty(&ccd->comm.rx_que);
			if(ent == 1) {					/* ������ۂ��I						*/
				/* ERROR TRACE */
				LOGE("[%s]ERR0012:ent=1",ccd->dev.spi.id_name);
				fc_ccd_drc_error(12,CHANNEL,0,0,MODE);
			}
			else {
				rcv = list_first_entry(&ccd->comm.rx_que, ST_CCD_TXRX_BUF, list);
				list_del(&rcv->list);		/* ���X�g����폜					*/
			}

			/*--------------------------------------------------*/
			/*	���[�U�����h�փR�s�[							*/
			/*--------------------------------------------------*/
			if(rcv != NULL) {
				/*--------------------------------------------------*/
				/*	��M�I���X�e�[�^�X�m�F							*/
				/*--------------------------------------------------*/
				if(rcv->sts == M_CCD_ISTS_CMPLT){
					cpy_sts = copy_to_user(buf, rcv->dat, rcv->len);
					if(cpy_sts != 0) {			/* �R�s�[���s�H						*/
						/* ERROR TRACE */
						LOGE("[%s]ERR0013:cpy_sts=%d rcv_len=%d",ccd->dev.spi.id_name, (int)cpy_sts, (int)rcv->len);
						fc_ccd_drc_error(13,CHANNEL,cpy_sts,rcv->len,MODE);

						sts = M_CCD_CSTS_NG;	/* �G���[�l�Z�b�g					*/
					}else{						/* �R�s�[����						*/
						/*--------------------------------------------------*/
						/*	��M�����O�X�m��								*/
						/*--------------------------------------------------*/
						read_length=rcv->len;
					}
				}else
				if(rcv->sts == M_CCD_ISTS_SUSPEND){
					sts = M_CCD_CSTS_SUS;
				}else{
					sts = M_CCD_CSTS_NG;	/* �G���[�l�Z�b�g					*/
					LOGW("[%s]WAR0000:sts=%d",ccd->dev.spi.id_name, ccd->comm.rx_buf.sts);
				}
				/*--------------------------------------------------*/
				/*	��M�o�b�t�@�̃������J��						*/
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
	/* 	�h���C�o���荞�݋���							*/
	/*--------------------------------------------------*/
	fc_ccd_enable_irq(ccd);

	/*--------------------------------------------------*/
	/*	�C���X�^���X�r��	�I��						*/
	/*--------------------------------------------------*/
	mutex_unlock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/*	��M�h�e�r������	�I��						*/
	/*--------------------------------------------------*/
	mutex_unlock(&ccd->sys.mutex.rd_if);

	/*--------------------------------------------------*/
	/*	�߂�l�ݒ�										*/
	/*--------------------------------------------------*/
	if(bf_ccd_shut_down_flg == ON)
		sts = M_CCD_CSTS_SUS;
	if(sts == M_CCD_CSTS_OK) {
		LOGD("[%s]READ <--(%d) ",ccd->dev.spi.id_name,read_length);
		return	read_length;					/* ����I��							*/
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
*		DESCRIPTION	: �h���C�o���C�g����									*
*																			*
*		PARAMETER	: IN  : *filp  : �t�@�C��								*
*						  : *buf   : �o�b�t�@								*
*						  : *f_pos : �|�W�V����								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���^�[���R�[�h									*
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
	/*	�R���e�L�X�g�`�F�b�N							*/
	/*--------------------------------------------------*/
	if(ccd == NULL) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0015:ccd=NULL",ccd->dev.spi.id_name);
		fc_ccd_drc_error(15,CHANNEL,0,0,MODE);
		return -EFAULT;
	}
	LOGD("[%s]WRITE -->",ccd->dev.spi.id_name);
	/*--------------------------------------------------*/
	/*	���M�h�e�r������	�J�n						*/
	/*--------------------------------------------------*/
	mutex_lock(&ccd->sys.mutex.wt_if);

	/*--------------------------------------------------*/
	/*	�C���X�^���X�r��	�J�n						*/
	/*--------------------------------------------------*/
	mutex_lock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/* 	�h���C�o���荞�݋֎~							*/
	/*--------------------------------------------------*/
	fc_ccd_disable_irq(ccd);

	snd = &ccd->comm.tx_buf;
	sts = M_CCD_CSTS_OK;

	/*--------------------------------------------------*/
	/*	�p�����[�^�`�F�b�N								*/
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
	/*	��ԃ`�F�b�N									*/
	/*--------------------------------------------------*/
	if(bf_ccd_shut_down_flg == ON) {
		sts = M_CCD_CSTS_SUS;
	}
	if((ccd->mng.mode == M_CCD_MODE_C0_INIT)
	 || (ccd->bf_ccd_acpt_flg == FALSE)){
		sts = M_CCD_CSTS_NG;
	}

	/*--------------------------------------------------*/
	/*	�p�����[�^�f�[�^�R�s�[							*/
	/*	��0�o�C�g�ڂ�LEN����Ŋi�[����					*/
	/*--------------------------------------------------*/
	if(sts == M_CCD_CSTS_OK) {
		cpy_sts	 = copy_from_user(&ccd->comm.tx_buf.dat[1], buf, count);
		if(cpy_sts != 0) {					/* �R�s�[���s�H						*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0018:cpy_sts=%d, count=%d",ccd->dev.spi.id_name, (int)cpy_sts, (int)count);
			fc_ccd_drc_error(18,CHANNEL,cpy_sts,count,MODE);
			sts = M_CCD_CSTS_NG;
		}
	}

	/*--------------------------------------------------*/
	/*	���M�p�k�d�m�f�[�^�v�Z�i4�̔{���j				*/
	/*--------------------------------------------------*/
	if((count % 4) != 0) {
		/* WARNING TRACE */
		LOGW("[%s]WAR0001:count=%d",ccd->dev.spi.id_name, count);

		count = (count + 3) & 0xfffffffc;	/* �S�̔{���ɕϊ�					*/
	}
	ccd->comm.tx_buf.dat[0]	= count / 4;
	ccd->comm.tx_buf.len	= count;

	/*--------------------------------------------------*/
	/*	��ԑJ�ڏ����i���M�v���j						*/
	/*--------------------------------------------------*/
	if(sts == M_CCD_CSTS_OK) {
		sts = fc_ccd_fsm_snd_req(ccd);
	}

	/*--------------------------------------------------*/
	/*	���M�N�����Ă���Α��M������҂�				*/
	/*--------------------------------------------------*/
	if(sts == M_CCD_CSTS_OK) {
		/*--------------------------------------------------*/
		/* 	�h���C�o���荞�݋���							*/
		/*--------------------------------------------------*/
		fc_ccd_enable_irq(ccd);

		/*--------------------------------------------------*/
		/*	�҂����̓C���X�^���X�r����������				*/
		/*--------------------------------------------------*/
		mutex_unlock(&ccd->sys.mutex.instnc);

		/*--------------------------------------------------*/
		/*	���M�����҂�									*/
		/*--------------------------------------------------*/
		ccd->sys.event.tx_flg = 0;
		wai_sts = wait_event_interruptible(ccd->sys.event.tx_evq, ccd->sys.event.tx_flg);
											/* ���M�����҂�						*/

		/*--------------------------------------------------*/
		/*	�҂����̃C���X�^���X�r�����ēx�J�n				*/
		/*--------------------------------------------------*/
		mutex_lock(&ccd->sys.mutex.instnc);

		/*--------------------------------------------------*/
		/* 	�h���C�o���荞�݋֎~							*/
		/*--------------------------------------------------*/
		fc_ccd_disable_irq(ccd);
	}

	/*--------------------------------------------------*/
	/*	�߂�l�ݒ�										*/
	/*--------------------------------------------------*/
	if(sts == M_CCD_CSTS_OK) {
		if(snd->sts == M_CCD_ISTS_CMPLT) {	/* ����I�����͑��M�o�C�g��			*/
			ret = count;
		}
		else
		if((snd->sts == M_CCD_ISTS_SUSPEND)	/* �T�X�y���h�ɂ��I��				*/
		 || (bf_ccd_shut_down_flg == ON)) {	/* �T�X�y���h���					*/
			ret = PRE_SUSPEND_RETVAL;
		}
		else {								/* ���̑� �� �G���[�I��				*/
			ret = -EFAULT;
		}
	}
	else
	if(sts == M_CCD_CSTS_SUS) {
		ret = PRE_SUSPEND_RETVAL;
	}
	else {
		if(count == 0) {					/* ������O�o�C�g�w��̏ꍇ�͐���	*/
			ret = 0;
		}
		else {
			ret = -EFAULT;
		}
	}
	snd->sts = M_CCD_ISTS_NONE;				/* ���M�f�[�^�����ݒ�				*/

	/*--------------------------------------------------*/
	/* 	�h���C�o���荞�݋���							*/
	/*--------------------------------------------------*/
	fc_ccd_enable_irq(ccd);

	/*--------------------------------------------------*/
	/*	�C���X�^���X�r��	�I��						*/
	/*--------------------------------------------------*/
	mutex_unlock(&ccd->sys.mutex.instnc);

	/*--------------------------------------------------*/
	/*	���M�h�e�r������	�J��						*/
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
*		DESCRIPTION	: �����N���E�I���ʒm����								*
*																			*
*		PARAMETER	: IN  : *filp  : �t�@�C��								*
*					: IN  : cmd    : �R�}���h								*
*					: IN  : arg	   : ����									*
*					  RET : ���^�[���R�[�h									*
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
	/*	�}�W�b�N�R�[�h�m�F								*/
	/*--------------------------------------------------*/
	if (_IOC_TYPE(cmd) != CPU_IOC_MAGIC){
		LOGE("MAGIC CODE NOT MUTCH");
		return -ENOTTY;
	}
	/*--------------------------------------------------*/
	/*	�A�N�Z�X�p�[�~�b�V�����m�F						*/
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
	/*	���M�h�e�r������	�J�n						*/
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
		/*	����M�҂��̑҂�����							*/
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
		/*	�h���C�o��񏉊���								*/
		/*--------------------------------------------------*/
		fc_ccd_init_drv_inf(ccd);
		
		fc_ccd_drc_event(CCD_EVENT_ID_FORCEEXIT,CCD_EVENT_FORMATID_IF,ccd->bf_ccd_acpt_flg,ccd->dev.spi.id_name[0]);
		break;
	default:
		LOGW("[%s]WAR0001:cmd=0x%x",ccd->dev.spi.id_name, cmd);
		break;
	}
	/*--------------------------------------------------*/
	/*	���M�h�e�r������	�J��						*/
	/*--------------------------------------------------*/
	fc_ccd_drc_event(CCD_EVENT_ID_IOCTL,CCD_EVENT_FORMATID_IF,ret,ccd->dev.spi.id_name[0]);
	return ret;
}
/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_fb_sdn_sdwn									*
*																			*
*		DESCRIPTION	: �����N���E�I���ʒm����								*
*																			*
*		PARAMETER	: IN  : dev�f�[�^										*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
*		CAUTION		: �����N���̃R�[���o�b�N�͎��Ԃ̂����鏈���͋֎~�I�I	*
*																			*
****************************************************************************/
VOID	fs_ccd_fb_sdn_sdwn(struct device *dev)
{
	/*--------------------------------------------------*/
	/*	�������ׁ̈A�r������͂��Ȃ�					*/
	/*--------------------------------------------------*/
	LOGD("sudden_device_poweroff");
	/*--------------------------------------------------*/
	/*	�V���b�g�_�E���t���O�n�m						*/
	/*--------------------------------------------------*/
	bf_ccd_shut_down_flg = ON;

	fc_ccd_drc_event(CCD_EVENT_ID_SHUTDOWN,CCD_EVENT_FORMATID_IF,0,0);
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fb_chk_sus										*
*																			*
*		DESCRIPTION	: �����N���E�T�X�y���h�m�F����							*
*																			*
*		PARAMETER	: IN  : dev�f�[�^										*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���^�[���R�[�h									*
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
*		DESCRIPTION	: ���荞�݃n���h��										*
*																			*
*		PARAMETER	: IN  : irq  : ���荞��									*
*																			*
*					  IN  : *context_data  : �R���e�L�X�g					*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
inline	irqreturn_t	fs_ccd_intr_int_sig(int irq, void *context_data)
{
	ST_CCD_MNG_INFO		*ccd;
	BYTE				sig;

	/*--------------------------------------------------*/
	/*	�C���X�^���X�擾								*/
	/*--------------------------------------------------*/

	ccd = (ST_CCD_MNG_INFO *)context_data;
	if(ccd == NULL) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0019:ccd=NULL",ccd->dev.spi.id_name);
		fc_ccd_drc_error(19,CHANNEL,0,0,MODE);
		return IRQ_HANDLED;
	}

	/*--------------------------------------------------*/
	/*	�h�m�s�M����Ԏ擾	���ł��邾�������擾�I		*/
	/*--------------------------------------------------*/
	sig = fc_ccd_get_sig_int(ccd);

	/*--------------------------------------------------*/
	/*	�ėp�^�C�}�J�E���^�N���A						*/
	/*--------------------------------------------------*/
	ccd->tim.tcnt = 0;

	/*--------------------------------------------------*/
	/*	���Ȑݒ莞���荞�ݖ�������						*/
	/*--------------------------------------------------*/
	if(ccd->cpu.irq.sreq_selfsts != 0xFF){
		if((BYTE)ccd->cpu.irq.sreq_selfsts == sig ){
			LOGD("[%s]ERR0045:interrupt ignore selfsetting sig[%d]",ccd->dev.spi.id_name,sig);
			return IRQ_HANDLED;
		}else{
			/* �Ⴄ���荞�݂�������N���A */
			ccd->cpu.irq.sreq_selfsts = 0xFF;
		}
	}
	/*--------------------------------------------------*/
	/*	�ω����ɏ�ԑJ�ڏ����Ăяo��					*/
	/*--------------------------------------------------*/
	if(ccd->cpu.irq.sreq_sts != sig) {
		/*--------------------------------------------------*/
		/*	������INT��M������							*/
		/*	INT FALSE TIMER ��~	�t���O������			*/
		/*--------------------------------------------------*/
		fc_ccd_tim_stop(ccd, M_CCD_TIM_INTFAIL);
		ccd->cpu.irq.tim_res &= ~M_CCD_TFG_INTFAIL;
		
		if(sig == ACTIVE) {
			fc_ccd_fsm_int_l(ccd);			/* INT L ������						*/
		}
		else {
			fc_ccd_fsm_int_h(ccd);			/* INT H ������						*/
		}
	}
	/*--------------------------------------------------*/
	/*	�F����ԂƓ���̂Ƃ��͒Z���Ԃłg�k�̉\���L��	*/
	/*--------------------------------------------------*/
	else {
		/* WARNING TRACE */
		//COLD BOOT �ESus-Res���ɏo�Ղ�
		//Resume�N�����ԂɊւ��̂�LOG�\�����Ȃ�
		//LOGW("[%s]WAR0002:sig=%d mode=%d",ccd->dev.spi.id_name, sig, ccd->mng.mode);

		fc_ccd_fsm_int_pulse(ccd, sig);
	}

	/*--------------------------------------------------*/
	/*	�h�m�s�M����Ԏ擾								*/
	/*--------------------------------------------------*/
	ccd->cpu.irq.sreq_sts = sig;

	return IRQ_HANDLED;
}

/****************************************************************************
*																			*
*		SYMBOL		: fs_ccd_timeout_cb										*
*																			*
*		DESCRIPTION	: �^�C���A�E�g�R�[���o�b�N								*
*																			*
*		PARAMETER	: IN  : *timer : �^�C�}�f�[�^							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
enum hrtimer_restart	fs_ccd_timeout_cb(struct hrtimer *timer)
{
	enum hrtimer_restart	ret = HRTIMER_NORESTART;
	ST_CCD_TIMER_INFO		*ts;
	ST_CCD_MNG_INFO			*ccd;

	/*--------------------------------------------------*/
	/*	�^�C�}���C���X�^���X�擾						*/
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
	/*	�h���C�o�����֎~��Ԃ̏ꍇ�͗v�������c���A		*/
	/*	�^�C���A�E�g�����͊��荞�݉��������̒��Ŏ��s����*/
	/*--------------------------------------------------*/


	if(ccd->cpu.irq.ena_sts == M_CCD_DISABLE) {
LOGT("[%s]Timer Pending(%s)",ccd->dev.spi.id_name,stateTBL[ts->tim_num].str);
		switch(ts->tim_num) {
			case	M_CCD_TIM_TCLK1:	/*---* ���������P*----------------------*/
				ccd->cpu.irq.tim_res |= M_CCD_TFG_TCLK1;
				break;

			case	M_CCD_TIM_TCSH:		/*---* ��������*------------------------*/
				ccd->cpu.irq.tim_res |= M_CCD_TFG_TCSH;
				break;

			case	M_CCD_TIM_TIDLE:	/*---* ����������*----------------------*/
				ccd->cpu.irq.tim_res |= M_CCD_TFG_TIDLE;
				break;

			case	M_CCD_TIM_TFAIL1:	/*---* �����������P*--------------------*/
				ccd->cpu.irq.tim_res |= M_CCD_TFG_TFAIL1;
				break;

			case	M_CCD_TIM_COMON:	/*---* �ėp�^�C�} *---------------------*/
				ccd->cpu.irq.tim_res |= M_CCD_TFG_COMON;
				break;

			case	M_CCD_TIM_INTFAIL:	/*---* INT FALESAFE�^�C�} *-------------*/
				ccd->cpu.irq.tim_res |= M_CCD_TFG_INTFAIL;
				break;

			default:					/*---* ���̑��i�G���[�j	*---------------*/
				/* ERROR TRACE */
				LOGE("[%s]ERR0021:tim_num=%d",ccd->dev.spi.id_name, ts->tim_num);
				fc_ccd_drc_error(21,CHANNEL,ts->tim_num,0,MODE);
				break;
		}
	}
	/*--------------------------------------------------*/
	/*	�h���C�o��������Ԃ̏ꍇ�̓^�C���A�E�g�������s*/
	/*--------------------------------------------------*/
	else{
		/*--------------------------------------------------*/
		/* 	�h���C�o���荞�݋֎~							*/
		/*--------------------------------------------------*/
		fc_ccd_disable_irq(ccd);

		/*--------------------------------------------------*/
		/*	�^�C���A�E�g��ʖ��ɏ�ԑJ�ڏ������{			*/
		/*--------------------------------------------------*/
		fc_ccd_fsm_timout(ccd, ts->tim_num);

		/*--------------------------------------------------*/
		/* 	�h���C�o���荞�݋���							*/
		/*--------------------------------------------------*/
		fc_ccd_enable_irq(ccd);
	}

	return ret;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_wthrd_main										*
*																			*
*		DESCRIPTION	: ���[�J�[�X���b�h���C������							*
*																			*
*		PARAMETER	: IN  : *work  :���[�J�[�X���b�h�f�[�^					*
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
	/*	�C���X�^���X�擾								*/
	/*--------------------------------------------------*/
	wkr_inf	= container_of(work, ST_CCD_WORKER_INFO, workq);
	ccd		= (ST_CCD_MNG_INFO*)(wkr_inf->inst_p);
	if(ccd == NULL) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0022:ccd=NULL",ccd->dev.spi.id_name);
		fc_ccd_drc_error(22,CHANNEL,0,0,MODE);
		return;								/* ���f								*/
	}

	/*--------------------------------------------------*/
	/*	����M����										*/
	/*--------------------------------------------------*/
	switch(wkr_inf->eve_num){
		case M_CCD_WINF_SND:	/*---* ���M�C�x���g *-----------------------*/
			LOGM("[%s] M_CCD_WINF_SND",ccd->dev.spi.id_name);
			/*--------------------------------------------------*/
			/*	���荞�ݓ��ŏ�ԕύX���Ă���Ή������Ȃ�		*/
			/*--------------------------------------------------*/
			if(ccd->mng.mode != M_CCD_MODE_S1_SNDC) {
				LOGT("[%s]%s(%d) NOT M_CCD_MODE_S1_SNDC(%s)",ccd->dev.spi.id_name,__FUNCTION__,__LINE__,stateTBL[ccd->mng.mode].str);
				break;
			}

			/*--------------------------------------------------*/
			/*	�f�[�^���M�i���k�d�m�A�e�b�b�f�[�^����������j	*/
			/*--------------------------------------------------*/
			ret = fc_ccd_snd_dat(ccd,
								 ccd->comm.tx_buf.dat,
								 ccd->comm.tx_buf.len + M_CCD_LEN_SIZE + M_CCD_FCC_SIZE);
								 			/* ���M����							*/
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
			/*	�C���X�^���X�r��	�J�n						*/
			/*--------------------------------------------------*/
			mutex_lock(&ccd->sys.mutex.instnc);

			/*--------------------------------------------------*/
			/* 	�h���C�o���荞�݋֎~							*/
			/*--------------------------------------------------*/
			fc_ccd_disable_irq(ccd);

			/*--------------------------------------------------*/
			/*	��ԑJ�ځF���M����								*/
			/*--------------------------------------------------*/
			fc_ccd_fsm_snd_cmplt(ccd, ret);	/* ���M��������						*/

			/*--------------------------------------------------*/
			/* 	�h���C�o���荞�݋���							*/
			/*--------------------------------------------------*/
			fc_ccd_enable_irq(ccd);

			/*--------------------------------------------------*/
			/*	�C���X�^���X�r��	�I��						*/
			/*--------------------------------------------------*/
			mutex_unlock(&ccd->sys.mutex.instnc);
			break;

		case M_CCD_WINF_RCV:	/*---* ��M�C�x���g *-----------------------*/
			LOGM("[%s] M_CCD_WINF_RCV",ccd->dev.spi.id_name);
			/*--------------------------------------------------*/
			/*	���荞�ݓ��ŏ�ԕύX���Ă���Ή������Ȃ�		*/
			/*--------------------------------------------------*/
			if(ccd->mng.mode != M_CCD_MODE_R1_RCVC) {
				LOGT("[%s]%s(%d) NOT M_CCD_WINF_RCV(%s)",ccd->dev.spi.id_name,__FUNCTION__,__LINE__,stateTBL[ccd->mng.mode].str);
				break;
			}

			/*--------------------------------------------------*/
			/*	�f�[�^��M�i���e�b�b�f�[�^����������j			*/
			/*--------------------------------------------------*/
			ret = fc_ccd_rcv_dat(ccd,
								 ccd->comm.rx_buf.dat,
								 ccd->comm.rx_buf.len + M_CCD_FCC_SIZE);
									 		/* ��M����							*/

			/*--------------------------------------------------*/
			/*	�C���X�^���X�r��	�J�n						*/
			/*--------------------------------------------------*/
			mutex_lock(&ccd->sys.mutex.instnc);

			/*--------------------------------------------------*/
			/* 	�h���C�o���荞�݋֎~							*/
			/*--------------------------------------------------*/
			fc_ccd_disable_irq(ccd);

			/*--------------------------------------------------*/
			/*	��ԑJ�ځF��M����								*/
			/*--------------------------------------------------*/
			fc_ccd_fsm_rcv_cmplt(ccd, ret);	/* ��M��������						*/

			/*--------------------------------------------------*/
			/* 	�h���C�o���荞�݋���							*/
			/*--------------------------------------------------*/
			fc_ccd_enable_irq(ccd);

			/*--------------------------------------------------*/
			/*	�C���X�^���X�r��	�I��						*/
			/*--------------------------------------------------*/
			mutex_unlock(&ccd->sys.mutex.instnc);
			break;

		default:				/*---* ���̑��i�G���[�j*--------------------*/
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
*		DESCRIPTION	: ��ԑJ�ڏ����u���M�v���v								*
*																			*
*		PARAMETER	: IN  : ccd		: �C���X�^���X							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���M�N���L��									*
*																			*
****************************************************************************/
SHORT	fc_ccd_fsm_snd_req(ST_CCD_MNG_INFO *ccd)
{
	ST_CCD_TXRX_BUF		*snd;
	ULONG				fcc;
	SHORT				ret;

	snd = &ccd->comm.tx_buf;
	ret = M_CCD_CSTS_NG;					/* �f�t�H���g�����M�N������		*/

	/*--------------------------------------------------*/
	/*	��ԃ`�F�b�N									*/
	/*--------------------------------------------------*/
	if(bf_ccd_shut_down_flg == ON) {
		return	M_CCD_CSTS_SUS;
	}
	if((ccd->mng.mode == M_CCD_MODE_C0_INIT)
	 || (ccd->bf_ccd_acpt_flg == FALSE)){
		return	M_CCD_CSTS_NG;
	}


	/*--------------------------------------------------*/
	/*	���M�f�[�^���c���Ă���ꍇ(���肦�Ȃ�)�̓��O�o��*/
	/*	�����M���b�N�������ׂɏ㏑������				*/
	/*--------------------------------------------------*/
	if(snd->sts != M_CCD_ISTS_NONE) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0024:snd->stsm=%d",ccd->dev.spi.id_name, snd->sts);
		fc_ccd_drc_error(24,CHANNEL,snd->sts,0,MODE);
	}

	/*--------------------------------------------------*/
	/*	���M�f�[�^�쐬									*/
	/*--------------------------------------------------*/
	fcc = fc_ccd_calc_fcc(snd->dat, snd->len+1);
											/* FCC�v�Z(LEN�f�[�^���܂߂�j		*/
	snd->dat[snd->len + 1] = (BYTE)((fcc & 0xFF000000) >> 24);
	snd->dat[snd->len + 2] = (BYTE)((fcc & 0x00FF0000) >> 16);
	snd->dat[snd->len + 3] = (BYTE)((fcc & 0x0000FF00) >>  8);
	snd->dat[snd->len + 4] = (BYTE)((fcc & 0x000000FF)      );
											/* FCC�i�[(�i�[�ʒu��LEN�f�[�^����)	*/
	snd->sts = M_CCD_ISTS_READY;			/* �X�e�[�^�X���������ɐݒ�			*/

	/*--------------------------------------------------*/
	/*	��Ԗ��̑Ή����{								*/
	/*--------------------------------------------------*/
	switch(ccd->mng.mode) {
		case M_CCD_MODE_C1_IDLE:	/*---* ���ʁF�A�C�h�� *---------------------*/
			/*--------------------------------------------------*/
			/*	�b�r���k�o�́i�A�N�e�B�u�j						*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, ACTIVE);

			/*--------------------------------------------------*/
			/*	���������P�^�C�}�N��							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TCLK1);

			/*--------------------------------------------------*/
			/*	���M�҂��֑J��									*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S0_SNDW);
			ret = M_CCD_CSTS_OK;			/* ���M�N������						*/
			break;

		case M_CCD_MODE_C2_KPID:	/*---* ���ʁF�A�C�h���ێ����� *-------------*/
		case M_CCD_MODE_S4_ITHW:	/*---* ���M�F��M�ڍs���@�h�m�s�g�҂� *-----*/
		case M_CCD_MODE_S5_ITLW:	/*---* ���M�F��M�ڍs���@�h�m�s�k�҂� *-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* ��M�F�h�m�s�@�g�҂� *---------------*/
		case M_CCD_MODE_R1_RCVC:	/*---* ��M�F��M�����҂� *-----------------*/
		case M_CCD_MODE_E0_INTE:	/*---* �ُ�F�h�m�s�ُ� *-------------------*/
///�d�l�ύX
		case M_CCD_MODE_S3_CSHK:	/*---* ���M�F���M��@�b�r�g�ێ����� *-------*/
			/*--------------------------------------------------*/
			/*	�A�C�h����ɑ��M���s���̂�҂�					*/
			/*--------------------------------------------------*/
			ret = M_CCD_CSTS_OK;			/* ���M�N������						*/
			break;

		/*--------------------------------------------------*/
		/*	���M���̓C�x���g�������鎖�͂Ȃ��i�G���[�����j	*/
		/*--------------------------------------------------*/
		case M_CCD_MODE_C0_INIT:	/*---* ���ʁF�������҂��i�����l�j*----------*/
		case M_CCD_MODE_S0_SNDW:	/*---* ���M�F���M�҂� *---------------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* ���M�F���M�����҂� *-----------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* ���M�F�b�r�@�g�҂� *-----------------*/
///�d�l�ύX				case M_CCD_MODE_S3_CSHK:	/*---* ���M�F���M��@�b�r�g�ێ����� *-------*/
		default:					/*---* ���̑��i�G���[�j*--------------------*/
			/*--------------------------------------------------*/
			/*	���������P�^�C�}��~	�i�����Ă�����j		*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TCLK1].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TCLK1);
			/*--------------------------------------------------*/
			/*	���������^�C�}��~	�i�����Ă�����j		*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TCSH].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TCSH);
			/*--------------------------------------------------*/
			/*	�����������^�C�}��~	�i�����Ă�����j		*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TIDLE].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TIDLE);
				
			/* ERROR TRACE */
			LOGE("[%s]ERR0025:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(25,CHANNEL,ccd->mng.mode,0,MODE);
			fc_ccd_fsm_clr(ccd);			/* ��ԑJ�ڏ��N���A�i�t�F�[���j	*/
			break;
	}

	return	ret;
}


/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_snd_cmplt									*
*																			*
*		DESCRIPTION	: ��ԑJ�ڏ����F�u���M�����v							*
*																			*
*		PARAMETER	: IN  : ccd		: �C���X�^���X							*
*						  : sts		: ���M�X�e�[�^�X						*
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
	/*	��Ԗ��̑Ή����{								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_S1_SNDC:	/*---* ���M�F���M�����҂� *-----------------*/
			/*--------------------------------------------------*/
			/*	����I�����͑��M����p��						*/
			/*--------------------------------------------------*/
			if(sts == M_CCD_ISTS_CMPLT) {
				LOGT("[%s]SEND M_CCD_ISTS_CMPLT",ccd->dev.spi.id_name);
///�d�l�ύX
				/*--------------------------------------------------*/
				/*	�b�r���g�o�́i�C���A�N�e�B�u�j					*/
				/*--------------------------------------------------*/
				fc_ccd_set_sig_cs(ccd, NEGATIVE);
				/*--------------------------------------------------*/
				/*	���������^�C�}�N��								*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_TCSH);

				/*--------------------------------------------------*/
				/*	�b�r�g�҂��֑J��								*/
				/*--------------------------------------------------*/
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S2_CSHW);

			}
			/*--------------------------------------------------*/
			/*	�V���b�g�_�E���̏ꍇ�͑��M�҂�������������Ԃ�	*/
			/*--------------------------------------------------*/
			else
			if(sts == M_CCD_ISTS_SUSPEND) {
				LOGT("[%s]SEND M_CCD_ISTS_SUSPEND",ccd->dev.spi.id_name);
				/*--------------------------------------------------*/
				/*	���M�X�e�[�^�X���T�X�y���h�ɐݒ�				*/
				/*--------------------------------------------------*/
				snd->sts = sts;		 		/* �I���X�e�[�^�X�ݒ�				*/

				/*--------------------------------------------------*/
				/*	�V���b�g�_�E�������i������Ԃ֑J�ځj			*/
				/*--------------------------------------------------*/
				fc_ccd_rdy_shtdwn(ccd);

			}
			/*--------------------------------------------------*/
			/*	���f�iINT#L�����j�͒��f�����A���̌ナ�g���C	*/
			/*--------------------------------------------------*/
			else
			if(sts == M_CCD_ISTS_STOP) {
				LOGT("[%s]SEND M_CCD_ISTS_STOP",ccd->dev.spi.id_name);
				/*--------------------------------------------------*/
				/*	���f�X�e�[�^�X��ݒ肷��						*/
				/*--------------------------------------------------*/
				snd->sts = sts;				/* �I���X�e�[�^�X�ݒ�				*/

				/*--------------------------------------------------*/
				/*	��{�I�ɂ͈ȍ~�͉������Ȃ����h�m�s�����݂ł̏�����҂�	*/
				/*	�h�m�s FALE SAFE Timer							*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			}
			/*--------------------------------------------------*/
			/*	�G���[�I���̏ꍇ�͒��f�X�e�[�^�X��ݒ肷��		*/
			/*	��CS H�ێ�������ASTOP�ł̃��g���C�����{		*/
			/*--------------------------------------------------*/
			else {
				LOGT("[%s]SEND other",ccd->dev.spi.id_name);
				/*--------------------------------------------------*/
				/*	���f�X�e�[�^�X��ݒ肷��						*/
				/*--------------------------------------------------*/
				snd->sts = M_CCD_ISTS_STOP;	/* �I���X�e�[�^�X=���f�ݒ�			*/

				/*--------------------------------------------------*/
				/*	�b�r�M�����g�ɂ���								*/
				/*--------------------------------------------------*/
				fc_ccd_set_sig_cs(ccd, NEGATIVE);

				/*--------------------------------------------------*/
				/*	�����������^�C�}�N��							*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_TIDLE);

				/*--------------------------------------------------*/
				/*	���M��b�r�g�ێ����Ԃֈڍs						*/
				/*--------------------------------------------------*/
///�d�l�ύX				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S3_CSHK);
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C2_KPID);
			}
			break;

		case M_CCD_MODE_C0_INIT:	/*---* ���ʁF�������҂��i�����l�j*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* ���ʁF�A�C�h�� *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* ���ʁF�A�C�h���ێ����� *-------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* ���M�F���M�҂� *---------------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* ���M�F�b�r�@�g�҂� *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* ���M�F���M��@�b�r�g�ێ����� *-------*/
		case M_CCD_MODE_S4_ITHW:	/*---* ���M�F��M�ڍs���@�h�m�s�g�҂� *-----*/
		case M_CCD_MODE_S5_ITLW:	/*---* ���M�F��M�ڍs���@�h�m�s�k�҂� *-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* ��M�F�h�m�s�@�g�҂� *---------------*/
		case M_CCD_MODE_R1_RCVC:	/*---* ��M�F��M�����҂� *-----------------*/
		case M_CCD_MODE_E0_INTE:	/*---* �ُ�F�h�m�s�ُ� *-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			break;

		default:					/*---* ���̑��i�G���[�j*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0026:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(26,CHANNEL,ccd->mng.mode,0,MODE);
			fc_ccd_fsm_clr(ccd);			/* ��ԑJ�ڏ��N���A�i�t�F�[���j	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_rcv_cmplt									*
*																			*
*		DESCRIPTION	: ��ԑJ�ڏ����F�u��M�����v							*
*																			*
*		PARAMETER	: IN  : ccd		: �C���X�^���X							*
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

	chk = STS_OK;							/* �f�t�H���g�n�j					*/

	/*--------------------------------------------------*/
	/*	��Ԗ��̑Ή����{								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_R1_RCVC:	/*---* ��M�F��M�����҂� *-----------------*/
			/*--------------------------------------------------*/
			/*	�V���b�g�_�E���̏ꍇ�͑��M�҂�������������Ԃ�	*/
			/*--------------------------------------------------*/
			if(sts == M_CCD_ISTS_SUSPEND) {
				LOGT("[%s]%s(%d) M_CCD_ISTS_SUSPEND",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);

				/*--------------------------------------------------*/
				/*	���M�X�e�[�^�X���T�X�y���h�ɐݒ�				*/
				/*--------------------------------------------------*/
				rcv->sts = sts;		 		/* �I���X�e�[�^�X�ݒ�				*/

				/*--------------------------------------------------*/
				/*	�V���b�g�_�E�������i������Ԃ֑J�ځj			*/
				/*--------------------------------------------------*/
				fc_ccd_rdy_shtdwn(ccd);

				break;						/* �����I��							*/
			}
			/*--------------------------------------------------*/
			/*	�����M�������̓f�[�^�m�F���{					*/
			/*--------------------------------------------------*/
			else
			if(sts == M_CCD_ISTS_CMPLT) {
				LOGT("[%s]%s(%d) M_CCD_ISTS_CMPLT",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
				/*--------------------------------------------------*/
				/*	�f�[�^�m�F										*/
				/*--------------------------------------------------*/
				chk = fc_ccd_chk_rcv_dat(rcv);

				/*--------------------------------------------------*/
				/*	��M�f�[�^�n�j�Ȃ�ReadIF�����֒ʒm				*/
				/*--------------------------------------------------*/
				if(chk == M_CCD_CSTS_OK) {
					LOGT("[%s]%s(%d) M_CCD_CSTS_OK",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
					/*--------------------------------------------------*/
					/*	�����X�e�[�^�X�ݒ�								*/
					/*--------------------------------------------------*/
					rcv->sts = M_CCD_ISTS_CMPLT;

					/*--------------------------------------------------*/
					/*	��M�f�[�^�����X�g�Ɋi�[						*/
					/*--------------------------------------------------*/
					fc_ccd_set_rcv_lst(ccd, rcv);
					rcv->sts = M_CCD_ISTS_NONE;

					/*--------------------------------------------------*/
					/*	��M�҂��̑҂�����								*/
					/*--------------------------------------------------*/
					if(ccd->sys.event.rx_flg == 0) {
						ccd->sys.event.rx_flg = 1;
						LOGT("[%s]%s(%d) Wake Up Rx",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
						wake_up_interruptible(&ccd->sys.event.rx_evq);
					}
				}
				/*--------------------------------------------------*/
				/*	��M�f�[�^�m�f�Ȃ牽�����Ȃ��i�ēx��M��҂j	*/
				/*--------------------------------------------------*/
				else {
					LOGT("[%s]%s(%d) M_CCD_CSTS_NG",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
					/* WARNING TRACE */
					LOGW("[%s]WAR0003:chk=%d",ccd->dev.spi.id_name, chk);
				}
			}

			/*--------------------------------------------------*/
			/*	�b�r�M�����g�ɂ���								*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);

			/*--------------------------------------------------*/
			/*	�����������^�C�}�N��							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TIDLE);

			/*--------------------------------------------------*/
			/*	�A�C�h���ێ����Ԃֈڍs							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C2_KPID);
			break;

		case M_CCD_MODE_C0_INIT:	/*---* ���ʁF�������҂��i�����l�j*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* ���ʁF�A�C�h�� *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* ���ʁF�A�C�h���ێ����� *-------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* ���M�F���M�҂� *---------------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* ���M�F�b�r�@�g�҂� *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* ���M�F���M��@�b�r�g�ێ����� *-------*/
		case M_CCD_MODE_S4_ITHW:	/*---* ���M�F��M�ڍs���@�h�m�s�g�҂� *-----*/
		case M_CCD_MODE_S5_ITLW:	/*---* ���M�F��M�ڍs���@�h�m�s�k�҂� *-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* ��M�F�h�m�s�@�g�҂� *---------------*/
		case M_CCD_MODE_E0_INTE:	/*---* �ُ�F�h�m�s�ُ� *-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			break;

		default:					/*---* ���̑��i�G���[�j*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0027:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(27,CHANNEL,ccd->mng.mode,0,MODE);
			fc_ccd_fsm_clr(ccd);			/* ��ԑJ�ڏ��N���A�i�t�F�[���j	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_int_l										*
*																			*
*		DESCRIPTION	: ��ԑJ�ڏ����u�h�m�s�|�k�����݁v						*
*																			*
*		PARAMETER	: IN  : ccd		: �C���X�^���X							*
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
	/*	��Ԗ��̑Ή����{								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_C1_IDLE:	/*---* ���ʁF�A�C�h�� *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* ���ʁF�A�C�h���ێ����� *-------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* ���M�F���M�҂� *---------------------*/
		case M_CCD_MODE_S5_ITLW:	/*---* ���M�F��M�ڍs���@�h�m�s�k�҂� *-----*/
			/*--------------------------------------------------*/
			/*	�����������P�^�C�}��~							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_stop(ccd, M_CCD_TIM_TFAIL1);
			/*--------------------------------------------------*/
			/*	���������P�^�C�}��~	�i�����Ă�����j		*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TCLK1].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TCLK1);
			/*--------------------------------------------------*/
			/*	�����������^�C�}��~	�i�����Ă�����j		*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TIDLE].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TIDLE);

			/*--------------------------------------------------*/
			/*	�b�r���k�o�́i�A�N�e�B�u�j						*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, ACTIVE);

			/*--------------------------------------------------*/
			/*	�k�d�m�i�P�o�C�g�j����M						*/
			/*--------------------------------------------------*/
#if	__INTERFACE__ == IF_SPI_TEGRA		/* SPI-TEGRA�ł̒ʐM */
			sts = fc_ccd_rcv_spi_res(ccd, &len, 1);
#else /* __INTERFACE__ == IF_GPIO */	/* GPIO����ł̒ʐM */
			len = fc_ccd_rcv_byte(ccd);
			/*�� �����O�X�`�F�b�N */
			if(len)	sts = M_CCD_CSTS_OK;
			else	sts = M_CCD_CSTS_NG;
#endif
			/*--------------------------------------------------*/
			/*	��M����I�������ꍇ�A��M����p��				*/
			/*--------------------------------------------------*/
			if(sts == M_CCD_CSTS_OK) {
				/*--------------------------------------------------*/
				/*	��M�f�[�^���i�[�i��M�l�̂S�{�j				*/
				/*--------------------------------------------------*/
				rcv->rcv_len = len;
				rcv->len = len * 4;

				/*--------------------------------------------------*/
				/*	�h�m�s�@�g�҂��֑J��							*/
				/*--------------------------------------------------*/
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_R0_ITHW);
				/*--------------------------------------------------*/
				/*	�h�m�s FALE SAFE Timer							*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			}
			/*--------------------------------------------------*/
			/*	��M�ُ�I�������ꍇ�A��M����L�����Z��		*/
			/*--------------------------------------------------*/
			else {
				LOGT("[%s]%s(%d) RX length error",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
				/*--------------------------------------------------*/
				/*	�b�r���g�o�́i�C���A�N�e�B�u�j					*/
				/*--------------------------------------------------*/
				fc_ccd_set_sig_cs(ccd, NEGATIVE);

				/*--------------------------------------------------*/
				/*	�����������^�C�}�N��							*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_TIDLE);

				/*--------------------------------------------------*/
				/*	�A�C�h���ێ����Ԃֈڍs							*/
				/*--------------------------------------------------*/
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C2_KPID);
			}
			break;

		case M_CCD_MODE_S1_SNDC:	/*---* ���M�F���M�����҂� *-----------------*/
			/*--------------------------------------------------*/
			/*	�b�r���g�o�́i�C���A�N�e�B�u�j					*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);

			/*--------------------------------------------------*/
			/*	�h�m�s�@�g�҂��֑J��							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S4_ITHW);
			/*--------------------------------------------------*/
			/*	�h�m�s FALE SAFE Timer							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			break;

		case M_CCD_MODE_S2_CSHW:	/*---* ���M�F�b�r�@�g�҂� *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* ���M�F���M��@�b�r�g�ێ����� *-------*/
			/*--------------------------------------------------*/
			/*	���������A�����������^�C�}��~					*/
			/*--------------------------------------------------*/
			fc_ccd_tim_stop(ccd, M_CCD_TIM_TCSH);
			fc_ccd_tim_stop(ccd, M_CCD_TIM_TIDLE);

			/*--------------------------------------------------*/
			/*	�b�r���g�o�́i�C���A�N�e�B�u�j					*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);

			/*--------------------------------------------------*/
			/*	�h�m�s�@�g�҂��֑J��							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S4_ITHW);
			/*--------------------------------------------------*/
			/*	�h�m�s FALE SAFE Timer							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			break;

		case M_CCD_MODE_R1_RCVC:	/*---* ��M�F��M�����҂� *-----------------*/
			/* WARNIG TRACE */
			LOGW("[%s]WAR0004:mode-event SH?",ccd->dev.spi.id_name);

			/*--------------------------------------------------*/
			/*	�b�r���g�o�́i�C���A�N�e�B�u�j					*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);

			/*--------------------------------------------------*/
			/*	�h�m�s�@�g�҂��֑J��							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S4_ITHW);
			/*--------------------------------------------------*/
			/*	�h�m�s FALE SAFE Timer							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			break;

		case M_CCD_MODE_C0_INIT:	/*---* ���ʁF�������҂��i�����l�j*----------*/
		case M_CCD_MODE_R0_ITHW:	/*---* ��M�F�h�m�s�@�g�҂� *---------------*/
		case M_CCD_MODE_S4_ITHW:	/*---* ���M�F��M�ڍs���@�h�m�s�g�҂� *-----*/
		case M_CCD_MODE_E0_INTE:	/*---* �ُ�F�h�m�s�ُ� *-------------------*/
			/* WARNING TRACE */
			LOGW("[%s]WAR0007:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			break;

		default:					/*---* ���̑��i�G���[�j*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0028:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(28,CHANNEL,ccd->mng.mode,0,MODE);
			fc_ccd_fsm_clr(ccd);			/* ��ԑJ�ڏ��N���A�i�t�F�[���j	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_int_h										*
*																			*
*		DESCRIPTION	: ��ԑJ�ڏ����u�h�m�s�|�g�����݁v						*
*																			*
*		PARAMETER	: IN  : ccd		: �C���X�^���X							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_int_h(ST_CCD_MNG_INFO *ccd)
{
	/*--------------------------------------------------*/
	/*	��Ԗ��̑Ή����{								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_S4_ITHW:	/*---* ���M�F��M�ڍs���@�h�m�s�g�҂� *-----*/
			/*--------------------------------------------------*/
			/*	�����������^�C�}��~	�i�����Ă�����j		*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TIDLE].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TIDLE);
			/*--------------------------------------------------*/
			/*	���������P�^�C�}��~	�i�����Ă�����j		*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TCLK1].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TCLK1);
			/*--------------------------------------------------*/
			/*	�����������P�^�C�}�N��							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TFAIL1);

			/*--------------------------------------------------*/
			/*	�h�m�s�@�k�҂��֑J��							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S5_ITLW);
			/*--------------------------------------------------*/
			/*	�h�m�s FALE SAFE Timer							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			break;

		case M_CCD_MODE_R0_ITHW:	/*---* ��M�F�h�m�s�@�g�҂� *---------------*/
			/*--------------------------------------------------*/
			/*	��M�����҂��֑J��								*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_R1_RCVC);
			/*--------------------------------------------------*/
			/*	��M�N���F���[�J�[�X���b�h�N��					*/
			/*--------------------------------------------------*/
			queue_work(ccd->mng.wkr_thrd,
						(struct work_struct*)&ccd->mng.wkr_inf[M_CCD_WINF_RCV]);
			break;

		case M_CCD_MODE_E0_INTE:	/*---* �ُ�F�h�m�s�ُ� *-------------------*/
			/*--------------------------------------------------*/
			/*	�����������^�C�}�N��							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TIDLE);

			/*--------------------------------------------------*/
			/*	�A�C�h���ێ����Ԋ��Ԃ֑J��						*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C2_KPID);
			break;

		case M_CCD_MODE_C0_INIT:	/*---* ���ʁF�������҂��i�����l�j*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* ���ʁF�A�C�h�� *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* ���ʁF�A�C�h���ێ����� *-------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* ���M�F���M�҂� *---------------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* ���M�F���M�����҂� *-----------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* ���M�F�b�r�@�g�҂� *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* ���M�F���M��@�b�r�g�ێ����� *-------*/
		case M_CCD_MODE_S5_ITLW:	/*---* ���M�F��M�ڍs���@�h�m�s�k�҂� *-----*/
		case M_CCD_MODE_R1_RCVC:	/*---* ��M�F��M�����҂� *-----------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			/* WARNING TRACE */
			LOGW("[%s]WAR0008:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			break;

		default:					/*---* ���̑��i�G���[�j*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0029:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(29,CHANNEL,ccd->mng.mode,0,MODE);
			fc_ccd_fsm_clr(ccd);			/* ��ԑJ�ڏ��N���A�i�t�F�[���j	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_int_pulse									*
*																			*
*		DESCRIPTION	: ��ԑJ�ڏ����u�h�m�s�p���X�����݁v					*
*																			*
*		PARAMETER	: IN  : ccd		: �C���X�^���X							*
*						  : sig		: INT�M�����							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_int_pulse(ST_CCD_MNG_INFO *ccd, BYTE sig)
{
	/*--------------------------------------------------*/
	/*	��Ԗ��̑Ή����{								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s <- %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str,sig == ACTIVE?"ACTIVE":"NEGATIVE");
	switch(ccd->mng.mode) {
		case M_CCD_MODE_C2_KPID:	/*---* ���ʁF�A�C�h���ێ����� H*-------------*/
			/*--------------------------------------------------*/
			/*	�����������^�C�}��~							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_stop(ccd, M_CCD_TIM_TIDLE);

			/*--------------------------------------------------*/
			/*	�����������P�^�C�}�N��							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TFAIL1);

			/*--------------------------------------------------*/
			/*	�h�m�s�@�k�҂��֑J��							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S5_ITLW);
			/*--------------------------------------------------*/
			/*	�h�m�s FALE SAFE Timer							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			break;
		case M_CCD_MODE_S0_SNDW:	/*---* ���M�F���M�҂� H*---------------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* ���M�F���M�����҂� H*-----------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* ���M�F�b�r�@�g�҂� H*-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* ���M�F���M��@�b�r�g�ێ����� H*-------*/

			/*--------------------------------------------------*/
			/*	���������P�^�C�}��~	�i�����Ă�����j	*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TCLK1].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TCLK1);
			/*--------------------------------------------------*/
			/*	�����������^�C�}��~	�i�����Ă�����j	*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TIDLE].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TIDLE);
			/*--------------------------------------------------*/
			/*	���������^�C�}��~	�i�����Ă�����j	*/
			/*--------------------------------------------------*/
			if(hrtimer_active(&ccd->tim.tmr_inf[M_CCD_TIM_TCSH].hndr))
				fc_ccd_tim_stop(ccd, M_CCD_TIM_TCSH);

			/*--------------------------------------------------*/
			/*	�b�r���g�o�́i�C���A�N�e�B�u�j					*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);
			/*--------------------------------------------------*/
			/*	�����������P�^�C�}�N��							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TFAIL1);
			/*--------------------------------------------------*/
			/*	�h�m�s�@�k�҂��֑J��							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S5_ITLW);
			/*--------------------------------------------------*/
			/*	�h�m�s FALE SAFE Timer							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			break;

		case M_CCD_MODE_S5_ITLW:	/*---* ���M�F��M�ڍs���@�h�m�s�k�҂� H*-----*/
			/*--------------------------------------------------*/
			/*	�����������P�^�C�}�ċN��						*/
			/*--------------------------------------------------*/
			fc_ccd_tim_stop(ccd, M_CCD_TIM_TFAIL1);
			fc_ccd_tim_start(ccd, M_CCD_TIM_TFAIL1);
			break;

		case M_CCD_MODE_R1_RCVC:	/*---* ��M�F��M�����҂� H*-----------------*/
			/* WARNING TRACE */
			LOGW("[%s]WAR0005:mode-event SH?",ccd->dev.spi.id_name);

			/*--------------------------------------------------*/
			/*	�b�r���g�o�́i�C���A�N�e�B�u�j					*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);

			/*--------------------------------------------------*/
			/*	�����������P�^�C�}�N��							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TFAIL1);

			/*--------------------------------------------------*/
			/*	�h�m�s�@�k�҂��֑J��							*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S5_ITLW);
			/*--------------------------------------------------*/
			/*	�h�m�s FALE SAFE Timer							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_INTFAIL);
			break;

		case M_CCD_MODE_C0_INIT:	/*---* ���ʁF�������҂��i�����l�j?*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* ���ʁF�A�C�h�� H*---------------------*/
		case M_CCD_MODE_S4_ITHW:	/*---* ���M�F��M�ڍs���@�h�m�s�g�҂� L*-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* ��M�F�h�m�s�@�g�҂� L*---------------*/
		case M_CCD_MODE_E0_INTE:	/*---* �ُ�F�h�m�s�ُ� L*-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			/* �������Ȃ� */
			break;

		default:					/*---* ���̑��i�G���[�j*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0030:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(30,CHANNEL,ccd->mng.mode,0,MODE);
			fc_ccd_fsm_clr(ccd);			/* ��ԑJ�ڏ��N���A�i�t�F�[���j	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_tout_clk1									*
*																			*
*		DESCRIPTION	: ��ԑJ�ڏ����u���������P�@�s�D�n�D�v					*
*																			*
*		PARAMETER	: IN  : ccd		: �C���X�^���X							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_tout_clk1(ST_CCD_MNG_INFO *ccd)
{
	/*--------------------------------------------------*/
	/*	��Ԗ��̑Ή����{								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_S0_SNDW:	/*---* ���M�F���M�҂� *---------------------*/
			/*--------------------------------------------------*/
			/*	���M�����҂��֑J��								*/
			/*--------------------------------------------------*/
			fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S1_SNDC);
			/*--------------------------------------------------*/
			/*	���M�N���F���[�J�[�X���b�h�N��					*/
			/*--------------------------------------------------*/
			queue_work(ccd->mng.wkr_thrd,
						(struct work_struct*)&ccd->mng.wkr_inf[M_CCD_WINF_SND]);

			break;

		case M_CCD_MODE_C0_INIT:	/*---* ���ʁF�������҂��i�����l�j*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* ���ʁF�A�C�h�� *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* ���ʁF�A�C�h���ێ����� *-------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* ���M�F���M�����҂� *-----------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* ���M�F�b�r�@�g�҂� *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* ���M�F���M��@�b�r�g�ێ����� *-------*/
		case M_CCD_MODE_S4_ITHW:	/*---* ���M�F��M�ڍs���@�h�m�s�g�҂� *-----*/
		case M_CCD_MODE_S5_ITLW:	/*---* ���M�F��M�ڍs���@�h�m�s�k�҂� *-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* ��M�F�h�m�s�@�g�҂� *---------------*/
		case M_CCD_MODE_R1_RCVC:	/*---* ��M�F��M�����҂� *-----------------*/
		case M_CCD_MODE_E0_INTE:	/*---* �ُ�F�h�m�s�ُ� *-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			/* �������Ȃ� */
			break;

		default:					/*---* ���̑��i�G���[�j*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0031:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(31,CHANNEL,ccd->mng.mode,0,MODE);

			fc_ccd_fsm_clr(ccd);			/* ��ԑJ�ڏ��N���A�i�t�F�[���j	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_tout_csh									*
*																			*
*		DESCRIPTION	: ��ԑJ�ڏ����u���������@�s�D�n�D�v					*
*																			*
*		PARAMETER	: IN  : ccd		: �C���X�^���X							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_tout_csh(ST_CCD_MNG_INFO *ccd)
{
	/*--------------------------------------------------*/
	/*	��Ԗ��̑Ή����{								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_S2_CSHW:	/*---* ���M�F�b�r�@�g�҂� *-----------------*/
///�d�l�ύX
			/*--------------------------------------------------*/
			/*	���M�����C�x���g���s�iWRITE�҂������j			*/
			/*--------------------------------------------------*/
			if(ccd->sys.event.tx_flg == 0){
				if(ccd->comm.tx_buf.sts == M_CCD_ISTS_READY) {
					ccd->comm.tx_buf.sts = M_CCD_ISTS_CMPLT;
												/* ���M�����ݒ�						*/
					ccd->sys.event.tx_flg = 1;
					LOGT("[%s]%s(%d) Wake Up Tx",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
					wake_up_interruptible(&ccd->sys.event.tx_evq);
												/* �C�x���g���s						*/
												/* �A�C�h����Ԃ֑J��				*/
				}
			}
			/*--------------------------------------------------*/
			/*	�b�r���g�o�́i�C���A�N�e�B�u�j					*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);

			/*--------------------------------------------------*/
			/*	�����������^�C�}�N��							*/
			/*--------------------------------------------------*/
			fc_ccd_tim_start(ccd, M_CCD_TIM_TIDLE);

			/*--------------------------------------------------*/
			/*	�b�r�g�ێ����Ԃ֑J��							*/
			/*--------------------------------------------------*/
///�d�l�ύX				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S3_CSHK);
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C2_KPID);
			break;

		case M_CCD_MODE_C0_INIT:	/*---* ���ʁF�������҂��i�����l�j*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* ���ʁF�A�C�h�� *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* ���ʁF�A�C�h���ێ����� *-------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* ���M�F���M�҂� *---------------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* ���M�F���M�����҂� *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* ���M�F���M��@�b�r�g�ێ����� *-------*/
		case M_CCD_MODE_S4_ITHW:	/*---* ���M�F��M�ڍs���@�h�m�s�g�҂� *-----*/
		case M_CCD_MODE_S5_ITLW:	/*---* ���M�F��M�ڍs���@�h�m�s�k�҂� *-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* ��M�F�h�m�s�@�g�҂� *---------------*/
		case M_CCD_MODE_R1_RCVC:	/*---* ��M�F��M�����҂� *-----------------*/
		case M_CCD_MODE_E0_INTE:	/*---* �ُ�F�h�m�s�ُ� *-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			/* �������Ȃ� */
			break;

		default:					/*---* ���̑��i�G���[�j*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0032:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(32,CHANNEL,ccd->mng.mode,0,MODE);

			fc_ccd_fsm_clr(ccd);			/* ��ԑJ�ڏ��N���A�i�t�F�[���j	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_tout_idle									*
*																			*
*		DESCRIPTION	: ��ԑJ�ڏ����u�����������@�s�D�n�D�v					*
*																			*
*		PARAMETER	: IN  : ccd		: �C���X�^���X							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_tout_idle(ST_CCD_MNG_INFO *ccd)
{
	/*--------------------------------------------------*/
	/*	��Ԗ��̑Ή����{								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_C2_KPID:	/*---* ���ʁF�A�C�h���ێ����� *-------------*/
			/*--------------------------------------------------*/
			/*	���M�v��������ꍇ�͑��M�N��					*/
			/*--------------------------------------------------*/
			if((ccd->comm.tx_buf.sts == M_CCD_ISTS_READY)
			 || (ccd->comm.tx_buf.sts == M_CCD_ISTS_STOP)) {
				ccd->comm.tx_buf.sts =M_CCD_ISTS_READY;			/* �X�e�[�^�X���������ɖ߂�			*/
				/*--------------------------------------------------*/
				/*	�b�r���k�o�́i�A�N�e�B�u�j						*/
				/*--------------------------------------------------*/
				fc_ccd_set_sig_cs(ccd, ACTIVE);

				/*--------------------------------------------------*/
				/*	���������P�^�C�}�N��							*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_TCLK1);

				/*--------------------------------------------------*/
				/*	���M�҂��֑J��									*/
				/*--------------------------------------------------*/
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S0_SNDW);
			}
			/*--------------------------------------------------*/
			/*	���M�v���Ȃ��ꍇ�̓A�C�h����Ԃ֑J��			*/
			/*--------------------------------------------------*/
			else {
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C1_IDLE);
			}
			break;

		case M_CCD_MODE_S3_CSHK:	/*---* ���M�F���M��@�b�r�g�ێ����� *-------*/
///�d�l�ύX
///			/*--------------------------------------------------*/
///			/*	���M�����C�x���g���s�iWRITE�҂������j			*/
///			/*--------------------------------------------------*/
///			if(ccd->comm.tx_buf.sts == M_CCD_ISTS_READY) {
///				ccd->comm.tx_buf.sts = M_CCD_ISTS_CMPLT;
///											/* ���M�����ݒ�						*/
///				ccd->sys.event.tx_flg = 1;
///				LOGT("[%s]%s(%d) Wake Up Tx",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
///				wake_up_interruptible(&ccd->sys.event.tx_evq);
///											/* �C�x���g���s						*/
///											/* �A�C�h����Ԃ֑J��				*/
///			}
			/*--------------------------------------------------*/
			/*	���M�����ł��Ă��Ȃ��ꍇ�͑��M�N���i���g���C�j	*/
			/*--------------------------------------------------*/
			if(ccd->comm.tx_buf.sts == M_CCD_ISTS_STOP) {
				ccd->comm.tx_buf.sts =M_CCD_ISTS_READY;			/* �X�e�[�^�X���������ɖ߂�				*/
				/*--------------------------------------------------*/
				/*	�b�r���k�o�́i�A�N�e�B�u�j						*/
				/*--------------------------------------------------*/
				fc_ccd_set_sig_cs(ccd, ACTIVE);

				/*--------------------------------------------------*/
				/*	���������P�^�C�}�N��							*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_TCLK1);

				/*--------------------------------------------------*/
				/*	���M�҂��֑J��									*/
				/*--------------------------------------------------*/
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S0_SNDW);
			}
			/*--------------------------------------------------*/
			/*	���M�v���Ȃ��ꍇ�̓A�C�h����Ԃ֑J��			*/
			/*--------------------------------------------------*/
			else {
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C1_IDLE);
			}
			break;

		case M_CCD_MODE_C0_INIT:	/*---* ���ʁF�������҂��i�����l�j*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* ���ʁF�A�C�h�� *---------------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* ���M�F���M�҂� *---------------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* ���M�F���M�����҂� *-----------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* ���M�F�b�r�@�g�҂� *-----------------*/
		case M_CCD_MODE_S4_ITHW:	/*---* ���M�F��M�ڍs���@�h�m�s�g�҂� *-----*/
		case M_CCD_MODE_S5_ITLW:	/*---* ���M�F��M�ڍs���@�h�m�s�k�҂� *-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* ��M�F�h�m�s�@�g�҂� *---------------*/
		case M_CCD_MODE_R1_RCVC:	/*---* ��M�F��M�����҂� *-----------------*/
		case M_CCD_MODE_E0_INTE:	/*---* �ُ�F�h�m�s�ُ� *-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			/* �������Ȃ� */
			break;

		default:					/*---* ���̑��i�G���[�j*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0033:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(33,CHANNEL,ccd->mng.mode,0,MODE);

			fc_ccd_fsm_clr(ccd);			/* ��ԑJ�ڏ��N���A�i�t�F�[���j	*/
			break;
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_tout_fail1									*
*																			*
*		DESCRIPTION	: ��ԑJ�ڏ����u����������1 �s�D�n�D�v					*
*																			*
*		PARAMETER	: IN  : ccd		: �C���X�^���X							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_tout_fail1(ST_CCD_MNG_INFO *ccd)
{
	/*--------------------------------------------------*/
	/*	��Ԗ��̑Ή����{								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_S5_ITLW:	/*---* ���M�F��M�ڍs���@�h�m�s�k�҂� *-----*/
			/*--------------------------------------------------*/
			/*	���M�v��������ꍇ�͑��M�N��					*/
			/*--------------------------------------------------*/
			if((ccd->comm.tx_buf.sts == M_CCD_ISTS_READY)
			 || (ccd->comm.tx_buf.sts == M_CCD_ISTS_STOP)) {
				ccd->comm.tx_buf.sts =M_CCD_ISTS_READY;			/* �X�e�[�^�X���������ɖ߂�			*/
				/*--------------------------------------------------*/
				/*	�b�r���k�o�́i�A�N�e�B�u�j						*/
				/*--------------------------------------------------*/
				fc_ccd_set_sig_cs(ccd, ACTIVE);

				/*--------------------------------------------------*/
				/*	���������P�^�C�}�N��							*/
				/*--------------------------------------------------*/
				fc_ccd_tim_start(ccd, M_CCD_TIM_TCLK1);

				/*--------------------------------------------------*/
				/*	���M�҂��֑J��									*/
				/*--------------------------------------------------*/
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_S0_SNDW);
			}
			/*--------------------------------------------------*/
			/*	���M�v���Ȃ��ꍇ�̓A�C�h����Ԃ֑J��			*/
			/*--------------------------------------------------*/
			else {
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C1_IDLE);
			}
			break;

		case M_CCD_MODE_C0_INIT:	/*---* ���ʁF�������҂��i�����l�j*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* ���ʁF�A�C�h�� *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* ���ʁF�A�C�h���ێ����� *-------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* ���M�F���M�҂� *---------------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* ���M�F���M�����҂� *-----------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* ���M�F�b�r�@�g�҂� *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* ���M�F���M��@�b�r�g�ێ����� *-------*/
		case M_CCD_MODE_S4_ITHW:	/*---* ���M�F��M�ڍs���@�h�m�s�g�҂� *-----*/
		case M_CCD_MODE_R0_ITHW:	/*---* ��M�F�h�m�s�@�g�҂� *---------------*/
		case M_CCD_MODE_R1_RCVC:	/*---* ��M�F��M�����҂� *-----------------*/
		case M_CCD_MODE_E0_INTE:	/*---* �ُ�F�h�m�s�ُ� *-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			break;

		default:					/*---* ���̑��i�G���[�j*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0034:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(34,CHANNEL,ccd->mng.mode,0,MODE);

			fc_ccd_fsm_clr(ccd);			/* ��ԑJ�ڏ��N���A�i�t�F�[���j	*/
			break;
	}
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_tout_intfale								*
*																			*
*		DESCRIPTION	: ��ԑJ�ڏ����uINT FALESAFE �^�C�}�s�D�n�D�v			*
*																			*
*		PARAMETER	: IN  : ccd		: �C���X�^���X							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_tout_intfale(ST_CCD_MNG_INFO *ccd)
{
	BYTE sig = fc_ccd_get_sig_int(ccd);	/* INT ��Ԏ擾 */
	/*--------------------------------------------------*/
	/*	��Ԗ��̑Ή����{								*/
	/*--------------------------------------------------*/
	LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_R0_ITHW:	/*---* ��M�F�h�m�s�@�g�҂� *---------------*/
			if(sig == NEGATIVE){
				/*--------------------------------------------------*/
				/*	�O��Ԗ��̑Ή����{								*/
				/*--------------------------------------------------*/
				switch(ccd->mng.previous_mode){
				case M_CCD_MODE_C1_IDLE:	/*---* ���ʁF�A�C�h�� *---------------------*/
				case M_CCD_MODE_C2_KPID:	/*---* ���ʁF�A�C�h���ێ����� *-------------*/
				case M_CCD_MODE_S0_SNDW:	/*---* ���M�F���M�҂� *---------------------*/
				case M_CCD_MODE_S5_ITLW:	/*---* ���M�F��M�ڍs���@�h�m�s�k�҂� *-----*/
					/*--------------------------------------------------*/
					/*	INT��ԊǗ��X�V									*/
					/*--------------------------------------------------*/
					ccd->cpu.irq.sreq_sts = sig;
					ccd->cpu.irq.sreq_selfsts= sig;
					LOGD("[%s]ERR1000:mode=%d sreq_selfsts=%d",ccd->dev.spi.id_name, ccd->mng.mode,ccd->cpu.irq.sreq_selfsts);
					fc_ccd_drc_error(1000,CHANNEL,ccd->mng.mode,0,MODE);
					/*--------------------------------------------------*/
					/*	��M�����҂��֑J��								*/
					/*--------------------------------------------------*/
					fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_R1_RCVC);
					/*--------------------------------------------------*/
					/*	��M�N���F���[�J�[�X���b�h�N��					*/
					/*--------------------------------------------------*/
					queue_work(ccd->mng.wkr_thrd,
								(struct work_struct*)&ccd->mng.wkr_inf[M_CCD_WINF_RCV]);
					break;
				case M_CCD_MODE_S1_SNDC:	/*---* ���M�F���M�����҂� *-----------------*/
				case M_CCD_MODE_S2_CSHW:	/*---* ���M�F�b�r�@�g�҂� *-----------------*/
				case M_CCD_MODE_S3_CSHK:	/*---* ���M�F���M��@�b�r�g�ێ����� *-------*/
				case M_CCD_MODE_R1_RCVC:	/*---* ��M�F��M�����҂� *-----------------*/
					/*--------------------------------------------------*/
					/*	INT��ԊǗ��X�V									*/
					/*--------------------------------------------------*/
					ccd->cpu.irq.sreq_sts = sig;
					ccd->cpu.irq.sreq_selfsts= sig;
					LOGD("[%s]ERR1001:mode=%d sreq_selfsts=%d",ccd->dev.spi.id_name, ccd->mng.mode,ccd->cpu.irq.sreq_selfsts);
					fc_ccd_drc_error(1001,CHANNEL,ccd->mng.mode,0,MODE);
					/*--------------------------------------------------*/
					/*	INT#H�������s									*/
					/*--------------------------------------------------*/
					fc_ccd_fsm_int_h(ccd);
					break;
				}
			}
			break;
		case M_CCD_MODE_S1_SNDC:	/*---* ���M�F���M�����҂� *-----------------*/
			if(sig == ACTIVE){
				/*--------------------------------------------------*/
				/*	INT��ԊǗ��X�V									*/
				/*--------------------------------------------------*/
				ccd->cpu.irq.sreq_sts = sig;
				ccd->cpu.irq.sreq_selfsts= sig;
				LOGD("[%s]ERR1002:mode=%d sreq_selfsts=%d",ccd->dev.spi.id_name, ccd->mng.mode,ccd->cpu.irq.sreq_selfsts);
				fc_ccd_drc_error(1002,CHANNEL,ccd->mng.mode,0,MODE);
				/*--------------------------------------------------*/
				/*	INT#L�������s									*/
				/*--------------------------------------------------*/
				fc_ccd_fsm_int_l(ccd);
			}
			break;
		case M_CCD_MODE_S4_ITHW:	/*---* ���M�F��M�ڍs���@�h�m�s�g�҂� *-----*/
			if(sig == NEGATIVE){
				/*--------------------------------------------------*/
				/*	INT��ԊǗ��X�V									*/
				/*--------------------------------------------------*/
				ccd->cpu.irq.sreq_sts = sig;
				ccd->cpu.irq.sreq_selfsts= sig;
				LOGD("[%s]ERR1003:mode=%d sreq_selfsts=%d",ccd->dev.spi.id_name, ccd->mng.mode,ccd->cpu.irq.sreq_selfsts);
				fc_ccd_drc_error(1003,CHANNEL,ccd->mng.mode,0,MODE);
				/*--------------------------------------------------*/
				/*	INT#L�������s									*/
				/*--------------------------------------------------*/
				fc_ccd_fsm_int_l(ccd);
			}
			break;
		case M_CCD_MODE_S5_ITLW:	/*---* ���M�F��M�ڍs���@�h�m�s�k�҂� *-----*/
			if(sig == ACTIVE){
				/*--------------------------------------------------*/
				/*	�O��Ԗ��̑Ή����{								*/
				/*--------------------------------------------------*/
				switch(ccd->mng.previous_mode){
					case M_CCD_MODE_C2_KPID:	/*---* ���ʁF�A�C�h���ێ����� H*-------------*/
					case M_CCD_MODE_S0_SNDW:	/*---* ���M�F���M�҂� H*---------------------*/
					case M_CCD_MODE_S1_SNDC:	/*---* ���M�F���M�����҂� H*-----------------*/
					case M_CCD_MODE_S2_CSHW:	/*---* ���M�F�b�r�@�g�҂� H*-----------------*/
					case M_CCD_MODE_S3_CSHK:	/*---* ���M�F���M��@�b�r�g�ێ����� H*-------*/
					case M_CCD_MODE_R1_RCVC:	/*---* ��M�F��M�����҂� H*-----------------*/
					/*--------------------------------------------------*/
					/*	INT��ԊǗ��X�V									*/
					/*--------------------------------------------------*/
					ccd->cpu.irq.sreq_sts = sig;
					ccd->cpu.irq.sreq_selfsts= sig;
					LOGD("[%s]ERR1004:mode=%d sreq_selfsts=%d",ccd->dev.spi.id_name, ccd->mng.mode,ccd->cpu.irq.sreq_selfsts);
					fc_ccd_drc_error(1004,CHANNEL,ccd->mng.mode,0,MODE);
					/*--------------------------------------------------*/
					/*	INT#L�������s									*/
					/*--------------------------------------------------*/
					fc_ccd_fsm_int_l(ccd);
					break;
				}
			}
			break;
		case M_CCD_MODE_C0_INIT:	/*---* ���ʁF�������҂��i�����l�j*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* ���ʁF�A�C�h�� *---------------------*/
		case M_CCD_MODE_C2_KPID:	/*---* ���ʁF�A�C�h���ێ����� *-------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* ���M�F�b�r�@�g�҂� *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* ���M�F���M��@�b�r�g�ێ����� *-------*/
		case M_CCD_MODE_R1_RCVC:	/*---* ��M�F��M�����҂� *-----------------*/
		case M_CCD_MODE_E0_INTE:	/*---* �ُ�F�h�m�s�ُ� *-------------------*/
			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			break;

		default:					/*---* ���̑��i�G���[�j*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0034:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(34,CHANNEL,ccd->mng.mode,0,MODE);

			fc_ccd_fsm_clr(ccd);			/* ��ԑJ�ڏ��N���A�i�t�F�[���j	*/
			break;
	}
}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_tout_comn									*
*																			*
*		DESCRIPTION	: ��ԑJ�ڏ����u�ėp�^�C�}�@�s�D�n�D�v					*
*																			*
*		PARAMETER	: IN  : ccd		: �C���X�^���X							*
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
	/*  �T�X�y���h��ԃ|�[�����O�m�F					*/
	/*--------------------------------------------------*/
	sus  = early_device_invoke_with_flag_irqsave(RNF_BUDET_ACCOFF_INTERRUPT_MASK,
													fc_ccd_fb_chk_sus, NULL);
	if(sus == TRUE){
		LOGT("[%s]%s(%d) bf_ccd_shut_down_flg ON !!!",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
		bf_ccd_shut_down_flg = ON;
	}

	/*--------------------------------------------------*/
	/*	�V���b�g�_�E�����͂q�v�҂��J��					*/
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
		/*	�������҂��֑J��								*/
		/*--------------------------------------------------*/
		fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C0_INIT);
		return;								/* �������f							*/
	}

	/*--------------------------------------------------*/
	/*	��ԌŒ��J�E���^�@�J�E���g�A�b�v				*/
	/*--------------------------------------------------*/
	if(ccd->tim.mchk < M_CCD_COMTIM_MAX) {
		ccd->tim.mchk++;
	}

	/*--------------------------------------------------*/
	/*	�h�m�s�M����Ԏ擾								*/
	/*--------------------------------------------------*/
	sig = fc_ccd_get_sig_int(ccd);

	/*--------------------------------------------------*/
	/*	��ԌŒ��m�F�i�h�m�s�l�K�e�B�u���̂݁j			*/
	/*--------------------------------------------------*/
	if(sig == NEGATIVE) {
		/*--------------------------------------------------*/
		/*	�Œ��������Ă������ԃN���A�i�A�C�h���j����	*/
		/*--------------------------------------------------*/
		if((ccd->tim.mchk > M_CCD_COMTIM_300MS)
		 && (ccd->mng.mode != M_CCD_MODE_C0_INIT)
		 && (ccd->mng.mode != M_CCD_MODE_C1_IDLE)
		 && (ccd->mng.mode != M_CCD_MODE_E0_INTE)) {
			/* ERROR TRACE */
			LOGE("[%s]ERR0035:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(35,CHANNEL,ccd->mng.mode,0,MODE);

			fc_ccd_fsm_clr(ccd);			/* ��ԑJ�ڏ��N���A				*/
			return;							/* �������f							*/
		}
	}

	/*--------------------------------------------------*/
	/*	�h�m�s���k�̂Ƃ��̂݃J�E���g�A�b�v				*/
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
	/*	��Ԗ��̑Ή����{								*/
	/*--------------------------------------------------*/
	///LOGM("[%s] mode : %s",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str);
	switch(ccd->mng.mode) {
		case M_CCD_MODE_C0_INIT:	/*---* ���ʁF�������҂��i�����l�j*----------*/
		case M_CCD_MODE_C1_IDLE:	/*---* ���ʁF�A�C�h�� *---------------------*/
			/*--------------------------------------------------*/
			/*	�b�r�M�����g�ɂ���i�t�F�[���Z�[�t�j			*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);
			break;

		case M_CCD_MODE_R0_ITHW:	/*---* ��M�F�h�m�s�@�g�҂� *---------------*/
		case M_CCD_MODE_S4_ITHW:	/*---* ���M�F��M�ڍs���@�h�m�s�g�҂� *-----*/
			/*--------------------------------------------------*/
			/*	�P�b(�b��l)�𒴂��Ă����瑗�M�͎̂Ďn�߂�		*/
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
			/*	TOUT�o�߂��Ă���ԌŒ��̏ꍇ�̓G���[��Ԃ֑J��	*/
			/*--------------------------------------------------*/
			if(ccd->tim.tcnt == M_CCD_COMTIM_5SEC) {
				LOGW("[%s]%s(%d) 5SEC Over",ccd->dev.spi.id_name,__FUNCTION__,__LINE__);
				/*--------------------------------------------------*/
				/*	�b�r�M�����g�ɂ���								*/
				/*--------------------------------------------------*/
				fc_ccd_set_sig_cs(ccd, NEGATIVE);

				/*--------------------------------------------------*/
				/*	�G���[��Ԃ֑J��								*/
				/*--------------------------------------------------*/
				fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_E0_INTE);
			}
			break;

		case M_CCD_MODE_E0_INTE:	/*---* �ُ�F�h�m�s�ُ� *-------------------*/
			/*--------------------------------------------------*/
			/*	���M�v���͎̂Ă�								*/
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
			/*	�b�r�M�����g�ɂ���i�t�F�[���Z�[�t�j			*/
			/*--------------------------------------------------*/
			fc_ccd_set_sig_cs(ccd, NEGATIVE);

			/*--------------------------------------------------*/
			/*	�P���҂��ĉ񕜂��Ȃ��ꍇ�͋L�^�i�d�l�j			*/
			/*--------------------------------------------------*/
			if(ccd->tim.tcnt == M_CCD_COMTIM_1MIN) {
				/* ERROR TRACE */
				LOGE("[%s]ERR0036:Cpu Communication Driver Error, Logged by Specification : 1min_Timeout",ccd->dev.spi.id_name);
				fc_ccd_drc_error(36,CHANNEL,0,0,MODE);
			}
			break;

		case M_CCD_MODE_C2_KPID:	/*---* ���ʁF�A�C�h���ێ����� *-------------*/
		case M_CCD_MODE_S0_SNDW:	/*---* ���M�F���M�҂� *---------------------*/
		case M_CCD_MODE_S1_SNDC:	/*---* ���M�F���M�����҂� *-----------------*/
		case M_CCD_MODE_S2_CSHW:	/*---* ���M�F�b�r�@�g�҂� *-----------------*/
		case M_CCD_MODE_S3_CSHK:	/*---* ���M�F���M��@�b�r�g�ێ����� *-------*/
		case M_CCD_MODE_S5_ITLW:	/*---* ���M�F��M�ڍs���@�h�m�s�k�҂� *-----*/
		case M_CCD_MODE_R1_RCVC:	/*---* ��M�F��M�����҂� *-----------------*/
//			LOGM("[%s] donothing",ccd->dev.spi.id_name);
			break;

		default:					/*---* ���̑��i�G���[�j*--------------------*/
			/* ERROR TRACE */
			LOGE("[%s]ERR0037:mode=%d",ccd->dev.spi.id_name, ccd->mng.mode);
			fc_ccd_drc_error(37,CHANNEL,ccd->mng.mode,0,MODE);

			fc_ccd_fsm_clr(ccd);			/* ��ԑJ�ڏ��N���A�i�t�F�[���j	*/
			break;
	}

	/*--------------------------------------------------*/
	/*	�^�C�}�ċN��									*/
	/*--------------------------------------------------*/
	fc_ccd_tim_start(ccd, M_CCD_TIM_COMON);

}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_timout										*
*																			*
*		DESCRIPTION	: ��ԑJ�ڃ^�C���A�E�g����								*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_fsm_timout(ST_CCD_MNG_INFO *ccd, INT tim_num)
{
	/*--------------------------------------------------*/
	/*	�^�C���A�E�g��ʖ��ɏ�ԑJ�ڏ������{			*/
	/*--------------------------------------------------*/
	switch(tim_num) {
		case	M_CCD_TIM_TCLK1:	/*---* ���������P*--------------------------*/
			fc_ccd_fsm_tout_clk1(ccd);
			break;

		case	M_CCD_TIM_TCSH:		/*---* ��������*----------------------------*/
			fc_ccd_fsm_tout_csh(ccd);
			break;

		case	M_CCD_TIM_TIDLE:	/*---* ����������*--------------------------*/
			fc_ccd_fsm_tout_idle(ccd);
			break;

		case	M_CCD_TIM_TFAIL1:	/*---* �����������P*------------------------*/
			fc_ccd_fsm_tout_fail1(ccd);
			break;

		case	M_CCD_TIM_COMON:	/*---* �ėp�^�C�} *-------------------------*/
			fc_ccd_fsm_tout_comn(ccd);
			break;

		case	M_CCD_TIM_INTFAIL:	/*---*INT FALESAFE�^�C�} *------------------*/
			fc_ccd_fsm_tout_intfale(ccd);
			break;

		default:					/*---* ���̑��i�G���[�j	*-------------------*/
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
*		DESCRIPTION	: �C���X�^���X����������								*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
INT	fc_ccd_init_instance(ST_CCD_MNG_INFO *ccd)
{
	BYTE	i;

	/*--------------------------------------------------*/
	/*  ��t����										*/
	/*--------------------------------------------------*/
	ccd->bf_ccd_acpt_flg = FALSE;

	/*--------------------------------------------------*/
	/*  �|�[�g�ݒ�										*/
	/*--------------------------------------------------*/
	if(ccd->dev.spi.sdev->master->bus_num == 0){
											/* SPI1 = CPU_COM1 �F��e�ʒʐM*/
		ccd->cpu.pin.MCLK = CPU_COM_MCLK1;
		ccd->cpu.pin.MREQ = CPU_COM_MREQ1;
		ccd->cpu.pin.SREQ = CPU_COM_SREQ1;
		ccd->cpu.pin.SDAT = CPU_COM_SDT1;
		ccd->cpu.pin.MDAT = CPU_COM_MDT1;
	}
	else
/*	if(ccd->dev.spi.sdev->master->bus_num == 1) */ {
											 /* SPI2 = CPU_COM0 : �R�}���h�ʐM*/
		ccd->cpu.pin.MCLK = CPU_COM_MCLK0;
		ccd->cpu.pin.MREQ = CPU_COM_MREQ0;
		ccd->cpu.pin.SREQ = CPU_COM_SREQ0;
		ccd->cpu.pin.SDAT = CPU_COM_SDT0;
		ccd->cpu.pin.MDAT = CPU_COM_MDT0;
	}

	/*--------------------------------------------------*/
	/*  wait que������									*/
	/*--------------------------------------------------*/
	ccd->sys.event.tx_flg = 1;
	ccd->sys.event.rx_flg = 1;
	init_waitqueue_head(&ccd->sys.event.tx_evq);
	init_waitqueue_head(&ccd->sys.event.rx_evq);

	/*--------------------------------------------------*/
	/*  Mutex que������									*/
	/*--------------------------------------------------*/
	mutex_init(&ccd->sys.mutex.instnc);
	mutex_init(&ccd->sys.mutex.wt_if);
	mutex_init(&ccd->sys.mutex.rd_if);

	/*--------------------------------------------------*/
	/*  ���[�J�[�X���b�h����							*/
	/*--------------------------------------------------*/
	ccd->mng.wkr_thrd = create_singlethread_workqueue("spi-tegra-cpucom-work");
	if(ccd->mng.wkr_thrd == 0) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0039:ccd->mng.wkr_thrd=0",ccd->dev.spi.id_name);
		fc_ccd_drc_error(39,CHANNEL,0,0,MODE);
		return	-ENOMEM;
	}

	/*--------------------------------------------------*/
	/*  ���[�J�X���b�h�h�e�p�C�x���g��񐶐�			*/
	/*--------------------------------------------------*/
	for(i=0; i<M_CCD_WINF_CNT; i++){
		memset((void*)&ccd->mng.wkr_inf[i], 0, sizeof(ccd->mng.wkr_inf[i]));
		INIT_WORK((struct work_struct*)&ccd->mng.wkr_inf[i], fc_ccd_wthrd_main);
		ccd->mng.wkr_inf[i].eve_num = i;
		ccd->mng.wkr_inf[i].inst_p	= ccd;
	}

	/*--------------------------------------------------*/
	/*  SPI-TEGRA�ݒ�									*/
	/*--------------------------------------------------*/
	ccd->dev.spi.cnfg.is_hw_based_cs = 1;
	ccd->dev.spi.cnfg.cs_setup_clk_count =0;
	ccd->dev.spi.cnfg.cs_hold_clk_count  =0;

	ccd->dev.spi.sdev->controller_data = (void*)&ccd->dev.spi.cnfg;
	ccd->dev.spi.sdev->bits_per_word=8;

	spi_setup(ccd->dev.spi.sdev);

	/*--------------------------------------------------*/
	/*  �^�C�}�[������									*/
	/*--------------------------------------------------*/
	fc_ccd_tim_init(ccd);

	/*--------------------------------------------------*/
	/*	���荞�ݐ���p�r��������						*/
	/*--------------------------------------------------*/
	spin_lock_init(&ccd->cpu.irq.irq_ctl);

	return	ESUCCESS;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_make_gpio_str									*
*																			*
*		DESCRIPTION	: �f�o�h�n�o�^�����񐶐�								*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*					      : *str : ���j�[�N������							*
*																			*
*					  OUT : *out : �A���f�[�^�i�[��							*
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
*		DESCRIPTION	: �e�b�b�v�Z����										*
*																			*
*		PARAMETER	: IN  : buf  : �v�Z�f�[�^								*
*						  : len  : �f�[�^��									*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : FCC�l											*
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
*		DESCRIPTION	: �f�[�^���M����										*
*																			*
*		PARAMETER	: IN  : *ccd	: �R���e�L�X�g							*
*						  : sdat	: ���M�f�[�^							*
*						  : len		: �f�[�^��								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : �����^���s										*
*																			*
*		CAUTION		: �C���X�^���X�r������Ă��Ȃ��̂Œ��ӁI�I				*
*					  �C���X�^���X�f�[�^�Q�ƈȊO�֎~�I						*
*																			*
****************************************************************************/
SHORT	fc_ccd_snd_dat(ST_CCD_MNG_INFO *ccd, BYTE *sdat, USHORT len)
{
	USHORT	i, j;
	USHORT	cnt, mod;
	SHORT	ret;
	BYTE	sig;

	/*--------------------------------------------------*/
	/*	�S�o�C�g�����M����̂ŉ񐔌v�Z				*/
	/*--------------------------------------------------*/
	cnt = len / M_CCD_COMM_PACKET;		/* ���M�N����						*/
	mod = len % M_CCD_COMM_PACKET;		/* ���܂�̑��M�f�[�^��				*/

	/*--------------------------------------------------*/
	/*	�S�o�C�g�����M								*/
	/*--------------------------------------------------*/
	ret = M_CCD_CSTS_OK;				/* �f�t�H���g���n�j					*/
	for(i=0; i<cnt; i++) {

#if	__INTERFACE__ == IF_SPI_TEGRA		/* SPI-TEGRA�ł̒ʐM 				*/
		ret = fc_ccd_snd_spi_res(ccd, &sdat[i*4], 4);
#else /* INTERFACE == IF_GPIO */		/* GPIO����ł̒ʐM 				*/
		for(j=0; j<4; j++) {
			fc_ccd_snd_byte(ccd, sdat[i*4+j]);
		}
#endif

		/*--------------------------------------------------*/
		/*	���f�m�F										*/
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
	/*	�c��̃f�[�^�𑗐M								*/
	/*--------------------------------------------------*/
#if	__INTERFACE__ == IF_SPI_TEGRA			/* SPI-TEGRA�ł̒ʐM */
	ret = fc_ccd_snd_spi_res(ccd, &sdat[i*4], mod);
#else /* INTERFACE == IF_GPIO */			/* GPIO����ł̒ʐM */
	for(j=0; j<mod; j++) {
		fc_ccd_snd_byte(ccd, sdat[i*4+j]);
	}
#endif

	/*--------------------------------------------------*/
	/*	���f�m�F										*/
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
*		DESCRIPTION	: �f�[�^��M����										*
*																			*
*		PARAMETER	: IN  : *ccd	: �R���e�L�X�g							*
						  : len		: �f�[�^��								*
*																			*
*					  OUT : rdat	: ��M�f�[�^							*
*																			*
*					  RET : �����^���s										*
*																			*
*		CAUTION		: �C���X�^���X�r������Ă��Ȃ��̂Œ��ӁI�I				*
*					  ��M�o�b�t�@�i�[�ȊO�C���X�^���X�f�[�^�ύX�֎~�I		*
*																			*
****************************************************************************/
SHORT	fc_ccd_rcv_dat(ST_CCD_MNG_INFO *ccd, BYTE *rdat, USHORT len)
{
	USHORT	i, j;
	USHORT	cnt, mod;
	SHORT	ret;
	BYTE	sig;

	/*--------------------------------------------------*/
	/*	�ݒ�o�C�g����M����̂ŉ񐔌v�Z				*/
	/*--------------------------------------------------*/
	cnt = len / M_CCD_COMM_PACKET;			/* ���M�N����						*/
	mod = len % M_CCD_COMM_PACKET;			/* ���܂�̎�M�f�[�^��				*/

	/*--------------------------------------------------*/
	/*	�ݒ�o�C�g�����M								*/
	/*--------------------------------------------------*/
	ret = M_CCD_CSTS_OK;					/* �f�t�H���g���n�j					*/
	for(i=0; i<cnt; i++) {
		BYTE sig;
#if	__INTERFACE__ == IF_SPI_TEGRA		/* SPI-TEGRA�ł̒ʐM */
		ret = fc_ccd_rcv_spi_res(ccd, &rdat[i*M_CCD_COMM_PACKET], M_CCD_COMM_PACKET);
#else /* __INTERFACE__ == IF_GPIO */	/* GPIO����ł̒ʐM */
		for(j=0; j<M_CCD_COMM_PACKET; j++) {
			rdat[i*M_CCD_COMM_PACKET+j] = fc_ccd_rcv_byte(ccd);
		}
#endif

		/*--------------------------------------------------*/
		/*	���f�m�F										*/
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
	/*	�c��̃f�[�^�𑗐M								*/
	/*--------------------------------------------------*/
#if	__INTERFACE__ == IF_SPI_TEGRA			/* SPI-TEGRA�ł̒ʐM */
	ret = fc_ccd_rcv_spi_res(ccd, &rdat[i*M_CCD_COMM_PACKET], mod);
#else /* __INTERFACE__ == IF_GPIO */		/* GPIO����ł̒ʐM */
	for(j=0; j<mod; j++) {
		rdat[i*M_CCD_COMM_PACKET+j] = fc_ccd_rcv_byte(ccd);
	}
#endif

	/*--------------------------------------------------*/
	/*	���f�m�F										*/
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
*		DESCRIPTION	: �P�o�C�g���M�����i�|�[�g����j						*
*																			*
*		PARAMETER	: IN  : *ccd	: �R���e�L�X�g							*
*						  : dat		: ���M�f�[�^							*
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
*		DESCRIPTION	: �P�o�C�g��M�����i�|�[�g����j						*
*																			*
*		PARAMETER	: IN  : *ccd	: �R���e�L�X�g							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : dat		: ��M�f�[�^							*
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
*		DESCRIPTION	: ���M�����i�V���A�����\�[�X����j						*
*																			*
*		PARAMETER	: IN  : *ccd	: �R���e�L�X�g							*
*						  : sdat	: ���M�f�[�^							*
*						  : len		: �f�[�^��								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : �����^���s										*
*																			*
****************************************************************************/
#if	__INTERFACE__ == IF_SPI_TEGRA			/* SPI-TEGRA�ł̒ʐM */
SHORT	fc_ccd_snd_spi_res(ST_CCD_MNG_INFO *ccd, BYTE *sdat, USHORT len)
{
	struct spi_transfer		spi_set;
	struct spi_message		msg;
	ssize_t					ret = 0;
	/*--------------------------------------------------*/
	/*	�r�o�h�ւ̑��M�w�����b�Z�[�W�쐬				*/
	/*--------------------------------------------------*/
	memset(&spi_set, 0, sizeof(spi_set));
	spi_set.tx_buf	= sdat;
	spi_set.len		= len;

	spi_message_init(&msg);
	spi_message_add_tail(&spi_set, &msg);

	/*--------------------------------------------------*/
	/*	�r�o�h�]��������s								*/
	/*--------------------------------------------------*/
	ret = fc_ccd_spicomm_ctrl(ccd, &msg);

	/*--------------------------------------------------*/
	/*	�]�������w���ʂ�Ő���I��						*/
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
#if	__INTERFACE__ == IF_SPI_TEGRA			/* SPI-TEGRA�ł̒ʐM */
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_rcv_spi_res									*
*																			*
*		DESCRIPTION	: ��M�����i�V���A�����\�[�X����j						*
*																			*
*		PARAMETER	: IN  : *ccd	: �R���e�L�X�g							*
						  : len		: �f�[�^��								*
*																			*
*					  OUT : rdat	: ��M�f�[�^							*
*																			*
*					  RET : �����^���s										*
*																			*
****************************************************************************/
SHORT	fc_ccd_rcv_spi_res(ST_CCD_MNG_INFO *ccd, BYTE *rdat, USHORT len)
{
	struct spi_transfer		spi_set;
	struct spi_message		msg;
	ssize_t					ret = 0;
	/*--------------------------------------------------*/
	/*	�r�o�h�ւ̎�M�w�����b�Z�[�W�쐬				*/
	/*--------------------------------------------------*/
	memset(&spi_set, 0, sizeof(spi_set));
	spi_set.rx_buf	= rdat;
	spi_set.len		= len;

	spi_message_init(&msg);
	spi_message_add_tail(&spi_set, &msg);

	/*--------------------------------------------------*/
	/*	�r�o�h�]��������s								*/
	/*--------------------------------------------------*/
	ret = fc_ccd_spicomm_ctrl(ccd, &msg);

	/*--------------------------------------------------*/
	/*	�]�������w���ʂ�Ő���I��						*/
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
*		DESCRIPTION	: �r�o�h����M���䏈��									*
*																			*
*		PARAMETER	: IN  : *ccd	: �R���e�L�X�g							*
*						  : *msg	: �r�o�h���b�Z�[�W						*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : �T�C�Y / ���^�[���R�[�h							*
*																			*
****************************************************************************/
#if __INTERFACE__ == IF_SPI_TEGRA	/* SPI-TEGRA�ł̒ʐM */
ssize_t	fc_ccd_spicomm_ctrl(ST_CCD_MNG_INFO *ccd, struct spi_message *msg)
{
	DECLARE_COMPLETION_ONSTACK(done);
	INT		status;

	msg->complete	= fc_ccd_spicomm_cb;
	msg->context	= &done;

	spin_lock_irq(&ccd->dev.spi.lockt);

	/*--------------------------------------------------*/
	/*	�r�o�h�֑���M�w�����b�Z�[�W���M				*/
	/*--------------------------------------------------*/
	if(ccd->dev.spi.sdev == NULL) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0042:sdev=NULL",ccd->dev.spi.id_name);
		fc_ccd_drc_error(42,CHANNEL,0,0,MODE);
		status = -ESHUTDOWN;
	}else{
		status = spi_async(ccd->spi.sdev, msg);
											/* ���b�Z�[�W���M					*/
	}

	spin_unlock_irq(&ccd->dev.spi.lockt);

	if(status != 0) {
		/* ERROR TRACE */
		LOGE("[%s]ERR0043:sdev=NULL",ccd->dev.spi.id_name);
		fc_ccd_drc_error(43,CHANNEL,0,0,MODE);

		return status;						/* �ُ�I��							*/
	}

	/*--------------------------------------------------*/
	/*	�r�o�h�]�������҂�								*/
	/*--------------------------------------------------*/
	wait_for_completion_interruptible(&done);
	status = msg->status;

	/*--------------------------------------------------*/
	/*	����I�����͓]���o�C�g����Ԃ�l�Ƃ���			*/
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
*		DESCRIPTION	: �r�o�h�]�������R�[���o�b�N							*
*																			*
*		PARAMETER	: IN  : *arg 	: ���g�p								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
#if __INTERFACE__ == IF_SPI_TEGRA	/* SPI-TEGRA�ł̒ʐM */
VOID	fc_ccd_spicomm_cb(void *arg)
{
	/*--------------------------------------------------*/
	/*	�]�������̑҂����N����							*/
	/*--------------------------------------------------*/
	complete(arg);
}
#endif

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_tim_init										*
*																			*
*		DESCRIPTION	: �^�C�}������											*
*																			*
*		PARAMETER	: IN  : *ccd	: �R���e�L�X�g							*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : ���^�[���R�[�h									*
*																			*
****************************************************************************/
VOID	fc_ccd_tim_init(ST_CCD_MNG_INFO *ccd)
{
	INT		i;

	/*--------------------------------------------------*/
	/*	�e�^�C�}�����ݒ�								*/
	/*--------------------------------------------------*/
	for(i=0; i<M_CCD_TIM_MAX; i++) {
		ccd->tim.tmr_inf[i].inst_p			= (void*)ccd;
		ccd->tim.tmr_inf[i].tim_num			= i;
		ccd->tim.tmr_inf[i].usec			= tb_ccd_timer_tim[i];

		hrtimer_init(&ccd->tim.tmr_inf[i].hndr, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

		/*--------------------------------------------------*/
		/*	�R�[���o�b�N�o�^�͂h�m�h�s�̌�ɐݒ肷��		*/
		/*--------------------------------------------------*/
		ccd->tim.tmr_inf[i].hndr.function	= fs_ccd_timeout_cb;
	}

	return;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_tim_start										*
*																			*
*		DESCRIPTION	: �^�C�}�[�X�^�[�g										*
*																			*
*		PARAMETER	: IN  : *ccd	: �R���e�L�X�g							*
*							timid	: �^�C�}ID								*
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
	/*	�N�����Ă������U�~�߂�						*/
	/*--------------------------------------------------*/
	if(hrtimer_active(&ccd->tim.tmr_inf[timid].hndr)) {
		fc_ccd_tim_stop(ccd, timid);
	}

	/*--------------------------------------------------*/
	/*	�^�C�}���Ԑݒ�									*/
	/*--------------------------------------------------*/
	sec  = M_CCD_MCR_US2SEC(ccd->tim.tmr_inf[timid].usec);
	nano = M_CCD_MCR_US2NS (ccd->tim.tmr_inf[timid].usec % M_CCD_MCR_SEC2US(1));
	time = ktime_set(sec, nano);

	/*--------------------------------------------------*/
	/*	�^�C�}�N��										*/
	/*--------------------------------------------------*/
	hrtimer_start(&ccd->tim.tmr_inf[timid].hndr, time, HRTIMER_MODE_REL);
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_tim_stop										*
*																			*
*		DESCRIPTION	: �^�C�}�[��~											*
*																			*
*		PARAMETER	: IN  : *ccd	: �R���e�L�X�g							*
*							timid	: �^�C�}ID								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_tim_stop(ST_CCD_MNG_INFO *ccd, USHORT timid)
{
	/*--------------------------------------------------*/
	/*	�^�C�}�[�L�����Z��								*/
	/*--------------------------------------------------*/
	hrtimer_try_to_cancel(&ccd->tim.tmr_inf[timid].hndr);
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_tim_all_stop									*
*																			*
*		DESCRIPTION	: �S�^�C�}�[��~										*
*																			*
*		PARAMETER	: IN  : *ccd	: �R���e�L�X�g							*
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
	/*	�e�^�C�}�����ݒ�								*/
	/*--------------------------------------------------*/
	for(i=0; i<M_CCD_TIM_MAX; i++) {
		hrtimer_try_to_cancel(&ccd->tim.tmr_inf[i].hndr);
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_set_sig_cs										*
*																			*
*		DESCRIPTION	: �b�r�M�����䏈��										*
*																			*
*		PARAMETER	: IN  : *ccd	: �R���e�L�X�g							*
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
	/*	�ݒ��Ԃɕω����Ȃ��Ƃ��͂��̂܂ܐݒ�			*/
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
	/*	�ݒ��Ԃɕω�������Ƃ��͂k���Ɏ��Ԋm�ۂ���	*/
	/*	�r�g�}�C�R���̎�蓦���΍�						*/
	/*--------------------------------------------------*/
	else {
		/*--------------------------------------------------*/
		/*	�b�r�k���͍Œ᎞�ԁi200us�j�ێ�����				*/
		/*--------------------------------------------------*/
		if(act == ACTIVE) {
			for(i=0; i<500; i++) {
				usec = ktime_us_delta(ktime_get(), ccd->cpu.prt.cs_tim);
				/*--------------------------------------------------*/
				/*	200us�o��or���ԃI�[�o�t���[�łb�r�Z�b�g����		*/
				/*--------------------------------------------------*/
				if((usec < 0) || (usec > 200)) {
					break;
				}
				/*--------------------------------------------------*/
				/*	�C�����[�v�ɂď����҂�							*/
				/*--------------------------------------------------*/
#if CPUCOMMODULE==1
				cm_delay(100);
#else
				__delay(500);				/* 1usec���傢���炢�E				*/
#endif
			}
			gpio_set_value(ccd->cpu.pin.MREQ, LOW);
		}
		/*--------------------------------------------------*/
		/*	�b�r�g�͎��Ԏ擾���Ă��̂܂ܐݒ�				*/
		/*--------------------------------------------------*/
		else {
			ccd->cpu.prt.cs_tim = ktime_get();
			gpio_set_value(ccd->cpu.pin.MREQ, HIGH);
		}
		/*--------------------------------------------------*/
		/*	�b�r�ɐM���o�͐ݒ�								*/
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
*		DESCRIPTION	: �h�m�s�M���擾����									*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
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
*		DESCRIPTION	: ���\�[�X�J������										*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
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
	/*	����M�o�b�t�@�N���A							*/
	/*--------------------------------------------------*/
	memset(&(ccd->comm.tx_buf), 0, sizeof(ccd->comm.tx_buf));
	memset(&(ccd->comm.rx_buf), 0, sizeof(ccd->comm.rx_buf));

	/*--------------------------------------------------*/
	/*	��M�L���[���������J��							*/
	/*--------------------------------------------------*/
	for(i=0; i<1000; i++) {
		ret = list_empty(&ccd->comm.rx_que);
		if(ret == 0) {						/* ���X�g����łȂ�					*/
			del = list_first_entry(&ccd->comm.rx_que, ST_CCD_TXRX_BUF, list);
											/* ���X�g�擾						*/
			list_del(&del->list);			/* ���X�g����폜					*/
			kfree(del);						/* �������J��						*/
		}
		else {								/* ���X�g����						*/
			break;							/* ���X�g�폜����					*/
		}
	}
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_fsm_clr										*
*																			*
*		DESCRIPTION	: ��ԑJ�ڃN���A����									*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
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
	/*	����M�҂��̑҂�����							*/
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
	/*	�b�r�M���C���A�N�e�B�u							*/
	/*--------------------------------------------------*/
	fc_ccd_set_sig_cs(ccd, NEGATIVE);

	/*--------------------------------------------------*/
	/*	����M�o�b�t�@�N���A							*/
	/*--------------------------------------------------*/
	memset(&ccd->comm.tx_buf, 0, sizeof(ccd->comm.tx_buf));
	memset(&ccd->comm.rx_buf, 0, sizeof(ccd->comm.rx_buf));

	/*--------------------------------------------------*/
	/*	�S�^�C�}��~									*/
	/*--------------------------------------------------*/
	fc_ccd_tim_all_stop(ccd);

	/*--------------------------------------------------*/
	/*	�ėp�^�C�}�N��									*/
	/*--------------------------------------------------*/
	fc_ccd_tim_start(ccd, M_CCD_TIM_COMON);
											/* �^�C�}�N��					*/
	ccd->tim.tcnt = 0;						/* �^�C�}�J�E���^�N���A			*/

	/*--------------------------------------------------*/
	/*	��Ԃ��A�C�h���֕ύX							*/
	/*--------------------------------------------------*/
	fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C1_IDLE);

	/*--------------------------------------------------*/
	/*	�h�m�s�M�����A�N�e�B�u�̏ꍇ�͈�U�A			*/
	/*	�h�m�s�g�҂���Ԃ֑J�ڂ����h�m�s�l�K�e�B�u��҂�*/
	/*--------------------------------------------------*/
	sig = fc_ccd_get_sig_int(ccd);
	if(sig == ACTIVE) {
		fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_E0_INTE);
											/* �h�m�s�G���[��Ԃ֑J��			*/
	}
	ccd->cpu.irq.sreq_sts = sig;
	ccd->cpu.irq.sreq_selfsts = 0xFF;

}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_init_drv_inf									*
*																			*
*		DESCRIPTION	: �h���C�o��񏉊�������								*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_init_drv_inf(ST_CCD_MNG_INFO *ccd)
{
	/*--------------------------------------------------*/
	/*	�b�r�M���C���A�N�e�B�u							*/
	/*--------------------------------------------------*/
	fc_ccd_set_sig_cs(ccd, NEGATIVE);

	/*--------------------------------------------------*/
	/*	���荞�݋֎~									*/
	/*--------------------------------------------------*/
	fc_ccd_disable_irq(ccd);

	/*--------------------------------------------------*/
	/*	�S�^�C�}��~									*/
	/*--------------------------------------------------*/
	fc_ccd_tim_all_stop(ccd);

	/*--------------------------------------------------*/
	/*	���\�[�X�J��									*/
	/*--------------------------------------------------*/
	fc_ccd_rel_res(ccd);

	/*--------------------------------------------------*/
	/*	��Ԃ��������O�֕ύX							*/
	/*--------------------------------------------------*/
	ccd->mng.mode = M_CCD_MODE_C0_INIT;

	/*--------------------------------------------------*/
	/*	�Ǘ����N���A									*/
	/*--------------------------------------------------*/
	ccd->tim.tcnt = 0;
	ccd->tim.mchk = 0;

	/*--------------------------------------------------*/
	/*	��t����										*/
	/*--------------------------------------------------*/
	ccd->bf_ccd_acpt_flg = FALSE;
	/*--------------------------------------------------*/
	/*	���荞�ݎ��Ȑݒ�								*/
	/*--------------------------------------------------*/
	ccd->cpu.irq.sreq_selfsts = 0xFF;

}
/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_chk_rcv_dat									*
*																			*
*		DESCRIPTION	: ��M�f�[�^�m�F����									*
*																			*
*		PARAMETER	: IN  : *rcv : ��M�f�[�^								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : �n�j�^�m�f										*
*																			*
****************************************************************************/
SHORT	fc_ccd_chk_rcv_dat(ST_CCD_TXRX_BUF *rcv)
{
	ULONG	clc_fcc, rcv_fcc;

	/*--------------------------------------------------*/
	/*	�e�b�b�擾										*/
	/*--------------------------------------------------*/
	rcv_fcc  = 0;
	rcv_fcc |= (ULONG)((((ULONG)(rcv->dat[rcv->len+0]) << 24) & 0xFF000000));
	rcv_fcc |= (ULONG)((((ULONG)(rcv->dat[rcv->len+1]) << 16) & 0x00FF0000));
	rcv_fcc |= (ULONG)((((ULONG)(rcv->dat[rcv->len+2]) <<  8) & 0x0000FF00));
	rcv_fcc |= (ULONG)((((ULONG)(rcv->dat[rcv->len+3])      ) & 0x000000FF));

	/*--------------------------------------------------*/
	/*	�e�b�b�v�Z										*/
	/*--------------------------------------------------*/
	clc_fcc = fc_ccd_calc_fcc(rcv->dat, rcv->len);
											/* FCC�v�Z							*/
	clc_fcc = clc_fcc + rcv->rcv_len;		/* LEN�f�[�^�𑫂�					*/

	/*--------------------------------------------------*/
	/*	�e�b�b�m�F										*/
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
*		DESCRIPTION	: ��M���X�g�i�[����									*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*						  : *rcv : ��M�f�[�^								*
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
	/*	�������m�ۂ��Ď�M�f�[�^���R�s�[				*/
	/*--------------------------------------------------*/
	ent = kmalloc(sizeof(*ent), GFP_KERNEL);
	if(ent == NULL) {						/* �m�ێ��s�I�I						*/
		/* ERROR TRACE */
		LOGE("[%s]ERR0044:ent=NULL",ccd->dev.spi.id_name);
		fc_ccd_drc_error(44,CHANNEL,0,0,MODE);
		return;
	}
	memcpy(ent, rcv, sizeof(*ent));			/* �o�^�p�f�[�^�ɃR�s�[				*/
	INIT_LIST_HEAD(&ent->list);

	/*--------------------------------------------------*/
	/*	��M�L���[���X�g�Ɋi�[							*/
	/*--------------------------------------------------*/
	list_add_tail(&ent->list, &ccd->comm.rx_que);
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_chg_fsm_mod									*
*																			*
*		DESCRIPTION	: ��ԑJ�ڏ�ԕύX����									*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*						  : mode : �ύX�惂�[�h								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_chg_fsm_mod(ST_CCD_MNG_INFO *ccd, BYTE mode)
{
	/*--------------------------------------------------*/
	/*	���[�h�ω�����Ƃ��͌Œ��Ď��J�E���^�N���A		*/
	/*--------------------------------------------------*/
	if(ccd->mng.mode != mode) {
		ccd->tim.mchk = 0;
	}

	/*--------------------------------------------------*/
	/*	���[�h�ύX										*/
	/*--------------------------------------------------*/
	LOGT("[%s]MODE CHANGE EVENT [%s  ->  %s]",ccd->dev.spi.id_name,stateTBL[ccd->mng.mode].str,stateTBL[mode].str);
	ccd->mng.previous_mode = ccd->mng.mode;
	ccd->mng.mode = mode;
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_rdy_shtdwn										*
*																			*
*		DESCRIPTION	: �V���b�g�_�E����������								*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
*																			*
*					  OUT : NONE											*
*																			*
*					  RET : NONE											*
*																			*
****************************************************************************/
VOID	fc_ccd_rdy_shtdwn(ST_CCD_MNG_INFO *ccd)
{
	/*--------------------------------------------------*/
	/*	�b�r���g�o�́i�C���A�N�e�B�u�j					*/
	/*--------------------------------------------------*/
	fc_ccd_set_sig_cs(ccd, NEGATIVE);

	/*--------------------------------------------------*/
	/*	��ԑJ�ڏ��N���A								*/
	/*--------------------------------------------------*/
	fc_ccd_fsm_clr(ccd);

	/*--------------------------------------------------*/
	/*	����M�҂��̑҂�����							*/
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
	/*	�������҂��֑J��								*/
	/*--------------------------------------------------*/
	fc_ccd_chg_fsm_mod(ccd, M_CCD_MODE_C0_INIT);
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_disable_irq									*
*																			*
*		DESCRIPTION	: ���荞�݊����݋֎~����								*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
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
	/*	�r�o�h�m�k�n�b�j�ł̔r���@�J�n					*/
	/*--------------------------------------------------*/
	spin_lock_irqsave(&ccd->cpu.irq.irq_ctl,flags);

	/*--------------------------------------------------*/
	/*	�h�m�s�M�������ݐ���							*/
	/*--------------------------------------------------*/
	if(ccd->cpu.irq.ena_sts == M_CCD_ENABLE) {
		disable_irq(ccd->cpu.irq.sreq_num);
	}

	/*--------------------------------------------------*/
	/*	���荞�ݏ�Ԑݒ�								*/
	/*--------------------------------------------------*/
	ccd->cpu.irq.ena_sts = M_CCD_DISABLE;

	/*--------------------------------------------------*/
	/*	�r�o�h�m�k�n�b�j�ł̔r���@�I��					*/
	/*--------------------------------------------------*/
	spin_unlock_irqrestore(&ccd->cpu.irq.irq_ctl,flags);
}

/****************************************************************************
*																			*
*		SYMBOL		: fc_ccd_enable_irq										*
*																			*
*		DESCRIPTION	: �h���C�o���荞�݋�����								*
*																			*
*		PARAMETER	: IN  : *ccd : �R���e�L�X�g								*
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
	/*	���荞�݋֎~���Ԓ��Ƀ^�C�}�����݂�				*/
	/*	�������Ă����珈������							*/
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
	/*	�r�o�h�m�k�n�b�j�ł̔r���@�J�n					*/
	/*--------------------------------------------------*/
	spin_lock_irqsave(&ccd->cpu.irq.irq_ctl,flags);

	/*--------------------------------------------------*/
	/*	�h�m�s�M�������ݐ���							*/
	/*--------------------------------------------------*/
	if(ccd->cpu.irq.ena_sts == M_CCD_DISABLE) {
		enable_irq(ccd->cpu.irq.sreq_num);
	}

	/*--------------------------------------------------*/
	/*	���荞�ݏ�Ԑݒ�								*/
	/*--------------------------------------------------*/
	ccd->cpu.irq.ena_sts = M_CCD_ENABLE;

	/*--------------------------------------------------*/
	/*	�r�o�h�m�k�n�b�j�ł̔r���@�I��					*/
	/*--------------------------------------------------*/
	spin_unlock_irqrestore(&ccd->cpu.irq.irq_ctl,flags);
}
/****************************************************************************
*				drv_spi_cm.c	END											*
****************************************************************************/




