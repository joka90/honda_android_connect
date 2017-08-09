/**
 * Copyright (c) 2011-2013 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
#ifndef __LITES_REGION_H
#define __LITES_REGION_H

#include <linux/rwsem.h>
#include "buffer.h"

#define LITES_NAME             "lites"
#define LITES_PATH_MAX         4096
#define LITES_DEFAULT_LEVEL    4
#define LITES_LEVEL_MAX        7
#ifndef LITES_FTEN
#define LITES_REGIONS_NR_MAX   32
#endif /* LITES_FTEN */
#define LITES_SYSLOG_PATH      "syslog/lites/dev/"
#ifdef LITES_FTEN_MM_DIRECO
#define LITES_TRACE_PATH       "trace/lites/dev/"
#define LITES_MESSAGES_PATH       "messages/lites/dev/"
#endif /* LITES_FTEN_MM_DIRECO */

// FUJITSU TEN 2013-0606 ADA LOG modify start
#define LITES_DIAG_LOG_LEVEL    0xFFFF

#define check_level_range(l, max) ((l) <= (max))
#define check_level_value(l)      ((l) == LITES_DIAG_LOG_LEVEL ||(l) == -1 || check_level_range(l, LITES_LEVEL_MAX))
// FUJITSU TEN 2013-0606 ADA LOG modify end

extern struct lites_region **regions;
extern int regions_nr;

/** regsにアクセスする場合はregs_read_lock等のマクロで排他制御を実施すること */
#define regions_read_lock()    /** Nothing to do */
#define regions_read_unlock()  /** Nothing to do */
#define regions_write_lock()   /** Nothing to do */
#define regions_write_unlock() /** Nothing to do */

/**
 * リージョンのヘッダ部分をロックするためのマクロ
 */
#define region_read_lock(region, flags)                 \
    do {                                \
        unsigned int this_cpu;                  \
                                    \
        preempt_disable();                  \
        raw_local_irq_save(flags);              \
        this_cpu = smp_processor_id();              \
                                    \
        if (region->this_cpu == this_cpu) {         \
            lites_pr_err("Recursion bug (cpuid = %u)",  \
                     this_cpu);             \
            /** ロック獲得元へ戻るかを判定 */       \
            if (!oops_in_progress) {            \
                raw_local_irq_restore(flags);       \
                preempt_enable();           \
                return -EINVAL;             \
            }                       \
                                    \
            /** ロック獲得元へ戻らない（do_exitで終了される）のでロック解放 */ \
            init_region_lock(region);           \
        }                           \
                                    \
        lockdep_off();                      \
        spin_lock(&(region)->lock);             \
        region->this_cpu = this_cpu;                \
    } while (0)
#define region_read_unlock(region, flags)           \
    do {                            \
        region->this_cpu = UINT_MAX;            \
        spin_unlock(&(region)->lock);           \
        lockdep_on();                   \
        raw_local_irq_restore(flags);           \
        preempt_enable();               \
    } while (0)
#define region_write_lock(region, flags)   region_read_lock(region, flags)
#define region_write_unlock(region, flags) region_read_unlock(region, flags)
#define init_region_lock(region)            \
    do {                        \
        region->this_cpu = UINT_MAX;        \
        spin_lock_init(&(region)->lock);    \
    } while (0)

/**
 * @struct lites_ring_buffer
 * @brief リージョンのヘッダとログ部分（リングバッファ）を管理する構造体
 * 　本構造体はioremapでマッピングされた領域にアクセスするために使用する。
 */
struct lites_ring_buffer {
    u32 region;       /**< リージョン番号 */
    u32 size;         /**< リージョンのサイズ */
    u64 addr;         /**< リージョンの開始物理アドレス */
    u32 level;        /**< 登録レベル */
    u32 access_byte;  /**< 単一ログのサイズ */
    u32 write;        /**< ライトポインタ */
    u32 read_sq;      /**< 逐次リードポインタ（参照用リードポインタ */
    u32 read;         /**< 一括リードポインタ */
    u32 pid;          /**< PID */
    u32 sig_no;       /**< シグナル番号 */
    u32 trgr_lvl;     /**< 残容量 */
    u32 sig_times;    /**< シグナル発行回数 */
    u16 block_rd;     /**< ブロックフラグ */
    u16 wrap;         /**< 上書き禁止フラグ */
    u32 cancel_times; /**< 上書きキャンセル回数 */
    u32 flags;        /**< 各種フラグ */
    char log[];
};

#ifndef LITES_FTEN
/** 各種フラグの値 */
#define LITES_FLAG_READ    0x00000001 /** readで読むべきデータが存在する */
#define LITES_FLAG_READ_SQ 0x00000002 /** read_sqで読むべきデータが存在する */
#endif /* LITES_FTEN */

struct lites_region;
struct lites_buffer;

/**
 * @struct lites_operations
 * @brief リージョン操作用のオペレーション
 * T.B.D: メンバ変更の余地あり
 */
struct lites_operations {
    u32 (*max_log_size)(struct lites_region *reg);
    u32 (*log_size)(struct lites_region *reg, u32 idx);
    u32 (*writable_size)(struct lites_region *reg);
    u32 (*next_index)(struct lites_region *reg, u32 idx);
    s64 (*calc_index_with_header)(struct lites_region *reg, u32 idx, u32 off);
    s64 (*calc_index)(struct lites_region *reg, u32 idx, u32 off);
    s64 (*update_index)(struct lites_region *reg, u32 start, u32 idx, u16 size, u32 flags);
    u32 (*write_header)(struct lites_region *reg, u32 write, u16 lsize);
    u32 (*write_log)(struct lites_region *reg, char *buf, u32 write, u16 lsize);
    int (*read_header)(struct lites_region *reg, char *buf, u32 read);
    int (*read_log)(struct lites_region *reg, char *buf, u32 read, u16 lsize);
};

