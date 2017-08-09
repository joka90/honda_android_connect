/**
 * Copyright (c) 2011-2012 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 *
 * @file ubinux_panic_info_customized.h
 *
 * @brief モデル別のPANIC情報の構造体や採取するレジスタ等を定義する。
 *
 */
#ifndef __UBINUX_PANIC_INFO_CUSTOMIZED_H
#define __UBINUX_PANIC_INFO_CUSTOMIZED_H


#ifdef __KERNEL__
#include <linux/time.h>
#else /* __KERNEL__ */
#include <linux/lites_trace.h>
#include <sys/time.h>
#endif /* __KERNEL__ */

/**
 * @enum FTEN_CONFIG_MODEL_ID
 * @brief モデルIDの定義
 *
 * <pre>
 * モデル別にkernel panic情報の領域サイズを変更する場合は、
 * 　・FTEN_CONFIG_MODEL_ID列挙体にモデルを追加
 * 　・モデル専用のサイズと構造体を追加(ubinux_panic_info_model_dep.h)
 * が必要になる </pre>
 */
enum FTEN_CONFIG_MODEL_ID {
	FTEN_CONFIG_MODEL_ID_COMMON = 1,

	FTEN_CONFIG_MODEL_ID_fe_avn_jp_bb,
	FTEN_CONFIG_MODEL_ID_fe_avn_jp_bb_mini,

	FTEN_CONFIG_MODEL_ID_ft_avn_adc_1m,
	FTEN_CONFIG_MODEL_ID_ft_avn_all_1m,
	FTEN_CONFIG_MODEL_ID_ft_avn_all_1s,
	FTEN_CONFIG_MODEL_ID_ft_avn_all_2s,

	FTEN_CONFIG_MODEL_ID_ft_avn_mini_1m,
	FTEN_CONFIG_MODEL_ID_ft_avn_mini_2s,
	FTEN_CONFIG_MODEL_ID_ft_avn_wifi_1m,

	FTEN_CONFIG_MODEL_ID_ft_avn_xm_1m,
	FTEN_CONFIG_MODEL_ID_ft_da_adc_1m,
	FTEN_CONFIG_MODEL_ID_ft_da_all_1m,
	FTEN_CONFIG_MODEL_ID_ft_da_all_1s,
	FTEN_CONFIG_MODEL_ID_ft_da_all_2s,
	FTEN_CONFIG_MODEL_ID_ft_da_mini_1m,
	FTEN_CONFIG_MODEL_ID_ft_da_mini_2s,
	FTEN_CONFIG_MODEL_ID_ft_da_none_1m,


	/* モデル追加時は、末尾にモデルIDを追加すること */
};

/** kernel panic情報ヘッダ部の構造体 */
typedef struct ubinux_kernel_panic_info_header {
	u32 model_id;						/**< モデルID */
	u32 format_version;					/**< フォーマットVersion */
	u32 status;							/**< ステータス */
	u32 boot_id;						/**< BootID */
	u64 timestamp;						/**< タイムスタンプ */
	struct timeval tv;					/**< 時刻情報 timeval構造体 */
	struct timezone tz;					/**< タイムゾーン timezone構造体 */
	u32 rsv[2];	 						/**< リザーブ */
} UBINUX_KERNEL_PANIC_INFO_HEADER;


/** NOPの値定義 */
#define UBINUX_PANIC_INFO_NOP			0x0


/***********************************************
 * モデル定義(COMMON)
 *
 ***********************************************/

/** カーネルパニック情報内の領域サイズ定義 */
#define UBINUX_PANIC_MSG_SIZE_COMMON		1024
#define UBINUX_KERNEL_STACK_SIZE_COMMON		8192
#define UBINUX_MMIO_REGISTER_SIZE_COMMON	4096

/** kernel panic情報構造体 */
typedef struct ubinux_kernel_panic_info_common {
	u32 magic_no;														/**< マジックナンバー */
	u32 rsv[3];															/**< リザーブ */
	UBINUX_KERNEL_PANIC_INFO_HEADER header;								/**< kernel panic情報ヘッダ部 */
	u32 general_registers[16];											/**< 汎用レジスタ */
	char panic_msg[UBINUX_PANIC_MSG_SIZE_COMMON];						/**< panicメッセージ */
	char kernel_stack[UBINUX_KERNEL_STACK_SIZE_COMMON];					/**< kernel stack */
	u32 mmio_registers[UBINUX_MMIO_REGISTER_SIZE_COMMON / sizeof(u32)];	/**< MMIOレジスタ */
	u32 tail_magic_no;													/**< 末尾マジックナンバー */
} UBINUX_KERNEL_PANIC_INFO_COMMON;


/**
 * @brief モデル定義のフォーマットバージョンの定義
 * モデル定義内を変更したとき、値を変更すること。
 * modelのenum種別や領域サイズが変更された場合へ変更すること。
 */
#define FTEN_CONFIG_UBINUX_PANIC_INFO_FORMAT_VER_COMMON		0x00000002

/**
 * @brief リージョン定義
 *	UBINUX_REGISTER_REGION構造体の中身。
 *	配列になるよう定義する。indexがリージョン番号となる。 
 */
/*      { phys_addr,	size       }						*/
#define FTEN_CONFIG_UBINUX_REGISTER_REGION_LIST_COMMON { \
		{ 0x16000000,	0x00001000 },	/* region 0 IO-ASIC */ \
\
		{ 0xFEC10000,	0x00000234 },	/* region 1 BSCレジスタ設定 */ \
\
		{ 0xC12A0000,	0x000000FC },	/* region 2 DBSCレジスタ設定 part1 */ \
		{ 0xE6058000,	0x00000040 },	/* region 3 DBSCレジスタ設定 part2 */ \
		{ 0xFE400000,	0x00000308 },	/* region 4 DBSCレジスタ設定 part3 */ \
