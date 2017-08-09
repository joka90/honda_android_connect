/**
 * Copyright (c) 2011 FUJITSU TEN LIMITED and
 * FUJITSU COMPUTER TECHNOLOGIES LIMITED. All rights reserved.
 */
/**
 * @file dev.c
 *
 * @brief ファイルオペレーションの定義、初期化/終了関数の定義
 */
#include <linux/kernel.h>
#include <linux/module.h>
// 2012-10.10 F-TEN add start.
#include <linux/device.h>
#include <linux/miscdevice.h>
// 2012-10.10 F-TEN add end.
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/lites_trace.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "region.h"
#include "ioctl.h"
#include "buffer.h"
#ifdef LITES_FTEN
#include "mgmt_info.h"
#include "trace_write.h"
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/lites_qb.h>
#include "kernel_panic_info.h"
#endif /* LITES_FTEN */

// ADA LOG 2013-0418 ADD
#if 1
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/time.h>
#include <linux/platform_device.h>
#include "../../../arch/arm/mach-tegra/gpio-names.h"
#include "../../../arch/arm/mach-tegra/pm-irq.h"
#include "../../../arch/arm/mach-tegra/board-p1852.h"

extern int lites_fault_info_regist_sh_reset_interrupt ( struct platform_device *pdev );
#endif
// ADA LOG 2013-0418 ADD

/**
 * @brief open処理用関数
 * 　iノードから辿れるファイルのパスと登録してあるリージョンのパスを比較し、
 * 一致した場合にファイル構造体のprivate_dataにリージョン構造体のアドレスを設定
 * する。該当するリージョンがない場合はファイル構造体のprivate_dataにNULLを設定
 * する。
 *
 * @param[in]  inode iノード
 * @param[out] file  ファイル構造体
 *
 * @retval 0 常に正常終了する
 */
static int lites_open(struct inode *inode, struct file *file)
{
	struct lites_region *region;

#ifdef LITES_FTEN
	lites_pr_dbg("lites_open ");
#endif /* LITES_FTEN */
	region = lites_find_region_with_inode(inode); /** T.B.D: iノードはキーとして有効？ */
	if (region == NULL)
		region = lites_find_region_with_file(file);
	file->private_data = (void *)region;

	return 0;
}

/**
 * @brief close処理用関数
 *
 * @retval 0 常に正常終了する
 */
static int lites_close(struct inode *inode, struct file *file)
{
#ifdef LITES_FTEN
	lites_pr_dbg("lites_close ");
#endif /* LITES_FTEN */
	return 0;
}

/**
 * @brief read処理用関数
 * 　ファイル構造体のprivate_dataに設定されたリージョンからログを読み込む。ただ
 * し、タイムスタンプ等のヘッダは読み込まない。
 *
 * @param[in]  file  ファイル構造体
 * @param[out] ubuf  ユーザ空間のバッファ
 * @param[in]  count ユーザ空間のバッファのサイズ
 * @param[out] f_pos 読み込み位置のオフセット
 *
 * @retval 0以上  読み込んだログのサイズ
 * @retval 負の値 読み込みに失敗した
 */
static ssize_t lites_read(struct file *file, char __user *ubuffer, size_t usize, loff_t *pos)
{
	EXTERN_LITES_BUFFER(lites_read);
	struct lites_region *region = file->private_data;
	struct lites_buffer *buffer = lites_buffer(lites_read);
	size_t size;
	ssize_t ret;

	if (unlikely(region == NULL)) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	lites_buffer_lock(buffer);

	size = min_t(size_t, buffer->size, usize);
	ret = lites_read_region(region, *pos, buffer->buffer, size);
	if (unlikely(ret < 0)) {
		lites_pr_err("lites_read_region");
		goto unlock;
	}

	if (unlikely(copy_to_user(ubuffer, buffer->buffer, ret))) {
		lites_pr_err("copy_to_user");
		ret = -EFAULT;
		goto unlock;
	}

	*pos += ret;
unlock:
	lites_buffer_unlock(buffer);
	return ret;
}

