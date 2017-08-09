/*
 * tegra20_i2s.c - Tegra20 I2S driver
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
#include <asm/mach-types.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <mach/iomap.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "tegra20_das.h"
#include "tegra20_i2s.h"

#define DRV_NAME "tegra20-i2s"

static inline void tegra20_i2s_write(struct tegra20_i2s *i2s, u32 reg, u32 val)
{
#ifdef CONFIG_PM
	i2s->reg_cache[reg >> 2] = val;
#endif
	__raw_writel(val, i2s->regs + reg);
}

static inline u32 tegra20_i2s_read(struct tegra20_i2s *i2s, u32 reg)
{
	return __raw_readl(i2s->regs + reg);
}

#ifdef CONFIG_DEBUG_FS
static int tegra20_i2s_show(struct seq_file *s, void *unused)
{
#define REG(r) { r, #r }
	static const struct {
		int offset;
		const char *name;
	} regs[] = {
		REG(TEGRA20_I2S_CTRL),
		REG(TEGRA20_I2S_STATUS),
		REG(TEGRA20_I2S_TIMING),
		REG(TEGRA20_I2S_FIFO_SCR),
		REG(TEGRA20_I2S_PCM_CTRL),
		REG(TEGRA20_I2S_NW_CTRL),
		REG(TEGRA20_I2S_TDM_CTRL),
		REG(TEGRA20_I2S_TDM_TX_RX_CTRL),
	};
#undef REG

	struct tegra20_i2s *i2s = s->private;
	int i;

	clk_enable(i2s->clk_i2s);

	for (i = 0; i < ARRAY_SIZE(regs); i++) {
		u32 val = tegra20_i2s_read(i2s, regs[i].offset);
		seq_printf(s, "%s = %08x\n", regs[i].name, val);
	}

	clk_disable(i2s->clk_i2s);

	return 0;
}

static int tegra20_i2s_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, tegra20_i2s_show, inode->i_private);
}

static const struct file_operations tegra20_i2s_debug_fops = {
	.open    = tegra20_i2s_debug_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

static void tegra20_i2s_debug_add(struct tegra20_i2s *i2s, int id)
{
	char name[] = DRV_NAME ".0";

	snprintf(name, sizeof(name), DRV_NAME".%1d", id);
	i2s->debug = debugfs_create_file(name, S_IRUGO, snd_soc_debugfs_root,
						i2s, &tegra20_i2s_debug_fops);
}

static void tegra20_i2s_debug_remove(struct tegra20_i2s *i2s)
{
	if (i2s->debug)
		debugfs_remove(i2s->debug);
}
#else
static inline void tegra20_i2s_debug_add(struct tegra20_i2s *i2s, int id)
{
}

static inline void tegra20_i2s_debug_remove(struct tegra20_i2s *i2s)
{
}
#endif

static int tegra20_i2s_set_fmt(struct snd_soc_dai *dai,
				unsigned int fmt)
{
	struct tegra20_i2s *i2s = snd_soc_dai_get_drvdata(dai);

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	default:
		return -EINVAL;
	}

	i2s->reg_ctrl &= ~TEGRA20_I2S_CTRL_MASTER_ENABLE;
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		i2s->reg_ctrl |= TEGRA20_I2S_CTRL_MASTER_ENABLE;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		break;
	default:
		return -EINVAL;
	}

	i2s->reg_ctrl &= ~(TEGRA20_I2S_CTRL_BIT_FORMAT_MASK |
				TEGRA20_I2S_CTRL_LRCK_MASK);
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_A:
		i2s->reg_ctrl |= TEGRA20_I2S_CTRL_BIT_FORMAT_DSP;
		i2s->reg_ctrl |= TEGRA20_I2S_CTRL_LRCK_L_LOW;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		i2s->reg_ctrl |= TEGRA20_I2S_CTRL_BIT_FORMAT_DSP;
		i2s->reg_ctrl |= TEGRA20_I2S_CTRL_LRCK_R_LOW;
		break;
	case SND_SOC_DAIFMT_I2S:
		i2s->reg_ctrl |= TEGRA20_I2S_CTRL_BIT_FORMAT_I2S;
		i2s->reg_ctrl |= TEGRA20_I2S_CTRL_LRCK_L_LOW;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		i2s->reg_ctrl |= TEGRA20_I2S_CTRL_BIT_FORMAT_RJM;
		i2s->reg_ctrl |= TEGRA20_I2S_CTRL_LRCK_L_LOW;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		i2s->reg_ctrl |= TEGRA20_I2S_CTRL_BIT_FORMAT_LJM;
		i2s->reg_ctrl |= TEGRA20_I2S_CTRL_LRCK_L_LOW;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int tegra20_i2s_tdm_tx_fifo_busy(struct tegra20_i2s *i2s)
{
	u32 reg;
	reg = tegra20_i2s_read(i2s, TEGRA20_I2S_TDM_TX_RX_CTRL);
	return reg & TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_TX_BUSY;
}

static int tegra20_i2s_tdm_rx_fifo_busy(struct tegra20_i2s *i2s)
{
	u32 reg;
	reg = tegra20_i2s_read(i2s, TEGRA20_I2S_TDM_TX_RX_CTRL);
	return reg & TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_RX_BUSY;
}

static int tegra20_i2s_tdm_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct device *dev = substream->pcm->card->dev;
	struct tegra20_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	u32 reg;
	int ret, sample_size, srate, i2sclock, bitcnt, i2sclk_div;

	srate = params_rate(params);

	/* Final "* 2" required by Tegra hardware */
	i2sclock = srate * i2s->dsp_config.num_slots
					* i2s->dsp_config.slot_width;
	sample_size = i2s->dsp_config.slot_width;

	ret = clk_set_rate(i2s->clk_i2s, i2sclock);
	if (ret) {
		dev_err(dev, "Can't set I2S clock rate: %d\n", ret);
		return ret;
	}

	i2sclk_div = srate;
	bitcnt = (i2sclock / i2sclk_div) - 1;

	if (bitcnt < 0 || bitcnt > TEGRA20_I2S_TIMING_CHANNEL_BIT_COUNT_MASK_US)
		return -EINVAL;
	reg = bitcnt << TEGRA20_I2S_TIMING_CHANNEL_BIT_COUNT_SHIFT;

	if (i2sclock % i2sclk_div)
		reg |= TEGRA20_I2S_TIMING_NON_SYM_ENABLE;

	clk_enable(i2s->clk_i2s);

	tegra20_i2s_write(i2s, TEGRA20_I2S_TIMING, reg);

	if (sample_size * params_channels(params) <= 32)
		tegra20_i2s_write(i2s, TEGRA20_I2S_FIFO_SCR,
			TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_FOUR_SLOTS |
			TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_FOUR_SLOTS);
	else
		tegra20_i2s_write(i2s, TEGRA20_I2S_FIFO_SCR,
			TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_EIGHT_SLOTS |
			TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_EIGHT_SLOTS);

	/* TDM mode registers */
	reg = tegra20_i2s_read(i2s, TEGRA20_I2S_TDM_CTRL);
	reg |= TEGRA20_I2S_TDM_CTRL_TDM_EN;
	reg |= TEGRA20_I2S_TDM_CTRL_NO_HIGHZ;

	if (i2s->dsp_config.num_slots == 4)
		reg |= TEGRA20_I2S_TDM_CTRL_TOTAL_SLOTS_FOUR;
	else if (i2s->dsp_config.num_slots == 8)
		reg |= TEGRA20_I2S_TDM_CTRL_TOTAL_SLOTS_EIGHT;
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		reg |= TEGRA20_I2S_TDM_CTRL_TX_MSB_FIRST;
		reg |= TEGRA20_I2S_TDM_CTRL_TX_DATAOFFSET_ALIGNED;
		i2s->playback_dma_data.width = sample_size;
	} else {
		reg |= TEGRA20_I2S_TDM_CTRL_RX_MSB_FIRST;
		reg |= TEGRA20_I2S_TDM_CTRL_RX_DATAOFFSET_ALIGNED;
		i2s->capture_dma_data.width = sample_size;
	}
		i2s->reg_ctrl = TEGRA20_I2S_CTRL_BIT_FORMAT_I2S;
	reg |= (((i2s->dsp_config.slot_width - 1) <<
					TEGRA20_I2S_TDM_CTRL_TDM_BITSIZE_SHIFT) &
					TEGRA20_I2S_TDM_CTRL_TDM_BITSIZE_MASK);
	reg |= (((i2s->dsp_config.slot_width - 1) <<
					TEGRA20_I2S_TDM_CTRL_FSYNC_WIDTH_SHIFT) &
					TEGRA20_I2S_TDM_CTRL_FSYNC_WIDTH_MASK);
	tegra20_i2s_write(i2s, TEGRA20_I2S_TDM_CTRL,  reg);

	reg = tegra20_i2s_read(i2s, TEGRA20_I2S_TDM_TX_RX_CTRL);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		reg |= ((i2s->dsp_config.tx_mask <<
				TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_TX_SLOT_ENABLES_SHIFT) &
				TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_TX_SLOT_ENABLES_MASK);
	} else {
		reg |= ((i2s->dsp_config.rx_mask <<
					TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_RX_SLOT_ENABLES_SHIFT) &
					TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_RX_SLOT_ENABLES_MASK);
	}
	tegra20_i2s_write(i2s, TEGRA20_I2S_TDM_TX_RX_CTRL ,  reg);
	clk_disable(i2s->clk_i2s);

	return 0;
}

