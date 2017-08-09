/**
 * @file kernel_panic_info.h
 *
 * @brief kernel panic情報APIの定義
 *
 * Copyright (c) 2011-2012 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#ifndef __LITES_KERNEL_PANIC_INFO_H
#define __LITES_KERNEL_PANIC_INFO_H


#ifdef LITES_FTEN

#include <linux/lites_trace.h>


extern int kernel_panic_info_init_done;		//!< kernel panic情報の初期化処理実施済みかどうかを示す



/** kernel panic情報API（kernel_panic_info.cで定義）*/


/**
 * @brief kernel panic情報格納領域獲得用関数
 *
 * @param[in]  addr kernel panic情報格納領域開始アドレス
 * @param[in]  size kernel panic情報格納領域情報のサイズ
 *
 * @retval 管理情報 正常終了
 * @retval NULL     異常終了
 */
extern void *ubinux_kernel_panic_info_alloc(u64 addr, u32 size);

/**
 * @brief kernel panic情報格納領域解放用関数
 */
extern void ubinux_kernel_panic_info_free(void);

/**
 * @brief magic_noチェック関数
 *
 * @retval 0  正常終了(magic_no/tail_magic_noに異常なし)
 * @retval 負 異常終了
 */
extern int ubinux_kernel_panic_info_check_magic_no(void);

/**
 * @brief カーネルパニック情報格納領域の初期化関数
 *
 * @retval 0  正常終了
 */
extern int ubinux_kernel_panic_info_initialize(void);



#endif /* LITES_FTEN */

#endif  /** __LITES_KERNEL_PANIC_INFO_H */