\
		{ 0xE6150000,	0x00000148 },	/* region 5 CPGレジスタ設定 part1 */ \
		{ 0xFFF20000,	0x00000018 },	/* region 6 CPGレジスタ設定 part2 */ \
		{ 0xE6C20000,	0x00000018 },	/* region 7 CPGレジスタ設定 part3 */ \
		{ 0xFE940000,	0x00004418 },	/* region 8 CPGレジスタ設定 part3 */ \
		{ 0xFE3C0000,	0x00000614 },	/* region 9 CPGレジスタ設定 part4 */ \
		{ 0xA4540000,	0x000000CC },	/* region 10 CPGレジスタ設定 part5 */ \
		{ 0xE6C40000,	0x0000000C },	/* region 11 CPGレジスタ設定 part6 */ \
		{ 0xE6C50000,	0x0000000C },	/* region 12 CPGレジスタ設定 part6 */ \
		{ 0xE6C60000,	0x0000000C },	/* region 13 CPGレジスタ設定 part6 */ \
		{ 0xE6C80000,	0x0000000C },	/* region 14 CPGレジスタ設定 part6 */ \
		{ 0xE6CB0000,	0x0000000C },	/* region 15 CPGレジスタ設定 part6 */ \
		{ 0xE6CC0000,	0x0000000C },	/* region 16 CPGレジスタ設定 part6 */ \
\
		{ 0xE6180000,	0x00008024 },	/* region 17 SYSCレジスタ設定 */ \
\
		{ 0xFE910000,	0x000020AC },	/* region 18 CEUレジスタ設定 */ \
\
		{ 0xFE3C0000,	0x00000614 },	/* region 19 FSI_FMSIレジスタ設定 part1 */ \
		{ 0xFE1F8000,	0x00000004 },	/* region 20 FSI_FMSIレジスタ設定 part2 */ \
\
		{ 0xE6058100,	0x00000088 },	/* region 21 GPIOレジスタ設定 */ \
\
		{ 0xE6900000,	0x00000100 },	/* region 22 INTCAレジスタ設定(H'E690 0000) */ \
		{ 0xE6940000,	0x00000200 },	/* region 23 INTCAレジスタ設定(H'E694 0000) */ \
		{ 0xE6950000,	0x00000200 },	/* region 24 INTCAレジスタ設定(H'E695 0000) */ \
		{ 0xE6980000,	0x00000040 },	/* region 25 INTCAレジスタ設定(H'E698 0000) */ \
		{ 0xE69C0000,	0x00000004 },	/* region 26 INTCAレジスタ設定(H'E69C 0000) */ \
\
		{ 0xE6058000,	0x00000060 },	/* region 27 INTCSレジスタ設定(H'E605 8000) */ \
		{ 0xFFD10000,	0x00000070 },	/* region 28 INTCSレジスタ設定(H'FFD1 0000) */ \
		{ 0xFFD20000,	0x00000200 },	/* region 29 INTCSレジスタ設定(H'FFD2 0000) */ \
		{ 0xFFD30000,	0x000000D0 },	/* region 30 INTCSレジスタ設定(H'FFD3 0000) */ \
		{ 0xFFD40000,	0x00000010 },	/* region 31 INTCSレジスタ設定(H'FFD4 0000) */ \
		{ 0xFFD50000,	0x00000200 },	/* region 32 INTCSレジスタ設定(H'FFD5 0000) */ \
	}

/**
 * @brief レジスタ定義
 *	UBINUX_REGISTER_DEF構造体の中身。
 *	配列になるよう定義する。
 */
/*      { 						access,		copy	 }		*/
/*      { region,	offset,		byte		length	 }		*/
#define FTEN_CONFIG_UBINUX_REGISTER_DEF_LIST_COMMON	{ \
		{ 0,		0x800,		0x2,		0x14	 },	/* IO-ASIC SCIFA0 */ \
		{ 0,		0x820,		0x2,		0x14	 },	/* IO-ASIC SCIFA1 */ \
		{ 0,		0x840,		0x2,		0x14	 },	/* IO-ASIC SCIFA2 */ \
		{ 0,		0x860,		0x2,		0x14	 },	/* IO-ASIC SCIFA3 */ \
		{ 0,		0x880,		0x2,		0x14	 },	/* IO-ASIC SCIFA4 */ \
		{ 0,		0x8A0,		0x2,		0x14	 },	/* IO-ASIC SCIFA5 */ \
		{ 0,		0x8C0,		0x2,		0x14	 },	/* IO-ASIC SCIFA6 */ \
		{ 0,		0x8E0,		0x2,		0x14	 },	/* IO-ASIC SCIFA7 */ \
		{ 0,		0x900,		0x2,		0x14	 },	/* IO-ASIC SCIFA8 */ \
		{ 0,		0x920,		0x2,		0x14	 },	/* IO-ASIC SCIFA9 */ \
		{ 0,		0x940,		0x2,		0x14	 },	/* IO-ASIC ステアリングセンサ */ \
		{ 0,		0x960,		0x2,		0x1C	 },	/* IO-ASIC IrDA */ \
		{ 0,		0x980,		0x2,		0x18	 },	/* IO-ASIC GPIO_A～D（ピンマルチ） */ \
		{ 0,		0x9A0,		0x2,		0x18	 },	/* IO-ASIC GPIO_E～H（ピンマルチ） */ \
		{ 0,		0x9C0,		0x2,		0x18	 },	/* IO-ASIC ROMデコーダ */ \
		{ 0,		0x9E0,		0x2,		0x20	 },	/* IO-ASIC CONFIG（設定レジスタ） */ \
		{ 0,		0xA00,		0x2,		0x26	 },	/* IO-ASIC ADC制御 */ \
		{ 0,		0xA40,		0x2,		0x20	 },	/* IO-ASIC リモコン */ \
		{ 0,		0xA80,		0x2,		0x2A	 },	/* IO-ASIC タッチSW */ \
		{ 0,		0xB00,		0x2,		0x12	 },	/* IO-ASIC I2C0 */ \
		{ 0,		0xB40,		0x2,		0x12	 },	/* IO-ASIC I2C1 */ \
		{ 0,		0xB80,		0x2,		0x12	 },	/* IO-ASIC I2C2 */ \
		{ 0,		0xBC0,		0x2,		0x14	 },	/* IO-ASIC SCIFA_10 */ \
		{ 0,		0xBE0,		0x2,		0x10	 },	/* IO-ASIC ロータリーエンコーダ */ \
		{ 0,		0xC00,		0x2,		0x20	 },	/* IO-ASIC INTC_H */ \
		{ 0,		0xC20,		0x2,		0x20	 },	/* IO-ASIC INTC_M */ \
		{ 0,		0xC40,		0x2,		0x20	 },	/* IO-ASIC INTC_L */ \
		{ 0,		0xC60,		0x2,		0x20	 },	/* IO-ASIC PWM */ \
		{ 0,		0xC80,		0x2,		0x20	 },	/* IO-ASIC CKG */ \
		{ 0,		0xCA0,		0x2,		0x10	 },	/* IO-ASIC 車速パルス */ \
		{ 0,		0xCB0,		0x2,		0x8		 },	/* IO-ASIC 音声認識A/D-I/F */ \
		{ 0,		0xCC0,		0x2,		0x10	 },	/* IO-ASIC キースキャン */ \
		{ 0,		0xD00,		0x2,		0x72	 },	/* IO-ASIC SD-IF */ \
		{ 0,		0xD80,		0x2,		0x40	 },	/* IO-ASIC AVCLAN */ \
		{ 0,		0xDC0,		0x2,		0x34	 },	/* IO-ASIC 画質調整 */ \
		{ 0,		0xF80,		0x2,		0x38	 },	/* IO-ASIC シリアル３線音声IF */ \
		{ 0,		0xFC0,		0x2,		0x20	 },	/* IO-ASIC ピンマルチ */ \
