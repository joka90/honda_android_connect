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

#ifndef	_LINUX_IF_H_
#define	_LINUX_IF_H_
#include	<stdio.h>
typedef	int	ID;
typedef	int	ER;
typedef	int	BOOL;
typedef	int				INT;
typedef	unsigned int	UINT;
#if 0
typedef	long			LONG;
typedef	unsigned long	ULONG;
typedef	short			SHORT;
typedef	unsigned short	USHORT;
#endif
typedef struct tagMSG {
	struct tagMSG *msg_header;
	DWORD			dwRFU;
} T_MSG;
typedef	int ATR;
#define	E_OK		0		
#define E_SYS		(-5)		
#define	E_NOMEM		(-10)		
#define	E_NOSPT		(-17)		
#define	E_INOSPT	(-18)		
#define	E_RSFN		(-20)		
#define	E_RSATR		(-24)		
#define	E_PAR		(-33)		
#define	E_ID		(-35)		
#define	E_NOEXS		(-52)		
#define	E_OBJ		(-63)		
#define	E_MACV		(-65)		
#define	E_OACV		(-66)		
#define	E_CTX		(-69)		
#define	E_QOVR		(-73)		
#define	E_DLT		(-81)		
#define	E_TMOUT		(-85)		
#define	E_RLWAI		(-86)		
#define	EN_NOND		(-113)		
#define	EN_OBJNO	(-114)		
#define	EN_PROTO	(-115)		
#define	EN_RSFN		(-116)		
#define	EN_COMM		(-117)		
#define	EN_RLWAI	(-118)		
#define	EN_PAR		(-119)		
#define	EN_RPAR		(-120)		
#define	EN_CTXID	(-121)		
#define	EN_EXEC		(-122)		
#define	EN_NOSPT	(-123)		
#define EV_FULL		(-225)		
#define	TA_TFIFO	1
#ifndef	TRUE
#define	TRUE	1
#endif
#ifndef	FALSE
#define	FALSE	0
#endif
#endif	
