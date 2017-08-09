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

#ifndef ADA_SUSPEND_H
#define ADA_SUSPEND_H

/**********/
/* Macros */
/**********/

/* ==================== */
/* BU-DET Mode Settings */
/* ==================== */

/* Suspend freeze time(ms) */
#define BUDET_SUSPEND_ASYNC_TASK_FREEZE_WAITTIME	(50)		/* Async task freeze */
#define BUDET_SUSPEND_FORCE_TASK_FREEZE_BUDET_WAITTIME	(200)		/* force task freeze(budet) */
#define BUDET_SUSPEND_FORCE_TASK_FREEZE_ACCOFF_WAITTIME	(2000)		/* force task freeze(accoff) */

/* Transition G to LP core (Suspend) Operation Mask */
#define BUDET_MODE_MASK_EXTRA_SUSPEND			(0x00000001)
#define BUDET_MODE_MASK_USE_ASYNC_SYNC			(0x00000002)
#define BUDET_MODE_MASK_USE_ASYNC_TASK_FREEZE		(0x00000004)
#define BUDET_MODE_MASK_NO_GCORE_DISABLE_CPU		(0x00000008)
#define BUDET_MODE_MASK_NO_GCORE_TO_LPCORE		(0x00000010)
#define BUDET_MODE_MASK_NO_COMPLETE_SYNC		(0x00000020)
#define BUDET_MODE_MASK_FORCE_TASK_FREEZE		(0x00000040)
#define BUDET_MODE_MASK_FORCE_STOP_READWRITE		(0x00000080)
#define BUDET_MODE_MASK_FORCE_STOP_EMMC			(0x00000100)

/* Transition LP to G core (Resume) Operation Mask */
#define BUDET_MODE_MASK_NO_LP_TO_G_CLUSTOR		(0x00001000)
#define	BUDET_MODE_MASK_RESUME_CPU1_ORIGINAL_TIMING	(0x00002000)

/* Flag Check Masks */
#define IS_BUDET_MODE_EXTRA_SUSPEND(c) 			( ( c ) & BUDET_MODE_MASK_EXTRA_SUSPEND )
#define IS_BUDET_MODE_USE_ASYNC_SYNC(c)			( ( c ) & BUDET_MODE_MASK_USE_ASYNC_SYNC )
#define IS_BUDET_MODE_USE_ASYNC_TASK_FREEZE(c)		( ( c ) & BUDET_MODE_MASK_USE_ASYNC_TASK_FREEZE )
#define IS_BUDET_MODE_NO_GCORE_DISABLE_CPU(c)		( ( c ) & BUDET_MODE_MASK_NO_GCORE_DISABLE_CPU )
#define IS_BUDET_MODE_NO_GCORE_TO_LPCORE(c)		( ( c ) & BUDET_MODE_MASK_NO_GCORE_TO_LPCORE )
#define IS_BUDET_MODE_NO_COMPLETE_SYNC(c)		( ( c ) & BUDET_MODE_MASK_NO_COMPLETE_SYNC )
#define IS_BUDET_MODE_FORCE_TASK_FREEZE(c)		( ( c ) & BUDET_MODE_MASK_FORCE_TASK_FREEZE )
#define IS_BUDET_MODE_FORCE_STOP_READWRITE(c)		( ( c ) & BUDET_MODE_MASK_FORCE_STOP_READWRITE )
#define IS_BUDET_MODE_FORCE_STOP_EMMC(c)		( ( c ) & BUDET_MODE_MASK_FORCE_STOP_EMMC )

#define IS_BUDET_MODE_NO_LP_TO_G_CLUSTOR(c)		( ( c ) & BUDET_MODE_MASK_NO_LP_TO_G_CLUSTOR )
#define IS_BUDET_MODE_RESUME_CPU1_ORIGINAL_TIMING(c)	( ( c ) & BUDET_MODE_MASK_RESUME_CPU1_ORIGINAL_TIMING )