\
		{ 1,		0x0000,		0x4,		0x8		 },	/* CMNCR - CS0BCR */ \
		{ 1,		0x0010,		0x4,		0x4		 },	/* CS4BCR */ \
		{ 1,		0x0018,		0x4,		0x8		 },	/* CS5BBCR - CS6ABCR */ \
		{ 1,		0x0024,		0x4,		0x4		 },	/* CS0WCR */ \
		{ 1,		0x0030,		0x4,		0x4		 },	/* CS4WCR */ \
		{ 1,		0x0038,		0x4,		0x8		 },	/* CS5BWCR - CS6AWCR */ \
		{ 1,		0x0090,		0x4,		0x84	 },	/* BPTCR00 - BPTCR31, BSWCR */ \
\
		{ 2,		0x0000,		0x4,		0x20	 },	/* FUNCCTRL - OUTCTRL */ \
		{ 2,		0x00E8,		0x4,		0x8		 },	/* DQCALOFS1 - DQCALOFS2 */ \
		{ 2,		0x00F8,		0x4,		0x4		 },	/* DQCALEXP */ \
		{ 3,		0x003C,		0x4,		0x4		 },	/* DDRPNCNT */ \
		{ 5,		0x00EC,		0x4,		0x4		 },	/* DDRVREFCNT */ \
		{ 4,		0x0010,		0x4,		0x8		 },	/* DBACEN(Enable) - DBRFEN(Enable) */ \
		{ 4,		0x0020,		0x4,		0x8		 },	/* DBKIND - DBCONF0 */ \
		{ 4,		0x0030,		0x4,		0x4		 },	/* DBPHYTYPE */ \
		{ 4,		0x0040,		0x4,		0xC		 },	/* DBTR0 - DBTR2 */ \
		{ 4,		0x0050,		0x4,		0x44	 },	/* DBTR3 - DBTR19 */ \
		{ 4,		0x00B0,		0x4,		0x4		 },	/* DBBL */ \
		{ 4,		0x00C0,		0x4,		0xC		 },	/* DBADJ0 - DBADJ2 */ \
		{ 4,		0x00E0,		0x4,		0x10	 },	/* DBRFCNF0 - DBRFCNF3 */ \
		{ 4,		0x00F4,		0x4,		0x8		 },	/* DBCALCNF - DBCALTR */ \
		{ 4,		0x0100,		0x4,		0x4		 },	/* DBRNK0 */ \
		{ 4,		0x0180,		0x4,		0x4		 },	/* DBPDNCNF */ \
		{ 4,		0x0240,		0x4,		0x8		 },	/* DBDFISTAT - DBDFICNT */ \
		{ 4,		0x0300,		0x4,		0x8		 },	/* DBBS0CNT0 - DBBS0CNT1 */ \
