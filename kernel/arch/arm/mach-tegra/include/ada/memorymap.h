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
 *
 */

#ifndef __ADA_MEMORYMAP_H__
#define __ADA_MEMORYMAP_H__

/* FUJITSU TEN:2013-04-16 mod start */
//#define MEMMAP_DIAGLOG_ADDR			0xBF400000
#define MEMMAP_DIAGLOG_ADDR			0xBF300000
/* FUJITSU TEN:2013-04-16 mod end */
#define MEMMAP_DIAGLOG_SIZE			(8*1024*1024)
/* FUJITSU TEN:2013-04-16 mod start */
//#define MEMMAP_NONVOLATILE_ADDR		0xBFC00000
#define MEMMAP_NONVOLATILE_ADDR		0xBFB00000
/* FUJITSU TEN:2013-04-16 mod end */
#define MEMMAP_NONVOLATILE_SIZE		(2*1024*1024)

#endif //__ADA_MEMORYMAP_H__
