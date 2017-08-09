/*
 * drivers/mtd/devices/tegra_nand.c
 *
 * Copyright (C) 2010 Google, Inc.
 * Author: Dima Zavin <dima@android.com>
 *         Colin Cross <ccross@android.com>
 *
 * Copyright (c) 2010-2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Derived from: drivers/mtd/nand/nand_base.c
 *               drivers/mtd/nand/pxa3xx.c
 *
 */

#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/gpio.h>

#include <mach/nand.h>

#include "tegra_nand.h"
#include <asm/cacheflush.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include "linux/delay.h"

#define DRIVER_NAME	"tegra_nand"
#define DRIVER_DESC	"Nvidia Tegra NAND Flash Controller driver"

#define MAX_DMA_SZ			SZ_64K
#define ECC_BUF_SZ			SZ_1K
#define MAX_CQ_SIZE			65
#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
/*BUFFER_NUM is number of mtd->writesize bytes buffer */
#define BUFFER_NUM			4
#endif

/*#define TEGRA_NAND_DEBUG
#define TEGRA_NAND_DEBUG_PEDANTIC*/

#ifdef TEGRA_NAND_DEBUG
#define TEGRA_DBG(fmt, args...) \
	do { pr_info(fmt, ##args); } while (0)
#else
#define TEGRA_DBG(fmt, args...)
#endif

/* TODO: will vary with devices, move into appropriate device spcific header */
#define SCAN_TIMING_VAL		0x3f0bd214
#define SCAN_TIMING2_VAL	0xb
#define LOGICAL_PAGES_COMBINE   8

#define TIMEOUT (2 * HZ)

static const char *part_probes[] = { "cmdlinepart", NULL, };
#ifdef CONFIG_MTD_NAND_TEGRA_CQ_MODE
struct cq_packet {
	uint32_t command;
	uint32_t data[CQ_MAX_REG];
};

struct cq_reg_val {
	uint32_t dmactrl_reg;
	uint32_t cmd_reg;
	uint32_t dma_addr_reg;
	uint32_t addr1;
	uint32_t addr2;
	uint32_t cmd1;
	uint32_t cmd2;
};
#endif

struct tegra_nand_chip {
	spinlock_t lock;
	uint32_t chipsize;
	int num_chips;
	int curr_chip;

	/* addr >> chip_shift == chip number */
	uint32_t chip_shift;
	/* (addr >> page_shift) & page_mask == page number within chip */
	uint32_t page_shift;
	uint32_t page_mask;
	/* column within page */
	uint32_t column_mask;
	/* addr >> block_shift == block number (across the whole mtd dev, not
	 * just a single chip. */
	uint32_t block_shift;

	void *priv;
};

#ifdef CONFIG_MTD_NAND_TEGRA_RW_STATS
struct tegra_nand_io_stats {
	uint64_t			erase_stats;
	uint64_t			read_req_bytes;
	uint64_t			oob_req_bytes;
	uint64_t			data_read_pages;
	uint64_t			oob_read_bytes;
	uint64_t			write_req_bytes;
	uint64_t			oobw_req_bytes;
	uint64_t			written_pages;
	uint64_t			oob_written_bytes;
	uint32_t			read_req_l2k;
	uint32_t			read_req_l4k;
	uint32_t			read_req_l8k;
	uint32_t			read_req_l16k;
	uint32_t			read_req_l32k;
	uint32_t			read_req_l64k;
	uint32_t			read_req_l128k;
	uint32_t			read_req_g128k;
};
#endif

struct tegra_nand_info {
	struct tegra_nand_chip chip;
	struct mtd_info mtd;
	struct tegra_nand_platform *plat;
	struct device *dev;
	struct mtd_partition *parts;

	/* synchronizes access to accessing the actual NAND controller */
	struct mutex lock;
	/* partial_unaligned_rw_buffer is temporary buffer used during
	   reading of unaligned data from nand pages or if data to be read
	   is less than nand page size.
	 */
	uint8_t	*partial_unaligned_rw_buffer;
#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
	long long *old_addresses;
	/* buffer_frequency indicates the last time it was accessed */
	int *buffer_frequency;
#endif
	void *oob_dma_buf;
	dma_addr_t oob_dma_addr;
#ifdef CONFIG_MTD_NAND_TEGRA_PAGE_CACHE
	void *dma_buf;
	dma_addr_t dma_addr;
#endif
	/* ecc error vector info (offset into page and data mask to apply */
	void *ecc_buf;
	dma_addr_t ecc_addr;
	/* ecc error status (page number, err_cnt) */
	uint32_t *ecc_errs;
	uint32_t num_ecc_errs;
	uint32_t max_ecc_errs;
	spinlock_t ecc_lock;

	uint32_t command_reg;
	uint32_t config_reg;
	uint32_t dmactrl_reg;

	struct completion cmd_complete;
	struct completion dma_complete;

	/* bad block bitmap: 1 == good, 0 == bad/unknown */
	unsigned long *bb_bitmap;

	struct clk *clk;
	uint32_t is_data_bus_width_16;
	uint32_t device_id;
	uint32_t vendor_id;
	uint32_t dev_parms;
	uint32_t num_bad_blocks;
#ifdef CONFIG_MTD_NAND_TEGRA_RW_STATS
	struct tegra_nand_io_stats rwe_stats;
#endif
#ifdef CONFIG_MTD_NAND_TEGRA_CQ_MODE
	/* command queue implementation */
	uint32_t ll_config_reg;
	struct completion cq_complete;
	uint8_t	 command_queue_en;
	struct cq_packet cq[MAX_CQ_SIZE];
	uint8_t  cq_current_packet;
	struct cq_reg_val cq_regs;
#endif
};

static void
set_chip_timing(struct tegra_nand_info *info, uint32_t vendor_id,
		uint32_t dev_id, uint32_t fourth_id_field);

#define MTD_TO_INFO(mtd)	container_of((mtd), struct tegra_nand_info, mtd)

/* 64 byte oob block info for large page (== 2KB) device
 *
 * OOB flash layout for Tegra with Reed-Solomon 4 symbol correct ECC:
 *      Skipped bytes(4)
 *      Main area Ecc(36)
 *      Tag data(20)
 *      Tag data Ecc(4)
 *
 * Yaffs2 will use 16 tag bytes.
 */

static struct nand_ecclayout tegra_nand_oob_64 = {
	.eccbytes = 36,
	.eccpos = {
		   4, 5, 6, 7, 8, 9, 10, 11, 12,
		   13, 14, 15, 16, 17, 18, 19, 20, 21,
		   22, 23, 24, 25, 26, 27, 28, 29, 30,
		   31, 32, 33, 34, 35, 36, 37, 38, 39,
		   },
	.oobavail = 20,
	.oobfree = {
		    {.offset = 40,
		     .length = 20,
		     },
		    },
};

static struct nand_ecclayout tegra_nand_oob_128 = {
	.eccbytes = 72,
	.eccpos = {
		   4, 5, 6, 7, 8, 9, 10, 11, 12,
		   13, 14, 15, 16, 17, 18, 19, 20, 21,
		   22, 23, 24, 25, 26, 27, 28, 29, 30,
		   31, 32, 33, 34, 35, 36, 37, 38, 39,
		   40, 41, 42, 43, 44, 45, 46, 47, 48,
		   49, 50, 51, 52, 53, 54, 55, 56, 57,
		   58, 59, 60, 61, 62, 63, 64, 65, 66,
		   /* ECC POS is only of size 64 bytes  so commenting the
		    * remaining bytes here. As driver uses the Hardware
		    * ECC so it there is no issue with it
		    */
		   /*67, 68, 69, 70, 71, 72, 73, 74, 75, */
		   },
	.oobavail = 48,
	.oobfree = {
		    {.offset = 76,
		     .length = 48,
		     },
		    },
};

static struct nand_flash_dev *find_nand_flash_device(int dev_id)
{
	struct nand_flash_dev *dev = &nand_flash_ids[0];

	while (dev->name && dev->id != dev_id)
		dev++;
	return dev->name ? dev : NULL;
}

static struct nand_manufacturers *find_nand_flash_vendor(int vendor_id)
{
	struct nand_manufacturers *vendor = &nand_manuf_ids[0];

	while (vendor->id && vendor->id != vendor_id)
		vendor++;
	return vendor->id ? vendor : NULL;
}

#define REG_NAME(name)			{ name, #name }
static struct {
	uint32_t addr;
	char *name;
} reg_names[] = {
	REG_NAME(COMMAND_REG),
	REG_NAME(STATUS_REG),
	REG_NAME(ISR_REG),
	REG_NAME(IER_REG),
	REG_NAME(CONFIG_REG),
	REG_NAME(TIMING_REG),
	REG_NAME(RESP_REG),
	REG_NAME(TIMING2_REG),
	REG_NAME(CMD_REG1),
	REG_NAME(CMD_REG2),
	REG_NAME(ADDR_REG1),
	REG_NAME(ADDR_REG2),
	REG_NAME(DMA_MST_CTRL_REG),
	REG_NAME(DMA_CFG_A_REG),
	REG_NAME(DMA_CFG_B_REG),
	REG_NAME(FIFO_CTRL_REG),
	REG_NAME(DATA_BLOCK_PTR_REG),
	REG_NAME(TAG_PTR_REG),
	REG_NAME(ECC_PTR_REG),
	REG_NAME(DEC_STATUS_REG),
	REG_NAME(HWSTATUS_CMD_REG),
	REG_NAME(HWSTATUS_MASK_REG),
	REG_NAME(LL_CONFIG_REG),
	REG_NAME(LL_STATUS_REG),
	REG_NAME(LL_PTR_REG),
	{ 0, NULL },
};

#undef REG_NAME

static int dump_nand_regs(void)
{
	int i = 0;

	TEGRA_DBG("%s: dumping registers\n", __func__);
	while (reg_names[i].name != NULL) {
		TEGRA_DBG("%s = 0x%08x\n", reg_names[i].name,
			  readl(reg_names[i].addr));
		i++;
	}
	TEGRA_DBG("%s: end of reg dump\n", __func__);
	return 1;
}

static inline void enable_ints(struct tegra_nand_info *info, uint32_t mask)
{
	(void)info;
	writel(readl(IER_REG) | mask, IER_REG);
}

static inline void disable_ints(struct tegra_nand_info *info, uint32_t mask)
{
	(void)info;
	writel(readl(IER_REG) & ~mask, IER_REG);
}

static inline void
split_addr(struct tegra_nand_info *info, loff_t offset, int *chipnr,
	   uint32_t *page, uint32_t *column)
{
	*chipnr = (int)(offset >> info->chip.chip_shift);
	*page = (offset >> info->chip.page_shift) & info->chip.page_mask;
	*column = offset & info->chip.column_mask;
}

static irqreturn_t tegra_nand_irq(int irq, void *dev_id)
{
	struct tegra_nand_info *info = dev_id;
	uint32_t isr;
	uint32_t ier;
	uint32_t ll;
	uint32_t dma_ctrl;
	uint32_t tmp;

	isr = readl(ISR_REG);
	ier = readl(IER_REG);
	ll = readl(LL_STATUS_REG);
	dma_ctrl = readl(DMA_MST_CTRL_REG);
#ifdef DEBUG_DUMP_IRQ
	pr_info("IRQ: ISR=0x%08x IER=0x%08x DMA_IS=%d DMA_IE=%d\n",
		isr, ier, !!(dma_ctrl & (1 << 20)), !!(dma_ctrl & (1 << 28)));
#endif
	if (isr & ISR_CMD_DONE) {
		if (likely(!(readl(COMMAND_REG) & COMMAND_GO)))
			complete(&info->cmd_complete);
		else
			pr_err("tegra_nand_irq: Spurious cmd done irq!\n");
	}

	if (isr & ISR_ECC_ERR) {
		/* always want to read the decode status so xfers dont stall.*/
		tmp = readl(DEC_STATUS_REG);

		/* was ECC check actually enabled */
		if ((ier & IER_ECC_ERR)) {
			unsigned long flags;
			spin_lock_irqsave(&info->ecc_lock, flags);

#ifdef TEGRA_NAND_DEBUG
			BUG_ON(info->num_ecc_errs >= info->max_ecc_errs);
#endif
			info->ecc_errs[info->num_ecc_errs++] = tmp;

			spin_unlock_irqrestore(&info->ecc_lock, flags);
		}
	}

	if ((dma_ctrl & DMA_CTRL_IS_DMA_DONE) &&
	    (dma_ctrl & DMA_CTRL_IE_DMA_DONE)) {
		complete(&info->dma_complete);
		writel(dma_ctrl, DMA_MST_CTRL_REG);
	}

#ifdef CONFIG_MTD_NAND_TEGRA_CQ_MODE
	if (isr & ISR_LL_DONE) {
		complete(&info->cq_complete);
		writel(isr, ISR_REG);
	}

	if (isr & ISR_LL_ERR)
		pr_err("%s: command queue error, RD status check failed\n",
					__func__);
#endif
	if ((isr & ISR_UND) && (ier & IER_UND))
		pr_err("%s: fifo underrun.\n", __func__);

	if ((isr & ISR_OVR) && (ier & IER_OVR))
		pr_err("%s: fifo overrun.\n", __func__);

	/* clear ALL interrupts?! */
	writel(isr & 0xffff, ISR_REG);

	return IRQ_HANDLED;
}

static inline int tegra_nand_is_cmd_done(struct tegra_nand_info *info)
{
	return (readl(COMMAND_REG) & COMMAND_GO) ? 0 : 1;
}

static int tegra_nand_wait_cmd_done(struct tegra_nand_info *info)
{
	uint32_t timeout = TIMEOUT;
	int ret;

	ret = wait_for_completion_timeout(&info->cmd_complete, timeout);

#ifdef TEGRA_NAND_DEBUG_PEDANTIC
	BUG_ON(!ret && dump_nand_regs());
#endif

	return ret ? 0 : -1;
}

static inline void select_chip(struct tegra_nand_info *info, int chipnr)
{
	BUG_ON(chipnr != -1 && chipnr >= info->plat->max_chips);
	info->chip.curr_chip = chipnr;
}

static void cfg_hwstatus_mon(struct tegra_nand_info *info)
{
	uint32_t val;

	val = (HWSTATUS_RDSTATUS_MASK(1) |
	       HWSTATUS_RDSTATUS_EXP_VAL(0) |
	       HWSTATUS_RBSY_MASK(NAND_STATUS_READY) |
	       HWSTATUS_RBSY_EXP_VAL(NAND_STATUS_READY));
	writel(NAND_CMD_STATUS, HWSTATUS_CMD_REG);
	writel(val, HWSTATUS_MASK_REG);
}

static int reset_controller(struct tegra_nand_info *info)
{
	int ret = 0;
	/* clear all pending interrupts */
	writel(readl(ISR_REG), ISR_REG);
	/* clear dma interrupt */
	writel(DMA_CTRL_IS_DMA_DONE, DMA_MST_CTRL_REG);

	/* enable interrupts */
	disable_ints(info, 0xffffffff);
	enable_ints(info,
			IER_ERR_TRIG_VAL(4) | IER_UND | IER_OVR | IER_CMD_DONE |
			IER_ECC_ERR | IER_GIE);

	writel(NAND_CMD_RESET, CMD_REG1);
	writel(0, CMD_REG2);
	writel(0, ADDR_REG1);
	writel(0, ADDR_REG2);
	writel(0, CONFIG_REG);
	writel(COMMAND_GO | COMMAND_CLE, COMMAND_REG);
	ret = tegra_nand_wait_cmd_done(info);
	if (ret)
		return -EIO;

	cfg_hwstatus_mon(info);

	set_chip_timing(info, info->vendor_id,
			info->device_id, info->dev_parms);
	return 0;

}

/* Tells the NAND controller to initiate the command. */
static int tegra_nand_go(struct tegra_nand_info *info)
{
	BUG_ON(!tegra_nand_is_cmd_done(info));

	INIT_COMPLETION(info->cmd_complete);
	writel(info->command_reg | COMMAND_GO, COMMAND_REG);

	if (unlikely(tegra_nand_wait_cmd_done(info))) {
		pr_err("%s: Timeout while waiting for command\n", __func__);
		reset_controller(info);
		return -ETIMEDOUT;
	}

	return 0;
}

#ifdef CONFIG_MTD_NAND_TEGRA_CQ_MODE

static inline int tegra_nand_is_cq_done(struct tegra_nand_info *info)
{
	return (readl(LL_CONFIG_REG) & LL_CONFIG_START) ? 0 : 1;
}

static int tegra_nand_cq_go(struct tegra_nand_info *info)
{
	uint32_t timeout = TIMEOUT;
	int ret;
	BUG_ON(!tegra_nand_is_cq_done(info));

	INIT_COMPLETION(info->cq_complete);
	writel(info->ll_config_reg | LL_CONFIG_START, LL_CONFIG_REG);

	ret = wait_for_completion_timeout(&info->cq_complete, timeout);
	if (!ret) {
		pr_err("%s: Timeout while waiting for cq\n", __func__);
		reset_controller(info);
		return -ETIMEDOUT;
	}
	return 0;
}
#endif

/* Tells the NAND controller to initiate the command. */
static void
tegra_nand_go_no_wait(struct tegra_nand_info *info)
{
	BUG_ON(!tegra_nand_is_cmd_done(info));
	INIT_COMPLETION(info->cmd_complete);
	writel(info->command_reg | COMMAND_GO, COMMAND_REG);
}

static void tegra_nand_prep_readid(struct tegra_nand_info *info)
{
	info->command_reg =
	    (COMMAND_CLE | COMMAND_ALE | COMMAND_PIO | COMMAND_RX |
	     COMMAND_ALE_BYTE_SIZE(0) | COMMAND_TRANS_SIZE(3) |
	     (COMMAND_CE(info->chip.curr_chip)));
	writel(NAND_CMD_READID, CMD_REG1);
	writel(0, CMD_REG2);
	writel(0, ADDR_REG1);
	writel(0, ADDR_REG2);
	writel(0, CONFIG_REG);
}

static int
tegra_nand_cmd_readid(struct tegra_nand_info *info, uint32_t *chip_id)
{
	int err;

#ifdef TEGRA_NAND_DEBUG_PEDANTIC
	BUG_ON(info->chip.curr_chip == -1);
#endif

	tegra_nand_prep_readid(info);
	err = tegra_nand_go(info);
	if (err != 0)
		return err;

	*chip_id = readl(RESP_REG);
	return 0;
}

/* assumes right locks are held */
static int nand_cmd_get_status(struct tegra_nand_info *info, uint32_t *status)
{
	int err;

	info->command_reg = (COMMAND_CLE | COMMAND_PIO | COMMAND_RX |
			     COMMAND_RBSY_CHK |
			     (COMMAND_CE(info->chip.curr_chip)));
	writel(NAND_CMD_STATUS, CMD_REG1);
	writel(0, CMD_REG2);
	writel(0, ADDR_REG1);
	writel(0, ADDR_REG2);
	writel(CONFIG_COM_BSY | CONFIG_EDO_MODE, CONFIG_REG);

	err = tegra_nand_go(info);
	if (err != 0)
		return err;

	*status = readl(RESP_REG) & 0xff;
	return 0;
}

/* must be called with lock held */
static int check_block_isbad(struct mtd_info *mtd, loff_t offs)
{
	struct tegra_nand_info *info = MTD_TO_INFO(mtd);
	uint32_t block = offs >> info->chip.block_shift;
	int chipnr;
	uint32_t page;
	uint32_t column;
	int ret = 0;
	int i;

	if (info->bb_bitmap[BIT_WORD(block)] & BIT_MASK(block))
		return 0;

	offs &= ~(mtd->erasesize - 1);

	if (info->is_data_bus_width_16)
		writel(CONFIG_COM_BSY | CONFIG_BUS_WIDTH | CONFIG_EDO_MODE,
				CONFIG_REG);
	else
		writel(CONFIG_COM_BSY | CONFIG_EDO_MODE, CONFIG_REG);

	split_addr(info, offs, &chipnr, &page, &column);
	select_chip(info, chipnr);

	column = mtd->writesize & 0xffff; /* force to be the offset of OOB */

	/* check fist two pages of the block */
	if (info->is_data_bus_width_16)
		column = column >> 1;
	for (i = 0; i < 2; ++i) {
		info->command_reg =
		    COMMAND_CE(info->chip.curr_chip) | COMMAND_CLE |
		    COMMAND_ALE | COMMAND_ALE_BYTE_SIZE(4) | COMMAND_RX |
		    COMMAND_PIO | COMMAND_TRANS_SIZE(1) | COMMAND_A_VALID |
		    COMMAND_RBSY_CHK | COMMAND_SEC_CMD;
		writel(NAND_CMD_READ0, CMD_REG1);
		writel(NAND_CMD_READSTART, CMD_REG2);

		writel(column | ((page & 0xffff) << 16), ADDR_REG1);
		writel((page >> 16) & 0xff, ADDR_REG2);

		/* ... poison me ... */
		writel(0xaa55aa55, RESP_REG);
		ret = tegra_nand_go(info);
		if (ret != 0) {
			pr_info("baaaaaad\n");
			goto out;
		}

		if ((readl(RESP_REG) & 0xffff) != 0xffff) {
			ret = 1;
			goto out;
		}

		/* Note: The assumption here is that we cannot cross chip
		 * boundary since the we are only looking at first 2 pages in
		 * a block, i.e. erasesize > writesize ALWAYS */
		page++;
	}

out:
	/* update the bitmap if the block is good */
	if (ret == 0)
		set_bit(block, info->bb_bitmap);
	return ret;
}

static int tegra_nand_block_isbad(struct mtd_info *mtd, loff_t offs)
{
	struct tegra_nand_info *info = MTD_TO_INFO(mtd);
	int ret;

	if (offs >= mtd->size)
		return -EINVAL;

	mutex_lock(&info->lock);
	ret = check_block_isbad(mtd, offs);
	mutex_unlock(&info->lock);

#if 0
	if (ret > 0)
		pr_info("block @ 0x%llx is bad.\n", offs);
	else if (ret < 0)
		pr_err("error checking block @ 0x%llx for badness.\n", offs);
#endif

	return ret;
}

/* This function invalidates the data in bufferpool if bufferpool address
 * range lie between 'from' to 'to'. Invalidating a buffer is done by changing
 * the buffer_frequency[index] array to 0.
 */
#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
static void
tegra_nand_invalidate_bufferpool(struct mtd_info *mtd, loff_t from, loff_t to)
{
	struct tegra_nand_info *info = MTD_TO_INFO(mtd);
	int i;
	for (i = 0; i < BUFFER_NUM; i++) {
		if (info->old_addresses[i] >= from  &&
				info->old_addresses[i] < to)
			info->buffer_frequency[i] = 0;
	}
}
#endif

static int
tegra_nand_block_markbad(struct mtd_info *mtd, loff_t offs)
{
	struct tegra_nand_info *info = MTD_TO_INFO(mtd);
	uint32_t block = offs >> info->chip.block_shift;
	int chipnr;
	uint32_t page;
	uint32_t column;
	int ret = 0;
	int i;

	if (offs >= mtd->size)
		return -EINVAL;

	pr_info("tegra_nand: setting block %d bad\n", block);

	mutex_lock(&info->lock);
	offs &= ~(mtd->erasesize - 1);

	/* mark the block bad in our bitmap */
	clear_bit(block, info->bb_bitmap);
	mtd->ecc_stats.badblocks++;

	if (info->is_data_bus_width_16)
		writel(CONFIG_COM_BSY | CONFIG_BUS_WIDTH | CONFIG_EDO_MODE,
				CONFIG_REG);
	else
		writel(CONFIG_COM_BSY | CONFIG_EDO_MODE, CONFIG_REG);

	split_addr(info, offs, &chipnr, &page, &column);
	select_chip(info, chipnr);

	column = mtd->writesize & 0xffff; /* force to be the offset of OOB */
	if (info->is_data_bus_width_16)
		column = column >> 1;
	/* write to fist two pages in the block */
	for (i = 0; i < 2; ++i) {
		info->command_reg =
		    COMMAND_CE(info->chip.curr_chip) | COMMAND_CLE |
		    COMMAND_ALE | COMMAND_ALE_BYTE_SIZE(4) | COMMAND_TX |
		    COMMAND_PIO | COMMAND_TRANS_SIZE(1) | COMMAND_A_VALID |
		    COMMAND_RBSY_CHK | COMMAND_AFT_DAT | COMMAND_SEC_CMD;
		writel(NAND_CMD_SEQIN, CMD_REG1);
		writel(NAND_CMD_PAGEPROG, CMD_REG2);

		writel(column | ((page & 0xffff) << 16), ADDR_REG1);
		writel((page >> 16) & 0xff, ADDR_REG2);

		writel(0x0, RESP_REG);
		ret = tegra_nand_go(info);
		if (ret != 0)
			goto out;

		/* TODO: check if the program op worked? */
		page++;
	}

out:
	mutex_unlock(&info->lock);
	return ret;
}

static int tegra_nand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct tegra_nand_info *info = MTD_TO_INFO(mtd);
	uint32_t num_blocks;
#ifdef CONFIG_MTD_NAND_TEGRA_RW_STATS
	uint32_t erase_count;
#endif
	uint32_t offs;
	int chipnr;
	uint32_t page;
	uint32_t column;
	uint32_t status = 0;

	TEGRA_DBG("tegra_nand_erase: addr=0x%08llx len=%lld\n", instr->addr,
		  instr->len);

	if ((instr->addr + instr->len) > mtd->size) {
		pr_err("tegra_nand_erase: Can't erase past end of device\n");
		instr->state = MTD_ERASE_FAILED;
		return -EINVAL;
	}

	if (instr->addr & (mtd->erasesize - 1)) {
		pr_err("tegra_nand_erase: addr=0x%08llx not block-aligned\n",
		       instr->addr);
		instr->state = MTD_ERASE_FAILED;
		return -EINVAL;
	}

	if (instr->len & (mtd->erasesize - 1)) {
		pr_err("tegra_nand_erase: len=%lld not block-aligned\n",
		       instr->len);
		instr->state = MTD_ERASE_FAILED;
		return -EINVAL;
	}

	instr->fail_addr = 0xffffffff;

	mutex_lock(&info->lock);

	instr->state = MTD_ERASING;

	offs = instr->addr;
	num_blocks = instr->len >> info->chip.block_shift;
#ifdef CONFIG_MTD_NAND_TEGRA_RW_STATS
	erase_count = num_blocks;
#endif

	select_chip(info, -1);

	while (num_blocks--) {
		split_addr(info, offs, &chipnr, &page, &column);
		if (chipnr != info->chip.curr_chip)
			select_chip(info, chipnr);
		TEGRA_DBG("tegra_nand_erase: addr=0x%08x, page=0x%08x\n", offs,
			  page);
#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
		tegra_nand_invalidate_bufferpool(mtd, offs,
				offs + mtd->erasesize);
#endif
		if (check_block_isbad(mtd, offs)) {
			pr_info("%s: skipping bad block @ 0x%08x\n", __func__,
				offs);
			goto next_block;
		}

		info->command_reg =
		    COMMAND_CE(info->chip.curr_chip) | COMMAND_CLE |
		    COMMAND_ALE | COMMAND_ALE_BYTE_SIZE(2) |
		    COMMAND_RBSY_CHK | COMMAND_SEC_CMD;
		writel(NAND_CMD_ERASE1, CMD_REG1);
		writel(NAND_CMD_ERASE2, CMD_REG2);

		writel(page & 0xffffff, ADDR_REG1);
		writel(0, ADDR_REG2);

		info->config_reg = CONFIG_EDO_MODE | CONFIG_COM_BSY;
		if (info->is_data_bus_width_16)
			info->config_reg |= CONFIG_BUS_WIDTH;

		writel(info->config_reg, CONFIG_REG);

		if (tegra_nand_go(info) != 0) {
			instr->fail_addr = offs;
			goto out_err;
		}

		if ((nand_cmd_get_status(info, &status) != 0) ||
		    (status & NAND_STATUS_FAIL) ||
		    ((status & NAND_STATUS_READY) != NAND_STATUS_READY)) {
			instr->fail_addr = offs;
			pr_info("%s: erase failed @ 0x%08x (stat=0x%08x)\n",
				__func__, offs, status);
			goto out_err;
		}
next_block:
		offs += mtd->erasesize;
	}

#ifdef CONFIG_MTD_NAND_TEGRA_RW_STATS
	info->rwe_stats.erase_stats += erase_count;
#endif
	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);
	mutex_unlock(&info->lock);
	return 0;

out_err:
#ifdef CONFIG_MTD_NAND_TEGRA_RW_STATS
	info->rwe_stats.erase_stats += (erase_count - num_blocks);
#endif
	instr->state = MTD_ERASE_FAILED;
	mutex_unlock(&info->lock);
	return -EIO;
}