\
		{ 5,		0x0000,		0x4,		0x8		 },	/* FRQCRA - FRQCRB */ \
		{ 5,		0x00E0,		0x4,		0x8		 },	/* FRQCRC - FRQCRD */ \
		{ 5,		0x0010,		0x4,		0xC		 },	/* FMSICKCR - FSIACKCR */ \
		{ 5,		0x0068,		0x4,		0x4		 },	/* ZTRCKCR */ \
		{ 5,		0x0080,		0x4,		0x8		 },	/* SUBCKCR - SPUCKCR */ \
		{ 5,		0x008C,		0x4,		0x4		 },	/* USBCKCR */ \
		{ 5,		0x009C,		0x4,		0x4		 },	/* STPRCKCR */ \
		{ 5,		0x0020,		0x4,		0x10	 },	/* RTSTBCR - PLLC2CR */ \
		{ 5,		0x00C8,		0x4,		0x4		 },	/* PLLC01STPCR */ \
		{ 5,		0x0030,		0x4,		0x4		 },	/* MSTPSR0 */ \
		{ 5,		0x0038,		0x4,		0x4		 },	/* MSTPSR1 */ \
		{ 5,		0x0040,		0x4,		0x4		 },	/* MSTPSR2 */ \
		{ 5,		0x0048,		0x4,		0x4		 },	/* MSTPSR3 */ \
		{ 5,		0x004C,		0x4,		0x4		 },	/* MSTPSR4 */ \
		{ 5,		0x003C,		0x4,		0x4		 },	/* MSTPSR5 */ \
		{ 5,		0x0110,		0x4,		0x18	 },	/* RMSTPCR0 - RMSTPCR5 */ \
		{ 5,		0x0130,		0x4,		0x18	 },	/* SMSTPCR0 - SMSTPCR5 */ \
		{ 5,		0x00A0,		0x4,		0x4		 },	/* SRCR0 */ \
		{ 5,		0x00A8,		0x4,		0x4		 },	/* SRCR1 */ \
		{ 5,		0x00B0,		0x4,		0x4		 },	/* SRCR2 */ \
		{ 5,		0x00B8,		0x4,		0x4		 },	/* SRCR3 */ \
		{ 5,		0x00BC,		0x4,		0x4		 },	/* SRCR4 */ \
		{ 5,		0x00C4,		0x4,		0x4		 },	/* SRCR5 */ \
		{ 5,		0x0054,		0x4,		0x4		 },	/* ASTAT */ \
		{ 6,		0x000C,		0x1,		0x1		 },	/* ICIC */ \
		{ 7,		0x000C,		0x1,		0x1		 },	/* ICIC */ \
		{ 6,		0x0010,		0x1,		0x1		 },	/* ICCL */ \
		{ 7,		0x0010,		0x1,		0x1		 },	/* ICCL */ \
		{ 6,		0x0014,		0x1,		0x1		 },	/* ICCH */ \
		{ 7,		0x0014,		0x1,		0x1		 },	/* ICCH */ \
		{ 8,		0x0410,		0x4,		0x4		 },	/* LDDCKR */ \
		{ 8,		0x4410,		0x4,		0x4		 },	/* LDDCKR */ \
		{ 8,		0x0414,		0x4,		0x4		 },	/* LDDCKSTPR */ \
		{ 8,		0x4414,		0x4,		0x4		 },	/* LDDCKSTPR */ \
		{ 9,		0x05C0,		0x4,		0x14	 },	/* SOMI_CKG_MD - CKG_SMD */ \
		{ 9,		0x0610,		0x4,		0x4		 },	/* CLK_RST */ \
		{ 9,		0x0018,		0x4,		0x8		 },	/* ACK_MD - ACK_RV */ \
		{ 9,		0x0210,		0x4,		0x4		 },	/* ACK_RST */ \
		{ 9,		0x0220,		0x4,		0x4		 },	/* CLK_SEL */ \
		{ 20,		0x0000,		0x4,		0x4		 },	/* FSIDIVA */ \
		{ 10,		0x0000,		0x1,		0x3		 },	/* (0x2) SCBRR */ \
		{ 10,		0x0014,		0x2,		0x2		 },	/* SCSMPL */ \
		{ 10,		0x00C4,		0x2,		0x6		 },	/* (0x2) IFSCH - IFSCL */ \
		{ 11,		0x0000,		0x2,		0x2		 },	/* SCASMR0 */ \
		{ 12,		0x0000,		0x2,		0x2		 },	/* SCASMR1 */ \
		{ 13,		0x0000,		0x2,		0x2		 },	/* SCASMR2 */ \
		{ 14,		0x0000,		0x2,		0x2		 },	/* SCASMR4 */ \
		{ 15,		0x0000,		0x2,		0x2		 },	/* SCASMR5 */ \
		{ 16,		0x0000,		0x2,		0x2		 },	/* SCASMR6 */ \
		{ 11,		0x0004,		0x1,		0x1		 },	/* SCABBR0 */ \
		{ 12,		0x0004,		0x1,		0x1		 },	/* SCABBR1 */ \
		{ 13,		0x0004,		0x1,		0x1		 },	/* SCABBR2 */ \
		{ 14,		0x0004,		0x1,		0x1		 },	/* SCABBR4 */ \
		{ 15,		0x0004,		0x1,		0x1		 },	/* SCABBR5 */ \
		{ 16,		0x0004,		0x1,		0x1		 },	/* SCABBR6 */ \
		{ 11,		0x0008,		0x2,		0x2		 },	/* SCASCR0 */ \
		{ 12,		0x0008,		0x2,		0x2		 },	/* SCASCR1 */ \
		{ 13,		0x0008,		0x2,		0x2		 },	/* SCASCR2 */ \
		{ 14,		0x0008,		0x2,		0x2		 },	/* SCASCR4 */ \
		{ 15,		0x0008,		0x2,		0x2		 },	/* SCASCR5 */ \
		{ 16,		0x0008,		0x2,		0x2		 },	/* SCASCR6 */ \
\
		{ 17,		0x0000,		0x1,		0x4		 },	/* STBCHR0 - STBCHR3 */ \
		{ 17,		0x0004,		0x4,		0x20	 },	/* RPDCR - WUPSMSK */ \
		{ 17,		0x0040,		0x1,		0x4		 },	/* STBCHRB0 - STBCHRB3 */ \
		{ 17,		0x0044,		0x4,		0x8		 },	/* WUPRMSK2 - WUPSMSK2 */ \
		{ 17,		0x0080,		0x4,		0xC		 },	/* PSTR - EXSTMON2 */ \
		{ 17,		0x0094,		0x4,		0x8		 },	/* WUPRFAC - WUPSFAC */ \
		{ 17,		0x00B0,		0x4,		0x8		 },	/* RRSTFR - SRSTFR */ \
		{ 17,		0x00C0,		0x4,		0x4		 },	/* DBGPOWCR */ \
		{ 17,		0x0200,		0x4,		0x8		 },	/* RWBCR - SWBCR */ \
		{ 17,		0x020C,		0x4,		0x8		 },	/* EXMSKCNT1 - EXMSKCNT2 */ \
		{ 17,		0x801C,		0x4,		0x8		 },	/* RESCNT - RESCNT2 */ \
		{ 17,		0x0228,		0x4,		0x8		 },	/* WUPSEL - IRQCR */ \
		{ 17,		0x0234,		0x4,		0x8		 },	/* APSCSTP - IRQCR2 */ \
		{ 17,		0x0244,		0x4,		0x8		 },	/* IRQCR3 - IRQCR4 */ \
		{ 17,		0x0250,		0x4,		0x4		 },	/* WUPSEL2 */ \
