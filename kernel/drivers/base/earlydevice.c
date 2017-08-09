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

#include <linux/earlydevice.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>

#if 0
#define EARLY_DEVICE_DEBUG
#endif

static LIST_HEAD(ada_susres_ops_list);
static DEFINE_MUTEX(ada_susres_ops_lock);
static spinlock_t lock;
static unsigned long resumeflags;

static int initialized = false;

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      early_device_register                                                */
/*  [DESCRIPTION]                                                            */
/*      callback registration                                                */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      struct early_device *ops: pointer to registration structure          */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      register callback for suspend/resume                                 */
/*****************************************************************************/
int early_device_register(struct early_device *ops)
{
    if(!initialized) {
        spin_lock_init(&lock);
    }
#ifdef EARLY_DEVICE_DEBUG
	printk("early_device_register(%d) \n", __LINE__);
	printk("early_device->reinit(0x%x) \n", ops->reinit);
	if(ops->reinit)	printk("reinit 0x%x\n", *(ops->reinit));
#endif
	mutex_lock(&ada_susres_ops_lock);
	list_add_tail(&ops->node, &ada_susres_ops_list);
	mutex_unlock(&ada_susres_ops_lock);
#ifdef EARLY_DEVICE_DEBUG
	printk("early_device_register(%d) \n", __LINE__);
#endif
	return 0;
}
EXPORT_SYMBOL_GPL(early_device_register);

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      early_device_unregister                                              */
/*  [DESCRIPTION]                                                            */
/*      callback unregistration                                              */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      struct early_device *ops: pointer to registration structure          */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      unregister callback for suspend/resume                               */
/*****************************************************************************/
void early_device_unregister(struct early_device *ops)
{
#ifdef EARLY_DEVICE_DEBUG
	printk("early_device_unregister(%d) \n", __LINE__);
#endif
	mutex_lock(&ada_susres_ops_lock);
	list_del(&ops->node);
	mutex_unlock(&ada_susres_ops_lock);
#ifdef EARLY_DEVICE_DEBUG
	printk("early_device_unregister(%d) \n", __LINE__);
#endif
}
EXPORT_SYMBOL_GPL(early_device_unregister);

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      early_device_suspend_notify                                          */
/*  [DESCRIPTION]                                                            */
/*      suspend notification callback                                        */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      None                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      execute callback registered as suspend notification                  */
/*****************************************************************************/
int early_device_suspend_notify(void)
{
	struct early_device *ops;
	int ret = 0;

#ifdef EARLY_DEVICE_DEBUG
	printk("early_device_suspend_notify(%d) \n", __LINE__);
#endif

	list_for_each_entry_reverse(ops, &ada_susres_ops_list, node)
		if (ops->sudden_device_poweroff) {
#ifdef EARLY_DEVICE_DEBUG
			printk("Start driver sudden_device_poweroff\n");
#endif
			ops->sudden_device_poweroff(ops->dev);
		}

#ifdef EARLY_DEVICE_DEBUG
	printk("early_device_suspend_notify(%d) \n", __LINE__);
#endif
	return ret;
}
EXPORT_SYMBOL_GPL(early_device_suspend_notify);

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      early_device_prepare_to_reinit_notify                                */
/*  [DESCRIPTION]                                                            */
/*      prepare_to_reinit notification callback                              */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      None                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      execute callback registered as prepare_to_reinit notification        */
/*****************************************************************************/
void early_device_prepare_to_reinit_notify(void)
{
	struct early_device *ops;

#ifdef EARLY_DEVICE_DEBUG
	printk("early_device_prepare_to_reinit_notify(%d) \n", __LINE__);
#endif

	list_for_each_entry(ops, &ada_susres_ops_list, node)
		if (ops->prepare_to_reinit) {
#ifdef EARLY_DEVICE_DEBUG
			printk("Start driver prepare_to_reinit\n");
#endif
			ops->prepare_to_reinit(ops->dev);
		}
#ifdef EARLY_DEVICE_DEBUG
	printk("early_device_prepare_to_reinit_notify(%d) \n", __LINE__);
#endif
}
EXPORT_SYMBOL_GPL(early_device_prepare_to_reinit_notify);

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      early_device_resume_notify                                           */
/*  [DESCRIPTION]                                                            */
/*      resume notification callback                                         */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      None                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      execute callback registered as resume notification                   */
/*****************************************************************************/
void early_device_resume_notify(void)
{
	struct early_device *ops;

#ifdef EARLY_DEVICE_DEBUG
	printk("early_device_resume_notify(%d) \n", __LINE__);
#endif

	list_for_each_entry(ops, &ada_susres_ops_list, node)
		if (ops->reinit) {
#ifdef EARLY_DEVICE_DEBUG
			printk("Start driver reinit\n");
#endif
			ops->reinit(ops->dev);
		}
#ifdef EARLY_DEVICE_DEBUG
	printk("early_device_resume_notify(%d) \n", __LINE__);
#endif
}
EXPORT_SYMBOL_GPL(early_device_resume_notify);

/**
 *
 */
void early_device_resume_flag_set(int flagid, bool value)
{
    resumeflags = value; //TODO
}
EXPORT_SYMBOL_GPL(early_device_resume_flag_set);

/**
 *
 */
int early_device_invoke_with_flag_irqsave(int flag_id, early_device_invoke_with_flag_function callback, void* param)
{
    unsigned long eflags;
    int ret = EINVAL;
    bool flag = resumeflags ? true : false;

    spin_lock_irqsave(&lock, eflags);
    if(callback) {
        ret = callback(flag, param);
    }
    spin_unlock_irqrestore(&lock, eflags);
    return ret;
}
EXPORT_SYMBOL_GPL(early_device_invoke_with_flag_irqsave);