static inline void dump_mtd_oob_ops(struct mtd_oob_ops *ops)
{
	pr_info("%s: oob_ops: mode=%s len=0x%x ooblen=0x%x "
		"ooboffs=0x%x dat=0x%p oob=0x%p\n", __func__,
		(ops->mode == MTD_OOB_AUTO ? "MTD_OOB_AUTO" :
		 (ops->mode ==
		  MTD_OOB_PLACE ? "MTD_OOB_PLACE" : "MTD_OOB_RAW")), ops->len,
		ops->ooblen, ops->ooboffs, ops->datbuf, ops->oobbuf);
}

static int
tegra_nand_read(struct mtd_info *mtd, loff_t from, size_t len,
		size_t *retlen, uint8_t *buf)
{
	struct mtd_oob_ops ops;
	int ret;

	pr_debug("%s: read: from=0x%llx len=0x%x\n", __func__, from, len);
	ops.mode = MTD_OOB_AUTO;
	ops.len = len;
	ops.datbuf = buf;
	ops.oobbuf = NULL;
	ret = mtd->read_oob(mtd, from, &ops);
	*retlen = ops.retlen;
	return ret;
}

static void
correct_ecc_errors_on_blank_page(struct tegra_nand_info *info, u8 *datbuf,
				 u8 *oobbuf, unsigned int a_len,
				 unsigned int b_len)
{
	int i;
	int all_ff = 1;
	unsigned long flags;

	spin_lock_irqsave(&info->ecc_lock, flags);
#ifdef TEGRA_NAND_DEBUG
	BUG_ON(info->num_ecc_errs > info->max_ecc_errs);
#endif
	if (info->num_ecc_errs) {
		if (datbuf) {
			for (i = 0; i < a_len; i++)
				if (datbuf[i] != 0xFF) {
					all_ff = 0;
					break;
				}
		}
		if (oobbuf) {
			for (i = 0; i < b_len; i++)
				if (oobbuf[i] != 0xFF) {
					all_ff = 0;
					break;
				}
		}
		if (all_ff)
			info->num_ecc_errs--;
	}
	spin_unlock_irqrestore(&info->ecc_lock, flags);
}