/* Operation mask code (for enable bu-det specific operation) */

#define BUDET_MODE_MASK_ENABLE	\
	( \
		BUDET_MODE_MASK_EXTRA_SUSPEND \
	  	| BUDET_MODE_MASK_USE_ASYNC_SYNC \
	  	| BUDET_MODE_MASK_USE_ASYNC_TASK_FREEZE \
	  	| BUDET_MODE_MASK_FORCE_TASK_FREEZE \
	  	| BUDET_MODE_MASK_FORCE_STOP_READWRITE \
	    | BUDET_MODE_MASK_NO_COMPLETE_SYNC \
	 )

/* Operation mask code (for disable bu-det specific operation) */
#define BUDET_MODE_MASK_DISABLE	(0)

/* DEFAULT Operation mask code */
#define BUDET_MODE_MASK_DEFAULT	BUDET_MODE_MASK_ENABLE

/* ========================== */
/* BU-DET Resume Unmask Mode  */
/* ========================== */
#define BUDET_RESUME_UNMASK_MODE_BEFORE_TRISTATE		(0)
#define BUDET_RESUME_UNMASK_MODE_AFTER_TRISTATE			(1)
#define BUDET_RESUME_UNMASK_MODE_AFTER_PREPARE_TO_REINIT	(2)
#define BUDET_RESUME_UNMASK_MODE_AFTER_REINIT			(3)
#define BUDET_RESUME_UNMASK_MODE_MAX				BUDET_RESUME_UNMASK_MODE_AFTER_REINIT

#if	defined(CONFIG_ADA_BUDET_RESUME_MODE_BEFORE_TRISTATE)
#define	BUDET_RESUME_UNMASK_MODE_DEFAULT	BUDET_RESUME_UNMASK_MODE_BEFORE_TRISTATE
#elif	defined(CONFIG_ADA_BUDET_RESUME_MODE_AFTER_TRISTATE)
#define	BUDET_RESUME_UNMASK_MODE_DEFAULT	BUDET_RESUME_UNMASK_MODE_AFTER_TRISTATE
#elif	defined(CONFIG_ADA_BUDET_RESUME_MODE_AFTER_PREPARE_TO_REINIT)
#define	BUDET_RESUME_UNMASK_MODE_DEFAULT	BUDET_RESUME_UNMASK_MODE_AFTER_PREPARE_TO_REINIT
#elif	defined(CONFIG_ADA_BUDET_RESUME_MODE_AFTER_REINIT)
#define	BUDET_RESUME_UNMASK_MODE_DEFAULT	BUDET_RESUME_UNMASK_MODE_AFTER_REINIT
#else	/* not defined */
#error	"CONFIG_ADA_BUDET_RESUME_MODE undefined."
#endif

/* ======================== */
/* BU-DET Mode Device Names */
/* ======================== */

/* [sysfs]BU-DETECT root tree name */
#define BUDET_SYSFS_ROOT_NAME	"budet"
/* FUJITSU TEN:2013-10-01 HDMI SUSPEND Start */
/* [sysfs]BU-DETECT notify device name */
#define BUDET_SYSFS_NOTIFY_DEVICE_NAME	notify
/* [sysfs]BU-DETECT complete device name */
#define BUDET_SYSFS_COMPLETE_DEVICE_NAME	complete
/* FUJITSU TEN:2013-10-01 HDMI SUSPEND End */
/* [sysfs]BU-DETECT mode device name */
#define BUDET_SYSFS_MODE_DEVICE_NAME	mode
/* [sysfs]BU-DETECT run device name */
#define BUDET_SYSFS_RUN_DEVICE_NAME	run
/* [sysfs]BU-DETECT unmask mode device name */
#define BUDET_SYSFS_UNMASK_DEVICE_NAME	unmask
/* [sysfs]BU-DETECT unmask mode device name */
#define BUDET_SYSFS_ASYNC_TASK_FREEZE_DEVICE_NAME	async_task_freeze_time
/* [sysfs]BU-DETECT unmask mode device name */
#define BUDET_SYSFS_FORCE_TASK_FREEZE_BUDET_DEVICE_NAME		force_task_freeze_budet_time
#define BUDET_SYSFS_FORCE_TASK_FREEZE_ACCOFF_DEVICE_NAME	force_task_freeze_accoff_time

