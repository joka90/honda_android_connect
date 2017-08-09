/*
 * tegra20_i2s.h - Definitions for Tegra20 I2S driver
 *
 * Author: Stephen Warren <swarren@nvidia.com>
 * Copyright (c) 2010-2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * Based on code copyright/by:
 *
 * Copyright (c) 2009-2012, NVIDIA CORPORATION.  All rights reserved.
 * Scott Peterson <speterson@nvidia.com>
 *
 * Copyright (C) 2010 Google, Inc.
 * Iliyan Malchev <malchev@google.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef __TEGRA20_I2S_H__
#define __TEGRA20_I2S_H__

#include "tegra_pcm.h"

/* Register offsets from TEGRA20_I2S1_BASE and TEGRA20_I2S2_BASE */

#define TEGRA20_I2S_CTRL				0x00
#define TEGRA20_I2S_STATUS				0x04
#define TEGRA20_I2S_TIMING				0x08
#define TEGRA20_I2S_FIFO_SCR				0x0c
#define TEGRA20_I2S_PCM_CTRL				0x10
#define TEGRA20_I2S_NW_CTRL				0x14
#define TEGRA20_I2S_TDM_CTRL				0x20
#define TEGRA20_I2S_TDM_TX_RX_CTRL			0x24
#define TEGRA20_I2S_FIFO1				0x40
#define TEGRA20_I2S_FIFO2				0x80

/* Fields in TEGRA20_I2S_CTRL */

#define TEGRA20_I2S_CTRL_FIFO2_TX_ENABLE		(1 << 30)
#define TEGRA20_I2S_CTRL_FIFO1_ENABLE			(1 << 29)
#define TEGRA20_I2S_CTRL_FIFO2_ENABLE			(1 << 28)
#define TEGRA20_I2S_CTRL_FIFO1_RX_ENABLE		(1 << 27)
#define TEGRA20_I2S_CTRL_FIFO_LPBK_ENABLE		(1 << 26)
#define TEGRA20_I2S_CTRL_MASTER_ENABLE			(1 << 25)

#define TEGRA20_I2S_LRCK_LEFT_LOW			0
#define TEGRA20_I2S_LRCK_RIGHT_LOW			1

#define TEGRA20_I2S_CTRL_LRCK_SHIFT			24
#define TEGRA20_I2S_CTRL_LRCK_MASK			(1                          << TEGRA20_I2S_CTRL_LRCK_SHIFT)
#define TEGRA20_I2S_CTRL_LRCK_L_LOW			(TEGRA20_I2S_LRCK_LEFT_LOW  << TEGRA20_I2S_CTRL_LRCK_SHIFT)
#define TEGRA20_I2S_CTRL_LRCK_R_LOW			(TEGRA20_I2S_LRCK_RIGHT_LOW << TEGRA20_I2S_CTRL_LRCK_SHIFT)

#define TEGRA20_I2S_BIT_FORMAT_I2S			0
#define TEGRA20_I2S_BIT_FORMAT_RJM			1
#define TEGRA20_I2S_BIT_FORMAT_LJM			2
#define TEGRA20_I2S_BIT_FORMAT_DSP			3

#define TEGRA20_I2S_CTRL_BIT_FORMAT_SHIFT		10
#define TEGRA20_I2S_CTRL_BIT_FORMAT_MASK		(3                          << TEGRA20_I2S_CTRL_BIT_FORMAT_SHIFT)
#define TEGRA20_I2S_CTRL_BIT_FORMAT_I2S			(TEGRA20_I2S_BIT_FORMAT_I2S << TEGRA20_I2S_CTRL_BIT_FORMAT_SHIFT)
#define TEGRA20_I2S_CTRL_BIT_FORMAT_RJM			(TEGRA20_I2S_BIT_FORMAT_RJM << TEGRA20_I2S_CTRL_BIT_FORMAT_SHIFT)
#define TEGRA20_I2S_CTRL_BIT_FORMAT_LJM			(TEGRA20_I2S_BIT_FORMAT_LJM << TEGRA20_I2S_CTRL_BIT_FORMAT_SHIFT)
#define TEGRA20_I2S_CTRL_BIT_FORMAT_DSP			(TEGRA20_I2S_BIT_FORMAT_DSP << TEGRA20_I2S_CTRL_BIT_FORMAT_SHIFT)

