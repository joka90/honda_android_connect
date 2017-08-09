/**
 * @file trace_init.c
 *
 * @brief トレースリージョン初期化処理の実装
 *
 * Copyright (c) 2012 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/lites_trace.h>
#include "region.h"
#include "mgmt_info.h"
#include "trace_write.h"


#ifdef LITES_FTEN_MM_DIRECO

// コンフィグレーションで指定したモデルに対応するMAPで
// トレースグループ定義を読み込む
#ifdef CONFIG_MACH_FTENBB
#include <linux/lites_trace_map.h>
#else	// CONFIG_MACH_FTENBB
// テスト用定義の読み込む
// #define	FTENBB_RAS_REGION_ADDR			0x20000000
// #include "test_lites_trace_map.h"
// ADA用ベースアドレスはlites_trace_map.hで定義
#include <linux/lites_trace_map.h>
#endif	// CONFIG_MACH_FTENBB

// トレースグループの定義
static struct lites_group_def	trace_group_list[] = LITES_TRACE_GROUP_LIST;

// 内部用関数宣言
static void lites_trace_get_trace_region_addr(u32 group_id, u32 index, unsigned int *addr_hi, unsigned int *addr_low);
static __init int lites_trace_init(void);


/**
 * @brief トレースリージョンのアドレス取得関数
 *
 * @param[in]      group_id  グループID
 * @param[in]      index     トレースグループ内のリージョンindex(0～)
 * @param[in/out]  addr_hi   トレースリージョンの64bitアドレスの上位32ビットの格納先
 * @param[in/out]  addr_low  トレースリージョンの64bitアドレスの下位32ビットの格納先
 */
static void lites_trace_get_trace_region_addr(u32 group_id, u32 index, u32 *addr_hi, u32 *addr_low)
{
	u64 addr_64 = trace_group_list[group_id].region_address + (trace_group_list[group_id].region_size * index);
	*addr_hi = (u32)(addr_64 >> 32);
	*addr_low = (u32)(addr_64);
}

/**
 * @brief トレースリージョンの初期化用関数
 *   コンフィグレーションで指定したモデルに対応するMAPでトレースグループ定義が
 *   展開されているので、そこに設定されているアドレス/サイズ/リージョン数で
 *   トレースグループの各リージョンを初期化する。
 *
 * @retval 0       正常終了
 * @retval -ENOMEM 領域のmapに失敗した
 * @retval -EINVAL 設定値等が不正
 * @retval -EPERM  管理情報なし
 */
