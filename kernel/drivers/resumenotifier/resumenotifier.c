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
 *
 * drivers/resumenotifier/resumenotifier.c
 *
 */

#include <linux/types.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/bu_notifier.h>
#include <linux/lites_trace.h>
// FUJITSU TEN:2014-01-08 #55575 start
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/init.h>
// FUJITSU TEN:2014-01-08 #55575 end

#include <asm/uaccess.h>

//#define DEBUG
#ifdef DEBUG
#define DEBUGK(...) printk(__VA_ARGS__)
#else
#define DEBUGK(...)
#endif

#define RESUMENOTIFIER_DRIVER_MODULE_ID (LITES_KNL_LAYER+89)
#define FORMAT_ID 7

#define DRVREC_TRACE( level_, trace_id_, errcode_, ... ) {\
    struct lites_trace_format data = { 0 }; \
    struct lites_trace_header header = { 0 }; \
    enum LOG_LEVEL level = level_; \
    char buf[36] = { 0 };\
    int ierr;\
    char *tmp;\
    \
    header.level = level; \
    header.trace_no = RESUMENOTIFIER_DRIVER_MODULE_ID; \
    header.trace_id = trace_id_; \
    header.format_id = FORMAT_ID; \
    data.trc_header = &header; \
    \
    ierr = errcode_; \
    tmp = kmalloc(512, GFP_TEMPORARY);\
    if(tmp) { \
        snprintf(tmp, 512, __VA_ARGS__);\
        if(strlen(tmp) > (36-sizeof(ierr))) {\
            memcpy(buf, tmp+(strlen(tmp)-(36-sizeof(ierr))), 36-sizeof(ierr)); \
        }\
        else {\
            strcpy(buf, tmp); \
        }\
        kfree(tmp);\
    }\
    else {\
        snprintf(buf, 36-sizeof(ierr), __VA_ARGS__);\
    }\
    memcpy(buf+(36-sizeof(ierr)), &ierr, sizeof(ierr)); \
    data.buf = buf; \
    data.count = 36; \
    \
    lites_trace_write( REGION_TRACE_BATTELY, &data ); \
    }

#define DRVREC_ERRORTRACE5( errcode0_,errcode1_,errcode2_,errcode3_,errcode4_) {\
    struct lites_trace_format data = { 0 }; \
    struct lites_trace_header header = { 0 }; \
    char buf[20] = { 0 };\
    \
    header.level = 1; \
    header.trace_no = RESUMENOTIFIER_DRIVER_MODULE_ID; \
    header.trace_id = 0; \
    header.format_id = 1; \
    data.trc_header = &header; \
    \
    int* ptr = (int*)buf;\
    *ptr++ = (int) errcode0_;\
    *ptr++ = (int) errcode1_;\
    *ptr++ = (int) errcode2_;\
    *ptr++ = (int) errcode3_;\
    *ptr++ = (int) errcode4_;\
    \
    data.buf = buf; \
    data.count = 20; \
    \
    lites_trace_write( REGION_TRACE_ERROR, &data ); \
    }

#define DRVREC_ERRORTRACE4( errcode0_,errcode1_,errcode2_,errcode3_) DRVREC_ERRORTRACE5( errcode0_,errcode1_,errcode2_,errcode3_,0)
#define DRVREC_ERRORTRACE3( errcode0_,errcode1_,errcode2_)           DRVREC_ERRORTRACE4( errcode0_,errcode1_,errcode2_,0)
#define DRVREC_ERRORTRACE2( errcode0_,errcode1_)                     DRVREC_ERRORTRACE3( errcode0_,errcode1_,0)
#define DRVREC_ERRORTRACE1( errcode0_)                               DRVREC_ERRORTRACE2( errcode0_,0)
#define DRVREC_ERRORTRACE( )                                         DRVREC_ERRORTRACE1( 0)
    
#define DRVREC_ERROR(trace_id_, errcode_, ... ) DRVREC_TRACE( 0, trace_id_, errcode_, __VA_ARGS__)
#define DRVREC_INFO(trace_id_, errcode_, ... )  DRVREC_TRACE( 3, trace_id_, errcode_, __VA_ARGS__)

