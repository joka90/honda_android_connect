/**
 * Copyright (c) 2011-2013 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/lites_trace.h>
#include "region.h"
#ifdef LITES_FTEN
#include "mgmt_info.h"
#include "trace_write.h"
#include <linux/sched.h>
#include <linux/time.h>

int lites_trace_init_done = 0;
spinlock_t lites_trace_write_spinlock;
unsigned int lites_trace_write_cpu = UINT_MAX;
wait_queue_head_t lites_trace_region_wait_queue;

/* FUJITSU TEN 2013-05-20: ADA_LOG add start */
unsigned int  lites_unixtime = 0;		//!< unixタイムスタンプ情報
unsigned char lites_state = 0x0;		//!< litesドライバ内部保持状態情報
unsigned char lites_rsv =   0x0;		//!< litesドライバ内部リザーブ領域用
/* FUJITSU TEN 2013-05-20: ADA_LOG add end */

#endif /* LITES_FTEN */

/* FUJITSU TEN 2013-02-27: ADA_LOG add start */
#if 0
#define CONFIG_LITES_TRACE_WRITE_D1
#endif
/* FUJITSU TEN 2013-02-27: ADA_LOG add end */

/* FUJITSU TEN 2013-02-14: ADA_LOG add start */
#ifdef CONFIG_LITES_LOG_SAVE
static int isSaveLog ( unsigned int group_id )
{
    int isSaveFlag = 0;            
    
    switch (group_id) {
//        case REGION_PRINTK:               // 既存printk用グループID.
//        case REGION_ADA_PRINTK:           // 車載用kernelログ用グループID.
//        case REGION_SYSLOG:               // syslog用グループID.
        case REGION_EVENT_LOG:            // LS1通信用グループID.
        case REGION_TRACE_BATTELY:        // 電源用グループID.
        case REGION_TRACE_MODE_SIG:       // モード管理/車両信号用グループID.
        case REGION_TRACE_AVC_LAN:        // AVC-LAN通信用グループID.
        case REGION_TRACE_KEY:            // キー用グループID.
        case REGION_TRACE_ERROR:          // エラー用グループID.
        case REGION_TRACE_DIAG_ERROR:     // ダイアグ用ソフトエラー用グループID.
        case REGION_TRACE_HARD_ERROR:     // ハードエラー用グループID.
        case REGION_TRACE_DIAG_HARD_ERROR: // ダイアグ用ハードエラー用グループID.
        case REGION_TRACE_APL_INSTALL:    // アプリインストール履歴用グループID.
        case REGION_TRACE_OS_UPDATE:      // OSバージョンアップ履歴ログ用グループID.
        case REGION_TRACE_DRIVER:         // ドライバトレース用グループID.
        case REGION_TRACE_MAKER:          // メーカ固有ログ用グループID.
//        case REGION_ANDROID_ADA_MAIN:     // 車載用Android(Main)ログ用グループID.
//        case REGION_ANDROID_MAIN:         // Android既存ログ用グループID.
//        case REGION_ANDROID_SYSTEM:       // systemログ用グループID.
//        case REGION_ANDROID_EVENT:        // eventログ用グループID.
//        case REGION_ANDROID_RADIO:        // radioログ用グループID.
        case REGION_PROC_SELF_NET_DEV:    // /proc/self/net/dev用グループID.
        case REGION_PROC_SLABINFO:        // /proc/slabinfo用グループID.
        case REGION_PROC_VMSTAT:          // /proc/vmstat用グループID.
        case REGION_PROC_INTERRUPTS:      // /proc/interrupts用グループID.
        case REGION_PROC_DISK:            // eMMC容量トレース用グループID.

            isSaveFlag = 1;               // ログの出力制限あり
            break;

        default:
            isSaveFlag = 0;               // ログの出力制限なし
            break;
    }

    return isSaveFlag;
}
#endif /* CONFIG_LITES_LOG_SAVE */
/* FUJITSU TEN 2013-02-14: ADA_LOG add end */

