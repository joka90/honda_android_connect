/**
 * @file syslog_init.c
 *
 * @brief syslogリージョン初期化処理の実装
 *
 * Copyright (c) 2012 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/lites_trace.h>
#include "region.h"
#include "mgmt_info.h"


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




/**
 * @brief syslogリージョンの初期化用関数
 *   コンフィグレーションで指定したモデルに対応するMAPでsyslogリージョンを初期化する。
 *
 * @retval 0       正常終了
 * @retval -ENOMEM 領域のmapに失敗した
 * @retval -EINVAL 設定値等が不正
 * @retval -EPERM  管理情報なし
 */
static __init int lites_syslog_init(void)
{
	static const char syslog_path[] = LITES_MESSAGES_PATH;
	static const int syslog_psize = ARRAY_SIZE(syslog_path) - 1;
	struct lites_region *region;
	int ret;
	int syslog_continue;
	u64 syslog_addr64;
	u32 syslog_size, syslog_level, syslog_access_byte, syslog_wrap;

	// 管理情報の初期化処理（確保）が出来てないので、高速ログへの出力は実施しない
	if (unlikely(mgmt_info_init_done == 0)) {
		lites_pr_err("syslog output is not changed to LITES. mgmt_info is not initialized.");
		return -EPERM;
	}

	// 定義からsyslogリージョンの設定を読み出す
	syslog_addr64		= LITES_SYSLOG_REGION_ADDR;
	syslog_size			= LITES_SYSLOG_REGION_SIZE;
	syslog_level		= LITES_SYSLOG_REGION_LEVEL;
	syslog_access_byte	= LITES_SYSLOG_REGION_ACCESS_BYTE;
	syslog_wrap			= LITES_SYSLOG_REGION_WRAP;

	// 管理情報内をチェックし、リージョンの継続が可能かどうかチェックする
	syslog_continue = lites_mgmt_info_check(LITES_SYSLOG_REGION,
											syslog_addr64 >> 32, syslog_addr64,
											syslog_size, syslog_level,
											syslog_access_byte, syslog_wrap);
	if (unlikely(syslog_continue < 0)) {
		return syslog_continue;
	} else if (syslog_continue == 1) {
		lites_pr_info("LITES(syslog) is continued. addr=%llx", syslog_addr64);
	} else {
		lites_pr_info("LITES(syslog) is restarted. addr=%llx", syslog_addr64);
	}

	ret = lites_check_region(syslog_addr64, syslog_size,
				 syslog_level, syslog_access_byte);
	if (unlikely(ret < 0)) {
		lites_pr_err("lites_check_region (ret = %d)", ret);
		return -EINVAL;
	}

	/* トレース初期化 or トレース継続 */
	region = lites_alloc_region(syslog_addr64, syslog_size, syslog_psize, syslog_continue);
	if (unlikely(region == NULL)) {
		lites_pr_err("lites_alloc_region");
		return -ENOMEM;
	}

	region->rb->addr		= syslog_addr64;
	region->rb->size		= syslog_size;
	region->rb->region		= LITES_SYSLOG_REGION;
	region->rb->level		= syslog_level;
	region->rb->access_byte	= syslog_access_byte;
	region->rb->wrap		= syslog_wrap;

	region->header_size	= LITES_COMMON_LOG_HEADER_STRUCT_SIZE;
	region->psize		= syslog_psize;
	strncpy(region->path, syslog_path, syslog_psize);
	region->total		= syslog_size - offsetof(struct lites_ring_buffer, log);

	if (region->rb->access_byte != LITES_VAR_ACCESS_BYTE) {
		region->total	-= region->total % region->rb->access_byte;
		region->lop		= &lites_const_lop;
	} else {
		region->lop		= &lites_var_lop;
	}

	init_region_lock(region);

	ret = lites_register_region(region);
	if (unlikely(ret < 0)) {
		lites_pr_err("lites_register_region (ret = %d)", ret);
		lites_free_region(region);
		return -ENOMEM;
	}

	return 0;
}
pure_initcall(lites_syslog_init);

#endif /* LITES_FTEN_MM_DIRECO */
