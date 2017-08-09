/*
 * Atmel maXTouch Touchscreen driver
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
/*
 *----------------------------------------------------------------------------
 * COPYRIGHT(C) FUJITSU TEN LIMITED 2012
 *----------------------------------------------------------------------------
 */

#ifndef __LINUX_ATMEL_MXT_TS_H
#define __LINUX_ATMEL_MXT_TS_H

#include <linux/types.h>

/* FUJITSU TEN:2012-06-19 touch use I2C address add. start */
/* I2c Slave Address */
#define MXT540E_I2C_ADDR1	0x4C
#define MXT_TSNS_RESET_EXEC_TIME	20
/* FUJITSU TEN:2012-06-19 touch use I2C address add. end */

/* Orient */
#define MXT_NORMAL		0x0
#define MXT_DIAGONAL		0x1
#define MXT_HORIZONTAL_FLIP	0x2
#define MXT_ROTATED_90_COUNTER	0x3
#define MXT_VERTICAL_FLIP	0x4
#define MXT_ROTATED_90		0x5
#define MXT_ROTATED_180		0x6
#define MXT_DIAGONAL_COUNTER	0x7

/* The platform data for the Atmel maXTouch touchscreen driver */
struct mxt_platform_data {
	unsigned long irqflags;
	u8(*read_chg) (void);
	/* FUJITSU TEN:2012-06-19 exec_tsns_reset add. start */
	void(*tsns_reset) (int);
	/* FUJITSU TEN:2012-06-19 exec_tsns_reset add. end */
	/* FUJITSU TEN:2012-08-10 low_tsns_reset add. start */
	void(*low_tsns_reset) (void);
	/* FUJITSU TEN:2012-08-10 low_tsns_reset add. end */
};

/* FUJITSU TEN:2012-06-19 logmacro add. start */
#define MXT_TS_EMERG_LOG( format, ...) { \
	printk( KERN_EMERG "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define MXT_TS_ERR_LOG( format, ...) { \
	printk( KERN_ERR "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define MXT_TS_WARNING_LOG( format, ...) { \
	printk( KERN_WARNING "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#if defined(CONFIG_TOUCH_LOG_NOTICE) || defined(CONFIG_TOUCH_LOG_INFO) \
	|| defined(CONFIG_TOUCH_LOG_DEBUG)
#define MXT_TS_NOTICE_LOG( format, ...) { \
	printk( KERN_NOTICE "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define MXT_TS_NOTICE_LOG( format, ... )
#endif

#if defined(CONFIG_TOUCH_LOG_INFO) || defined(CONFIG_TOUCH_LOG_DEBUG)
#define MXT_TS_INFO_LOG( format, ...) { \
	printk( KERN_INFO "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define MXT_TS_INFO_LOG( format, ... )
#endif

#ifdef CONFIG_TOUCH_LOG_DEBUG 
#define MXT_TS_DEBUG_LOG( format, ...) { \
	printk( KERN_DEBUG "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define MXT_TS_DEBUG_LOG( format, ... )
#endif

/* FUJITSU TEN:2012-06-19 logmacro add. edit */
#endif /* __LINUX_ATMEL_MXT_TS_H */