\
		{ 18,		0x0000,		0x4,		0x10	 },	/* CAPSR - CMCYR */ \
		{ 18,		0x0010,		0x4,		0x4		 },	/* CAMOR(A) */ \
		{ 18,		0x1010,		0x4,		0x4		 },	/* CAMOR(B) */ \
		{ 18,		0x2010,		0x4,		0x4		 },	/* CAMOR(mirror) */ \
		{ 18,		0x0014,		0x4,		0x4		 },	/* CAPWR(A) */ \
		{ 18,		0x1014,		0x4,		0x4		 },	/* CAPWR(B) */ \
		{ 18,		0x2014,		0x4,		0x4		 },	/* CAPWR(mirror) */ \
		{ 18,		0x0018,		0x4,		0x4		 },	/* CAIFR */ \
		{ 18,		0x0028,		0x4,		0x8		 },	/* CRCNTR - CRCMPR */ \
		{ 18,		0x0030,		0x4,		0x4		 },	/* CFLCR(A) */ \
		{ 18,		0x1030,		0x4,		0x4		 },	/* CFLCR(B) */ \
		{ 18,		0x2030,		0x4,		0x4		 },	/* CFLCR(mirror) */ \
		{ 18,		0x0034,		0x4,		0x4		 },	/* CFSZR(A) */ \
		{ 18,		0x1034,		0x4,		0x4		 },	/* CFSZR(B) */ \
		{ 18,		0x2034,		0x4,		0x4		 },	/* CFSZR(mirror) */ \
		{ 18,		0x0038,		0x4,		0x4		 },	/* CDWDR(A) */ \
		{ 18,		0x1038,		0x4,		0x4		 },	/* CDWDR(B) */ \
		{ 18,		0x2038,		0x4,		0x4		 },	/* CDWDR(mirror) */ \
		{ 18,		0x003C,		0x4,		0x4		 },	/* CDAYR(A) */ \
		{ 18,		0x103C,		0x4,		0x4		 },	/* CDAYR(B) */ \
		{ 18,		0x203C,		0x4,		0x4		 },	/* CDAYR(mirror) */ \
		{ 18,		0x0040,		0x4,		0x4		 },	/* CDACR(A) */ \
		{ 18,		0x1040,		0x4,		0x4		 },	/* CDACR(B) */ \
		{ 18,		0x2040,		0x4,		0x4		 },	/* CDACR(mirror) */ \
		{ 18,		0x0044,		0x4,		0x4		 },	/* CDBYR(A) */ \
		{ 18,		0x1044,		0x4,		0x4		 },	/* CDBYR(B) */ \
		{ 18,		0x2044,		0x4,		0x4		 },	/* CDBYR(mirror) */ \
		{ 18,		0x0048,		0x4,		0x4		 },	/* CDBCR(A) */ \
		{ 18,		0x1048,		0x4,		0x4		 },	/* CDBCR(B) */ \
		{ 18,		0x2048,		0x4,		0x4		 },	/* CDBCR(mirror) */ \
		{ 18,		0x004C,		0x4,		0x4		 },	/* CBDSR(A) */ \
		{ 18,		0x104C,		0x4,		0x4		 },	/* CBDSR(B) */ \
		{ 18,		0x204C,		0x4,		0x4		 },	/* CBDSR(mirror) */ \
		{ 18,		0x205C,		0x4,		0x4		 },	/* CFWCR */ \
		{ 18,		0x0060,		0x4,		0x4		 },	/* CLFCR(A) */ \
		{ 18,		0x1060,		0x4,		0x4		 },	/* CLFCR(B) */ \
		{ 18,		0x2060,		0x4,		0x4		 },	/* CLFCR(mirror) */ \
		{ 18,		0x0064,		0x4,		0x4		 },	/* CDOCR(A) */ \
		{ 18,		0x1064,		0x4,		0x4		 },	/* CDOCR(B) */ \
		{ 18,		0x2064,		0x4,		0x4		 },	/* CDOCR(mirror) */ \
		{ 18,		0x0068,		0x4,		0x4		 },	/* CDDCR(A) */ \
		{ 18,		0x1068,		0x4,		0x4		 },	/* CDDCR(B) */ \
		{ 18,		0x2068,		0x4,		0x4		 },	/* CDDCR(mirror) */ \
		{ 18,		0x006C,		0x4,		0x4		 },	/* CDDAR(A) */ \
		{ 18,		0x106C,		0x4,		0x4		 },	/* CDDAR(B) */ \
		{ 18,		0x206C,		0x4,		0x4		 },	/* CDDAR(mirror) */ \
		{ 18,		0x2070,		0x4,		0x8		 },	/* CEIER - CETCR */ \
		{ 18,		0x207C,		0x4,		0xC		 },	/* CSTSR - CDSSR */ \
		{ 18,		0x0090,		0x4,		0x4		 },	/* CDAYR2(A) */ \
		{ 18,		0x1090,		0x4,		0x4		 },	/* CDAYR2(B) */ \
		{ 18,		0x2090,		0x4,		0x4		 },	/* CDAYR2(mirror) */ \
		{ 18,		0x0094,		0x4,		0x4		 },	/* CDACR2(A) */ \
		{ 18,		0x1094,		0x4,		0x4		 },	/* CDACR2(B) */ \
		{ 18,		0x2094,		0x4,		0x4		 },	/* CDACR2(mirror) */ \
		{ 18,		0x0098,		0x4,		0x4		 },	/* CDBYR2(A) */ \
		{ 18,		0x1098,		0x4,		0x4		 },	/* CDBYR2(B) */ \
		{ 18,		0x2098,		0x4,		0x4		 },	/* CDBYR2(mirror) */ \
		{ 18,		0x009C,		0x4,		0x4		 },	/* CDBCR2(A) */ \
		{ 18,		0x109C,		0x4,		0x4		 },	/* CDBCR2(B) */ \
		{ 18,		0x209C,		0x4,		0x4		 },	/* CDBCR2(mirror) */ \
		{ 18,		0x00A8,		0x4,		0x4		 },	/* CNFSZR(A) */ \
		{ 18,		0x10A8,		0x4,		0x4		 },	/* CNFSZR(B) */ \
		{ 18,		0x20A8,		0x4,		0x4		 },	/* CNFSZR(mirror) */ \
\
		{ 19,		0x0400,		0x4,		0x4		 },	/* S_DO_FMT */ \
		{ 19,		0x040C,		0x4,		0x4		 },	/* S_DI_FMT */ \
		{ 19,		0x05D0,		0x4,		0x14	 },	/* SOMI_CKG_MD - CKG_SMD */ \
		{ 19,		0x0610,		0x4,		0x4		 },	/* CLK_RST */ \
		{ 19,		0x0000,		0x4,		0x4		 },	/* DO_FMT */ \
		{ 19,		0x0018,		0x4,		0x8		 },	/* ACK_MD - ACK_RV */ \
		{ 19,		0x0210,		0x4,		0x4		 },	/* ACK_RST */ \
		{ 19,		0x0220,		0x4,		0x4		 },	/* CLK_SEL */ \
		{ 20,		0x0000,		0x4,		0x4		 },	/* FSIDIVA */ \
\
		{ 21,		0x0010,		0x2,		0x2		 },	/* CKOCR */ \
		{ 21,		0x0040,		0x2,		0x2		 },	/* VCCQ1CR */ \
		{ 21,		0x0084,		0x2,		0x4		 },	/* (0x2) VCCQ1LCDCR */ \