static int tegra20_i2s_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct device *dev = substream->pcm->card->dev;
	struct tegra20_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	u32 reg;
	int ret, sample_size, srate, i2sclock, bitcnt, i2sclk_div;
	u32 bit_format = i2s->reg_ctrl & TEGRA20_I2S_CTRL_BIT_FORMAT_MASK;

	/* TDM mode */
	if ((i2s->reg_ctrl & TEGRA20_I2S_CTRL_BIT_FORMAT_DSP) &&
		(i2s->dsp_config.slot_width > 2))
		return tegra20_i2s_tdm_hw_params(substream, params, dai);

	if ((bit_format == TEGRA20_I2S_CTRL_BIT_FORMAT_I2S) &&
					(params_channels(params) != 2)) {
		dev_err(dev, "Only Stereo is supported in I2s mode\n");
		return -EINVAL;
	}

	i2s->reg_ctrl &= ~TEGRA20_I2S_CTRL_BIT_SIZE_MASK;
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		i2s->reg_ctrl |= TEGRA20_I2S_CTRL_BIT_SIZE_16;
		sample_size = 16;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		i2s->reg_ctrl |= TEGRA20_I2S_CTRL_BIT_SIZE_24;
		sample_size = 24;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		i2s->reg_ctrl |= TEGRA20_I2S_CTRL_BIT_SIZE_32;
		sample_size = 32;
		break;
	default:
		return -EINVAL;
	}

	srate = params_rate(params);

	/* Final "* 2" required by Tegra hardware */
	i2sclock = srate * params_channels(params) * sample_size * 2;

	/* Additional "* 2" is needed for DSP mode */
	if (i2s->reg_ctrl & TEGRA20_I2S_CTRL_BIT_FORMAT_DSP && !machine_is_whistler())
		i2sclock *= 2;

	ret = clk_set_rate(i2s->clk_i2s, i2sclock);
	if (ret) {
		dev_err(dev, "Can't set I2S clock rate: %d\n", ret);
		return ret;
	}

	if (i2s->reg_ctrl & TEGRA20_I2S_CTRL_BIT_FORMAT_DSP)
		i2sclk_div = srate;
	else
		i2sclk_div = params_channels(params) * srate;

	bitcnt = (i2sclock / i2sclk_div) - 1;

	if (bitcnt < 0 || bitcnt > TEGRA20_I2S_TIMING_CHANNEL_BIT_COUNT_MASK_US)
		return -EINVAL;
	reg = bitcnt << TEGRA20_I2S_TIMING_CHANNEL_BIT_COUNT_SHIFT;

	if (i2sclock % i2sclk_div)
		reg |= TEGRA20_I2S_TIMING_NON_SYM_ENABLE;

	clk_enable(i2s->clk_i2s);

	tegra20_i2s_write(i2s, TEGRA20_I2S_TIMING, reg);

	if (sample_size * params_channels(params) >= 32)
		tegra20_i2s_write(i2s, TEGRA20_I2S_FIFO_SCR,
			TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_FOUR_SLOTS |
			TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_FOUR_SLOTS);
	else
		tegra20_i2s_write(i2s, TEGRA20_I2S_FIFO_SCR,
			TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_EIGHT_SLOTS |
			TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_EIGHT_SLOTS);

	i2s->reg_ctrl &= ~TEGRA20_I2S_CTRL_FIFO_FORMAT_MASK;
	reg = tegra20_i2s_read(i2s, TEGRA20_I2S_PCM_CTRL);
	if (i2s->reg_ctrl & TEGRA20_I2S_CTRL_BIT_FORMAT_DSP) {
		if (sample_size == 16)
			i2s->reg_ctrl |= TEGRA20_I2S_CTRL_FIFO_FORMAT_16_LSB;
		else if (sample_size == 24)
			i2s->reg_ctrl |= TEGRA20_I2S_CTRL_FIFO_FORMAT_24_LSB;
		else
			i2s->reg_ctrl |= TEGRA20_I2S_CTRL_FIFO_FORMAT_32;

		i2s->capture_dma_data.width = sample_size;
		i2s->playback_dma_data.width = sample_size;

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			reg |= TEGRA20_I2S_PCM_CTRL_TRM_MODE_EN;
		else
			reg |= TEGRA20_I2S_PCM_CTRL_RCV_MODE_EN;
	} else {
		i2s->reg_ctrl |= TEGRA20_I2S_CTRL_FIFO_FORMAT_PACKED;
		i2s->capture_dma_data.width = 32;
		i2s->playback_dma_data.width = 32;
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			reg &= ~TEGRA20_I2S_PCM_CTRL_TRM_MODE_EN;
		else
			reg &= ~TEGRA20_I2S_PCM_CTRL_RCV_MODE_EN;
	}
	tegra20_i2s_write(i2s, TEGRA20_I2S_PCM_CTRL, reg);

	clk_disable(i2s->clk_i2s);

	return 0;
}

