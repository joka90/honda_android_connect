/**
 * @file mgmt_info.h
 *
 * @brief 管理情報APIの定義
 *
 * Copyright (c) 2011-2012 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#ifndef __LITES_MGMT_INFO_H
#define __LITES_MGMT_INFO_H


#ifdef LITES_FTEN

#include <linux/preempt.h>
#include <linux/rwsem.h>
#include <linux/spinlock.h>
#include <linux/lites_trace.h>


extern int mgmt_info_init_done;			//!< 管理情報の初期化処理実施済みかどうかを示す

extern spinlock_t mgmt_info_spinlock;	//!< 管理情報ロック用spinlock_t
extern unsigned int mgmt_info_cpu;		//!< 管理情報ロック用cpu id

/* FUJITSU TEN 2013-05-20: ADA_LOG add start */
extern unsigned int  lites_unixtime;	//!< unixタイムスタンプ情報
extern unsigned char lites_state;		//!< litesドライバ内部保持状態情報
extern unsigned char lites_rsv;			//!< litesドライバ内部リザーブ領域用
/* FUJITSU TEN 2013-05-20: ADA_LOG add end */

#ifdef LITES_FTEN_MM_DIRECO

extern struct lites_management_info *mgmt_info;	//!< マップした管理情報(マクロでのアクセス専用)

/**
 * グループ内の次のリージョン番号を取得するマクロ
 */
#define lites_trace_next_region(group_id, region_no)		\
	(														\
		((region_no + 1) <= mgmt_info->group_list[LITES_MGMT_GROUP_LIST_ACCESS_INDEX(group_id)].tail_region_no) ?	\
		(region_no + 1) : (mgmt_info->group_list[LITES_MGMT_GROUP_LIST_ACCESS_INDEX(group_id)].head_region_no)		\
	)

/**
 * グループ内のトレース書き込み先リージョン番号を取得するマクロ
 */
#define lites_trace_write_trace_region_no(group_id)			\
	(	mgmt_info->group_list[LITES_MGMT_GROUP_LIST_ACCESS_INDEX(group_id)].write_trace_region_no	)

/**
 * グループ内のリージョン数を取得するマクロ
 */
#define lites_trace_group_region_num(group_id)		(mgmt_info->group_list[LITES_MGMT_GROUP_LIST_ACCESS_INDEX(group_id)].region_num)

/**
 * グループ数を取得するマクロ
 */
#define lites_trace_group_num()		(mgmt_info->trace_group_num)

#endif /* LITES_FTEN_MM_DIRECO */

/**
 * 管理情報をロックするためのマクロ
 * 管理情報内の更新する際に使用する。参照では使用せず、上位側に任せる。
 */
#define mgmt_info_lock(flags, ret)						\
	do {												\
		unsigned int this_cpu;							\
														\
		preempt_disable();								\
		raw_local_irq_save(flags);						\
		this_cpu = smp_processor_id();					\
														\
		if (mgmt_info_cpu == this_cpu) {				\
			lites_pr_err("MGMT Recursion bug (mgmt_info_cpu=%u this_cpu=%u)",	\
					mgmt_info_cpu, this_cpu);			\
			/** ロック獲得元へ戻るかを判定 */			\
			if (!oops_in_progress) {					\
				raw_local_irq_restore(flags);			\
				preempt_enable();						\
				lites_pr_info("MGMT not lock");			\
				return ret;								\
			}											\
														\
			/** ロック獲得元へ戻らない（do_exitで終了される）のでロック解放 */ \
			mgmt_info_cpu = UINT_MAX;					\
			spin_lock_init(&mgmt_info_spinlock);		\
		}												\
														\
		lockdep_off();									\
		spin_lock(&mgmt_info_spinlock);					\
		mgmt_info_cpu = this_cpu;						\
	} while (0)
#define mgmt_info_unlock(flags)				\
	do {									\
		mgmt_info_cpu = UINT_MAX;			\
		spin_unlock(&mgmt_info_spinlock);	\
		lockdep_on();						\
		raw_local_irq_restore(flags);		\
		preempt_enable();					\
	} while (0)
#define init_mgmt_info_lock()				\
	do {									\
		mgmt_info_cpu = UINT_MAX;			\
		spin_lock_init(&mgmt_info_spinlock);\
	} while (0)


/** 管理情報操作用のAPI（mgmt_info.cで定義）*/


/**
 * @brief 管理情報獲得用関数
 *
 * @param[in]  addr 管理情報開始アドレス
 * @param[in]  size  管理情報のサイズ
 *
 * @retval リージョン構造体 正常終了
 * @retval NULL             異常終了
 */
extern struct lites_management_info *lites_mgmt_info_alloc(u64 addr, u32 size);

/**
 * @brief 管理情報解放用関数
 */