#ifdef LITES_FTEN
/**
 * @brief ログトレース用ヘッダ書き込み関数
 *
 * @param[in]  reg   lites_region構造体
 * @param[in]  write 書き込み先のアドレス
 * @param[in]  data  書き込みデータのアドレス
 *
 * @retval 0以上  書き込んだヘッダのサイズ
 */
static u32 write_dev_trace_header(struct lites_region *reg, u32 write, void *data)
{
	size_t size = sizeof(struct lites_log_header); /** boot_id, seq_no, level */
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
#endif /* LITES_FTEN */

/**
 * @brief write処理用関数
 * 　ファイル構造体のprivate_dataに設定されたリージョンへログを書き込む。
 *
 * @param[in]  file   ファイル構造体
 * @param[in]  ubuf   ユーザ空間のバッファ
 * @param[in]  count  ユーザ空間のバッファのサイズ
 * @param[in]  unused 未使用
 *
 * @retval 0以上  書き込んだログのサイズ
 * @retval 負の値 書き込みに失敗した
 */
static ssize_t lites_write(struct file *file, const char __user *ubuffer, size_t usize, loff_t *unused)
{
	EXTERN_LITES_BUFFER(lites_write);
	struct lites_region *region = file->private_data;
	struct lites_buffer *buffer = lites_buffer(lites_write);
	u32 level;
	size_t size = min_t(size_t, buffer->size, usize);
	ssize_t ret;
#ifdef LITES_FTEN
	struct lites_log_header winfo;
	memset(&winfo, 0, sizeof(winfo));
#endif /* LITES_FTEN */

	if (unlikely(region == NULL)) {
		lites_pr_err("not initialized region");
		return -EPERM;
	}

	lites_buffer_lock(buffer);

	if (unlikely(copy_from_user(buffer->buffer, ubuffer, size))) {
		lites_pr_err("copy_from_user");
		ret = -EFAULT;
		goto unlock;
	}

	level = parse_syslog_level(buffer->buffer, size);
#ifdef LITES_FTEN
	/* ログレコードの固定ヘッダ(lites_log_header構造体)も付加して、ログレコードを書き込む */
	ret = lites_write_region(region, buffer->buffer, size, level, write_dev_trace_header, &winfo, sizeof(winfo));
	if (unlikely(ret < 0)) {
		lites_pr_err("lites_write_region (ret = %d)", ret);
	} else {
		// 書き込んだログは、上位が指定したログの他に、
		// 先頭にlites_log_header構造体のサイズ分だけ付加している。
		// 上位では、指定したログのサイズ分だけ書き込まれてるか？チェックしているのもあるので、
		// 付加したlites_log_header構造体のサイズ分だけ書き込みサイズから引いておく。
		// syslog-ngはこれを実施しておかないと、ログが記録されなくなる。
		ret -= sizeof(struct lites_log_header);
	}
#else /* LITES_FTEN */
	ret = lites_write_region(region, buffer->buffer, size, level, NULL, NULL, 0);
	if (unlikely(ret < 0))
		lites_pr_err("lites_write_region (ret = %d)", ret);
#endif /* LITES_FTEN */


unlock:
	lites_buffer_unlock(buffer);
	return ret;
}

/**
 * @brief ioctl処理用関数
 * 　cmdに設定されたコマンドから各種処理を実行する。コマンドの実体はioctl.cに定
 * 義されている。
 *
 * @param[in]  file   ファイル構造体
 * @param[in]  cmd    コマンド
 * @param[in]  addr   コマンドの引数を格納した領域のアドレス
 *
 * @retval -ENOIOCTLCMD 非対応のコマンドが指定された
 * @retval その他       各種コマンドの処理の戻り値
 */
static long lites_ioctl(struct file *file, unsigned int cmd, unsigned long addr)
{
	u32 dir, type, nr, size;

	dir = _IOC_DIR(cmd);
	type = _IOC_TYPE(cmd);
	size = _IOC_SIZE(cmd);

#ifdef LITES_FTEN
	lites_pr_dbg("lites_ioctl cmd=%u dir=%u type=%u size=%u", cmd, dir, type, size);
#endif /* LITES_FTEN */

	if (unlikely(dir != LITES_IOC_DIR || type != LITES_IOC_TYPE || size != LITES_IOC_SIZE)) {
		lites_pr_err("invalid command (cmd = %u)", cmd);
		return -ENOIOCTLCMD;
	}

	nr = _IOC_NR(cmd);
	if (unlikely(nr >= lites_ioctls_nr)) {
		lites_pr_err("invalid command (cmd = %u)", cmd);
		return -ENOIOCTLCMD;
	}

	return lites_ioctls[nr].ioctl(file, addr);
}

#ifdef LITES_FTEN
/**
 * @brief poll処理用関数
 * 　退避（読み出し）可能なリージョンが出来るまで、待ち合わせ処理を実行する
 *
 * @param[in]  file   ファイル構造体
 * @param[in]  ptable poll table
 *
 * @retval poll mask
 */
static unsigned int lites_poll(struct file *file, poll_table *ptable)
{
	unsigned long mask = 0;
	int ret;
	u32 full_region_no_num = 0;

	// 当関数内でsleep禁止
	if (unlikely(lites_trace_init_done == 0)) {
		lites_pr_err("lites_trace_init is not completed");
		return POLLERR;
	}

	// wait_queueの登録
	lites_pr_dbg("lites_poll poll_wait");
	poll_wait(file, &lites_trace_region_wait_queue, ptable);

	// 読み出し可能なリージョンがあるかどうかのチェック
	ret = lites_trace_get_readable_region_list(&full_region_no_num, NULL);
	if (unlikely(ret < 0)) {
		lites_pr_err("lites_trace_exist_read_region error (ret=%d)", ret);
		return POLLERR;
	} else if (ret == 1) {
		// 読み出し可能なリージョンがあるケース
		mask |= POLLIN | POLLRDNORM;	// 読み込み可
	}

	lites_pr_dbg("lites_poll mask=%08x", mask);
    return mask;
}
#endif /* LITES_FTEN */

static struct file_operations lites_fops = {
	.owner          = THIS_MODULE,
	.open           = lites_open,
	.release        = lites_close,
	.read           = lites_read,
	.write          = lites_write,
	.unlocked_ioctl = lites_ioctl,
#ifdef LITES_FTEN
	.poll           = lites_poll
#endif /* LITES_FTEN */
};

/** FUJITSU TEN:2012-10-10 add start. */
static struct class *lites_class;	/* lites class during class_create */
static struct device *lites_dev;	/* lites dev during device_create */
/** FUJITSU TEN:2012-10-10 add end. */

// ADA LOG 2013-0418 ADD
#if 1
static struct platform_driver shResetIntr_device_driver = {
    .probe = lites_fault_info_regist_sh_reset_interrupt,
    .driver = {
        .name = "shrstintr_dev",
        .owner = THIS_MODULE,
    },
};
#endif
// ADA LOG 2013-0418 ADD

/**
 * @brief LITES初期化関数
 * 　ユーザ空間のインターフェースとして、char型デバイスを登録する。
 *
 * @retval 0    正常終了
 * @retval -EIO char型デバイスの登録に失敗した
 */
static __init int lites_init(void)
{
	int ret;

	ret = lites_init_buffer();
	if (ret < 0) {
		lites_pr_err("lites_init_buffer (ret = %d)", ret);
		return ret;
	}

/** FUJITSU TEN:2012-10-10 mod start. */
//	ret = register_chrdev(LITES_MAJOR, LITES_NAME, &lites_fops);
	ret = register_chrdev(MISC_DYNAMIC_MINOR, LITES_NAME, &lites_fops);
/** FUJITSU TEN:2012-10-10 mod end. */
	if (ret < 0) {
		lites_pr_err("register_chrdevr (ret = %d)");
		goto err;
	}

/** FUJITSU TEN:2012-10-10 add start. */
	/*  udev */
	lites_class = class_create(THIS_MODULE, LITES_NAME);
	if (IS_ERR(lites_class)) {
		lites_pr_err(" Something went wrong in class_create");
		unregister_chrdev(MISC_DYNAMIC_MINOR, LITES_NAME);
		ret = PTR_ERR(lites_class);
		goto err;
	}

	lites_dev =
		device_create(lites_class, NULL, MKDEV(MISC_DYNAMIC_MINOR, 0),
				NULL, LITES_NAME);

	if (IS_ERR(lites_dev)) {
		lites_pr_err(" Error in class_create");
		class_destroy(lites_class);
		unregister_chrdev(MISC_DYNAMIC_MINOR, LITES_NAME);
		ret = PTR_ERR(lites_dev);
		goto err;
	}
/** FUJITSU TEN:2012-10-10 add end. */

// ADA LOG 2013-0418 ADD
#if 1
        platform_driver_register ( &shResetIntr_device_driver );

printk ( KERN_INFO "platform_driver_register end (regist lites_fault_info_regist_sh_reset_interrupt()" );
#endif
// ADA LOG 2013-0418 ADD

	return 0;

err:
	lites_exit_buffer();
	return ret;
}
module_init(lites_init);

/**
 * @brief LITES終了処理
 * 　char型デバイスを抹消する。
 */
static __exit void lites_exit(void)
{
/** FUJITSU TEN:2012-10-10 remove start. */
//	unregister_chrdev(LITES_MAJOR, LITES_NAME);
/** FUJITSU TEN:2012-10-10 remove start. */
/** FUJITSU TEN:2012-10-10 add start. */
	device_destroy(lites_class, MKDEV(MISC_DYNAMIC_MINOR, 0));
	class_destroy(lites_class);
	unregister_chrdev(MISC_DYNAMIC_MINOR, LITES_NAME);
/** FUJITSU TEN:2012-10-10 add end. */
	lites_exit_buffer();
}
module_exit(lites_exit);


/**
 * @brief suspend処理関数
 *        shapshotdriverのハードレジスタ退避の最後で呼ばれる想定
 *        suspendはQuickBoot用のsnapshotイメージ作成時の1回のみしか呼ばれない
 *
 * @retval 0  正常終了
 */
int lites_suspend(void)
{
	// 管理情報の初期化が済んでない場合、終了させる。
	if (unlikely(mgmt_info_init_done == 0)) {
		lites_pr_info("LITES(mgmt_info) is not initialized.");
		return 0;
	}

	lites_mgmt_info_backup();
	return 0;
}
EXPORT_SYMBOL(lites_suspend);

/**
 * @brief resume処理関数
 *        shapshotdriverのハードレジスタ復元の最初で呼ばれる想定
 *        resumeはQuickBootで起動する度に呼ばれる
 *
 * @retval 0  正常終了
 */
int lites_resume(void)
{
	int i;

	// 管理情報の初期化処理が済んでない場合、終了させる。
	if (unlikely(mgmt_info_init_done == 0)) {
		lites_pr_info("LITES(mgmt_info) is not initialized.");
		return 0;
	}
	// KernelPanic情報退避領域の初期化処理が済んでない場合、終了させる。
	if (unlikely(kernel_panic_info_init_done == 0)) {
		lites_pr_info("RAS(ubinux_kernel_panic_info) is not initialized.");
		return 0;
	}

	// 管理情報とKernelPanic情報退避領域のチェック
	// 壊れていた場合は、どちらも初期化と復元
	if (unlikely((lites_mgmt_info_check_magic_no() < 0) ||
		         (ubinux_kernel_panic_info_check_magic_no() < 0))) {
		// KernelPanic情報退避領域の初期化
		ubinux_kernel_panic_info_initialize();
		lites_pr_info("RAS(ubinux_kernel_panic_info) was broken, reinitialized.");

		// 管理情報を復元
		lites_mgmt_info_restore();

		// 各リージョンを初期化
		lites_mgmt_info_initialize_region(LITES_PRINTK_REGION);
		lites_mgmt_info_initialize_region(LITES_SYSLOG_REGION);
		for (i = 0; i < (int)lites_mgmt_info_get_trace_region_num(); ++i) {
			lites_mgmt_info_initialize_region(i);
		}

		lites_pr_info("LITES(mgmt_info) was broken, reinitialized.");
	}

	// boot_idをカウントアップ
	lites_mgmt_info_update_boot_id();

	return 0;
}
EXPORT_SYMBOL(lites_resume);