static u32 write_trace_header(struct lites_region *reg, u32 write, void *data)
{
#ifdef LITES_FTEN
	size_t size = sizeof(struct lites_trace_header); /** boot_id, seq_no, level, trace_id, trace_no */
#else /* LITES_FTEN */
	size_t size = 2 * sizeof(unsigned int); /** trace_id, trace_no */
#endif /* LITES_FTEN */
	u32 rest;

	lites_pr_dbg("write = %08x, size = %08x", write, size);

	lites_assert(write <= reg->total);

	if (write + size >= reg->total) {
		rest = size - (reg->total - write);

		memcpy(&reg->rb->log[write], data, reg->total - write);
		memcpy(&reg->rb->log[0], data + size - rest, rest);

		return rest;
	} else {
		memcpy(&reg->rb->log[write], data, size);

		if (write + size == reg->total)
			return 0;
		return write + size;
	}
}

/**
 * @brief トレースリージョン内をクリアする関数
 *
 * @param[in]  region リージョン情報のアドレス
 * @param[in]  region_no リージョン番号
 *
 * @retval 0 正常終了
 * @retval 負 異常終了
 */
int lites_trace_clear_region(struct lites_region *region, unsigned int region_no)
{
	unsigned long flags = 0;

	// 読み出しが完了してないトレースリージョンをクリアする場合は、メッセージを残す(デバッグ用)
	if (region->rb->flags & LITES_FLAG_READ) {
		lites_pr_dbg("trace_region(%08x) is cleared though reading is not completed.", region_no);
	}

	// リージョン内をクリア
	region_write_lock(region, flags);	/*pgr0039*/
	region->rb->read     = 0;
	region->rb->read_sq  = 0;
	region->rb->write    = 0;
	region->rb->flags   &= ~(LITES_FLAG_READ | LITES_FLAG_READ_SQ);
	region_write_unlock(region, flags);

	lites_pr_dbg("trace_region(%08x) is cleared.", region_no);
	return 0;
}

#ifdef LITES_FTEN_MM_DIRECO
/**
 * @brief トレースリージョン情報を取得する関数
 *        書き込み先予定のトレースリージョンにログが書き込めない場合は、
 *        次のリージョン番号へ変更し、そのトレースリージョン情報を返却する。
 *
 * @param[in]      logsize    トレースに付加するログのサイズ
 * @param[in]      group_id   書き込み先のトレースグループのID
 * @param[in/out]  region_no  in :書き込み予定のトレースリージョン番号
 *                            out:書き込み先となったトレースリージョン番号
 *                                （リージョン番号変更込み）
 *
 * @retval トレースリージョン情報のアドレス 正常終了
 * @retval NULL      異常終了
 */
struct lites_region* lites_trace_get_trace_region(size_t logsize, unsigned int group_id, unsigned int *region_no)
{
	struct lites_region *region;
	int ret;
	int trace_size;
	unsigned int next_region_no = 0;	// 次の書き込み先のリージョン番号

	// 書き込み先のリージョンのリージョン情報を取得
	region = lites_find_region_with_number(*region_no);
	if (unlikely(region == NULL)) {
		lites_pr_err("not initialized region. region_no:%d", *region_no);
		return NULL;
	}

	// 書き込み先リージョンの残容量をチェックし、
	// 書き込むログ（レコード長）が入らない場合は、シフト制御（リージョン切り替え）を実施する。
	if (region->rb->access_byte == 0xFFFFFFFF) {
		// 可変長ログの場合は、トレースレコード長で
		trace_size = sizeof(struct lites_trace_record_header) + logsize;
	} else {
		// 固定長ログの場合は、access_byteで
		trace_size = region->rb->access_byte;
	}

	if (unlikely(region->lop->writable_size(region) < trace_size)) {
		// 次の書き込み先のリージョン番号を取得
		next_region_no = lites_trace_next_region(group_id, *region_no);

		// 書き込み先のリージョン（新）のリージョン情報を取得
		region = lites_find_region_with_number(next_region_no);
		if (unlikely(region == NULL)) {
			lites_pr_err("not initialized region. region_no:%d", *region_no);
			return NULL;
		}

		// リージョン内をクリア
		ret = lites_trace_clear_region(region, next_region_no);
		if (unlikely(ret < 0)) {
			lites_pr_err("lites_trace_clear_region error");
			return NULL;
		}

		// 変更したリージョン番号を管理情報に設定する
		ret = lites_mgmt_info_set_write_trace_region_no(group_id, next_region_no);
		if (unlikely(ret < 0)) {
			lites_pr_err("lites_mgmt_info_set_write_trace_region_no (ret=%d)", ret);
			return NULL;
		}

		// 書き込み先のリージョン番号を格納
		*region_no = next_region_no;
	}

