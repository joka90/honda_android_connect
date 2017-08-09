/* 
 * nonvolatile data READ/WRITE driver for Android
 *
 * Copyright(C) 2009,2011 FUJITSU LIMITED
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

#ifndef _NONVOLATILE_H
#define _NONVOLATILE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <linux/nonvolatile.h>

#define	NONVOLATILE_BLOCK_DEV	"mmcblk0"
#define	NONVOLATILE_PART_NAME	"NV1"

//#define	NONVOLATILE_APL0_ADDR	0xBEC00000
//#define	NONVOLATILE_APL0_OFFSET	0x42400000

#define	NONVOLATILE_SIZE_ACC	0x400		// Nonvolatile Header data size
#define	NONVOLATILE_OFFSET_ACC	0x00000400	// Nonvolatile Header data offset

#define NONVOLATILE_MODE_NORMAL		0
#define NONVOLATILE_MODE_REFRESH    (NONVOLATILE_SIZE_ACC \
								   + NONVOLATILE_SIZE \
								   + NONVOLATILE_SIZE_NONACCESS )

#define NONVOLATILE_MAX_COUNT	10
#define	NONVOLATILE_WAIT_TIME	5
#define	NONVOLATILE_ACCLOG_FLG	0x01B3

#define CLASS_NAME				"nonvolatile"
#define DRIVER_NAME				"nonvolatile"

#define DEVICE_NAME				"nonvolatile"
#define	DEVICE_NAME_MODE		"mode"

#define NONVOLATILE_CTL_HEADER0 0x33333334
#define NONVOLATILE_CTL_HEADER1 0x55555556
#define NONVOLATILE_PAGE_SIZE (256 * 1024)

#define NONVOL_MODE_NORMAL		0
#define	NONVOL_MODE_ONLY_SDRAM	1
#define NONVOL_MODE_MEMORY		2

/* UNIT TEST DEBUG */
#ifndef CONFIG_NONVOLATILE_MODE_NORMAL
#define DEBUG_CREATE_HEADER
#endif
/* UNIT TEST DEBUG */

#define NONVOLATILE_HEADER_SIZE	12

int GetNONVOLATILE(unsigned char*, uint, uint);
int SetNONVOLATILE(unsigned char*, uint,  uint);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define __file__ (strrchr(__FILE__,'/' ) + 1 )

#define NONVOL_EMERG_LOG( format, ...) { \
        printk( KERN_EMERG "[%s:%4d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define NONVOL_ERR_LOG( format, ...) { \
        printk( KERN_ERR "[%s:%4d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define NONVOL_WARNING_LOG( format, ...) { \
        printk( KERN_WARNING "[%s:%4d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#if defined(CONFIG_NONVOL_LOG_NOTICE) || defined(CONFIG_NONVOL_LOG_INFO) \
        || defined(CONFIG_NONVOL_LOG_DEBUG)
#define NONVOL_NOTICE_LOG( format, ...) { \
        printk( KERN_NOTICE "[%s:%4d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define NONVOL_NOTICE_LOG( format, ... )
#endif

#if defined(CONFIG_NONVOL_LOG_INFO) || defined(CONFIG_NONVOL_LOG_DEBUG)
#define NONVOL_INFO_LOG( format, ...) { \
        printk( KERN_INFO "[%s:%4d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define NONVOL_INFO_LOG( format, ... )
#endif

#ifdef CONFIG_NONVOL_LOG_DEBUG
#define NONVOL_DEBUG_LOG( format, ...) { \
        printk( KERN_DEBUG "[%s:%4d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define NONVOL_DEBUG_LOG( format, ... )
#endif

#ifdef CONFIG_NONVOL_DEBUG_FUNCTION_INOUT
#define NONVOL_DEBUG_FUNCTION_ENTER      {NONVOL_INFO_LOG("in\n");}
#define NONVOL_DEBUG_FUNCTION_ENTER_ONLY {NONVOL_INFO_LOG("in(out)\n");}
#define NONVOL_DEBUG_FUNCTION_EXIT       {NONVOL_DEBUG_LOG("out\n");}
#else
#define NONVOL_DEBUG_FUNCTION_ENTER
#define NONVOL_DEBUG_FUNCTION_ENTER_ONLY
#define NONVOL_DEBUG_FUNCTION_EXIT
#endif


#endif	/* _NONVOLATILE_H */