static __init int lites_trace_init(void)
{
	static const char trace_path[] = LITES_TRACE_PATH;
	static const int trace_psize = ARRAY_SIZE(trace_path) - 1;
	struct lites_region *region;
	u32 addr_hi = 0;
	u32 addr_low = 0;
	u64 addr64 = 0;
	int i, j, ret;
	int trace_continue;
	u32 region_no;
	int loop_max;

	lites_pr_dbg("lites_trace_init");

	// lites_trace_write_lock関連の初期化
	lites_trace_init_done = 0;
	init_lites_trace_write_lock();
	init_waitqueue_head(&lites_trace_region_wait_queue);

	// 管理情報の初期化処理（確保）が出来てないので、高速ログへの出力は実施しない
	if (mgmt_info_init_done == 0) {
		lites_pr_err("trace output is not changed to LITES. mgmt_info is not initialized.");
		return -EPERM;
	}

	// 管理情報に記録されてるリージョン情報と指定されたリージョン情報をチェック
	// 各リージョン毎のチェック
	trace_continue = 1;	// 初期値：継続

	// トレースグループ定義は配列で定義されており、グループIDが配列のindexとしている。
	// index0は使わないが、配列数として含まれるので注意すること。
	loop_max = ARRAY_SIZE(trace_group_list);
	if (unlikely(ARRAY_SIZE(trace_group_list) > (LITES_TRACE_GROUP_MAX + 1))) {
		lites_pr_info("The number of group Definitions exceeds the group maxima.");
		loop_max = LITES_TRACE_GROUP_MAX + 1;
	}

	lites_pr_info("The number of group : %lx.", loop_max);

	// トレースグループ定義[1]から順次参照した情報を用いて
	// 管理情報内のリージョン情報のチェック/登録を行う。
	region_no = 0;
	for (i = 1; i < loop_max; ++i) {
		for (j = 0; j < trace_group_list[i].region_num; ++j) {
			// 管理情報内をチェックし、リージョンの継続が可能かどうかチェックする
			// リージョン情報が新規/更新されてる場合は、管理情報への登録も行う
			lites_trace_get_trace_region_addr(i, j, &addr_hi, &addr_low);
			ret = lites_mgmt_info_check(region_no,
						addr_hi,		// 64bitアドレスの上位32bit
						addr_low,		// 64bitアドレスの下位32bit
						trace_group_list[i].region_size,
						trace_group_list[i].region_level,
						trace_group_list[i].region_access_byte,
						trace_group_list[i].region_wrap);
			if (unlikely(ret < 0)) {
				// 異常メッセージは、lites_mgmt_info_check関数内で出力済み
				return -EINVAL;
			} else if (ret == 0) {
				// １つでも不一致があった場合は、トレース継続無効とする
				trace_continue = 0;
			}

			++region_no;
		}
	}

	if (ARRAY_SIZE(trace_group_list) > 1) {
		addr64 = trace_group_list[1].region_address;
	}

	if (trace_continue == 1) {
		lites_pr_info("LITES(trace) is continued. addr=%llx", addr64);
	} else {
		// トレースグループ定義[1]から順次参照した情報を用いて
		// 管理情報のグループ情報を更新する
		region_no = 0;
		for (i = 1; i < loop_max; ++i) {
			ret = lites_mgmt_info_add_group_info(i, 
					trace_group_list[i].region_num,
					region_no,
					region_no,
					region_no + trace_group_list[i].region_num - 1);
			if (unlikely(ret < 0)) {
				// 異常メッセージは、lites_mgmt_info_check関数内で出力済み
				return -EINVAL;
			}
			region_no += trace_group_list[i].region_num;
		}

		lites_pr_info("LITES(trace) is restarted. addr=%llx", addr64);
	}


	// トレースグループ定義[1]から順次参照した情報を用いて
	// トレースリージョンを登録する
	region_no = 0;
	for (i = 1; i < loop_max; ++i) {
		for (j = 0; j < trace_group_list[i].region_num; ++j) {
			// 配置アドレスは、グループ内のリージョン毎にことなるので、算出する
			lites_trace_get_trace_region_addr(i, j, &addr_hi, &addr_low);
			addr64 = ( ((u64)addr_hi << 32) | ((u64)addr_low) );
			ret = lites_check_region(addr64, 
						trace_group_list[i].region_size,
						trace_group_list[i].region_level,
						trace_group_list[i].region_access_byte);
			if (unlikely(ret < 0)) {
				lites_pr_err("lites_check_region (ret = %d)", ret);
				return -EINVAL;
			}

			// トレース初期化 or トレース継続
			region = lites_alloc_region(addr64, trace_group_list[i].region_size, trace_psize, trace_continue);
			if (unlikely(region == NULL)) {
				lites_pr_err("lites_alloc_region (group = %d, index = %d)", i, j);
				return -ENOMEM;
			}

			region->rb->addr        = addr64;
			region->rb->size        = trace_group_list[i].region_size;
			region->rb->region      = region_no;
			region->rb->level       = trace_group_list[i].region_level;
			region->rb->access_byte = trace_group_list[i].region_access_byte;
			region->rb->wrap        = trace_group_list[i].region_wrap;

			region->header_size = LITES_COMMON_LOG_HEADER_STRUCT_SIZE;
			region->psize       = trace_psize;
			region->total       = trace_group_list[i].region_size - offsetof(struct lites_ring_buffer, log);
			strncpy(region->path, trace_path, trace_psize);

			if (region->rb->access_byte != LITES_VAR_ACCESS_BYTE) {
				region->total -= region->total % region->rb->access_byte;
				region->lop    = &lites_const_lop;
			} else {
				region->lop = &lites_var_lop;
			}
			init_region_lock(region);

			ret = lites_register_region(region);
			if (unlikely(ret < 0)) {
				lites_pr_err("lites_register_region (group = %d, index = %d, ret = %d)", i, j, ret);
				lites_free_region(region);
				return -ENOMEM;
			}

			++region_no;
		}
	}

	lites_trace_init_done = 1;	// トレースリージョン初期化完了

	return 0;
}
pure_initcall(lites_trace_init);

#endif /* LITES_FTEN_MM_DIRECO */
