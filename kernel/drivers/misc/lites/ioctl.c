/**
 * Copyright (c) 2011-2013 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
/**
 * @file ioctl.c
 *
 * @brief ioctlのコマンドの定義
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dcache.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/mm.h>
#include <linux/lites_trace.h>
#include <linux/lites_trace_map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "region.h"
#include "ioctl.h"
#include "buffer.h"
#ifdef LITES_FTEN
#include "mgmt_info.h"
#include "trace_write.h"
#include "kernel_panic_info.h"
#include <linux/sched.h>
#ifdef CONFIG_MACH_FTENNBB
#include <linux/ftenbb_config.h>	/* モデル別定義の読み出し for UBINUX_KERNEL_PANIC_INFO */
#else	/* CONFIG_MACH_FTENNBB */
#include <linux/ubinux_panic_info_customized.h>

/* FUJITSU-TEN 2013-11-20 :LITES AREA DUMP add start */
#include <linux/types.h>
#include <linux/pagemap.h>
#include <linux/genhd.h>
#include <linux/fs.h>


/** ダンプ対象となるRAM上のLITES用メモリ領域情報 */
#define LITES_DUMP_RAM_ADDR		FTENBB_RAS_REGION_ADDR
#define LITES_DUMP_RAM_SIZE		FTENBB_RAS_REGION_SIZE

/** ダンプ先となるeMMC上の情報 */
#define LITES_DUMP_BLOCK_DEV 		"mmcblk0"
#define LITES_DUMP_PART_NAME 		"ALG"

#define LOCAL static
/* FUJITSU-TEN 2013-11-20 :LITES AREA DUMP add end   */

/** カーネルパニック情報内の領域サイズ定義 */
#define UBINUX_PANIC_MSG_SIZE		UBINUX_PANIC_MSG_SIZE_COMMON
#define UBINUX_KERNEL_STACK_SIZE	UBINUX_KERNEL_STACK_SIZE_COMMON
#define UBINUX_REGISTER_SIZE		UBINUX_REGISTER_SIZE_COMMON

/** kernel panic情報構造体 */
typedef UBINUX_KERNEL_PANIC_INFO_COMMON		UBINUX_KERNEL_PANIC_INFO;
#endif	/* CONFIG_MACH_FTENNBB */

#endif /* LITES_FTEN */

/**
 * @brief LITES_GET_ATTRコマンド用関数
 * 　リージョン番号で指定したリージョンの属性値を取得する。
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のcurrent_attribute構造体のアドレス
 *
 * @retval 0       正常終了した
 * @retval -EINVAL リージョン番号に該当するリージョンが存在しない
 * @retval -EFAULT コピーに失敗した
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_GET_ATTR)
{
	struct lites_region *region;
	struct current_attribute args;
	struct misc_attribute misc_attr;
	unsigned long flags = 0;

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	region = lites_find_region_with_number(args.region_no);
	if (region == NULL) {
		lites_pr_err("not initialized region (region_no = %08x)",
			     args.region_no);
		return -EPERM;
	}

	region_read_lock(region, flags);

	misc_attr.size             = region->rb->size;
	misc_attr.region_addr_high = (u32)(region->rb->addr >> 32);
	misc_attr.region_addr_low  = (u32)region->rb->addr;
	misc_attr.level            = region->rb->level;
	misc_attr.access_byte      = region->rb->access_byte;
	misc_attr.pid              = region->rb->pid;
	misc_attr.sig_no           = region->rb->sig_no;
	misc_attr.trgr_lvl         = region->rb->trgr_lvl;
	misc_attr.block_rd         = region->rb->block_rd;
	misc_attr.wrap             = region->rb->wrap;
	/** T.B.D: Struct member name is different */
	misc_attr.cancel_no        = region->rb->cancel_times;

	region_read_unlock(region, flags);

	if (copy_to_user(args.misc_attr, &misc_attr, sizeof(misc_attr))) {
		lites_pr_err("copy_to_user");
		return -EFAULT;
	}

	return 0;
}

/**
 * @brief LITES_CHG_LVLコマンド用関数
 * 　リージョン番号で指定したリージョンの登録レベルを変更する。
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のchange_level構造体
 *
 * @retval 0       正常終了した
 * @retval -EFAULT コピーに失敗した
 * @retval -EINVAL 不正な引数が指定された
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_CHG_LVL)
{
	struct lites_region *region;
	struct change_level args;

#ifdef LITES_FTEN
	int i;
	lites_pr_dbg("DEFINE_LITES_COMPAT_IOCTL(LITES_CHG_LVL)");
#endif /* LITES_FTEN */

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	if (!check_level_value(args.level)) {
		lites_pr_err("level range (level = %u)", args.level);
		return -EINVAL;
	}

#ifdef LITES_FTEN
	if (args.region == LITES_TRACE_REGIONS) {
		// 全てのトレースリージョンのレベルを変更する
		for (i = 0; i < lites_mgmt_info_get_trace_region_num(); ++i) {
			region = lites_find_region_with_number(i);
			if (region == NULL) {
				lites_pr_err("not initialized region");
				return -EPERM;
			}

			region->rb->level = args.level;
		}
	} else {
		// 指定リージョンのみのレベルを変更する
		region = lites_find_region_with_number(args.region);
		if (region == NULL) {
			lites_pr_err("not initialized region");
			return -EPERM;
		}

		region->rb->level = args.level;
	}
#else /* LITES_FTEN */
	region = lites_find_region_with_number(args.region);
	if (region == NULL) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	region->rb->level = args.level;
#endif /* LITES_FTEN */

	return 0;
}

/**
 * @brief LITES_CHG_LVL_ALLコマンド用関数
 * 　全てのリージョンの登録レベルを変更する。
 *
 * @param[in]  file 未使用
 * @param[in]  addr 登録レベルの値
 *
 * @retval 0       正常終了した
 * @retval -EINVAL 登録レベルの値が不正である
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_CHG_LVL_ALL)
{
	int i;
	unsigned int level = (unsigned int)get_arg2();

	if (!check_level_value(level)) {
		lites_pr_err("level range (level = %u)", level);
		return -EINVAL;
	}

	regions_read_lock();
	for (i = 0; i < regions_nr; i++)
		regions[i]->rb->level = level;
	regions_read_unlock();

	return 0;
}

/**
 * @brief LITES_INQ_LVLコマンド用関数
 * 　リージョン番号で指定したリージョンの登録レベルを取得する。
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のget_level構造体
 *
 * @retval 0 正常終了した
 * @retval -EFAULT コピーに失敗した
 * @retval -EINVAL 不正な引数が指定された
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_INQ_LVL)
{
	struct lites_region *region;
	struct get_level args;
	unsigned int level;

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	region = lites_find_region_with_number(args.region);
	if (region == NULL) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	level = region->rb->level;

	if (copy_to_user(args.level, &level, sizeof(*args.level))) {
		lites_pr_err("copy_to_user");
		return -EFAULT;
	}

	return 0;
}

/**
 * @brief LITES_NTFY_SIGコマンド用関数
 * 　リージョン番号で指定したリージョンのシグナル通知機能を有効にする。
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のnotify_signal構造体
 *
 * @retval 0 正常終了した
 * @retval -EFAULT コピーに失敗した
 * @retval -EINVAL 不正な引数が指定された
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_NTFY_SIG)
{
	unsigned long flags = 0;
	struct lites_region *region;
	struct notify_signal args;

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	region = lites_find_region_with_number(args.region);
	if (region == NULL) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	lites_pr_dbg("args.pid = %08x, args.sig_no = %08x, args.trgr_lvl = %08x",
		     (u32)args.pid, (u32)args.sig_no, (u32)args.trgr_lvl);

	region_write_lock(region, flags);

	/** Signal handler is used with PID and SIGNO, if 2 process are racing
	 * here, they will not know SIGNO that other has. */
	region->rb->pid      = args.pid;
	region->rb->sig_no   = args.sig_no;
	region->rb->trgr_lvl = args.trgr_lvl;

	region_write_unlock(region, flags);

	return 0;
}

