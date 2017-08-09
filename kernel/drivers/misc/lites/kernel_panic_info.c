/**
 * @file kernel_panic_info.c
 *
 * @brief kernel panic情報に関する操作用関数の実装
 *
 * Copyright (c) 2011-2012 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/lites_trace.h>
#include <linux/ubinux_panic_info.h>
#include <linux/slab.h>
#include "parse.h"
#include "region.h"
#include "kernel_panic_info.h"
#include "tsc.h"
#include "mgmt_info.h"
#include <linux/sched.h>
#include <linux/reboot.h>

#ifdef CONFIG_MACH_FTENBB
#include <linux/ftenbb_config.h>	/* モデル別定義の読み出し */
/* モデル定義に応じた、サイズ定義、構造体定義、採取レジスタ定義が展開される */

// コンフィグレーションで指定したモデルに対応するMAPで
// トレースグループ定義を読み込む
#include <linux/lites_trace_map.h>

#else	/* CONFIG_MACH_FTENBB */

// モデル別定義の読み出しが出来ない場合は、固定で。

#include <linux/ubinux_panic_info_customized.h>

/**
 * @brief kernel panic情報構造体（共通モデル用）
 */
typedef UBINUX_KERNEL_PANIC_INFO_COMMON		UBINUX_KERNEL_PANIC_INFO;

/**
 * @brief モデルIDの定義（共通モデル用）
 */
#define UBINUX_PANIC_INFO_MODEL_ID	FTEN_CONFIG_MODEL_ID_COMMON

/**
 * @brief モデル定義のフォーマットバージョンの定義（共通モデル用）
 */
#define UBINUX_PANIC_INFO_FORMAT_VER	FTEN_CONFIG_UBINUX_PANIC_INFO_FORMAT_VER_COMMON

/**
 * @brief リージョン定義
 *	UBINUX_REGISTER_REGION構造体の中身。
 *	配列になるよう定義する。indexがリージョン番号となる。 
 */
/*      { phys_addr,	size       }						*/
#define UBINUX_REGISTER_REGION_LIST { \
		{ 0x16000000,	0x00001000 }	/* region 0 ASIC */ \
	}

/**
 * @brief レジスタ定義
 *	UBINUX_REGISTER_DEF構造体の中身。
 *	配列になるよう定義する。
 */
/*      { 						access,		copy	 }		*/
/*      { region,	offset,		byte		length	 }		*/
#define UBINUX_REGISTER_DEF_LIST { \
		{ 0,		0x800,		0x2,		0x800     }	/* ASIC */ \
	}


// テスト用定義の読み込む
// #define	FTENBB_RAS_REGION_ADDR			0x20000000
// #include "test_lites_trace_map.h"
// ADA用ベースアドレスはlites_trace_map.hで定義
#include <linux/lites_trace_map.h>

#endif	/* CONFIG_MACH_FTENBB */



#ifdef LITES_FTEN

int kernel_panic_info_init_done = 0;				// カーネルパニック情報格納領域init実施有無

static UBINUX_KERNEL_PANIC_INFO *ukp_info = NULL;	// マップしたカーネルパニック情報格納領域

#ifndef LITES_FTEN_MM_DIRECO
static u64 kernel_panic_info_start;				// カーネル起動パラメータ解析で解析した開始アドレス
static u32 kernel_panic_info_size;				// カーネル起動パラメータ解析で解析したサイズ
static int kernel_panic_info_parse_done = 0;	// カーネル起動パラメータ解析実施の有無
#endif	// LITES_FTEN_MM_DIRECO


// MMIOレジスタ関連の定義
static UBINUX_REGISTER_REGION region_list[] = UBINUX_REGISTER_REGION_LIST;
static UBINUX_REGISTER_DEF register_list[] = UBINUX_REGISTER_DEF_LIST;
static volatile void __iomem	*region_pointer[ ARRAY_SIZE(region_list) ];


// 内部用関数宣言
static void ubinux_panic_save_timestamp(void);
static int ubinux_panic_save_printk_region_message(void);
static int ubinux_panic_save_printk_region_var_message(struct lites_region *region);
static int ubinux_panic_save_printk_region_const_message(struct lites_region *region);
static int ubinux_panic_save_printk_buffer_message(void);
static void ubinux_panic_save_message(void);
static void ubinux_panic_save_kernel_stack(void);
static int ubinux_panic_save( struct notifier_block *, unsigned long, void *);
static unsigned int ubinux_alignment_size(unsigned int size);
static void ubinux_register_memcpy_u8(u8 *dest, const u8 *src, int copy_num);
static void ubinux_register_memcpy_u16(u16 *dest, const u16 *src, int copy_num);
static void ubinux_register_memcpy_u32(u32 *dest, const u32 *src, int copy_num);
static void ubinux_panic_save_mmio_register(void);
static void ubinux_panic_save_general_register(void);
static __init int ubinux_kernel_panic_info_init(void);
#ifndef LITES_FTEN_MM_DIRECO
static __init int early_ubinux_kernel_panic_info(char *arg);
#endif	// LITES_FTEN_MM_DIRECO
// この関数のみ、最適化によるインライン展開を抑止
void ubinux_copy_general_register(unsigned int *sp, unsigned int *pc) __attribute__((__noinline__));