\
		{ 22,		0x0000,		0x4,		0x10	 },	/* ICR1A - ICR4A */ \
		{ 25,		0x0018,		0x4,		0x4		 },	/* INTFLGA */ \
		{ 25,		0x0020,		0x4,		0x4		 },	/* INTEVTA */ \
		{ 25,		0x0030,		0x1,		0x1		 },	/* INTLVLA */ \
		{ 25,		0x0034,		0x1,		0x1		 },	/* INTLVLB */ \
		{ 23,		0x0100,		0x4,		0x4		 },	/* INTEVTAS */ \
		{ 23,		0x0104,		0x2,		0x2		 },	/* INTSMASK */ \
		{ 22,		0x0010,		0x4,		0x10	 },	/* INTPRI00A - INTPRI30A */ \
		{ 22,		0x0020,		0x1,		0x1		 },	/* INTREQ00A */ \
		{ 22,		0x0024,		0x1,		0x1		 },	/* INTREQ10A */ \
		{ 22,		0x0028,		0x1,		0x1		 },	/* INTREQ20A */ \
		{ 22,		0x002C,		0x1,		0x1		 },	/* INTREQ30A */ \
		{ 22,		0x0040,		0x1,		0x1		 },	/* INTMSK00A */ \
		{ 22,		0x0044,		0x1,		0x1		 },	/* INTMSK10A */ \
		{ 22,		0x0048,		0x1,		0x1		 },	/* INTMSK20A */ \
		{ 22,		0x004C,		0x1,		0x1		 },	/* INTMSK30A */ \
		{ 26,		0x0000,		0x4,		0x4		 },	/* USERIMASKA */ \
		{ 23,		0x0000,		0x2,		0x2		 },	/* IPRAA */ \
		{ 23,		0x0004,		0x2,		0x2		 },	/* IPRBA */ \
		{ 23,		0x0008,		0x2,		0x2		 },	/* IPRCA */ \
		{ 23,		0x000C,		0x2,		0x2		 },	/* IPRDA */ \
		{ 23,		0x0010,		0x2,		0x2		 },	/* IPREA */ \
		{ 23,		0x0014,		0x2,		0x2		 },	/* IPRFA */ \
		{ 23,		0x0018,		0x2,		0x2		 },	/* IPRGA */ \
		{ 23,		0x001C,		0x2,		0x2		 },	/* IPRHA */ \
		{ 23,		0x0020,		0x2,		0x2		 },	/* IPRIA */ \
		{ 23,		0x0024,		0x2,		0x2		 },	/* IPRJA */ \
		{ 23,		0x0028,		0x2,		0x2		 },	/* IPRKA */ \
		{ 23,		0x002C,		0x2,		0x2		 },	/* IPRLA */ \
		{ 23,		0x0030,		0x2,		0x2		 },	/* IPRMA */ \
		{ 23,		0x0034,		0x2,		0x2		 },	/* IPRNA */ \
		{ 23,		0x0038,		0x2,		0x2		 },	/* IPROA */ \
		{ 24,		0x0000,		0x2,		0x2		 },	/* IPRAA3 */ \
		{ 24,		0x0004,		0x2,		0x2		 },	/* IPRBA3 */ \
		{ 24,		0x0008,		0x2,		0x2		 },	/* IPRCA3 */ \
		{ 24,		0x000C,		0x2,		0x2		 },	/* IPRDA3 */ \
		{ 24,		0x0010,		0x2,		0x2		 },	/* IPREA3 */ \
		{ 24,		0x0014,		0x2,		0x2		 },	/* IPRFA3 */ \
		{ 24,		0x0018,		0x2,		0x2		 },	/* IPRGA3 */ \
		{ 24,		0x001C,		0x2,		0x2		 },	/* IPRHA3 */ \
		{ 24,		0x0020,		0x2,		0x2		 },	/* IPRIA3 */ \
		{ 24,		0x0024,		0x2,		0x2		 },	/* IPRJA3 */ \
		{ 24,		0x0028,		0x2,		0x2		 },	/* IPRKA3 */ \
		{ 24,		0x002C,		0x2,		0x2		 },	/* IPRLA3 */ \
		{ 24,		0x0030,		0x2,		0x2		 },	/* IPRMA3 */ \
		{ 24,		0x0034,		0x2,		0x2		 },	/* IPRNA3 */ \
		{ 24,		0x0038,		0x2,		0x2		 },	/* IPROA3 */ \
		{ 24,		0x003C,		0x2,		0x2		 },	/* IPRPA3 */ \
		{ 24,		0x0040,		0x2,		0x2		 },	/* IPRQA3 */ \
		{ 24,		0x0044,		0x2,		0x2		 },	/* IPRRA3 */ \
		{ 24,		0x0048,		0x2,		0x2		 },	/* IPRSA3 */ \
		{ 24,		0x004C,		0x2,		0x2		 },	/* IPRTA3 */ \
		{ 24,		0x0050,		0x2,		0x2		 },	/* IPRUA3 */ \
		{ 23,		0x0080,		0x1,		0x1		 },	/* IMR0A */ \
		{ 23,		0x0084,		0x1,		0x1		 },	/* IMR1A */ \
		{ 23,		0x0088,		0x1,		0x1		 },	/* IMR2A */ \
		{ 23,		0x008C,		0x1,		0x1		 },	/* IMR3A */ \
		{ 23,		0x0090,		0x1,		0x1		 },	/* IMR4A */ \
		{ 23,		0x0094,		0x1,		0x1		 },	/* IMR5A */ \
		{ 23,		0x0098,		0x1,		0x1		 },	/* IMR6A */ \
		{ 23,		0x009C,		0x1,		0x1		 },	/* IMR7A */ \
		{ 23,		0x00A0,		0x1,		0x1		 },	/* IMR8A */ \
		{ 23,		0x00A4,		0x1,		0x1		 },	/* IMR9A */ \
		{ 23,		0x00A8,		0x1,		0x1		 },	/* IMR10A */ \
		{ 23,		0x00AC,		0x1,		0x1		 },	/* IMR11A */ \
		{ 23,		0x00B0,		0x1,		0x1		 },	/* IMR12A */ \
		{ 23,		0x00B4,		0x1,		0x1		 },	/* IMR13A */ \
		{ 24,		0x0080,		0x1,		0x1		 },	/* IMR0A3 */ \
		{ 24,		0x0084,		0x1,		0x1		 },	/* IMR1A3 */ \
		{ 24,		0x0088,		0x1,		0x1		 },	/* IMR2A3 */ \
		{ 24,		0x008C,		0x1,		0x1		 },	/* IMR3A3 */ \
		{ 24,		0x0090,		0x1,		0x1		 },	/* IMR4A3 */ \
		{ 24,		0x0094,		0x1,		0x1		 },	/* IMR5A3 */ \
		{ 24,		0x0098,		0x1,		0x1		 },	/* IMR6A3 */ \
		{ 24,		0x009C,		0x1,		0x1		 },	/* IMR7A3 */ \
		{ 24,		0x00A0,		0x1,		0x1		 },	/* IMR8A3 */ \
		{ 24,		0x00A4,		0x1,		0x1		 },	/* IMR9A3 */ \
		{ 24,		0x00A8,		0x1,		0x1		 },	/* IMR10A3 */ \
		{ 23,		0x0180,		0x1,		0x1		 },	/* IMR0AS */ \
		{ 23,		0x0184,		0x1,		0x1		 },	/* IMR1AS */ \
		{ 23,		0x0188,		0x1,		0x1		 },	/* IMR2AS */ \
		{ 23,		0x018C,		0x1,		0x1		 },	/* IMR3AS */ \
		{ 23,		0x0190,		0x1,		0x1		 },	/* IMR4AS */ \
		{ 23,		0x0194,		0x1,		0x1		 },	/* IMR5AS */ \
		{ 23,		0x0198,		0x1,		0x1		 },	/* IMR6AS */ \
		{ 23,		0x019C,		0x1,		0x1		 },	/* IMR7AS */ \
		{ 23,		0x01A0,		0x1,		0x1		 },	/* IMR8AS */ \
		{ 23,		0x01A4,		0x1,		0x1		 },	/* IMR9AS */ \
		{ 23,		0x01A8,		0x1,		0x1		 },	/* IMR10AS */ \
		{ 23,		0x01AC,		0x1,		0x1		 },	/* IMR11AS */ \
		{ 23,		0x01B0,		0x1,		0x1		 },	/* IMR12AS */ \
		{ 23,		0x01B4,		0x1,		0x1		 },	/* IMR13AS */ \
		{ 24,		0x0180,		0x1,		0x1		 },	/* IMR0AS3 */ \
		{ 24,		0x0184,		0x1,		0x1		 },	/* IMR1AS3 */ \
		{ 24,		0x0188,		0x1,		0x1		 },	/* IMR2AS3 */ \
		{ 24,		0x018C,		0x1,		0x1		 },	/* IMR3AS3 */ \
		{ 24,		0x0190,		0x1,		0x1		 },	/* IMR4AS3 */ \
		{ 24,		0x0194,		0x1,		0x1		 },	/* IMR5AS3 */ \
		{ 24,		0x0198,		0x1,		0x1		 },	/* IMR6AS3 */ \
		{ 24,		0x019C,		0x1,		0x1		 },	/* IMR7AS3 */ \
		{ 24,		0x01A0,		0x1,		0x1		 },	/* IMR8AS3 */ \
		{ 24,		0x01A4,		0x1,		0x1		 },	/* IMR9AS3 */ \
		{ 24,		0x01A8,		0x1,		0x1		 },	/* IMR10AS3 */ \
		{ 22,		0x00C4,		0x4,		0x8		 },	/* ISCR0 - ISCR1 */ \
		{ 23,		0x0118,		0x1,		0x1		 },	/* DIRCR */ \