static void tegra20_i2s_tdm_start_playback(struct tegra20_i2s *i2s)
{
	u32 reg;
	int count = 0;
	int level = 1;
	int dcnt = 10;
	/* Wait for the FIFO1 to start */
	while (dcnt--) {
		reg = tegra20_i2s_read(i2s, TEGRA20_I2S_FIFO_SCR);
		count = reg >> TEGRA20_I2S_FIFO_SCR_FIFO1_FULL_EMPTY_COUNT_SHIFT;
		count = count & TEGRA20_I2S_FIFO_SCR_FIFO_FULL_EMPTY_COUNT_MASK;
		level = reg & TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_MASK;
		level = level >> TEGRA20_I2S_FIFO_SCR_FIFO1_ATN_LVL_SHIFT;
		level = (level == 0) ? 1 : level*4;
		if (count > level)
			break;
		udelay(100);
	}
	/* Start the TDM engine */
	reg = tegra20_i2s_read(i2s, TEGRA20_I2S_TDM_TX_RX_CTRL);
	reg |= TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_TX_ENABLE;
	tegra20_i2s_write(i2s, TEGRA20_I2S_TDM_TX_RX_CTRL , reg);
}

static void tegra20_i2s_start_playback(struct tegra20_i2s *i2s)
{
	u32 reg;

	i2s->reg_ctrl |= TEGRA20_I2S_CTRL_IE_FIFO1_ERR;
	i2s->reg_ctrl |= TEGRA20_I2S_CTRL_FIFO1_ENABLE;
	tegra20_i2s_write(i2s, TEGRA20_I2S_CTRL, i2s->reg_ctrl);
	reg = tegra20_i2s_read(i2s, TEGRA20_I2S_TDM_CTRL);
	if (reg & TEGRA20_I2S_TDM_CTRL_TDM_EN)
		tegra20_i2s_tdm_start_playback(i2s);
}