// 外部関数宣言
extern int last_log_buf_copy(char *dest, int len);	// kernel/printk.c


#ifdef TEST_PANIC_SAVE
NORET_TYPE void test_panic_save()
{
	ubinux_panic_save(NULL, 0, NULL);
}
EXPORT_SYMBOL(test_panic_save);
#endif	// TEST_PANIC_SAVE

/**
 * @brief kernel panic情報格納領域獲得用関数
 *
 * @param[in]  addr kernel panic情報格納領域開始アドレス
 * @param[in]  size kernel panic情報格納領域情報のサイズ
 *
 * @retval 管理情報 正常終了
 * @retval NULL     異常終了
 */
void *ubinux_kernel_panic_info_alloc(u64 addr, u32 size)
{
	if (ukp_info != NULL) {
		return ukp_info;
	}

	ukp_info = ioremap_nocache(addr, size);
	if (ukp_info == NULL) {
		lites_pr_err("ioremap_nocache error. addr=%064x size=%08x", addr, size);
	}

	return (void *)ukp_info;
}


/**
 * @brief kernel panic情報格納領域解放用関数
 */
void ubinux_kernel_panic_info_free(void)
{
	if (ukp_info != NULL) {
		iounmap(ukp_info);
		ukp_info = NULL;
	}
}


/**
 * @brief タイムスタンプ保存関数
 */
static void ubinux_panic_save_timestamp(void)
{
	// jiffies64値を取得
	lites_get_tsc(ukp_info->header.timestamp);
	// 管理情報から取得したBootIDを設定
	if (unlikely(mgmt_info_init_done == 0)) {
		lites_pr_err("not initialized mgmt_info");
		ukp_info->header.boot_id = 0;	// BootID取得不可のため、0代入
	} else {
		ukp_info->header.boot_id = lites_mgmt_info_get_boot_id();
	}
	// 時刻情報を取得
	do_gettimeofday(&ukp_info->header.tv);
	// timezoneを取得
	memcpy(&ukp_info->header.tz, &sys_tz, sizeof(sys_tz));

	ukp_info->header.status |= UBINUX_TIMESTAMP_AVAILABLE;
}


/**
 * @brief カーネルパニックメッセージ（printk_region）保存関数
 *
 * @retval  0  正常終了
 * @retval -1  異常が発生した
 */
static int ubinux_panic_save_printk_region_message(void)
{
	struct lites_region *region;

	// printkリージョンのリージョン管理を取得
	region = lites_find_region_with_number(LITES_PRINTK_REGION);
	if (unlikely(region == NULL)) {
		lites_pr_err("not initialized region");
		return -1;
	}

	// 読み出すデータが無い場合は、異常
	if((region->rb->flags & LITES_FLAG_READ) == 0) {
		lites_pr_err("data not exist (region = 0x%08x)", LITES_PRINTK_REGION);
		return -1;
	}

	if (is_lites_var_log(region->rb->access_byte)) {
		// 可変長メッセージ処理
		ubinux_panic_save_printk_region_var_message(region);
	} else {
		// 固定長メッセージ処理
		ubinux_panic_save_printk_region_const_message(region);
	}

	return 0;
}


/**
 * @brief カーネルパニック可変長メッセージ（printk_region）保存関数
 *
 * @param[in]  region  リージョン管理
 *
 * @retval  0  正常終了
 * @retval -1  異常が発生した
 */
