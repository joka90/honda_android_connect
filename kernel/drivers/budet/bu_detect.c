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

/*----------------------------------------------------------------------------*/
// COPYRIGHT(C) FUJITSU TEN LIMITED 2012
/*----------------------------------------------------------------------------*/

#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/delay.h>
#include "../../arch/arm/mach-tegra/gpio-names.h"
#include "../../arch/arm/mach-tegra/pm-irq.h"
#include "../../arch/arm/mach-tegra/board-e1853.h"
#include <linux/pm_runtime.h>
#include <linux/bu_notifier.h>
#include <linux/bu_trace.h>

/* FUJITSU TEN:2012-12-14 Plan-G Start */
#include <linux/ada_suspend.h>
#if	BUDET_SYSFS_DEVICE_CREATE
#include <linux/kobject.h>
#include <linux/sysfs.h>
#endif	/* BUDET_SYSFS_DEVICE_CREATE */
/* FUJITSU TEN:2012-12-14 Plan-G End */

/* driver callback */
#include <linux/earlydevice.h>
extern int early_device_suspend_notify(void);

/* SUSPEND state */
#include <linux/suspend.h>
#include <mach/gpio.h>

/* defines for the setting of tristate pins */
#include <mach/iomap.h>
#include <mach/pinmux.h>
#include <mach/gpio_ada_tristate.h>

MODULE_LICENSE("GPL");

/* for DEBUG */
#if 0
#define DEF_BUDET_KPRINT
#endif

/* TG_WAKEUP_REQ interrupt setting */
#if 1
#define DEF_WAKEUP_REQ_ENABLE
#endif

/* defines GPIO High/Low */
#define BU_LOW			0
#define BU_HIGH			1

/* BU-DET mode configuration value  */
unsigned int budet_suspend_config = BUDET_MODE_MASK_DEFAULT;

/* unmask mode */
unsigned int budet_resume_unmask_mode = BUDET_RESUME_UNMASK_MODE_DEFAULT;

/* Waittime settings */
unsigned int budet_suspend_force_taskfreeze_budet_timeout_time = BUDET_SUSPEND_FORCE_TASK_FREEZE_BUDET_WAITTIME ;
unsigned int budet_suspend_force_taskfreeze_accoff_timeout_time = BUDET_SUSPEND_FORCE_TASK_FREEZE_ACCOFF_WAITTIME ;

unsigned int budet_suspend_async_taskfreeze_timeout_time = BUDET_SUSPEND_ASYNC_TASK_FREEZE_WAITTIME;

/* work queue */
static struct work_struct		workq;
/* bu-det sleep/wakeup workqueue */
static struct workqueue_struct		*wq_st = NULL ;

static DEFINE_SPINLOCK(bu_suspend_spinlock);
static bool bu_detect_suspend_oprating = false;

/* FUJITSU TEN:2013-10-01 HDMI SUSPEND add Start */
/* BU-DET notify(for user land) value */
unsigned int budet_notify_value = 0;

/* BU-DET complete(from user land) value */
unsigned int budet_complete_value = 0;

/* BU-DET (for/from user land) semaphore */
static struct rw_semaphore rw_sem;
/* FUJITSU TEN:2013-10-01 HDMI SUSPEND add End */

/* FUJITSU TEN:2013-11-21 HDMI SUSPEND add Start */
/* macros for loop */
#define BUDET_LOOP_WAIT_TIME		(20)
/* FUJITSU TEN:2013-11-21 HDMI SUSPEND add End */

static DEFINE_MUTEX(bu_suspend_workqueue_op_mutex);

/* suspend function(extern) */
extern int enter_state(suspend_state_t state);

/* macros for GPIO */
#define GPIO_BANK(x)		((x) >> 5)
#define GPIO_PORT(x)		(((x) >> 3) & 0x3)

/* macros for acquiring the address of GPIO register */
#define GPIO_REG(x)			(IO_TO_VIRT(TEGRA_GPIO_BASE) +	\
							GPIO_BANK(x) * 0x100 +			\
							GPIO_PORT(x) * 4)

#define GPIO_BIT(x)			((x) & 0x7)

/* GPIO interrupt status register */
#define GPIO_INT_STA(x)     (GPIO_REG(x) + 0x40)
/* GPIO interrupt enable register */
#define GPIO_INT_ENB(x)     (GPIO_REG(x) + 0x50)