static void tegra20_i2s_tdm_stop_playback(struct tegra20_i2s *i2s)
{
	int dcnt = 20;
	u32 reg;
	reg = tegra20_i2s_read(i2s, TEGRA20_I2S_TDM_TX_RX_CTRL);
	reg &= ~TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_TX_ENABLE;
	tegra20_i2s_write(i2s, TEGRA20_I2S_TDM_TX_RX_CTRL , reg);
	while (tegra20_i2s_tdm_tx_fifo_busy(i2s) && dcnt--)
		udelay(1000);
	reg = tegra20_i2s_read(i2s, TEGRA20_I2S_FIFO_SCR);
	reg |= TEGRA20_I2S_FIFO_SCR_FIFO1_CLR;
	tegra20_i2s_write(i2s, TEGRA20_I2S_FIFO_SCR, reg);
	i2s->reg_ctrl &= ~TEGRA20_I2S_CTRL_IE_FIFO1_ERR;
	i2s->reg_ctrl &= ~TEGRA20_I2S_CTRL_QE_FIFO1;
}

static void tegra20_i2s_stop_playback(struct tegra20_i2s *i2s)
{
	u32 tdm_ctrl;
	tdm_ctrl = tegra20_i2s_read(i2s, TEGRA20_I2S_TDM_CTRL);
	if (tdm_ctrl & TEGRA20_I2S_TDM_CTRL_TDM_EN)
		tegra20_i2s_tdm_stop_playback(i2s);
	i2s->reg_ctrl &= ~TEGRA20_I2S_CTRL_IE_FIFO1_ERR;
	i2s->reg_ctrl &= ~TEGRA20_I2S_CTRL_FIFO1_ENABLE;
	tegra20_i2s_write(i2s, TEGRA20_I2S_CTRL, i2s->reg_ctrl);
}