/**
 * @brief LITES_STOP_SIGコマンド用関数
 * 　リージョン番号で指定したリージョンのシグナル通知機能を無効にする。
 *
 * @param[in]  file 未使用
 * @param[in]  addr リージョン番号
 *
 * @retval 0       正常終了した
 * @retval -EINVAL 不正な引数が指定された
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_STOP_SIG)
{
	unsigned long flags = 0;
	struct lites_region *region;

	region = lites_find_region_with_number(get_arg2());
	if (region == NULL) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	region_write_lock(region, flags);

	region->rb->pid       = 0;
	region->rb->sig_no    = 0;
	region->rb->trgr_lvl  = 0;
	region->rb->sig_times = 0;

	region_write_unlock(region, flags);

	return 0;
}

/**
 * @brief LITES_INQ_SIGコマンド用関数
 * 　シグナル通知元のリージョン番号を取得する。複数のリージョンからシグナルを通
 * 知された場合もあるので、複数のリージョン番号を渡す処理になっている。シグナル
 * 通知元の総数がユーザが用意したバッファに満たない場合はバッファを0でパディング
 * する。本コマンドを実行後にリージョンからデータを読み込むことを想定している。
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のinq_signal_region構造体
 *
 * @retval 0       正常終了した
 * @retval -EFAULT コピーに失敗した
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_INQ_SIG)
{
	struct inq_signal_region args;
	unsigned long flags = 0;
	unsigned int region[LITES_REGIONS_NR_MAX];
	int i, j = 0;

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	memset(region, 0, sizeof(region));

	regions_read_lock();

	for (i = 0; i < regions_nr; i++) {

		region_read_lock(regions[i], flags);

		if (regions[i]->rb->sig_times) {
			region[j++] = regions[i]->rb->region;
			if (args.tbl_clr)
				regions[i]->rb->sig_times = 0;
		}

		region_read_unlock(regions[i], flags);

	}

	regions_read_unlock();

	args.num = j;
	if (copy_to_user(args.region, region, sizeof(*region) * j)) {
		lites_pr_err("copy_to_user");
		return -EFAULT;
	}

	if (copy_to_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_to_user_addr");
		return -EFAULT;
	}

	return 0;
}

/**
 * @brief LITES_CLR_SIG_TBLコマンド用関数
 * 　シグナル通知元のフラグをリセットする。本フラグをリセットすることで、再びシ
 * グナル通知機能が動作するようになる。
 *
 * @param[in]  file 未使用
 * @param[in]  addr 未使用
 *
 * @retval 0 常に正常終了
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_CLR_SIG_TBL)
{
	int i;

	regions_read_lock();

	for (i = 0; i < regions_nr; i++)
		regions[i]->rb->sig_times = 0;

	regions_read_unlock();

	return 0;
}

/**
 * @brief LITES_INQ_OVERコマンド用関数
 * 　abc
 *
 * @param[in]  file 未使用
 * @param[in]  addr 未使用
 *
 * @retval 0       正常に終了した
 * @retval -EFAULT コピーに失敗した
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_INQ_OVER)
{
	struct lites_region *region;
	struct overflow_detail args;
	unsigned int cancel_times;

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	region = lites_find_region_with_number(args.region);
	if (region == NULL) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	cancel_times = region->rb->cancel_times;
	if (args.clr)
		region->rb->cancel_times = 0;

	if (copy_to_user(args.num, &cancel_times, sizeof(*args.num))) {
		lites_pr_err("copy_to_user");
		return -EFAULT;
	}

	return 0;
}

/**
 * @brief LITES_CLR_DATAコマンド用関数
 * 　リージョン番号で指定したリージョンのログを消去する。
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のclear_region構造体
 *
 * @retval 0       正常終了した
 * @retval -EINVAL 不正な引数が指定された
 * @retval -EFAULT コピーに失敗した
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_CLR_DATA)
{
	unsigned long flags = 0;
	struct lites_region *region;
	struct clear_region args;
	u32 region_no;
	int i;

#ifdef LITES_FTEN
	lites_pr_dbg("DEFINE_LITES_COMPAT_IOCTL(LITES_CLR_DATA)");
#endif /* LITES_FTEN */

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	for (i = 0; i < args.num; i++) {
		if (copy_from_user(&region_no, &args.region[i], sizeof(region_no))) {
			lites_pr_err("copy_from_user");
			return -EFAULT;
		}

		region = lites_find_region_with_number(region_no);
		if (region == NULL) {
			lites_pr_err("not initialized region");
			return -EPERM;
		}

		region_write_lock(region, flags);

		region->rb->read     = 0;
		region->rb->read_sq  = 0;
		region->rb->write    = 0;
		region->rb->flags   &= ~(LITES_FLAG_READ | LITES_FLAG_READ_SQ);

		region_write_unlock(region, flags);
	}

	return 0;
}

/**
 * @brief LITES_CLR_DATA_ZEROコマンド用関数
 * 　リージョン番号で指定したリージョンのログをゼロクリアする。
 *
 * @param[in]  file 未使用
 * @param[in]  addr リージョン番号
 *
 * @retval 0       正常終了した
 * @retval -EINVAL 不正な引数が指定された
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_CLR_DATA_ZERO)
{
	unsigned long flags = 0;
	struct lites_region *region;

#ifdef LITES_FTEN
	lites_pr_dbg("DEFINE_LITES_COMPAT_IOCTL(LITES_CLR_DATA_ZERO)");
#endif /* LITES_FTEN */

	region = lites_find_region_with_number(get_arg2());
	if (region == NULL) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	region_write_lock(region, flags);

	region->rb->read     = 0;
	region->rb->read_sq  = 0;
	region->rb->write    = 0;
	region->rb->flags   &= ~(LITES_FLAG_READ | LITES_FLAG_READ_SQ);

	memset(region->rb->log, 0, region->total);

	region_write_unlock(region, flags);

	return 0;
}

