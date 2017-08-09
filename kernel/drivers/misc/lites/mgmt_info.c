/**
 * @file mgmt_info.c
 *
 * @brief 管理情報に関する操作用関数の実装
 *
 * Copyright (c) 2011-2013 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/rcupdate.h>
#include <linux/lites_trace.h>
#include <asm/io.h>
#include "region.h"
#include "parse.h"
#include "mgmt_info.h"
#include "tsc.h"

#ifdef LITES_FTEN

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

struct lites_management_info *mgmt_info = NULL;			// マップした管理情報
#else
static struct lites_management_info *mgmt_info = NULL;	// マップした管理情報
#endif /* LITES_FTEN_MM_DIRECO */
int mgmt_info_init_done = 0;							// 管理情報init実施有無
static int mgmt_info_continue = 0;						// 管理情報の継続有無
static struct lites_management_info backup_mgmt_info;	// 管理情報のバックアップ

#ifndef LITES_FTEN_MM_DIRECO
static u64 mgmt_info_start;				// カーネル起動パラメータ解析で解析した開始アドレス
static u32 mgmt_info_size;				// カーネル起動パラメータ解析で解析したサイズ
static int mgmt_info_parse_done = 0;	// カーネル起動パラメータ解析実施の有無
#endif	// LITES_FTEN_MM_DIRECO


spinlock_t mgmt_info_spinlock;			// 管理情報lock/unlock時に使用
unsigned int mgmt_info_cpu = UINT_MAX;	// 管理情報lock/unlock時に使用

static unsigned int  unixtime_tmp = 0;				// unixtime補正用unixtime格納値
static unsigned int  dif_sec_tmp = 0;				// unixtime補正用sec差分格納値
static u64 jiffies_cnt  = 0;						// unixtime補正用jiffies格納値
static u64 jiffies_tmp  = 0;						// unixtime補正用jiffies格納値

/**
 * @brief 管理情報領域獲得用関数
 *
 * @param[in]  addr 管理情報開始アドレス
 * @param[in]  size 管理情報のサイズ
 *
 * @retval 管理情報 正常終了
 * @retval NULL     異常終了
 */
struct lites_management_info *lites_mgmt_info_alloc(u64 addr, u32 size)
{
	if (mgmt_info != NULL) {
		return mgmt_info;
	}

	mgmt_info = (struct lites_management_info *)ioremap_nocache(addr, size);
	if (unlikely(mgmt_info == NULL)) {
		lites_pr_err("ioremap_nocache error. addr=%064x size=%08x", addr, size);
	}

	init_mgmt_info_lock();

	return mgmt_info;
}


/**
 * @brief 管理情報解放用関数
 */
void lites_mgmt_info_free(void)
{
	if (mgmt_info != NULL) {
		iounmap(mgmt_info);
		mgmt_info = NULL;
	}
}

/**
 * @brief 管理情報のチェック関数
 *
 * @param[in]  region_no   リージョン番号
 * @param[in]  addr_high   リージョンの開始アドレス（上位32bit）
 * @param[in]  addr_low    リージョンの開始アドレス（下位32bit）
 * @param[in]  size        リージョンのサイズ
 * @param[in]  level       リージョンのレベル
 * @param[in]  access_byte リージョンの単一ログのサイズ
 * @param[in]  wrap        リージョンの上書き可能/禁止を示すフラグ
 *
 * @retval 1  リージョンの継続可能（継続）
 * @retval 0  リージョンの継続はしない（新規）
 * @retval 負 異常終了
 */