static void tegra20_i2s_tdm_start_capture(struct tegra20_i2s *i2s)
{
	u32 reg;
	int count = 0;
	int level = 1;
	int dcnt = 10;

	/* Wait for the FIFO1 to start */
	while (dcnt--) {
		reg = tegra20_i2s_read(i2s, TEGRA20_I2S_FIFO_SCR);
		count = reg >> TEGRA20_I2S_FIFO_SCR_FIFO2_FULL_EMPTY_COUNT_SHIFT;
		count = count & TEGRA20_I2S_FIFO_SCR_FIFO_FULL_EMPTY_COUNT_MASK;
		level = reg & TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_MASK;
		level = level >> TEGRA20_I2S_FIFO_SCR_FIFO2_ATN_LVL_SHIFT;
		level = (level == 0) ? 1 : level*4;
		if (count > level)
			break;
		udelay(100);
	}
	/* Start the TDM engine */
	reg = tegra20_i2s_read(i2s, TEGRA20_I2S_TDM_TX_RX_CTRL);
	reg |= TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_RX_ENABLE;
	tegra20_i2s_write(i2s, TEGRA20_I2S_TDM_TX_RX_CTRL , reg);
}

static void tegra20_i2s_start_capture(struct tegra20_i2s *i2s)
{
	u32 reg;
	i2s->reg_ctrl |= TEGRA20_I2S_CTRL_IE_FIFO2_ERR;
	i2s->reg_ctrl |= TEGRA20_I2S_CTRL_FIFO2_ENABLE;
	tegra20_i2s_write(i2s, TEGRA20_I2S_CTRL, i2s->reg_ctrl);
	reg = tegra20_i2s_read(i2s, TEGRA20_I2S_TDM_CTRL);
	if (reg & TEGRA20_I2S_TDM_CTRL_TDM_EN)
		tegra20_i2s_tdm_start_capture(i2s);
}

