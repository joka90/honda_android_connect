/**
 * Copyright (c) 2011-2012 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
/**
 * @file region.c
 *
 * @brief リージョンに関する共通操作用関数を定義
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
#ifdef LITES_FTEN
#include "mgmt_info.h"
#include "parse.h"
#endif /* LITES_FTEN */

static struct lites_region *__regions[LITES_REGIONS_NR_MAX];
struct lites_region **regions = __regions;
int regions_nr = 0;
#ifdef LITES_FTEN
int lites_log_filter = 0;   // 初期値はフィルタなし
int lites_trace_out = 1;
#endif /* LITES_FTEN */

/**
 * @brief リージョン登録用関数
 *
 * @param[in]  reg リージョン構造体
 *
 * @retval 0  正常終了
 * @retval -1 異常終了
 */
int lites_register_region(struct lites_region *new)
{
    static DEFINE_SPINLOCK(spinlock); /** リエントラント禁止 */
    int ret;

    if (new == NULL)
        return -EINVAL;

    ret = -ENOMEM;
    regions_write_lock();
    spin_lock(&spinlock);
    if (regions_nr == LITES_REGIONS_NR_MAX)
        goto ret;

    ret = 0;
    regions[regions_nr++] = new;

ret:
    spin_unlock(&spinlock);
    regions_write_unlock();
    return ret;
}

/** T.B.D: 検索関数の共通化が必要 */
/**
 * @brief リージョン検索用関数（キーはiノード）
 *
 * @param[in]  inode iノード
 *
 * @retval リージョン構造体 見つかった
 * @retval NULL             見つからなかった
 */
struct lites_region *lites_find_region_with_inode(struct inode *inode)
{
    int i;

    regions_read_lock();
    for (i = 0; i < regions_nr; i++)
        if (regions[i]->inode == inode)
            break;
    regions_read_unlock();

    return i == regions_nr ? NULL : regions[i];
}

/**
 * @brief リージョン検索用関数（キーはリージョン番号）
 *
 * @param  region リージョン番号
 *
 * @retval リージョン構造体 見つかった
 * @retval NULL             見つからなかった
 */
struct lites_region *lites_find_region_with_number(unsigned int region)
{
    int i;

    regions_read_lock();

    for (i = 0; i < regions_nr; i++)
        if (region == regions[i]->rb->region)
            break;

    regions_read_unlock();

    return i == regions_nr ? NULL : regions[i];
}

/**
 * @brief リージョン検索用関数（キーはパス）
 *
 * @param  path パス
 *
 * @retval リージョン構造体 見つかった
 * @retval NULL             見つからなかった
 */
struct lites_region *lites_find_region_with_path(char *path)
{
    int i;

    regions_read_lock();

    for (i = 0; i < regions_nr; i++)
        if (!strcmp(regions[i]->path, path))
            break;

    regions_read_unlock();

    return i == regions_nr ? NULL : regions[i];
}

/**
 * @brief パス比較用関数
 * 　ファイル構造体のパスと指定したパスを比較する。
 *
 * @param[in]  file  ファイル構造体
 * @param[in]  path  パス
 * @param[in]  psize パスのサイズ
 *
 * @retval 0     ファイル構造体のパスと指定したパスが完全一致した
 * @retval 0以外 ファイル構造体のパスと指定したパスが一致しなかった
 */
int strcmp_file(struct file *file, const char *path, size_t psize)
{
    struct dentry *dentry = file->f_dentry;
    struct vfsmount *mnt = file->f_vfsmnt;
    int i = 0;

    /** T.B.D: 参照カウンタの操作が必要かも */
    while (1) {
        while (!IS_ROOT(dentry)) {
            if (i >= psize)
                break;
            if (strncmp(dentry->d_name.name, &path[i], dentry->d_name.len))
                break;
            i += dentry->d_name.len + 1;
            dentry = dentry->d_parent;
        }

        if (mnt == mnt->mnt_parent)
            break;

        dentry = mnt->mnt_mountpoint;
        mnt = mnt->mnt_parent;
    }

    return i != psize || strncmp(dentry->d_name.name, "/", dentry->d_name.len);
}

/**
 * @brief リージョン検索用関数（キーはファイル構造体）
 *
 * @param  file ファイル構造体
 *
 * @retval リージョン構造体 見つかった
 * @retval NULL             見つからなかった
 */
struct lites_region *lites_find_region_with_file(struct file *file)
{
    int i;

    regions_read_lock();

    for (i = 0; i < regions_nr; i++)
        if (!strcmp_file(file, regions[i]->path, regions[i]->psize))
            break;