int lites_mgmt_info_check(u32 region_no, u32 addr_high, u32 addr_low, u32 size, u32 level, u32 access_byte, u16 wrap)
{
	int ret = 0;
	struct region_attribute region_attr;

	// リージョン情報の作成
	memset(&region_attr, 0, sizeof(region_attr));
	region_attr.size = size;
	region_attr.region_addr_high = addr_high;
	region_attr.region_addr_low = addr_low;
	region_attr.level = level;
	region_attr.access_byte = access_byte;
	region_attr.wrap = wrap;

	// 管理情報に記録されてるリージョン情報と指定されたリージョン情報をチェック
	// 管理情報自体の有効/無効も判定される。負の場合は無効。
	ret = lites_mgmt_info_check_region(region_no, &region_attr);
	if (ret < 0) {
		/* 不一致があった場合、継続を無効とする */
		lites_pr_dbg("lites_mgmt_info_check_region ret=%d region_no=%08x", ret, region_no);

		/* 管理情報にもリージョン情報を記録 */
		ret = lites_mgmt_info_add_region(region_no, &region_attr);
		if (unlikely(ret < 0)) {
			lites_pr_err("lites_mgmt_info_add_region ret=%d", ret);
			return -ENOMEM;
		}

		ret = 0;	// 継続しない
	} else {
		/* 継続 */
		lites_pr_dbg("lites_mgmt_info_check_region region_no=%08x OK", region_no);
		ret = 1;	// 継続
	}

	return ret;
}

/**
 * @brief 管理情報のリージョン属性値チェック関数
 *
 * @param[in]  region_no リージョン番号
 * @param[in]  region_attr  管理情報のリージョン属性と比較するリージョンの属性値
 *
 * @retval 0 正常終了
 * @retval 負 異常終了
 */
int lites_mgmt_info_check_region(u32 region_no, struct region_attribute *region_attr)
{
	struct region_attribute *target;
	int ret = 0;

	if (unlikely(mgmt_info == NULL)) {
		lites_pr_err("mgmt_info is not ioremap.");
		return -98;
	}

	// 管理情報の継続不可が認識されてる場合は、ログ継続不可なので異常終了とする
	if (mgmt_info_continue == 0) {
		lites_pr_dbg("lites_mgmt_info_check_region mgmt_info is not continue.");
		return -50;
	}

	do {
		target = lites_mgmt_info_find_region(region_no);
		if(target == NULL) {
			lites_pr_dbg("lites_mgmt_info_check_region error. region_no:%08x is nothing. region_num=%d", region_no, mgmt_info->region_num);
			ret = -99;
			break;
		}

		// memcmpで一括チェック
		if (unlikely(memcmp(target, region_attr, sizeof(struct region_attribute)) != 0)) {
			// 不一致の場合は、どこの項目が不一致だったと復帰値で通知させるため、1個ずつ比較
			if (unlikely(target->size != region_attr->size)) {
				lites_pr_dbg("lites_mgmt_info_check_region error. target->size=%08x region_attr->size=%08x", target->size, region_attr->size);
				ret = -1;
				break;
			}
			if (unlikely(target->region_addr_high != region_attr->region_addr_high)) {
				lites_pr_dbg("lites_mgmt_info_check_region error. target->region_addr_high=%08x region_attr->region_addr_high=%08x", target->region_addr_high, region_attr->region_addr_high);
				ret = -2;
				break;
			}
			if (unlikely(target->region_addr_low != region_attr->region_addr_low)) {
				lites_pr_dbg("lites_mgmt_info_check_region error. target->region_addr_low=%08x region_attr->region_addr_low=%08x", target->region_addr_low, region_attr->region_addr_low);
				ret = -3;
				break;
			}
			if (unlikely(target->level != region_attr->level)) {
				lites_pr_dbg("lites_mgmt_info_check_region error. target->level=%d region_attr->level=%d", target->level, region_attr->level);
				ret = -4;
				break;
			}
			if (unlikely(target->wrap != region_attr->wrap)) {
				lites_pr_dbg("lites_mgmt_info_check_region error. target->wrap=%d region_attr->wrap=%d", target->wrap, region_attr->wrap);
				ret = -5;
				break;
			}
			if (unlikely(target->access_byte != region_attr->access_byte)) {
				lites_pr_dbg("lites_mgmt_info_check_region error. target->access_byte=%d region_attr->access_byte=%d", target->access_byte, region_attr->access_byte);
				ret = -6;
				break;
			}
		}
	} while (0);

	return ret;
}


