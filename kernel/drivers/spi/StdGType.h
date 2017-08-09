/*--------------------------------------------------------------------------*/
/* COPYRIGHT(C) FUJITSU TEN LIMITED 2012-2014                               */
/*--------------------------------------------------------------------------*/
/****************************************************************************
*																			*
*		シリーズ名称	: ＦＴ－ＩＰＡＳシリーズ							*
*		マイコン名称	: 共通												*
*		Ｏ．Ｓ．名称	: 共通												*
*		ブロック名称	: 共通												*
*		DESCRIPTION		: 標準タイプ定義									*
*																			*
*		ファイル名称	: StdGType.h										*
*		作	 成	  者	: MICWARE CO.,LTD.									*
*		作　 成　 日	: 2005/11/01	<Ver. 3.00.00>					   	*
*		改　 訂　 日	: 2006/06/30	<Ver. 3.10.00>						*
*																			*
****************************************************************************/
#ifndef	__StdGType_H__
#define	__StdGType_H__
///#include	<string.h>
///#include	<stddef.h>
/************************************************************************
*																		*
*			Ｄefine Ｄeclare Ｓection									*
*																		*
************************************************************************/
/*----------------------------------------------*/
/*	共通型定義									*/
/*----------------------------------------------*/
#ifndef		BYTE
#define		BYTE			unsigned char
#endif
#ifndef		WORD
#define		WORD			unsigned short
#endif
#ifndef		DWORD
#define		DWORD			unsigned long
#endif

#ifndef		uchar
#define		uchar			unsigned char
#endif
#ifndef		ushort
#define		ushort			unsigned short
#endif
#ifndef		uint
#define		uint			unsigned int
#endif
#ifndef		ulong
#define		ulong			unsigned long
#endif

#ifndef		INT
#define		INT				signed int
#endif

#ifndef		UINT
#define		UINT			unsigned int
#endif

#ifndef		_INT
#define		_INT			signed int
#endif
#ifndef		_UINT
#define		_UINT			unsigned int
#endif
#ifndef		UINT8
#define		UINT8			unsigned char
#endif
#ifndef		UINT16
#define		UINT16			unsigned short
#endif
#ifndef		UINT32
#define		UINT32			unsigned int
#endif

#ifndef		INT8
#define		INT8			signed char
#endif
#ifndef		INT16
#define		INT16			signed short
#endif
#ifndef		INT32
#define		INT32			signed int
#endif

#ifndef		UCHAR
#define		UCHAR			unsigned char
#endif
#ifndef		USHORT
#define		USHORT			unsigned short
#endif
#ifndef		ULONG
#define		ULONG			unsigned long
#endif
#ifndef		VOID
#define		VOID			void
#endif

#ifndef		CHAR
#define		CHAR			signed char
#endif
#ifndef		SHORT
#define		SHORT			signed short
#endif
#ifndef		LONG
#define		LONG			signed long
#endif

#ifndef		LONGLONG
#ifdef	WIN32
#define		LONGLONG		__int64
#else
#define		LONGLONG		long long
#endif
#endif

/*----------------------------------------------*/
/*	標準定数定義								*/
/*----------------------------------------------*/
#define		FOREVER			1

#define		STS_OK			0
#define		STS_NG			1

#define		OFF				0
#define		ON				1

#define		LOW				0
#define		HIGH			1

#define		NONE			0
#define		EXIST			1

#ifndef		TRUE
#define		TRUE			1
#endif
#ifndef		FALSE
#define		FALSE			0
#endif

#define		BIT_D00			0x1
#define		BIT_D01			0x2
#define		BIT_D02			0x4
#define		BIT_D03			0x8
#define		BIT_D04			0x10
#define		BIT_D05			0x20
#define		BIT_D06			0x40
#define		BIT_D07			0x80
#define		BIT_D08			0x100
#define		BIT_D09			0x200
#define		BIT_D10			0x400
#define		BIT_D11			0x800
#define		BIT_D12			0x1000
#define		BIT_D13			0x2000
#define		BIT_D14			0x4000
#define		BIT_D15			0x8000
#define		BIT_D16			0x10000
#define		BIT_D17			0x20000
#define		BIT_D18			0x40000
#define		BIT_D19			0x80000
#define		BIT_D20			0x100000
#define		BIT_D21			0x200000
#define		BIT_D22			0x400000
#define		BIT_D23			0x800000
#define		BIT_D24			0x1000000
#define		BIT_D25			0x2000000
#define		BIT_D26			0x4000000
#define		BIT_D27			0x8000000
#define		BIT_D28			0x10000000
#define		BIT_D29			0x20000000
#define		BIT_D30			0x40000000
#define		BIT_D31			0x80000000

/*------------------------------------------*/
/*	ブロック間 API callback type 			*/
/*------------------------------------------*/
typedef	LONG (*API_CALLBACK)(DWORD, DWORD, DWORD, LONG, void*);

#endif	/* __StdGType_H__	*/
/****************************************************************************
*						StdGType.h	END										*
****************************************************************************/