	lites_pr_dbg("select region_no=%08x", *region_no);
	return region;
}

/**
 * @brief トレース書き込み関数
 *
 * @param[in]      group_id   書き込み先のトレースグループのID
 * @param[in/out]  trc_fmt    トレースフォーマット
 *
 * @retval 0以上   書き込んだサイズ
 * @retval 負      異常終了
 */
static int lites_trace_write_inter(unsigned int group_id, struct lites_trace_format *trc_fmt)
{
	struct lites_region *region;
	struct lites_trace_header *trc_header;
	int ret;
	unsigned long flags = 0;
	unsigned int new_region_no, old_region_no;

	if (unlikely(lites_trace_init_done == 0)) {
		lites_pr_err("lites_trace_init is not completed");
		return -EPERM;
	}

	trc_header = trc_fmt->trc_header;
/* FUJITSU TEN 2013-05-20: ADA_LOG add start */
    // レコードヘッダに特殊操作を実施
   if (unlikely(group_id == REGION_TRACE_HEADER)) {
//        lites_pr_info("lites_trace_write_inter group_id = %d(0x%08x), trace_id = %d(0x%08x)", group_id, group_id, trc_header->trace_id, trc_header->trace_id);
        unsigned char mask_state = 0x0;
        switch (trc_header->trace_id) {
          case SET_HEADER_TSTMP:
               memcpy(&lites_unixtime, (char*)(trc_fmt->buf), sizeof(lites_unixtime));
               break;
               
          case SET_SUSPEND_SQ_S:
               mask_state = 0x01;
               lites_state = lites_state | mask_state;
               break;
               
          case SET_SUSPEND_SQ_E:
               mask_state = 0x01;
               lites_state = lites_state & ~(mask_state);
               break;
               
          default:
               lites_pr_err("lites_trace_write_inter not support trace_id(%d)", trc_header->trace_id);
               break;
        }
        /* ドラレコへの保存処理は行わない */
        return 0;
    }

    trc_header->i_state  = lites_state;
    trc_header->rsv      = lites_rsv;
/* FUJITSU TEN 2013-05-20: ADA_LOG add end */

	if (unlikely((group_id == 0) || (group_id > lites_trace_group_num()))) {
		lites_pr_err("invalid group_id:%d", group_id);
		return -EINVAL;
	}

/* FUJITSU TEN 2012-12-21: ADA_LOG modify start */
/* 車載用ログをドラレコに保存しない             */
#ifdef CONFIG_LITES_TRACE_WRITE_D1
	if ( ( likely ( group_id == REGION_ADA_PRINTK ) ||
                       ( group_id == REGION_ANDROID_ADA_MAIN) ) ) {
		return 0;
	} 
#endif
/* FUJITSU TEN 2012-12-21: ADA_LOG modify end */
/* FUJITSU TEN 2013-02-14: ADA_LOG add start  */
/* 車載用ログをドラレコに保存しない           */
#ifdef CONFIG_LITES_LOG_SAVE
    if ( isSaveLog ( group_id ) == 1 ) {
        /* ログの出力制限のあるログは出力しない(ログ出力制限用) */
        return 0;
    }
#endif
/* FUJITSU TEN 2013-02-14: ADA_LOG add end */

	lites_pr_dbg("trc_fmt->count = %u", trc_fmt->count);

	// トレース書き込み専用ロック 
	lites_trace_write_lock(flags);	/*pgr0039*/

	// 管理情報から書き込み先のリージョン番号を取得
	// この時点では、まだ仮のリージョン番号
	new_region_no = lites_trace_write_trace_region_no(group_id);
	old_region_no = new_region_no;

	// 書き込み先のリージョンのリージョン情報を取得
	// この時点で、ログが書き込めるリージョン情報＆リージョン番号が確定される
	region = lites_trace_get_trace_region(trc_fmt->count, group_id, &new_region_no);
	if (unlikely(region == NULL)) {
		lites_pr_err("lites_trace_get_trace_region error");
		lites_trace_write_unlock(flags);
		return -EPERM;
	}

	// write_trace_headerでレコードヘッダの一部（lites_trace_header構造体）を書き込む
	ret = lites_write_region(region, (char *)trc_fmt->buf, trc_fmt->count, trc_header->level,
				 write_trace_header, trc_header, sizeof(struct lites_trace_header));

	if (unlikely(ret < 0)) {
		lites_pr_err("lites_write_region (ret=%d)", ret);
	} else if (unlikely(new_region_no != old_region_no)) {
		// lites_poll(dev.c)の待ち合わせを解除
		lites_pr_dbg("wake_up_interruptible");
		wake_up_interruptible(&lites_trace_region_wait_queue);

		// printkリージョンのリージョンフルを確認
		if (group_id == REGION_PRINTK) {
			LITES_LOGFALL_DEBUG("wake_up_interruptible!!!(REGION_PRINTK region:%u -> %u)", old_region_no, new_region_no);
		}
	}

	lites_trace_write_unlock(flags);
	return ret;
}