/**
 * @brief リージョン属性検索用関数（キーはリージョン番号。上位が排他を実施すること。）
 *
 * @param  region リージョン番号
 *
 * @retval リージョン属性構造体 見つかった
 * @retval NULL                 見つからなかった
 */
struct region_attribute *lites_mgmt_info_find_region(u32 region_no)
{
	// トレース二次仕様
	// 管理情報内に格納されるリージョン属性は、以下のindexのところに格納される
	// printkリージョン 0x80000000           index:0
	// syslogリージョン 0x80000001           index:1
	// traceリージョン  0x0～                index:2～
	// リージョン番号の有効性を確認後、index指定でアクセスする
	if (LITES_CHECK_REGION_NO(region_no, LITES_TRACE_REGION_MAX_NUM)) {
		if (mgmt_info->region_attr_list[LITES_MGMT_LIST_ACCESS_INDEX(region_no)].size > 0) {
			return &(mgmt_info->region_attr_list[LITES_MGMT_LIST_ACCESS_INDEX(region_no)]);
		}
	}

	return NULL;
}

/**
 * @brief 管理情報にリージョン属性を登録する関数
 *
 * @param[in]  region_no リージョン番号
 * @param[in]  region_attr リージョン属性構造体
 *
 * @retval 0   正常終了
 * @retval -1  異常終了
 */
int lites_mgmt_info_add_region(u32 region_no, struct region_attribute *region_attr)
{
	struct region_attribute *target;
	unsigned long flags = 0;
	int ret = 0;

	// 管理情報内を更新するため、ロック
	mgmt_info_lock(flags, -EINVAL);		/*pgr0039*/
	do {
		// 既に同じリージョン番号が存在する場合は、リージョン情報のみ更新して終了
		target = lites_mgmt_info_find_region(region_no);
		if (target != NULL) {
			memcpy(target, region_attr, sizeof(struct region_attribute));
			lites_pr_dbg("lites_mgmt_info_add_region(modify) region_no=%08x", region_no);
			ret = 0;
			break;
		}

		if (unlikely(mgmt_info->region_num == LITES_REGIONS_NR_MAX)) {
			lites_pr_err("lites_mgmt_info_add_region error. Entry is Max! region_attr cannot be added.");
			ret = -1;
			break;
		}

		// 登録されてない場合は、新規追加  リージョン属性を登録
		if (LITES_CHECK_REGION_NO(region_no, LITES_TRACE_REGION_MAX_NUM)) {
			memcpy(&mgmt_info->region_attr_list[LITES_MGMT_LIST_ACCESS_INDEX(region_no)], region_attr, sizeof(struct region_attribute));
			mgmt_info->region_num++;
			if (region_no < LITES_TRACE_REGION_MAX_NUM) {
				mgmt_info->trace_region_num++;
			}
			lites_pr_dbg("lites_mgmt_info_add_region region_no=%08x region_num=%d trace_region_num=%d",
				region_no, mgmt_info->region_num, mgmt_info->trace_region_num);
		} else {
			lites_pr_err("lites_mgmt_info_add_region error. region_no(%08x) is outside the range.", region_no);
			ret = -1;
			break;
		}
	} while (0);
	mgmt_info_unlock(flags);

	return ret;
}

#ifdef LITES_FTEN_MM_DIRECO
/**
 * @brief 管理情報のグループ情報を登録する関数
 *
 * @param[in]  group_id              グループID(1～ group_idの範囲チェックは呼び出し元で実施すること)
 * @param[in]  region_num            リージョン数
 * @param[in]  write_trace_region_no 書き込み先トレースリージョン番号
 * @param[in]  head_region_no        先頭のリージョン番号
 * @param[in]  tail_region_no        末尾リージョン番号
 *
 * @retval 0  正常終了
 * @retval 負 異常終了
 */
