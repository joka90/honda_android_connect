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

#ifndef _EARLYDEVICE_H
#define _EARLYDEVICE_H

#include <linux/list.h>
#include <linux/device.h>



/**
 * {@link #sudden_device_poweroff notification for immediate device power off},
 * {@link #reinit notification for reinit} which early devices need is defined.
 *
 * @version 1.0
 */
struct early_device {
	struct list_head node;
	/**
	 * callback called when #early_device is reinitialized right after the
	 * process scheduler restarts.
	 *
     * @param dev pass #dev of this structure.
	 * @return 0 Success.
	 * @return non-0 Failure. output a log with printk.
	 */
	int (*reinit)(struct device* dev);

	/**
	 * callback called from worker thread right before the process scheduler restarts
	 * during resume sequence.
	 * 
     * @param dev pass #dev of this structure.
	 */
	void (*prepare_to_reinit)(struct device* dev);

	/**
	 * callback called from interrupt handler when immediate device power off
	 * caused by BU-DET, ACC-OFF is occured.
	 * 
     * @param dev pass #dev of this structure.
	 * @remark please note this callback is called
	 * as a context of interrupt handler.
	 */
	void (*sudden_device_poweroff)(struct device* dev);

	/**
	 * #reinit,{@link #sudden_device_poweroff immediate device power off}
	 * set the device structure passed as a parameter in notification.
	 */
	struct device *dev;
};

/**
 * Callback type definition
 * @param [in] flag  flag value. 
 * @param [in] param a given parameter. value specified
 * by #early_device_invoke_with_flag_irqsavewill is passed without change.
 * @return a given value with int type. error code is assumed normally.
 */
typedef int (*early_device_invoke_with_flag_function)(bool flag, void* param);

/**
 * #reinit, register #early_device needed 
 * by immediate device power off notification.
 * @param edev pass #early_device structure to be noticed.
 * @return always return 0.
 *
 * @see early_device_unregister
 * @version 1.0
 */
extern int early_device_register(struct early_device *edev);

/**
 * #reinit, cancel the notification of #early_device
 * which registers notification of immediate device power off.
 * 
 * @param edev pass #early_device structure which cancels notification.
 *
 * @see early_device_register
 * @version 1.0
 */
extern void early_device_unregister(struct early_device *edev);



/** 
 * Acquisition of flag state and callback execution atomically
 * in the interval excluded by spin_lock_irqsave.
 *
 * spin_lock will be acquired and callback passed will be done
 * if the specified flag is enabled.
 * param is passed as a given parameter for callback function.
 * spin_lock will be cancelled after callback function is done.
 * If callback cannot be done for some reason, error code will be returned.
 *
 * @param [in] flag_id flag id for acquiring the state.
 * @param [in] callback function run if the flag specified by flag_id is true.
 * @param [in] param a given parameter passed when callback is done.
 * @return - callback return value
 */
int early_device_invoke_with_flag_irqsave(int flag_id,
                early_device_invoke_with_flag_function callback, void* param);

void early_device_resume_flag_set(int flagid, bool value);

#define EARLY_DEVICE_BUDET_INTERRUPTION_MASKED 0

#endif