    regions_read_unlock();

    return i == regions_nr ? NULL : regions[i];
}

#define lites_range_check(val, reg) \
    ((u64)(val) >= (u64)((reg)->rb->addr) && \
     (u64)(val) < (u64)((reg)->rb->addr + ((reg)->rb->size)))

int lites_region_range(u64 addr, u32 size)
{
    unsigned long flags = 0;
    int i;

    if (addr + size < addr)
        return -EINVAL;

    regions_read_lock();

    for (i = 0; i < regions_nr; i++) {
        region_read_lock(regions[i], flags);
        if (lites_range_check(addr, regions[i])
            || lites_range_check(addr + size, regions[i]))
            goto err;
        region_read_unlock(regions[i], flags);
    }

    regions_read_unlock();
    return 0;

err:
    region_read_unlock(regions[i], flags);
    regions_read_unlock();
    return -EINVAL;
}

#ifdef LITES_FTEN
/**
 * @brief リージョン獲得用関数
 *
 * @param[in]  start リージョン開始アドレス
 * @param[in]  size  リージョンのサイズ
 * @param[in]  psize パス名のサイズ
 * @param[in]  flag  マップしたトレース領域の初期化有無 0：初期化 1:初期化しない
 *
 * @retval リージョン構造体 正常終了
 * @retval NULL             異常終了
 */
struct lites_region *lites_alloc_region(u64 start, u32 size, u32 psize, u32 flag)
{
    struct lites_region *region;

    region = (struct lites_region *)vmalloc(sizeof(*region));
    if (region == NULL)
        return NULL;

    memset(region, 0, sizeof(*region));
    region->rb = (struct lites_ring_buffer *)ioremap_nocache(start, size);
    if (region->rb == NULL)
        goto err1;

    /* 初期化の有無に応じて初期化する */
    if (flag == 0) {
        memset(region->rb, 0, size);
    }

    region->path = (char *)vmalloc(psize);
    if (region->path == NULL)
        goto err2;

    memset(region->path, 0, psize);
    spin_lock_init(&region->lock);
    init_waitqueue_head(&region->waitq);

    return region;

err2:
    iounmap(region->rb);
err1:
    vfree(region);
    return NULL;
}
#else /* LITES_FTEN */
/**
 * @brief リージョン獲得用関数
 *
 * @param[in]  start リージョン開始アドレス
 * @param[in]  size  リージョンのサイズ
 * @param[in]  psize パス名のサイズ
 *
 * @retval リージョン構造体 正常終了
 * @retval NULL             異常終了
 */
struct lites_region *lites_alloc_region(u64 start, u32 size, u32 psize)
{
    struct lites_region *region;

    region = (struct lites_region *)vmalloc(sizeof(*region));
    if (region == NULL)
        return NULL;

    memset(region, 0, sizeof(*region));
    region->rb = (struct lites_ring_buffer *)ioremap_nocache(start, size);
    if (region->rb == NULL)
        goto err1;

    memset(region->rb, 0, size);
    region->path = (char *)vmalloc(psize);
    if (region->path == NULL)
        goto err2;

    memset(region->path, 0, psize);
    spin_lock_init(&region->lock);
    init_waitqueue_head(&region->waitq);

    return region;

err2:
    iounmap(region->rb);
err1:
    vfree(region);
    return NULL;
}
#endif /* LITES_FTEN */

/**
 * @brief リージョン解放用関数
 *
 * @param[in]  reg リージョン構造体
 */
void lites_free_region(struct lites_region *region)
{
    if (region == NULL)
        return;

    if (region->rb != NULL)
        iounmap(region->rb);

    if (region->path != NULL)
        vfree(region->path);

    vfree(region);
}

int lites_check_region(u64 addr, u32 size, u32 level, u32 access_byte)
{
    if (lites_region_range(addr, size) < 0)
        return -1;
    if (size < offsetof(struct lites_ring_buffer, log))
        return -2;
    if (!check_level_value(level))
        return -3;
    if (access_byte == LITES_VAR_ACCESS_BYTE)
        return 0;
    if (access_byte == 0)
        return -4;
    if (access_byte > LITES_ACCESS_BYTE_MAX)
        return -5;
    if (size - offsetof(struct lites_ring_buffer, log) < access_byte)
        return -6;
    return 0;
}

