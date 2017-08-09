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

#ifndef _BU_NOTIFIER_H
#define _BU_NOTIFIER_H

#include <linux/list.h>
#include <linux/device.h>


/*************************/
/* BU-Notifier Structure */
/*************************/

/**
 * battery mode notification for resume sequence
 *
 * @version 1.0
 */
struct bu_notifier {
	struct list_head node;

	/**
	 * Before tristate-on callback
	 * @remark please note this callback called before schduler start.
	 */
	void (*resume_before_tristate)(void);

	/**
	 * Tristate-on complete callback
	 * @remark please note this callback called before schduler start.
	 */
	void (*resume_after_tristate)(void);

	/**
	 * After prepare to reinit callback
	 * @remark please note this callback called before schduler start.
	 */
	void (*resume_after_prepare_to_reinit)(void);

	/**
	 * before re-init callback
	 * @remark please note this callback called after schduler start.
	 */
	void (*resume_before_reinit)(void);
	
	/**
	 * Post re-init callback
	 */
	void (*resume_after_reinit)(void);
};

/**
 * Register battery mode notification.
 * @param device pass bu_notifier structure to be noticed.
 *
 * @see bu_notifier_unregister
 * @version 1.0
 */
extern int bu_notifier_register(struct bu_notifier *device);

/**
 * Unregister battery mode notification.
 * @param device pass bu_notifier structure which cancelled notification.
 *
 * @see bu_notifier_register
 * @version 1.0
 */
extern void bu_notifier_unregister(struct bu_notifier *device);

/**
 * Notify battery mode notification.
 * @param mode notification kind.
 *
 * @version 1.0
 */
extern void bu_notifier_notify(int mode);


/*********************/
/* Notification Kind */
/*********************/
enum {
	BU_RESUME_BEFORE_TRISTATE=0,	/* Before tristate, No schduler */
	BU_RESUME_AFTER_TRISTATE,	/* Between tristate and prepare to re-init, No schduler */
	BU_RESUME_AFTER_PREPARE_TO_REINIT,	/* Between prepare to re-init and start schduler, No schduler */
	BU_RESUME_BEFORE_REINIT,	/* Between start schduler and re-init */
	BU_RESUME_AFTER_REINIT		/* After re-init */
};

#endif /* BU_NOTIFIER_H */