/**
 * @brief LITES_WRAPコマンド用関数
 * 　リージョン番号で指定したリーションの上書きを有効/無効にする。
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のwrap_condition構造体
 *
 * @retval 0       正常終了した
 * @retval -EFAULT コピーに失敗した
 * @retval -EINVAL 不正な引数が指定された
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_WRAP)
{
	struct lites_region *region;
	struct wrap_condition args;

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	region = lites_find_region_with_number(args.region);
	if (region == NULL) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	region->rb->wrap = args.cond;

	return 0;
}

/**
 * @brief LITES_BLOCKコマンド用関数
 * 　リージョン番号で指定したリーションのブロック機能を有効/無効にする。
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のblock_condition構造体
 *
 * @retval 0       正常終了した
 * @retval -EFAULT コピーに失敗した
 * @retval -EINVAL 不正な引数が指定された
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_BLOCK)
{
	struct lites_region *region;
	struct block_condition args;

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	region = lites_find_region_with_number(args.region);
	if (region == NULL) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	region->rb->block_rd = args.cond;

	return 0;
}

#ifndef LITES_FTEN_MM_DIRECO
/**
 * @brief パスのコピー用関数
 * 　ファイル構造体のパスをコピーする。
 *
 * @param[in]  file ファイル構造体
 * @param[out] path バッファ
 * @param[in]  size バッファのサイズ
 *
 * @retval 0  正常終了
 * @retval -1 パスの長さがバッファのサイズを上回った
 */
static int strcpy_lookup(struct file *file, char *path, size_t size)
{
	struct dentry *dentry = file->f_dentry;
	struct vfsmount *mnt = file->f_vfsmnt;
	char *p;

	p = path;
	while (1) {
		while (!IS_ROOT(dentry)) {
			if (p + dentry->d_name.len + 1 > path + size)
				return -1;
			memcpy(p, dentry->d_name.name, dentry->d_name.len);
			p[dentry->d_name.len] = '/';
			p += dentry->d_name.len + 1;
			dentry = dentry->d_parent;
		}

		if (mnt == mnt->mnt_parent)
			break;

		dentry = mnt->mnt_mountpoint;
		mnt = mnt->mnt_parent;
	}
	*p = '\0';

	lites_pr_dbg("path = %s", path);
	return 0;
}

/**
 * @brief パスの長さ取得用関数
 * 　ファイル構造体のパスの長さを取得する。
 *
 * @param[in] file ファイル構造体
 *
 * @retval 正の値 ファイル構造体のパスの長さ
 */
static u32 strlen_lookup(struct file *file)
{
	struct dentry *dentry = file->f_dentry;
	struct vfsmount *mnt = file->f_vfsmnt;
	u32 len = 0;

	while (1) {
		while (!IS_ROOT(dentry)) {
			len += dentry->d_name.len + 1;
			dentry = dentry->d_parent;
		}

		if (mnt == mnt->mnt_parent)
			break;

		dentry = mnt->mnt_mountpoint;
		mnt = mnt->mnt_parent;
	}

	return len;
}
#endif /* LITES_FTEN_MM_DIRECO */


/** T.B.D: need function for setting path and psize */
/**
 * @brief LITES_INITコマンド用関数
 * 　指定された属性値でリージョンを初期化する。その際、ファイル構造体のパスをリー
 * ジョンの属性値として設定する。
 *
 * @param[in]  file ファイル構造体
 * @param[in]  addr ユーザ空間のzone_attribute構造体
 *
 * @retval 0       正常終了
 * @retval -EFAULT コピーに失敗した
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_INIT)
{
#ifdef LITES_FTEN_MM_DIRECO
	// MMダイレコ対応により、トレースリージョンの初期化は、APIの指定ではなく、
	// ドライバ側で直接定義を参照し、トレースリージョンを初期化するように変更。
	// トレースリージョンの初期化は、別のところで実施するので、
	// 処理を実施せず、0で復帰するよう、修正

	return 0;

#else /* LITES_FTEN_MM_DIRECO */
	struct lites_region *region;
	struct zone_attribute args;
	struct region_attribute region_attr;
	u64 addr;
	u32 psize;
	int i, ret;
	int trace_continue;

	lites_pr_dbg("DEFINE_LITES_COMPAT_IOCTL(LITES_INIT)");

	// lites_trace_write_lock関連の初期化
	lites_trace_init_done = 0;
	init_lites_trace_write_lock();
    init_waitqueue_head(&lites_trace_region_wait_queue);

	// 管理情報の初期化処理（確保）が出来てないので、高速ログへの出力は実施しない
	if (mgmt_info_init_done == 0) {
		lites_pr_err("trace output is not changed to LITES. mgmt_info is not initialized.");
		return -EPERM;
	}

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	/* 管理情報に記録されてるリージョン情報と指定されたリージョン情報をチェック */
	/* 各リージョン毎のチェック */
	trace_continue = 1;	// 初期値：継続
	for (i = 0; i < args.total_regions; i++) {
		if (copy_from_user(&region_attr, &args.region_attr[i],
				   sizeof(struct region_attribute))) {
			lites_pr_err("copy_from_user (i = %d)", i);
			return -EFAULT;
		}

		// 管理情報内をチェックし、リージョンの継続が可能かどうかチェックする
		ret = lites_mgmt_info_check(i,
									region_attr.region_addr_high, region_attr.region_addr_low,
									region_attr.size, region_attr.level,
									region_attr.access_byte, region_attr.wrap);
		if (unlikely(ret < 0)) {
			return ret;
		} else if (ret == 0) {
			/* １つでも不一致があった場合は、トレース継続無効とする */
			trace_continue = 0;
		}
	}
	if (trace_continue == 1) {
		lites_pr_info("LITES(trace) is continued");
	} else {
		lites_pr_info("LITES(trace) is restarted");
	}


	for (i = 0; i < args.total_regions; i++) {
		if (unlikely(copy_from_user(&region_attr, &args.region_attr[i],
				   sizeof(struct region_attribute)))) {
			lites_pr_err("copy_from_user (i = %d)", i);
			return -EFAULT;
		}

		addr = ((u64)region_attr.region_addr_high << 32) | ((u64)region_attr.region_addr_low);
		psize = strlen_lookup(get_arg1());

		ret = lites_check_region(addr, region_attr.size,
					region_attr.level, region_attr.access_byte);
		if (unlikely(ret < 0)) {
			lites_pr_err("lites_check_region (ret = %d)", ret);
			return -EINVAL;
		}

		/* トレース初期化 or トレース継続 */
		region = lites_alloc_region(addr, region_attr.size, psize, trace_continue);
		if (unlikely(region == NULL)) {
			lites_pr_err("lites_alloc_region (i = %d)", i);
			return -ENOMEM;
		}

		if (unlikely(strcpy_lookup(get_arg1(), region->path, psize))) {
			lites_pr_err("strcpy_lookup (i = %d)", i);
			lites_free_region(region);
			return -EINVAL;
		}

		region->rb->addr        = addr;
		region->rb->size        = region_attr.size;
		region->rb->region      = i;
		region->rb->level       = region_attr.level;
		region->rb->access_byte = region_attr.access_byte;
		region->rb->wrap        = region_attr.wrap;

		region->header_size = LITES_COMMON_LOG_HEADER_STRUCT_SIZE;
		region->psize       = psize;
		region->total       = region_attr.size - offsetof(struct lites_ring_buffer, log);

		if (region->rb->access_byte != LITES_VAR_ACCESS_BYTE) {
			region->total -= region->total % region->rb->access_byte;
			region->lop    = &lites_const_lop;
		} else {
			region->lop = &lites_var_lop;
		}
		init_region_lock(region);

		ret = lites_register_region(region);
		if (unlikely(ret < 0)) {
			lites_pr_err("lites_register_region (i = %d, ret = %d)", i, ret);
			lites_free_region(region);
			return -ENOMEM;
		}
	}

	lites_trace_init_done = 1;	// トレースリージョン初期化完了

	return 0;
