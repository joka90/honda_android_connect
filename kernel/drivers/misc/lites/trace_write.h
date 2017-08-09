/**
 * Copyright (c) 2011 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#ifndef __TRACE_WRITE_H
#define __TRACE_WRITE_H

#ifdef LITES_FTEN

#include <linux/preempt.h>
#include <linux/rwsem.h>
#include <linux/spinlock.h>
#include <linux/wait.h>


extern int lites_trace_init_done;						//!< トレースリージョンの初期化処理実施済みかどうかを示す

extern spinlock_t lites_trace_write_spinlock;			//!< トレース書き込みロック用spinlock_t
extern unsigned int lites_trace_write_cpu;				//!< トレース書き込みロック用cpu id

extern wait_queue_head_t lites_trace_region_wait_queue;	//!< poll handlerへ通知用wait queue


/**
 * lites_trace_writeをロックするためのマクロ
 */
#define lites_trace_write_lock(flags)							\
	do {														\
		unsigned int this_cpu;									\
																\
		preempt_disable();										\
		raw_local_irq_save(flags);								\
		this_cpu = smp_processor_id();							\
																\
		if (lites_trace_write_cpu == this_cpu) {				\
			lites_pr_err("Recursion bug (lites_trace_write_cpu=%u this_cpu = %u)",	\
							lites_trace_write_cpu, this_cpu);	\
			/** ロック獲得元へ戻るかを判定 */					\
			if (!oops_in_progress) {							\
				raw_local_irq_restore(flags);					\
				preempt_enable();								\
				lites_pr_info("lites_trace_write_lock not lock");	\
				return -EINVAL;									\
			}													\
																\
			/** ロック獲得元へ戻らない（do_exitで終了される）のでロック解放 */ \
			init_lites_trace_write_lock();						\
		}														\
																\
		lockdep_off();											\
		spin_lock(&lites_trace_write_spinlock);					\
		lites_trace_write_cpu = this_cpu;						\
	} while (0)
#define lites_trace_write_unlock(flags)				\
	do {											\
		lites_trace_write_cpu = UINT_MAX;			\
		spin_unlock(&lites_trace_write_spinlock);	\
		lockdep_on();								\
		raw_local_irq_restore(flags);				\
		preempt_enable();							\
	} while (0)
#define init_lites_trace_write_lock()				\
	do {											\
		lites_trace_write_cpu = UINT_MAX;			\
		spin_lock_init(&lites_trace_write_spinlock);\
	} while (0)


#ifdef LITES_FTEN_MM_DIRECO
/**
 * @brief 読み出し可能なリージョンリストを取得する関数（for ioctl.c）
 *
 * @param[in/out]  region_num     確保したリストサイズとリスト数の格納先
 *                                読み出し可能なリージョン有無のみの確認の場合は値に0を指定
 * @param[in/out]  region_list    読み出し可能なリージョン情報リストの格納先アドレス
 *                                読み出し可能なリージョン有無のみの確認の場合はNULLを指定
 *
 * @retval 1 読み出し可能なリージョンがある。（結果は引数の先に格納）
 * @retval 0 読み出し可能なリージョンがない
 * @retval 負 異常終了
 *
 */
extern int lites_trace_get_readable_region_list(unsigned int *region_num, struct lites_read_region_info *region_list);

/**
 * @brief 読み出し可能なトレースリージョン番号の連続性をチェックする関数（for ioctl.c）
 *
 * @param[in]  region_num     チェックするリージョン情報の数
 * @param[in]  region_list    チェックするリージョン情報リストのアドレス
 *
 */
extern void lites_trace_check_readable_region_list(unsigned int region_num, struct lites_read_region_info *region_list);

#else /* LITES_FTEN_MM_DIRECO */
/**
 * @brief 読み出し可能なリージョン番号リストを取得する関数（for ioctl.c）
 *
 * @param[in/out]  region_num     確保したリストサイズとリスト数の格納先
 *                                読み出し可能なリージョン有無のみの確認の場合は値に0を指定
 * @param[in/out]  region_no_list リージョンリストの格納先アドレス
 *                                読み出し可能なリージョン有無のみの確認の場合はNULLを指定
 *
 * @retval 1 読み出し可能なリージョンがある。（結果は引数の先に格納）
 * @retval 0 読み出し可能なリージョンがない
 * @retval 負 異常終了
 *
 */
extern int lites_trace_get_readable_region_list(unsigned int *region_num, unsigned int *region_no_list);

/**
 * @brief 読み出し可能なトレースリージョン番号の連続性をチェックする関数（for ioctl.c）
 *
 * @param[in]  region_num     チェックするリージョン番号の数
 * @param[in]  region_no_list チェックするリージョンリストのアドレス
 *
 */
extern void lites_trace_check_readable_region_list(unsigned int region_num, unsigned int *region_no_list);

#endif /* LITES_FTEN_MM_DIRECO */


#endif /* LITES_FTEN */

#endif  /** __TRACE_WRITE_H */
