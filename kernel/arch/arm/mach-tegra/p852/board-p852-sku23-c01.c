/*
 * arch/arm/mach-tegra/board-p852-sku23-c01.c
 *
 * Copyright (c) 2010-2013, NVIDIA Corporation.  All rights reserved.
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

#include "board-p852.h"

static inline void p852_sku23_c01_spi_init(void)
{
	p852_sku_peripherals |= P852_SKU_SPI_ENABLE;
	p852_spi_peripherals |=
	    ((P852_SPI_SLAVE_ASYNC | P852_SPI_ENABLE) << P852_SPI1_SHIFT) |
	    ((P852_SPI_MASTER | P852_SPI_ENABLE) << P852_SPI2_SHIFT) |
	    ((P852_SPI_SLAVE_ASYNC | P852_SPI_ENABLE) << P852_SPI4_SHIFT);
}

static inline void p852_sku23_c01_i2s_init(void)
{
	p852_sku_peripherals |= P852_SKU_I2S_ENABLE;
	p852_i2s_peripherals |=
	    ((P852_I2S_TDM | P852_I2S_ENABLE) << P852_I2S1_SHIFT) |
	    ((P852_I2S_TDM | P852_I2S_ENABLE) << P852_I2S2_SHIFT);
}

static inline void p852_sku23_c01_sdhci_init(void)
{
	p852_sku_peripherals |= P852_SKU_SDHCI_ENABLE;
	p852_sdhci_peripherals |= (P852_SDHCI_ENABLE << P852_SDHCI1_SHIFT) |
	    (P852_SDHCI_ENABLE << P852_SDHCI2_SHIFT) |
	    ((P852_SDHCI_ENABLE | P852_SDHCI_CD_EN | P852_SDHCI_PW_EN)
	    << P852_SDHCI3_SHIFT);

	p852_sdhci_platform_data[1].is_8bit = true;
	p852_sdhci_platform_data[2].cd_gpio = TEGRA_GPIO_PD7;
	p852_sdhci_platform_data[2].power_gpio = TEGRA_GPIO_PK5;
}

static inline void p852_sku23_c01_uart_init(void)
{
	p852_sku_peripherals |= P852_SKU_UART_ENABLE;
	p852_uart_peripherals |=
	    ((P852_UART_ENABLE) << P852_UARTA_SHIFT) |
	    ((P852_UART_ENABLE | P852_UART_HS) << P852_UARTB_SHIFT) |
	    ((P852_UART_ENABLE | P852_UART_DB) << P852_UARTC_SHIFT);
}

static inline void p852_sku23_c01_display_init(void)
{
	p852_sku_peripherals |= P852_SKU_DISPLAY_ENABLE;
	p852_display_peripherals |=
		(P852_DISP_ENABLE << P852_DISPA_SHIFT);
}

static inline void p852_sku23_c01_i2c_init(void)
{
	p852_sku_peripherals |= P852_SKU_I2C_ENABLE;
	p852_i2c_peripherals |=
	    ((P852_I2C_ENABLE) << P852_I2C1_SHIFT) |
	    ((P852_I2C_ENABLE) << P852_I2C2_SHIFT) |
	    ((P852_I2C_ENABLE) << P852_I2C3_SHIFT) |
	    ((P852_I2C_ENABLE) << P852_I2C4_SHIFT);
	p852_i2c_set_default_clock(0, 50*1000);
	p852_i2c_set_default_clock(1, 100*1000);
	p852_i2c_set_default_clock(2, 348*1000);
}

void __init p852_sku23_c01_init(void)
{
	p852_sku_peripherals |= P852_SKU_NAND_ENABLE;

	p852_sku23_c01_spi_init();
	p852_sku23_c01_i2s_init();
	p852_sku23_c01_uart_init();
	p852_sku23_c01_sdhci_init();
	p852_sku23_c01_i2c_init();
	p852_sku23_c01_display_init();

	p852_common_init();
}

static void __init p852_sku24_usb_init(void)
{
	p852_sku_peripherals |= P852_SKU_ULPI_DISABLE;
}

void __init p852_sku24_c01_init(void)
{
	p852_sku_peripherals |= P852_SKU_NAND_ENABLE;

	/* common sku23 part */
	p852_sku23_c01_spi_init();
	p852_sku23_c01_i2s_init();
	p852_sku23_c01_uart_init();
	p852_sku23_c01_sdhci_init();
	p852_sku23_c01_i2c_init();
	p852_sku23_c01_display_init();

	/* sku24 specific */
	p852_sku24_usb_init();

	p852_common_init();
}