extern void lites_mgmt_info_free(void);

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
extern int lites_mgmt_info_check(u32 region_no, u32 addr_high, u32 addr_low, u32 size, u32 level, u32 access_byte, u16 wrap);

/**
 * @brief 管理情報のリージョン属性値チェック関数
 *
 * @param[in]  region_no リージョン番号
 * @param[in]  region_attr  管理情報のリージョン属性と比較するリージョンの属性値
 *
 * @retval 0 正常終了
 * @retval 負 異常終了
 */
extern int lites_mgmt_info_check_region(u32 region_no, struct region_attribute *region_attr);

/**
 * @brief リージョン属性検索用関数（キーはリージョン番号）
 *
 * @param  region リージョン番号
 *
 * @retval リージョン属性構造体 見つかった
 * @retval NULL             見つからなかった
 */
extern struct region_attribute *lites_mgmt_info_find_region(u32 region_no);

/**
 * @brief 管理情報にリージョン属性を登録する関数
 *
 * @param[in]  region_no リージョン番号
 * @param[in]  region_attr リージョン属性構造体
 *
 * @retval 0   正常終了
 * @retval -1  異常終了
 */
extern int lites_mgmt_info_add_region(u32 region_no, struct region_attribute *region_attr);

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
extern int lites_mgmt_info_add_group_info(u32 group_id, u32 region_num, u32 write_trace_region_no, u32 head_region_no, u32 tail_region_no);
#endif /* LITES_FTEN_MM_DIRECO */

/**
 * @brief 管理情報のコピーを作成する関数
 *
 * @param[in/out]  dst 管理情報のコピー先
 *
 * @retval 0   正常終了
 * @retval 負 異常終了
 */
extern int lites_mgmt_info_copy(struct lites_management_info *dst);

/**
 * @brief ヘッダ情報（boot_id,seq_no,level）設定関数
 *
 * @param[in]      level       トレースレベル
 * @param[in/out]  extra_data  ヘッダ情報のアドレス
 *
 * @retval 0  正常終了
 * @retval 負 異常終了
 */
extern int lites_mgmt_info_set_header_info(u32 level, void *extra_data);

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
extern int lites_mgmt_info_set_write_trace_region_no(u32 group_id, u32 region_no);

#else /* LITES_FTEN_MM_DIRECO */
/**
 * @brief トレース書き込み先リージョン番号取得関数
 *
 * @retval トレース書き込み先リージョン番号
 */
extern u32 lites_mgmt_info_get_write_trace_region_no(void);

/**
 * @brief 次のトレース書き込み先リージョン番号取得関数
 *
 * @param[in/out]  region_no inputとして現在のリージョン番号を指定。
 *                           outputは次の書き込み先のリージョン番号
 *
 * @retval 0  正常終了
 */
extern void lites_mgmt_info_get_next_trace_region_no(u32* region_no);

/**
 * @brief 前のトレース書き込み先リージョン番号取得関数
 *
 * @param[in/out]  region_no inputとして現在のリージョン番号を指定。
 *                           outputは前の書き込み先のリージョン番号
 *
 * @retval 0  正常終了
 */
extern void lites_mgmt_info_get_back_trace_region_no(u32* region_no);

/**
 * @brief トレース書き込みリージョン番号設定関数
 *
 * @param[in]  region_no リージョン番号
 *
 * @retval 0  正常終了
 * @retval 負 異常終了
 */
extern int lites_mgmt_info_set_write_trace_region_no(u32 region_no);
#endif /* LITES_FTEN_MM_DIRECO */

/**
 * @brief トレースリージョン数取得関数
 *
 * @retval トレースリージョン数
 */
extern u32 lites_mgmt_info_get_trace_region_num(void);

/**
 * @brief BootID取得関数
 *
 * @retval BootID
 */
extern u32 lites_mgmt_info_get_boot_id(void);

/**
 * @brief BootID更新関数
 *
 */
extern void lites_mgmt_info_update_boot_id(void);

/**
 * @brief magic_noチェック関数
 *
 * @retval 0  正常終了(magic_no/tail_magic_noに異常なし)
 * @retval 負 異常終了
 */
extern int lites_mgmt_info_check_magic_no(void);

/**
 * @brief 管理情報バックアップ関数
 *
 * @retval 0  正常終了
 */
extern int lites_mgmt_info_backup(void);

/**
 * @brief 管理情報リストア関数
 *
 * @retval 0        正常終了
 * @retval -EINVAL  lock異常
 */
extern int lites_mgmt_info_restore(void);

/**
 * @brief リージョンを初期化関数
 *        resume時に管理情報が壊れていた時の処理向け
 *
 * @param[in]  region_no リージョン番号
 *
 * @retval  0  正常終了
 * @retval 負  異常終了
 */
extern int lites_mgmt_info_initialize_region(u32 region_no);

#endif /* LITES_FTEN */

#endif  /** __LITES_MGMT_INFO_H */
