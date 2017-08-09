/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2004-2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/*----------------------------------------------------------------------------*/
// COPYRIGHT(C) FUJITSU TEN LIMITED 2012
/*----------------------------------------------------------------------------*/

// Log tag define.
#ifndef	LOG_TAG
#define LOG_TAG "Bluez"
#endif

#include <ada/log.h>
#include <cutils/log.h>

void info(const char *format, ...) __attribute__((format(printf, 1, 2)));
void error(const char *format, ...) __attribute__((format(printf, 1, 2)));

void btd_debug(const char *format, ...) __attribute__((format(printf, 1, 2)));

void __btd_log_init(const char *debug, int detach);
void __btd_log_cleanup(void);
void __btd_toggle_debug(void);

struct btd_debug_desc {
	const char *file;
#define BTD_DEBUG_FLAG_DEFAULT (0)
#define BTD_DEBUG_FLAG_PRINT   (1 << 0)
	unsigned int flags;
} __attribute__((aligned(8)));

/**
 * DBG:
 * @fmt: format string
 * @arg...: list of arguments
 *
 * Simple macro around btd_debug() which also include the function
 * name it is called in.
 */
#define DBG(fmt, arg...) do { \
	static struct btd_debug_desc __btd_debug_desc \
	__attribute__((used, section("__debug"), aligned(8))) = { \
		.file = __FILE__, .flags = BTD_DEBUG_FLAG_DEFAULT, \
	}; \
	if (__btd_debug_desc.flags & BTD_DEBUG_FLAG_PRINT) \
		btd_debug("%s:%s() " fmt,  __FILE__, __FUNCTION__ , ## arg); \
} while (0)

/* 14A-DA 2012.10.05 ADD-S */
// Logging define.
#define USE_LOG 4

// non log output.
#if (USE_LOG==0)
#define BLUEZ_DEBUG_ERROR(fmt, ...)
#define BLUEZ_DEBUG_WARN(fmt, ...)
#define BLUEZ_DEBUG_INFO(fmt, ...)
#define BLUEZ_DEBUG_LOG(fmt, ...)
#define BLUEZ_DEBUG_MUST_LOG(fmt, ...)
#endif

// 14A-DA original log output.
#if (USE_LOG==1)
#define BLUEZ_DEBUG_ERROR(fmt, ...) \
		if(ada_log_isLoggable( "Bluez" , ANDROID_LOG_ERROR )){ \
			((void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_ERROR, LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)); \
		}
#define BLUEZ_DEBUG_WARN(fmt, ...)
#define BLUEZ_DEBUG_INFO(fmt, ...)
#define BLUEZ_DEBUG_LOG(fmt, ...)
#define BLUEZ_DEBUG_MUST_LOG(fmt, ...)
#endif

#if (USE_LOG==2)
#define BLUEZ_DEBUG_ERROR(fmt, ...) \
		if(ada_log_isLoggable( "Bluez" , ANDROID_LOG_ERROR )){ \
			((void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_ERROR, LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)); \
		}
#define BLUEZ_DEBUG_WARN(fmt, ...) \
		if(ada_log_isLoggable( "Bluez" , ANDROID_LOG_WARN )){ \
			((void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_WARN,  LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)); \
		}
#define BLUEZ_DEBUG_INFO(fmt, ...)
#define BLUEZ_DEBUG_LOG(fmt, ...)
#define BLUEZ_DEBUG_MUST_LOG(fmt, ...)
#endif

#if (USE_LOG==3)
#define BLUEZ_DEBUG_ERROR(fmt, ...) \
		if(ada_log_isLoggable( "Bluez" , ANDROID_LOG_ERROR )){ \
			((void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_ERROR, LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)); \
		}
#define BLUEZ_DEBUG_WARN(fmt, ...) \
		if(ada_log_isLoggable( "Bluez" , ANDROID_LOG_WARN )){ \
			((void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_WARN,  LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)); \
		}
#define BLUEZ_DEBUG_INFO(fmt, ...)  \
		if(ada_log_isLoggable( "Bluez" , ANDROID_LOG_INFO )){ \
			((void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_INFO,  LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)); \
		}
#define BLUEZ_DEBUG_LOG(fmt, ...)
#define BLUEZ_DEBUG_MUST_LOG(fmt, ...)
#endif

#if (USE_LOG==4)
#define BLUEZ_DEBUG_ERROR(fmt, ...) \
		if(ada_log_isLoggable( "Bluez" , ANDROID_LOG_ERROR )){ \
			((void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_ERROR, LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)); \
		}
#define BLUEZ_DEBUG_WARN(fmt, ...) \
		if(ada_log_isLoggable( "Bluez" ,ANDROID_LOG_WARN )){ \
			((void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_WARN,  LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)); \
		}
#define BLUEZ_DEBUG_INFO(fmt, ...)  \
		if(ada_log_isLoggable( "Bluez" , ANDROID_LOG_INFO )){ \
			((void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_INFO,  LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)); \
		}
#define BLUEZ_DEBUG_LOG(fmt, ...)   \
		if(ada_log_isLoggable( "Bluez" , ANDROID_LOG_DEBUG )){ \
			((void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_DEBUG, LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)); \
		}
#define BLUEZ_DEBUG_MUST_LOG(fmt, ...)   \
		if(ada_log_isLoggable( "Bluez" , ANDROID_LOG_WARN )){ \
			((void)ADA_LOG_PRINT_FORMAT(ANDROID_LOG_WARN,  LOG_TAG, __FUNCTION__, fmt, ##__VA_ARGS__)); \
		}
#endif
/* 14A-DA 2012.10.05 ADD-E */
