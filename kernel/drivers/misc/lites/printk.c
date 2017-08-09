/**
 * Copyright (c) 2011 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
/**
 * @file printk.c
 *
 * @brief LITES用printkの定義
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/lites_trace.h>
#include "region.h"
#include "parse.h"
#ifdef LITES_FTEN
#include "mgmt_info.h"
#endif /* LITES_FTEN */

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

#define LITES_FTEN_ADA
#define ADA_PREFIX							"ADA_"

asmlinkage int lites_vprintk(const char *fmt, va_list args);

/** printk出力先を変更するための変数 */
struct vprintk_caller_t vprintk_caller[] = { {&vprintk}, {&lites_vprintk} };
struct lites_region *printk_region;

/** printk.c等から参照 */
int vprintk_caller_index = 0;

#ifndef LITES_FTEN_MM_DIRECO
static u64 printk_start;
static u32 printk_size, printk_level, printk_access_byte;
static int printk_parse_done = 0;
#endif	// LITES_FTEN_MM_DIRECO

/** static u32 printk_console = 0; 13Cy's default */
static u32 printk_console = 1;


#ifdef LITES_FTEN
#ifndef LITES_FTEN_ADA
/**
 * @brief printkトレース用ヘッダ書き込み関数
 *
 * @param[in]  reg   lites_region構造体
 * @param[in]  write 書き込み先のアドレス
 * @param[in]  data  書き込みデータのアドレス
 *
 * @retval 0以上  書き込んだヘッダのサイズ
 */