static int ubinux_panic_save_printk_region_var_message(struct lites_region *region)
{
	// リージョン内のデータ量を算出し、
	// 最古のログレコードから参照し、残りデータ量が格納領域に収まる位置を見つける
	//   ※リージョン内のログ配置で回り込みが行われている状態は、
	//     末尾の空き領域を考慮する。
	// 格納領域に収まる位置以降は、回り込みする前までは、ログをログレコード単位で格納する。
	// 回り込み後であれば、ログを一括で残データ分を格納する。
	//
	// 回り込みして読むケース
	//
	//    region->rb->size
	//    <------------------------------------>
	//   sizeof(struct lites_trace_region_header)
	//    <--->
	//         (a) + remaining_part_size = remaining_data_size
	//              (a)        remaining_part_size
	//         <---------->    <---------------->
	//   |-----|++++++++++|----|+++++++++++++|--|
	//         *-------------->read
	//         *--------->write
	//         *
	//    region->rb->log
	//                         *read_point
	//                              *read_point
	//                          <-------------->|
	//                               <--------->|
	//                          remaining_part_size
	//
	//
	// readの位置からwriteの位置まで読むケース
	//    region->rb->size
	//    <------------------------------------>
	//   sizeof(struct lites_trace_region_header)
	//    <--->
	//                 remaining_data_size
	//             <------------------------>
	//   |-----|--|++++++++++++++++++++++++++|--|
	//         *---------------------------->write
	//         *-->read
	//         *
	//    region->rb->log
	//             *read_point
	//                   *read_point

	int remaining_part_size = 0;
	int remaining_data_size = 0;
	int record_size = 0;
	int write_offset = 0;
	char *read_point = region->rb->log + region->rb->read;

	// リージョン内のデータ量を算出
	if (region->rb->write <= region->rb->read) {
		// 回り込みを考慮するケース
		remaining_part_size = region->rb->size - sizeof(struct lites_trace_region_header) - region->rb->read;
		remaining_data_size = region->rb->write + remaining_part_size;
	} else {
		// 回り込み考慮不要
		remaining_data_size = region->rb->write - region->rb->read;
	}


	// 回り込み前
	while (remaining_part_size >= sizeof(LITES_LOG_COMMON_HEADER)) {
		// １件分のログレコードサイズ
		record_size = sizeof(LITES_LOG_COMMON_HEADER) + ((LITES_LOG_COMMON_HEADER*)read_point)->size;
		if (unlikely(!lites_check_log_record_size(record_size))) {
			// ログレコードサイズが不正値のため、終了
			lites_pr_err("log record size error.");
			return -1;
		}

		if (unlikely(remaining_part_size < record_size)) {
			// 末尾から先頭に渡ってログレコードが配置されてるケース
			if (remaining_data_size <= sizeof(ukp_info->panic_msg)) {
				// 末尾から先頭にかけてログレコードを格納するケース
				// 末尾の部分の処理
				memcpy(ukp_info->panic_msg + write_offset, read_point, remaining_part_size);
				// 残りの先頭からの処理
				memcpy(ukp_info->panic_msg + write_offset + remaining_part_size, region->rb->log, (record_size - remaining_part_size) );
				write_offset += record_size;	// 格納した分だけ進める。
			}
			read_point = region->rb->log + (record_size - remaining_part_size);	// 読み出し位置を先頭へ戻した後、残り分も進める
			remaining_part_size = 0;	// 末尾のデータ処理済み
		} else {
			// そのまま処理できるケース
			if (remaining_data_size <= sizeof(ukp_info->panic_msg)) {
				// ログレコードを格納するケース
				memcpy(ukp_info->panic_msg + write_offset, read_point, record_size);
				write_offset += record_size;	// 格納した分だけ進める。
			}
			read_point += record_size;	// ログレコード分だけ進める
			remaining_part_size -= record_size;	// 折り返しの残りサイズから、完了したログレコードサイズ分を減らす
		}

		remaining_data_size -= record_size;	// 残データサイズから、完了したログレコードサイズ分を減らす
	}

	// 回り込み前の処理で残領域にLITES_LOG_COMMON_HEADERが格納できなケースの後処理
	if (unlikely(remaining_part_size > 0)) {
		read_point = region->rb->log;	// 読み出し位置を先頭へ
		remaining_data_size -= remaining_part_size;	// 末尾の無視したデータ分だけ、残データサイズから減らす
	}

	// 回り込み考慮不要 or 回り込み後 の処理
	while (remaining_data_size > sizeof(ukp_info->panic_msg)) {
		// ログレコードを読み飛ばすケース
		// １件分のログレコードサイズ
		record_size = sizeof(LITES_LOG_COMMON_HEADER) + ((LITES_LOG_COMMON_HEADER*)read_point)->size;
		if (unlikely(!lites_check_log_record_size(record_size))) {
			// ログレコードサイズが不正値のため、終了
			lites_pr_err("log record size error.");
			return -1;
		}
		read_point += record_size;	// ログレコード分だけ進める
		remaining_data_size -= record_size;	// 残データサイズから、完了したログレコードサイズ分を減らす
	}
	// 残データを一括で格納する
	memcpy(ukp_info->panic_msg + write_offset, read_point, remaining_data_size);

	ukp_info->header.status |= UBINUX_PANIC_MESSAGE_AVAILABLE;
	return 0;
}


/**
 * @brief カーネルパニック固定長メッセージ（printk_region）保存関数
 *
 * @param[in]  region  リージョン管理
 *
 * @retval  0  正常終了
 * @retval -1  異常が発生した
 */