#define TRACE_READ_WAIT_EVENT_INTERRUPTIBLE_ERROR 0
#define TRACE_READ_ID_INCREMENT 1
#define TRACE_AFTER_REINIT 2
#define TRACE_READ_COPY_TO_USER_ERROR 3
#define TRACE_INIT_PLATFORM_DRIVER_REGISTER_ERROR 4
#define TRACE_PROVE_MISC_REGISTER_ERROR 5
#define TRACE_CLEAR_RESUMEFLAG_STATE 6
#define TRACE_SET_RESUME_STATUS 7
#define TRACE_INIT_BU_NOTIFIER_REGISTER 8
#define TRACE_PROBE_INIT_WAITQUEUE_HEAD 9
/*
 * Function Prototypes
 */
static int resume_notifier_init(void);
static void resume_notifier_exit(void);
static int resume_notifier_probe(struct platform_device *);
static int resume_notifier_remove(struct platform_device *);
static int resume_notifier_suspend(struct platform_device *, pm_message_t state);
static int resume_notifier_resume(struct platform_device *);
// FUJITSU TEN:2014-01-08 #55575 start
static ssize_t resume_notifier_sysfs_preparing_read(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static int resume_notifier_sysfs_init(void);
static void resume_notifier_sysfs_exit(void);
// FUJITSU TEN:2014-01-08 #55575 end

/*
 * Local Definitions and Variables
 */
static spinlock_t lock;

static int initialized = false;

#define GET_RESUME_ID 0
#define CLEAR_RESUME_FLAG 1
#define IS_ALL_RESUMEFLAGS_CLEARED 2
#define SET_RESUME_STATUS 3
#define GET_RESUME_STATUS 4
#define CLEANUP 5

/* resumeId counter */
static int resume_notifier_id = 0;

/* ResumeFlag格納配列 */
#define MAXRESUMEID 64
static bool resume_flags[MAXRESUMEID];

/* resume状態フラグ */
#define EXCEPT_RESUME 0
#define NOW_RESUME 1
static int resume_status = EXCEPT_RESUME;

/* condition to finish waiting */
static int resume_state = 0;

/* wait queue */
static wait_queue_head_t read_q;

// FUJITSU TEN:2014-01-08 #55575 start
/* sysfs flag of between "ACC-ON" and "resumeId incrementing" */
static int sysfs_between_accon_and_resumeid_incrementing = 0;

/* sysfs objects */
static struct kobject *resume_notifier_sysfs_kobj;
static struct kobj_attribute resume_notifier_sysfs_preparing_attr = __ATTR(preparing, 0444, resume_notifier_sysfs_preparing_read, NULL);

static struct attribute *resume_notifier_sysfs_attrs[] = {
    &resume_notifier_sysfs_preparing_attr.attr,
    NULL,
};

static struct attribute_group resume_notifier_sysfs_attr_group = {
    .attrs = resume_notifier_sysfs_attrs,
};
// FUJITSU TEN:2014-01-08 #55575 end

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      resume_notifier_read                                                 */
/*  [DESCRIPTION]                                                            */
/*      read from a file                                                     */
/*  [TYPE]                                                                   */
/*      static ssize_t                                                       */
/*  [PARAMETER]                                                              */
/*      struct file *filp:      file information                             */
/*      const charioctl *readbuf:   buffer to read                               */
/*      size_t count:           size to read                                 */
/*      loff_t *pos:            offset                                       */
/*  [RETURN]                                                                 */
/*      non -1: success（the number of bytes read is returned）              */
/*      -1:     failure（errno is set）                                      */
/*  [NOTE]                                                                   */
/*      This function informs user space of resume.                          */
/*****************************************************************************/
static ssize_t resume_notifier_read(struct file *filp, char *readbuf,
                                                    size_t count, loff_t *pos)
{ 
    int ret = 0, i = 0;
    unsigned long eflags; 

    if(!initialized) {
        spin_lock_init(&lock);
    }

    /* wait for resume */
    ret = wait_event_interruptible(read_q, (resume_state != 0));

    if (ret) {
        /* interrupted by a signal */
        printk(KERN_DEBUG "wait_event_interruptible: ret(%d)\n", ret);
        DRVREC_ERRORTRACE2(TRACE_READ_WAIT_EVENT_INTERRUPTIBLE_ERROR, ret);
        return -ERESTARTSYS;
    }

    resume_notifier_id++;
    DEBUGK(KERN_DEBUG "notify resume to user space: resumeID(%d)\n",
                                                        resume_notifier_id);
    DRVREC_INFO(TRACE_READ_ID_INCREMENT, resume_notifier_id, "resume_notifier_id++");
    
    /* inform user space of resume, since the condition evaluated to be true */
    if (ret = copy_to_user(readbuf, &resume_notifier_id, sizeof(resume_notifier_id))) {
        printk(KERN_ERR "resumenotifier: copy_to_user failed.\n");
        DRVREC_ERRORTRACE2(TRACE_READ_COPY_TO_USER_ERROR, ret);
        return -EFAULT;
    }
    resume_state = 0;
    ret = count;

    spin_lock_irqsave(&lock, eflags);
    for(i = 0; i < MAXRESUMEID; i++){ 
        resume_flags[i] = true;
    }
    spin_unlock_irqrestore(&lock, eflags);

// FUJITSU TEN:2014-01-08 #55575 start
    sysfs_between_accon_and_resumeid_incrementing = 0;
// FUJITSU TEN:2014-01-08 #55575 end

    return ret;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      resume_notifier_close                                                */
/*  [DESCRIPTION]                                                            */
/*      close a file                                                         */
/*  [TYPE]                                                                   */
/*      static int                                                           */
/*  [PARAMETER]                                                              */
/*      struct inode *inode:        inode information                        */
/*      struct file *filp:          file information                         */
/*  [RETURN]                                                                 */
/*      0:  success                                                          */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static int resume_notifier_close(struct inode *inode, struct file *filp)
{
    return 0;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      resume_notifier_open                                                 */
/*  [DESCRIPTION]                                                            */
/*      open a file                                                          */
/*  [TYPE]                                                                   */
/*      static int                                                           */
/*  [PARAMETER]                                                              */
/*      struct inode *inode:        inode information                        */
/*      struct file *filp:          file information                         */
/*  [RETURN]                                                                 */
/*      0:  success                                                          */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static int resume_notifier_open(struct inode *inode, struct file *filp)
{
    DEBUGK(KERN_DEBUG "resume_notifier_open\n");

    return 0;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      get_resume_notifier_id                                               */
/*  [DESCRIPTION]                                                            */
/*      最新のResumeId取得処理                                               */
/*  [TYPE]                                                                   */
/*      static int                                                           */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      0以上：成功                                                          */
/*      0未満：失敗                                                          */
/*  [NOTE]                                                                   */
/*      なし                                                                 */
/*****************************************************************************/
static long get_resume_notifier_id(void)
{
    return resume_notifier_id;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      clear_resumeflag_state                                               */
/*  [DESCRIPTION]                                                            */
/*      Resumeフラグを落とす処理                                             */
/*  [TYPE]                                                                   */
/*      static int                                                           */
/*  [PARAMETER]                                                              */
/*      int resume_id:    フラグを落としたいResumeId                         */
/*  [RETURN]                                                                 */
/*      0：成功                                                              */
/*      0以外：失敗                                                          */
/*  [NOTE]                                                                   */
/*      なし                                                                 */
/*****************************************************************************/
static long clear_resumeflag_state(int resume_id)
{
    long ret = 0;
    DRVREC_INFO(TRACE_CLEAR_RESUMEFLAG_STATE, resume_id, "clear_resumeflag_state");
    if(0 <= resume_id && resume_id < MAXRESUMEID){
        resume_flags[resume_id] = false;
    }else{
        ret = -EINVAL;
    }
    return ret;
}

static int is_all_resumeflags_cleared(int range)
{
    int i=0;
    if(range > MAXRESUMEID) {
        return -EINVAL;
    }

    for(i=0; i<range; i++) {
        if(resume_flags[i] == true) {
            return 0;
        }
    }
    return 1;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      set_resume_status                                               */
/*  [DESCRIPTION]                                                            */
/*      resume状態フラグを更新する                                               */
/*  [TYPE]                                                                   */
/*      static void                                                          */
/*  [PARAMETER]                                                              */
/*      int status : resume状態フラグ                                               */
/*  [RETURN]                                                                 */
/*       0 : 成功                                                                        */
/*  [NOTE]                                                                   */
/*      なし                                                                 */
/*****************************************************************************/
static int set_resume_status(int status)
{
    DRVREC_INFO(TRACE_SET_RESUME_STATUS, status, "set_resume_status");
    resume_status = status;
    return 0;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      get_resume_status                                               */
/*  [DESCRIPTION]                                                            */
/*      最新のresume状態フラグを取得する                                               */
/*  [TYPE]                                                                   */
/*      static void                                                          */
/*  [PARAMETER]                                                              */
/*      void                                               */
/*  [RETURN]                                                                 */
/*      0  : resume処理以外                                                         */
/*      1  : resume処理中                                                       */
/*  [NOTE]                                                                   */
/*      なし                                                                 */
/*****************************************************************************/
static int get_resume_status(void)
{
    return resume_status;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      cleanup                                                              */
/*  [DESCRIPTION]                                                            */
/*      内部変数を初期化する                                                 */
/*  [TYPE]                                                                   */
/*      static void                                                          */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      0  : 必ず0を返す                                                     */
/*  [NOTE]                                                                   */
/*      なし                                                                 */
/*****************************************************************************/
static int cleanup(void)
{
    resume_notifier_id = 0;
    resume_status = EXCEPT_RESUME;
    resume_state = 0; 
    memset(resume_flags, 0, sizeof(resume_flags) );
// FUJITSU TEN:2014-01-08 #55575 start
    sysfs_between_accon_and_resumeid_incrementing = 0;
// FUJITSU TEN:2014-01-08 #55575 end
    
    return 0;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      resume_notifier_ioctl                                                */
/*  [DESCRIPTION]                                                            */
/*      ioctl処理                                                            */
/*  [TYPE]                                                                   */
/*      static long                                                          */
/*  [PARAMETER]                                                              */
/*      struct file *file:    ファイルディスクリプタ                         */
/*      unsigned int cmd:     コマンド種別                                   */
/*      unsigned long arg:    nativeメソッドに渡された引数                   */
/*  [RETURN]                                                                 */
/*      0以上：成功                                                          */
/*      0未満：失敗                                                          */
/*  [NOTE]                                                                   */
/*      なし                                                                 */
/*****************************************************************************/
static long resume_notifier_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret = 0;
    DEBUGK("resume_notifier_ioctl(%d) cmd(%d)\n", __LINE__, cmd);
    switch(cmd){
        case GET_RESUME_ID:
            ret = get_resume_notifier_id();
            break;
        case IS_ALL_RESUMEFLAGS_CLEARED:
            ret = is_all_resumeflags_cleared(arg);
            break;
        case CLEAR_RESUME_FLAG:
            ret = clear_resumeflag_state(arg);
            break;
        case SET_RESUME_STATUS:
            ret = set_resume_status(arg);
            break;
        case GET_RESUME_STATUS:
            ret = get_resume_status();
            break;
        case CLEANUP:
            ret = cleanup();
            break;
        default:
            ret = -EINVAL;
            break;
    }
     return ret;
}

struct file_operations resume_notifier_fops = {
    .owner = THIS_MODULE,
    .open = resume_notifier_open,
    .release = resume_notifier_close,
    .read = resume_notifier_read,
    .unlocked_ioctl = resume_notifier_ioctl,
};

static struct miscdevice resume_notifier_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name  = "resumenotifier",
    .fops  = &resume_notifier_fops,
};

static struct platform_driver resume_notifier_pfdriver = {
    .probe = resume_notifier_probe,
    .remove = resume_notifier_remove,
    .resume = resume_notifier_resume,
    .suspend = resume_notifier_suspend,
    .driver = {
        .name = "resumenotifier",
        .owner = THIS_MODULE,
    },
};

#ifdef CONFIG_SUSPEND
// FUJITSU TEN:2014-01-08 #55575 start
/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      resume_notifier_bu_notify_resume_before_tristate                     */
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
static void resume_notifier_bu_notify_resume_before_tristate(void)
{
#ifdef DEF_BUDET_KPRINT
	printk("resume_notifier_bu_notify_resume_before_tristate() called\n");
#endif

    sysfs_between_accon_and_resumeid_incrementing = 1;
}
// FUJITSU TEN:2014-01-08 #55575 end

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      resume_notifier_bu_notify_resume_after_tristate                      */
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
static void resume_notifier_bu_notify_resume_after_tristate(void)
{
#ifdef DEF_BUDET_KPRINT
	printk("resume_notifier_bu_notify_resume_after_tristate() called\n");
#endif

    set_resume_status(NOW_RESUME);
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      resume_notifier_bu_notify_resume_before_reinit                       */
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
static void resume_notifier_bu_notify_resume_before_reinit(void)
{
#ifdef DEF_BUDET_KPRINT
	printk("resume_notifier_bu_notify_resume_before_reinit() called\n");
#endif
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      resume_notifier_bu_notify_resume_after_reinit                        */
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
static void resume_notifier_bu_notify_resume_after_reinit(void)
{
//#ifdef DEF_BUDET_KPRINT
	printk("[resumenotifier] <<< resume_notifier_bu_notify_resume_after_reinit() called, unlock fops >>>\n");
    DRVREC_INFO(TRACE_AFTER_REINIT, 0, "resume_notifier_bu_notify_resume_after_reinit");
//#endif
	/* waking read_q leads to the cancellation of waiting resume */
	resume_state = 1;
	wake_up_interruptible(&read_q);
}

static struct bu_notifier resume_notifier_bu_notify = {
// FUJITSU TEN:2014-01-08 #55575 start
	.resume_before_tristate = resume_notifier_bu_notify_resume_before_tristate,
// FUJITSU TEN:2014-01-08 #55575 end
	.resume_after_tristate = resume_notifier_bu_notify_resume_after_tristate,
	.resume_before_reinit = resume_notifier_bu_notify_resume_before_reinit,
	.resume_after_reinit = resume_notifier_bu_notify_resume_after_reinit,
};
#endif /* CONFIG_SUSPEND */

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      resume_notifier_resume                                               */
/*  [DESCRIPTION]                                                            */
/*      This function cancels the waiting state of read()                    */
/*      to inform user space of resume.                                      */
/*  [TYPE]                                                                   */
/*      static int                                                           */
/*  [PARAMETER]                                                              */
/*      struct platform_device *pfdev: platform_device structure             */
/*  [RETURN]                                                                 */
/*      0:  success                                                          */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static int resume_notifier_resume(struct platform_device* pfdev)
{
#if 0
    /* waking read_q leads to the cancellation of waiting resume */
    resume_state = 1;
// FUJITSU TEN:2014-01-08 #55575 start
    sysfs_between_accon_and_resumeid_incrementing = 1;
// FUJITSU TEN:2014-01-08 #55575 end
    wake_up_interruptible(&read_q);
#endif

    return 0;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      resume_notifier_init                                                 */
/*  [DESCRIPTION]                                                            */
/*      Initialization                                                       */
/*  [TYPE]                                                                   */
/*      static int                                                           */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      0:      success                                                      */
/*      non 0:  failure                                                      */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static int __init resume_notifier_init(void)
{
    int ret;

    ret = platform_driver_register(&resume_notifier_pfdriver);
    if (ret) {
        printk(KERN_ERR "resume_notifier_init() failed. err(%d)\n", ret);
        DRVREC_ERRORTRACE2(TRACE_INIT_PLATFORM_DRIVER_REGISTER_ERROR, ret);
        return ret;
    }

// FUJITSU TEN:2014-01-08 #55575 start
    resume_notifier_sysfs_init();
// FUJITSU TEN:2014-01-08 #55575 end

#ifdef CONFIG_SUSPEND
    DRVREC_INFO(TRACE_INIT_BU_NOTIFIER_REGISTER, 0, "resume_notifier_init");
	bu_notifier_register(&resume_notifier_bu_notify);
#endif /* CONFIG_SUSPEND */
    return ret;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      resume_notifier_probe                                                */
/*  [DESCRIPTION]                                                            */
/*      probe                                                                */
/*  [TYPE]                                                                   */
/*      static int                                                           */
/*  [PARAMETER]                                                              */
/*      struct platform_device *pfdev: platform_device structure             */
/*  [RETURN]                                                                 */
/*      0:      success                                                      */
/*      non 0:  failure                                                      */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static int resume_notifier_probe(struct platform_device* pfdev)
{
    int ret;

    ret = misc_register(&resume_notifier_device);
    if (ret) {
        printk(KERN_ERR "resume_notifier_probe() failed. err(%d)\n", ret);
        DRVREC_ERRORTRACE2(TRACE_PROVE_MISC_REGISTER_ERROR, ret);
        return ret;
    }
    
    /* initialize wait queue */
    DRVREC_INFO(TRACE_PROBE_INIT_WAITQUEUE_HEAD, 0, "init_waitqueue_head");
    init_waitqueue_head(&read_q);

    return 0;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      resume_notifier_exit                                                 */
/*  [DESCRIPTION]                                                            */
/*      Termination                                                          */
/*  [TYPE]                                                                   */
/*      static void                                                          */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static void __exit resume_notifier_exit(void)
{
    DEBUGK("resume_notifier_exit\n");
// FUJITSU TEN:2014-01-08 #55575 start
    resume_notifier_sysfs_exit();
// FUJITSU TEN:2014-01-08 #55575 end
    platform_driver_unregister(&resume_notifier_pfdriver);
    DEBUGK("end resume_notifier_exit\n");
#ifdef CONFIG_SUSPEND
	bu_notifier_unregister(&resume_notifier_bu_notify);
#endif /* CONFIG_SUSPEND */
}

static int resume_notifier_remove(struct platform_device* pfdev)
{
    DEBUGK("resume_notifier_remove\n");

    misc_deregister(&resume_notifier_device);
    DEBUGK("end resume_notifier_remove\n");
    return 0;
}

static int resume_notifier_suspend(struct platform_device* pfdev, pm_message_t state )
{
    //nothing to do.
    int ret = set_resume_status(EXCEPT_RESUME);
    return 0;
}

// FUJITSU TEN:2014-01-08 #55575 start
/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      resume_notifier_sysfs_preparing_read                                 */
/*  [DESCRIPTION]                                                            */
/*      Read sysfs flag of between "ACC-ON" and "resumeId incrementing".     */
/*  [TYPE]                                                                   */
/*      static ssize_t                                                       */
/*  [PARAMETER]                                                              */
/*      struct kobject *kobj:         Sysfs kobject                          */
/*      struct kobj_attribute *attr:  Attribute of sysfs kobject             */
/*      char *buf:                    Value of sysfs flag                    */
/*  [RETURN]                                                                 */
/*      More than 1:  Success                                                */
/*      0 or less:    Failure                                                */
/*  [NOTE]                                                                   */
/*      Values of *buf are as follows.                                       */
/*      "0":  State is not between "ACC-ON" and "resumeId incrementing".     */
/*      "1":  State is between "ACC-ON" and "resumeId incrementing".         */
/*****************************************************************************/
static ssize_t resume_notifier_sysfs_preparing_read(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", sysfs_between_accon_and_resumeid_incrementing);
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      resume_notifier_sysfs_init                                           */
/*  [DESCRIPTION]                                                            */
/*      Initialization of sysfs.                                             */
/*  [TYPE]                                                                   */
/*      static int                                                           */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      0:      Success                                                      */
/*      Non 0:  Failure                                                      */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static int resume_notifier_sysfs_init(void)
{
    int retval;

    resume_notifier_sysfs_kobj = kobject_create_and_add("resumenotifier", kernel_kobj);

    if(!resume_notifier_sysfs_kobj){
        return -ENOMEM;
    }

    retval = sysfs_create_group(resume_notifier_sysfs_kobj, &resume_notifier_sysfs_attr_group);

    if(retval){
        kobject_put(resume_notifier_sysfs_kobj);
    }

    return retval;
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      resume_notifier_sysfs_exit                                           */
/*  [DESCRIPTION]                                                            */
/*      Termination of sysfs.                                                */
/*  [TYPE]                                                                   */
/*      static void                                                          */
/*  [PARAMETER]                                                              */
/*      void                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static void resume_notifier_sysfs_exit(void)
{
    kobject_put(resume_notifier_sysfs_kobj);
}
// FUJITSU TEN:2014-01-08 #55575 end


module_init(resume_notifier_init);
module_exit(resume_notifier_exit);

extern bool get_resume_notifier_flag(unsigned int offset)
{
	return 0;
}

MODULE_LICENSE("GPL");

