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
// COPYRIGHT(C) FUJITSU TEN LIMITED 2013
/*----------------------------------------------------------------------------*/

#include <stdarg.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/lites_trace.h>
#include <linux/lites_trace_wireless.h>

//#define LOCAL_DBG

typedef struct _lites_hard_err {
	unsigned int	err_id;
	unsigned int	param1;
	unsigned int	param2;
	unsigned int	param3;
	unsigned int	param4;
} lites_hard_err;

typedef struct _lites_err {
	unsigned int	err_id;
	unsigned int	param1;
	unsigned int	param2;
	unsigned int	param3;
	unsigned int	param4;
} lites_err;

typedef struct _lites_drv_trace_err {
	unsigned char	msg[36];
} lites_drv_trace_err;

/**
 * @brief       write hard error on driving recorder.
 * @param[in]   drvno: driver No.
 * @param[in]   level: log level.
 * @param[in]   errid: error level.
 * @param[in]   param1: parameter.
 * @param[in]   param2: parameter.
 * @return      return of lites_trace_write.
 */
int st_lites_harderr(const int drvno, const unsigned short level, const int errid, const int param1, const int param2)
{
	lites_hard_err errinfo;
	ssize_t size = 0;
	int result;
	struct lites_trace_header trc_header;
	struct lites_trace_format trc_format;
	unsigned int gid = REGION_TRACE_HARD_ERROR;
	unsigned short tid = 0;
	unsigned int tno = LITES_KNL_LAYER + drvno;

	errinfo.err_id = errid;
	errinfo.param1 = param1;
	errinfo.param2 = param2;
	errinfo.param3 = 0;
	errinfo.param4 = 0;

	size = sizeof(errinfo);
#ifdef LOCAL_DBG
	pr_err("st_lites_h_err: size=%d gid=%04X tid=%04X tno=%04X level=%d errid=%d param1=%d param2=%d",
		size, gid, tid, tno, level, errid, param1, param2);
#endif
	trc_header.level = level;
	trc_header.trace_id = tid;
	trc_header.trace_no = tno;
	trc_header.format_id = 1;
	trc_format.trc_header = &trc_header;
	trc_format.count = size;
	trc_format.buf = &errinfo;

	result = lites_trace_write(gid, &trc_format);
#ifdef LOCAL_DBG
	pr_err("st_lites_harderr = %d\n",result);
#endif
	return result;
}
EXPORT_SYMBOL(st_lites_harderr);

/**
 * @brief       write error on driving recorder.
 * @param[in]   drvno: driver No.
 * @param[in]   level: log level.
 * @param[in]   errid: error level.
 * @param[in]   param1: parameter.
 * @param[in]   param2: parameter.
 * @return      return of lites_trace_write.
 */
int st_lites_err(const int drvno, const unsigned short level, const int errid, const int param1, const int param2)
{
	lites_err errinfo;
	ssize_t size = 0;
	int result;
	struct lites_trace_header trc_header;
	struct lites_trace_format trc_format;
	unsigned int gid = REGION_TRACE_ERROR;
	unsigned short tid = 0;
	unsigned int tno = LITES_KNL_LAYER + drvno;

	errinfo.err_id = errid;
	errinfo.param1 = param1;
	errinfo.param2 = param2;
	errinfo.param3 = 0;
	errinfo.param4 = 0;

	size = sizeof(errinfo);
#ifdef LOCAL_DBG
	pr_err("st_lites_s_err: size=%d gid=%04X tid=%04X tno=%04X level=%d errid=%d param1=%d param2=%d",
		size, gid, tid, tno, level, errid, param1, param2);
#endif
	trc_header.level = level;
	trc_header.trace_id = tid;
	trc_header.trace_no = tno;
	trc_header.format_id = 1;
	trc_format.trc_header = &trc_header;
	trc_format.count = size;
	trc_format.buf = &errinfo;

	result = lites_trace_write(gid, &trc_format);
#ifdef LOCAL_DBG
	pr_err("st_lites_err = %d\n",result);
#endif
	return result;
}
EXPORT_SYMBOL(st_lites_err);