static void update_ecc_counts(struct tegra_nand_info *info, int check_oob)
{
	unsigned long flags;
	int i;

	spin_lock_irqsave(&info->ecc_lock, flags);
#ifdef TEGRA_NAND_DEBUG
	BUG_ON(info->num_ecc_errs > info->max_ecc_errs);
#endif
	for (i = 0; i < info->num_ecc_errs; ++i) {
		/* correctable */
		info->mtd.ecc_stats.corrected +=
		    DEC_STATUS_ERR_CNT(info->ecc_errs[i]);

		/* uncorrectable */
		if (info->ecc_errs[i] & DEC_STATUS_ECC_FAIL_A)
			info->mtd.ecc_stats.failed++;
		if (check_oob && (info->ecc_errs[i] & DEC_STATUS_ECC_FAIL_B))
			info->mtd.ecc_stats.failed++;
	}
	info->num_ecc_errs = 0;
	spin_unlock_irqrestore(&info->ecc_lock, flags);
}

static inline void clear_regs(struct tegra_nand_info *info)
{
	info->command_reg = 0;
	info->config_reg = 0;
	info->dmactrl_reg = 0;
}

static void
prep_transfer_dma(struct tegra_nand_info *info, int rx, int do_ecc,
		  uint32_t page, uint32_t column, dma_addr_t data_dma,
		  uint32_t data_len, dma_addr_t oob_dma, uint32_t oob_len)
{
	uint32_t tag_sz = oob_len;

	/*PAGE_SIZE_2048 = 3 in the Tegra2 TRM */
	uint32_t page_size_sel = (info->mtd.writesize >> 11) + 2;
#if 0
	pr_info("%s: rx=%d ecc=%d  page=%d col=%d data_dma=0x%x "
			"data_len=0x%08x oob_dma=0x%x ooblen=%d\n", __func__,
			rx, do_ecc, page, column, data_dma, data_len, oob_dma,
			oob_len);
#endif

	info->command_reg =
	    COMMAND_CE(info->chip.curr_chip) | COMMAND_CLE | COMMAND_ALE |
	    COMMAND_ALE_BYTE_SIZE(4) | COMMAND_SEC_CMD | COMMAND_RBSY_CHK |
	    COMMAND_TRANS_SIZE(8);

	info->config_reg = (CONFIG_PIPELINE_EN | CONFIG_EDO_MODE |
			    CONFIG_COM_BSY);
	if (info->is_data_bus_width_16)
		info->config_reg |= CONFIG_BUS_WIDTH;
	info->dmactrl_reg = (DMA_CTRL_DMA_GO |
				DMA_CTRL_DMA_PERF_EN | DMA_CTRL_IE_DMA_DONE |
				DMA_CTRL_IS_DMA_DONE | DMA_CTRL_BURST_SIZE(4));

	if (rx) {
		if (do_ecc)
			info->config_reg |= CONFIG_HW_ERR_CORRECTION;
		info->command_reg |= COMMAND_RX;
		info->dmactrl_reg |= DMA_CTRL_REUSE_BUFFER;
		writel(NAND_CMD_READ0, CMD_REG1);
		writel(NAND_CMD_READSTART, CMD_REG2);
	} else {
		info->command_reg |= (COMMAND_TX | COMMAND_AFT_DAT);
		info->dmactrl_reg |= DMA_CTRL_DIR; /* DMA_RD == TX */
		writel(NAND_CMD_SEQIN, CMD_REG1);
		writel(NAND_CMD_PAGEPROG, CMD_REG2);
	}

	if (data_len) {
		if (do_ecc)
			info->config_reg |= CONFIG_HW_ECC | CONFIG_ECC_SEL;
		info->config_reg |=
		    CONFIG_PAGE_SIZE_SEL(page_size_sel) | CONFIG_TVALUE(0) |
		    CONFIG_SKIP_SPARE | CONFIG_SKIP_SPARE_SEL(0);
		info->command_reg |= COMMAND_A_VALID;
		info->dmactrl_reg |= DMA_CTRL_DMA_EN_A;
		writel(DMA_CFG_BLOCK_SIZE(data_len - 1), DMA_CFG_A_REG);
		writel(data_dma, DATA_BLOCK_PTR_REG);
	} else {
		column = info->mtd.writesize;
		if (do_ecc)
			column += info->mtd.ecclayout->oobfree[0].offset;
		writel(0, DMA_CFG_A_REG);
		writel(0, DATA_BLOCK_PTR_REG);
	}

	if (oob_len) {
		if (do_ecc) {
			oob_len = info->mtd.oobavail;
			tag_sz = info->mtd.oobavail;
			tag_sz += 4;	/* size of tag ecc */
			if (rx)
				oob_len += 4; /* size of tag ecc */
			info->config_reg |= CONFIG_ECC_EN_TAG;
		}
		if (data_len && rx)
			oob_len += 4; /* num of skipped bytes */

		info->command_reg |= COMMAND_B_VALID;
		info->config_reg |= CONFIG_TAG_BYTE_SIZE(tag_sz - 1);
		info->dmactrl_reg |= DMA_CTRL_DMA_EN_B;
		writel(DMA_CFG_BLOCK_SIZE(oob_len - 1), DMA_CFG_B_REG);
		writel(oob_dma, TAG_PTR_REG);
	} else {
		writel(0, DMA_CFG_B_REG);
		writel(0, TAG_PTR_REG);
	}
	/* For x16 bit we needs to divide the column number by 2 */
	if (info->is_data_bus_width_16)
		column = column >> 1;
	writel((column & 0xffff) | ((page & 0xffff) << 16), ADDR_REG1);
	writel((page >> 16) & 0xff, ADDR_REG2);
}

static dma_addr_t
tegra_nand_dma_map(struct device *dev, void *addr, size_t size,
		   enum dma_data_direction dir)
{
	struct page *page;
	unsigned long offset = (unsigned long)addr & ~PAGE_MASK;
	if (virt_addr_valid(addr))
		page = virt_to_page(addr);
	else {
		if (WARN_ON(size + offset > PAGE_SIZE))
			return ~0;
		page = vmalloc_to_page(addr);
	}
	return dma_map_page(dev, page, offset, size, dir);
}

#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
/* This function implements LRU
 * If same address is found then it returns its index, if same address is not
 * present then it looks for unused buffer and returns its index and both if
 * these conditions are not met then index of buffer which has maximum
 * frequency(i.e which was not used for longest time) is returned.
 */
static inline int lru_implementation(long long address,
		long long *buffer_addresses, int *buffer_frequency)
{
	int i = 0;
	int same_address_index = -1;
	int unused_address_index = -1;
	int maxfreq_address_index = -1;
	int max = -1;
	for (; i < BUFFER_NUM; i++) {
		if (address == buffer_addresses[i])
			same_address_index = i;
		if (buffer_frequency[i] == 0) {
			unused_address_index = i;
			buffer_frequency[i] -= 1;
		}
		if (max < buffer_frequency[i]) {
			max = buffer_frequency[i];
			maxfreq_address_index = i;
		}
		buffer_frequency[i] += 1;
	}
	return (same_address_index == -1) ? ((unused_address_index == -1)
			? maxfreq_address_index : unused_address_index)
						: same_address_index;
}


void print_info(struct tegra_nand_info *in)
{
	int i = 0;
	printk(KERN_INFO "\n");
	for (; i < BUFFER_NUM; i++)
		printk(KERN_INFO "buffer_freq[%d]:%d old_addrs[%d]:0x%llx\n",
			i, in->buffer_frequency[i], i, in->old_addresses[i]);
	printk(KERN_INFO "\n");
}
#endif

