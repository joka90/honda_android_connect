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

#ifndef _BUDET_INTERRUPTIBLE_H
#define _BUDET_INTERRUPTIBLE_H

#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/completion.h>
#include <linux/firmware.h>
#include <linux/earlydevice.h>
#include <linux/resumenotifier.h>

/* polling itnerval (milli second) */
#define BUDET_INTERRUPTIBLE_INTERVAL_MS 4
/* polling itnerval (jiffies) */
#define BUDET_INTERRUPTIBLE_INTERVAL_JIFFIES \
	msecs_to_jiffies(BUDET_INTERRUPTIBLE_INTERVAL_MS)

/* simple callback */
static int get_flag_callback(bool flag, void* param) {
	return flag;
}

static inline int get_budet_flag(void) {
	return early_device_invoke_with_flag_irqsave(
		RNF_BUDET_ACCOFF_INTERRUPT_MASK, get_flag_callback, NULL);
}

static inline unsigned long msleep_budet_interruptible(unsigned int msecs) {
	unsigned int retval = msecs, sleeptime;

	while ( !get_budet_flag() ) {
		if (retval > BUDET_INTERRUPTIBLE_INTERVAL_MS) {
			sleeptime = BUDET_INTERRUPTIBLE_INTERVAL_MS;
		} else {
			sleeptime = retval;
		}
		retval -= sleeptime;

		msleep(sleeptime);
		if (retval == 0)
			break;
	}
	return retval;
}	

static inline unsigned long mdelay_budet_interruptible(unsigned int msecs) {
	unsigned int retval = msecs, sleeptime;

	while ( !get_budet_flag() ) {
		if (retval > BUDET_INTERRUPTIBLE_INTERVAL_MS) {
			sleeptime = BUDET_INTERRUPTIBLE_INTERVAL_MS;
		} else {
			sleeptime = retval;
		}
		retval -= sleeptime;

		mdelay(sleeptime);
		if (retval == 0)
			break;
	}
	return retval;
}

#define wait_event_budet_interruptible(wq, condition)					\
({																		\
	int ___ret = -ERESTARTSYS;											\
	long timeout;														\
																		\
	while ( !get_budet_flag() ) {										\
		timeout = wait_event_timeout(wq, condition, 					\
				BUDET_INTERRUPTIBLE_INTERVAL_JIFFIES);					\
																		\
		if (timeout > 0) {												\
			/* condition was true */									\
			___ret = 0;													\
			break;														\
		}																\
	}																	\
	___ret;																\
})


#define wait_event_budet_interruptible_timeout( wq, condition, timeout)	\
({																		\
	long ___ret = -ERESTARTSYS;											\
	long ___remain = timeout, ___sleeptime;								\
																		\
	if (___remain < 0) {											\
		___ret = -EINVAL;											\
	} else {														\
		while (1) {													\
			if ( get_budet_flag() ) {										\
				___ret = -ERESTARTSYS;										\
				break;														\
			}																\
			if (___remain > BUDET_INTERRUPTIBLE_INTERVAL_JIFFIES) {			\
				___sleeptime = BUDET_INTERRUPTIBLE_INTERVAL_JIFFIES;		\
			} else {														\
				___sleeptime = ___remain;									\
			}																\
			___remain -= ___sleeptime;										\
			___ret = wait_event_timeout(wq, condition, 						\
					___sleeptime);											\
																			\
			if (___ret > 0) {												\
				/* condition was true */									\
				___ret += ___remain;										\
				break;														\
			} else if (___remain == 0) {									\
				/* timeout */												\
				___ret = 0;													\
				break;														\
			}																\
		}																	\
	}																\
	___ret;																\
})


static inline int wait_for_completion_budet_interruptible(struct completion *x) {
	int retval = -ERESTARTSYS;
	unsigned long timeout;

	while ( !get_budet_flag() ) {
		timeout = wait_for_completion_timeout(x,
				BUDET_INTERRUPTIBLE_INTERVAL_JIFFIES);
		if (timeout > 0) {
			// complete
			retval = 0;
			break;
		}
	}
	return retval;
}

static inline long wait_for_completion_budet_interruptible_timeout(
	struct completion *x, unsigned long timeout) {
	long retval;
	unsigned long remain = timeout, sleeptime;

	while (1) {
		if ( get_budet_flag() ) {
			// interrupt
			retval = -ERESTARTSYS;
			break;
		}

		if (remain > BUDET_INTERRUPTIBLE_INTERVAL_JIFFIES) {
			sleeptime = BUDET_INTERRUPTIBLE_INTERVAL_JIFFIES;
		} else {
			sleeptime = remain;
		}
		remain -= sleeptime;

		retval = wait_for_completion_timeout(x, sleeptime);
		if (retval > 0) {
			// complete
			retval += remain;
			break;
		} else if (remain == 0) {
			// timeout
			retval = 0;
			break;
		}
	}
	return retval;
}

enum request_firmware_state {
	FW_STATE_LOADING,
	FW_STATE_DONE,
	FW_STATE_INTERRUPTED
};

struct firmware_result {
	const struct firmware *fw;
	struct completion comp;
	enum request_firmware_state state;
};

int request_firmware_budet_interruptible(
        const struct firmware **firmware_p, const char *name,
        struct device *device);

#endif  /* _BUDET_INTERRUPTIBLE_H */
