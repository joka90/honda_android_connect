/**
 * Copyright (c) 2011-2012 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 *
 * @file ubinux_panic_info.h
 *
 * @brief PANIC発生時の処理にて必要な構造体や値を定義する。
 *
 */
#ifndef __UBINUX_PANIC_INFO_H
#define __UBINUX_PANIC_INFO_H


/**
 * @brief ubinux_kernel_panic_info.flagsに入れるフラグ。
 *	各情報を格納したときに設定する。
 */
#define	UBINUX_TIMESTAMP_AVAILABLE					0x00000001
#define	UBINUX_PANIC_MESSAGE_AVAILABLE				0x00000002	// printk_region
#define	UBINUX_KERNEL_STACK_AVAILABLE				0x00000004
#define	UBINUX_MMIO_REGISTER_AVAILABLE				0x00000008
#define	UBINUX_PANIC_MESSAGE_NO_TIMESTAMP_AVAILABLE	0x00000010	// printk_buffer
#define	UBINUX_GENERAL_REGISTER_AVAILABLE			0x00000020


/**
 * @brief PANICメッセージを保存するときに一緒に保存するレジスタを定義する。
 *	レジスタを読み込むときのためのMAP空間をリージョンとして定義する。
 */
typedef struct ubinux_register_region {
	unsigned long	phys_addr;	/**< MAPリージョンの先頭アドレス */
	unsigned long	size;		/**< MAPリージョンのサイズ */
} UBINUX_REGISTER_REGION;


/**
 * @brief PANICメッセージを保存するときに一緒に保存するレジスタを定義する。
 */
typedef struct ubinux_register_def {
	unsigned long	region;			/**< MAPリージョンの番号 */
	unsigned long	offset;			/**< リージョンの先頭からレジスタまでのオフセット */
	unsigned long	access_byte;	/**< レジスタへアクセスする幅(1 or 2 or 4) */
	unsigned long	copy_length;	/**< access_byteでコピーする領域サイズ */
} UBINUX_REGISTER_DEF;


#endif	// __UBINUX_PANIC_INFO_H