static u32 write_printk_header(struct lites_region *reg, u32 write, void *data)
{
	size_t size = sizeof(struct lites_log_header); /** boot_id, seq_no,level */
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
#endif /* LITES_FTEN_ADA */
#endif /* LITES_FTEN */

/**
 * @brief LITES用vprintk関数
 * 　printkから呼ばれる。
 *
 * @param[in]  fmt  printkフォーマット
 * @param[in]  args printkの可変長引数
 *
 * @retval 0以上 書き込んだ文字列の長さ
 */
asmlinkage int lites_vprintk(const char *fmt, va_list args)
{
	static DEFINE_SPINLOCK(lites_vprintk_lock);
	static volatile unsigned int printk_cpu = UINT_MAX;
	static char lites_printk_buffer[LITES_ACCESS_BYTE_MAX];
	u32 level;
	unsigned long flags = 0;
	ssize_t size = 0;
	int this_cpu;
	char *buffer = lites_printk_buffer; /** T.B.D: 要SMP化 */
/* FUJITSU TEN:2012-11-16 ADA_LOG add start */
	struct lites_trace_header trc_header;
	struct lites_trace_format trc_format;
	int group_id = 0;
/* FUJITSU TEN:2012-11-16 ADA_LOG add end */
#ifdef LITES_FTEN
	struct lites_log_header pinfo;
	memset(&trc_header, 0, sizeof(trc_header));
	memset(&pinfo, 0, sizeof(pinfo));

	if (unlikely(printk_console == 1)) {
		// コンソール出力モード
		call_native_vprintk(fmt, args);
	}

	if (unlikely(printk_region == NULL)) {
		lites_pr_err("not initialized printk_region");
		return -EPERM;
	}
#endif /* LITES_FTEN */

	preempt_disable();
	raw_local_irq_save(flags);
	this_cpu = smp_processor_id();

	/**
	 * スピンロック獲得中に、例外が発生した場合の対策。例外が発生した場合は
	 * ネイティブのprintk関数を呼び出す。例外処理は複数回lites_vprintk関数を
	 * 呼び出すことに留意すること。
	 */
	if (this_cpu == printk_cpu) {
		lites_pr_err("Recursion bug (cpuid = %u)", this_cpu);
		/** ロック獲得元へ戻るかを判定 */
		if (!oops_in_progress)
			goto ret;
		/** ロック獲得元へ戻らない（do_exitで終了される）のでロック解放 */
		spin_lock_init(&lites_vprintk_lock);
	}

	lockdep_off();
	spin_lock(&lites_vprintk_lock);
	printk_cpu = this_cpu;

	size = vscnprintf(buffer, sizeof(lites_printk_buffer), fmt, args);
	level = parse_syslog_level(buffer, size);
	lites_pr_dbg("size = %08x, level = %08x, buffer = %s", size, level, buffer);
#ifdef LITES_FTEN
/* FUJITSU TEN:2012-11-16 ADA_LOG mod start */
#ifdef LITES_FTEN_ADA
	trc_header.level = level;
	trc_format.trc_header = &trc_header;
	trc_format.count = size;
	trc_format.buf = buffer;

	/* printkリージョンにログレコードを書き込む */
	group_id = REGION_PRINTK;
	size = lites_trace_write( group_id, &trc_format);

	/* ADA車載用プレフィックスの抽出 */
	if ( memcmp(&buffer[4], ADA_PREFIX, strlen(ADA_PREFIX)) == 0) {
		group_id = REGION_ADA_PRINTK;
		/* ADA用printkリージョンにログレコードを書き込む */
		size = lites_trace_write( group_id, &trc_format);
	}

	if (trc_header.level <= 2 ) {
	/* KERN_EMERG = 0, KERN_ALERT   = 1, KERN_CRIT   = 2    : ハードエラー対象 */
	/* KERN_ERR   = 3, KERN_WARNING = 4, KERN_NOTICE = 5... : ハードエラー対象外 */
		/* ハードエラーリージョンにログレコードを書き込む */
		group_id = REGION_TRACE_HARD_ERROR;
		size = lites_trace_write( group_id, &trc_format);
	}
#else /* LITES_FTEN_ADA */
	/* ログレコードの固定ヘッダ(lites_log_header構造体)も付加して、ログレコードを書き込む */
	size = lites_write_region(printk_region, buffer, size, level, write_printk_header, &pinfo, sizeof(pinfo));
#endif /* LITES_FTEN_ADA */
/* FUJITSU TEN:2012-11-16 ADA_LOG mod end */
#else /* LITES_FTEN */
	size = lites_write_region(printk_region, buffer, size, level, NULL, NULL, 0);
#endif /* LITES_FTEN */

	printk_cpu = UINT_MAX;
	spin_unlock(&lites_vprintk_lock);
	lockdep_on();

ret:
	raw_local_irq_restore(flags);
	preempt_enable();
	return size;
}

/**
 * @brief ネイティブなprintk出力用関数
 *
 * @param[in]  fmt  printkフォーマット
 * @param[in]  args printkの可変長引数
 */
int lites_native_printk(const char *fmt, ...)
{
	int ret;

	va_list args;
	va_start(args, fmt);
	ret = call_native_vprintk(fmt, args);
	va_end(args);

	return ret;
}
EXPORT_SYMBOL(lites_native_printk);

#ifdef LITES_FTEN_MM_DIRECO
/**
 * @brief LITES用printkの初期化用関数
 * 　early_lites_printk関数で解析したパラメータを元にprintk用リージョンを登録す
 * る。登録が成功した場合、それ以降のprintkの出力はLITESで管理される。
 *
 * @retval 0       正常終了（または管理情報の初期化処理が完了してない）
 * @retval -ENOMEM リージョンの獲得に失敗した
 * @retval 負の値  リージョンの登録に失敗した
 */
static __init int lites_printk_init(void)
{
	static const char printk_path[] = LITES_SYSLOG_PATH;
	static const int printk_psize = ARRAY_SIZE(printk_path) - 1;
	int ret;
	int printk_continue;
	u64 printk_addr64;
	u32 printk_size, printk_level, printk_access_byte, printk_wrap;

	// 管理情報の初期化処理（確保）が出来てないので、高速ログへの出力は実施しない
	if (unlikely(mgmt_info_init_done == 0)) {
		lites_pr_info("printk output is not changed to LITES. mgmt_info is not initialized.");
		return 0;
	}

	// 定義からprintkリージョンの設定を読み出す
	printk_addr64		= LITES_PRINTK_REGION_ADDR;
	printk_size			= LITES_PRINTK_REGION_SIZE;
	printk_level		= LITES_PRINTK_REGION_LEVEL;
	printk_access_byte	= LITES_PRINTK_REGION_ACCESS_BYTE;
	printk_wrap			= LITES_PRINTK_REGION_WRAP;

	// 管理情報内をチェックし、リージョンの継続が可能かどうかチェックする
	printk_continue = lites_mgmt_info_check(LITES_PRINTK_REGION,
											printk_addr64 >> 32, printk_addr64,
											printk_size, printk_level,
											printk_access_byte, printk_wrap);
	if (unlikely(printk_continue < 0)) {
		return printk_continue;
	} else if (printk_continue == 1) {
		lites_pr_info("LITES(printk) is continued. addr=%llx", printk_addr64);
	} else {
		lites_pr_info("LITES(printk) is restarted. addr=%llx", printk_addr64);
	}

	ret = lites_check_region(printk_addr64, printk_size,
				 printk_level, printk_access_byte);
	if (ret < 0) {
		lites_pr_err("lites_check_region (ret = %d)", ret);
		return -EINVAL;
	}

	/* トレース初期化 or トレース継続 */
	printk_region = lites_alloc_region(printk_addr64, printk_size, printk_psize, printk_continue);
	if (unlikely(printk_region == NULL)) {
		lites_pr_err("lites_alloc_region");
		return -ENOMEM;
	}

	printk_region->rb->addr        = printk_addr64;
	printk_region->rb->size        = printk_size;
	printk_region->rb->region      = LITES_PRINTK_REGION;
	printk_region->rb->level       = printk_level;
	printk_region->rb->access_byte = printk_access_byte;
	printk_region->rb->wrap        = printk_wrap;
	printk_region->psize           = printk_psize;
	printk_region->total           = printk_size - offsetof(struct lites_ring_buffer, log);
	printk_region->header_size     = LITES_COMMON_LOG_HEADER_STRUCT_SIZE;
	init_region_lock(printk_region);
	strncpy(printk_region->path, printk_path, printk_psize);

	/** When access_byte is 0, lites_alloc_region is failed */
	if (printk_region->rb->access_byte != LITES_VAR_ACCESS_BYTE) {
		printk_region->total -= printk_region->total % printk_region->rb->access_byte;
		printk_region->lop    = &lites_const_lop;
	} else {
		printk_region->lop = &lites_var_lop;
	}

	ret = lites_register_region(printk_region);
	if (unlikely(ret < 0)) {
		lites_pr_err("lites_register_region (ret = %d)", ret);
		lites_free_region(printk_region);
		return ret;
	}

	lites_pr_info("printk output is changed to LITES\n");
	if (unlikely(printk_console == 1)) {
		lites_pr_info("LITES(printk) is console mode");
	}
	vprintk_caller_index = 1;

	return 0;
}
pure_initcall(lites_printk_init);

#else	// LITES_FTEN_MM_DIRECO
/**
 * @brief LITES用printkの初期化用関数
 * 　early_lites_printk関数で解析したパラメータを元にprintk用リージョンを登録す
 * る。登録が成功した場合、それ以降のprintkの出力はLITESで管理される。
 *
 * @retval 0       正常終了（またはカーネル起動パラメータの解析が完了していない、
 *                 または管理情報の初期化処理が完了してない）
 * @retval -ENOMEM リージョンの獲得に失敗した
 * @retval 負の値  リージョンの登録に失敗した
 */
static __init int lites_printk_init(void)
{
	static const char printk_path[] = LITES_SYSLOG_PATH;
	static const int printk_psize = ARRAY_SIZE(printk_path) - 1;
	int ret;
	int printk_continue;

	if (unlikely(!printk_parse_done)) {
		lites_pr_info("printk output is not changed to LITES\n");
		return 0;
	}

	// 管理情報の初期化処理（確保）が出来てないので、高速ログへの出力は実施しない
	if (unlikely(mgmt_info_init_done == 0)) {
		lites_pr_info("printk output is not changed to LITES. mgmt_info is not initialized.");
		return 0;
	}

	// 管理情報内をチェックし、リージョンの継続が可能かどうかチェックする
	printk_continue = lites_mgmt_info_check(LITES_PRINTK_REGION,
											printk_start >> 32, printk_start,
											printk_size, printk_level,
											printk_access_byte, LITES_WRAP_OFF);	// 上書き可能
	if (unlikely(printk_continue < 0)) {
		return printk_continue;
	} else if (printk_continue == 1) {
		lites_pr_info("LITES(printk) is continued");
	} else {
		lites_pr_info("LITES(printk) is restarted");
	}

	ret = lites_check_region(printk_start, printk_size,
				 printk_level, printk_access_byte);
	if (ret < 0) {
		lites_pr_err("lites_check_region (ret = %d)", ret);
		return -EINVAL;
	}

	/* トレース初期化 or トレース継続 */
	printk_region = lites_alloc_region(printk_start, printk_size, printk_psize, printk_continue);
	if (unlikely(printk_region == NULL)) {
		lites_pr_err("lites_alloc_region");
		return -ENOMEM;
	}

	printk_region->rb->addr        = printk_start;
	printk_region->rb->size        = printk_size;
	printk_region->rb->region      = LITES_PRINTK_REGION;
	printk_region->rb->level       = printk_level;
	printk_region->rb->access_byte = printk_access_byte;
	printk_region->psize           = printk_psize;
	printk_region->total           = printk_size - offsetof(struct lites_ring_buffer, log);
	printk_region->header_size     = LITES_COMMON_LOG_HEADER_STRUCT_SIZE;
	init_region_lock(printk_region);
	strncpy(printk_region->path, printk_path, printk_psize);

	/** When access_byte is 0, lites_alloc_region is failed */
	if (printk_region->rb->access_byte != LITES_VAR_ACCESS_BYTE) {
		printk_region->total -= printk_region->total % printk_region->rb->access_byte;
		printk_region->lop    = &lites_const_lop;
	} else {
		printk_region->lop = &lites_var_lop;
	}

	ret = lites_register_region(printk_region);
	if (unlikely(ret < 0)) {
		lites_pr_err("lites_register_region (ret = %d)", ret);
		goto err;
	}

	lites_pr_info("printk output is changed to LITES\n");
	if (unlikely(printk_console == 1)) {
		lites_pr_info("LITES(printk) is console mode");
	}
	vprintk_caller_index = 1;

	return 0;

err:
	lites_free_region(printk_region);
	return ret;
}
pure_initcall(lites_printk_init);

/**
 * @brief LITES用printkのカーネル起動パラメータ解析用関数
 * 　カーネル起動パラメータに含まれる"lites_printk="に続く文字列を解析する。
 * lites_alloc_region関数が動作可能になるのはしばらく後なので、パラメータ解析以
 * 降の処理はlites_printk_initで実行する。

 * @param[in]  arg "lites_printk="に続く文字列
 *
 * @retval 0  解析に成功した
 * @retval -1 解析に失敗した
 */
static __init int early_lites_printk(char *arg)
{
	int ret;

#ifdef LITES_FTEN
	printk_parse_done = 0;
	vprintk_caller_index = 0;
#endif /* LITES_FTEN */

	ret = parse_addr(&arg, &printk_start);
	if (ret < 0) {
		lites_pr_err("parse_addr (arg = %s, ret = %d)", arg, ret);
		return ret;
	}

	ret = parse_size(&arg, &printk_size);
	if (ret < 0) {
		lites_pr_err("parse_size (arg = %s, ret = %d)", arg, ret);
		return ret;
	}

	ret = parse_level(&arg, &printk_level);
	if (ret < 0) {
		lites_pr_err("parse_level (arg = %s, ret = %d)", arg, ret);
		return ret;
	}

	ret = parse_size(&arg, &printk_access_byte);
	if (ret < 0) {
		lites_pr_err("parse_size (arg = %s, ret = %d)", arg, ret);
		return ret;
	}

	/** 余分な文字がある場合も解析を失敗したことにする */
	if (!is_delimiter(*arg)) {
		lites_pr_err("is_delimiter (arg = %s)", arg);
		return -EINVAL;
	}

	lites_pr_dbg("printk_start = %llx, printk_size = %x, "
		     "printk_level = %x, printk_access_byte = %x",
		     printk_start, printk_size, printk_level, printk_access_byte);

	printk_parse_done = 1;
	return 0;
}
early_param("lites_printk", early_lites_printk);
#endif	// LITES_FTEN_MM_DIRECO


/**
 * @brief LITES用printkコンソール出力モードのカーネル起動パラメータ解析用関数
 * 　カーネル起動パラメータに含まれる"lites_printk_console="に続く文字列を解析する。

 * @param[in]  arg "lites_printk_console="に続く文字列
 *
 * @retval 0  解析に成功した
 * @retval -1 解析に失敗した
 */
static __init int early_lites_printk_console(char *arg)
{
	int ret;

	/* printk_console = 0; FUJITSU TEN:2012-11-12 mod adalog **/
	printk_console = 1;

	ret = parse_level(&arg, &printk_console);
	if (ret < 0) {
		lites_pr_err("parse (arg = %s, ret = %d)", arg, ret);
		return ret;
	}

	/** 余分な文字がある場合も解析を失敗したことにする */
	if (!is_delimiter(*arg)) {
		lites_pr_err("is_delimiter (arg = %s)", arg);
		/* printk_console = 0; FUJITSU TEN:2012-11-12 mod adalog **/
		printk_console = 1;
		return -EINVAL;
	}

	lites_pr_dbg("printk_console = %x", printk_console);
	return 0;
}
early_param("lites_printk_console", early_lites_printk_console);