static int ubinux_panic_save_printk_region_const_message(struct lites_region *region)
{
	// 固定長であるため、格納領域へ格納できるログ数と、
	// リージョン内のa_partとb_part内のログ数が算出可能である。
	//
	// 回り込みして読むケース
	//
	//    region->rb->size
	//    <------------------------------------>
	//   sizeof(struct lites_trace_region_header)
	//    <--->
	//            a_part             b_part
	//         <---------->    <---------------->
	//   |-----|++++++++++|----|+++++++++++++|--|
	//         *-------------->read
	//         *--------->write
	//         *
	//    region->rb->log
	//                         *read_point
	//
	//
	// 先頭からwriteの位置まで読むケース
	//    region->rb->size
	//    <------------------------------------>
	//   sizeof(struct lites_trace_region_header)
	//    <--->
	//                       a_part
	//          <--------------------------->
	//   |-----|+++++++++++++++++++++++++++++|--|
	//         *---------------------------->write
	//         *read
	//         *region->rb->log
	//         *read_point

	char *read_point;
	int a_part_log_num = 0;
	int b_part_log_num = 0;
	int panic_log_num = 0;
	int unnecessary_num = 0;
	int i;
	int offset = 0;
	unsigned short new_size = 0;
	unsigned short log_size = 0;

	// リージョン内のa_partとb_partに格納されてるログ数を求める
	if (region->rb->write <= region->rb->read) {
		// 回り込みのケース
		a_part_log_num = region->rb->write / region->rb->access_byte;
		b_part_log_num = 
			(region->rb->size - region->rb->read - sizeof(struct lites_trace_region_header)) / region->rb->access_byte;
	} else {
		a_part_log_num = (region->rb->write - region->rb->read) / region->rb->access_byte;
		b_part_log_num = 0;
	}

	// 領域に入るログ数
	panic_log_num = sizeof(ukp_info->panic_msg) / region->rb->access_byte;

	if (a_part_log_num >= panic_log_num) {
		// 回り込みを考慮せず、格納できる
		read_point = region->rb->log +	// a_partの先頭
					((a_part_log_num - panic_log_num) * region->rb->access_byte);	// 余分データ分のオフセット
		memcpy(ukp_info->panic_msg, read_point, panic_log_num * region->rb->access_byte);
	} else {
		// 回り込み部分を考慮して格納する
		// 回り込み前
		unnecessary_num = b_part_log_num - (panic_log_num - a_part_log_num);	// b_partの不要なログ数
		read_point = region->rb->log + region->rb->read +	// b_partの先頭
					(unnecessary_num * region->rb->access_byte);	// 余分データ分のオフセット
		memcpy(ukp_info->panic_msg,
				read_point,
				(b_part_log_num - unnecessary_num) * region->rb->access_byte);
		// 回り込み後
		read_point = region->rb->log;	// a_partの先頭
		memcpy(ukp_info->panic_msg + ((b_part_log_num - unnecessary_num) * region->rb->access_byte),
				read_point,
				a_part_log_num * region->rb->access_byte);
	}

	// 格納データのsizeを差し替える
	read_point = ukp_info->panic_msg;
	new_size = region->rb->access_byte - sizeof(LITES_LOG_COMMON_HEADER);
	for (i = 0; i < panic_log_num; ++i) {
		offset = i * region->rb->access_byte;
		log_size = ((LITES_LOG_COMMON_HEADER*)(ukp_info->panic_msg + offset))->size;
		// 1レコードの残領域部分を0クリア(回り込みが行われた後のゴミを消去)
		if (new_size - log_size > 0) {
			memset(ukp_info->panic_msg + offset + sizeof(LITES_LOG_COMMON_HEADER) + log_size,
				0, new_size - log_size);
		}
		((LITES_LOG_COMMON_HEADER*)(ukp_info->panic_msg + offset))->size = new_size;
	}

	ukp_info->header.status |= UBINUX_PANIC_MESSAGE_AVAILABLE;
	return 0;
}


/**
 * @brief カーネルパニックメッセージ（printk_buffer）保存関数
 *
 * @retval  0  正常終了
 */
static int ubinux_panic_save_printk_buffer_message(void)
{
	int ret;

	// printkバッファの最終から指定サイズ分だけ格納
	ret = last_log_buf_copy(ukp_info->panic_msg, sizeof(ukp_info->panic_msg) - 1);
	ukp_info->panic_msg[sizeof(ukp_info->panic_msg) - 1] = '\0';	// 終端
	if( ret > 0 ) {
		ukp_info->header.status |= UBINUX_PANIC_MESSAGE_NO_TIMESTAMP_AVAILABLE;
	}

	return 0;
}


/**
 * @brief カーネルパニックメッセージ保存関数
 */
static void ubinux_panic_save_message(void)
{
	// printkの出力先に応じて、その出力先からメッセージを取得する
	if (vprintk_caller_index == 1) {
		// printkリージョンから取得
		ubinux_panic_save_printk_region_message();
	} else {
		// printkバッファから取得
		ubinux_panic_save_printk_buffer_message();
	}
}


/**
 * @brief カーネルスタック保存関数
 */