int lites_mgmt_info_add_group_info(u32 group_id, u32 region_num, u32 write_trace_region_no, u32 head_region_no, u32 tail_region_no)
{
	int index = LITES_MGMT_GROUP_LIST_ACCESS_INDEX(group_id);
	if (unlikely(mgmt_info->trace_group_num == LITES_TRACE_GROUP_MAX)) {
		lites_pr_err("Entry is Max! group_info cannot be added.");
		return -1;
	}

	/* 2013-11-26 klocwork(ID155300)指摘修正 start */
	if (index < 0) {
		lites_pr_err("index error! (index:%d < 0)", index);
		return -1;
	}
	/* 2013-11-26 klocwork(ID155300)指摘修正 end    */

	mgmt_info->group_list[index].region_num = region_num;
	mgmt_info->group_list[index].write_trace_region_no = write_trace_region_no;
	mgmt_info->group_list[index].head_region_no = head_region_no;
	mgmt_info->group_list[index].tail_region_no = tail_region_no;
	mgmt_info->trace_group_num++;
	return 0;
}
#endif /* LITES_FTEN_MM_DIRECO */

/**
 * @brief 管理情報のコピーを作成する関数
 *
 * @param[in/out]  dst 管理情報のコピー先
 *
 * @retval 0  正常終了
 * @retval 負 異常終了
 */
int lites_mgmt_info_copy(struct lites_management_info *dst)
{
	unsigned long flags = 0;

	// 管理情報全体を参照するため、ロック
	mgmt_info_lock(flags, -EINVAL);		/*pgr0039*/
	memcpy(dst, mgmt_info, sizeof(struct lites_management_info));
	mgmt_info_unlock(flags);

	return 0;
}


/**
 * @brief ヘッダ情報（boot_id,seq_no,level）設定関数
 *
 * @param[in]      level       トレースレベル
 * @param[in/out]  extra_data  ヘッダ情報のアドレス
 *
 * @retval 0  正常終了
 * @retval 負 異常終了
 */
int lites_mgmt_info_set_header_info(u32 level, void *extra_data)
{
	unsigned long flags = 0;
	struct lites_log_header *log_header;

	log_header = (struct lites_log_header *)extra_data;
	log_header->boot_id = 0;
	log_header->seq_no = 0;
	log_header->level = level;

	// 管理情報内を更新するため、ロック
	mgmt_info_lock(flags, -EINVAL);		/*pgr0039*/
	// シーケンス番号のみ、呼び出し単位でインクリメント
	// 0の場合はスキップ
	++mgmt_info->seq_no;
	if (unlikely(mgmt_info->seq_no == 0)) {
		++mgmt_info->seq_no;
	}
/* FUJITSU-TEN 2013-05-23: ADA_LOG mod start */
#ifdef LITES_FTEN
	// lites_unixtimeはtrace_write.cで設定される値
	log_header->boot_id = lites_unixtime;
	
	// unixtimeで前回のログからの経過時刻を判定
	if (unixtime_tmp != lites_unixtime) {
		// 1秒以上経過時は基準時刻を再設定
		lites_get_tsc(jiffies_tmp);
		// 基準時刻からの経過時刻が1秒未満のため
		// 差分を初期化
		dif_sec_tmp = 0;
	} else {
		// 前回のログ出力からの経過時刻が1秒未満の場合
		// 基準時刻はそのまま
		// 差分もそのまま
	}

	// unixtime比較用現在時刻取得
	lites_get_tsc(jiffies_cnt);

	// jiffies値での経過時刻判定
	// jiffies値で1秒以上の経過が確認できた時、unixtimeを補正
	if (unlikely(jiffies_tmp == 0)) {
		// 基準時刻が0秒(初回)のとき、現在のjiffiesの値を基準値に設定
		jiffies_tmp = jiffies_cnt;
		// 差分は初期化
		dif_sec_tmp = 0;
	} else if (jiffies_cnt - jiffies_tmp >= 100) {
		// 経過時刻が100カウント以上の場合、基準時刻と差分を1秒進める
		jiffies_tmp += 100;
		dif_sec_tmp += 1;
	} else {
		// 前回のログ出力からの経過時刻が1秒未満の場合
		// 差分そのまま
	}
	// 基準時刻の差分を補正
	log_header->boot_id += dif_sec_tmp;
	
	// 次のログ出力時のunixtimeと比較するために、
	// 今回のログ出力のunixtimeを一時的に保存
	unixtime_tmp = lites_unixtime;
#else
	log_header->boot_id = mgmt_info->boot_id;
#endif /* LITES_FTEN */
/* FUJITSU-TEN 2013-05-23: ADA_LOG mod end */
	log_header->seq_no = mgmt_info->seq_no;
	mgmt_info_unlock(flags);

	lites_pr_dbg("lites_mgmt_info_set_header_info boot_id:%08x seq_no:%08x level:%d",
		log_header->boot_id, log_header->seq_no, log_header->level);
	return 0;
}


