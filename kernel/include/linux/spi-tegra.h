/*
 * include/linux/spi-tegra.c
 *
 * Copyright (c) 2011-2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _LINUX_SPI_TEGRA_H
#define _LINUX_SPI_TEGRA_H

#include <linux/clk.h>

struct spi_clk_parent {
	const char *name;
	struct clk *parent_clk;
	unsigned long fixed_clk_rate;
};

struct tegra_spi_platform_data {
	bool is_dma_based;
	int max_dma_buffer;
	bool is_clkon_always;
	int nop_buf_sz;
	unsigned char *nop_tx_buf;
	int cs_gpio;
	unsigned int max_rate;
	struct spi_clk_parent *parent_clk_list;
	int  parent_clk_count;
	bool lsbfe;
};

/* Controller data from device to pass some info like
 * hw based chip select can be used or not and if yes
 * then CS hold and setup time. */
struct tegra_spi_device_controller_data {
	bool is_hw_based_cs;
	int cs_setup_clk_count;
	int cs_hold_clk_count;
	/* CS line should stay active between bytes or not */
	bool cs_active_between;
	/* No of cycles CS line to be deasserted */
	int cs_inactive_period;
};

#endif /* _LINUX_SPI_TEGRA_H */
