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
*/

#ifndef	CMN_TYPES_H
#define	CMN_TYPES_H	1
typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned long	DWORD;
typedef unsigned char	UCHAR;
typedef unsigned short	USHORT;
typedef unsigned long	ULONG;
typedef unsigned char	BOOL8;
#ifndef VOID	
typedef	void			VOID;
typedef signed char		CHAR;
typedef signed short	SHORT;
typedef signed long		LONG;
#endif
#ifndef	NULL
#ifdef __cplusplus
#define	NULL	0
#else
#define	NULL	((VOID *)0)
#endif
#endif
#endif	
