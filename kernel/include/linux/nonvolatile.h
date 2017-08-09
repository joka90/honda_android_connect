/* 
 * nonvolatile data READ/WRITE driver for Android
 *
 * Copyright(C) 2011,2012 FUJITSU LIMITED
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
// COPYRIGHT(C) FUJITSU TEN LIMITED 2012,2013
/*----------------------------------------------------------------------------*/

#ifndef _NONVOLATILE_COMMON_H
#define _NONVOLATILE_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <sys/types.h>
#endif

#include  <linux/nonvolatile_def.h>

#define	NONVOLATILE_SIZE_FIXED          0x1800
#define	NONVOLATILE_SIZE_VARIABLE       0x1800
#define	NONVOLATILE_SIZE_AK_FIXED       0x7A00
#define	NONVOLATILE_SIZE_AK_VARIABLE    0x3200
#define	NONVOLATILE_SIZE_AK_DSP         0x400

#define	NONVOLATILE_SIZE                (NONVOLATILE_SIZE_FIXED \
                                       + NONVOLATILE_SIZE_VARIABLE \
                                       + NONVOLATILE_SIZE_AK_FIXED \
                                       + NONVOLATILE_SIZE_AK_VARIABLE \
                                       + NONVOLATILE_SIZE_AK_DSP)

#define NONVOLATILE_SIZE_REFRESH        0x7800
#define NONVOLATILE_SIZE_NONACCESS      (0x100000 - NONVOLATILE_SIZE)
#define NONVOLATILE_SIZE_ALL            (NONVOLATILE_SIZE \
                                       + NONVOLATILE_SIZE_NONACCESS \
                                       + NONVOLATILE_SIZE_REFRESH)

#define NONVOLATILE_DRV_NAME	"/dev/nonvolatile"

/*------------------------------------------------------------------*/
// Non volatile operation data
/*------------------------------------------------------------------*/
typedef struct nonvolatile_data {
  uint8_t* iBuff;			//Data storage address
  unsigned int iOffset;		//Data offset
  unsigned int iSize;		//Data size
} nonvolatile_data_t;

#define IOC_MAGIC 'w'


#ifdef __KERNEL__
int GetNONVOLATILE(uint8_t *, unsigned int, unsigned int);
int SetNONVOLATILE(uint8_t *, unsigned int, unsigned int);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* _NONVOLATILE_COMMON_H */