#ifdef LITES_FTEN_MM_DIRECO
/**
 * @brief トレース書き込みリージョン番号設定関数
 *
 * @param[in]  group_id  グループID(1～ group_idの範囲チェックは呼び出し元で実施すること)
 * @param[in]  region_no リージョン番号
 *
 * @retval 0  正常終了
 * @retval 負 異常終了
 */
int lites_mgmt_info_set_write_trace_region_no(u32 group_id, u32 region_no)
{
	unsigned long flags = 0;
	int index = LITES_MGMT_GROUP_LIST_ACCESS_INDEX(group_id);

	/* 2013-11-26 klocwork(ID155301)指摘修正 start */
	if (index < 0) {
		lites_pr_err("index error! (index:%d < 0)", index);
		return -1;
	}
	/* 2013-11-26 klocwork(ID155301)指摘修正 end    */

	// 管理情報内を更新するため、ロック
	mgmt_info_lock(flags, -EINVAL);		/*pgr0039*/
	mgmt_info->group_list[index].write_trace_region_no = region_no;
	mgmt_info_unlock(flags);
	return 0;
}

#else /* LITES_FTEN_MM_DIRECO */
/**
 * @brief トレース書き込み先リージョン番号取得関数
 *
 * @retval トレース書き込み先リージョン番号
 */
u32 lites_mgmt_info_get_write_trace_region_no(void)
{
	return mgmt_info->write_trace_region_no;
}

/**
 * @brief 次のトレース書き込み先リージョン番号取得関数
 *
 * @param[in/out]  region_no  inputとして現在のリージョン番号を指定。
 *                            outputは次の書き込み先のリージョン番号。
 *
 * @retval 0  正常終了
 */
void lites_mgmt_info_get_next_trace_region_no(u32* region_no)
{
	u32 next_region_no = *region_no + 1;

	if (unlikely(next_region_no == mgmt_info->trace_region_num)) {
		next_region_no = 0;
	}

	*region_no = next_region_no;
}

/**
 * @brief 前のトレース書き込み先リージョン番号取得関数
 *
 * @param[in/out]  region_no  inputとして現在のリージョン番号を指定。
 *                            outputは前の書き込み先のリージョン番号。
 *
 * @retval 0  正常終了
 */
void lites_mgmt_info_get_back_trace_region_no(u32* region_no)
{
	u32 back_region_no = *region_no - 1;

	if (unlikely(back_region_no == 0xFFFFFFFF)) {
		back_region_no = mgmt_info->trace_region_num - 1;
	}

	*region_no = back_region_no;
}

/**
 * @brief トレース書き込みリージョン番号設定関数
 *
 * @param[in]  region_no リージョン番号
 *
 * @retval 0  正常終了
 * @retval 負 異常終了
 */
int lites_mgmt_info_set_write_trace_region_no(u32 region_no)
{
	unsigned long flags = 0;

	// 管理情報内を更新するため、ロック
	mgmt_info_lock(flags, -EINVAL);		/*pgr0039*/
	mgmt_info->write_trace_region_no = region_no;
	mgmt_info_unlock(flags);
	return 0;
}
#endif /* LITES_FTEN_MM_DIRECO */

/**
 * @brief トレースリージョン数取得関数
 *
 * @retval トレースリージョン数
 */