/**
 * @struct lites_region
 * @brief リージョンを管理する構造体
 */
struct lites_region {
    struct lites_region *next;    /**< 次に登録されたリージョンへのポインタ */
    struct inode *inode;          /**< デバイスノードのiノード */
    u32 psize;                    /**< デバイスノード名の長さ */
    char *path;                   /**< デバイスノード名（ただし逆順）*/
    spinlock_t lock;              /**< 排他制御用のロック */
    struct lites_ring_buffer *rb; /**< ioremapされる領域 */
    u32 total;                    /**< ログとして使用可能な領域 */
    u32 header_size;              /**< ログのヘッダのサイズ */
    struct lites_operations *lop; /**< リージョン操作用オペレーション */
    wait_queue_head_t waitq;      /**< ブロックリード用の待ち行列 */
    unsigned int this_cpu;        /**< 例外処理の再帰バグのチェックフラグ */
};

/** リージョン操作用のAPI（region.cで定義）*/
extern int lites_register_region(struct lites_region *);

extern struct lites_region *lites_find_region_with_inode(struct inode *);
extern struct lites_region *lites_find_region_with_path(char *);
extern struct lites_region *lites_find_region_with_number(unsigned int);
extern struct lites_region *lites_find_region_with_file(struct file *);

#ifdef LITES_FTEN
extern struct lites_region *lites_alloc_region(u64, u32, u32, u32);
#else /* LITES_FTEN */
extern struct lites_region *lites_alloc_region(u64, u32, u32);
#endif 
extern void lites_free_region(struct lites_region *);
extern int lites_region_range(u64, u32);
extern int lites_check_region(u64, u32, u32, u32);
extern int strcmp_file(struct file *, const char *, size_t);
extern u32 parse_syslog_level(char *, size_t);

extern int lites_write_region(struct lites_region *, const char *, u16, u32,
                  u32 (*write_extra_data)(struct lites_region *, u32, void *),
                  void *, size_t);

extern int lites_read_region(struct lites_region *, loff_t, char *, size_t);
extern int lites_ioctl_read_region(struct lites_region *, loff_t, char *, size_t, int);
extern int lites_ioctl_read_sq_region(struct lites_region *, loff_t, struct lites_buffer *, size_t);

/** ネイティブなprintkを利用する出力関数 */
extern int lites_native_printk(const char *, ...);

extern int lites_log_filter;

#ifdef LITES_FTEN
#define lites_pr_err(fmt, args...) \
    do {                                                                    \
        if (lites_log_filter < 1) {                                         \
            lites_native_printk(KERN_ERR "<error> %s(%d): " fmt "\n",       \
                __FUNCTION__, __LINE__, ##args);                            \
        }                                                                   \
    } while (0)
#define lites_pr_info(fmt, args...) \
    do {                                                                    \
        if (lites_log_filter < 2) {                                         \
            lites_native_printk(KERN_INFO "%s: " fmt, __FUNCTION__, ##args);\
        }                                                                   \
    } while (0)
#else /* LITES_FTEN */
#define lites_pr_err(fmt, args...) \
    lites_native_printk(KERN_ERR "<error> %s(%d): " fmt "\n", \
                __FUNCTION__, __LINE__, ##args)
#define lites_pr_info(fmt, args...) \
    lites_native_printk(KERN_INFO "%s: " fmt, __FUNCTION__, ##args)
#endif /* LITES_FTEN */

#ifdef CONFIG_LITES_DEBUG
# define lites_assert(expr)                     \
    do {                                \
        if (!(expr)) {                      \
            lites_native_printk("<fatal error> %s(%d):%s is failed", \
                        __FUNCTION__, __LINE__, #expr); \
            while (1);                  \
        }                           \
    } while (0)
# define lites_pr_dbg(fmt, args...) \
    lites_native_printk(KERN_DEBUG "<debug> %s(%d): " fmt "\n", \
                __FUNCTION__, __LINE__, ##args)
#else /* CONFIG_LITES_DEBUG */
# define lites_assert(expr)         /** nothing to do */
# define lites_pr_dbg(fmt, args...) /** nothing to do */
#endif /* CONFIG_LITES_DEBUG */

/* FUJITSU-TEN 2013-11-20 :LITES AREA DUMP add start */
#if 1
#define LITES_DUMP_DEBUG(fmt, ...)
#else
#define LITES_DUMP_DEBUG(fmt, ...) lites_pr_err(fmt, ##__VA_ARGS__)
#endif

#if 1
#define LITES_LOGFALL_DEBUG(fmt, ...)
#else
#define LITES_LOGFALL_DEBUG(fmt, ...) lites_pr_err(fmt, ##__VA_ARGS__)
#endif

#define LITES_LOGFALL_ERROR(fmt, ...) lites_pr_err(fmt, ##__VA_ARGS__)
#define LITES_DUMP_ERROR(fmt, ...) lites_pr_err(fmt, ##__VA_ARGS__)
/* FUJITSU-TEN 2013-11-20 :LITES AREA DUMP add end   */


/** printkの出力先をLITESに変更するための構造体と変数（printk.cで定義） */
extern struct lites_region *printk_reg;

/** 可変長リージョンと固定長リージョンのオペレーション */
extern struct lites_operations lites_var_lop;
extern struct lites_operations lites_const_lop;

#ifdef LITES_FTEN
/** トレース出力の状態(0:OFF 1:ON(default)) */
extern int lites_trace_out;
#endif /* LITES_FTEN */

#endif /** __LITES_REGION_H */