#endif /* LITES_FTEN_MM_DIRECO */
}

#ifdef LITES_FTEN_MM_DIRECO
static int __LITES_WRITE(unsigned int group_id, struct lites_trace_format *user_trc_fmt)
#else /* LITES_FTEN_MM_DIRECO */
static int __LITES_WRITE(unsigned int region, struct lites_trace_format *user_trc_fmt)
#endif /* LITES_FTEN_MM_DIRECO */
{
	EXTERN_LITES_BUFFER(__lites_write);
	struct lites_trace_format trc_fmt;
	struct lites_trace_header trc_header;
	struct lites_buffer *buffer = lites_buffer(__lites_write);
	size_t size;
	int ret;

	if (unlikely(copy_from_user(&trc_fmt, user_trc_fmt, sizeof(trc_fmt)))) {
		lites_pr_err("copy_from_user");
		return -EFAULT;
	}

	if (unlikely(copy_from_user(&trc_header, trc_fmt.trc_header,
				    sizeof(trc_header)))) {
		lites_pr_err("copy_from_user");
		return -EFAULT;
	}

	lites_buffer_lock(buffer);
	size = min_t(size_t, trc_fmt.count, buffer->size);

	lites_pr_dbg("buffer = %p, buffer->buffer = %p, "
		     "trc_fmt.trc_header = %p, trc_fmt.count = %u, "
		     "trc_fmt.buf = %p, size = %u",
		     buffer, buffer->buffer,
		     trc_fmt.trc_header, trc_fmt.count,
		     trc_fmt.buf, size);

	ret = -EFAULT;
	if (unlikely(copy_from_user(buffer->buffer, trc_fmt.buf, size))) {
		lites_pr_err("copy_from_user");
		goto ret;
	}

	trc_fmt.trc_header = &trc_header;
	trc_fmt.buf = buffer->buffer;
#ifdef LITES_FTEN_MM_DIRECO
	ret = lites_trace_write(group_id, &trc_fmt);
#else /* LITES_FTEN_MM_DIRECO */
	ret = lites_trace_write(region, &trc_fmt);
#endif /* LITES_FTEN_MM_DIRECO */
	if (unlikely(ret < 0))
		lites_pr_err("lites_trace_write (ret = %d)", ret);
ret:
	lites_buffer_unlock(buffer);
	return ret;
}

/**
 * @brief LITES_TRACE_WRITEコマンド用関数
 * 　リージョン番号で指定したリージョンのログを書き込む。さらに、
 * lites_trace_format構造体の書式を利用する。
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のlites_write_param構造体
 *
 * @retval 0以上   書き込んだログのサイズ
 * @retval -EFAULT コピーに失敗した
 * @retval -EINVAL 最大値よりも大きいアクセスバイトが指定された
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_WRITE)
{
	struct lites_write_param args;
	int ret;

#ifdef LITES_FTEN
	lites_pr_dbg("DEFINE_LITES_COMPAT_IOCTL(LITES_WRITE)");
#endif /* LITES_FTEN */

	if (unlikely(copy_from_user_addr(&args, sizeof(args)))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

#ifdef LITES_FTEN_MM_DIRECO
	ret = __LITES_WRITE(args.group_id, args.trc_fmt);
#else /* LITES_FTEN_MM_DIRECO */
	ret = __LITES_WRITE(args.region, args.trc_fmt);
#endif /* LITES_FTEN_MM_DIRECO */
	if (unlikely(ret < 0))
		lites_pr_err("__LITE_WRITE (ret = %d)", ret);

	return ret;
}

/**
 * @brief LITES_TRACE_WRITE_MULTIコマンド用関数
 * 　複数のリージョン番号で指定したリージョンのログを書き込む。さらに、
 * lites_trace_format構造体の書式を利用する。
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のlites_write_multi_param構造体
 *
 * @retval 0       正常にコピーできた
 * @retval -EFAULT コピーに失敗した
 * @retval -EINVAL リージョン番号に該当するリージョンが存在しない
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_WRITE_MULTI)
{
	struct lites_write_multi_param args;
	unsigned int region;
	int i, ret = 0;

	if (unlikely(copy_from_user_addr(&args, sizeof(args)))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	if (unlikely(args.num == 0 || args.pos == NULL || args.trc_fmt == NULL)) {
		lites_pr_err("invalid arguments");
		return -EINVAL;
	}

	for (i = 0; i < args.num; i++) {
		if (unlikely(copy_from_user(&region, &args.pos[i], sizeof(region)))) {
			lites_pr_err("copy_from_user");
			return -EFAULT;
		}

		ret = __LITES_WRITE(region, &args.trc_fmt[i]);
		if (unlikely(ret < 0)) {
			lites_pr_err("__LITES_WRITE (ret = %d)", ret);
			break;
		}
	}

	return ret < 0 ? ret : 0; /** 0 is normal */
}

/**
 * @brief デバイス情報取得用
 * 　指定された文字列と同じ名前を持つブロックデバイスのデバイス情報を取得する。
 *
 * @param[in]  name 取得するデバイスの名称
 *
 * @retval デバイス情報格納アドレス
 */
/* FUJITSU-TEN 2013-11-20 :LITES AREA DUMP add start */
LOCAL struct block_device *block_finddev(const char *name)
{
	struct class_dev_iter diter;
	struct block_device *bdev = NULL;
	struct device *dev = NULL;
	struct gendisk *disk = NULL;
	struct hd_struct *part = NULL;
	char buf[BDEVNAME_SIZE];
	struct disk_part_iter piter;

	LITES_DUMP_DEBUG("block_finddev start");

	class_dev_iter_init(&diter, &block_class, NULL, NULL);
	LITES_DUMP_DEBUG("class_dev_iter_init");

	while(bdev == NULL){
		LITES_DUMP_DEBUG("while (0x%llx)", dev);
		dev = class_dev_iter_next(&diter);
		if (dev==NULL){
			LITES_DUMP_ERROR("class_dev_iter_next: dev == NULL");
			 break;
		}
		LITES_DUMP_DEBUG("class_dev_iter_next:0x%llx ", dev);
		LITES_DUMP_DEBUG("class_dev_iter_next:(name:%s)", dev->type->name);

		if (strcmp(dev->type->name, "disk") != 0) continue;

		disk = dev_to_disk(dev);
		LITES_DUMP_DEBUG("dev_to_disk get_capacity:%ld", get_capacity(disk));
		if (get_capacity(disk) == 0) continue;

		LITES_DUMP_DEBUG("disk->flags & GENHD_FL_SUPPRESS_PARTITION_INFO");
		if (disk->flags & GENHD_FL_SUPPRESS_PARTITION_INFO) continue;
		LITES_DUMP_DEBUG("disk->flags & GENHD_FL_SUPPRESS_PARTITION_INFO is false");
	
		disk_part_iter_init(&piter, disk, DISK_PITER_INCL_PART0);
		LITES_DUMP_DEBUG("disk_part_iter_init");

		part = disk_part_iter_next(&piter);
		while( part != NULL) {
			disk_name(disk, part->partno, buf);
			LITES_DUMP_DEBUG("part:%d buf:%s", part->partno, buf);
			if(strcmp(buf, name) == 0) {
				bdev = bdget(part_devt(part));
				break;
			}
			part = disk_part_iter_next(&piter);
		}
		disk_part_iter_exit(&piter);
	}
	class_dev_iter_exit(&diter);
	LITES_DUMP_DEBUG("class_dev_iter_exit");

	LITES_DUMP_DEBUG("block_finddev end");

	return bdev;

}
/* FUJITSU-TEN 2013-11-20 :LITES AREA DUMP add end   */