/**
 * @brief       write driver trace log on driving recorder.
 * @param[in]   drvno: Driver No.
 * @param[in]   level: log level.
 * @param[in]   fmt: output format.
 * @param[in]   args: parameter for fmt.
 * @return      return of lites_trace_write.
 */
static int st_lites_drv_trace_log_native(const int drvno, const unsigned short level, const char *fmt, va_list args)
{
	lites_drv_trace_err errinfo;
	ssize_t size = 0;
	int result;
	struct lites_trace_header  trc_header;
	struct lites_trace_format  trc_format;
	unsigned int gid = REGION_TRACE_DRIVER;
	unsigned short tid = 0;
	unsigned int tno = LITES_KNL_LAYER + drvno;

	memset(errinfo.msg, 0, sizeof(errinfo.msg));
	size = vscnprintf(errinfo.msg, sizeof(errinfo.msg), fmt, args);
	size = size > sizeof(errinfo.msg) ? sizeof(errinfo.msg) : size;
#ifdef LOCAL_DBG
	pr_err("st_drvtrerr: size=%d gid=%d tid=%d tno=%d level=%d msg=%s",
		size, gid, tid, tno, level, errinfo.msg);
#endif
	trc_header.level = level;
	trc_header.trace_id = tid;
	trc_header.trace_no = tno;
	trc_header.format_id = 1;
	trc_format.trc_header = &trc_header;
	trc_format.count = size+1;
	trc_format.buf = &errinfo;

	result = lites_trace_write(gid, &trc_format);
#ifdef LOCAL_DBG
	pr_err("st_lites_drv_trace_log_native = %d\n",result);
#endif
	return result;
}

/**
 * @brief       write driver trace log on driving recorder.
 * @param[in]   drvno: Driver No.
 * @param[in]   level: log level.
 * @param[in]   fmt: output format.
 * @return      return of st_lites_drv_trace_log_native.
 */
int st_lites_drv_trace_log(const int drvno, const unsigned short level, const char *fmt, ...)
{
	int ret;

	va_list args;
	va_start(args, fmt);
	ret = st_lites_drv_trace_log_native(drvno, level, fmt, args);
	va_end(args);
	return ret;
}
EXPORT_SYMBOL(st_lites_drv_trace_log);

/**
 * @brief       write BTdrv log on driving recorder.
 * @param[in]   drvno: Driver No.
 * @param[in]   level: log level.
 * @param[in]   errid: error level.
 * @param[in]   param1: parameter.
 * @return      return of lites_trace_write.
 */
int st_lites_evt(const int drvno, const unsigned short level, const int errid, const int param1)
{
	lites_hard_err errinfo;
	ssize_t size = 0;
	int result;
	struct lites_trace_header trc_header;
	struct lites_trace_format trc_format;
	unsigned int gid = REGION_EVENT_LOG;
	unsigned short tid = 0;
	unsigned int tno = LITES_KNL_LAYER + drvno;	

	switch(drvno) {
	case WL128X_WIFI_DRIVER:
		tid = REGION_EVENT_WIFI;
		break;
	case BLUETOOTH_DRIVER:
		tid = REGION_EVENT_BLUETOOTH_DRV;
		break;
	case USB_DRIVER:
		tid = REGION_EVENT_USB_DRV;
		break;
	default:
		tid = REGION_EVENT_WIFI;
		break;
	}

	errinfo.err_id = errid;
	errinfo.param1 = param1;
	errinfo.param2 = 0;
	errinfo.param3 = 0;
	errinfo.param4 = 0;

	size = sizeof(errinfo);
#ifdef LOCAL_DBG
	pr_err("st_lites_btdrv: size=%d gid=%04X tid=%04X tno=%04X level=%d errid=%d param1=%d",
		size, gid, tid, tno, level, errid, param1);
#endif
	trc_header.level = level;
	trc_header.trace_id = tid;
	trc_header.trace_no = tno;
	trc_header.format_id = 1;
	trc_format.trc_header = &trc_header;
	trc_format.count = size;
	trc_format.buf = &errinfo;

	result = lites_trace_write(gid, &trc_format);
#ifdef LOCAL_DBG
	pr_err("wl_lites_wifi = %d\n",result);
#endif
	return result;
}
EXPORT_SYMBOL(st_lites_evt);