/* FUJITSU TEN:2013-10-01 HDMI SUSPEND add Start */
#if	BUDET_SYSFS_DEVICE_CREATE
static struct kobject *budet_sysfs_root_kobject = NULL;
#endif /* BUDET_SYSFS_DEVICE_CREATE */

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_notify_read                                                    */
/*  [DESCRIPTION]                                                            */
/*      サスペンドのユーザランド通知読出                                     */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj          : kobject                              */
/*      struct kobj_attribute *attr   : kobjectの属性                        */
/*      char *buf                     : 出力用の文字バッファ                 */
/*  [RETURN]                                                                 */
/*      出力文字数                                                           */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static ssize_t budet_notify_read( struct kobject *kobj, struct kobj_attribute *attr, char *buf )
{
	printk("budet_notify_read start:rw_sem.activity = %d\n", rw_sem.activity);
	return sprintf( buf , "%d" , budet_notify_value );
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_notify_write                                                   */
/*  [DESCRIPTION]                                                            */
/*      サスペンドのユーザランド通知書込                                     */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj          : kobject                              */
/*      struct kobj_attribute *attr   : kobjectの属性                        */
/*      char *buf                     : 入力文字バッファ                     */
/*      size_t count                  : 入力文字数                           */
/*  [RETURN]                                                                 */
/*      処理文字数                                                           */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static ssize_t budet_notify_write(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int	val;
	int ret;

	printk("budet_notify_write start:rw_sem.activity = %d\n", rw_sem.activity);
	ret = mutex_lock_interruptible(&bu_suspend_workqueue_op_mutex);
	if (ret == -EINTR) {
		return ret;
	}

	sscanf(buf, "%du", &val);

	budet_notify_value = val;
	// kernel側で待ち合わせる
	if (rw_sem.activity <= 1) {
		down_read(&rw_sem);
	} else {
		printk("budet_notify_write error :rw_sem.activity = %d\n", rw_sem.activity);
	}

	mutex_unlock(&bu_suspend_workqueue_op_mutex);

	printk("budet_notify_write end\n");
	return count;
}

/* notify用のsysfsファイル属性 */
static struct kobj_attribute budet_sysfs_notify_attribute = \
	__ATTR(BUDET_SYSFS_NOTIFY_DEVICE_NAME, 0600, budet_notify_read, budet_notify_write);

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_complete_read                                                  */
/*  [DESCRIPTION]                                                            */
/*      サスペンドのkernel側待ち合わせ解除用読出                             */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj          : kobject                              */
/*      struct kobj_attribute *attr   : kobjectの属性                        */
/*      char *buf                     : 出力用の文字バッファ                 */
/*  [RETURN]                                                                 */
/*      出力文字数                                                           */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static ssize_t budet_complete_read( struct kobject *kobj, struct kobj_attribute *attr, char *buf )
{
	printk("budet_complete_read start:rw_sem.activity = %d\n", rw_sem.activity);
	return sprintf( buf , "%d" , budet_complete_value );
}
/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_complete_write                                                 */
/*  [DESCRIPTION]                                                            */
/*      サスペンドのkernel側待ち合わせ解除用書込                             */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj          : kobject                              */
/*      struct kobj_attribute *attr   : kobjectの属性                        */
/*      char *buf                     : 入力文字バッファ                     */
/*      size_t count                  : 入力文字数                           */
/*  [RETURN]                                                                 */
/*      処理文字数                                                           */
/*  [NOTE]                                                                   */
/*      ユーザランド側でnotify後のサスペンド有無に関わらず、必ず解放すること */
/*****************************************************************************/
static ssize_t budet_complete_write(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int	val;
	printk("budet_complete_write start:rw_sem.activity = %d\n", rw_sem.activity);
	
	sscanf(buf, "%du", &val);

	budet_complete_value = val;

	if (rw_sem.activity == 1) {
		// 待ち合わせ用セマフォ解放
		up_read(&rw_sem);
	} else {
		printk("budet_complete_write error :rw_sem.activity = %d\n", rw_sem.activity);
	}
	
	printk("budet_complete_write end\n");
	return count;
}

/* complete用のsysfsファイル属性 */
static struct kobj_attribute budet_sysfs_complete_attribute = \
	__ATTR(BUDET_SYSFS_COMPLETE_DEVICE_NAME, 0600, budet_complete_read, budet_complete_write);

/* FUJITSU TEN:2013-10-01 HDMI SUSPEND add End */


/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      ada_gpio_mask_write                                                  */
/*  [DESCRIPTION]                                                            */
/*      mask the interrupt of GPIO                                           */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      u32       reg:        I       register to mask                       */
/*      int       gpio:       I       GPIO number                            */
/*      int       value:      I       setting value                          */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      mask the interrupt of the corresponding GPIO number                  */
/*****************************************************************************/
static void ada_gpio_mask_write(u32 reg, int gpio, int value)
{
	u32 val;

#ifdef DEF_BUDET_KPRINT
	printk("ada_gpio_mask_write write reg(0x%x)\n", reg);
	printk("ada_gpio_mask_write write gpio(0x%x)\n", gpio);
	printk("ada_gpio_mask_write write value(0x%x)\n", value);
	printk("ada_gpio_mask_write register value(0x%x)\n", __raw_readl(reg));
#endif

	/* set INT MASK */
	val = 0x100 << GPIO_BIT(gpio);
	if (value)
		val |= 1 << GPIO_BIT(gpio);
	__raw_writel(val, reg);
#ifdef DEF_BUDET_KPRINT
	printk("ada_gpio_mask_write register value(0x%x)\n", __raw_readl(reg));
#endif
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_get_gpio_budet_pin_state                                       */
/*  [DESCRIPTION]                                                            */
/*      Get current BUDET pin state                                          */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      0: off, !0: on                                                       */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
int budet_get_gpio_budet_pin_state(void)
{
	/* BU_DET(R.02): High=Inactive, Low=Active */
	return !gpio_get_value(TEGRA_GPIO_PR2);
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_get_gpio_accoff_pin_state                                      */
/*  [DESCRIPTION]                                                            */
/*      Get current ACCOFF pin state                                         */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      0: off, !0: on                                                       */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
int budet_get_gpio_accoff_pin_state(void)
{
	/* ACCOFF(S.00): High=Inactive, Low=Active */
	return !gpio_get_value(TEGRA_GPIO_PS0);
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      bu_det_workqueue                                                     */
/*  [DESCRIPTION]                                                            */
/*      work queue for interrupt                                             */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      struct work_struct  *work:  I       work queue                       */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      process the work queue from ISR interrupt                            */
/*****************************************************************************/
void bu_det_workqueue(struct work_struct *work)
{
	int error = -EINVAL;

	/* wait until get mutex */
	mutex_lock(&bu_suspend_workqueue_op_mutex);
	
/* FUJITSU TEN:2013-10-01 HDMI SUSPEND add Start */
#if	BUDET_SYSFS_DEVICE_CREATE
/* FUJITSU TEN:2013-11-21 HDMI SUSPEND del Start */
#if 0
	printk("camera close start\n");
	budet_suspend_config += 1;
	printk("notify for poll\n");
	sysfs_notify(budet_sysfs_root_kobject, NULL, "notify");  //poll解除のsysfs_notify

	printk("camera close down_write done, rw_sem.activity = %d\n", rw_sem.activity);
	// kernel側をwaitし、complete書き込み待ち
	down_write(&rw_sem); 

	printk("camera close up_write done\n");
	up_write(&rw_sem);
#endif
/* FUJITSU TEN:2013-11-21 HDMI SUSPEND del End */
/* FUJITSU TEN:2013-11-21 HDMI SUSPEND add Start */
	printk("camera close start\n");
	while(1) {
		budet_notify_value += 1;
		sysfs_notify(budet_sysfs_root_kobject, NULL, "notify");  //poll解除のsysfs_notify
		// kernel側をwait可能になるまで待ち合わせ、userからのcomplete書き込みを待つ
		if (down_write_trylock(&rw_sem)) {
			printk("down_write_trylock successful(%d)\n", budet_notify_value);
			break;
		} else {
			if ((budet_notify_value % 50) == 0) {
				printk("down_write_try_lock takes over 1s.\n");
			}
			current->state = TASK_INTERRUPTIBLE;  // 寝た状態
			schedule_timeout(msecs_to_jiffies(BUDET_LOOP_WAIT_TIME)); // wait
		}
	}

	printk("camera close up_write done\n");
	up_write(&rw_sem);
	budet_notify_value = 0;
/* FUJITSU TEN:2013-11-21 HDMI SUSPEND add End */
	printk("camera close end\n");
#endif
/* FUJITSU TEN:2013-10-01 HDMI SUSPEND add End */
	/* suspend start */
	error = enter_state(PM_SUSPEND_MEM);

	if( error ){
		/* enter_state(suspend/resume start) failed */
		printk( "bu_det_workqueue: ENTER_STATE FAILED!(%d)\n", error );
	}
	
#ifdef DEF_BUDET_KPRINT
	printk( "bu_det_workqueue(%d) error(%d)\n", __LINE__, error );
#endif

	mutex_unlock(&bu_suspend_workqueue_op_mutex);
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      tg_bu_det_interrupt                                                  */
/*  [DESCRIPTION]                                                            */
/*      interrupt handler                                                    */
/*  [TYPE]                                                                   */
/*      irqreturn_t                                                          */
/*  [PARAMETER]                                                              */
/*      int       irq:        I       IRQ number                             */
/*      void      *dev_id:    I       device ID                              */
/*  [RETURN]                                                                 */
/*      IRQ_HANDLED                                                          */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static irqreturn_t tg_bu_det_interrupt(int irq, void *dev_id)
{
	int ret = 0;
	bool bu_detect_interrupt_nested = false;
#ifdef DEF_BUDET_KPRINT
    int gpio_value = BU_LOW;
#endif

#ifdef DEF_BUDET_KPRINT
	printk("Start tg_bu_det_interrupt(%d)\n",__LINE__);
#endif
	LITES_SET_SUSPEND_STATE(SET_SUSPEND_SQ_S);
	FASTBOOT_LOG_TRACE(0x00);
	/* check interrupt is nested. */
	spin_lock( &bu_suspend_spinlock );
	bu_detect_interrupt_nested = bu_detect_suspend_oprating ;
	bu_detect_suspend_oprating = true ;
	spin_unlock( &bu_suspend_spinlock );

	/* if suspend interrupt is nested, exit interrupt without following codes. */
	if( bu_detect_interrupt_nested ){
		/* printk("tg_bu_det_interrupt: nested interrupt detected, ignored.\n"); */
		return IRQ_HANDLED;
	}

	/* execute the suspend sequence if BU-DET interrupt or ACC OFF(High). */
	/* disable interrupt */
	disable_irq_nosync(gpio_to_irq(TEGRA_GPIO_PR2) );
	disable_irq_nosync(gpio_to_irq(TEGRA_GPIO_PS0) );
	ada_gpio_mask_write(GPIO_INT_ENB(TEGRA_GPIO_PR2), TEGRA_GPIO_PR2, 0);
	ada_gpio_mask_write(GPIO_INT_ENB(TEGRA_GPIO_PS0), TEGRA_GPIO_PS0, 0);

	early_device_resume_flag_set(EARLY_DEVICE_BUDET_INTERRUPTION_MASKED, true);

	/* clear the factor of BU and WAKEUP_REQ interrupt */
	ada_gpio_mask_write(GPIO_INT_STA(TEGRA_GPIO_PR2), TEGRA_GPIO_PR2, 0);
	ada_gpio_mask_write(GPIO_INT_STA(TEGRA_GPIO_PS0), TEGRA_GPIO_PS0, 0);

	/* driver callback */
	early_device_suspend_notify();

	/* set Hi-Z of IO pins */
	pm_set_tristate_pin(TEGRA_TRI_TRISTATE);

/* Design_document_PlanG_0829_FTEN Start */
	/* set TG_WAKEUP2 = LOW */
    gpio_set_value(TG_WAKEUP2, BU_LOW);
/* FUJITSU TEN:2012-12-19 TG_WAKEUP3 support BEGIN */
    gpio_set_value(TG_WAKEUP3, BU_LOW);
/* FUJITSU TEN:2012-12-19 TG_WAKEUP3 support BEGIN */
    
#ifdef DEF_BUDET_KPRINT
    printk("%d %d:TG_WAKEUP1\n", __LINE__, gpio_get_value(TEGRA_GPIO_PI2));
    printk("%d %d:TG_WAKEUP2\n", __LINE__, gpio_get_value(TEGRA_GPIO_PJ2));
#endif
/* Design_document_PlanG_0829_FTEN End */

    /* FUJITSU TEN:2012-12-21 HDMI SUPPORT BEGIN */
    /* SET HDMI EN2 to LOW */
    gpio_set_value(TEGRA_GPIO_PA0, 0);
    //gpio_set_value(TEGRA_GPIO_PR5, 0);
    /* FUJITSU TEN:2012-12-21 HDMI SUPPORT END */

    /* DEV_SW_BUDET(GPIO_PR4) Low */
    gpio_set_value(TEGRA_GPIO_PR4, BU_LOW);

    /* FUJITSU TEN:2012-12-21 HDMI SUPPORT BEGIN */
    /* wait 5ms for HDMI */
    mdelay(5);

    /* set HDMI_EN1 to LOW */
    gpio_set_value(TEGRA_GPIO_PCC3, 0);
    /* FUJITSU TEN:2012-12-21 HDMI SUPPORT END */
#ifdef DEF_BUDET_KPRINT
    gpio_value = gpio_get_value(TEGRA_GPIO_PR4);
    printk("%d:DEV_SW_BUDET(GPIO_PR4)\n", gpio_value);
#endif

	/* work queue start */
	if( wq_st ){
		// start work
		ret = queue_work( wq_st , &workq );
		if( !ret ){
			printk("tg_bu_det_interrupt: queue_work failed.(%d) \n", ret);
		}
	}

#ifdef DEF_BUDET_KPRINT
    printk("End tg_bu_det_interrupt(%d) ret:%d\n",__LINE__, ret);
#endif
	FASTBOOT_LOG_TRACE(0x10);

	return IRQ_HANDLED;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_probe                                                          */
/*  [DESCRIPTION]                                                            */
/*      probe                                                                */
/*  [TYPE]                                                                   */
/*      static int                                                           */
/*  [PARAMETER]                                                              */
/*      struct platform_device *pdev: platform_device structure              */
/*  [RETURN]                                                                 */
/*      0:      Success                                                      */
/*      non-0:  Failure(return the value of request_threaded_irq())          */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static int budet_probe(struct platform_device *pdev)
{
	int ret = 0;
	unsigned long int g_bu_det_sence;
	unsigned long int g_setflag;

#ifdef DEF_BUDET_KPRINT
	printk( "budet_probe(%d)\n", __LINE__ );
#endif
	FASTBOOT_LOG_TRACE(0x20);

/* FUJITSU TEN:2013-10-01 HDMI SUSPEND add Start */
	init_rwsem(&rw_sem);
/* FUJITSU TEN:2013-10-01 HDMI SUSPEND add End */

	/* work queue initialization */
	INIT_WORK(&workq, bu_det_workqueue);

	/* sense for /Bu-DET detection */
	g_bu_det_sence = IRQF_TRIGGER_LOW | IRQF_NO_THREAD | IRQF_ONESHOT;

	/* set BU-DET IRQ */
	ret = request_threaded_irq(
				gpio_to_irq(TEGRA_GPIO_PR2),
				NULL,
				tg_bu_det_interrupt,
				g_bu_det_sence, 
				"budet_dev", 
				NULL
				);
	if (ret) return ret;

#ifdef DEF_WAKEUP_REQ_ENABLE
	/* set the sense for /WAKEUP_REQ(ACC OFF) detection */
	/* to be able to detect OFF as /WAKEUP_REQ is ACC ON(LOW) in start-up */
	g_setflag = IRQF_TRIGGER_HIGH | IRQF_NO_THREAD | IRQF_ONESHOT;

	/* set WAKEUP_REQ IRQ */
	ret = request_threaded_irq(
				gpio_to_irq(TEGRA_GPIO_PS0),
				NULL,
				tg_bu_det_interrupt,
				g_setflag,
				"budet_dev",
				NULL
				);
	if (ret) return ret;
#endif

#ifdef DEF_BUDET_KPRINT
	printk("budet_probe WAKEUP_REQ(%d)\n", gpio_get_value(TEGRA_GPIO_PS0));
#endif

	ret = tegra_pm_irq_set_wake(gpio_to_irq(TEGRA_GPIO_PS0), 1);
#ifdef DEF_BUDET_KPRINT
	printk("budet_probe tegra_pm_irq_set_wake(ret == %d)\n", ret);
#endif

	ret = tegra_pm_irq_set_wake_type(gpio_to_irq(TEGRA_GPIO_PS0), IRQF_TRIGGER_LOW);
#ifdef DEF_BUDET_KPRINT
	printk("budet_probe tegra_pm_irq_set_wake_type(ret == %d)\n", ret);
#endif

	return 0;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_suspend                                                        */
/*  [DESCRIPTION]                                                            */
/*      suspend callback                                                     */
/*  [TYPE]                                                                   */
/*      static int                                                           */
/*  [PARAMETER]                                                              */
/*      struct platform_device *pdev: platform_device structure              */
/*      pm_message_t state: PM state                                         */
/*  [RETURN]                                                                 */
/*      0:  Success                                                          */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static int budet_suspend(struct platform_device *pdev,  pm_message_t state)
{
#ifdef DEF_BUDET_KPRINT
	printk( "budet_suspend(%d)\n", __LINE__ );
#endif
	return 0;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_resume                                                         */
/*  [DESCRIPTION]                                                            */
/*      resume callback                                                      */
/*  [TYPE]                                                                   */
/*      static int                                                           */
/*  [PARAMETER]                                                              */
/*      struct platform_device *pdev: platform_device structure              */
/*  [RETURN]                                                                 */
/*      0:  Success                                                          */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static int budet_resume(struct platform_device *pdev)
{
#ifdef DEF_BUDET_KPRINT
	printk( "budet_resume(%d)\n", __LINE__ );
#endif
#if 0

	/* reregistration of WAKEUP_REQ interrupt */
	enable_irq(gpio_to_irq(TEGRA_GPIO_PS0));

	/* enable the IRQ of BU-DET pins */
	enable_irq(gpio_to_irq(TEGRA_GPIO_PR2));
    early_device_resume_flag_set(EARLY_DEVICE_BUDET_INTERRUPTION_MASKED, false);
#endif
	/* for compatible, This flag currently uses BU-DET interrupt */
	early_device_resume_flag_set(EARLY_DEVICE_BUDET_INTERRUPTION_MASKED, false);

	/* reset suspend oprating flag */
	spin_lock( &bu_suspend_spinlock );
	bu_detect_suspend_oprating = false ;
	spin_unlock( &bu_suspend_spinlock );

#ifdef DEF_BUDET_KPRINT
	printk("budet_resume: resume flag(s) reseted\n");
#endif
	return 0;
}

static struct platform_driver budet_device_driver = {
	.probe  = budet_probe,
	.suspend = budet_suspend,
	.resume = budet_resume,
	.driver = { 
		.name = "budet_dev",
		.owner = THIS_MODULE,
	},
};


/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      bu_detect_unmask_interrupts                                          */
/*  [DESCRIPTION]                                                            */
/*      Unmask interrupts                                                    */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static void bu_detect_unmask_interrupts(void)
{
	unsigned long flags;
	
	printk("bu_detect_unmask_interrupts: Unmask BU-DET interrupts\n");
	FASTBOOT_LOG_TRACE(0x160);

	local_irq_save(flags);

	/* reregistration of WAKEUP_REQ interrupt */
	enable_irq(gpio_to_irq(TEGRA_GPIO_PS0));

	/* enable the IRQ of BU-DET pins */
	enable_irq(gpio_to_irq(TEGRA_GPIO_PR2));

	local_irq_restore(flags);

	FASTBOOT_LOG_TRACE(0x170);
}


/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      bu_notifier_resume_before_tristate                                   */
/*  [DESCRIPTION]                                                            */
/*      Before Tristate operation(No Schduler works)                         */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static void bu_notifier_resume_before_tristate(void)
{
#ifdef DEF_BUDET_KPRINT
	printk("bu_notifier_resume_before_tristate() called\n");
#endif
	FASTBOOT_LOG_TRACE(0x110);
	if( budet_resume_unmask_mode == BUDET_RESUME_UNMASK_MODE_BEFORE_TRISTATE ){
	bu_detect_unmask_interrupts();
	}
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      bu_notifier_resume_after_tristate                                    */
/*  [DESCRIPTION]                                                            */
/*      After Tristate operation(No Schduler works)                          */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static void bu_notifier_resume_after_tristate(void)
{
#ifdef DEF_BUDET_KPRINT
	printk("bu_notifier_resume_after_tristate() called\n");
#endif
	FASTBOOT_LOG_TRACE(0x120);
	if( budet_resume_unmask_mode == BUDET_RESUME_UNMASK_MODE_AFTER_TRISTATE ){
	bu_detect_unmask_interrupts();
	}
}
/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      bu_notifier_resume_after_prepare_to_reinit                           */
/*  [DESCRIPTION]                                                            */
/*      After Prepare to reinit operation(No Schduler works)                 */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static void bu_notifier_resume_after_prepare_to_reinit(void)
{
#ifdef DEF_BUDET_KPRINT
	printk("bu_notifier_resume_after_prepare_to_reinit() called\n");
#endif
	FASTBOOT_LOG_TRACE(0x130);
	if( budet_resume_unmask_mode == BUDET_RESUME_UNMASK_MODE_AFTER_PREPARE_TO_REINIT ){
	bu_detect_unmask_interrupts();
	}
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      bu_notifier_resume_before_reinit                                     */
/*  [DESCRIPTION]                                                            */
/*      Before reinit operation(Schduler works)                              */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static void bu_notifier_resume_before_reinit(void)
{
	FASTBOOT_LOG_TRACE(0x140);
/* FUJITSU TEN:2012-12-25 control TG-WAKEUP2 start */
    gpio_set_value( TEGRA_GPIO_PJ2, 1 );
/* FUJITSU TEN:2012-12-25 control TG-WAKEUP2 end */
#ifdef DEF_BUDET_KPRINT
	printk("bu_notifier_resume_before_reinit() called\n");
#endif
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      bu_notifier_resume_after_reinit                                      */
/*  [DESCRIPTION]                                                            */
/*      After reinit operation                                               */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static void bu_notifier_resume_after_reinit(void)
{
#ifdef DEF_BUDET_KPRINT
	printk("bu_notifier_resume_after_reinit() called\n");
#endif
	FASTBOOT_LOG_TRACE(0x150);
	if( budet_resume_unmask_mode == BUDET_RESUME_UNMASK_MODE_AFTER_REINIT ){
	bu_detect_unmask_interrupts();
	}
	LITES_SET_SUSPEND_STATE(SET_SUSPEND_SQ_E);
}

static struct bu_notifier bu_detect_notifier = {
	.resume_before_tristate = bu_notifier_resume_before_tristate,
	.resume_after_tristate = bu_notifier_resume_after_tristate,
	.resume_after_prepare_to_reinit = bu_notifier_resume_after_prepare_to_reinit,
	.resume_before_reinit = bu_notifier_resume_before_reinit,
	.resume_after_reinit = bu_notifier_resume_after_reinit,
};

/* FUJITSU TEN:2012-12-14 Plan-G Start */
#if	BUDET_SYSFS_DEVICE_CREATE
/* FUJITSU TEN:2013-10-01 HDMI SUSPEND del Start */
//static struct kobject *budet_sysfs_root_kobject = NULL;
/* FUJITSU TEN:2013-10-01 HDMI SUSPEND del End */

#if	BUDET_SYSFS_DEVICE_MODE_CREATE
/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_sysfs_mode_show                                                */
/*  [DESCRIPTION]                                                            */
/*      sysfs用のモード表示                                                  */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj          : kobject                              */
/*      struct kobj_attribute *attr   : kobjectの属性                        */
/*      char *buf                     : 出力用の文字バッファ                 */
/*  [RETURN]                                                                 */
/*      出力文字数                                                           */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static ssize_t budet_sysfs_mode_show( struct kobject *kobj, struct kobj_attribute *attr, char *buf )
{
	return sprintf( buf , "%d" , budet_suspend_config );
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_sysfs_mode_store                                               */
/*  [DESCRIPTION]                                                            */
/*      sysfs用のモード設定                                                  */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj          : kobject                              */
/*      struct kobj_attribute *attr   : kobjectの属性                        */
/*      char *buf                     : 入力文字バッファ                     */
/*      size_t count                  : 入力文字数                           */
/*  [RETURN]                                                                 */
/*      処理文字数                                                           */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static ssize_t budet_sysfs_mode_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int	val;
	
	sscanf(buf, "%du", &val);

	budet_suspend_config = val;

	return count;
}

/* mode用のsysfsファイル属性 */
static struct kobj_attribute budet_sysfs_mode_attribute = \
	__ATTR(BUDET_SYSFS_MODE_DEVICE_NAME, 0600, budet_sysfs_mode_show, budet_sysfs_mode_store);

#endif	/* BUDET_SYSFS_DEVICE_MODE_CREATE */

#if BUDET_SYSFS_DEVICE_RUN_CREATE
/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_sysfs_run_store                                               */
/*  [DESCRIPTION]                                                            */
/*      sysfs用のモード設定                                                  */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj          : kobject                              */
/*      struct kobj_attribute *attr   : kobjectの属性                        */
/*      char *buf                     : 入力文字バッファ                     */
/*      size_t count                  : 入力文字数                           */
/*  [RETURN]                                                                 */
/*      処理文字数                                                           */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static ssize_t budet_sysfs_run_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	/* unsigned int	val; */

	/* Work Queue 開始 */
	schedule_work(&workq);

	/* sscanf(buf, "%du", &val); */

	return count;
}

/* run用のsysfsファイル属性 */
static struct kobj_attribute budet_sysfs_run_attribute = \
	__ATTR(BUDET_SYSFS_RUN_DEVICE_NAME, 0600, NULL, budet_sysfs_run_store);
#endif /* BUDET_SYSFS_DEVICE_RUN_CREATE */

#if	BUDET_SYSFS_DEVICE_UNMASK_CREATE
/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_sysfs_unmask_show                                                */
/*  [DESCRIPTION]                                                            */
/*      sysfs用のモード表示                                                  */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj          : kobject                              */
/*      struct kobj_attribute *attr   : kobjectの属性                        */
/*      char *buf                     : 出力用の文字バッファ                 */
/*  [RETURN]                                                                 */
/*      出力文字数                                                           */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static ssize_t budet_sysfs_unmask_show( struct kobject *kobj, struct kobj_attribute *attr, char *buf )
{
	return sprintf( buf , "%d" , budet_resume_unmask_mode );
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_sysfs_unmask_store                                               */
/*  [DESCRIPTION]                                                            */
/*      sysfs用のモード設定                                                  */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj          : kobject                              */
/*      struct kobj_attribute *attr   : kobjectの属性                        */
/*      char *buf                     : 入力文字バッファ                     */
/*      size_t count                  : 入力文字数                           */
/*  [RETURN]                                                                 */
/*      処理文字数                                                           */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static ssize_t budet_sysfs_unmask_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int	val;
	
	sscanf(buf, "%du", &val);

	if( val <= BUDET_RESUME_UNMASK_MODE_MAX ){
		budet_resume_unmask_mode = val;
	}

	return count;
}

/* unmask用のsysfsファイル属性 */
static struct kobj_attribute budet_sysfs_unmask_attribute = \
	__ATTR(BUDET_SYSFS_UNMASK_DEVICE_NAME, 0600, budet_sysfs_unmask_show, budet_sysfs_unmask_store);
#endif	/* BUDET_SYSFS_DEVICE_UNMASK_CREATE */

#if	BUDET_SYSFS_DEVICE_ASYNC_TASK_FREEZE_CREATE
/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_sysfs_async_task_freeze_show                                                */
/*  [DESCRIPTION]                                                            */
/*      sysfs用のモード表示                                                  */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj          : kobject                              */
/*      struct kobj_attribute *attr   : kobjectの属性                        */
/*      char *buf                     : 出力用の文字バッファ                 */
/*  [RETURN]                                                                 */
/*      出力文字数                                                           */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static ssize_t budet_sysfs_async_task_freeze_show( struct kobject *kobj, struct kobj_attribute *attr, char *buf )
{
	return sprintf( buf , "%d" , budet_suspend_async_taskfreeze_timeout_time );
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_sysfs_async_task_freeze_store                                               */
/*  [DESCRIPTION]                                                            */
/*      sysfs用のモード設定                                                  */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj          : kobject                              */
/*      struct kobj_attribute *attr   : kobjectの属性                        */
/*      char *buf                     : 入力文字バッファ                     */
/*      size_t count                  : 入力文字数                           */
/*  [RETURN]                                                                 */
/*      処理文字数                                                           */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static ssize_t budet_sysfs_async_task_freeze_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int	val;
	
	sscanf(buf, "%u", &val);

	budet_suspend_async_taskfreeze_timeout_time = val;

	return count;
}

/* unmask用のsysfsファイル属性 */
static struct kobj_attribute budet_sysfs_async_task_freeze_attribute = \
	__ATTR(BUDET_SYSFS_ASYNC_TASK_FREEZE_DEVICE_NAME, 0600, budet_sysfs_async_task_freeze_show, budet_sysfs_async_task_freeze_store);

#endif	/* BUDET_SYSFS_DEVICE_ASYNC_TASK_FREEZE_CREATE */

#if	BUDET_SYSFS_DEVICE_FORCE_TASK_FREEZE_CREATE
/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_sysfs_force_task_freeze_show                                                */
/*  [DESCRIPTION]                                                            */
/*      sysfs用のモード表示                                                  */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj          : kobject                              */
/*      struct kobj_attribute *attr   : kobjectの属性                        */
/*      char *buf                     : 出力用の文字バッファ                 */
/*  [RETURN]                                                                 */
/*      出力文字数                                                           */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static ssize_t budet_sysfs_force_task_freeze_budet_show( struct kobject *kobj, struct kobj_attribute *attr, char *buf )
{
	return sprintf( buf , "%d" , budet_suspend_force_taskfreeze_budet_timeout_time );
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_sysfs_force_task_freeze_store                                               */
/*  [DESCRIPTION]                                                            */
/*      sysfs用のモード設定                                                  */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj          : kobject                              */
/*      struct kobj_attribute *attr   : kobjectの属性                        */
/*      char *buf                     : 入力文字バッファ                     */
/*      size_t count                  : 入力文字数                           */
/*  [RETURN]                                                                 */
/*      処理文字数                                                           */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static ssize_t budet_sysfs_force_task_freeze_budet_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int	val;
	
	sscanf(buf, "%u", &val);

	budet_suspend_force_taskfreeze_budet_timeout_time = val;

	return count;
}

/* unmask用のsysfsファイル属性 */
static struct kobj_attribute budet_sysfs_force_task_freeze_budet_attribute = \
	__ATTR(BUDET_SYSFS_FORCE_TASK_FREEZE_BUDET_DEVICE_NAME, 0600, budet_sysfs_force_task_freeze_budet_show, budet_sysfs_force_task_freeze_budet_store);
/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_sysfs_force_task_freeze_show                                                */
/*  [DESCRIPTION]                                                            */
/*      sysfs用のモード表示                                                  */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj          : kobject                              */
/*      struct kobj_attribute *attr   : kobjectの属性                        */
/*      char *buf                     : 出力用の文字バッファ                 */
/*  [RETURN]                                                                 */
/*      出力文字数                                                           */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static ssize_t budet_sysfs_force_task_freeze_accoff_show( struct kobject *kobj, struct kobj_attribute *attr, char *buf )
{
	return sprintf( buf , "%d" , budet_suspend_force_taskfreeze_accoff_timeout_time );
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_sysfs_force_task_freeze_store                                               */
/*  [DESCRIPTION]                                                            */
/*      sysfs用のモード設定                                                  */
/*  [TYPE]                                                                   */
/*      int                                                                  */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj          : kobject                              */
/*      struct kobj_attribute *attr   : kobjectの属性                        */
/*      char *buf                     : 入力文字バッファ                     */
/*      size_t count                  : 入力文字数                           */
/*  [RETURN]                                                                 */
/*      処理文字数                                                           */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static ssize_t budet_sysfs_force_task_freeze_accoff_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned int	val;
	
	sscanf(buf, "%u", &val);

	budet_suspend_force_taskfreeze_accoff_timeout_time = val;

	return count;
}

/* unmask用のsysfsファイル属性 */
static struct kobj_attribute budet_sysfs_force_task_freeze_accoff_attribute = \
	__ATTR(BUDET_SYSFS_FORCE_TASK_FREEZE_ACCOFF_DEVICE_NAME, 0600, budet_sysfs_force_task_freeze_accoff_show, budet_sysfs_force_task_freeze_accoff_store);
#endif	/* BUDET_SYSFS_DEVICE_FORCE_TASK_FREEZE_CREATE */

/* sysfsファイル属性リスト */
static struct attribute *attrs[] = {
/* FUJITSU TEN:2013-10-01 HDMI SUSPEND add Start */
       &budet_sysfs_notify_attribute.attr,
       &budet_sysfs_complete_attribute.attr,
/* FUJITSU TEN:2013-10-01 HDMI SUSPEND add End */
#if	BUDET_SYSFS_DEVICE_MODE_CREATE
       &budet_sysfs_mode_attribute.attr,
#endif	/* BUDET_SYSFS_DEVICE_MODE_CREATE */
#if BUDET_SYSFS_DEVICE_RUN_CREATE
       &budet_sysfs_run_attribute.attr,
#endif	/* BUDET_SYSFS_DEVICE_RUN_CREATE */
#if	BUDET_SYSFS_DEVICE_UNMASK_CREATE
       &budet_sysfs_unmask_attribute.attr,
#endif	/* BUDET_SYSFS_DEVICE_UNMASK_CREATE */
#if	BUDET_SYSFS_DEVICE_ASYNC_TASK_FREEZE_CREATE
       &budet_sysfs_async_task_freeze_attribute.attr,
#endif	/* BUDET_SYSFS_DEVICE_ASYNC_TASK_FREEZE_CREATE */
#if	BUDET_SYSFS_DEVICE_FORCE_TASK_FREEZE_CREATE
       &budet_sysfs_force_task_freeze_budet_attribute.attr,
       &budet_sysfs_force_task_freeze_accoff_attribute.attr,
#endif	/* BUDET_SYSFS_DEVICE_FORCE_TASK_FREEZE_CREATE */
	NULL,   /* need to NULL terminate the list of attributes */
};

/* sysfsファイル属性グループ */
static struct attribute_group attr_group = {
       .attrs = attrs,
};

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_sysfs_create_device                                            */
/*  [DESCRIPTION]                                                            */
/*      sysfsデバイス作成処理                                                */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      int リターンコード(errno)                                            */
/*  [NOTE]                                                                   */
/*      現在、使用予定なし                                                   */
/*****************************************************************************/
static int budet_sysfs_create_device(void)
{
	int ret = 0;

	if( budet_sysfs_root_kobject != NULL ){
		/* 再入しているのでエラーとする */
		return -ENOMEM;
	}
	
	budet_sysfs_root_kobject = kobject_create_and_add(BUDET_SYSFS_ROOT_NAME, kernel_kobj);
	if( budet_sysfs_root_kobject == NULL ){
		/* kobject作成失敗 */
		return -ENOMEM;
	}

	ret = sysfs_create_group(budet_sysfs_root_kobject, &attr_group);
	if( ret ){
		/* Eror occured, exit with kobject delete. */
		kobject_put( budet_sysfs_root_kobject );
	}
	
	return ret;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_sysfs_remove_device                                            */
/*  [DESCRIPTION]                                                            */
/*      sysfsデバイス作成処理                                                */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      特になし                                                             */
/*****************************************************************************/
static void budet_sysfs_remove_device(void)
{
	if( budet_sysfs_root_kobject != NULL ){
		kobject_put( budet_sysfs_root_kobject );
		budet_sysfs_root_kobject = NULL;
	}
}
#endif	/* BUDET_SYSFS_DEVICE_CREATE */
/* FUJITSU TEN:2012-12-14 Plan-G End */

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_init                                                           */
/*  [DESCRIPTION]                                                            */
/*      Initialization                                                       */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static int __init budet_init(void)
{
	/* FUJITSU TEN:2012-12-14 Plan-G Start */
#if	BUDET_SYSFS_DEVICE_CREATE
	int ret = 0;
#endif	/* BUDET_SYSFS_DEVICE_CREATE */
	
#if	BUDET_SYSFS_DEVICE_CREATE
	ret = budet_sysfs_create_device();
	if( ret != 0 ){
		printk("budet_init: budet_sysfs_create_device failed(%d)\n", ret);
		return ret;
	}
#endif	/* BUDET_SYSFS_DEVICE_CREATE */
	/* FUJITSU TEN:2012-12-14 Plan-G End */

	/* Workqueue initialize */
	wq_st = alloc_workqueue("budet_work" , WQ_MEM_RECLAIM | WQ_HIGHPRI | WQ_CPU_INTENSIVE, 2);
	//wq_st = alloc_workqueue("budet_work" , WQ_HIGHPRI | WQ_CPU_INTENSIVE, 2);

	if( !wq_st ){
		printk("budet_init: can't create budet_work\n");
		return -ENOMEM;
	}

	bu_notifier_register(&bu_detect_notifier);
	return platform_driver_register(&budet_device_driver);
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      budet_exit                                                           */
/*  [DESCRIPTION]                                                            */
/*      Termination                                                          */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static void __exit budet_exit(void)
{
	/* Workqueue destroy */
	if( wq_st ){
		destroy_workqueue(wq_st);
	}

	free_irq(gpio_to_irq(TEGRA_GPIO_PR2), NULL);
	free_irq(gpio_to_irq(TEGRA_GPIO_PS0), NULL);
	platform_driver_unregister(&budet_device_driver);
	bu_notifier_unregister(&bu_detect_notifier);

	/* FUJITSU TEN:2012-12-14 Plan-G Start */
#if	BUDET_SYSFS_DEVICE_CREATE
	budet_sysfs_remove_device();
#endif	/* BUDET_SYSFS_DEVICE_CREATE */
	/* FUJITSU TEN:2012-12-14 Plan-G End */
}

module_init(budet_init);
module_exit(budet_exit);