\
		{ 28,		0x0000,		0x4,		0x10	 },	/* ICR0S - ICR3S */ \
		{ 27,		0x0040,		0x4,		0x20	 },	/* ICR3 - ICR10 */ \
		{ 28,		0x0010,		0x4,		0x10	 },	/* INTPRI00S - INTPRI30S */ \
		{ 28,		0x0020,		0x1,		0x1		 },	/* INTREQ00S */ \
		{ 28,		0x0024,		0x1,		0x1		 },	/* INTREQ10S */ \
		{ 28,		0x0028,		0x1,		0x1		 },	/* INTREQ20S */ \
		{ 28,		0x002C,		0x1,		0x1		 },	/* INTREQ30S */ \
		{ 28,		0x0040,		0x1,		0x1		 },	/* INTMSK00S */ \
		{ 28,		0x0044,		0x1,		0x1		 },	/* INTMSK10S */ \
		{ 28,		0x0048,		0x1,		0x1		 },	/* INTMSK20S */ \
		{ 28,		0x004C,		0x1,		0x1		 },	/* INTMSK30S */ \
		{ 29,		0x0100,		0x4,		0x4		 },	/* INTEVTSA */ \
		{ 30,		0x00C0,		0x2,		0x2		 },	/* NMIFCRS */ \
		{ 29,		0x0104,		0x2,		0x2		 },	/* INTAMASK */ \
		{ 31,		0x0000,		0x4,		0x4		 },	/* USERIMASKS */ \
		{ 29,		0x0000,		0x2,		0x2		 },	/* IPRAS */ \
		{ 29,		0x0004,		0x2,		0x2		 },	/* IPRBS */ \
		{ 29,		0x0008,		0x2,		0x2		 },	/* IPRCS */ \
		{ 29,		0x000C,		0x2,		0x2		 },	/* IPRDS */ \
		{ 29,		0x0010,		0x2,		0x2		 },	/* IPRES */ \
		{ 29,		0x0014,		0x2,		0x2		 },	/* IPRFS */ \
		{ 29,		0x0018,		0x2,		0x2		 },	/* IPRGS */ \
		{ 29,		0x001C,		0x2,		0x2		 },	/* IPRHS */ \
		{ 29,		0x0020,		0x2,		0x2		 },	/* IPRIS */ \
		{ 29,		0x0024,		0x2,		0x2		 },	/* IPRJS */ \
		{ 29,		0x0028,		0x2,		0x2		 },	/* IPRKS */ \
		{ 29,		0x002C,		0x2,		0x2		 },	/* IPRLS */ \
		{ 32,		0x0000,		0x2,		0x2		 },	/* IPRAS3 */ \
		{ 32,		0x0004,		0x2,		0x2		 },	/* IPRBS3 */ \
		{ 32,		0x0008,		0x2,		0x2		 },	/* IPRCS3 */ \
		{ 32,		0x000C,		0x2,		0x2		 },	/* IPRDS3 */ \
		{ 32,		0x0010,		0x2,		0x2		 },	/* IPRES3 */ \
		{ 32,		0x0014,		0x2,		0x2		 },	/* IPRFS3 */ \
		{ 32,		0x0018,		0x2,		0x2		 },	/* IPRGS3 */ \
		{ 32,		0x001C,		0x2,		0x2		 },	/* IPRHS3 */ \
		{ 32,		0x0020,		0x2,		0x2		 },	/* IPRIS3 */ \
		{ 32,		0x0024,		0x2,		0x2		 },	/* IPRJS3 */ \
		{ 32,		0x0028,		0x2,		0x2		 },	/* IPRKS3 */ \
		{ 32,		0x002C,		0x2,		0x2		 },	/* IPRLS3 */ \
		{ 32,		0x0030,		0x2,		0x2		 },	/* IPRMS3 */ \
		{ 32,		0x0034,		0x2,		0x2		 },	/* IPRNS3 */ \
		{ 32,		0x0038,		0x2,		0x2		 },	/* IPROS3 */ \
		{ 32,		0x003C,		0x2,		0x2		 },	/* IPRPS3 */ \
		{ 29,		0x0080,		0x1,		0x1		 },	/* IMR0S */ \
		{ 29,		0x0084,		0x1,		0x1		 },	/* IMR1S */ \
		{ 29,		0x0088,		0x1,		0x1		 },	/* IMR2S */ \
		{ 29,		0x008C,		0x1,		0x1		 },	/* IMR3S */ \
		{ 29,		0x0090,		0x1,		0x1		 },	/* IMR4S */ \
		{ 29,		0x0094,		0x1,		0x1		 },	/* IMR5S */ \
		{ 29,		0x0098,		0x1,		0x1		 },	/* IMR6S */ \
		{ 29,		0x009C,		0x1,		0x1		 },	/* IMR7S */ \
		{ 29,		0x00A0,		0x1,		0x1		 },	/* IMR8S */ \
		{ 29,		0x00A4,		0x1,		0x1		 },	/* IMR9S */ \
		{ 29,		0x00A8,		0x1,		0x1		 },	/* IMR10S */ \
		{ 29,		0x00AC,		0x1,		0x1		 },	/* IMR11S */ \
		{ 29,		0x00B0,		0x1,		0x1		 },	/* IMR12S */ \
		{ 32,		0x0080,		0x1,		0x1		 },	/* IMR0S3 */ \
		{ 32,		0x0084,		0x1,		0x1		 },	/* IMR1S3 */ \
		{ 32,		0x0088,		0x1,		0x1		 },	/* IMR2S3 */ \
		{ 32,		0x008C,		0x1,		0x1		 },	/* IMR3S3 */ \
		{ 32,		0x0090,		0x1,		0x1		 },	/* IMR4S3 */ \
		{ 32,		0x0094,		0x1,		0x1		 },	/* IMR5S3 */ \
		{ 32,		0x0098,		0x1,		0x1		 },	/* IMR6S3 */ \
		{ 32,		0x009C,		0x1,		0x1		 },	/* IMR7S3 */ \
		{ 29,		0x0180,		0x1,		0x1		 },	/* IMR0SA */ \
		{ 29,		0x0184,		0x1,		0x1		 },	/* IMR1SA */ \
		{ 29,		0x0188,		0x1,		0x1		 },	/* IMR2SA */ \
		{ 29,		0x018C,		0x1,		0x1		 },	/* IMR3SA */ \
		{ 29,		0x0190,		0x1,		0x1		 },	/* IMR4SA */ \
		{ 29,		0x0194,		0x1,		0x1		 },	/* IMR5SA */ \
		{ 29,		0x0198,		0x1,		0x1		 },	/* IMR6SA */ \
		{ 29,		0x019C,		0x1,		0x1		 },	/* IMR7SA */ \
		{ 29,		0x01A0,		0x1,		0x1		 },	/* IMR8SA */ \
		{ 29,		0x01A4,		0x1,		0x1		 },	/* IMR9SA */ \
		{ 29,		0x01A8,		0x1,		0x1		 },	/* IMR10SA */ \
		{ 29,		0x01AC,		0x1,		0x1		 },	/* IMR11SA */ \
		{ 29,		0x01B0,		0x1,		0x1		 },	/* IMR12SA */ \
		{ 32,		0x0180,		0x1,		0x1		 },	/* IMR0SA3 */ \
		{ 32,		0x0184,		0x1,		0x1		 },	/* IMR1SA3 */ \
		{ 32,		0x0188,		0x1,		0x1		 },	/* IMR2SA3 */ \
		{ 32,		0x018C,		0x1,		0x1		 },	/* IMR3SA3 */ \
		{ 32,		0x0190,		0x1,		0x1		 },	/* IMR4SA3 */ \
		{ 32,		0x0194,		0x1,		0x1		 },	/* IMR5SA3 */ \
		{ 32,		0x0198,		0x1,		0x1		 },	/* IMR6SA3 */ \
		{ 32,		0x019C,		0x1,		0x1		 },	/* IMR7SA3 */ \
	}