static void tegra20_i2s_tdm_stop_capture(struct tegra20_i2s *i2s)
{
	int dcnt = 20;
	u32 reg;
	reg = tegra20_i2s_read(i2s, TEGRA20_I2S_TDM_TX_RX_CTRL);
	reg &= ~TEGRA20_I2S_TDM_TX_RX_CTRL_TDM_RX_ENABLE;
	tegra20_i2s_write(i2s, TEGRA20_I2S_TDM_TX_RX_CTRL , reg);
	while (tegra20_i2s_tdm_rx_fifo_busy(i2s) && dcnt--)
		udelay(1000);
	reg = tegra20_i2s_read(i2s, TEGRA20_I2S_FIFO_SCR);
	reg |= TEGRA20_I2S_FIFO_SCR_FIFO2_CLR;
	tegra20_i2s_write(i2s, TEGRA20_I2S_FIFO_SCR, reg);
	i2s->reg_ctrl &= ~TEGRA20_I2S_CTRL_IE_FIFO2_ERR;
	i2s->reg_ctrl &= ~TEGRA20_I2S_CTRL_QE_FIFO2;
}

static void tegra20_i2s_stop_capture(struct tegra20_i2s *i2s)
{
	u32 tdm_ctrl;
	tdm_ctrl = tegra20_i2s_read(i2s, TEGRA20_I2S_TDM_CTRL);
	if (tdm_ctrl & TEGRA20_I2S_TDM_CTRL_TDM_EN)
		tegra20_i2s_tdm_stop_capture(i2s);
	i2s->reg_ctrl &= ~TEGRA20_I2S_CTRL_IE_FIFO2_ERR;
	i2s->reg_ctrl &= ~TEGRA20_I2S_CTRL_FIFO2_ENABLE;
	tegra20_i2s_write(i2s, TEGRA20_I2S_CTRL, i2s->reg_ctrl);
}

