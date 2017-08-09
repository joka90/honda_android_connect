/****************************************************************************
*																			*
*		�V���[�Y����	: A-DA												*
*		�}�C�R������	: tegra3											*
*		�I�[�_�[����	: 													*
*		�u���b�N����	: kernel driver										*
*		DESCRIPTION		: iPod�F�؃h���C�o�w�b�_							*
*		�t�@�C������	: spi_ipodauth.h									*
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
*		�rtruct �ceclare �rection										*
*																		*
************************************************************************/
/*----------------------------------------------*/
/*	�h�n�b�s�k�@�h�e��`						*/
/*----------------------------------------------*/
typedef	struct	{
	unsigned char*	pucSndData;		/* ���M�f�[�^			*/
	unsigned long	ulSndLength;	/* ���M�f�[�^�T�C�Y		*/
} ST_KDV_IPD_IOC_WR;

typedef	struct	{
	unsigned char*	pucSndData;		/* ���M�f�[�^			*/
	unsigned long	ulSndLength;	/* ���M�f�[�^�T�C�Y		*/
	unsigned long	ulRcvLength;	/* ��M�f�[�^�T�C�Y		*/
	unsigned char*	pucRcvData;		/* ��M�f�[�^			*/
	unsigned long*	pulOutLength;	/* ��M�T�C�Y			*/
} ST_KDV_IPD_IOC_RD;

#endif /* SPI_IPODAUTH_H */
