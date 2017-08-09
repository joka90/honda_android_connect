/****************************************************************************
*																			*
*		シリーズ名称	: A-DA												*
*		マイコン名称	: tegra3											*
*		オーダー名称	: 													*
*		ブロック名称	: kernel driver										*
*		DESCRIPTION		: iPod認証ドライバヘッダ							*
*		ファイル名称	: spi_ipodauth.h									*
*																			*
*		Copyright(C) 2012 FUJITSU TEN LIMITED								*
*																			*
****************************************************************************/
#ifndef SPI_IPODAUTH_H
#define SPI_IPODAUTH_H

//#include <linux/types.h>
#define IPD_IOC_MAGIC			'i'


#define	IPD_IOC_READ		((IPD_IOC_MAGIC<<8) | 1)//_IOR(IPD_IOC_MAGIC, 1, unsigned char)
#define	IPD_IOC_WRITE		((IPD_IOC_MAGIC<<8) | 2)//_IOW(IPD_IOC_MAGIC, 1, unsigned char)
#define	IPD_IOC_ISSUSPEND	((IPD_IOC_MAGIC<<8) | 3)

/* Retuen code suspend-resume */
#define	IPD_PRE_SUSPEND_RETVAL		0x7FFF

/************************************************************************
*																		*
*		Ｓtruct Ｄeclare Ｓection										*
*																		*
************************************************************************/
/*----------------------------------------------*/
/*	ＩＯＣＴＬ　ＩＦ定義						*/
/*----------------------------------------------*/
typedef	struct	{
	unsigned char*	pucSndData;		/* 送信データ			*/
	unsigned long	ulSndLength;	/* 送信データサイズ		*/
} ST_KDV_IPD_IOC_WR;

typedef	struct	{
	unsigned char*	pucSndData;		/* 送信データ			*/
	unsigned long	ulSndLength;	/* 送信データサイズ		*/
	unsigned long	ulRcvLength;	/* 受信データサイズ		*/
	unsigned char*	pucRcvData;		/* 受信データ			*/
	unsigned long*	pulOutLength;	/* 受信サイズ			*/
} ST_KDV_IPD_IOC_RD;

#endif /* SPI_IPODAUTH_H */
