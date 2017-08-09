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

#include <linux/bu_notifier.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>

//#define BU_NOTIFIER_DEBUG 1

static LIST_HEAD(bu_notifier_ops_list);
static DEFINE_MUTEX(bu_notifier_ops_list_mutex);

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      bu_notifier_register                                                 */
/*  [DESCRIPTION]                                                            */
/*      callback registration                                                */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      struct bu_notifier *ops: pointer to registration structure           */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      register callback for suspend/resume                                 */
/*****************************************************************************/
int bu_notifier_register(struct bu_notifier *ops)
{
	mutex_lock(&bu_notifier_ops_list_mutex);
	list_add_tail(&ops->node, &bu_notifier_ops_list);
	mutex_unlock(&bu_notifier_ops_list_mutex);

	return 0;
}
EXPORT_SYMBOL_GPL(bu_notifier_register);

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      bu_notifier_unregister                                               */
/*  [DESCRIPTION]                                                            */
/*      callback unregistration                                              */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      struct bu_notifier *ops: pointer to registration structure           */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      unregister callback for suspend/resume                               */
/*****************************************************************************/
void bu_notifier_unregister(struct bu_notifier *ops)
{
	mutex_lock(&bu_notifier_ops_list_mutex);
	list_del(&ops->node);
	mutex_unlock(&bu_notifier_ops_list_mutex);
}
EXPORT_SYMBOL_GPL(bu_notifier_unregister);

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      bu_notifier_notify                                                   */
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
void bu_notifier_notify(int mode)
{
	struct bu_notifier *ops;

#ifdef BU_NOTIFIER_DEBUG
	printk("bu_notifier_suspend_notify(%d) : %d \n", __LINE__, mode);
#endif
	mutex_lock(&bu_notifier_ops_list_mutex);

	list_for_each_entry(ops, &bu_notifier_ops_list, node){
		switch( mode ){
		
		case BU_RESUME_BEFORE_TRISTATE:
			if (ops->resume_before_tristate) {
#ifdef BU_NOTIFIER_DEBUG
				printk("Start BU_RESUME_BEFORE_TRISTATE\n");
#endif
				ops->resume_before_tristate();
			}
			break;

		case BU_RESUME_AFTER_TRISTATE:
			if (ops->resume_after_tristate) {
#ifdef BU_NOTIFIER_DEBUG
				printk("Start BU_RESUME_AFTER_TRISTATE\n");
#endif
				ops->resume_after_tristate();
			}
			break;

		case BU_RESUME_AFTER_PREPARE_TO_REINIT:
			if (ops->resume_after_prepare_to_reinit) {
#ifdef BU_NOTIFIER_DEBUG
				printk("Start BU_RESUME_AFTER_PREPARE_TO_REINIT\n");
#endif
				ops->resume_after_prepare_to_reinit();
			}
			break;

		case BU_RESUME_BEFORE_REINIT:
			if (ops->resume_before_reinit) {
#ifdef BU_NOTIFIER_DEBUG
				printk("Start BU_RESUME_BEFORE_REINIT\n");
#endif
				ops->resume_before_reinit();
			}
			break;
			
		case BU_RESUME_AFTER_REINIT:
			if (ops->resume_after_reinit) {
#ifdef BU_NOTIFIER_DEBUG
				printk("Start BU_RESUME_AFTER_REINIT\n");
#endif
				ops->resume_after_reinit();
			}
			break;
			

		default:
			printk("bu_notifier_notify: unknown mode %d\n", mode);
			break;
		}
	}
	mutex_unlock(&bu_notifier_ops_list_mutex);
}
EXPORT_SYMBOL_GPL(bu_notifier_notify);

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      bu_notifier_notify                                                   */
/*  [DESCRIPTION]                                                            */
/*      suspend notification initialize                                      */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      None                                                                 */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
static int __init bu_notifier_init(void)
{
#ifdef BU_NOTIFIER_DEBUG
				printk("bu_notifier_init()\n");
#endif
	return 0;
}

module_init(bu_notifier_init);
