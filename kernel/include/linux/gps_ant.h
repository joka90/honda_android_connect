/* 
 * GPS Antenna detect driver header
 *
 * Copyright(C) 2012 FUJITSU TEN LIMITED
 * Author: TAKESHI tsukui
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

#ifndef GPS_ANT_HEADER
#define GPS_ANT_HEADER

#define GPS_ANT_EMERG_LOG( format, ...) { \
	printk( KERN_EMERG "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define GPS_ANT_ERR_LOG( format, ...) { \
	printk( KERN_ERR "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define GPS_ANT_WARNING_LOG( format, ...) { \
	printk( KERN_WARNING "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#if defined(CONFIG_GPS_ANT_LOG_NOTICE) || defined(CONFIG_GPS_ANT_LOG_INFO) \
	|| defined(CONFIG_GPS_ANT_LOG_DEBUG)
#define GPS_ANT_NOTICE_LOG( format, ...) { \
	printk( KERN_NOTICE "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define GPS_ANT_NOTICE_LOG( format, ... )
#endif

#if defined(CONFIG_GPS_ANT_LOG_INFO) || defined(CONFIG_GPS_ANT_LOG_DEBUG)
#define GPS_ANT_INFO_LOG( format, ...) { \
	printk( KERN_INFO "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define GPS_ANT_INFO_LOG( format, ... )
#endif

#ifdef CONFIG_GPS_ANT_LOG_DEBUG 
#define GPS_ANT_DEBUG_LOG( format, ...) { \
	printk( KERN_DEBUG "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define GPS_ANT_DEBUG_LOG( format, ... )
#endif
#endif