static void ubinux_panic_save_kernel_stack(void)
{
	// カーネルスタックの保存
	// 
	//        0x****0000 ----> |----------| <- カーネルスタックの末尾
	//                         |          |
	//                         |          |
	//                         |----------|
	//                         |          |
	//                         |          |
	//        stack_pointer -> |----------| <- カーネルスタックは↑に成長する
	//        0x****####       | 使用中   |  ↑
	//        ↑               | カーネル |  │
	// ※取得したstack_pointer | スタック |  ※この領域を格納領域[8]以降に格納する
	// 　のアドレス(4byte)を   | 領域     |  │
	// 　格納領域[0]から格納   |          |  ↓
	//        0x****1fff ----> |----------| <- カーネルスタックの先頭
	// ※stack_size(4byte)を
	// 　格納領域[4]から格納

	unsigned int stack_pointer;
	unsigned int stack_size = 0;

#ifdef CONFIG_ARM
	register unsigned int sp asm ("sp");
#elif defined CONFIG_X86
	register unsigned int sp asm("esp");
#else
	// not support
	return;
#endif

	stack_pointer = sp;		/*pgr0039*/
	stack_size = THREAD_SIZE - ((THREAD_SIZE - 1) & sp);
	if (stack_size > (sizeof(ukp_info->kernel_stack) - 8)) {
		stack_size = (sizeof(ukp_info->kernel_stack) - 8);
	}
	memcpy(&ukp_info->kernel_stack[0], &stack_pointer, 4);
	memcpy(&ukp_info->kernel_stack[4], &stack_size, 4);
	memcpy(&ukp_info->kernel_stack[8], (void *)stack_pointer, stack_size);
	ukp_info->header.status |= UBINUX_KERNEL_STACK_AVAILABLE;
}


/**
 * @brief カーネルパニック情報採取関数
 *
 * @param[in]  nv   notifier_callを登録するために自分で定義したstruct notifier_block構造体へのポインタ
 * @param[in]  val  0固定
 * @param[in]  v    パニックメッセージへのポインタ（メッセージは呼び元で出力済み）
 *
 * @retval NOTIFY_DONE
 */
static int ubinux_panic_save( struct notifier_block *nv, unsigned long val, void *v )
{

	/* FUJITSU-TEN 2013-09-05 : WARNING対策 add start */
	return NOTIFY_DONE;
	/* FUJITSU-TEN 2013-09-05 : WARNING対策 add end */

	// 既にカーネルパニック情報退避領域内に情報が格納されてる場合は、上書きしない
	if ( ukp_info->header.status != 0 ) {
		lites_pr_info("Kernel panic information has already been stored.");
		return NOTIFY_DONE;
	}

	// 汎用レジスタの保存
	ubinux_panic_save_general_register();

	// モデルIDの保存
	ukp_info->header.model_id = UBINUX_PANIC_INFO_MODEL_ID;

	// フォーマットVersionの保存
	ukp_info->header.format_version = UBINUX_PANIC_INFO_FORMAT_VER;

	// タイムスタンプの保存
	ubinux_panic_save_timestamp();

	// Panicメッセージの保存
	ubinux_panic_save_message();

	// カーネルスタックの保存
	ubinux_panic_save_kernel_stack();

	// MMIOレジスタの保存
	ubinux_panic_save_mmio_register();

	return NOTIFY_DONE;
}


/**
 * @brief アライメント調整関数
 *
 * @param[in]  size  調整する値
 *
 * @return 0以上  4バイトアライメントした値
 */
static unsigned int ubinux_alignment_size(unsigned int size)
{
	// 4バイトアライメント
	return ((size + 3) / 4) * 4;
}


/**
 * @brief レジスタ値取得(1バイトアクセス)関数
 *
 * @param[in]  dest      レジスタ値の格納先アドレス
 * @param[in]  src       レジスタ値の取得先アドレス
 * @param[in]  copy_num  1バイトアクセスで取得する数
 */
static void ubinux_register_memcpy_u8(u8 *dest, const u8 *src, int copy_num)
{
	int offset = 0;
	for ( offset = 0; offset < copy_num; ++offset ) {
		*(dest + offset) = *(src + offset);
	}
}


/**
 * @brief レジスタ値取得(2バイトアクセス)関数
 *
 * @param[in]  dest      レジスタ値の格納先アドレス
 * @param[in]  src       レジスタ値の取得先アドレス
 * @param[in]  copy_num  4バイトアクセスで取得する数
 */
static void ubinux_register_memcpy_u16(u16 *dest, const u16 *src, int copy_num)
{
	int offset = 0;
	for ( offset = 0; offset < copy_num; ++offset ) {
		*(dest + offset) = *(src + offset);
	}
}


/**
 * @brief レジスタ値取得(4バイトアクセス)関数
 *
 * @param[in]  dest      レジスタ値の格納先アドレス
 * @param[in]  src       レジスタ値の取得先アドレス
 * @param[in]  copy_num  4バイトアクセスで取得する数
 */
static void ubinux_register_memcpy_u32(u32 *dest, const u32 *src, int copy_num)
{
	int offset = 0;
	for ( offset = 0; offset < copy_num; ++offset ) {
		*(dest + offset) = *(src + offset);
	}
}