#define TEGRA20_I2S_BIT_SIZE_16				0
#define TEGRA20_I2S_BIT_SIZE_20				1
#define TEGRA20_I2S_BIT_SIZE_24				2
#define TEGRA20_I2S_BIT_SIZE_32				3

#define TEGRA20_I2S_CTRL_BIT_SIZE_SHIFT			8
#define TEGRA20_I2S_CTRL_BIT_SIZE_MASK			(3                       << TEGRA20_I2S_CTRL_BIT_SIZE_SHIFT)
#define TEGRA20_I2S_CTRL_BIT_SIZE_16			(TEGRA20_I2S_BIT_SIZE_16 << TEGRA20_I2S_CTRL_BIT_SIZE_SHIFT)
#define TEGRA20_I2S_CTRL_BIT_SIZE_20			(TEGRA20_I2S_BIT_SIZE_20 << TEGRA20_I2S_CTRL_BIT_SIZE_SHIFT)
#define TEGRA20_I2S_CTRL_BIT_SIZE_24			(TEGRA20_I2S_BIT_SIZE_24 << TEGRA20_I2S_CTRL_BIT_SIZE_SHIFT)
#define TEGRA20_I2S_CTRL_BIT_SIZE_32			(TEGRA20_I2S_BIT_SIZE_32 << TEGRA20_I2S_CTRL_BIT_SIZE_SHIFT)

#define TEGRA20_I2S_FIFO_16_LSB				0
#define TEGRA20_I2S_FIFO_20_LSB				1
#define TEGRA20_I2S_FIFO_24_LSB				2
#define TEGRA20_I2S_FIFO_32				3
#define TEGRA20_I2S_FIFO_PACKED				7

#define TEGRA20_I2S_CTRL_FIFO_FORMAT_SHIFT		4
#define TEGRA20_I2S_CTRL_FIFO_FORMAT_MASK		(7                       << TEGRA20_I2S_CTRL_FIFO_FORMAT_SHIFT)
#define TEGRA20_I2S_CTRL_FIFO_FORMAT_16_LSB		(TEGRA20_I2S_FIFO_16_LSB << TEGRA20_I2S_CTRL_FIFO_FORMAT_SHIFT)
#define TEGRA20_I2S_CTRL_FIFO_FORMAT_20_LSB		(TEGRA20_I2S_FIFO_20_LSB << TEGRA20_I2S_CTRL_FIFO_FORMAT_SHIFT)
#define TEGRA20_I2S_CTRL_FIFO_FORMAT_24_LSB		(TEGRA20_I2S_FIFO_24_LSB << TEGRA20_I2S_CTRL_FIFO_FORMAT_SHIFT)
#define TEGRA20_I2S_CTRL_FIFO_FORMAT_32			(TEGRA20_I2S_FIFO_32     << TEGRA20_I2S_CTRL_FIFO_FORMAT_SHIFT)
#define TEGRA20_I2S_CTRL_FIFO_FORMAT_PACKED		(TEGRA20_I2S_FIFO_PACKED << TEGRA20_I2S_CTRL_FIFO_FORMAT_SHIFT)

#define TEGRA20_I2S_CTRL_IE_FIFO1_ERR			(1 << 3)
#define TEGRA20_I2S_CTRL_IE_FIFO2_ERR			(1 << 2)
#define TEGRA20_I2S_CTRL_QE_FIFO1			(1 << 1)
#define TEGRA20_I2S_CTRL_QE_FIFO2			(1 << 0)

/* Fields in TEGRA20_I2S_STATUS */

#define TEGRA20_I2S_STATUS_FIFO1_RDY			(1 << 31)
#define TEGRA20_I2S_STATUS_FIFO2_RDY			(1 << 30)
#define TEGRA20_I2S_STATUS_FIFO1_BSY			(1 << 29)
#define TEGRA20_I2S_STATUS_FIFO2_BSY			(1 << 28)
#define TEGRA20_I2S_STATUS_FIFO1_ERR			(1 << 3)
#define TEGRA20_I2S_STATUS_FIFO2_ERR			(1 << 2)
#define TEGRA20_I2S_STATUS_QS_FIFO1			(1 << 1)
#define TEGRA20_I2S_STATUS_QS_FIFO2			(1 << 0)