/**
 * @brief シスログレベルの解析用関数
 * 　syslogシステムコールやprintk経由の文字列の先頭3文字を解析する。解析が成功し
 * た場合、文字列のポインタと文字列のサイズを3文字分更新して登録レベルを返す。解
 * 析が失敗した場合、デフォルトの登録レベル（４）を返す。
 *
 * @param[in]  buf   文字列
 * @param[in]  count 文字列のサイズ
 *
 * @retval 0以上   登録レベル
 */
u32 parse_syslog_level(char *str, size_t count)
{
    static const char level[] = "01234567";
    char *found;

    if (count < 3)
        return LITES_DEFAULT_LEVEL;

    if (str[0] != '<' || str[2] != '>')
        return LITES_DEFAULT_LEVEL;

    found = strchr(level, str[1]);
    if (found == NULL)
        return LITES_DEFAULT_LEVEL;

    return found - level;
}

/**
 * @brief ログ書き込み用共通関数
 *
 * @param[in]  reg   リージョン構造体
 * @param[in]  buf   書き込むログ
 * @param[in]  lsize ログのサイズ
 *
 * @retval 0以上   書き込んだログのサイズ
 * @retval -EINVAL リージョンが不正である
 * @retval -ENOMEM リージョンに書き込むスペースがない
 * @retval -EFAULT コピーに失敗した
 * @retval -EPERM  トレース出力停止のため書き込み不可
 */
int lites_write_region(struct lites_region *region, const char *buffer, u16 logsize, u32 level,
               u32 (*write_extra_data)(struct lites_region *, u32, void *),
               void *extra_data, size_t extra_data_size)
{
    unsigned long flags = 0;
    struct task_struct *task;
    s64 read, read_sq;
    u32 write, trgr_lvl, size;
    int ret;

#ifdef LITES_FTEN
    /* トレース出力停止の場合は、トレースを書き込まない */
    if (lites_trace_out == 0) {
/* FUJITSU TEN 2012-12-26: ADA_LOG mod start */
#if 0
        lites_pr_err("logging is stopping now.");
        return -EPERM;
#else
        /* 準正常系：ログ出力停止のため、書込みサイズは0バイト */
        return 0;
#endif
/* FUJITSU TEN 2012-12-26: ADA_LOG mod end */
    }
#endif /* LITES_FTEN */


    region_write_lock(region, flags);

    /** ヘッダを除いたログのサイズを算出する（アクセスバイトを超えるログは切り捨てる） */
    size = logsize + extra_data_size;
    size = min_t(size_t, region->lop->max_log_size(region), size);

    lites_pr_dbg("write = %08x, read = %08x, read_sq = %08x, total = %08x, "
             "writable_size = %08x, size = %08x",
             region->rb->write, region->rb->read, region->rb->read_sq, region->total,
             region->lop->writable_size(region), size);

    ret = -EINVAL;
/* FUJITSU TEN 2012-12-14: ADA_LOG add start */
        /** トレースレベルがサポート範囲外の場合はエラーメッセージを出力する */
    if (!check_level_value(level))
        goto unlock;

        /** トレースレベルがサポート範囲内の場合はエラーメッセージを出力させない */
    ret = 0;
/* FUJITSU TEN 2012-12-14: ADA_LOG add end */
/* FUJITSU TEN 2012-12-14: ADA_LOG mod start */
/*  if (region->rb->level != LITES_LEVEL_NONE && unlikely(!check_level_range(level, region->rb->level))) */
// FUJITSU TEN 2013-0606 ADA LOG modify start
#if 0
    if (region->rb->level != :ITES_LEVEL_NONE && level != LITES_LEVEL_NONE && unlikely(!check_level_range(level, region->rb->level)))
#else
    if (region->rb->level != LITES_LEVEL_NONE && level != LITES_DIAG_LOG_LEVEL && unlikely(!check_level_range(level, region->rb->level)))
#endif
        goto unlock;
// FUJITSU TEN 2013-0606 ADA LOG modify end
/* FUJITSU TEN 2012-12-14: ADA_LOG mod end */

    ret = -ENOMEM;
    /** 書き込み可能なサイズはreadとwriteで決定する（read_sqの更新は防げない） */
    if (unlikely(size + region->header_size > region->lop->writable_size(region) && region->rb->wrap != LITES_WRAP_OFF)) {
        region->rb->cancel_times++;
        goto sendsig;
    }

    /** readとread_sqを更新する */
    ret = -EINVAL;
    read = region->rb->read;
    if (region->rb->block_rd == LITES_BLK_OFF)
        read = region->lop->update_index(region, region->rb->write, read,
                         size, region->rb->flags & LITES_FLAG_READ);
    read_sq = region->lop->update_index(region, region->rb->write, region->rb->read_sq,
                        size, region->rb->flags & LITES_FLAG_READ_SQ);
    if (unlikely(read < 0 || read_sq < 0))
        goto unlock;
    region->rb->read = read;
    region->rb->read_sq = read_sq;

    lites_pr_dbg("write = %08x, read = %08x, read_sq = %08x, total = %08x",
             region->rb->write, region->rb->read, region->rb->read_sq, region->total);

    /** ログを書き込む */
    write = region->lop->write_header(region, region->rb->write, size);
    if (write_extra_data) {
        // boot_id, seq_no, level を設定し、そのヘッダを書き込む
        lites_mgmt_info_set_header_info(level, extra_data);
        write = write_extra_data(region, write, extra_data);
    }
    write = region->lop->write_log(region, (char *)buffer, write, size - extra_data_size);

    /** writeを更新する */
    region->rb->write = write;
    region->rb->flags |= LITES_FLAG_READ | LITES_FLAG_READ_SQ;
    ret = size;

sendsig:
    /** シグナル通知 */
    trgr_lvl = region->rb->trgr_lvl;
    if (unlikely(trgr_lvl != LITES_TRGR_LVL_OFF &&
             region->lop->writable_size(region) <= trgr_lvl &&
             region->rb->sig_times == 0)) {
        region->rb->sig_times++;
        region_write_unlock(region, flags);

        rcu_read_lock();
        task = find_task_by_vpid(region->rb->pid);
        if (unlikely(task == NULL)) {
            rcu_read_unlock();
            lites_pr_err("Invalid pid 0x%08x", region->rb->pid);
            return -ESRCH;
        }
        send_sig(region->rb->sig_no, task, 1);
        rcu_read_unlock();

        goto wakeup;
    }

unlock:
    lites_pr_dbg("write = %08x, read = %08x, read_sq = %08x, total = %08x",
             region->rb->write, region->rb->read, region->rb->read_sq, region->total);
    region_write_unlock(region, flags);
wakeup:
    /** lites_read_sq_regionで待っているプロセスを起床する */
    wake_up_interruptible(&region->waitq);
    return ret;
}