u32 lites_mgmt_info_get_trace_region_num(void)
{
	return mgmt_info->trace_region_num;
}

/**
 * @brief BootID取得関数
 *
 * @retval トレースリージョン数
 */
u32 lites_mgmt_info_get_boot_id(void)
{
	return mgmt_info->boot_id;
}

/**
 * @brief BootID更新関数
 *
 */
void lites_mgmt_info_update_boot_id(void)
{
	++mgmt_info->boot_id;
	if (unlikely(mgmt_info->boot_id == 0)) {
		++mgmt_info->boot_id;
	}
}

/**
 * @brief magic_noチェック関数
 *
 * @retval 0  正常終了(magic_no/tail_magic_noに異常なし)
 * @retval 負 異常終了
 */
int lites_mgmt_info_check_magic_no(void)
{
// ADA_LOG 2012/12/19 modify
#if 0
	if ((mgmt_info->magic_no != LITES_MGMT_MAGIC_NO) ||
	    (mgmt_info->tail_magic_no != LITES_MGMT_TAIL_MAGIC_NO)) {
		return -1;
	}
	return 0;
#else
	return -1;
#endif
// ADA_LOG 2012/12/19 modify
}

/**
 * @brief 管理情報バックアップ関数
 *
 * @retval 0  正常終了
 */
int lites_mgmt_info_backup(void)
{
#ifdef LITES_FTEN_MM_DIRECO
	int i;
#endif /* LITES_FTEN_MM_DIRECO */

	// 内部のバックアップ用変数へ格納
	memset(&backup_mgmt_info, 0, sizeof(struct lites_management_info));
	memcpy(&backup_mgmt_info, mgmt_info, sizeof(struct lites_management_info));
	backup_mgmt_info.seq_no = 0;
	backup_mgmt_info.boot_id = 0;
#ifdef LITES_FTEN_MM_DIRECO
	// グループ内の書き込み先リージョン番号を初期値へ
	for (i = 0; i < backup_mgmt_info.trace_group_num; ++i) {
		backup_mgmt_info.group_list[i].write_trace_region_no = backup_mgmt_info.group_list[i].head_region_no;
	}
#endif /* LITES_FTEN_MM_DIRECO */

	return 0;
}

/**
 * @brief 管理情報リストア関数
 *
 * @retval 0        正常終了
 * @retval -EINVAL  lock異常
 */
int lites_mgmt_info_restore(void)
{
	unsigned long flags = 0;

	// バックアップ用変数に格納されてる情報を用いて復元
	// 管理情報内を更新するため、ロック
	mgmt_info_lock(flags, -EINVAL);		/*pgr0039*/
	memcpy(mgmt_info, &backup_mgmt_info, sizeof(struct lites_management_info));
	mgmt_info_unlock(flags);

	return 0;
}

/**
 * @brief リージョンを初期化関数
 *        resume時に管理情報が壊れていた時の処理向け
 *
 * @param[in]  region_no リージョン番号
 *
 * @retval  0  正常終了
 * @retval 負  異常終了
 */
int lites_mgmt_info_initialize_region(u32 region_no)
{
	struct lites_region *region;
	struct region_attribute *region_attr;
	unsigned long flags = 0;

	// リージョン番号のチェック
	if (unlikely(!LITES_CHECK_REGION_NO(region_no, lites_mgmt_info_get_trace_region_num()))) {
		return -1;
	}

	// 対象のリージョン情報が格納されてる場合のみ、復元を行う。
	region_attr = &(mgmt_info->region_attr_list[LITES_MGMT_LIST_ACCESS_INDEX(region_no)]);
	if (region_attr->size > 0) {
		region = lites_find_region_with_number(region_no);
		if (unlikely(region == NULL)) {
			return -1;
		}

		region_write_lock(region, flags);

		// ioremapされてる領域（リージョンのヘッダとログ部分）をクリア
		memset(region->rb, 0, region_attr->size);

		// 必要な部分の情報を再設定
		region->rb->addr        = ((u64)region_attr->region_addr_high << 32) | ((u64)region_attr->region_addr_low);
		region->rb->size        = region_attr->size;
		region->rb->region      = region_no;
		region->rb->level       = region_attr->level;
		region->rb->access_byte = region_attr->access_byte;
		region->rb->wrap        = region_attr->wrap;

		// ここまでで、init実施後の状態に戻る。

		region_write_unlock(region, flags);
	}

	return 0;
}


