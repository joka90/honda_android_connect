/**
 * Copyright (c) 2013 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
/**
 * @file trace_fault_info.c
 *
 * @brief ファイルオペレーションの定義、初期化/終了関数の定義
 */
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include "../../../arch/arm/mach-tegra/gpio-names.h"
#include "../../../arch/arm/mach-tegra/pm-irq.h"
#include "../../../arch/arm/mach-tegra/board-e1853.h"

#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/lites_trace.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "region.h"
#include "ioctl.h"
#include "buffer.h"
#include "mgmt_info.h"
#include "trace_write.h"
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/lites_qb.h>
#include <linux/kmod.h>
#include "kernel_panic_info.h"
#include "tsc.h"

#if 0
int    faultProcStatus;
int    faultType;
#endif

void lites_fault_info_userhelper ( void )
{
    int rtn;
    char *argv[] = { "/system/xbin/drclogbup", "", NULL };
    char *envp[] = { "HOME=/",
                     "TERM=linux",
                     "PATH=/sbin:/usr/sbin/:/bin:/usr/bin",
                     NULL };

    struct subprocess_info *info;

    info = call_usermodehelper_setup ( argv[0], argv, envp, GFP_ATOMIC );
    if ( info == NULL ) {
        printk ( KERN_INFO "call_usermodehelper_setup return address NULL" );
        return;
    }

    // drclogbup return value : IPAS_OK(0),IPAS_NG(-1)
    printk ( KERN_INFO "##### call_usermodehelper_exec start #####");
    rtn = call_usermodehelper_exec ( info, UMH_WAIT_PROC );
    printk ( KERN_INFO "##### call_usermodehelper_exec end #####");
    if ( rtn != 0 ) {
        printk ( KERN_INFO "Error call_usermodehelper_exec return = %d", rtn );
    }

    return;
}

#if 0
static unsigned char lites_conv_num_to_hexchar ( unsigned char num )
{
    if ( num < 10 ) {
        return ( num + '0' );
    }
    else {
        return ( num + 'a' - 10 );
    }
}

static void lites_conv_to_hex_timecount ( unsigned char *inTimeCount, unsigned char *outTimeCount )
{
    unsigned char wk1, wk2;
    int           i, j;

    for ( i = 0, j = 0; i < sizeof ( u64 ); i++,j++ ) {
        wk1 = ( inTimeCount[i] & 0xf0 ) >> 4;
        outTimeCount[j] = lites_conv_num_to_hexchar ( wk1 ); 

        j++;

        wk2 = ( inTimeCount[i] & 0x0f );
        outTimeCount[j] = lites_conv_num_to_hexchar ( wk2 ); 
    }
}
#endif

/**
 * @brief 割り込みハンドラ処理関数
 *        割り込み発生時に呼び出される
 *
 * @retval 0  正常終了
 */
static irqreturn_t tg_lites_fault_info_shReset_interrupt ( int irq, void *dev_id )
{
    int           rtn = 0;
    u64           tsc;

    disable_irq_nosync ( gpio_to_irq ( TEGRA_GPIO_PR3 ) );

    lites_get_tsc ( tsc );

    // SHからのリセット受付
    printk ( KERN_INFO "##### SH RESET INTERRUPT ##### : jiffer time counter = %llu (0x%016llx)\n", tsc, tsc );

/*
 * MIC様提供ライブラリ結合APIを呼び出す
 *
 */
    lites_fault_info_userhelper ( );

    return rtn;
}

/**
 * @brief 割り込みハンドラ登録処理関数
 *        lites_init関数より呼び出される
 *
 * @retval 0  正常終了
 */
int lites_fault_info_regist_sh_reset_interrupt ( struct platform_device *pdev )
{
    unsigned int irq;
    unsigned long int interruptFlag = 0;

    int rtn = 0;

    interruptFlag = IRQF_TRIGGER_LOW | IRQF_ONESHOT;
    irq = gpio_to_irq( TEGRA_GPIO_PR3 );
    rtn = request_threaded_irq ( irq,
                                 NULL,
                                 tg_lites_fault_info_shReset_interrupt,
                                 interruptFlag,
                                 "shrstintr_dev",
                                 NULL );

    if ( rtn ) {
        printk ( KERN_INFO "request_threaded_irq:TEGRA_GPIO_PR3 ERROR %d", rtn );
    }
    return rtn;
}

static int lites_kernel_panic_info_save( struct notifier_block *nv, unsigned long val, void *v )
{
    int budet_status = gpio_get_value(TEGRA_GPIO_PR2);
    if (budet_status == 0) {
      /* budet割り込み発生時はログ保存抑制 */
      printk ( KERN_INFO "stop log keep(catch budet)!" );
      return 0;
    }

    lites_fault_info_userhelper ( );
    return 0;
}

static struct notifier_block lites_kernel_panic_notifier = {
        .notifier_call = lites_kernel_panic_info_save,
};

static int lites_kernel_panic_info_init(void)
{
    atomic_notifier_chain_register(&panic_notifier_list, &lites_kernel_panic_notifier);

    return 0;
}
pure_initcall ( lites_kernel_panic_info_init );

#if 0
static void lites_fault_message_write ( int status )
{
    u64           tsc;


    lites_get_tsc ( tsc );

    if ( status & LITES_FLAG_FAULT_APP_PROC_START ) {
        printk ( KERN_INFO  "##### APPLICATION LAYER FAULT LOG START ##### jiffies time count : %llu (0x%016llx)\n", tsc, tsc );
    }
    else if ( status & LITES_FLAG_FAULT_APP_PROC_END ) {
        printk ( KERN_INFO  "##### APPLICATION LAYER FAULT LOG END ##### jiffies time count : %llu (0x%016llx)\n", tsc, tsc );
    }
    else if ( status & LITES_FLAG_FAULT_KERNEL_PROC_START ) {
        printk ( KERN_INFO  "##### KERNEL LAYER FAULT LOG START ##### jiffies time count : %llu (0x%016llx)\n", tsc, tsc );
    }
    else if ( status & LITES_FLAG_FAULT_KERNEL_PROC_END ) {
        printk ( KERN_INFO  "##### KERNEL LAYER FAULT LOG END ##### jiffies time count : %llu (0x%016llx)\n", tsc, tsc ); 
    }

    return;
}

void lites_fault_info_set_fault_proc_status ( int status )
{

    faultProcStatus |= status;

    lites_fault_message_write ( status );

    return;
}
#endif