#ifdef CONFIG_MTD_NAND_TEGRA_PAGE_CACHE
static int
prep_and_transfer_dma_cache(struct tegra_nand_info *info, int rx, int do_ecc,
		uint32_t page, uint32_t column, dma_addr_t data_dma,
		uint32_t data_len, dma_addr_t oob_dma, uint32_t oob_len,
		uint8_t *user_addr)
{
	uint32_t i = 0;
	int err;
	/*PAGE_SIZE_2048 = 3 in the Tegra2 TRM */
	uint32_t page_size_sel = (info->mtd.writesize >> 11) + 2;
#if 0
	pr_info("%s: rx=%d ecc=%d  page=%d col=%d data_dma=0x%x "
			"data_len=0x%08x oob_dma=0x%x ooblen=%d\n", __func__,
			rx, do_ecc, page, column, data_dma, data_len, oob_dma,
			oob_len);
#endif
	/* Start the cache mode */
	{
		info->command_reg =
			COMMAND_CE(info->chip.curr_chip) | COMMAND_CLE |
			COMMAND_ALE | COMMAND_ALE_BYTE_SIZE(4) |
			COMMAND_SEC_CMD | COMMAND_RBSY_CHK |
			COMMAND_TRANS_SIZE(8);

		info->config_reg = (CONFIG_PIPELINE_EN | CONFIG_EDO_MODE|
				CONFIG_COM_BSY);
		if (info->is_data_bus_width_16)
			info->config_reg |= CONFIG_BUS_WIDTH;
		info->dmactrl_reg = (DMA_CTRL_DMA_GO |
				DMA_CTRL_DMA_PERF_EN |
				DMA_CTRL_BURST_SIZE(4));

		if (rx) {
			if (do_ecc)
				info->config_reg |= CONFIG_HW_ERR_CORRECTION;
			/* Just read from the flash to the buffer don't transfer
			 * to the controller buffer */
			info->dmactrl_reg |= DMA_CTRL_REUSE_BUFFER;
			writel(NAND_CMD_READ0, CMD_REG1);
			writel(NAND_CMD_READSTART, CMD_REG2);
		}

		if (data_len) {
			if (do_ecc)
				info->config_reg |= CONFIG_HW_ECC |
					CONFIG_ECC_SEL;
			info->config_reg |=
				CONFIG_PAGE_SIZE_SEL(page_size_sel) |
				CONFIG_TVALUE(0) | CONFIG_SKIP_SPARE |
				CONFIG_SKIP_SPARE_SEL(0);
			info->command_reg |= COMMAND_A_VALID;
			info->dmactrl_reg |= DMA_CTRL_DMA_EN_A;
			/* Configure the DMA for mtd->writesize at one go */
		} else {
			return -EINVAL;
		}
		if (oob_len)
			return -EINVAL;

		writel(0, DMA_CFG_B_REG);
		writel(0, TAG_PTR_REG);

		if (info->is_data_bus_width_16)
			column = column/2;
		writel((column & 0xffff) | ((page & 0xffff) << 16), ADDR_REG1);
		writel((page >> 16) & 0xff, ADDR_REG2);
		writel(info->config_reg, CONFIG_REG);

		err = tegra_nand_go(info);
		if (err != 0)
			goto out_err;
	}
	/* Read the intermediate pages
	 * Here we will be doing dma read and memcpy in pipeline
	 * Issue the dma-read for next page and start memcpy for the previous
	 * page simultaneously */
	for (i = 0; i < (data_len - info->mtd.writesize)/info->mtd.writesize
			; i++) {
		/* reuse the data_buf as request can be for more than
		 * LOGICAL_PAGES_COMBINE, Here j will be use for indexing
		 * in the local dma buffer and i for user_addr buffer */
		uint32_t j = i & (LOGICAL_PAGES_COMBINE - 1);
		info->command_reg =
			COMMAND_CE(info->chip.curr_chip) | COMMAND_CLE |
			COMMAND_RBSY_CHK ;

		writel(NAND_CMD_READCACHESTART, CMD_REG1);
		writel(0, CMD_REG2);
		err = tegra_nand_go(info);
		if (err != 0)
			goto out_err;

		info->command_reg =
			COMMAND_CE(info->chip.curr_chip) |
			COMMAND_RBSY_CHK | COMMAND_RX | COMMAND_TRANS_SIZE(8) |
			COMMAND_A_VALID;
		writel(NAND_CMD_READSTART, CMD_REG1);
		writel(0, CMD_REG2);

		writel(DMA_CFG_BLOCK_SIZE(info->mtd.writesize - 1),
				DMA_CFG_A_REG);
		writel(data_dma + j * info->mtd.writesize, DATA_BLOCK_PTR_REG);
		writel(info->dmactrl_reg, DMA_MST_CTRL_REG);

		INIT_COMPLETION(info->dma_complete);
		tegra_nand_go_no_wait(info);

		if (i) {
			uint32_t buf_offs = (j) ?
				((j-1)*info->mtd.writesize) :
				((LOGICAL_PAGES_COMBINE-1)*info->mtd.writesize);
			memcpy(user_addr + (i-1) * info->mtd.writesize,
				info->dma_buf + buf_offs,
				info->mtd.writesize);
		}
		if (unlikely(tegra_nand_wait_cmd_done(info))) {
			reset_controller(info);
			pr_err("%s: Timeout while waiting for command\n",
					__func__);
			return -ETIMEDOUT;
		}
		{
			int counter = 0;
			while (counter < 100) {
				int val = readl(DMA_MST_CTRL_REG) >> 31;
				counter++;
				if (!val)
					break;
				usleep_range(0, 1);
			}
			if (counter == 100) {
				pr_err("%s:%d: DMA operation timed out!",
						__func__, __LINE__);
				return -ETIMEDOUT;
			}
		}
		dmac_unmap_area(info->dma_buf + j * info->mtd.writesize,
				info->mtd.writesize, DMA_FROM_DEVICE);
		outer_inv_range(data_dma + j * info->mtd.writesize,
				data_dma + info->mtd.writesize*(j+1));
	}
	/* Last Page read */
	{
		uint32_t j = i & (LOGICAL_PAGES_COMBINE - 1);
		uint32_t buf_offs;

		info->command_reg =
			COMMAND_CE(info->chip.curr_chip) | COMMAND_CLE |
			COMMAND_RBSY_CHK ;

		writel(NAND_CMD_READCACHEEND, CMD_REG1);
		writel(NAND_CMD_READSTART, CMD_REG2);
		err = tegra_nand_go(info);
		if (err != 0)
			goto out_err;
		info->command_reg =
			COMMAND_CE(info->chip.curr_chip) |
			COMMAND_RX | COMMAND_RBSY_CHK | COMMAND_TRANS_SIZE(8);
		if (data_len)
			info->command_reg |= COMMAND_A_VALID;
		writel(NAND_CMD_READSTART, CMD_REG1);
		writel(NAND_CMD_READSTART, CMD_REG2);

		writel(DMA_CFG_BLOCK_SIZE(info->mtd.writesize - 1),
				DMA_CFG_A_REG);
		writel(data_dma + j * info->mtd.writesize, DATA_BLOCK_PTR_REG);
		writel(info->dmactrl_reg, DMA_MST_CTRL_REG);

		INIT_COMPLETION(info->dma_complete);
		tegra_nand_go_no_wait(info);

		buf_offs = (j) ?
			((j-1)*info->mtd.writesize) :
			((LOGICAL_PAGES_COMBINE-1)*info->mtd.writesize);
		memcpy(user_addr + (i-1) * info->mtd.writesize,
				info->dma_buf + buf_offs,
				info->mtd.writesize);

		if (unlikely(tegra_nand_wait_cmd_done(info))) {
			reset_controller(info);
			pr_err("%s: Timeout while waiting for command\n",
					__func__);
			return -ETIMEDOUT;
		}
		{
			int counter = 0;
			while (counter < 100) {
				int val = readl(DMA_MST_CTRL_REG) >> 31;
				counter++;
				if (!val)
					break;
				usleep_range(0, 1);
			}
			if (counter == 100) {
				pr_err("%s:%d: DMA operation timed out!",
						__func__, __LINE__);
				return -ETIMEDOUT;
			}
		}
		dmac_unmap_area(info->dma_buf + j*info->mtd.writesize,
			info->mtd.writesize, DMA_FROM_DEVICE);
		outer_inv_range(data_dma + j*info->mtd.writesize,
			data_dma + info->mtd.writesize*(j+1));
		memcpy(user_addr + (i)*info->mtd.writesize,
			info->dma_buf + (j)*info->mtd.writesize,
			info->mtd.writesize);
	}
out_err:
	return err;
}
#endif

static ssize_t show_vendor_id(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct tegra_nand_info *info = dev_get_drvdata(dev);
	return sprintf(buf, "0x%x\n", info->vendor_id);
}

static DEVICE_ATTR(vendor_id, S_IRUSR, show_vendor_id, NULL);

#ifdef CONFIG_MTD_NAND_TEGRA_RW_STATS
static ssize_t show_rw_stats(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	struct tegra_nand_info *info = dev_get_drvdata(dev);
	ssize_t retval = 0;

	mutex_lock(&info->lock);
	retval = sprintf(buf, "blocks_erased=%llu\nread_req=%llu\n"
	"oob_req=%llu\ndata_read=%llu\noob_read=%llu\nwrite_req=%llu\n"
	"oobw_req=%llu\ndata_wrote=%llu\noob_wrote=%llu\nr_l2k=%u\n"
	"r_l4k=%u\nr_l8k=%u\nr_l16k=%u\nr_l32k=%u\nr_l64k=%u\n"
	"r_l128k=%u\nr_g128k=%u\n",
		info->rwe_stats.erase_stats,
		(info->rwe_stats.read_req_bytes)/2048,
		info->rwe_stats.oob_req_bytes,
		info->rwe_stats.data_read_pages,
		info->rwe_stats.oob_read_bytes,
		(info->rwe_stats.write_req_bytes)/2048,
		info->rwe_stats.oobw_req_bytes,
		info->rwe_stats.written_pages,
		info->rwe_stats.oob_written_bytes,
		info->rwe_stats.read_req_l2k,
		info->rwe_stats.read_req_l4k,
		info->rwe_stats.read_req_l8k,
		info->rwe_stats.read_req_l16k,
		info->rwe_stats.read_req_l32k,
		info->rwe_stats.read_req_l64k,
		info->rwe_stats.read_req_l128k,
		info->rwe_stats.read_req_g128k);
	mutex_unlock(&info->lock);
	return retval;
}

static ssize_t clear_rw_stats(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct tegra_nand_info *info = dev_get_drvdata(dev);

	mutex_lock(&info->lock);
	info->rwe_stats.erase_stats = 0;
	info->rwe_stats.read_req_bytes = 0;
	info->rwe_stats.oob_req_bytes = 0;
	info->rwe_stats.data_read_pages = 0;
	info->rwe_stats.oob_read_bytes = 0;
	info->rwe_stats.write_req_bytes = 0;
	info->rwe_stats.oobw_req_bytes = 0;
	info->rwe_stats.written_pages = 0;
	info->rwe_stats.oob_written_bytes = 0;
	info->rwe_stats.read_req_l2k = 0;
	info->rwe_stats.read_req_l4k = 0;
	info->rwe_stats.read_req_l8k = 0;
	info->rwe_stats.read_req_l16k = 0;
	info->rwe_stats.read_req_l32k = 0;
	info->rwe_stats.read_req_l64k = 0;
	info->rwe_stats.read_req_l128k = 0;
	info->rwe_stats.read_req_g128k = 0;
	mutex_unlock(&info->lock);
	return count;
}
static DEVICE_ATTR(rw_stats, S_IRUSR | S_IWUSR, show_rw_stats, clear_rw_stats);
#endif

static ssize_t show_device_id(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct tegra_nand_info *info = dev_get_drvdata(dev);
	return sprintf(buf, "0x%x\n", info->device_id);
}

static DEVICE_ATTR(device_id, S_IRUSR, show_device_id, NULL);

static ssize_t show_flash_size(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct tegra_nand_info *info = dev_get_drvdata(dev);
	struct mtd_info *mtd = &info->mtd;
	return sprintf(buf, "%llu bytes\n", mtd->size);
}

static DEVICE_ATTR(flash_size, S_IRUSR, show_flash_size, NULL);

static ssize_t show_num_bad_blocks(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct tegra_nand_info *info = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", info->num_bad_blocks);
}

static DEVICE_ATTR(num_bad_blocks, S_IRUSR, show_num_bad_blocks, NULL);

static ssize_t show_bb_bitmap(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct tegra_nand_info *info = dev_get_drvdata(dev);
	struct mtd_info *mtd = &info->mtd;
	int num_blocks = mtd->size >> info->chip.block_shift, i, ret = 0, size =
	    0;

	for (i = 0; i < num_blocks / (8 * sizeof(unsigned long)); i++) {
		size = sprintf(buf, "0x%lx\n", info->bb_bitmap[i]);
		ret += size;
		buf += size;
	}
	return ret;
}

static DEVICE_ATTR(bb_bitmap, S_IRUSR, show_bb_bitmap, NULL);

#ifdef CONFIG_MTD_NAND_TEGRA_CQ_MODE
void init_cq(struct tegra_nand_info *info)
{
	memset(info->cq, 0, (MAX_CQ_SIZE * sizeof(struct cq_packet)));
	info->cq_current_packet = 0;
}

