/*
 * arch/arm/mach-tegra/board-p852-audio.c
 *
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
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
 */

#include <linux/resource.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>

#include <asm/mach-types.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <linux/i2c.h>
#include <mach/pinmux.h>
#include <asm/mach-types.h>
#include <mach/tegra_vcm_pdata.h>

#include "board-p852.h"

static struct tegra_vcm_platform_data p852_audio_tdm_pdata = {
	.codec_info[0] = {
		.codec_dai_name = "dit-hifi",
		.cpu_dai_name = "tegra20-i2s.0",
		.codec_name = "spdif-dit.0",
		.name = "tegra-i2s-1",
		.pcm_driver = "tegra-tdm-pcm-audio",
		.i2s_format = format_tdm,
		/* Defines whether the Codec Chip is Master or Slave */
		.master = 1,
		/* Defines the number of TDM slots */
		.num_slots = 4,
		/* Defines the width of each slot */
		.slot_width = 32,
		/* Defines which slots are enabled */
		.tx_mask = 0x0f,
		.rx_mask = 0x0f,
	},
	.dac_info[0] = {
		.dac_id = 0,
		.dap_id = 0,
	},
	.codec_info[1] = {
		.codec_dai_name = "dit-hifi",
		.cpu_dai_name = "tegra20-i2s.1",
		.codec_name = "spdif-dit.1",
		.name = "tegra-i2s-2",
		.pcm_driver = "tegra-tdm-pcm-audio",
		.i2s_format = format_tdm,
		/* Defines whether the Codec Chip is Master or Slave */
		.master = 1,
		/* Defines the number of TDM slots */
		.num_slots = 8,
		.slot_width = 32,
		/* Defines which slots are enabled */
		.tx_mask = 0xff,
		.rx_mask = 0xff,
	},
	.dac_info[1] = {
		.dac_id = 1,
		.dap_id = 1,
	},
};
static struct platform_device generic_codec_1 = {
	.name       = "spdif-dit",
	.id         = 0,
};
static struct platform_device generic_codec_2 = {
	.name       = "spdif-dit",
	.id         = 1,
};

static struct platform_device tegra_snd_p852 = {
	.name       = "tegra-snd-p852",
	.id = 0,
	.dev    = {
		.platform_data = &p852_audio_tdm_pdata,
	},
};

void p852_i2s_audio_init(void)
{
	platform_device_register(&tegra_pcm_device);
	platform_device_register(&tegra_tdm_pcm_device);
	platform_device_register(&generic_codec_1);
	platform_device_register(&generic_codec_2);
	platform_device_register(&tegra_i2s_device1);
	platform_device_register(&tegra_i2s_device2);
	platform_device_register(&tegra_das_device);
	platform_device_register(&tegra_snd_p852);
}