/**
 * @brief MMIOレジスタ保存関数
 *        rma1上にあるMMIOレジスタをカーネルパニック情報格納領域へ保存する。
 */
static void ubinux_panic_save_mmio_register(void)
{
	int		n_register;
	int		write_offset = 0;	// registers配列のインデックス相当
	void 	*map_address;
	UBINUX_REGISTER_DEF		*register_def;

	// 既にmap済みのMMIO領域へアクセスし、定義で指定されたレジスタの値を取得し、
	// 格納領域へ格納する。
	// 
	// ※MMIO領域は、参照のみで、書き換えは厳禁！


	for ( n_register = 0; n_register < ARRAY_SIZE(register_list) ; n_register ++ ) {
		// レジスタ定義を取得
		register_def = &register_list[ n_register ];

		if (unlikely(register_def->copy_length == UBINUX_PANIC_INFO_NOP)) {
			// copy_lengthがUBINUX_PANIC_INFO_NOP(0)の場合はNOP
			lites_pr_info("register_def[%d] is nop.", n_register);
			continue;
		}

		if ( ( (write_offset * sizeof(ukp_info->mmio_registers[0])) +
			   ubinux_alignment_size(register_def->copy_length)  ) > sizeof(ukp_info->mmio_registers) ) {
			// 格納領域が不足しているので、中断する
			lites_pr_err("registers area is short.");
			break;
		}

		// レジスタ定義で指定されたリージョンのMMIO領域を取得し、
		// そこからレジスタの内容を格納する
		map_address = (void *)region_pointer[ register_def->region ];
		if ( map_address == NULL ) {
			// 初期化時のMAPに失敗している
			// MMIO領域にアクセスできないので何もしない
			lites_pr_err("region(%d) is not mapping.", register_def->region);
		} else {
			// MMIO領域からレジスタを読む処理
			switch (register_def->access_byte) {
				case 1:
					ubinux_register_memcpy_u8((u8*)(ukp_info->mmio_registers + write_offset),
						map_address + register_def->offset,
						register_def->copy_length / register_def->access_byte);
					break;
				case 2:
					ubinux_register_memcpy_u16((u16*)(ukp_info->mmio_registers + write_offset),
						(u16*)(map_address + register_def->offset),
						register_def->copy_length / register_def->access_byte);
					break;
				case 4:
				default:
					ubinux_register_memcpy_u32((u32*)(ukp_info->mmio_registers + write_offset),
						(u32*)(map_address + register_def->offset),
						register_def->copy_length / register_def->access_byte);
					break;
			}

			ukp_info->header.status |= UBINUX_MMIO_REGISTER_AVAILABLE;
		}

		// レジスタ定義の単位で4バイトアライメントとして格納することを意識して、
		// write_offsetの値を調整する。
		// write_offsetはu32型配列のオフセット値となるため、注意すること。
		write_offset += ( ubinux_alignment_size(register_def->copy_length) / sizeof(ukp_info->mmio_registers[0]) );

	}
}


/**
 * @brief 汎用レジスタ保存関数
 *        rma1上にある汎用レジスタをカーネルパニック情報格納領域へ保存する。
 */
static void ubinux_panic_save_general_register(void)
{
#ifdef CONFIG_ARM
	register unsigned int* current_sp asm ("r13");	// sp
	register unsigned int* current_pc asm ("r15");	// pc

	// r13,r15以外をスタックに積む
	asm("stmfd   sp!, {r0-r12,r14}");

	// 汎用レジスタの内容をコピー r0 - r15
	ubinux_copy_general_register(current_sp, current_pc);

	// 元に戻す
	asm("ldmfd   sp!, {r0-r12,r14}");

	ukp_info->header.status |= UBINUX_GENERAL_REGISTER_AVAILABLE;

	return;
#elif defined CONFIG_X86
	// not support
	return;
#else
	// not support
	return;
#endif
}


/**
 * @brief 汎用レジスタコピー関数
 *        スタックに積んだ汎用レジスタと引数のsp, pcを格納領域へ保存する。
 *        この関数は、インラインasmに挟まれて呼ばれるので、static宣言は行わない。
 *        また、関数宣言の方で、最適化のよるインライン展開の抑止も行っている。
 *
 * @param[in]  dest      spのアドレス
 * @param[in]  src       pcのアドレス
 */
void ubinux_copy_general_register(unsigned int *sp, unsigned int *pc)
{
	int i;
	for ( i = 0; i <= 12; ++i) {
		ukp_info->general_registers[i] = sp[i];
	}

	// 引数のspは、スタックに積んだ後のspなので、積む前の状態のspに戻して格納する。
	ukp_info->general_registers[13] = (unsigned int)sp + (4 * 14);

	ukp_info->general_registers[14] = sp[i];
	ukp_info->general_registers[15] = (unsigned int)pc;
}

