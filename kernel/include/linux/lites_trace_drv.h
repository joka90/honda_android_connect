/* 
 * Device driver lites trace header
 *
 * Copyright(C) 2013 FUJITSU TEN LIMITED
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

#ifndef LITES_TRACE_DRV
#define LITES_TRACE_DRV

#include <linux/lites_trace.h>

#define KERNEL_TRACE_NO			LITES_KNL_LAYER
#define MXT_DRV_TRACE_NO		( KERNEL_TRACE_NO + 46 )
#define AK7736_DSP_DRV_TRACE_NO		( KERNEL_TRACE_NO + 17 )
#define TAG206N_DRV_TRACE_NO		( KERNEL_TRACE_NO + 42 )
#define USBOCP_DRV_TRACE_NO		( KERNEL_TRACE_NO + 81 )
#define NONVOLATILE_DRV_TRACE_NO	( KERNEL_TRACE_NO + 60 )
#define TMP401_DRV_TRACE_NO		( KERNEL_TRACE_NO + 90 )
#define GPS_ANT_DRV_TRACE_NO		( KERNEL_TRACE_NO + 91 )
/* FUJITSU TEN:2013-06-27 debug function add. start */
#define USB_DRV_TRACE_NO     ( KERNEL_TRACE_NO + 5 )
/* FUJITSU TEN:2013-06-27 debug function add. end */

#define DRV_DRC_ERR( group_id, drv_no, error_id, buf0, buf1, buf2, buf3 ) { \
	struct lites_trace_format trace_data = { 0 }; \
	struct lites_trace_header header = { 0 }; \
	unsigned short level = 3; \
	unsigned int set_buf[ 5 ] = { 0 }; \
\
	header.level = level; \
	header.trace_no = drv_no; \
	header.trace_id = 0; \
	header.format_id = 1; \
	trace_data.trc_header = &header; \
\
	set_buf[ 0 ] = error_id; \
	set_buf[ 1 ] = buf0; \
	set_buf[ 2 ] = buf1; \
	set_buf[ 3 ] = buf2; \
	set_buf[ 4 ] = buf3; \
	trace_data.buf = &set_buf[ 0 ]; \
	trace_data.count = sizeof( set_buf ); \
	lites_trace_write( group_id, &trace_data ); \
}

#define DRV_DRC_TRC( drv_no, msg ) {\
	struct lites_trace_format trace_data = { 0 }; \
	struct lites_trace_header header = { 0 }; \
	unsigned short level = 3; \
\
	header.level = level; \
	header.trace_no = drv_no; \
	header.trace_id = 0; \
	header.format_id = 1; \
	trace_data.trc_header = &header; \
\
	trace_data.buf = &msg; \
	trace_data.count = strlen( msg ); \
\
	lites_trace_write( REGION_TRACE_DRIVER, &trace_data ); \
}

#define MXT_DRC_ERR( error_id, buf0, buf1, buf2, buf3 ) { \
	DRV_DRC_ERR( REGION_TRACE_ERROR, MXT_DRV_TRACE_NO, \
		error_id, buf0, buf1, buf2, buf3 ); \
}
#define MXT_DRC_HARD_ERR( error_id, buf0, buf1, buf2, buf3 ) { \
	DRV_DRC_ERR( REGION_TRACE_HARD_ERROR, MXT_DRV_TRACE_NO, \
		error_id, buf0, buf1, buf2, buf3 ); \
}
#define MXT_DRC_TRC( msg ) { \
	DRV_DRC_TRC( MXT_DRV_TRACE_NO, msg ); \
}
#define TAG206N_DRC_ERR( error_id, buf0, buf1, buf2, buf3 ) { \
	DRV_DRC_ERR( REGION_TRACE_ERROR, TAG206N_DRV_TRACE_NO, \
		error_id, buf0, buf1, buf2, buf3 ); \
}
#define TAG206N_DRC_HARD_ERR( error_id, buf0, buf1, buf2, buf3 ) { \
	DRV_DRC_ERR( REGION_TRACE_HARD_ERROR, TAG206N_DRV_TRACE_NO, \
		error_id, buf0, buf1, buf2, buf3 ); \
}
#define TAG206N_DRC_TRC( msg ) { \
	DRV_DRC_TRC( TAG206N_DRV_TRACE_NO, msg ); \
}
#define AK7736_DRC_ERR( error_id, buf0, buf1, buf2, buf3 ) { \
	DRV_DRC_ERR( REGION_TRACE_ERROR, AK7736_DSP_DRV_TRACE_NO, \
		error_id, buf0, buf1, buf2, buf3 ); \
}
#define AK7736_DRC_HARD_ERR( error_id, buf0, buf1, buf2, buf3 ) { \
	DRV_DRC_ERR( REGION_TRACE_HARD_ERROR, AK7736_DSP_DRV_TRACE_NO, \
		error_id, buf0, buf1, buf2, buf3 ); \
}
#define AK7736_DRC_TRC( msg ) { \
	DRV_DRC_TRC( AK7736_DSP_DRV_TRACE_NO, msg ); \
}
#define USBOCP_DRC_HARD_ERR( error_id, buf0, buf1, buf2, buf3 ) { \
	DRV_DRC_ERR( REGION_TRACE_HARD_ERROR, USBOCP_DRV_TRACE_NO, \
		error_id, buf0, buf1, buf2, buf3 ); \
}
#define USBOCP_DRC_TRC( msg ) { \
	DRV_DRC_TRC( USBOCP_DRV_TRACE_NO, msg ); \
}
#define NONVOL_DRC_TRC( msg ) { \
	DRV_DRC_TRC( NONVOLATILE_DRV_TRACE_NO, msg ); \
}
/* FUJITSU TEN:2013-06-27 debug function add. start */
#define USB_DRC_TRC( msg ) { \
	DRV_DRC_TRC( USB_DRV_TRACE_NO, msg ); \
}
/* FUJITSU TEN:2013-06-27 debug function add. end */
#endif