/**
 * @brief LITES_DUMPコマンド用関数
 * 　リージョン番号で指定したリージョンのダンプを取得する。
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のlites_dump_param構造体
 *
 * @retval 0以上   ダンプしたサイズ
 * @retval -EFAULT コピーに失敗した
 * @retval -EINVAL 引数が不正である
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_DUMP)
{
#if 1 /* FUJITSU-TEN 2013-11-20 :LITES AREA DUMP add start   */
	loff_t part_offset;
	pgoff_t index = 0;
	size_t start  = 0;
	size_t size   = 0;

	size_t  old_len   = 0;
	pgoff_t old_index = 0;
	char*   old_mgmt_buff = NULL;

	size_t len = LITES_DUMP_RAM_SIZE; /* 8 MB */

	struct block_device *bdev = NULL;
	int ret;
	fmode_t mode=FMODE_READ|FMODE_WRITE;
	struct page *page = NULL;
	uint8_t *page_addr = NULL;

	EXTERN_LITES_BUFFER(lites_dump);
	struct lites_buffer *buffer = lites_buffer(lites_dump);

	char* mgmt_buff = (char*)ioremap_nocache(LITES_DUMP_RAM_ADDR, LITES_DUMP_RAM_SIZE);
	struct lites_management_info* mgmt_buff_check = (struct lites_management_info*)mgmt_buff;
	char* mgmt_top = mgmt_buff;

	if (mgmt_buff == NULL) {
		LITES_DUMP_ERROR("ioremap_nocache error!!!");
		return -1;
	}

	LITES_DUMP_DEBUG("lites_dump start");
	LITES_DUMP_DEBUG("ioremap(0x%x, %llu) = 0x%llx", LITES_DUMP_RAM_ADDR, LITES_DUMP_RAM_SIZE, mgmt_buff);

	lites_buffer_lock(buffer);

	LITES_DUMP_DEBUG("lites_drv stop...");
	lites_trace_out = 0;

	part_offset = find_partition_start_address_by_name(LITES_DUMP_BLOCK_DEV, LITES_DUMP_PART_NAME);
	LITES_DUMP_DEBUG("part_offset(ALG) %lld", part_offset);
	if (part_offset < 0) {
		LITES_DUMP_ERROR("find_partition_start_address_by_name error!!!");
		iounmap(mgmt_top);
		lites_buffer_unlock(buffer);
		lites_trace_out = 1;
		return -1;
	}

	bdev = block_finddev(LITES_DUMP_BLOCK_DEV);
	LITES_DUMP_DEBUG("bdev 0x%llx", bdev);
	/* 2013-12-03 klocwork(ID:246141) add start */
	if (bdev == NULL) {
		LITES_DUMP_ERROR("block_finddev return NULL!!!");
		iounmap(mgmt_top);
		lites_buffer_unlock(buffer);
		lites_trace_out = 1;
		return -1;
	}
	/* 2013-12-03 klocwork(ID:246141) add end   */

	ret = blkdev_get(bdev, mode, NULL);
	LITES_DUMP_DEBUG("blkdev_get %d", ret);

	index = part_offset >> PAGE_SHIFT;
	start = part_offset % PAGE_SIZE;
	LITES_DUMP_DEBUG("index %lld, start %lld", index, start);

	if (mgmt_buff_check->magic_no == 0x53534152) /* RASS */
	{
		LITES_DUMP_DEBUG(" MAGIC_NO = 0x53534152");
	}

	do {
		if((start + len) > PAGE_SIZE) {
			size = PAGE_SIZE - start;
		}else{
			size = len;
		}
		LITES_DUMP_DEBUG("start:%lu len:%lu size:%lu", start, len, size);

		// copy buffer form DDR
		LITES_DUMP_DEBUG("copy to buffer len:%lu size:%lu", len, size);
		memcpy(buffer->buffer, &mgmt_buff[0], size);

		// get page info
		page = read_mapping_page(bdev->bd_inode->i_mapping, index, NULL); 
		LITES_DUMP_DEBUG("page 0x%llx", page);

		if(IS_ERR(page)) {
			ret = PTR_ERR(page);
			LITES_DUMP_ERROR("read_mapping_page err!(IS_ERR(%p)=%d)", page, ret);
			lites_trace_out = 1;
			lites_buffer_unlock(buffer);
			return -1;
		}

		lock_page(page);
		LITES_DUMP_DEBUG("lock_page");

		// get page address
		page_addr = (uint8_t*)page_address(page);
		LITES_DUMP_DEBUG("page_addr 0x%llx", page_addr);

		page_addr += start;
		LITES_DUMP_DEBUG("page_addr+start 0x%llx", page_addr);

		// copy eMMC from buffer
		memcpy(page_addr, &buffer->buffer[0], size);
		LITES_DUMP_DEBUG("memcpy");

		set_page_dirty(page);
		LITES_DUMP_DEBUG("set_page_dirty");

		unlock_page(page);
		LITES_DUMP_DEBUG("unlock_page");

		page_cache_release(page);
		LITES_DUMP_DEBUG("page_cache_release");
	
		old_index = index;
		old_mgmt_buff = mgmt_buff;
		old_len = len;
	
		index++;
		start = 0;
		mgmt_buff += size;
		len -= size;
		LITES_DUMP_DEBUG("index:%ld->%ld mgmt_buff:0x%llx->0x%llx len:%lld->%lld", old_index, index, old_mgmt_buff, mgmt_buff, old_len, len);
	} while(len>0);

	LITES_DUMP_DEBUG("while(len>0) end");
	ret = sync_blockdev(bdev);
	LITES_DUMP_DEBUG("sync_blockdev");

	blkdev_put(bdev, mode);
	LITES_DUMP_DEBUG("blkdev_put");

	iounmap(mgmt_top);
	LITES_DUMP_DEBUG("iounmap");

	LITES_DUMP_DEBUG("lites_drv restart");
	lites_trace_out = 1;

	lites_buffer_unlock(buffer);
	LITES_DUMP_DEBUG("lites_dump end");

	return 0;
#else  /* FUJITSU-TEN 2013-11-20 :LITES AREA DUMP add      */
/* original source ... */

	struct lites_region *region;
	struct lites_dump_param args;
	size_t size;
	u8 *from;

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

#ifdef LITES_FTEN
	lites_pr_dbg("DEFINE_LITES_COMPAT_IOCTL(LITES_DUMP) region_no=%08x", args.region);
#endif /* LITES_FTEN */

	region = lites_find_region_with_number(args.region);
	if (region == NULL) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	size = region->rb->size;
	lites_pr_dbg("args.offset = %016llx, size = %016llx", (u64)args.offset, (u64)size);

	if (args.offset >= size) {
		lites_pr_err("offset is too big");
		return -EINVAL;
	}

	if (args.offset + args.count > size)
		size -= args.offset;
	else
		size = args.count;

	from = (u8 *)region->rb;
	if (copy_to_user(args.buf, from + args.offset, size)) {
		lites_pr_err("copy_to_user");
		return -EFAULT;
	}

	return size;