/**
 * @brief magic_noチェック関数
 *
 * @retval 0  正常終了(magic_no/tail_magic_noに異常なし)
 * @retval 負 異常終了
 */
int ubinux_kernel_panic_info_check_magic_no(void)
{
	if ((ukp_info->magic_no != LITES_MGMT_MAGIC_NO) ||
	    (ukp_info->tail_magic_no != LITES_MGMT_TAIL_MAGIC_NO)) {
		return -1;
	}
	return 0;
}

/**
 * @brief カーネルパニック情報格納領域の初期化関数
 *
 * @retval 0  正常終了
 */
int ubinux_kernel_panic_info_initialize(void)
{
	memset(ukp_info, 0, sizeof(UBINUX_KERNEL_PANIC_INFO));
	ukp_info->magic_no = LITES_MGMT_MAGIC_NO;
	ukp_info->tail_magic_no = LITES_MGMT_TAIL_MAGIC_NO;
	return 0;
}





/**
 * @brief Panic notifier list変数に、ubinux_panic_save関数を登録するための変数。
 */
static struct notifier_block ubinux_panic_notifier = {
	.notifier_call = ubinux_panic_save,
};



#ifdef LITES_FTEN_MM_DIRECO
/**
 * @brief kernel panic情報の初期化用関数
 *   early_ubinux_kernel_panic_info関数で解析したパラメータを元に管理情報領域マップし、
 *   状況に応じて初期化する。
 *
 * @retval 0       正常終了（またはカーネル起動パラメータの解析が完了していない）
 * @retval -ENOMEM kernel panic情報格納領域のmapに失敗した
 * @retval -EINVAL サイズが不正
 */
static __init int ubinux_kernel_panic_info_init(void)
{
	UBINUX_KERNEL_PANIC_INFO *ukpi;
	int	region;
	u64 kernel_panic_info_start;
	u32 kernel_panic_info_size;

	// 変数の初期化
	kernel_panic_info_init_done = 0;// kernel panic情報初期化処理未実施
	ukp_info = NULL;

	// 定義から管理情報の設定を読み出す
	kernel_panic_info_start	= LITES_UKPI_REGION_ADDR;
	kernel_panic_info_size	= LITES_UKPI_REGION_SIZE;

	if (unlikely(kernel_panic_info_size < sizeof(UBINUX_KERNEL_PANIC_INFO))) {
		lites_pr_err("kernel_panic_info_size error (size = %08x)", kernel_panic_info_size);
		return -EINVAL;
	}

	// kernel panic情報格納領域を指定してioremapする
	ukpi = ubinux_kernel_panic_info_alloc(kernel_panic_info_start, kernel_panic_info_size);
	if (unlikely(ukpi == NULL)) {
		lites_pr_err("ubinux_kernel_panic_info_alloc error (addr = %016x, size = %08x)", kernel_panic_info_start, kernel_panic_info_size);
		return -ENOMEM;
	}

	// mapしたkernel panic情報格納領域内の情報の正当性を確認
	// 壊れてる、AC-offされたような形跡があるなら、初期化する
	if (ubinux_kernel_panic_info_check_magic_no() < 0) {
		// 電源OFF or kernel panic情報が壊れてる形跡があるので、初期化
		ubinux_kernel_panic_info_initialize();
		lites_pr_info("RAS(ubinux_kernel_panic_info) is restarted. addr=%llx", kernel_panic_info_start);
	} else {
		lites_pr_info("RAS(ubinux_kernel_panic_info) is continued. addr=%llx", kernel_panic_info_start);
	}

	// あらかじめkernel panic情報採取時にレジスタが保存できるよう、
	// レジスタマップ領域をioremapしておく
	for ( region = 0; region < ARRAY_SIZE(region_list); region ++ ) {

		if ( unlikely(region_list[region].size == UBINUX_PANIC_INFO_NOP) ) {
			// sizeがUBINUX_PANIC_INFO_NOP(0)の場合はNOP
			region_pointer[region] = NULL;
			lites_pr_info("region_list[%d] is nop.", region);
			continue;
		}

		region_pointer[region] = ioremap_nocache( region_list[region].phys_addr,
												  region_list[region].size );
		if ( region_pointer[region] == NULL ) {
			lites_pr_err("register_region(%d) ioremap fail\n", region);
		}
	}

/* FUJITSU-TEN 2013-09-11:ADA LOG mod start */
#if 1
	// kernel panic情報採取関数を登録
	atomic_notifier_chain_register(&panic_notifier_list, &ubinux_panic_notifier);
#endif
/* FUJITSU-TEN 2013-09-11:ADA LOG mod end */

	kernel_panic_info_init_done = 1;	// kernel panic情報初期化処理実施済み

	return 0;
}
pure_initcall(ubinux_kernel_panic_info_init);