#ifdef LITES_FTEN_MM_DIRECO
/**
 * @brief LITES用管理情報の初期化用関数
 *   early_lites_mgmt_info関数で解析したパラメータを元に管理情報領域マップし、
 *   状況に応じて初期化する。
 *
 * @retval 0       正常終了（またはカーネル起動パラメータの解析が完了していない）
 * @retval -ENOMEM 管理情報の獲得に失敗した
 * @retval -EINVAL サイズが不正
 */
static __init int lites_mgmt_info_init(void)
{
	struct lites_management_info *mgmt;
	u64 mgmt_info_start;
	u32 mgmt_info_size;

	// 変数の初期化
	mgmt_info = NULL;
	mgmt_info_init_done = 0;// 管理情報初期化未実施
	mgmt_info_continue = 0;	// 管理情報継続無効
	lites_trace_out = 1;	// 出力OK
	mgmt_info_cpu = UINT_MAX;

	// 定義から管理情報の設定を読み出す
	mgmt_info_start	= LITES_MGMT_REGION_ADDR;
	mgmt_info_size	= LITES_MGMT_REGION_SIZE;

	// 指定されたアドレスの先頭から0x400までを管理情報として利用する
	if (unlikely(mgmt_info_size < LITES_MGMT_INFO_SIZE)) {
		lites_pr_err("mgmt_info_size error (size = %08x)", mgmt_info_size);
		return -EINVAL;
	}

	// 管理情報領域を指定してioremapする
	mgmt =lites_mgmt_info_alloc(mgmt_info_start, mgmt_info_size);
	if (unlikely(mgmt == NULL)) {
		lites_pr_err("lites_mgmt_info_alloc error (addr = %016x, size = %08x)", mgmt_info_start, mgmt_info_size);
		return -ENOMEM;
	}

	// 先頭のマジックナンバーをチェック
	if (lites_mgmt_info_check_magic_no() < 0) {
#if 0
		lites_pr_dbg("magic_no mismatch. magic_no=%08x tail_magic_no=%08x", mgmt_info->magic_no, mgmt_info->tail_magic_no);
#else
		lites_pr_info("magic_no mismatch. magic_no=%08x tail_magic_no=%08x", mgmt_info->magic_no, mgmt_info->tail_magic_no);
#endif

		// 電源OFF or 管理情報が壊れてる形跡があるので、初期化
		memset(mgmt_info, 0, sizeof(struct lites_management_info));
		mgmt_info->magic_no = LITES_MGMT_MAGIC_NO;
		mgmt_info->tail_magic_no = LITES_MGMT_TAIL_MAGIC_NO;
		mgmt_info_continue = 0;	// 管理情報継続無効
		lites_pr_info("LITES(mgmt_info) is restarted. addr=%llx",mgmt_info_start);
	} else {
		mgmt_info_continue = 1;	// 管理情報継続有効
		lites_pr_info("LITES(mgmt_info) is continued. addr=%llx",mgmt_info_start);
	}

	// bootカウンタのインクリメント
	lites_mgmt_info_update_boot_id();

	mgmt_info_init_done = 1;	// 管理情報初期化実施済み

	return 0;
}
pure_initcall(lites_mgmt_info_init);

#else	// LITES_FTEN_MM_DIRECO
/**
 * @brief LITES用管理情報の初期化用関数
 *   early_lites_mgmt_info関数で解析したパラメータを元に管理情報領域マップし、
 *   状況に応じて初期化する。
 *
 * @retval 0       正常終了（またはカーネル起動パラメータの解析が完了していない）
 * @retval -ENOMEM 管理情報の獲得に失敗した
 * @retval -EINVAL サイズが不正
 */