/* Fields in TEGRA20_I2S_TIMING */

#define TEGRA20_I2S_TIMING_NON_SYM_ENABLE		(1 << 12)
#define TEGRA20_I2S_TIMING_CHANNEL_BIT_COUNT_SHIFT	0
#define TEGRA20_I2S_TIMING_CHANNEL_BIT_COUNT_MASK_US	0x7fff
#define TEGRA20_I2S_TIMING_CHANNEL_BIT_COUNT_MASK	(TEGRA20_I2S_TIMING_CHANNEL_BIT_COUNT_MASK_US << TEGRA20_I2S_TIMING_CHANNEL_BIT_COUNT_SHIFT)

/* Fields in TEGRA20_I2S_FIFO_SCR */

#define TEGRA20_I2S_FIFO_SCR_FIFO2_FULL_EMPTY_COUNT_SHIFT	24
#define TEGRA20_I2S_FIFO_SCR_FIFO1_FULL_EMPTY_COUNT_SHIFT	16
#define TEGRA20_I2S_FIFO_SCR_FIFO_FULL_EMPTY_COUNT_MASK		0x3f

#define TEGRA20_I2S_FIFO_SCR_FIFO2_CLR			(1 << 12)
#define TEGRA20_I2S_FIFO_SCR_FIFO1_CLR			(1 << 8)

#define TEGRA20_I2S_FIFO_ATN_LVL_ONE_SLOT		0
#define TEGRA20_I2S_FIFO_ATN_LVL_FOUR_SLOTS		1
#define TEGRA20_I2S_FIFO_ATN_LVL_EIGHT_SLOTS		2
#define TEGRA20_I2S_FIFO_ATN_LVL_TWELVE_SLOTS		3

#define TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_SHIFT	4
#define TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_MASK		(3                                     << TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_SHIFT)
#define TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_ONE_SLOT	(TEGRA20_I2S_FIFO_ATN_LVL_ONE_SLOT     << TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_SHIFT)
#define TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_FOUR_SLOTS	(TEGRA20_I2S_FIFO_ATN_LVL_FOUR_SLOTS   << TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_SHIFT)
#define TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_EIGHT_SLOTS	(TEGRA20_I2S_FIFO_ATN_LVL_EIGHT_SLOTS  << TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_SHIFT)
#define TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_TWELVE_SLOTS	(TEGRA20_I2S_FIFO_ATN_LVL_TWELVE_SLOTS << TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_SHIFT)

#define TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_SHIFT	0
#define TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_MASK		(3                                     << TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_SHIFT)
#define TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_ONE_SLOT	(TEGRA20_I2S_FIFO_ATN_LVL_ONE_SLOT     << TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_SHIFT)
#define TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_FOUR_SLOTS	(TEGRA20_I2S_FIFO_ATN_LVL_FOUR_SLOTS   << TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_SHIFT)
#define TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_EIGHT_SLOTS	(TEGRA20_I2S_FIFO_ATN_LVL_EIGHT_SLOTS  << TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_SHIFT)
#define TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_TWELVE_SLOTS	(TEGRA20_I2S_FIFO_ATN_LVL_TWELVE_SLOTS << TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_SHIFT)

/* Fields in TEGRA20_I2S_PCM_CTRL */

#define TEGRA20_I2S_PCM_TX_POS_EDGE_NO_HIGHZ		0
#define TEGRA20_I2S_PCM_TX_POS_EDGE_HIGHZ		1
#define TEGRA20_I2S_PCM_TX_NEG_EDGE_NO_HIGHZ		2
#define TEGRA20_I2S_PCM_TX_NEG_EDGE_HIGHZ		3

