/**
 * Copyright (c) 2011-2013 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#ifndef _BU_TRACE_H
#define _BU_TRACE_H

#include <linux/lites_trace.h>

#define LOG_TRACE_NO_FASTBOOT_KERNEL (89 + LITES_KNL_LAYER)
#define LOG_TRACE_FMT_FASTBOOT_KERNEL (1)
#define LOG_TRACE_FMT_DRIVER_SUSPEND (10)
#define LOG_TRACE_INFO 3
#define LOG_TRACE_DEBUG 6
#define LOG_TRACE_ID_BU_TRACE (0)
 
#define BUDET_LITES_LOG_TRACE(pvalue_, tid_, gid_) ({\
    struct lites_trace_header trc_hdr = { \
        .level = LOG_TRACE_INFO, \
        .trace_no = LOG_TRACE_NO_FASTBOOT_KERNEL, \
        .trace_id = (tid_), \
        .format_id = LOG_TRACE_FMT_FASTBOOT_KERNEL, \
    }; \
    struct lites_trace_format trc_fmt = { \
        .buf = (pvalue_), \
        .count = sizeof(*(pvalue_)), \
        .trc_header= &trc_hdr, \
    }; \
    lites_trace_write((gid_), &trc_fmt); \
})

/**
 * Set suspend state flag in lites log header.
 *
 * Argument:
 *    SET_SUSPEND_SQ_S --- Suspend sequence started
 *    SET_SUSPEND_SQ_E --- Suspend sequence compelted
 */
#define LITES_SET_SUSPEND_STATE(value_) ({\
	unsigned short*p = 0; \
	BUDET_LITES_LOG_TRACE(p, (value_), REGION_TRACE_HEADER); \
})

/**
 * Write suspend/resume sequence trace log.
 *
 * Argument:
 *     Suspend/Resume seuqnce No.
 */
#define FASTBOOT_LOG_TRACE(value_) ({\
	unsigned short value = (value_); \
	unsigned short* p = &value; \
	BUDET_LITES_LOG_TRACE(p, LOG_TRACE_ID_BU_TRACE, REGION_TRACE_BATTELY); \
})

#define FASTBOOT_DRIVER_TRACE(name_) ({\
    struct lites_trace_header trc_hdr = { \
        .level = LOG_TRACE_DEBUG, \
        .trace_no = LOG_TRACE_NO_FASTBOOT_KERNEL, \
        .trace_id = 0, \
        .format_id = LOG_TRACE_FMT_DRIVER_SUSPEND, \
    }; \
    struct lites_trace_format trc_fmt = { \
        .buf = name_, \
        .count = strlen(name_), \
        .trc_header= &trc_hdr, \
    }; \
    lites_trace_write(REGION_TRACE_BATTELY, &trc_fmt); \
})

#endif /* BU_TRACE_H */