#else	// LITES_FTEN_MM_DIRECO
/**
 * @brief kernel panic情報の初期化用関数
 *   early_ubinux_kernel_panic_info関数で解析したパラメータを元に管理情報領域マップし、
 *   状況に応じて初期化する。
 *
 * @retval 0       正常終了（またはカーネル起動パラメータの解析が完了していない）
 * @retval -ENOMEM kernel panic情報格納領域のmapに失敗した
 * @retval -EINVAL サイズが不正
 */
static __init int ubinux_kernel_panic_info_init(void)
{
	UBINUX_KERNEL_PANIC_INFO *ukpi;
	int	region;

	// 変数の初期化
	kernel_panic_info_init_done = 0;// kernel panic情報初期化処理未実施
	ukp_info = NULL;

	if (unlikely(!kernel_panic_info_parse_done)) {
		lites_pr_info("ubinux_kernel_panic is not specified, it is invalidated.");
		return -EINVAL;
	}

	if (unlikely(kernel_panic_info_size < sizeof(UBINUX_KERNEL_PANIC_INFO))) {
		lites_pr_err("kernel_panic_info_size error (size = %08x)", kernel_panic_info_size);
		return -EINVAL;
	}

	// kernel panic情報格納領域を指定してioremapする
	ukpi = ubinux_kernel_panic_info_alloc(kernel_panic_info_start, kernel_panic_info_size);
	if (unlikely(ukpi == NULL)) {
		lites_pr_err("ubinux_kernel_panic_info_alloc error (addr = %016x, size = %08x)", kernel_panic_info_start, kernel_panic_info_size);
		return -ENOMEM;
	}

	// mapしたkernel panic情報格納領域内の情報の正当性を確認
	// 壊れてる、AC-offされたような形跡があるなら、初期化する
	if (ubinux_kernel_panic_info_check_magic_no() < 0) {
		// 電源OFF or kernel panic情報が壊れてる形跡があるので、初期化
		ubinux_kernel_panic_info_initialize();
		lites_pr_info("RAS(ubinux_kernel_panic_info) is restarted");
	} else {
		lites_pr_info("RAS(ubinux_kernel_panic_info) is continued");
	}

	// あらかじめkernel panic情報採取時にレジスタが保存できるよう、
	// レジスタマップ領域をioremapしておく
	for ( region = 0; region < ARRAY_SIZE(region_list); region ++ ) {

		if ( unlikely(region_list[region].size == UBINUX_PANIC_INFO_NOP) ) {
			// sizeがUBINUX_PANIC_INFO_NOP(0)の場合はNOP
			region_pointer[region] = NULL;
			lites_pr_info("region_list[%d] is nop.", region);
			continue;
		}

		region_pointer[region] = ioremap_nocache( region_list[region].phys_addr,
												  region_list[region].size );
		if ( region_pointer[region] == NULL ) {
			lites_pr_err("register_region(%d) ioremap fail\n", region);
		}
	}

	// kernel panic情報採取関数を登録
	atomic_notifier_chain_register(&panic_notifier_list, &ubinux_panic_notifier);


	kernel_panic_info_init_done = 1;	// kernel panic情報初期化処理実施済み

	return 0;
}
pure_initcall(ubinux_kernel_panic_info_init);

/**
 * @brief kernel panic情報のカーネル起動パラメータ解析用関数
 * 　カーネル起動パラメータに含まれる"ubinux_kernel_panic="に続く文字列を解析する。
 * ubinux_kernel_panic_info_alloc関数が動作可能になるのはしばらく後なので、
 * パラメータ解析以降の処理はubinux_kernel_panic_info_initで実行する。

 * @param[in]  arg "ubinux_kernel_panic="に続く文字列
 *
 * @retval 0  解析に成功した
 * @retval -1 解析に失敗した
 */
static __init int early_ubinux_kernel_panic_info(char *arg)
{
	int ret;

	kernel_panic_info_parse_done = 0;

	ret = parse_addr(&arg, &kernel_panic_info_start);
	if (ret < 0) {
		lites_pr_err("parse_addr (arg = %s, ret = %d)", arg, ret);
		return ret;
	}

	ret = parse_size(&arg, &kernel_panic_info_size);
	if (ret < 0) {
		lites_pr_err("parse_size (arg = %s, ret = %d)", arg, ret);
		return ret;
	}

	/** 余分な文字がある場合も解析を失敗したことにする */
	if (!is_delimiter(*arg)) {
		lites_pr_err("is_delimiter (arg = %s)", arg);
		return -EINVAL;
	}

	lites_pr_dbg("kernel_panic_info_start = %llx, kernel_panic_info_size = %x",
		     kernel_panic_info_start, kernel_panic_info_size);

	kernel_panic_info_parse_done = 1;
	return 0;
}
early_param("ubinux_kernel_panic", early_ubinux_kernel_panic_info);
#endif	// LITES_FTEN_MM_DIRECO

#endif /* LITES_FTEN */