/**
 * @brief トレース書き込み関数
 *
 * @param[in]      group_id   書き込み先のトレースグループのID
 * @param[in/out]  trc_fmt    トレースフォーマット
 *
 * @retval 0以上   書き込んだサイズ
 * @retval 負      異常終了
 */
int lites_trace_write(unsigned int group_id, struct lites_trace_format *trc_fmt)
{
	int ret = 0;
	struct lites_trace_header *trc_header;

	if ((trc_fmt == NULL) || (trc_fmt->trc_header == NULL)) {
		lites_pr_err("invalid trc_fmt");
		return -EINVAL;
	}

	trc_header = trc_fmt->trc_header;

	ret = lites_trace_write_inter(group_id, trc_fmt);

	if (trc_header->level == LITES_DIAG_LOG_LEVEL) {
		if (group_id == REGION_TRACE_ERROR) {
			/* ダイアグ用ソフトエラーログ */
			ret = lites_trace_write_inter(REGION_TRACE_DIAG_ERROR, trc_fmt);
		} else if (group_id == REGION_TRACE_HARD_ERROR) {
			/* ダイアグ用ハードエラーログ */
			ret = lites_trace_write_inter(REGION_TRACE_DIAG_HARD_ERROR, trc_fmt);
		} else {
			/* 対象外のグループＩＤ */
		}
	}

	return ret;
}
EXPORT_SYMBOL(lites_trace_write);

#else /* LITES_FTEN_MM_DIRECO */
/**
 * @brief トレースリージョン情報を取得する関数
 *        書き込み先予定のトレースリージョンにログが書き込めない場合は、
 *        次のリージョン番号へ変更し、そのトレースリージョン情報を返却する。
 *
 * @param[in]      logsize    トレースに付加するログのサイズ
 * @param[in/out]  region_no  in :書き込み予定のトレースリージョン番号
 *                            out:書き込み先となったトレースリージョン番号
 *                                （リージョン番号変更込み）
 *
 * @retval トレースリージョン情報のアドレス 正常終了
 * @retval NULL      異常終了
 */
struct lites_region* lites_trace_get_trace_region(size_t logsize, unsigned int *region_no)
{
	struct lites_region *region;
	int ret;
	int trace_size;

	// 書き込み先のリージョンのリージョン情報を取得
	region = lites_find_region_with_number(*region_no);
	if (unlikely(region == NULL)) {
		lites_pr_err("not initialized region. region_no:%d", *region_no);
		return NULL;
	}

	// 書き込み先リージョンの残容量をチェックし、
	// 書き込むログ（レコード長）が入らない場合は、シフト制御（リージョン切り替え）を実施する。
	if (region->rb->access_byte == 0xFFFFFFFF) {
		// 可変長ログの場合は、トレースレコード長で
		trace_size = sizeof(struct lites_trace_record_header) + logsize;
	} else {
		// 固定長ログの場合は、access_byteで
		trace_size = region->rb->access_byte;
	}

	if (unlikely(region->lop->writable_size(region) < trace_size)) {
		// 次の書き込み先のリージョン番号を取得
		lites_mgmt_info_get_next_trace_region_no(region_no);

		// 書き込み先のリージョン（新）のリージョン情報を取得
		region = lites_find_region_with_number(*region_no);
		if (unlikely(region == NULL)) {
			lites_pr_err("not initialized region. region_no:%d", *region_no);
			return NULL;
		}

		// リージョン内をクリア
		ret = lites_trace_clear_region(region, *region_no);
		if (unlikely(ret < 0)) {
			lites_pr_err("lites_trace_clear_region error");
			return NULL;
		}

		// 変更したリージョン番号を管理情報に設定する
		ret = lites_mgmt_info_set_write_trace_region_no(*region_no);
		if (unlikely(ret < 0)) {
			lites_pr_err("lites_mgmt_info_set_write_trace_region_no (ret=%d)", ret);
			return NULL;
		}
	}