void write_packet(struct tegra_nand_info *info)
{
	struct cq_reg_val *cq_regs = &info->cq_regs;
	info->cq[info->cq_current_packet].command = CQ_COMMAND_EN |
		CQ_ADDR_REG2_EN | CQ_ADDR_REG1_EN | CQ_DMA_MST_CTRL_EN |
		CQ_DATA_BLOCK_PTR_EN | CQ_CMD_REG1_EN | CQ_CMD_REG2_EN |
		CQ_COMMAND_PACKETID(info->cq_current_packet + 1);

	info->cq[info->cq_current_packet].data[CQ_COMMAND_REG] = cq_regs->cmd_reg;
	info->cq[info->cq_current_packet].data[CQ_ADDR_REG2_REG] = cq_regs->addr2;
	info->cq[info->cq_current_packet].data[CQ_ADDR_REG1_REG] = cq_regs->addr1;
	info->cq[info->cq_current_packet].data[CQ_DMA_MST_CTRL_REG] = cq_regs->dmactrl_reg;
	info->cq[info->cq_current_packet].data[CQ_DATA_BLOCK_PTR_REG] = cq_regs->dma_addr_reg;
	info->cq[info->cq_current_packet].data[CQ_CMD_REG1_REG] = cq_regs->cmd1;
	info->cq[info->cq_current_packet].data[CQ_CMD_REG2_REG] = cq_regs->cmd2;

	info->cq_current_packet++;
}

static void
prep_and_transfer_dma_cache_cq(struct tegra_nand_info *info, int rx,
		int do_ecc, uint32_t from, uint8_t *datbuf,  uint32_t len,
		dma_addr_t oob_dma, uint32_t oob_len)
{
	uint32_t i = 0;
	/*PAGE_SIZE_2048 = 3 in the Tegra2 TRM */
	uint32_t page_size_sel = (info->mtd.writesize >> 11) + 2;
	uint32_t data_len = info->mtd.writesize;
	uint32_t page, column, chipnr;

	column = 0;
	split_addr(info, from, &chipnr, &page, &column);
	select_chip(info, chipnr);

#if 0
	pr_info("%s: rx=%d ecc=%d  page=%d col=%d data_dma=0x%x "
			"data_len=0x%08x oob_dma=0x%x ooblen=%d\n", __func__,
			rx, do_ecc, page, column, data_dma, data_len, oob_dma,
			oob_len);
#endif
	/* Start the cache mode */
	{
		info->command_reg = COMMAND_CE(info->chip.curr_chip) |
			COMMAND_CLE | COMMAND_ALE | COMMAND_ALE_BYTE_SIZE(4) |
			COMMAND_SEC_CMD | COMMAND_RBSY_CHK |
			COMMAND_TRANS_SIZE(8);

		info->config_reg = CONFIG_PIPELINE_EN | CONFIG_EDO_MODE |
			CONFIG_COM_BSY;
		if (info->is_data_bus_width_16)
			info->config_reg |= CONFIG_BUS_WIDTH;
		info->dmactrl_reg = (DMA_CTRL_DMA_GO |
				DMA_CTRL_DMA_PERF_EN |
				DMA_CTRL_BURST_SIZE(4));

		if (rx) {
			if (do_ecc)
				info->config_reg |= CONFIG_HW_ERR_CORRECTION;
			/* Just read from the flash to the buffer don't transfer
			 * to the controller buffer */
			info->dmactrl_reg |= DMA_CTRL_REUSE_BUFFER;
		}

		if (data_len) {
			info->config_reg |= CONFIG_HW_ECC | CONFIG_ECC_SEL |
				CONFIG_PAGE_SIZE_SEL(page_size_sel) |
				CONFIG_TVALUE(0) | CONFIG_SKIP_SPARE |
				CONFIG_SKIP_SPARE_SEL(0);
			info->command_reg |= COMMAND_A_VALID;
			info->dmactrl_reg |= DMA_CTRL_DMA_EN_A;
		} else
			return;

		writel(0, DMA_CFG_B_REG);
		writel(0, TAG_PTR_REG);
		writel(DMA_CFG_BLOCK_SIZE(info->mtd.writesize - 1),
				DMA_CFG_A_REG);
		writel(info->config_reg, CONFIG_REG);

		if (info->is_data_bus_width_16)
			column = column/2;


		info->cq_regs.addr1 = (column & 0xffff) |
			((page & 0xffff) << 16);
		info->cq_regs.addr2 = (page >> 16) & 0xff;
		info->cq_regs.cmd_reg = info->command_reg | COMMAND_GO;
		info->cq_regs.cmd1 = NAND_CMD_READ0;
		info->cq_regs.cmd2 = NAND_CMD_READSTART;
		/* No DMA on first page request */
		info->cq_regs.dmactrl_reg = 0;
		info->cq_regs.dma_addr_reg = 0;
		write_packet(info);

	}

	for (i = 0; i < len - info->mtd.writesize; i += data_len) {
		dma_addr_t data_dma = tegra_nand_dma_map(info->dev, datbuf + i,
						data_len, DMA_FROM_DEVICE);

		info->command_reg =
			COMMAND_CE(info->chip.curr_chip) | COMMAND_CLE |
			COMMAND_RBSY_CHK ;

		info->cq_regs.cmd_reg = info->command_reg | COMMAND_GO;
		info->cq_regs.cmd1 = NAND_CMD_READCACHESTART;
		info->cq_regs.cmd2 = NAND_CMD_READ0;
		/* No DMA on first page request */
		info->cq_regs.dmactrl_reg = 0;
		info->cq_regs.dma_addr_reg = 0;
		write_packet(info);

		info->command_reg =
			COMMAND_CE(info->chip.curr_chip) |
			COMMAND_RBSY_CHK | COMMAND_RX | COMMAND_TRANS_SIZE(8) |
			COMMAND_A_VALID;

		info->cq_regs.cmd_reg = info->command_reg | COMMAND_GO;
		info->cq_regs.cmd1 = NAND_CMD_READSTART;
		info->cq_regs.cmd2 = NAND_CMD_READ0;
		info->cq_regs.dmactrl_reg = info->dmactrl_reg;
		info->cq_regs.dma_addr_reg = data_dma;
		write_packet(info);

	}
	/* Last Page read */
	{
		dma_addr_t data_dma = tegra_nand_dma_map(info->dev, datbuf + i,
						data_len, DMA_FROM_DEVICE);

		info->command_reg =
			COMMAND_CE(info->chip.curr_chip) | COMMAND_CLE |
			COMMAND_RBSY_CHK;

		info->cq_regs.cmd_reg = info->command_reg | COMMAND_GO;
		info->cq_regs.cmd1 = NAND_CMD_READCACHEEND;
		info->cq_regs.cmd2 = NAND_CMD_READSTART;
		/* No DMA on page request */
		info->cq_regs.dmactrl_reg = 0;
		info->cq_regs.dma_addr_reg = 0;
		write_packet(info);

		info->command_reg =
			COMMAND_CE(info->chip.curr_chip) | COMMAND_A_VALID |
			COMMAND_RX | COMMAND_RBSY_CHK | COMMAND_TRANS_SIZE(8);

		info->cq_regs.cmd_reg = info->command_reg | COMMAND_GO;
		info->cq_regs.cmd1 = NAND_CMD_READSTART;
		info->cq_regs.cmd2 = NAND_CMD_READSTART;
		info->cq_regs.dmactrl_reg = info->dmactrl_reg;
		info->cq_regs.dma_addr_reg = data_dma;
		write_packet(info);

	}
}

static void
prep_transfer_dma_cq(struct tegra_nand_info *info, int rx, int do_ecc,
		uint32_t from, uint8_t *datbuf,  uint32_t len,
		dma_addr_t oob_dma, uint32_t oob_len)
{
	/*PAGE_SIZE_2048 = 3 in the Tegra2 TRM */
	uint32_t page_size_sel = (info->mtd.writesize >> 11) + 2;
	uint32_t i = 0;
	uint32_t data_len = info->mtd.writesize;
	uint32_t page, column, chipnr;
#if 0
	pr_info("%s: rx=%d ecc=%d  page=%d col=%d data_dma=0x%x "
			"data_len=0x%08x oob_dma=0x%x ooblen=%d\n", __func__,
			rx, do_ecc, page, column, data_dma,
			data_len/info->mtd.writesize, oob_dma, oob_len);
#endif
	for (i = 0; i < len; i += data_len) {
		dma_addr_t data_dma = tegra_nand_dma_map(info->dev, datbuf + i,
						data_len, DMA_FROM_DEVICE);
		column = 0;
		split_addr(info, from, &chipnr, &page, &column);
		select_chip(info, chipnr);

		info->command_reg =
			COMMAND_CE(info->chip.curr_chip) | COMMAND_CLE |
			COMMAND_ALE | COMMAND_ALE_BYTE_SIZE(4) |
			COMMAND_SEC_CMD | COMMAND_RBSY_CHK |
			COMMAND_TRANS_SIZE(8) | COMMAND_RD_STATUS_CHK;

		info->config_reg = CONFIG_PIPELINE_EN | CONFIG_EDO_MODE|
				CONFIG_COM_BSY | CONFIG_HW_ERR_CORRECTION;

		if (info->is_data_bus_width_16)
			info->config_reg |= CONFIG_BUS_WIDTH;
		info->dmactrl_reg = (DMA_CTRL_DMA_GO |
				DMA_CTRL_DMA_PERF_EN |
				DMA_CTRL_BURST_SIZE(4));

		info->command_reg |= COMMAND_RX;
		info->dmactrl_reg |= DMA_CTRL_REUSE_BUFFER;
		writel(NAND_CMD_READ0, CMD_REG1);
		writel(NAND_CMD_READSTART, CMD_REG2);

		if (data_len) {
			info->config_reg |= CONFIG_HW_ECC |
				CONFIG_ECC_SEL | CONFIG_SKIP_SPARE_SEL(0) |
				CONFIG_PAGE_SIZE_SEL(page_size_sel) |
				CONFIG_TVALUE(0) | CONFIG_SKIP_SPARE;
			info->command_reg |= COMMAND_A_VALID;
			info->dmactrl_reg |= DMA_CTRL_DMA_EN_A;
			writel(DMA_CFG_BLOCK_SIZE(data_len - 1), DMA_CFG_A_REG);
		} else {
			return;
		}

		writel(info->config_reg, CONFIG_REG);
		writel(0, DMA_CFG_B_REG);
		writel(0, TAG_PTR_REG);

		/* For x16 bit we needs to divide the column number by 2 */
		if (info->is_data_bus_width_16)
			column = column >> 1;

		info->cq_regs.addr1 = (column & 0xffff) |
					((page & 0xffff) << 16);
		info->cq_regs.addr2 = (page >> 16) & 0xff;
		info->cq_regs.dmactrl_reg = info->dmactrl_reg | DMA_CTRL_DMA_GO;
		info->cq_regs.dma_addr_reg = data_dma;
		info->cq_regs.cmd_reg = info->command_reg | COMMAND_GO;
		info->cq_regs.cmd1 = NAND_CMD_READ0;
		info->cq_regs.cmd2 = NAND_CMD_READSTART;

		write_packet(info);

		from += data_len;
	}
}

static int cq_run(struct tegra_nand_info *info, uint32_t cq_count)
{

	int err = 0;
	uint32_t cq_phy = tegra_nand_dma_map(info->dev, &(info->cq),
				(cq_count * sizeof(struct cq_packet)),
				DMA_TO_DEVICE);

	disable_ints(info, IER_CMD_DONE);
	enable_ints(info, ISR_LL_DONE | ISR_LL_ERR);

	info->ll_config_reg = LL_CONFIG_WORD_CNT_EN |
		LL_CONFIG_BURST_WORD(4) |
		LL_CONFIG_LENGTH((cq_count * sizeof(struct cq_packet))>>2);
	writel(cq_phy, LL_PTR_REG);
	/* perform the transaction */
	err = tegra_nand_cq_go(info);
	if (err)
		goto out_err;
out_err:
	disable_ints(info, ISR_LL_DONE | ISR_LL_ERR);
	enable_ints(info, IER_CMD_DONE);

	return err;
}
#endif

/*
 * Independent of Mode, we read main data and the OOB data
 * from the oobfree areas as specified nand_ecclayout
 * This function also checks buffer pool partial_unaligned_rw_buffer
 * if the address is already present and is not 'unused' then it will use
 * data in buffer else it will go for DMA.
 */
static int
do_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	struct tegra_nand_info *info = MTD_TO_INFO(mtd);
	struct mtd_ecc_stats old_ecc_stats;
	int chipnr;
	uint32_t page;
	uint32_t column;
	uint8_t *datbuf = ops->datbuf;
	uint8_t *oobbuf = ops->oobbuf;
	uint32_t ooblen = oobbuf ? ops->ooblen : 0;
	uint32_t oobsz;
	uint32_t page_count;
	int err;
	int unaligned = from & info->chip.column_mask;
	uint32_t len = datbuf ? ((ops->len) + unaligned) : 0;
	int do_ecc = 1;
	dma_addr_t datbuf_dma_addr = 0;
#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
	int address_index_in_buffer = -1;
	int is_buffered = 0;
	/* Address to be used for invalidating bufferpool */
	int aligned_address_for_bufferpool = from - unaligned;
	int initial_aligned_data_length_to_read = len;
#endif

#if 0
	dump_mtd_oob_ops(ops);