/* [sysfs]BU-DETECT root enable flag */
#define BUDET_SYSFS_DEVICE_CREATE	1
/* FUJITSU TEN:2013-10-01 HDMI SUSPEND Start */
/* [sysfs]BU-DETECT notify device enable flag */
#define BUDET_SYSFS_DEVICE_NOTIFY_CREATE	1
/* [sysfs]BU-DETECT complete device enable flag */
#define BUDET_SYSFS_DEVICE_COMPLETE_CREATE	1
/* FUJITSU TEN:2013-10-01 HDMI SUSPEND End */
/* [sysfs]BU-DETECT mode device enable flag */
#define BUDET_SYSFS_DEVICE_MODE_CREATE	1
/* [sysfs]BU-DETECT run device enable flag(for debug only) */
#define BUDET_SYSFS_DEVICE_RUN_CREATE	0
/* [sysfs]BU-DETECT unmask mode device enable flag */
#define BUDET_SYSFS_DEVICE_UNMASK_CREATE	1
/* [sysfs]BU-DETECT async task freeze device enable flag */
#define BUDET_SYSFS_DEVICE_ASYNC_TASK_FREEZE_CREATE	1
/* [sysfs]BU-DETECT force task freeze device enable flag */
#define BUDET_SYSFS_DEVICE_FORCE_TASK_FREEZE_CREATE	1


/******************************************************/
/* exported symbols from "drivers/bu_det/bu_detect.c" */
/******************************************************/
/**
 * Get current BUDET pin state
 * @returns pin state (0:off, !0: on)
 */
extern int budet_get_gpio_budet_pin_state(void);
/**
 * Get current ACCOFF pin state
 * @returns pin state (0:off, !0: on)
 */
extern int budet_get_gpio_accoff_pin_state(void);
/****************************************************/
/* exported symbols from "kernel/suspend/process.c" */
/****************************************************/
/**
 * Try freeze tasks with timeout
 * @param sig_only Send process-signal only(don't freeze other threads)
 *                 Note: if false sets, freeze all process/kernel threads
 * @param end_time freeze timeout time(jiffies)
 * @returns 0: successful, others are error(errno)
 */
extern int try_to_freeze_tasks_time(bool sig_only, unsigned long end_time );

/****************************************************/
/* exported symbols from "arch/arm/mach-tegra/pm.c" */
/****************************************************/
/**
 * Transition from G core to LP core.
 */
extern int tegra_switch_to_lp_cluster(void);

/**
 * Transition from LP core to G core.
 */
extern int tegra_switch_to_g_cluster(void);

/**************************************************/
/* exported symbols from "kernel/power/suspend.c" */
/**************************************************/
/**
 * BU-DET mode configuration value
 */
extern unsigned int budet_suspend_config;

/**
 * BU-DET mode interrupt unmask mode.
 * (0: Before tri-state, 1: After tri-state, 2: After prepare-reinit, 3: After reinit)
 */
extern unsigned int budet_resume_unmask_mode ;

/**
 * BU-DET mode async taskfreeze timeout time(ms)
 */
extern unsigned int budet_suspend_async_taskfreeze_timeout_time;

/**
 * BU-DET mode force taskfreeze timeout time(ms)
 */
extern unsigned int budet_suspend_force_taskfreeze_budet_timeout_time;

/**
 * ACC-OFF mode force taskfreeze timeout time(ms)
 */
extern unsigned int budet_suspend_force_taskfreeze_accoff_timeout_time;

#endif /* ADA_SUSPEND_H */