/***********************************************
 * モデル定義(EMPTY)
 *
 ***********************************************/

/**
 * @brief リージョン定義
 *	UBINUX_REGISTER_REGION構造体の中身。
 *	配列になるよう定義する。indexがリージョン番号となる。 
 */
/*      { phys_addr,	size       }						*/
#define FTEN_CONFIG_UBINUX_REGISTER_REGION_LIST_EMPTY { \
		{ 0x0,			UBINUX_PANIC_INFO_NOP }	/* NOP */ \
	}

/**
 * @brief レジスタ定義
 *	UBINUX_REGISTER_DEF構造体の中身。
 *	配列になるよう定義する。
 */
/*      { 						access,		copy	 }		*/
/*      { region,	offset,		byte		length	 }		*/
#define FTEN_CONFIG_UBINUX_REGISTER_DEF_LIST_EMPTY { \
		{ 0,		0x0,		0x0,		UBINUX_PANIC_INFO_NOP      }	/* NOP */ \
	}



/***********************************************
 * モデル定義
 *
 * 新たにモデル定義を追加する場合は、
 * FTEN_CONFIG_UBINUX_PANIC_INFO_FORMAT_VER_モデル名
 * FTEN_CONFIG_UBINUX_REGISTER_REGION_LIST_モデル名
 * FTEN_CONFIG_UBINUX_REGISTER_DEF_LIST_モデル名
 * 等、後ろにモデル名を付加して定義すること。
 *
 ***********************************************/


#endif	// __UBINUX_PANIC_INFO_CUSTOMIZED_H