#endif

	if (!oobbuf && !datbuf) {
		pr_err("%s: Invalid read 0 bytes from data and oob\n",
						__func__);
		return -EINVAL;
	}

	if (unlikely(oobbuf && !ooblen)) {
		pr_err("%s: Invalid read 0 bytes from OOB\n",
						__func__);
		return -EINVAL;
	}

	if (unlikely(datbuf && !len)) {
		pr_err("%s: Invalid read 0 bytes from data\n", __func__);
		return -EINVAL;
	}

	ops->retlen = 0;
	ops->oobretlen = 0;
	from = from - unaligned;

	/* Don't care about the MTD_OOB_ value field, use oobavail and ecc */
	oobsz = mtd->oobavail;
	if (unlikely(oobbuf && ooblen > oobsz)) {
		pr_err("%s: can't read OOB from multiple pages (%d > %d)\n",
		       __func__, ooblen, oobsz);
		return -EINVAL;
	} else if (oobbuf && !len) {
		page_count = 1;
	} else {
		page_count =
		    (uint32_t) ((len + mtd->writesize - 1) / mtd->writesize);
	}

	mutex_lock(&info->lock);

#ifdef CONFIG_MTD_NAND_TEGRA_RW_STATS
	info->rwe_stats.read_req_bytes += len;
	info->rwe_stats.oob_req_bytes += ooblen;
	if (page_count <= 1)
		info->rwe_stats.read_req_l2k += 1;
	else if (page_count <= 2)
		info->rwe_stats.read_req_l4k += 1;
	else if (page_count <= 4)
		info->rwe_stats.read_req_l8k += 1;
	else if (page_count <= 8)
		info->rwe_stats.read_req_l16k += 1;
	else if (page_count <= 16)
		info->rwe_stats.read_req_l32k += 1;
	else if (page_count <= 32)
		info->rwe_stats.read_req_l64k += 1;
	else if (page_count <= 64)
		info->rwe_stats.read_req_l128k += 1;
	else
		info->rwe_stats.read_req_g128k += 1;