static __init int lites_mgmt_info_init(void)
{
	struct lites_management_info *mgmt;

	// 変数の初期化
	mgmt_info = NULL;
	mgmt_info_init_done = 0;// 管理情報初期化未実施
	mgmt_info_continue = 0;	// 管理情報継続無効
	lites_trace_out = 1;	// 出力OK
	mgmt_info_cpu = UINT_MAX;

	if (!mgmt_info_parse_done) {
		lites_pr_info("lites_mgmt_info is not specified, it is invalidated.");
		return -EINVAL;
	}

	// 指定されたアドレスの先頭から0x400までを管理情報として利用する
	if (mgmt_info_size < LITES_MGMT_INFO_SIZE) {
		lites_pr_err("mgmt_info_size error (size = %08x)", mgmt_info_size);
		return -EINVAL;
	}

	// 管理情報領域を指定してioremapする
	mgmt =lites_mgmt_info_alloc(mgmt_info_start, mgmt_info_size);
	if (unlikely(mgmt == NULL)) {
		lites_pr_err("lites_mgmt_info_alloc error (addr = %016x, size = %08x)", mgmt_info_start, mgmt_info_size);
		return -ENOMEM;
	}

	// 先頭のマジックナンバーをチェック
	if (lites_mgmt_info_check_magic_no() < 0) {
		lites_pr_dbg("magic_no mismatch. magic_no=%08x tail_magic_no=%08x", mgmt_info->magic_no, mgmt_info->tail_magic_no);

		// 電源OFF or 管理情報が壊れてる形跡があるので、初期化
		memset(mgmt_info, 0, sizeof(struct lites_management_info));
		mgmt_info->magic_no = LITES_MGMT_MAGIC_NO;
		mgmt_info->tail_magic_no = LITES_MGMT_TAIL_MAGIC_NO;
		mgmt_info_continue = 0;	// 管理情報継続無効
		lites_pr_info("LITES(mgmt_info) is restarted");
	} else {
		mgmt_info_continue = 1;	// 管理情報継続有効
		lites_pr_info("LITES(mgmt_info) is continued");
	}

	// bootカウンタのインクリメント
	lites_mgmt_info_update_boot_id();

	mgmt_info_init_done = 1;	// 管理情報初期化実施済み

	return 0;
}
pure_initcall(lites_mgmt_info_init);

/**
 * @brief LITES用管理情報のカーネル起動パラメータ解析用関数
 * 　カーネル起動パラメータに含まれる"lites_mgmt_info="に続く文字列を解析する。
 * lites_mgmt_info_alloc関数が動作可能になるのはしばらく後なので、パラメータ解析以
 * 降の処理はlites_mgmt_info_initで実行する。

 * @param[in]  arg "lites_mgmt_info="に続く文字列
 *
 * @retval 0  解析に成功した
 * @retval -1 解析に失敗した
 */
static __init int early_lites_mgmt_info(char *arg)
{
	int ret;

	mgmt_info_parse_done = 0;

	ret = parse_addr(&arg, &mgmt_info_start);
	if (ret < 0) {
		lites_pr_err("parse_addr (arg = %s, ret = %d)", arg, ret);
		return ret;
	}

	ret = parse_size(&arg, &mgmt_info_size);
	if (ret < 0) {
		lites_pr_err("parse_size (arg = %s, ret = %d)", arg, ret);
		return ret;
	}

	/** 余分な文字がある場合も解析を失敗したことにする */
	if (!is_delimiter(*arg)) {
		lites_pr_err("is_delimiter (arg = %s)", arg);
		return -EINVAL;
	}

	lites_pr_dbg("mgmt_info_start = %llx, mgmt_info_size = %x",
		     mgmt_info_start, mgmt_info_size);

	mgmt_info_parse_done = 1;
	return 0;
}
early_param("lites_mgmt_info", early_lites_mgmt_info);
#endif	// LITES_FTEN_MM_DIRECO

#endif /* LITES_FTEN */