static int tegra20_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
				struct snd_soc_dai *dai)
{
	struct tegra20_i2s *i2s = snd_soc_dai_get_drvdata(dai);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
	case SNDRV_PCM_TRIGGER_RESUME:
		clk_enable(i2s->clk_i2s);
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			tegra20_i2s_start_playback(i2s);
		else
			tegra20_i2s_start_capture(i2s);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			tegra20_i2s_stop_playback(i2s);
		else
			tegra20_i2s_stop_capture(i2s);
		clk_disable(i2s->clk_i2s);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static irqreturn_t tdm_i2s_interrupt_handler(int irq, void *data)
{
	u32 status, control;
	struct snd_soc_dai *dai = (struct snd_soc_dai *)data;
	struct tegra_runtime_data *prtd;
	struct tegra20_i2s *i2s;

	if (!data || !dai->runtime || !dai->runtime->private_data) {
		pr_err("%s :could not clear interrupt flag\n", __func__);
		return IRQ_HANDLED;
	}

	prtd = dai->runtime->private_data;
	i2s = snd_soc_dai_get_drvdata(dai);

	status = tegra20_i2s_read(i2s, TEGRA20_I2S_STATUS);
	control = tegra20_i2s_read(i2s, TEGRA20_I2S_CTRL);

	if (status & TEGRA20_I2S_STATUS_FIFO1_ERR) {
		if ((control & TEGRA20_I2S_CTRL_FIFO1_RX_ENABLE) == 0)
			prtd->fifo_err = 1;
	}

	if (status & TEGRA20_I2S_STATUS_FIFO2_ERR) {
		if ((control & TEGRA20_I2S_CTRL_FIFO2_TX_ENABLE) == 1)
			prtd->fifo_err = 1;
	}
	prtd->fifo_err_count++;
	pr_err("%s :Fifo Error :I2S=%d Fifo Error Count=%d\n",
		__func__, dai->id, prtd->fifo_err_count);

	tegra20_i2s_write(i2s, TEGRA20_I2S_STATUS, status);
	return IRQ_HANDLED;
}

static int tegra20_i2s_probe(struct snd_soc_dai *dai)
{
	struct tegra20_i2s * i2s = snd_soc_dai_get_drvdata(dai);
#ifdef CONFIG_PM
	int i;
#endif

	dai->capture_dma_data = &i2s->capture_dma_data;
	dai->playback_dma_data = &i2s->playback_dma_data;

#ifdef CONFIG_PM
	/* populate the i2s reg cache with POR values*/
	clk_enable(i2s->clk_i2s);

	for (i = 0; i < ((TEGRA20_I2S_TDM_TX_RX_CTRL >> 2) + 1); i++) {
		if ((i == TEGRA20_I2S_CACHE_RSVD_6) ||
			(i == TEGRA20_I2S_CACHE_RSVD_7))
			continue;

		i2s->reg_cache[i] = tegra20_i2s_read(i2s, i << 2);
	}

	clk_disable(i2s->clk_i2s);
#endif

	/* Default values for DSP mode */
	i2s->dsp_config.num_slots = 1;
	i2s->dsp_config.slot_width = 2;
	i2s->dsp_config.tx_mask = 1;
	i2s->dsp_config.rx_mask = 1;
	i2s->dsp_config.rx_data_offset = 1;
	i2s->dsp_config.tx_data_offset = 1;

	if (request_irq(i2s->irq,
			tdm_i2s_interrupt_handler,
			IRQF_DISABLED, i2s->name,
			dai) < 0)
		pr_err("%s: could not register handler for irq: %s\n",
			__func__, i2s->name);

	return 0;
}

int tegra20_i2s_set_tdm_slot(struct snd_soc_dai *cpu_dai,
							unsigned int tx_mask,
							unsigned int rx_mask,
							int slots,
							int slot_width)
{
	struct tegra20_i2s *i2s = snd_soc_dai_get_drvdata(cpu_dai);
	i2s->dsp_config.num_slots = slots;
	i2s->dsp_config.slot_width = slot_width;
	i2s->dsp_config.tx_mask = tx_mask;
	i2s->dsp_config.rx_mask = rx_mask;
	i2s->dsp_config.rx_data_offset = 0;
	i2s->dsp_config.tx_data_offset = 0;
	return 0;
}

#ifdef CONFIG_PM
int tegra20_i2s_resume(struct snd_soc_dai *cpu_dai)
{
	struct tegra20_i2s *i2s = snd_soc_dai_get_drvdata(cpu_dai);
	int i;

	clk_enable(i2s->clk_i2s);

	/*restore the i2s regs*/
	for (i = 0; i < ((TEGRA20_I2S_TDM_TX_RX_CTRL >> 2) + 1); i++) {
		if ((i == TEGRA20_I2S_CACHE_RSVD_6) ||
			(i == TEGRA20_I2S_CACHE_RSVD_7))
			continue;

		tegra20_i2s_write(i2s, i << 2, i2s->reg_cache[i]);
	}

	/*restore the das regs*/
	tegra20_das_resume();

	clk_disable(i2s->clk_i2s);

	return 0;
}
#else
#define tegra20_i2s_resume NULL
#endif

static struct snd_soc_dai_ops tegra20_i2s_dai_ops = {
	.set_fmt	= tegra20_i2s_set_fmt,
	.hw_params	= tegra20_i2s_hw_params,
	.trigger	= tegra20_i2s_trigger,
	.set_tdm_slot = tegra20_i2s_set_tdm_slot,
};

struct snd_soc_dai_driver tegra20_i2s_dai[] = {
	{
		.name = DRV_NAME ".0",
		.probe = tegra20_i2s_probe,
		.resume = tegra20_i2s_resume,
		.playback = {
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.ops = &tegra20_i2s_dai_ops,
		.symmetric_rates = 1,
	},
	{
		.name = DRV_NAME ".1",
		.probe = tegra20_i2s_probe,
		.resume = tegra20_i2s_resume,
		.playback = {
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 16,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.ops = &tegra20_i2s_dai_ops,
		.symmetric_rates = 1,
	},
};

static __devinit int tegra20_i2s_platform_probe(struct platform_device *pdev)
{
	struct tegra20_i2s * i2s;
	struct resource *mem, *memregion, *dmareq;
	int ret;

	if ((pdev->id < 0) ||
		(pdev->id >= ARRAY_SIZE(tegra20_i2s_dai))) {
		dev_err(&pdev->dev, "ID %d out of range\n", pdev->id);
		return -EINVAL;
	}

	i2s = kzalloc(sizeof(struct tegra20_i2s), GFP_KERNEL);
	if (!i2s) {
		dev_err(&pdev->dev, "Can't allocate tegra20_i2s\n");
		ret = -ENOMEM;
		goto exit;
	}
	dev_set_drvdata(&pdev->dev, i2s);

	i2s->clk_i2s = clk_get(&pdev->dev, NULL);
	if (IS_ERR(i2s->clk_i2s)) {
		dev_err(&pdev->dev, "Can't retrieve i2s clock\n");
		ret = PTR_ERR(i2s->clk_i2s);
		goto err_free;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "No memory resource\n");
		ret = -ENODEV;
		goto err_clk_put;
	}