#endif

	memcpy(&old_ecc_stats, &mtd->ecc_stats, sizeof(old_ecc_stats));

	if (do_ecc) {
		enable_ints(info, IER_ECC_ERR);
		writel(info->ecc_addr, ECC_PTR_REG);
	} else
		disable_ints(info, IER_ECC_ERR);

	split_addr(info, from, &chipnr, &page, &column);
	select_chip(info, chipnr);

	/* reset it to point back to beginning of page */
	from -= column;

	while (page_count--) {
		int a_len = min(mtd->writesize - column, len);
		int b_len = min(oobsz, ooblen);
		int temp_len = 0;
		char *temp_buf = NULL;
#if 0
#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
		print_info(info);
#endif
#endif
		/* Take care when read is of less than page size.
		 * Otherwise there will be kernel Panic due to DMA timeout */
		if (((a_len < mtd->writesize) && len) || unaligned) {
			temp_len = a_len;
			a_len = mtd->writesize;
			temp_buf = datbuf;

#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
			address_index_in_buffer = lru_implementation(from,
				info->old_addresses, info->buffer_frequency);
			if ((info->old_addresses[address_index_in_buffer] ==
						from) &&
				(info->buffer_frequency[address_index_in_buffer]
				 != 0)) {
				datbuf = info->partial_unaligned_rw_buffer +
				((mtd->writesize)*address_index_in_buffer);
				is_buffered = 1;
			} else {
				datbuf = info->partial_unaligned_rw_buffer +
				((mtd->writesize)*address_index_in_buffer);
				info->old_addresses[address_index_in_buffer] =
					from;
				is_buffered = 0;
			}
			/* buffer_frequency denotes last time it was accessed */
			info->old_addresses[address_index_in_buffer] = from;
			info->buffer_frequency[address_index_in_buffer] = 1;
#else
			datbuf = info->partial_unaligned_rw_buffer;
#endif

		}
#if 0
		pr_info("%s: chip:=%d page=%d col=%d\n", __func__, chipnr,
			page, column);
#endif
#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
		if (!is_buffered) {
#else
		{
#endif
			clear_regs(info);
			if (datbuf)
				datbuf_dma_addr = tegra_nand_dma_map(info->dev,
						datbuf, a_len, DMA_FROM_DEVICE);
			disable_ints(info, IER_CMD_DONE);
			prep_transfer_dma(info, 1, do_ecc, page, column,
					datbuf_dma_addr, a_len,
					info->oob_dma_addr, b_len);
			writel(info->config_reg, CONFIG_REG);
			writel(info->dmactrl_reg, DMA_MST_CTRL_REG);

			INIT_COMPLETION(info->dma_complete);
			tegra_nand_go_no_wait(info);

			if (!wait_for_completion_timeout(&info->dma_complete,
						2*TIMEOUT)) {
				/* Check the DMA Register and see if we go bit
				 * is clear before declaring it as error. We
				 * might have lost a interrupt here */
				if (readl(DMA_MST_CTRL_REG) & COMMAND_GO) {
					pr_err("%s: dma completion timeout\n",
							__func__);
					dump_nand_regs();
					err = -ETIMEDOUT;
					enable_ints(info, IER_CMD_DONE);
					goto out_err;
				}
			}
			enable_ints(info, IER_CMD_DONE);

			/*pr_info("tegra_read_oob: DMA complete\n");*/

			/* if we are here, transfer is done */
			if (datbuf)
				dma_unmap_page(info->dev, datbuf_dma_addr,
						a_len, DMA_FROM_DEVICE);
		}
		if (oobbuf) {
			/* skipped bytes */
			uint32_t ofs = datbuf && oobbuf ? 4 : 0;
			memcpy(oobbuf, info->oob_dma_buf + ofs, b_len);
		}

		correct_ecc_errors_on_blank_page(info, datbuf, oobbuf, a_len,
						 b_len);
		/* Take care when read is of less than page size */
		if (temp_len) {
			memcpy(temp_buf, datbuf + unaligned,
			       temp_len - unaligned);
			a_len = temp_len;
			datbuf = temp_buf;
		}
#ifdef CONFIG_MTD_NAND_TEGRA_RW_STATS
#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
		if (!is_buffered) {
#else
		{
#endif
			if (len)
				info->rwe_stats.data_read_pages += 1;
			if (ooblen)
				info->rwe_stats.oob_read_bytes += b_len;
		}
#endif

		if (datbuf) {
			len -= a_len;
			datbuf += a_len - unaligned;
			ops->retlen += a_len - unaligned;
		}

		if (oobbuf) {
			ooblen -= b_len;
			oobbuf += b_len;
			ops->oobretlen += b_len;
		}

#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
		/*is_buffered is set to 0 so that it go for dma next time */
		is_buffered = 0;
#endif
		unaligned = 0;
		update_ecc_counts(info, oobbuf != NULL);

		if (!page_count)
			break;

		from += mtd->writesize;
		column = 0;

		split_addr(info, from, &chipnr, &page, &column);
		if (chipnr != info->chip.curr_chip)
			select_chip(info, chipnr);

#if defined(CONFIG_MTD_NAND_TEGRA_PAGE_CACHE) ||\
	defined(CONFIG_MTD_NAND_TEGRA_CQ_MODE)
		if (page_count >=
			(mtd->writebufsize-mtd->writesize)/mtd->writesize) {
			int i = 0, num_pages = 0;

			/* Assuming:
			 * 1) Previous ecc errors are already accounted and
			 * 2) current num_ecc_errs = 0,
			 * page cache read mode would be used for maximum of
			 * 32 pages in one go */
			if (page_count > info->max_ecc_errs)
				num_pages = info->max_ecc_errs;
			else
				num_pages = page_count;

			a_len = num_pages * mtd->writesize;
			if (len < a_len) {
				a_len -= mtd->writesize;
				num_pages--;
			}
#ifdef CONFIG_MTD_NAND_TEGRA_CQ_MODE
			init_cq(info);
			prep_and_transfer_dma_cache_cq(info, 1, do_ecc, from,
					datbuf, a_len, info->oob_dma_addr, 0);
			err = cq_run(info, num_pages * 2 + 1);
			/*
			 * cq[0] & cq[1] contains the intial setup info
			 * cq[2][4].... contains the dma map address
			 */
			for (i = 2; i < num_pages*2 + 1; i += 2)
				dma_unmap_page(info->dev,
					info->cq[i].data[CQ_DATA_BLOCK_PTR_REG],
					mtd->writesize, DMA_FROM_DEVICE);
			/* NonPageCache CQ mode implementation call */
			if (0) {
				prep_transfer_dma_cq(info, 1, do_ecc, from,
					datbuf, a_len, info->oob_dma_addr, 0);
				err = cq_run(info, num_pages);
				for (i = 0 ; i < num_pages; i++)
					dma_unmap_page(info->dev,
						info->cq[i].data[CQ_DATA_BLOCK_PTR_REG],
						mtd->writesize, DMA_FROM_DEVICE);

			}
				if (err != 0)
					goto out_err;

#else
			{
				dma_addr_t datbuf_dma_addr = info->dma_addr;

				err = prep_and_transfer_dma_cache(info, 1,
					do_ecc, page, column, datbuf_dma_addr,
					a_len, info->oob_dma_addr, 0, datbuf);
				if (err != 0)
					goto out_err;
			}
#endif
			for (i = 0 ; i < a_len; i += mtd->writesize)
				correct_ecc_errors_on_blank_page(info, datbuf+i,
						NULL, mtd->writesize, 0);
			update_ecc_counts(info, 0);
			len -= a_len;
			datbuf += a_len;
			ops->retlen += a_len;
			from += a_len;
			page_count -= a_len/info->mtd.writesize;
#ifdef CONFIG_MTD_NAND_TEGRA_RW_STATS
			info->rwe_stats.data_read_pages += num_pages;
#endif
		}
		if (!page_count)
			break;
		split_addr(info, from, &chipnr, &page, &column);
		if (chipnr != info->chip.curr_chip)
			select_chip(info, chipnr);
#endif
	}

	disable_ints(info, IER_ECC_ERR);

	if (mtd->ecc_stats.failed != old_ecc_stats.failed)
		err = -EBADMSG;
	else if (mtd->ecc_stats.corrected != old_ecc_stats.corrected)
		err = -EUCLEAN;
	else
		err = 0;

#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
	if (err != 0)
		tegra_nand_invalidate_bufferpool(mtd,
			aligned_address_for_bufferpool,
			aligned_address_for_bufferpool +
			initial_aligned_data_length_to_read);
#endif
	mutex_unlock(&info->lock);
	return err;

out_err:
	ops->retlen = 0;
	ops->oobretlen = 0;

	disable_ints(info, IER_ECC_ERR);
	mutex_unlock(&info->lock);
	return err;
}

/* just does some parameter checking and calls do_read_oob */
static int
tegra_nand_read_oob(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
	if (ops->datbuf && unlikely((from + ops->len) > mtd->size)) {
		pr_err("%s: Can't read past end of device.\n", __func__);
		return -EINVAL;
	}

	if (unlikely(ops->mode != MTD_OOB_AUTO)) {
		if (ops->oobbuf && ops->datbuf) {
			pr_err("%s: can't read OOB + Data in non-AUTO mode.\n",
			       __func__);
			return -EINVAL;
		}
		if ((ops->mode == MTD_OOB_RAW) && !ops->datbuf) {
			pr_err("%s: Raw mode only supports reading data area.\n",
			     __func__);
			return -EINVAL;
		}
	}

	return do_read_oob(mtd, from, ops);
}

static int
tegra_nand_write(struct mtd_info *mtd, loff_t to, size_t len,
		 size_t *retlen, const uint8_t *buf)
{
	struct mtd_oob_ops ops;
	int ret;

	pr_debug("%s: write: to=0x%llx len=0x%x\n", __func__, to, len);
	ops.mode = MTD_OOB_AUTO;
	ops.len = len;
	ops.datbuf = (uint8_t *) buf;
	ops.oobbuf = NULL;
	ret = mtd->write_oob(mtd, to, &ops);
	*retlen = ops.retlen;
	return ret;
}
/*
For writing partial pages it takes buffer from buffer pool and if the
address where data to be written is in buffer pool then it invalidates
that buffer from buffer pool
*/
static int
do_write_oob(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops)
{
	struct tegra_nand_info *info = MTD_TO_INFO(mtd);
	int chipnr;
	uint32_t page;
	uint32_t column;
	uint8_t *datbuf = ops->datbuf;
	uint8_t *oobbuf = ops->oobbuf;
	uint32_t len = datbuf ? ops->len : 0;
	uint32_t ooblen = oobbuf ? ops->ooblen : 0;
	uint32_t oobsz;
	uint32_t page_count;
	int err;
	int do_ecc = 1;
	dma_addr_t datbuf_dma_addr = 0;

#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
	int address_index_in_buffer = -1;
#endif

#if 0
	dump_mtd_oob_ops(ops);
#endif

	if (!oobbuf && !datbuf) {
		pr_err("%s: Invalid write 0 bytes to data and oob\n",
						__func__);
		return -EINVAL;
	}

	if (unlikely(oobbuf && !ooblen)) {
		pr_err("%s: Invalid write 0 bytes to OOB\n", __func__);
		return -EINVAL;
	}

	if (unlikely(datbuf && !len)) {
		pr_err("%s: Invalid write 0 bytes to data\n", __func__);
		return -EINVAL;
	}

	ops->retlen = 0;
	ops->oobretlen = 0;

	oobsz = mtd->oobavail;

	if (unlikely(oobbuf && ooblen > oobsz)) {
		pr_err("%s: can't write OOB to multiple pages (%d > %d)\n",
		       __func__, ooblen, oobsz);
		return -EINVAL;
	} else if (oobbuf && !len) {
		page_count = 1;
	} else
		page_count =
		    max((uint32_t) (len / mtd->writesize), (uint32_t) 1);

	mutex_lock(&info->lock);

	split_addr(info, to, &chipnr, &page, &column);

#ifdef CONFIG_MTD_NAND_TEGRA_RW_STATS
	info->rwe_stats.write_req_bytes += len;
	info->rwe_stats.oobw_req_bytes += ooblen;
#endif
	select_chip(info, chipnr);
	while (page_count--) {
		int a_len = min(mtd->writesize, len);
		int b_len = min(oobsz, ooblen);
		int temp_len = 0;
		char *temp_buf = NULL;

#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
	address_index_in_buffer = lru_implementation(to,
			info->old_addresses, info->buffer_frequency);
#if 0
	print_info(info);
#endif

		/* Take care when write is of less than page size. Otherwise
		 * there will be kernel panic due to dma timeout */
		if (((a_len < mtd->writesize) && len) ||
			(info->old_addresses[address_index_in_buffer] == to)) {
#else
		if ((a_len < mtd->writesize) && len) {
#endif
			temp_len = a_len;
			a_len = mtd->writesize;
			temp_buf = datbuf;

#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
			if (info->old_addresses[address_index_in_buffer] == to)
				datbuf = info->partial_unaligned_rw_buffer+
				((mtd->writesize)*address_index_in_buffer);
			else
				datbuf = info->partial_unaligned_rw_buffer +
				((mtd->writesize)*address_index_in_buffer);

			/* invalidating buffer/marking it unused */
			info->old_addresses[address_index_in_buffer] = to;
			info->buffer_frequency[address_index_in_buffer] = 0;
#else
			datbuf = info->partial_unaligned_rw_buffer;
#endif

			memset(datbuf, 0xff, a_len);
			memcpy(datbuf, temp_buf, temp_len);
		}

		if (datbuf)
			datbuf_dma_addr =
			    tegra_nand_dma_map(info->dev, datbuf, a_len,
					       DMA_TO_DEVICE);
		if (oobbuf)
			memcpy(info->oob_dma_buf, oobbuf, b_len);

		clear_regs(info);
		prep_transfer_dma(info, 0, do_ecc, page, column,
				  datbuf_dma_addr, a_len, info->oob_dma_addr,
				  b_len);

		writel(info->config_reg, CONFIG_REG);
		writel(info->dmactrl_reg, DMA_MST_CTRL_REG);

		INIT_COMPLETION(info->dma_complete);
		err = tegra_nand_go(info);
		if (err != 0)
			goto out_err;

		if (!wait_for_completion_timeout(&info->dma_complete,
					TIMEOUT)) {
			/* Check the DMA Register and see if we go bit
			 * is clear before declaring it as error. We
			 * might have lost a interrupt here */
			if (readl(DMA_MST_CTRL_REG) & COMMAND_GO) {
				pr_err("%s: dma completion timeout\n",
						__func__);
				dump_nand_regs();
				err = -ETIMEDOUT;
				goto out_err;
			}
		}
		if (temp_len) {
			a_len = temp_len;
			datbuf = temp_buf;
		}

#ifdef CONFIG_MTD_NAND_TEGRA_RW_STATS
		if (len)
			info->rwe_stats.written_pages += 1;
		if (ooblen)
			info->rwe_stats.oob_written_bytes += b_len;
#endif
		if (datbuf) {
			dma_unmap_page(info->dev, datbuf_dma_addr, a_len,
				       DMA_TO_DEVICE);
			len -= a_len;
			datbuf += a_len;
			ops->retlen += a_len;
		}
		if (oobbuf) {
			ooblen -= b_len;
			oobbuf += b_len;
			ops->oobretlen += b_len;
		}

		if (!page_count)
			break;

		to += mtd->writesize;
		column = 0;

		split_addr(info, to, &chipnr, &page, &column);
		if (chipnr != info->chip.curr_chip)
			select_chip(info, chipnr);
	}

	mutex_unlock(&info->lock);
	return err;

out_err:
	ops->retlen = 0;
	ops->oobretlen = 0;

	mutex_unlock(&info->lock);
	return err;
}

static int
tegra_nand_write_oob(struct mtd_info *mtd, loff_t to, struct mtd_oob_ops *ops)
{
	struct tegra_nand_info *info = MTD_TO_INFO(mtd);

	if (unlikely(to & info->chip.column_mask)) {
		pr_err("%s: Unaligned write (to 0x%llx) not supported\n",
		       __func__, to);
		return -EINVAL;
	}


	return do_write_oob(mtd, to, ops);
}

static int tegra_nand_suspend(struct mtd_info *mtd)
{
	return 0;
}

static void
set_chip_timing(struct tegra_nand_info *info, uint32_t vendor_id,
		uint32_t dev_id, uint32_t fourth_id_field)
{
	struct tegra_nand_chip_parms *chip_parms = NULL;
	uint32_t tmp;
	int i = 0;
	unsigned long nand_clk_freq_khz = clk_get_rate(info->clk) / 1000;
	for (i = 0; i < info->plat->nr_chip_parms; i++)
		if (info->plat->chip_parms[i].vendor_id == vendor_id &&
		    info->plat->chip_parms[i].device_id == dev_id &&
		    info->plat->chip_parms[i].read_id_fourth_byte ==
		    fourth_id_field)
			chip_parms = &info->plat->chip_parms[i];

	if (!chip_parms) {
		pr_warn("WARNING:tegra_nand: timing for vendor-id: "
			"%x device-id: %x fourth-id-field: %x not found. Using Bootloader timing",
			vendor_id, dev_id, fourth_id_field);
		return;
	}
	/* TODO: Handle the change of frequency if DVFS is enabled */
#define CNT(t)	(((((t) * nand_clk_freq_khz) + 1000000 - 1) / 1000000) - 1)
	tmp = (TIMING_TRP_RESP(CNT(chip_parms->timing.trp_resp)) |
	       TIMING_TWB(CNT(chip_parms->timing.twb)) |
	       TIMING_TCR_TAR_TRR(CNT(chip_parms->timing.tcr_tar_trr)) |
	       TIMING_TWHR(CNT(chip_parms->timing.twhr)) |
	       TIMING_TCS(CNT(chip_parms->timing.tcs)) |
	       TIMING_TWH(CNT(chip_parms->timing.twh)) |
	       TIMING_TWP(CNT(chip_parms->timing.twp)) |
	       TIMING_TRH(CNT(chip_parms->timing.trh)) |
	       TIMING_TRP(CNT(chip_parms->timing.trp)));
	writel(tmp, TIMING_REG);
	writel(TIMING2_TADL(CNT(chip_parms->timing.tadl)), TIMING2_REG);
#undef CNT
}

static void tegra_nand_resume(struct mtd_info *mtd)
{
	struct tegra_nand_info *info = MTD_TO_INFO(mtd);

	cfg_hwstatus_mon(info);

	/* clear all pending interrupts */
	writel(readl(ISR_REG), ISR_REG);

	/* clear dma interrupt */
	writel(DMA_CTRL_IS_DMA_DONE, DMA_MST_CTRL_REG);

	/* enable interrupts */
	disable_ints(info, 0xffffffff);
	enable_ints(info,
		    IER_ERR_TRIG_VAL(4) | IER_UND | IER_OVR | IER_CMD_DONE |
		    IER_ECC_ERR | IER_GIE);

	writel(0, CONFIG_REG);

	set_chip_timing(info, info->vendor_id,
				info->device_id, info->dev_parms);

	return;
}

static int scan_bad_blocks(struct tegra_nand_info *info)
{
#ifndef CONFIG_MTD_DEFER_BLOCK_SCAN
	struct mtd_info *mtd = &info->mtd;
	int num_blocks = mtd->size >> info->chip.block_shift;
	uint32_t block;
	int is_bad = 0;
	info->num_bad_blocks = 0;

	for (block = 0; block < num_blocks; ++block) {
		/* make sure the bit is cleared, meaning it's bad/unknown before
		 * we check. */
		clear_bit(block, info->bb_bitmap);
		is_bad = mtd->block_isbad(mtd, block << info->chip.block_shift);

		if (is_bad == 0)
			set_bit(block, info->bb_bitmap);
		else if (is_bad > 0) {
			info->num_bad_blocks++;
			pr_debug("block 0x%08x is bad.\n", block);
		} else {
			pr_err("Fatal error (%d) while scanning for "
			       "bad blocks\n", is_bad);
			return is_bad;
		}
	}
#endif
	return 0;
}

/* Scans for nand flash devices, identifies them, and fills in the
 * device info. */
static int tegra_nand_scan(struct mtd_info *mtd, int maxchips)
{
	struct tegra_nand_info *info = MTD_TO_INFO(mtd);
	struct nand_flash_dev *dev_info;
	struct nand_manufacturers *vendor_info;
	uint32_t tmp;
	uint32_t dev_id;
	uint32_t vendor_id;
	uint32_t dev_parms;
	uint32_t mlc_parms;
	int cnt;
	int err = 0;

	writel(SCAN_TIMING_VAL, TIMING_REG);
	writel(SCAN_TIMING2_VAL, TIMING2_REG);
	writel(0, CONFIG_REG);

	select_chip(info, 0);
	err = tegra_nand_cmd_readid(info, &tmp);
	if (err != 0)
		goto out_error;

	vendor_id = tmp & 0xff;
	dev_id = (tmp >> 8) & 0xff;
	mlc_parms = (tmp >> 16) & 0xff;
	dev_parms = (tmp >> 24) & 0xff;

	dev_info = find_nand_flash_device(dev_id);
	if (dev_info == NULL) {
		pr_err("%s: unknown flash device id (0x%02x) found.\n",
		       __func__, dev_id);
		err = -ENODEV;
		goto out_error;
	}

	vendor_info = find_nand_flash_vendor(vendor_id);
	if (vendor_info == NULL) {
		pr_err("%s: unknown flash vendor id (0x%02x) found.\n",
		       __func__, vendor_id);
		err = -ENODEV;
		goto out_error;
	}

	/* loop through and see if we can find more devices */
	for (cnt = 1; cnt < info->plat->max_chips; ++cnt) {
		select_chip(info, cnt);
		/* TODO: figure out what to do about errors here */
		err = tegra_nand_cmd_readid(info, &tmp);
		if (err != 0)
			goto out_error;
		if ((dev_id != ((tmp >> 8) & 0xff)) ||
		    (vendor_id != (tmp & 0xff)))
			break;
	}

	pr_info("%s: %d NAND chip(s) found (vend=0x%02x, dev=0x%02x) (%s %s)\n",
		DRIVER_NAME, cnt, vendor_id, dev_id, vendor_info->name,
		dev_info->name);
	info->vendor_id = vendor_id;
	info->device_id = dev_id;
	info->dev_parms = dev_parms;
	info->chip.num_chips = cnt;
	info->chip.chipsize = dev_info->chipsize << 20;
	mtd->size = info->chip.num_chips * info->chip.chipsize;

	/* format of 4th id byte returned by READ ID
	 *   bit     7 = rsvd
	 *   bit     6 = bus width. 1 == 16bit, 0 == 8bit
	 *   bits  5:4 = data block size. 64kb * (2^val)
	 *   bit     3 = rsvd
	 *   bit     2 = spare area size / 512 bytes. 0 == 8bytes, 1 == 16bytes
	 *   bits  1:0 = page size. 1kb * (2^val)
	 */

	/* page_size */
	tmp = dev_parms & 0x3;
	mtd->writesize = 1024 << tmp;
	mtd->writebufsize = mtd->writesize;
#if defined(CONFIG_MTD_NAND_TEGRA_PAGE_CACHE) ||\
	defined(CONFIG_MTD_NAND_TEGRA_CQ_MODE)
	mtd->writebufsize = LOGICAL_PAGES_COMBINE * mtd->writesize;
#endif
	info->chip.column_mask = mtd->writesize - 1;

	if (mtd->writesize > 4096) {
		pr_err("%s: Large page devices with pagesize > 4kb are NOT "
		       "supported\n", __func__);
		goto out_error;
	} else if (mtd->writesize < 2048) {
		pr_err("%s: Small page devices are NOT supported\n", __func__);
		goto out_error;
	}

	/* spare area, must be at least 64 bytes */
	tmp = (dev_parms >> 2) & 0x1;
	tmp = (8 << tmp) * (mtd->writesize / 512);
	if (tmp < 64) {
		pr_err("%s: Spare area (%d bytes) too small\n", __func__, tmp);
		goto out_error;
	}
	mtd->oobsize = tmp;

	/* data block size (erase size) (w/o spare) */
	tmp = (dev_parms >> 4) & 0x3;
	mtd->erasesize = (64 * 1024) << tmp;
	info->chip.block_shift = ffs(mtd->erasesize) - 1;
	/* bus width of the nand chip 8/16 */
	tmp = (dev_parms >> 6) & 0x1;
	info->is_data_bus_width_16 = tmp;
	/* used to select the appropriate chip/page in case multiple devices
	 * are connected */
	info->chip.chip_shift = ffs(info->chip.chipsize) - 1;
	info->chip.page_shift = ffs(mtd->writesize) - 1;
	info->chip.page_mask =
	    (info->chip.chipsize >> info->chip.page_shift) - 1;

	/* now fill in the rest of the mtd fields */
	if (mtd->oobsize == 64)
		mtd->ecclayout = &tegra_nand_oob_64;
	else
		mtd->ecclayout = &tegra_nand_oob_128;

	mtd->oobavail = mtd->ecclayout->oobavail;
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;

	mtd->erase = tegra_nand_erase;
	mtd->lock = NULL;
	mtd->point = NULL;
	mtd->unpoint = NULL;
	mtd->read = tegra_nand_read;
	mtd->write = tegra_nand_write;
	mtd->read_oob = tegra_nand_read_oob;
	mtd->write_oob = tegra_nand_write_oob;

	mtd->resume = tegra_nand_resume;
	mtd->suspend = tegra_nand_suspend;
	mtd->block_isbad = tegra_nand_block_isbad;
	mtd->block_markbad = tegra_nand_block_markbad;

	set_chip_timing(info, vendor_id, dev_id, dev_parms);

	return 0;

out_error:
	pr_err("%s: NAND device scan aborted due to error(s).\n", __func__);
	return err;
}

static int __devinit tegra_nand_probe(struct platform_device *pdev)
{
	struct tegra_nand_platform *plat = pdev->dev.platform_data;
	struct tegra_nand_info *info = NULL;
	struct tegra_nand_chip *chip = NULL;
	struct mtd_info *mtd = NULL;
	int err = 0;
	uint64_t num_erase_blocks;

	pr_debug("%s: probing (%p)\n", __func__, pdev);

	if (!plat) {
		pr_err("%s: no platform device info\n", __func__);
		return -EINVAL;
	} else if (!plat->chip_parms) {
		pr_err("%s: no platform nand parms\n", __func__);
		return -EINVAL;
	}

	info = kzalloc(sizeof(struct tegra_nand_info), GFP_KERNEL);
	if (!info) {
		pr_err("%s: no memory for flash info\n", __func__);
		return -ENOMEM;
	}

	info->dev = &pdev->dev;
	info->plat = plat;

	platform_set_drvdata(pdev, info);

	init_completion(&info->cmd_complete);
	init_completion(&info->dma_complete);
#ifdef CONFIG_MTD_NAND_TEGRA_CQ_MODE
	init_completion(&info->cq_complete);
#endif

	mutex_init(&info->lock);
	spin_lock_init(&info->ecc_lock);

	chip = &info->chip;
	chip->priv = &info->mtd;
	chip->curr_chip = -1;

	mtd = &info->mtd;
	mtd->name = dev_name(&pdev->dev);
	mtd->priv = &info->chip;
	mtd->owner = THIS_MODULE;

	/* HACK: allocate a dma buffer to hold 1 page oob data */
	info->oob_dma_buf = dma_alloc_coherent(NULL, 128,
					       &info->oob_dma_addr, GFP_KERNEL);
	if (!info->oob_dma_buf) {
		err = -ENOMEM;
		goto out_free_info;
	}

	/* this will store the ecc error vector info */
	info->ecc_buf = dma_alloc_coherent(NULL, ECC_BUF_SZ, &info->ecc_addr,
					   GFP_KERNEL);
	if (!info->ecc_buf) {
		err = -ENOMEM;
		goto out_free_dma_buf;
	}

	/* grab the irq */
	if (!(pdev->resource[0].flags & IORESOURCE_IRQ)) {
		pr_err("NAND IRQ resource not defined\n");
		err = -EINVAL;
		goto out_free_ecc_buf;
	}

	err = request_irq(pdev->resource[0].start, tegra_nand_irq,
			  IRQF_DISABLED, DRIVER_NAME, info);
	if (err) {
		pr_err("Unable to request IRQ %d (%d)\n",
		       pdev->resource[0].start, err);
		goto out_free_ecc_buf;
	}

	info->clk = clk_get(&pdev->dev, NULL);

	if (IS_ERR(info->clk)) {
		err = PTR_ERR(info->clk);
		goto out_free_ecc_buf;
	}
	err = clk_enable(info->clk);
	if (err != 0)
		goto out_free_ecc_buf;

	if (plat->wp_gpio) {
		gpio_request(plat->wp_gpio, "nand_wp");
		tegra_gpio_enable(plat->wp_gpio);
		gpio_direction_output(plat->wp_gpio, 1);
	}

	cfg_hwstatus_mon(info);

	/* clear all pending interrupts */
	writel(readl(ISR_REG), ISR_REG);

	/* clear dma interrupt */
	writel(DMA_CTRL_IS_DMA_DONE, DMA_MST_CTRL_REG);

	/* enable interrupts */
	disable_ints(info, 0xffffffff);
	enable_ints(info,
		    IER_ERR_TRIG_VAL(4) | IER_UND | IER_OVR | IER_CMD_DONE |
		    IER_ECC_ERR | IER_GIE);

	if (tegra_nand_scan(mtd, plat->max_chips)) {
		err = -ENXIO;
		goto out_dis_irq;
	}
	pr_info("%s: NVIDIA Tegra NAND controller @ base=0x%08x irq=%d.\n",
		DRIVER_NAME, TEGRA_NAND_PHYS, pdev->resource[0].start);

#ifdef CONFIG_MTD_NAND_TEGRA_PAGE_CACHE
	info->dma_buf = kmalloc(mtd->writebufsize,
					   GFP_KERNEL);
	if (!info->dma_buf) {
		err = -ENOMEM;
		goto out_dis_irq;
	}
	info->dma_addr = tegra_nand_dma_map(info->dev, info->dma_buf,
			mtd->writebufsize, DMA_TO_DEVICE);
#endif
	/* allocate memory to hold the ecc error info */
	info->max_ecc_errs = MAX_DMA_SZ / mtd->writesize;
	info->ecc_errs = kmalloc(info->max_ecc_errs * sizeof(uint32_t),
				 GFP_KERNEL);
	if (!info->ecc_errs) {
		err = -ENOMEM;
		goto out_dis_irq;
	}

	/* alloc the bad block bitmap */
	num_erase_blocks = mtd->size;
	do_div(num_erase_blocks, mtd->erasesize);
	info->bb_bitmap = kzalloc(BITS_TO_LONGS(num_erase_blocks) *
				  sizeof(unsigned long), GFP_KERNEL);
	if (!info->bb_bitmap) {
		err = -ENOMEM;
		goto out_free_ecc;
	}

	err = scan_bad_blocks(info);
	if (err != 0)
		goto out_free_bbbmap;

#if 0
	dump_nand_regs();
#endif

	err = parse_mtd_partitions(mtd, part_probes, &info->parts, 0);
	if (err > 0)
		err = mtd_device_register(mtd, info->parts, err);
	else if (err <= 0 && plat->parts)
		err = mtd_device_register(mtd, plat->parts, plat->nr_parts);
	else
		err = mtd_device_register(mtd, NULL, 0);
	if (err != 0)
		goto out_free_bbbmap;

	dev_set_drvdata(&pdev->dev, info);

#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
	info->partial_unaligned_rw_buffer = kmalloc((mtd->writesize)*BUFFER_NUM,
			GFP_KERNEL);
#else
	info->partial_unaligned_rw_buffer = kzalloc((mtd->writesize),
			GFP_KERNEL);
#endif

	if (!info->partial_unaligned_rw_buffer) {
		err = -ENOMEM;
		goto out_free_bbbmap;
	}

#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
	/* kzalloc is done so that it initializes the buffers
	 * with unused flag(0) */
	info->buffer_frequency = kzalloc(sizeof(int) * BUFFER_NUM, GFP_KERNEL);
	if (!info->buffer_frequency) {
		err = -ENOMEM;
		goto out_free_rw_buffer;
	}

	info->old_addresses = kmalloc(sizeof(long long) * BUFFER_NUM,
			GFP_KERNEL);
	if (!info->old_addresses) {
		err = -ENOMEM;
		goto out_free_buff_freq;
	}
#endif

	err = device_create_file(&pdev->dev, &dev_attr_device_id);
	if (err != 0)
#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
		goto out_free_old_addr;
#else
		goto out_free_rw_buffer;
#endif

	err = device_create_file(&pdev->dev, &dev_attr_vendor_id);
	if (err != 0)
		goto err_nand_sysfs_vendorid_failed;

	err = device_create_file(&pdev->dev, &dev_attr_flash_size);
	if (err != 0)
		goto err_nand_sysfs_flash_size_failed;

	err = device_create_file(&pdev->dev, &dev_attr_num_bad_blocks);
	if (err != 0)
		goto err_nand_sysfs_num_bad_blocks_failed;

	err = device_create_file(&pdev->dev, &dev_attr_bb_bitmap);
	if (err != 0)
		goto err_nand_sysfs_bb_bitmap_failed;

#ifdef CONFIG_MTD_NAND_TEGRA_RW_STATS
	err = device_create_file(&pdev->dev, &dev_attr_rw_stats);
	if (err != 0)
		goto err_nand_sysfs_rw_stats_failed;
#endif

	pr_debug("%s: probe done.\n", __func__);
	return 0;

#ifdef CONFIG_MTD_NAND_TEGRA_RW_STATS
err_nand_sysfs_rw_stats_failed:
	device_remove_file(&pdev->dev, &dev_attr_bb_bitmap);
#endif

err_nand_sysfs_bb_bitmap_failed:
	device_remove_file(&pdev->dev, &dev_attr_num_bad_blocks);

err_nand_sysfs_num_bad_blocks_failed:
	device_remove_file(&pdev->dev, &dev_attr_flash_size);

err_nand_sysfs_flash_size_failed:
	device_remove_file(&pdev->dev, &dev_attr_vendor_id);

err_nand_sysfs_vendorid_failed:
	device_remove_file(&pdev->dev, &dev_attr_device_id);

#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
out_free_old_addr:
	kfree(info->old_addresses);

out_free_buff_freq:
	kfree(info->buffer_frequency);
#endif

out_free_rw_buffer:
	kfree(info->partial_unaligned_rw_buffer);

out_free_bbbmap:
	kfree(info->bb_bitmap);

out_free_ecc:
	kfree(info->ecc_errs);

out_dis_irq:
	disable_ints(info, 0xffffffff);
	free_irq(pdev->resource[0].start, info);

out_free_ecc_buf:
	dma_free_coherent(NULL, ECC_BUF_SZ, info->ecc_buf, info->ecc_addr);

out_free_dma_buf:
	dma_free_coherent(NULL, 128, info->oob_dma_buf, info->oob_dma_addr);
#ifdef CONFIG_MTD_NAND_TEGRA_PAGE_CACHE
	kfree(info->dma_buf);
#endif
out_free_info:
	platform_set_drvdata(pdev, NULL);
	kfree(info);

	return err;
}

static int __devexit tegra_nand_remove(struct platform_device *pdev)
{
	struct tegra_nand_info *info = dev_get_drvdata(&pdev->dev);

	dev_set_drvdata(&pdev->dev, NULL);

	if (info) {
		free_irq(pdev->resource[0].start, info);
		kfree(info->bb_bitmap);
		kfree(info->ecc_errs);
		kfree(info->partial_unaligned_rw_buffer);

		device_remove_file(&pdev->dev, &dev_attr_device_id);
		device_remove_file(&pdev->dev, &dev_attr_vendor_id);
		device_remove_file(&pdev->dev, &dev_attr_flash_size);
		device_remove_file(&pdev->dev, &dev_attr_num_bad_blocks);
		device_remove_file(&pdev->dev, &dev_attr_bb_bitmap);
#ifdef CONFIG_MTD_NAND_TEGRA_RW_STATS
		device_remove_file(&pdev->dev, &dev_attr_rw_stats);
#endif

#ifdef CONFIG_MTD_NAND_TEGRA_BUFFER_POOL
		kfree(info->buffer_frequency);
		kfree(info->old_addresses);
#endif

		dma_free_coherent(NULL, ECC_BUF_SZ, info->ecc_buf,
				info->ecc_addr);
		dma_free_coherent(NULL, info->mtd.writesize + info->mtd.oobsize,
				  info->oob_dma_buf, info->oob_dma_addr);
#ifdef CONFIG_MTD_NAND_TEGRA_PAGE_CACHE
		kfree(info->dma_buf);
#endif
		kfree(info);
	}

	return 0;
}

static struct platform_driver tegra_nand_driver = {
	.probe = tegra_nand_probe,
	.remove = __devexit_p(tegra_nand_remove),
	.suspend = NULL,
	.resume = NULL,
	.driver = {
		   .name = "tegra_nand",
		   .owner = THIS_MODULE,
		   },
};

static int __init tegra_nand_init(void)
{
	return platform_driver_register(&tegra_nand_driver);
}

static void __exit tegra_nand_exit(void)
{
	platform_driver_unregister(&tegra_nand_driver);
}

module_init(tegra_nand_init);
module_exit(tegra_nand_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_DESC);