	lites_pr_dbg("select region_no=%08x", *region_no);
	return region;
}

int lites_trace_write(unsigned int region_no, struct lites_trace_format *trc_fmt)
{
	struct lites_region *region;
	struct lites_trace_header *trc_header;
	int ret;
#ifdef LITES_FTEN
	unsigned long flags = 0;
	unsigned int new_region_no, old_region_no;

	if (unlikely(lites_trace_init_done == 0)) {
		lites_pr_err("lites_trace_init is not completed");
		return -EPERM;
	}

	trc_header = trc_fmt->trc_header;
	lites_pr_dbg("trc_fmt->count = %u", trc_fmt->count);

	// トレース書き込み専用ロック 
	lites_trace_write_lock(flags);	/*pgr0039*/

	// 管理情報から書き込み先のリージョン番号を取得
	// この時点では、まだ仮のリージョン番号
	new_region_no = lites_mgmt_info_get_write_trace_region_no();
	old_region_no = new_region_no;

	// 書き込み先のリージョンのリージョン情報を取得
	// この時点で、ログが書き込めるリージョン情報＆リージョン番号が確定される
	region = lites_trace_get_trace_region(trc_fmt->count, &new_region_no);
	if (unlikely(region == NULL)) {
		lites_pr_err("lites_trace_get_trace_region error");
		lites_trace_write_unlock(flags);
		return -EPERM;
	}

	// write_trace_headerでレコードヘッダの一部（lites_trace_header構造体）を書き込む
	ret = lites_write_region(region, (char *)trc_fmt->buf, trc_fmt->count, trc_header->level,
				 write_trace_header, trc_header, sizeof(struct lites_trace_header));
	if (unlikely(ret < 0)) {
		lites_pr_err("lites_write_region (ret=%d)", ret);
	} else if (unlikely(new_region_no != old_region_no)) {
		// lites_poll(dev.c)の待ち合わせを解除
		lites_pr_dbg("wake_up_interruptible");
		wake_up_interruptible(&lites_trace_region_wait_queue);
	}

	lites_trace_write_unlock(flags);
	return ret;

#else /* LITES_FTEN */

	region = lites_find_region_with_number(region_no);
	if (unlikely(region == NULL)) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	trc_header = trc_fmt->trc_header;

	lites_pr_dbg("trc_fmt->count = %u", trc_fmt->count);

	ret = lites_write_region(region, (char *)trc_fmt->buf, trc_fmt->count, trc_header->level,
				 write_trace_header, &trc_header->trace_id, 2 * sizeof(unsigned int));
	if (unlikely(ret < 0))
		lites_pr_err("lites_write_region (ret = %d)", ret);

	return ret;
#endif /* LITES_FTEN */
}
EXPORT_SYMBOL(lites_trace_write);
#endif /* LITES_FTEN_MM_DIRECO */

int lites_trace_write_multi(unsigned int num, unsigned int *pos,
			    struct lites_trace_format *trc_fmt)
{
	int i, ret;

	for (i = 0; i < num; i++) {
		ret = lites_trace_write(pos[i], &trc_fmt[i]);
		if (unlikely(ret < 0))
			return ret;
	}

	return 0;
}
EXPORT_SYMBOL(lites_trace_write_multi);



#ifdef LITES_FTEN_MM_DIRECO
/**
 * @brief 読み出し可能なトレースリージョン番号の連続性をチェックする関数（for ioctl.c）
 *
 * @param[in]  region_num     チェックするリージョン情報の数
 * @param[in]  region_list    チェックするリージョン情報リストのアドレス
 *
 */
void lites_trace_check_readable_region_list(unsigned int region_num, struct lites_read_region_info *region_list)
{
	int i;
	unsigned int next_region_no;	// リストに格納されてるリージョン番号から次のリージョン番号を求めた場合の値

	// 0,1個はチェックする必要なし
	// リスト内のリージョン番号の連続性を確認する
	for (i = 1; i < region_num; ++i) {
		// 1つ前のindexが指すリージョン情報内のグループIDと
		// indexが指すリージョン情報内のグループIDが一致する場合のみ、連続性をチェックする
		if (region_list[i-1].group_id == region_list[i].group_id) {
			// 1つ前のindexが指すリージョン番号から次のリージョン番号を求める。
			next_region_no = lites_trace_next_region(region_list[i-1].group_id, region_list[i-1].region_no);
			// これがindex指すリージョン番号と一致している場合は、
			// リージョン番号に連続性があるということになる。
			if (unlikely(region_list[i].region_no != next_region_no)) {
				// 歯抜けなので、メッセージで通知する
				lites_pr_info("readable region number is not consecutive.");
			}
		}
	}
}