static int __lites_read_region(struct lites_region *region, u32 *read, char *buffer, size_t size)
{
    u32 index = *read;
    u32 copied = 0;

    lites_pr_dbg("index = %08x, size - copied = %08x, region->lop->log_size(region, index) = %08x",
             index, size - copied, region->lop->log_size(region, index));

    /** バッファに満たないログは読み込まない */
    while (likely(size - copied >= region->lop->log_size(region, index))) {
        region->lop->read_log(region, &buffer[copied], index + region->header_size,
                      region->lop->log_size(region, index));
        copied += region->lop->log_size(region, index);
        index = region->lop->next_index(region, index);

        lites_pr_dbg("index = %08x, size - copied = %08x, region->lop->log_size(region, index) = %08x",
                 index, size - copied, region->lop->log_size(region, index));

        if (unlikely(index == region->rb->write))
            break;
    }

    *read = index;
    return copied;
}

int lites_read_region(struct lites_region *region, loff_t offset, char *buffer, size_t size)
{
    unsigned long flags = 0;
    s64 __read;
    u32 read;
    int ret = 0;

    if (unlikely(region == NULL || buffer == NULL))
        return -EINVAL;

    region_read_lock(region, flags);

    if (!(region->rb->flags & LITES_FLAG_READ))
        goto unlock; /** There is no data */

    __read = region->lop->calc_index(region, region->rb->read, offset);
    if (__read < 0)
        goto unlock; /** Maybe offset is bigger than region size */

    read = (u32)__read;
    ret = __lites_read_region(region, &read, buffer, size);
    if (ret < 0)
        goto unlock;

unlock:
    region_read_unlock(region, flags);
    return ret;
}

static int __lites_read_region_with_header(struct lites_region *region, u32 *read, char *buffer, size_t size)
{
    u32 index = *read;
    u32 copied = 0;

    lites_pr_dbg("index = %08x, size - copied = %08x, region->lop->log_size(region, index) = %08x",
             index, size - copied, region->lop->log_size(region, index));

    /** バッファに満たないログは読み込まない */
    while (likely(size - copied >= region->lop->log_size(region, index) + region->header_size)) {
        region->lop->read_header(region, &buffer[copied], index);
        copied += region->header_size;
        region->lop->read_log(region, &buffer[copied], index + region->header_size,
                      region->lop->log_size(region, index));
        copied += region->lop->log_size(region, index);
        index = region->lop->next_index(region, index);

        lites_pr_dbg("index = %08x, size - copied = %08x, region->lop->log_size(region, index) = %08x",
                 index, size - copied, region->lop->log_size(region, index));

        if (unlikely(index == region->rb->write))
            break;
    }

    *read = index;
    return copied;
}