#define TEGRA20_I2S_PCM_CTRL_TX_EDGE_CTRL_SHIFT		9
#define TEGRA20_I2S_PCM_CTRL_TX_EDGE_CTRL_MASK		(0x3                                   << TEGRA20_I2S_PCM_CTRL_TX_EDGE_CTRL_SHIFT)
#define TEGRA20_I2S_PCM_CTRL_TX_POS_EDGE_NO_HIGHZ	(TEGRA20_I2S_PCM_TX_POS_EDGE_NO_HIGHZ  << TEGRA20_I2S_PCM_CTRL_TX_EDGE_CTRL_SHIFT)
#define TEGRA20_I2S_PCM_CTRL_TX_POS_EDGE_HIGHZ		(TEGRA20_I2S_PCM_TX_POS_EDGE_HIGHZ     << TEGRA20_I2S_PCM_CTRL_TX_EDGE_CTRL_SHIFT)
#define TEGRA20_I2S_PCM_CTRL_TX_NEG_EDGE_NO_HIGHZ	(TEGRA20_I2S_PCM_TX_NEG_EDGE_NO_HIGHZ  << TEGRA20_I2S_PCM_CTRL_TX_EDGE_CTRL_SHIFT)
#define TEGRA20_I2S_PCM_CTRL_TX_NEG_EDGE_HIGHZ		(TEGRA20_I2S_PCM_TX_NEG_EDGE_HIGHZ     << TEGRA20_I2S_PCM_CTRL_TX_EDGE_CTRL_SHIFT)

#define TEGRA20_I2S_PCM_CTRL_TX_MASK_BITS_SHIFT		6
#define TEGRA20_I2S_PCM_CTRL_TX_MASK_BITS_MASK		(0x7 << TEGRA20_I2S_PCM_CTRL_TX_MASK_BITS_SHIFT)

#define TEGRA20_I2S_PCM_CTRL_FSYNC_LONG	                (1 << 5)
#define TEGRA20_I2S_PCM_CTRL_TRM_MODE_EN                (1 << 4)

#define TEGRA20_I2S_PCM_CTRL_RX_MASK_BITS_SHIFT		1
#define TEGRA20_I2S_PCM_CTRL_RX_MASK_BITS_MASK		(0x7 << TEGRA20_I2S_PCM_CTRL_RX_MASK_BITS_SHIFT)

#define TEGRA20_I2S_PCM_CTRL_RCV_MODE_EN                (1 << 0)

/* Fields in TEGRA20_I2S_TDM_CTRL */
#define TEGRA20_I2S_TDM_CTRL_TDM_EN					(1 << 31)
#define TEGRA20_I2S_TDM_CTRL_TDM_DISABLE			(0 << 31)

#define TEGRA20_I2S_TDM_CTRL_TX_MSB_FIRST			(0 << 25)
#define TEGRA20_I2S_TDM_CTRL_TX_LSB_FIRST			(1 << 25)

#define TEGRA20_I2S_TDM_CTRL_RX_MSB_FIRST			(0 << 24)
#define TEGRA20_I2S_TDM_CTRL_RX_LSB_FIRST			(1 << 24)

#define TEGRA20_I2S_TDM_CTRL_NO_HIGHZ				(0 << 22)
#define TEGRA20_I2S_TDM_CTRL_HIGHZ					(1 << 22)

#define TEGRA20_I2S_TDM_CTRL_TOTAL_SLOTS_SHIFT		18
#define TEGRA20_I2S_TDM_CTRL_TOTAL_SLOTS_MASK		(0x7 << TEGRA20_I2S_TDM_CTRL_TOTAL_SLOTS_SHIFT)
#define TEGRA20_I2S_TDM_CTRL_TOTAL_SLOTS_FOUR		(3 << TEGRA20_I2S_TDM_CTRL_TOTAL_SLOTS_SHIFT)
#define TEGRA20_I2S_TDM_CTRL_TOTAL_SLOTS_EIGHT		(7 << TEGRA20_I2S_TDM_CTRL_TOTAL_SLOTS_SHIFT)

#define TEGRA20_I2S_TDM_CTRL_TDM_BITSIZE_SHIFT		12
#define TEGRA20_I2S_TDM_CTRL_TDM_BITSIZE_MASK		(0x1F << TEGRA20_I2S_TDM_CTRL_TDM_BITSIZE_SHIFT)