#endif  /* FUJITSU-TEN 2013-11-20 :LITES AREA DUMP add end   */
}

/**
 * @brief LITES_TRACE_READコマンド用関数
 * 　リージョン番号で指定したリージョンのログを読み込む。タイムスタンプ等のヘッ
 * ダも読み込む。さらに、lites_write_param構造体の書式をそのままコピーするので、
 * ユーザ側でバッファの解析が必要である。
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のlites_read_param構造体
 *
 * @retval 0以上   読み込んだログのサイズ
 * @retval -EFAULT コピーに失敗した
 * @retval -EINVAL リージョン番号に該当するリージョンが存在しない
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_READ)
{
	EXTERN_LITES_BUFFER(LITES_READ);
	struct lites_region *region;
	struct lites_read_param args;
	struct lites_buffer *buffer = lites_buffer(LITES_READ);
	size_t size;
	int ret;

	if (unlikely(copy_from_user_addr(&args, sizeof(args)))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

#ifdef LITES_FTEN
	lites_pr_dbg("DEFINE_LITES_COMPAT_IOCTL(LITES_READ) region_no=%08x", args.region);
#endif /* LITES_FTEN */

	region = lites_find_region_with_number(args.region);
	if (unlikely(region == NULL)) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	lites_buffer_lock(buffer);

	size = min_t(size_t, args.count, buffer->size);

	ret = lites_ioctl_read_region(region, args.offset, buffer->buffer, size, args.erase);
	if (unlikely(ret < 0)) {
		lites_pr_err("lites_ioctl_read_region (ret = %d)", ret);
		goto unlock;
	}

	if (unlikely(copy_to_user(args.buf, buffer->buffer, ret))) {
		lites_pr_err("copy_to_user");
		ret = -EFAULT;
	}

unlock:
	lites_buffer_unlock(buffer);
	return ret;
}