/**
 * @brief トレースグループ内における読み出し可能なリージョンリストを取得する関数
 *
 * @param[in]      group_id       トレースグループのID
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
int lites_trace_get_readable_region_list_in_group(unsigned int group_id, unsigned int *region_num, struct lites_read_region_info *region_list)
{
	unsigned int trace_region_num;	// トレースグループ内のトレースリージョン数
	unsigned int before_region_no;	// 以前にチェックしたリージョン番号
	unsigned int check_region_no;	// グループ内のチェックするリージョン番号
	struct lites_region *region;
	unsigned int find_num;			// グループ内の読み出し可能なリージョン数
	unsigned int array_num = *region_num;	// 格納先リージョンリストの残り配列数
	int i;
	int ret = 0;

	// トレースグループ内のトレースリージョン数の取得
	trace_region_num = lites_trace_group_region_num(group_id);

	// 初期値の設定
	before_region_no = lites_trace_write_trace_region_no(group_id);	// 初期値はトレースグループ内の書き込み先リージョン番号
	find_num = 0;

	// 最古のリージョンから、リージョン情報をチェック（書き込み先リージョンは除外）
	// 読み出し可能なデータが残ってる場合は、返却リストに追加
	for (i = 0; i < trace_region_num - 1; ++i) {

		// チェック対象のリージョン番号を取得する
		check_region_no = lites_trace_next_region(group_id, before_region_no);

		region = lites_find_region_with_number(check_region_no);
		if (unlikely(region == NULL)) {
			lites_pr_err("not initialized region(%08x).", check_region_no);
			return -EPERM;
		}

		if (region->rb->flags & LITES_FLAG_READ) {
			ret = 1;
			//  読み出し可能なリージョン有無チェックの場合は、見つけ次第、ループ終了
			if (array_num == 0 || region_list == NULL) {
				break;
			}

			if (unlikely(find_num == array_num)) {
				// 既に読み出し可能リージョン数が、格納先の配列数に達している場合は、
				// ここでループを抜ける
				lites_pr_info("buffer area is a little, full_region_no cannot finish being stored.");
				break;
			}
			region_list[find_num].group_id = group_id;
			region_list[find_num].region_no = check_region_no;
			++find_num;
		}

		before_region_no = check_region_no;	// 次の検索用に格納
	}

	*region_num = find_num;
	return ret;
}

/**
 * @brief 読み出し可能なリージョンリストを取得する関数（for ioctl.c, dev.c）
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
int lites_trace_get_readable_region_list(unsigned int *region_num, struct lites_read_region_info *region_list)
{
	int i;
	int ret = 0;
	unsigned long flags = 0;				// ロック時のflagを格納
	unsigned int group_num;					// グループ数
	unsigned int array_num = *region_num;	// 格納先の配列確保数
	int index = 0;							// region_listの配列index
	struct lites_read_region_info *list = NULL;	// region_listの書き込み先ポインタ 
	unsigned int find_num = 0;
	int return_code = 0;

	if (unlikely(region_num == NULL)) {
		lites_pr_err("parameter error.");
		return -EPERM;
	}

	// 読み出し可能なリストを作成中にトレース書き込みによる更新を避けるため、
	// トレース書き込みのロックを実施
	lites_trace_write_lock(flags);	/*pgr0039*/

	// 検索開始前の初期値設定
	*region_num = 0;	// カウント数格納先の値を0とする（配列数は既にarray_numに格納済み）
	index = 0;

	// 各トレースグループ内を検索する
	// group_idは１からスタート
	group_num = lites_trace_group_num();
	for (i = 1; i <= group_num; ++i) {
		find_num = array_num - index;	// 書き込み先のregion_list配列の残数を設定する
		if (find_num == 0) {
			list = NULL;
		} else {
			list = &region_list[index];	// 書き込み先の開始位置を設定
		}
		// lites_trace_get_readable_region_list_in_group関数実施前のfind_numは、
		// 格納用に確保した配列数が入る。
		// 関数実施後は、確保領域に格納した数が返る。
		ret = lites_trace_get_readable_region_list_in_group(i, &find_num, list);
		if (unlikely(ret < 0)) {
			lites_pr_err("readable_region_list_in_group error. group_id(%d).", i);
			lites_trace_write_unlock(flags);
			return -EPERM;
		}
		else if (ret == 1) {
			// グループ内に読み出し可能なリージョンを見つけた場合は、復帰値を1へ
			return_code = 1;
			//  読み出し可能なリージョン有無チェックの場合は、見つけ次第、ループ終了
			if (array_num == 0 || region_list == NULL) {
				break;
			}

			index += find_num;
			*region_num += find_num;	// 見つけた分だけ、格納先のカウント数をプラス
			// region_list は、lites_trace_get_readable_region_list_in_group関数内で
			// 見つけた分だけ格納済みである。
		}
	}

	// アンロック
	lites_trace_write_unlock(flags);

	return return_code;
}
#else /* LITES_FTEN_MM_DIRECO */
/**
 * @brief 読み出し可能なトレースリージョン番号の連続性をチェックする関数（for ioctl.c）
 *
 * @param[in]  region_num     チェックするリージョン番号の数
 * @param[in]  region_no_list チェックするリージョンリストのアドレス
 */