	dmareq = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (!dmareq) {
		dev_err(&pdev->dev, "No DMA resource\n");
		ret = -ENODEV;
		goto err_clk_put;
	}

	memregion = request_mem_region(mem->start, resource_size(mem),
					DRV_NAME);
	if (!memregion) {
		dev_err(&pdev->dev, "Memory region already claimed\n");
		ret = -EBUSY;
		goto err_clk_put;
	}

	i2s->regs = ioremap(mem->start, resource_size(mem));
	if (!i2s->regs) {
		dev_err(&pdev->dev, "ioremap failed\n");
		ret = -ENOMEM;
		goto err_release;
	}

	i2s->capture_dma_data.addr = mem->start + TEGRA20_I2S_FIFO2;
	i2s->capture_dma_data.wrap = 4;
	i2s->capture_dma_data.width = 32;
	i2s->capture_dma_data.req_sel = dmareq->start;

	i2s->playback_dma_data.addr = mem->start + TEGRA20_I2S_FIFO1;
	i2s->playback_dma_data.wrap = 4;
	i2s->playback_dma_data.width = 32;
	i2s->playback_dma_data.req_sel = dmareq->start;

	ret = snd_soc_register_dai(&pdev->dev, &tegra20_i2s_dai[pdev->id]);
	if (ret) {
		dev_err(&pdev->dev, "Could not register DAI: %d\n", ret);
		ret = -ENOMEM;
		goto err_unmap;
	}

	tegra20_i2s_debug_add(i2s, pdev->id);
	i2s->irq = platform_get_irq(pdev, 0);
	i2s->name = pdev->name;

	return 0;

err_unmap:
	iounmap(i2s->regs);
err_release:
	release_mem_region(mem->start, resource_size(mem));
err_clk_put:
	clk_put(i2s->clk_i2s);
err_free:
	kfree(i2s);
exit:
	return ret;
}

static int __devexit tegra20_i2s_platform_remove(struct platform_device *pdev)
{
	struct tegra20_i2s *i2s = dev_get_drvdata(&pdev->dev);
	struct resource *res;

	snd_soc_unregister_dai(&pdev->dev);

	tegra20_i2s_debug_remove(i2s);

	iounmap(i2s->regs);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(res->start, resource_size(res));

	clk_put(i2s->clk_i2s);

	kfree(i2s);

	return 0;
}

static struct platform_driver tegra20_i2s_driver = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
	},
	.probe = tegra20_i2s_platform_probe,
	.remove = __devexit_p(tegra20_i2s_platform_remove),
};

static int __init snd_tegra20_i2s_init(void)
{
	return platform_driver_register(&tegra20_i2s_driver);
}
module_init(snd_tegra20_i2s_init);

static void __exit snd_tegra20_i2s_exit(void)
{
	platform_driver_unregister(&tegra20_i2s_driver);
}
module_exit(snd_tegra20_i2s_exit);

MODULE_AUTHOR("Stephen Warren <swarren@nvidia.com>");
MODULE_DESCRIPTION("Tegra I2S ASoC driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