int lites_ioctl_read_region(struct lites_region *region, loff_t offset, char *buffer, size_t size, int erase)
{
    unsigned long flags = 0;
    s64 __read;
    u32 read;
    int ret = 0;

    if (unlikely(region == NULL || buffer == NULL))
        return -EINVAL;

    region_read_lock(region, flags);

    if (!(region->rb->flags & LITES_FLAG_READ))
        goto unlock;

    __read = region->lop->calc_index_with_header(region, region->rb->read, offset);
    if (__read < 0)
        goto unlock;

    read = (u32)__read;
    ret = __lites_read_region_with_header(region, &read, buffer, size);
    if (ret < 0)
        goto unlock;

    if (erase != LITES_ERASE_OFF) {
        region->rb->read = read;
        /** 全てのデータを読み込んだ */
        if (region->rb->read == region->rb->write)
            region->rb->flags &= ~LITES_FLAG_READ;
    }

unlock:
    region_read_unlock(region, flags);
    return ret;
}

int lites_ioctl_read_sq_region(struct lites_region *region, loff_t offset, struct lites_buffer *buffer, size_t size)
{
    unsigned long flags = 0;
    s64 __read_sq;
    u32 read_sq;
    int ret;

    if (unlikely(region == NULL || buffer == NULL || buffer->buffer == NULL))
        return -EINVAL;

    while (1) {
        ret = 0;
        region_read_lock(region, flags);

        if (region->rb->flags & LITES_FLAG_READ_SQ) {
            /** オフセットが示す位置にデータが無い場合は、その位置に
             * データが来るまで待っても良いが、待っている間に他のプ
             * ロセスでread_sqが更新されたりする等、長い間待ち続けて
             * しまう可能性がある。よって、データが無い場合は即座に
             * ユーザへ戻すことにする。*/
            __read_sq = region->lop->calc_index_with_header(region, region->rb->read_sq, offset);
            if (__read_sq < 0)
                break;

            read_sq = (u32)__read_sq;
            ret = __lites_read_region_with_header(region, &read_sq, buffer->buffer, size);
            if (ret != 0)
                break;
        }

        /** 以降は読むべきデータが無い場合の処理 */
        if (region->rb->block_rd == LITES_BLK_OFF)
            break;

        region_read_unlock(region, flags);

        /** sleepする前にバッファを他のプロセスに譲る。 */
        lites_buffer_unlock(buffer);

        ret = -ERESTARTSYS;
        interruptible_sleep_on(&region->waitq);

        /** シグナルでログの書き込み待ちが中断されたかもしれないが、呼び
         *  出し元の関数ではシグナルで中断されたか、読み込みが失敗したか
         *  を判断しないので、ここで再度バッファのロックを獲得する。 */
        lites_buffer_lock(buffer);

        /** ユーザでタイムアウトを要実装 */
        if (signal_pending(current))
            goto ret;
    }

    /** 読み込みが成功した場合、read_sqは必ず更新される */
    if (ret > 0) {
        region->rb->read_sq = read_sq;
        if (region->rb->read_sq == region->rb->write)
            region->rb->flags &= ~LITES_FLAG_READ_SQ;
    }

    region_read_unlock(region, flags);
ret:
    return ret;
}

/**
 * @brief LITES用logフィルタのカーネル起動パラメータ解析用関数
 * 　カーネル起動パラメータに含まれる"lites_log_filter="に続く文字列を解析する。

 * @param[in]  arg "lites_log_filter="に続く文字列
 *
 * @retval 0  解析に成功した
 * @retval -1 解析に失敗した
 */
static __init int early_lites_log_filter(char *arg)
{
    int ret;
    lites_log_filter = 0;

    ret = parse_level(&arg, &lites_log_filter);
    if (ret < 0) {
        lites_pr_err("parse (arg = %s, ret = %d)", arg, ret);
        return ret;
    }

    /** 余分な文字がある場合も解析を失敗したことにする */
    if (!is_delimiter(*arg)) {
        lites_pr_err("is_delimiter (arg = %s)", arg);
        lites_log_filter = 0;
        return -EINVAL;
    }

    lites_pr_dbg("lites_log_filter = %x", lites_log_filter);
    return 0;
}
early_param("lites_log_filter", early_lites_log_filter);