void lites_trace_check_readable_region_list(unsigned int region_num, unsigned int *region_no_list)
{
	int i;
	unsigned int next_region_no;

	// 0,1個はチェックする必要なし
	// リスト内のリージョン番号の連続性を確認する
	for (i = 1; i < region_num; ++i) {
		// 1つ前のindexが指すリージョン番号から次のリージョン番号を求める。
		// これがindex指すリージョン番号と一致している場合は、
		// リージョン番号に連続性があるということになる。
		next_region_no = region_no_list[i-1];
		lites_mgmt_info_get_next_trace_region_no(&next_region_no);
		if (unlikely(region_no_list[i] != next_region_no)) {
			// 歯抜けなので、メッセージで通知する
			lites_pr_info("readable region number is not consecutive.");
		}
	}
}

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
 */
int lites_trace_get_readable_region_list(unsigned int *region_num, unsigned int *region_no_list)
{
	unsigned int write_trace_region_no;
	unsigned int trace_region_num;
	unsigned int check_region_no;
	struct lites_region *region;
	unsigned int full_region_no_num;
	unsigned long flags = 0;
	int i;
	int ret = 0;

	// トレース書き込みのロックを実施
	lites_trace_write_lock(flags);	/*pgr0039*/

	// 書き込み先リージョン番号の取得
	write_trace_region_no = lites_mgmt_info_get_write_trace_region_no();

	// トレースリージョン数の取得
	trace_region_num = lites_mgmt_info_get_trace_region_num();

	// 初期値の設定
	check_region_no = write_trace_region_no;
	full_region_no_num = 0;

	// 最古のリージョンから、リージョン情報をチェック（書き込み先リージョンは除外）
	// 読み出し可能なデータが残ってる場合は、返却リストに追加
	for (i = 0; i < trace_region_num - 1; ++i) {
		// チェックするリージョン番号を取得し、リージョン情報を取得する
		lites_mgmt_info_get_next_trace_region_no(&check_region_no);
		region = lites_find_region_with_number(check_region_no);
		if (unlikely(region == NULL)) {
			lites_pr_err("not initialized region(%08x).", check_region_no);
			lites_trace_write_unlock(flags);
			return -EPERM;
		}

		if (region->rb->flags & LITES_FLAG_READ) {
			ret = 1;
			//  読み出し可能なリージョン有無チェックの場合は、見つけ次第、ループ終了
			if (*region_num == 0 || region_no_list == NULL) {
				break;
			}

			if (unlikely(full_region_no_num == *region_num)) {
				// 既に読み出し可能リージョン数が、格納先の配列数に達している場合は、
				// ここでループを抜ける
				lites_pr_info("buffer area is a little, full_region_no cannot finish being stored.");
			}
			region_no_list[full_region_no_num] = check_region_no;
			++full_region_no_num;
		}
	}

	// アンロック
	lites_trace_write_unlock(flags);

	*region_num = full_region_no_num;
	return ret;
}
#endif /* LITES_FTEN_MM_DIRECO */