DEFINE_LITES_COMPAT_IOCTL(LITES_READ_SQ)
{
	EXTERN_LITES_BUFFER(LITES_READ_SQ);
	struct lites_region *region;
	struct lites_read_sq_param args;
	struct lites_buffer *buffer = lites_buffer(LITES_READ_SQ);
	size_t size;
	int ret;

	if (unlikely(copy_from_user_addr(&args, sizeof(args)))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	region = lites_find_region_with_number(args.region);
	if (unlikely(region == NULL)) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	size = min_t(size_t, buffer->size, args.count);
	if (size < LITES_ACCESS_BYTE_MAX) {
		/** バッファのサイズよりも大きいログを読み込めない場合に、延々と
		 *  書き込みを待つ場合があるので、バッファのサイズがログの最大値
		 *  よりも大きくない場合はエラーとする */
		lites_pr_err("size is too small");
		return -EINVAL;
	}

	lites_buffer_lock(buffer);

	ret = lites_ioctl_read_sq_region(region, args.offset, buffer, size);
	if (unlikely(ret < 0)) {
		lites_pr_err("lites_read_sq_region (ret = %d)", ret);
		goto ret;
	}

	if (unlikely(copy_to_user(args.buf, buffer->buffer, ret))) {
		lites_pr_err("copy_to_user");
		ret = -EFAULT;
	}

ret:
	lites_buffer_unlock(buffer);
	return ret;
}

/**
 * @brief SYSLOG_DUMPコマンド用関数
 * 　シスログ領域からダンプを取得する。ダンプ用の領域はユーザ側で十分に確保して
 * いることを前提とする。
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のバッファ
 *
 * @retval 0       正常終了した
 * @retval -EINVAL シスログ領域が存在しない
 * @retval -EFAULT コピーに失敗した
 */
DEFINE_LITES_COMPAT_IOCTL(SYSLOG_DUMP)
{
	struct lites_region *region;
	size_t size;

	region = lites_find_region_with_path(LITES_SYSLOG_PATH);
	if (region == NULL) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	size = region->rb->size;
	if (copy_to_user_addr(region->rb, size)) {
		lites_pr_err("copy_to_user_addr");
		return -EFAULT;
	}

	return 0;
}

DEFINE_LITES_IOCTL(LITES_CHG_NR)
{
	struct lites_region *region = get_arg1()->private_data;

	if (region == NULL) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	if (lites_find_region_with_number(get_arg2()) != NULL) {
		lites_pr_err("region number %u is used", get_arg2());
		return -EINVAL;
	}

	region->rb->region = get_arg2();
	return 0;
}

DEFINE_LITES_IOCTL(LITES_INIT_NG)
{
#ifdef LITES_FTEN_MM_DIRECO
	// MMダイレコ対応により、syslogリージョンの初期化は、litesコマンドからではなく、
	// ドライバ側で直接定義を参照し、syslogリージョンを初期化するように変更。
	// syslogリージョンの初期化は、別のところで実施するので、
	// 処理を実施せず、0で復帰するよう、修正
	return 0;
#else /* LITES_FTEN_MM_DIRECO */
	struct lites_region *region;
	struct region_attribute_ng args;
	u32 psize;
	int ret;
	int initng_continue;

	lites_pr_dbg("DEFINE_LITES_IOCTL(LITES_INIT_NG)");

	if (unlikely(copy_from_user_addr(&args, sizeof(args)))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	// 管理情報の初期化処理（確保）が出来てないので、高速ログへの出力は実施しない
	if (unlikely(mgmt_info_init_done == 0)) {
		lites_pr_err("log output is not changed to LITES. mgmt_info is not initialized.");
		return -EPERM;
	}

	// 管理情報内をチェックし、リージョンの継続が可能かどうかチェックする
	initng_continue = lites_mgmt_info_check(args.region,
											args.addr >> 32, args.addr,
											args.size, args.level,
											args.access_byte, args.wrap);
	if (unlikely(initng_continue < 0)) {
		return initng_continue;
	} else if (initng_continue == 1) {
		lites_pr_info("LITES(region_no=%08x) is continued", args.region);
	} else {
		lites_pr_info("LITES(region_no=%08x) is restarted", args.region);
	}

	if (unlikely(lites_find_region_with_number(args.region))) {
		lites_pr_err("region %u is already exist", args.region);
		return -EINVAL;
	}

	if (unlikely(lites_find_region_with_file(get_arg1()))) {
		lites_pr_err("this path is already exist");
		return -EINVAL;
	}

	ret = lites_check_region(args.addr, args.size,
				 args.level, args.access_byte);
	if (unlikely(ret < 0)) {
		lites_pr_err("lites_check_region (ret = %d)", ret);
		return -EINVAL;
	}

	psize = strlen_lookup(get_arg1());
	/* トレース初期化 or トレース継続 */
	region = lites_alloc_region(args.addr, args.size, psize, initng_continue);
	if (unlikely(region == NULL)) {
		lites_pr_err("lites_alloc_region");
		return -ENOMEM;
	}

	if (unlikely(strcpy_lookup(get_arg1(), region->path, psize))) {
		lites_pr_err("strcpy_lookup");
		lites_free_region(region);
		return -EINVAL;
	}

	region->rb->addr        = args.addr;
	region->rb->size        = args.size;
	region->rb->region      = args.region;
	region->rb->level       = args.level;
	region->rb->access_byte = args.access_byte;
	region->rb->wrap        = args.wrap;
	region->rb->block_rd    = args.block_rd;

	region->header_size = LITES_COMMON_LOG_HEADER_STRUCT_SIZE;
	region->psize       = psize;
	region->total       = args.size - offsetof(struct lites_ring_buffer, log);

	if (region->rb->access_byte != LITES_VAR_ACCESS_BYTE) {
		region->total -= region->total % region->rb->access_byte;
		region->lop    = &lites_const_lop;
	} else {
		region->lop = &lites_var_lop;
	}

	init_region_lock(region);

	ret = lites_register_region(region);
	if (unlikely(ret < 0)) {
		lites_pr_err("lites_register_region (ret = %d)", ret);
		lites_free_region(region);
		return -ENOMEM;
	}

	return 0;
#endif /* LITES_FTEN_MM_DIRECO */
}

#ifdef LITES_FTEN
/**
 * @brief LITES_TRACE_STOPコマンド用関数
 * トレース出力を停止する
 * 再開にはLITES_RESTARTコマンドを利用する。
 *
 * @param[in]  file 未使用
 * @param[in]  addr 未使用
 *
 * @retval 0       正常終了した
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_STOP)
{
	lites_pr_dbg("DEFINE_LITES_COMPAT_IOCTL(LITES_STOP)");

	lites_trace_out = 0;
	return 0;
}

/**
 * @brief LITES_TRACE_RESTARTコマンド用関数
 * トレース出力を再開する
 * 停止にはLITES_STOPコマンドを利用する。
 *
 * @param[in]  file 未使用
 * @param[in]  addr 未使用
 *
 * @retval 0       正常終了した
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_RESTART)
{
	lites_pr_dbg("DEFINE_LITES_COMPAT_IOCTL(LITES_RESTART)");

	lites_trace_out = 1;
	return 0;
}

/**
 * @brief MGMT_INFO_DUMPコマンド用関数
 * 　管理情報のダンプを取得する。ダンプ用の領域はユーザ側で十分に確保して
 * いることを前提とする。
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のバッファ
 *
 * @retval 0       正常終了した
 * @retval -EFAULT コピーに失敗した
 * @retval -EPERM  領域サイズが少ない
 */
DEFINE_LITES_COMPAT_IOCTL(MGMT_INFO_DUMP)
{
// FUJITSU TEN 2013-06-06 ADA LOG delete 
#ifndef LITES_FTEN

	struct lites_management_info_dump_param args;
	struct lites_management_info mgmtinfo;

	lites_pr_dbg("DEFINE_LITES_COMPAT_IOCTL(MGMT_INFO_DUMP)");

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	if (args.bufsize < sizeof(struct lites_management_info)) {
		lites_pr_err("bufsize is short");
		return -EPERM;
	}

	if (lites_mgmt_info_copy(&mgmtinfo) < 0) {
		lites_pr_err("lites_mgmt_info_copy");
		return -EFAULT;
	}

	if (copy_to_user(args.buf, &mgmtinfo, sizeof(struct lites_management_info))) {
		lites_pr_err("copy_to_user_addr");
		return -EFAULT;
	}
#endif
// FUJITSU TEN 2013-06-06 ADA LOG delete 
	return 0;
}

#ifdef LITES_FTEN_MM_DIRECO
/**
 * @brief LITES_GET_R_RGN_Nコマンド用関数
 * 退避（読み出し）可能なリージョン情報の取得する
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のバッファ
 *
 * @retval 0       正常終了した
 * @retval 負      異常終了した
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_GET_R_RGN_N)
{
	struct lites_read_region_number_param args;
	int ret;
	u32 full_region_num = LITES_TRACE_REGION_MAX_NUM; // 32-2
	struct lites_read_region_info full_region_list[LITES_TRACE_REGION_MAX_NUM]; // 32-2


	lites_pr_dbg("DEFINE_LITES_COMPAT_IOCTL(LITES_GET_R_RGN_N)");

	if (unlikely(lites_trace_init_done == 0)) {
		lites_pr_err("lites_trace_init is not completed");
		return -EPERM;
	}

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	// 読み出し可能なトレースリージョンリストを取得
	ret = lites_trace_get_readable_region_list(&full_region_num, full_region_list);
	if (unlikely(ret < 0)) {
		lites_pr_err("lites_trace_get_readable_region_list error  ret=%d", ret);
		return ret;
	}

	// 読み出し可能なリージョンリストのリージョン番号の連続性チェックと
	// 格納先に収まるかどうかのチェック
	lites_trace_check_readable_region_list(full_region_num, full_region_list);
	if (unlikely(*args.num < full_region_num)) {
		lites_pr_err("storage area is insufficient. region number list cannot be stored.");
		return -EINVAL;
	}

	// 結果の設定
	if (copy_to_user(args.num, &full_region_num, sizeof(full_region_num))) {
		lites_pr_err("copy_to_user, num");
		return -EFAULT;
	}
	if (copy_to_user(args.info_list, &full_region_list, sizeof(struct lites_read_region_info) * full_region_num)) {
		lites_pr_err("copy_to_user, info_list");
		return -EFAULT;
	}

	return 0;
}
#else /* LITES_FTEN_MM_DIRECO */
/**
 * @brief LITES_GET_R_RGN_Nコマンド用関数
 * 退避（読み出し）可能なリージョン番号の取得する
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のバッファ
 *
 * @retval 0       正常終了した
 * @retval 負      異常終了した
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_GET_R_RGN_N)
{
	struct lites_read_region_number_param args;
	int ret;
	u32 full_region_no_num = LITES_TRACE_REGION_MAX_NUM; // 32-2
	u32 full_region_no_list[LITES_TRACE_REGION_MAX_NUM]; // 32-2


	lites_pr_dbg("DEFINE_LITES_COMPAT_IOCTL(LITES_GET_R_RGN_N)");

	if (unlikely(lites_trace_init_done == 0)) {
		lites_pr_err("lites_trace_init is not completed");
		return -EPERM;
	}

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	// 読み出し可能なトレースリージョンリストを取得
	ret = lites_trace_get_readable_region_list(&full_region_no_num, full_region_no_list);
	if (unlikely(ret < 0)) {
		lites_pr_err("lites_trace_get_readable_region_list error  ret=%d", ret);
		return ret;
	}

	// 読み出し可能なリージョンリストのリージョン番号の連続性チェックと
	// 格納先に収まるかどうかのチェック
	lites_trace_check_readable_region_list(full_region_no_num, full_region_no_list);
	if (unlikely(*args.num < full_region_no_num)) {
		lites_pr_err("storage area is insufficient. region number list cannot be stored.");
		return -EINVAL;
	}

	// 結果の設定
	if (copy_to_user(args.num, &full_region_no_num, sizeof(full_region_no_num))) {
		lites_pr_err("copy_to_user, num");
		return -EFAULT;
	}
	if (copy_to_user(args.region, &full_region_no_list, sizeof(u32) * full_region_no_num)) {
		lites_pr_err("copy_to_user, region");
		return -EFAULT;
	}

	return 0;
}
#endif /* LITES_FTEN_MM_DIRECO */

/**
 * @brief LITES_GET_W_RGN_Nコマンド用関数
 * トレースログの書き込み先リージョン番号の取得する
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のバッファ
 *
 * @retval 0       正常終了した
 * @retval 負      異常終了した
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_GET_W_RGN_N)
{
	struct lites_write_region_number_param args;
	u32 write_trace_region_no;

	lites_pr_dbg("DEFINE_LITES_COMPAT_IOCTL(LITES_GET_W_RGN_N)");

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	// 書き込み先リージョン番号の取得
#ifdef LITES_FTEN_MM_DIRECO
	if (unlikely((args.group_id == 0) || (args.group_id > lites_trace_group_num()))) {
		lites_pr_err("invalid group_id:%d", args.group_id);
		return -EINVAL;
	}
	write_trace_region_no = lites_trace_write_trace_region_no(args.group_id);
#else /* LITES_FTEN_MM_DIRECO */
	write_trace_region_no = lites_mgmt_info_get_write_trace_region_no();
#endif /* LITES_FTEN_MM_DIRECO */
	
	if (copy_to_user(args.region, &write_trace_region_no, sizeof(write_trace_region_no))) {
		lites_pr_err("copy_to_user");
		return -EFAULT;
	}

	return 0;
}

/**
 * @brief UBINUX_K_INFO_DUMPコマンド用関数
 * kernel panic情報をダンプする
 *
 * @param[in]  file 未使用
 * @param[in]  addr ユーザ空間のバッファ
 *
 * @retval 0より大  ダンプしたサイズ
 * @retval 0        kernel panic情報なし
 * @retval 負       異常終了した
 */
DEFINE_LITES_COMPAT_IOCTL(UBINUX_K_INFO_DUMP)
{
	struct ubinux_kernel_panic_dump_param args;
	size_t size;
	u8 *from;
	UBINUX_KERNEL_PANIC_INFO *ukpi;

	lites_pr_dbg("DEFINE_LITES_COMPAT_IOCTL(UBINUX_K_INFO_DUMP)");

	if (kernel_panic_info_init_done == 0) {
		lites_pr_err("ubinux_kernel_panic_info is not initialized.");
		return -EPERM;
	}

	if (copy_from_user_addr(&args, sizeof(args))) {
		lites_pr_err("copy_from_user_addr");
		return -EFAULT;
	}

	size = sizeof(UBINUX_KERNEL_PANIC_INFO);

	// offset位置がデータサイズ越えてる場合は、0読みだすデータなし：0で復帰
	if (args.offset >= size) {
		lites_pr_dbg("offset is too big");
		return 0;
	}

	// 用意されたバッファサイズに合わせて、読みだすサイズを決定する
	if (args.offset + args.bufsize > size) {
		size -= args.offset;
	} else {
		size = args.bufsize;
	}

	// 既にubinux_kernel_panic_info_alloc済みのはずであるため、
	// ここでは、確保済みのアドレスが取得される予定
	ukpi = ubinux_kernel_panic_info_alloc(0, 0);
	from = (u8 *)ukpi;

	// kernel panic情報が格納されてない場合は、読みだすデータなし：0で復帰
	if (ukpi->header.status == 0) {
		lites_pr_dbg("kernel panic information is not stored.");
		return 0;
	}

	if (copy_to_user(args.buf, from + args.offset, size)) {
		lites_pr_err("copy_to_user");
		return -EFAULT;
	}

	// クリア指定があり、全てのデータの読み出しが完了したときは、
	// 格納されているkernel panic情報をクリアする
	if ((args.panicclr == 1) &&
		((args.offset + size) >= sizeof(UBINUX_KERNEL_PANIC_INFO))) {
		// マジックナンバと末尾マジックナンバの間にある領域を初期化
		memset(&ukpi->rsv, 0, ((char *)&ukpi->tail_magic_no) - ((char *)&ukpi->rsv));
		lites_pr_dbg("ubinux_kernel_panic_info clear");
	}

	return size;
}

#ifdef LITES_FTEN_MM_DIRECO
/**
 * @brief LITES_READ_COMPコマンド用関数
 * 　リージョン番号で指定したリージョンのログを読み出し済み状態にする。
 *
 * @param[in]  file 未使用
 * @param[in]  addr リージョン番号
 *
 * @retval 0       正常終了した
 * @retval -EINVAL 不正な引数が指定された
 */
DEFINE_LITES_COMPAT_IOCTL(LITES_READ_COMP)
{
	unsigned long flags = 0;
	struct lites_region *region;
	unsigned int region_no;

	lites_pr_dbg("DEFINE_LITES_COMPAT_IOCTL(LITES_READ_COMP)");

	region_no = get_arg2();
	region = lites_find_region_with_number(region_no);
	if (region == NULL) {
		lites_pr_err("not initialized region. region_no: %d", region_no);
		return -EPERM;
	}

	region_write_lock(region, flags);

	region->rb->read     = 0;
	region->rb->read_sq  = 0;
	region->rb->write    = 0;
	region->rb->flags   &= ~(LITES_FLAG_READ | LITES_FLAG_READ_SQ);

	region_write_unlock(region, flags);

	return 0;
}
#endif /* LITES_FTEN_MM_DIRECO */

#endif /* LITES_FTEN */

struct lites_ioctl lites_ioctls[] = {
	ENTRY_LITES_COMPAT_IOCTL(LITES_INIT),          /**  0 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_GET_ATTR),      /**  1 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_CHG_LVL),       /**  2 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_CHG_LVL_ALL),   /**  3 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_INQ_LVL),       /**  4 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_NTFY_SIG),      /**  5 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_STOP_SIG),      /**  6 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_INQ_SIG),       /**  7 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_CLR_SIG_TBL),   /**  8 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_INQ_OVER),      /**  9 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_CLR_DATA),      /** 10 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_CLR_DATA_ZERO), /** 11 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_WRAP),          /** 12 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_BLOCK),         /** 13 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_WRITE),         /** 14 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_WRITE_MULTI),   /** 15 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_DUMP),          /** 16 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_READ),          /** 17 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_READ_SQ),       /** 18 */
	ENTRY_LITES_COMPAT_IOCTL(SYSLOG_DUMP),         /** 19 */
	ENTRY_LITES_IOCTL(LITES_CHG_NR),               /** 20 */
	ENTRY_LITES_IOCTL(LITES_INIT_NG),              /** 21 */
#ifdef LITES_FTEN
	ENTRY_LITES_COMPAT_IOCTL(LITES_STOP),          /** 22 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_RESTART),       /** 23 */
	ENTRY_LITES_COMPAT_IOCTL(MGMT_INFO_DUMP),      /** 24 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_GET_R_RGN_N),   /** 25 */
	ENTRY_LITES_COMPAT_IOCTL(LITES_GET_W_RGN_N),   /** 26 */
	ENTRY_LITES_COMPAT_IOCTL(UBINUX_K_INFO_DUMP),  /** 27 */
#ifdef LITES_FTEN_MM_DIRECO
	ENTRY_LITES_COMPAT_IOCTL(LITES_READ_COMP),     /** 28 */
#endif /* LITES_FTEN_MM_DIRECO */
#endif /* LITES_FTEN */
};

size_t lites_ioctls_nr = ARRAY_SIZE(lites_ioctls);