#define TEGRA20_I2S_TDM_CTRL_RX_DATAOFFSET_SHIFT	8
#define TEGRA20_I2S_TDM_CTRL_RX_DATAOFFSET_MASK		(0x3 << TEGRA20_I2S_TDM_CTRL_RX_DATAOFFSET_SHIFT)
#define TEGRA20_I2S_TDM_CTRL_RX_DATAOFFSET_ALIGNED	(0 << TEGRA20_I2S_TDM_CTRL_RX_DATAOFFSET_SHIFT)
#define TEGRA20_I2S_TDM_CTRL_RX_DATAOFFSET_ONE_CLOCK	(1 << TEGRA20_I2S_TDM_CTRL_RX_DATAOFFSET_SHIFT)
#define TEGRA20_I2S_TDM_CTRL_RX_DATAOFFSET_TWO_CLOCK	(2 << TEGRA20_I2S_TDM_CTRL_RX_DATAOFFSET_SHIFT)
#define TEGRA20_I2S_TDM_CTRL_RX_DATAOFFSET_THREE_CLOCK	(3 << TEGRA20_I2S_TDM_CTRL_RX_DATAOFFSET_SHIFT)

#define TEGRA20_I2S_TDM_CTRL_TX_DATAOFFSET_SHIFT	6
#define TEGRA20_I2S_TDM_CTRL_TX_DATAOFFSET_MASK		(0x3 << TEGRA20_I2S_TDM_CTRL_TX_DATAOFFSET_SHIFT)
#define TEGRA20_I2S_TDM_CTRL_TX_DATAOFFSET_ALIGNED	(0 << TEGRA20_I2S_TDM_CTRL_TX_DATAOFFSET_SHIFT)
#define TEGRA20_I2S_TDM_CTRL_TX_DATAOFFSET_ONE_CLOCK	(1 << TEGRA20_I2S_TDM_CTRL_TX_DATAOFFSET_SHIFT)
#define TEGRA20_I2S_TDM_CTRL_TX_DATAOFFSET_TWO_CLOCK	(2 << TEGRA20_I2S_TDM_CTRL_TX_DATAOFFSET_SHIFT)
#define TEGRA20_I2S_TDM_CTRL_TX_DATAOFFSET_THREE_CLOCK	(3 << TEGRA20_I2S_TDM_CTRL_TX_DATAOFFSET_SHIFT)

#define TEGRA20_I2S_TDM_CTRL_FSYNC_WIDTH_SHIFT	0
#define TEGRA20_I2S_TDM_CTRL_FSYNC_WIDTH_MASK	(0x3F << TEGRA20_I2S_TDM_CTRL_FSYNC_WIDTH_SHIFT)

/* Fields in TEGRA20_I2S_TDM_TX_RX_CTRL */
#define TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_TX_ENABLE	(1 << 31)

#define TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_TX_BUSY	(1 << 30)

#define TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_RX_ENABLE	(1 << 29)
#define TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_RX_DISABLE	(0 << 29)

#define TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_RX_BUSY	(1 << 28)

#define TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_RX_SLOT_ENABLES_SHIFT	(8)
#define TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_RX_SLOT_ENABLES_MASK	(0xFF << TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_RX_SLOT_ENABLES_SHIFT)

#define TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_TX_SLOT_ENABLES_SHIFT	(0)
#define TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_TX_SLOT_ENABLES_MASK	(0xFF << TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_TX_SLOT_ENABLES_SHIFT)



#ifdef CONFIG_PM
/* unused cache locations for i2s reg cache */
#define TEGRA20_I2S_CACHE_RSVD_6			((TEGRA20_I2S_NW_CTRL>>2) + 1)
#define TEGRA20_I2S_CACHE_RSVD_7			(TEGRA20_I2S_CACHE_RSVD_6 + 1)
#endif

struct dsp_config_t {
	int num_slots;
	int rx_mask;
	int tx_mask;
	int slot_width;
	int rx_data_offset;
	int tx_data_offset;
};

struct tegra20_i2s {
	struct clk *clk_i2s;
	struct tegra_pcm_dma_params capture_dma_data;
	struct tegra_pcm_dma_params playback_dma_data;
	void __iomem *regs;
	struct dentry *debug;
	u32 reg_ctrl;
	struct dsp_config_t dsp_config;
	unsigned irq;
	const char *name;
#ifdef CONFIG_PM
	u32  reg_cache[(TEGRA20_I2S_TDM_TX_RX_CTRL >> 2) + 1];
#endif
};

#endif
