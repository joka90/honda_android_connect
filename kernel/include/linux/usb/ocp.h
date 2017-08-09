/*
 * Copyright(C) 2012-2013 FUJITSU TEN LIMITED
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
#ifndef __LINUX_USBOCP_H
#define __LINUX_USBOCP_H


#define USB_OVER_CURRENT_PROTECTION "usbocp"

#define THRESHOLD_USB_CURRENT_OVER (3)

#define USBOCP_START_POLLING_INTERVAL (100)

#define USBOCP_POLLING_INTERVAL (10)

#define THRESHOLD_USB_CURRENT_OVER_MIN      0x01
#define USBOCP_START_POLLING_INTERVAL_MIN   0x01
#define USBOCP_POLLING_INTERVAL_MIN         0x01

#define USBOCP_USB_ENABLE	1
#define USBOCP_USB_DISABLE	0

#define USBOCP_ERRORID_OVERCURRENT	0x0000

#define USBOCP_USBOCP2 2
#define USBOCP_USBOCP3 3

enum {
	USBOCP2_PGOOD = 0,
	USBOCP3_PGOOD,
	USBOCP3_EN,
	USBOCP2_EN,
};

#define USBOCP_POLLING_DISABLED	0
#define USBOCP_POLLING_ENABLED	1

#define USBOCP_POLLING_STOP	0
#define USBOCP_POLLING_BUSY	1

#define USBOCP_RUNNING	0
#define USBOCP_FAST_SHUTDOWN	1

#define __file__ (strrchr(__FILE__,'/' ) + 1 )

#define USBOCP_EMERG_LOG( format, ...) { \
        printk( KERN_EMERG "[%s:%d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define USBOCP_ERR_LOG( format, ...) { \
        printk( KERN_ERR "[%d] %s " format, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define USBOCP_WARNING_LOG( format, ...) { \
        printk( KERN_WARNING "[%d] %s " format, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#if defined(CONFIG_OZ8052F_OCP_LOG_NOTICE) || defined(CONFIG_OZ8052F_OCP_LOG_INFO) \
        || defined(CONFIG_OZ8052F_OCP_LOG_DEBUG)
#define USBOCP_NOTICE_LOG( format, ...) { \
        printk( KERN_NOTICE "[%s:%d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define USBOCP_NOTICE_LOG( format, ... )
#endif

#if defined(CONFIG_OZ8052F_OCP_LOG_INFO) || defined(CONFIG_OZ8052F_OCP_LOG_DEBUG)
#define USBOCP_INFO_LOG( format, ...) { \
        printk( KERN_INFO "[%s:%d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define USBOCP_INFO_LOG( format, ... )
#endif

#ifdef CONFIG_OZ8052F_OCP_LOG_DEBUG
#define USBOCP_DEBUG_LOG( format, ...) { \
        printk( KERN_DEBUG "[%s:%d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define USBOCP_DEBUG_LOG( format, ... )
#endif


#endif /* __LINUX_USBOCP_H */

