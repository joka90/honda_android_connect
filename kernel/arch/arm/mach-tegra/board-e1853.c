/*
 * arch/arm/mach-tegra/board-e1853.c
 *
 * Copyright (c) 2012-2013, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 *----------------------------------------------------------------------------
 * COPYRIGHT(C) FUJITSU TEN LIMITED 2012-2013
 *----------------------------------------------------------------------------
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/serial_8250.h>
#include <linux/i2c.h>
#include <linux/i2c/panjit_ts.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/i2c-tegra.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/platform_data/tegra_usb.h>
#include <linux/platform_data/tegra_nor.h>
#include <linux/spi/spi.h>
#include <linux/tegra_uart.h>
#include <linux/mtd/partitions.h>
#if defined(CONFIG_TOUCHSCREEN_ATMEL_MXT)
#include <linux/i2c/atmel_mxt_ts.h>
#endif
#include <linux/platform_data/tmon_tmp411.h>
#include <linux/platform_data/tegra_ahci.h>
#include <mach/clk.h>
#include <mach/iomap.h>
#include <mach/io_dpd.h>
#include <mach/irqs.h>
#include <mach/pinmux.h>
#include <mach/iomap.h>
#include <mach/io.h>
#include <mach/pci.h>
#include <mach/audio.h>
#include <mach/tegra_vcm_pdata.h>
#include <asm/mach/flash.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/usb_phy.h>
#include <sound/wm8903.h>
#include <mach/tsensor.h>
#include "board.h"
#include "clock.h"
#include "board-e1853.h"
#include "devices.h"
#include "gpio-names.h"
#include "fuse.h"
#include "pm.h"
#include <mach/board_id.h>
#include "therm-monitor.h"

/* Add @FCT 20130517 start */
#ifdef CONFIG_KEYBOARD_ADA_HWSW
#include <linux/gpio_keys.h>
#endif
/* Add @FCT 20130517 end */
/* FUJITSU TEN:2012-08-30 include accelerometer & gyro add. start */
#include <linux/i2c/lis331dlh.h>
#include <linux/i2c/tag206n.h>
/* FUJITSU TEN:2012-08-30 include accelerometer & gyro add. end */
/* FUJITSU TEN:2012-07-11 over current protection init add. start */
#include <linux/usb/ocp.h>
/* FUJITSU TEN:2012-07-11 over current protection init add. end */
/* FUJITSU TEN:2012-07-03 Bluetooth use UART start */
#include <linux/skbuff.h>
#include <linux/ti_wilink_st.h>
/* FUJITSU TEN:2012-07-03 Bluetooth use UART end */
/* FUJITSU TEN:2012-09-21 include gps_ant.h add. start */
#include <linux/gps_ant.h>
/* FUJITSU TEN:2012-09-21 include gps_ant.h add. end */

/* FUJITSU TEN:2012-07-04 dsp init add. start */
#include <linux/i2c/ak7736_dsp.h>
/* FUJITSU TEN:2012-07-04 dsp init add. end */
#include <linux/spi-tegra.h>

#if defined(CONFIG_BCM4329_RFKILL)
static struct resource e1853_bcm4329_rfkill_resources[] = {
	{
		.name   = "bcm4329_nreset_gpio",
		.start  = MISCIO_BT_RST_GPIO,
		.end    = MISCIO_BT_RST_GPIO,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "bcm4329_nshutdown_gpio",
		.start  = MISCIO_BT_EN_GPIO,
		.end    = MISCIO_BT_EN_GPIO,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device e1853_bcm4329_rfkill_device = {
	.name = "bcm4329_rfkill",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(e1853_bcm4329_rfkill_resources),
	.resource       = e1853_bcm4329_rfkill_resources,
};
#endif

static __initdata struct tegra_clk_init_table e1853_clk_init_table[] = {
	/* name		parent		rate		enabled */
	{ "pll_m",		NULL,		0,		true},
	{ "hda",		"pll_p",	108000000,	false},
	{ "hda2codec_2x",	"pll_p",	48000000,	false},
	{ "pwm",		"clk_32k",	32768,		false},
/* FUJITSU TEN:2012-12-21 HDMI SUPPORT BEGIN */
/* "blink" is disabled for disable clk32k_out */
/*	{ "blink",		"clk_32k",	32768,		true}, */
/* FUJITSU TEN:2012-12-21 HDMI SUPPORT END */
	{ "pll_a",		NULL,		552960000,	false},
	/* audio cif clock should be faster than i2s */
	{ "pll_a_out0",		NULL,		24576000,	false},
	{ "d_audio",		"pll_a_out0",	24576000,	false},
	{ "nor",		"pll_p",	102000000,	true},
//FTEN	{ "uarta",		"pll_p",	480000000,	true},
	{ "uartb",		"pll_p",	480000000,	true},
	{ "uartd",		"pll_p",	480000000,	true},
	{ "uarte",		"pll_p",	480000000,	true},
	{ "sbc1",		"pll_m",	100000000,	true},
	{ "sdmmc2",		"pll_p",	52000000,	false}, //FTEN disable clk
//FTEN	{ "sdmmc3",		"pll_p",	104000000,	true},
//FTEN	{ "sdmmc4",		"pll_p",	52000000,	true},
	{ "sbc2",		"pll_m",	100000000,	true},
	{ "sbc3",		"pll_m",	100000000,	true},
	{ "sbc4",		"pll_m",	100000000,	true},
	{ "sbc5",		"pll_m",	100000000,	true},
	{ "sbc6",		"pll_m",	100000000,	true},
	{ "cpu_g",		"cclk_g",	900000000,	false},
	{ "i2s0",		"pll_a_out0",	24576000,	false},
	{ "i2s1",		"pll_a_out0",	24576000,	false},
	{ "i2s2",		"pll_a_out0",	24576000,	false},
	{ "i2s3",		"pll_a_out0",	24576000,	false},
	{ "i2s4",		"pll_a_out0",	24576000,	false},
//FTEN	{ "audio0",		"i2s0_sync",	12288000,	false},
//FTEN	{ "audio1",		"i2s1_sync",	12288000,	false},
//FTEN	{ "audio2",		"i2s2_sync",	12288000,	false},
//FTEN	{ "audio3",		"i2s3_sync",	12288000,	false},
//FTEN	{ "audio4",		"i2s4_sync",	12288000,	false},
	{ "apbif",		"clk_m",	12000000,	false},
	{ "dam0",		"clk_m",	12000000,	true},
	{ "dam1",		"clk_m",	12000000,	true},
	{ "dam2",		"clk_m",	12000000,	true},
	{ "vi",			"pll_p",	470000000,	false},
	{ "vi_sensor",		"pll_p",	150000000,	false},
	{ "vde",		"pll_c",	484000000,	false},
	{ "host1x",		"pll_c",	242000000,	false},
	{ "mpe",		"pll_c",	484000000,	false},
	{ "i2c1",		"pll_p",	3200000,	true},
	{ "i2c2",		"pll_p",	3200000,	true},
	{ "i2c3",		"pll_p",	3200000,	true},
	{ "i2c4",		"pll_p",	3200000,	true},
	{ "i2c5",		"pll_p",	3200000,	true},
	{ "se",			"pll_p",	204000000,	true},
	{"wake.sclk",		NULL,		334000000,	true },
	{ NULL,			NULL,		0,		0},
};

struct therm_monitor_ldep_data ltemp_reg_data[] = {
	{
		.reg_addr = 0x000008b0, /* only Offset */
		.temperat = {15000, 85000, INT_MAX}, /* Maximum 9 values*/
		.value    = {0xf1a18000, 0xf1f1a000}, /* Maximum 9 values*/
	},
	{
		.reg_addr = INVALID_ADDR,
	},
};/* Local sensor temperature dependent register data. */

struct therm_monitor_data e1853_therm_monitor_data = {
	.brd_ltemp_reg_data = ltemp_reg_data,
	.delta_temp = 4000,
	.delta_time = 2000,
	.remote_offset = 8000,
	.alert_gpio = TEGRA_GPIO_PR3,
	.local_temp_update = true,
	.utmip_reg_update = false, /* USB registers update is not
						required for now */
	.i2c_bus_num = I2C_BUS_TMP411,
	.i2c_dev_addrs = I2C_ADDR_TMP411,
	.i2c_dev_name = "tmon-tmp411-sensor",
};

static struct tegra_i2c_platform_data e1853_i2c1_platform_data = {
	.adapter_nr	= 0,
	.bus_count	= 1,
/* FUJITSU TEN:2012-08-30 dsp init edit. start */
	.bus_clk_rate	= { 400000, 0 },
/* FUJITSU TEN:2012-08-30 dsp init edit. start */
};

static struct tegra_i2c_platform_data e1853_i2c2_platform_data = {
	.adapter_nr	= 1,
	.bus_count	= 1,
/* FUJITSU TEN:2012-06-28 touch use I2C clk edit. start */
	.bus_clk_rate	= { 400000, 0 },
/* FUJITSU TEN:2012-06-28 touch use I2C clk edit. end */
	.is_clkon_always = true,
};

static struct tegra_i2c_platform_data e1853_i2c4_platform_data = {
	.adapter_nr	= 3,
	.bus_count	= 1,
/* FUJITSU TEN:2013-06-26 transfer rate 20Kbps -> 400Kbps start */
//	.bus_clk_rate	= { 20000, 0 },
	.bus_clk_rate	= { 400000, 0 },
/* FUJITSU TEN:2013-06-26 transfer rate 20Kbps -> 400Kbps end */

};

static struct tegra_i2c_platform_data e1853_i2c5_platform_data = {
	.adapter_nr	= 4,
	.bus_count	= 1,
	.bus_clk_rate	= { 100000, 0 },
};

static struct tegra_pci_platform_data e1853_pci_platform_data = {
	.port_status[0] = 0,
	.port_status[1] = 1,
	.port_status[2] = 1,
	.use_dock_detect = 0,
	.gpio 		= 0,
};

static void e1853_pcie_init(void)
{
//	tegra_pci_device.dev.platform_data = &e1853_pci_platform_data;
//	platform_device_register(&tegra_pci_device);
}

static void e1853_i2c_init(void)
{
	tegra_i2c_device1.dev.platform_data = &e1853_i2c1_platform_data;
	tegra_i2c_device2.dev.platform_data = &e1853_i2c2_platform_data;
	tegra_i2c_device4.dev.platform_data = &e1853_i2c4_platform_data;
	tegra_i2c_device5.dev.platform_data = &e1853_i2c5_platform_data;

	platform_device_register(&tegra_i2c_device5);
	platform_device_register(&tegra_i2c_device4);
	platform_device_register(&tegra_i2c_device2);
	platform_device_register(&tegra_i2c_device1);
}

static struct platform_device *e1853_uart_devices[] __initdata = {
//FTEN
	&tegra_uartb_device,
	&tegra_uarta_device,
	&tegra_uartd_device,
	&tegra_uarte_device,
};

static void __init uart_debug_init(void)
{
	/* UARTA is the debug port. */
	pr_info("Selecting UARTA as the debug console\n");
	e1853_uart_devices[1] = &debug_uarta_device;
	debug_uart_clk = clk_get_sys("serial8250.0", "uarta");
	debug_uart_port_base = ((struct plat_serial8250_port *)(
				debug_uarta_device.dev.platform_data))->mapbase;
}

/* FUJITSU TEN:2012-07-03 Bluetooth use UART start */
static struct uart_clk_parent uart_parent_clk[] = {
	[0] = {.name = "pll_p"},
	[1] = {.name = "clk_m"},
	[2] = {.name = "pll_m"},
};
static struct tegra_uart_platform_data f12arc_uart_pdata;
/* FUJITSU TEN:2012-07-03 Bluetooth use UART end */

static void __init e1853_uart_init(void)
{
/* FUJITSU TEN:2012-07-03 Bluetooth use UART start */
	int i;
	struct clk *c;

	for (i = 0; i < ARRAY_SIZE(uart_parent_clk); ++i) {
		c = tegra_get_clock_by_name(uart_parent_clk[i].name);
		if (IS_ERR_OR_NULL(c)) {
			pr_err("Not able to get the clock for %s\n",
						uart_parent_clk[i].name);
			continue;
		}
		uart_parent_clk[i].parent_clk = c;
		uart_parent_clk[i].fixed_clk_rate = clk_get_rate(c);
	}

	f12arc_uart_pdata.parent_clk_list = uart_parent_clk;
	f12arc_uart_pdata.parent_clk_count = ARRAY_SIZE(uart_parent_clk);
	tegra_uarta_device.dev.platform_data = &f12arc_uart_pdata;
	tegra_uartb_device.dev.platform_data = &f12arc_uart_pdata;
	tegra_uartd_device.dev.platform_data = &f12arc_uart_pdata;
	tegra_uarte_device.dev.platform_data = &f12arc_uart_pdata;
/* FUJITSU TEN:2012-07-03 Bluetooth use UART end */

	/* Register low speed only if it is selected */
	if (!is_tegra_debug_uartport_hs()) {
		uart_debug_init();
		/* Clock enable for the debug channel */
		if (!IS_ERR_OR_NULL(debug_uart_clk)) {
			pr_info("The debug console clock name is %s\n",
						debug_uart_clk->name);
			clk_enable(debug_uart_clk);
			clk_set_rate(debug_uart_clk, 408000000);
		} else {
			pr_err("Not getting the clock %s for debug console\n",
					debug_uart_clk->name);
		}
	}

	platform_add_devices(e1853_uart_devices,
				ARRAY_SIZE(e1853_uart_devices));
}

#if defined(CONFIG_RTC_DRV_TEGRA)
static struct resource tegra_rtc_resources[] = {
	[0] = {
		.start = TEGRA_RTC_BASE,
		.end = TEGRA_RTC_BASE + TEGRA_RTC_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = INT_RTC,
		.end = INT_RTC,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device tegra_rtc_device = {
	.name = "tegra_rtc",
	.id   = -1,
	.resource = tegra_rtc_resources,
	.num_resources = ARRAY_SIZE(tegra_rtc_resources),
};
#endif

static struct tegra_vcm_platform_data generic_audio_pdata = {
	.codec_info[0] = {
		.codec_dai_name = "dit-hifi",
		.cpu_dai_name = "tegra30-i2s.0",
		.codec_name = "spdif-dit.0",
		.name = "tegra-i2s-1",
		.pcm_driver = "tegra-pcm-audio",
		.i2s_format = format_i2s,
		/* Defines whether the Audio codec chip is master or slave */
		.master = 0,
	},
	.codec_info[1] = {
		.codec_dai_name = "dit-hifi",
		.cpu_dai_name = "tegra30-i2s.4",
		.codec_name = "spdif-dit.1",
		.name = "tegra-i2s-2",
		.pcm_driver = "tegra-pcm-audio",
		.i2s_format = format_i2s,
		/* Defines whether the Audio codec chip is master or slave */
/* FUJITSU TEN:2012-12-20 AUDIO(10B) add start */
		.master = 1,
/* FUJITSU TEN:2012-12-20 AUDIO(10B) add start */
	},
/* FUJITSU TEN:2012-11-22 AUDIO(11E) add start */
	.codec_info[2] = {
		.codec_dai_name = "dit-hifi",
		.cpu_dai_name = "tegra30-i2s.2",
		.codec_name = "spdif-dit.0",
		.name = "tegra-i2s-3",
		.pcm_driver = "tegra-pcm-audio",
		.i2s_format = format_i2s,
		.master = 0,
/* FUJITSU TEN:2012-11-22 AUDIO(11E) add end */
	}
};

static struct tegra_vcm_platform_data jetson_audio_pdata = {
	.codec_info[0] = {
		.codec_dai_name = "wm8731-hifi",
		.cpu_dai_name = "tegra30-i2s.0",
		.codec_name = "wm8731.0-001a",
		.name = "tegra-i2s-1",
		.pcm_driver = "tegra-pcm-audio",
		.i2s_format = format_i2s,
		/* Audio Codec is Slave */
		.master = 0,
	},
	.codec_info[1] = {
		.codec_dai_name = "ad193x-hifi",
		.cpu_dai_name = "tegra30-i2s.3",
		.codec_name = "ad193x.0-0007",
		.name = "tegra-i2s-2",
		.pcm_driver = "tegra-tdm-pcm-audio",
		.i2s_format = format_tdm,
		/* Audio Codec is Master */
		.master = 1,
		.num_slots = 8,
		.slot_width = 32,
		.tx_mask = 0xff,
		.rx_mask = 0xff,
	},
};

static struct platform_device generic_codec_1 = {
	.name		= "spdif-dit",
	.id			= 0,
};
static struct platform_device generic_codec_2 = {
	.name		= "spdif-dit",
	.id			= 1,
};

static struct platform_device tegra_snd_e1853 = {
	.name       = "tegra-snd-e1853",
	.id = 0,
	.dev    = {
		.platform_data = &generic_audio_pdata,
	},
};

static struct i2c_board_info __initdata wm8731_board_info = {
	I2C_BOARD_INFO("wm8731", 0x1a),
};

static struct i2c_board_info __initdata ad1937_board_info = {
	I2C_BOARD_INFO("ad1937", 0x07),
};

static void e1853_i2s_audio_init(void)
{
	platform_device_register(&tegra_pcm_device);
	platform_device_register(&tegra_tdm_pcm_device);
	platform_device_register(&generic_codec_1);
	platform_device_register(&generic_codec_2);
	platform_device_register(&tegra_i2s_device0);
/* FUJITSU TEN:2012-11-22 AUDIO(11E) add start */
	platform_device_register(&tegra_i2s_device2);
/* FUJITSU TEN:2012-11-22 AUDIO(11E) add end */
	platform_device_register(&tegra_i2s_device4);
	platform_device_register(&tegra_ahub_device);

	if (tegra_is_board(NULL, "61860", NULL, NULL, NULL)) {
		i2c_register_board_info(0, &wm8731_board_info, 1);
		i2c_register_board_info(0, &ad1937_board_info, 1);
		tegra_snd_e1853.dev.platform_data = &jetson_audio_pdata;
		platform_device_register(&tegra_snd_e1853);
	} else {
		platform_device_register(&tegra_snd_e1853);
	}
}

#if defined(CONFIG_SPI_TEGRA) && defined(CONFIG_SPI_SPIDEV)
static struct tegra_spi_platform_data e1853_spi_pdata[] = {
	{
		.is_dma_based		= 1,
		.is_clkon_always	= 0,
		.max_rate			= 500*1000,//500kHz
		.lsbfe				= true,
	},
	{
		.is_dma_based		= 1,
		.is_clkon_always	= 0,
		.max_rate			= 500*1000,//500kHz
		.lsbfe				= true,
	},
	{
		.is_dma_based		= 1,
		.is_clkon_always	= 0,
		.max_rate			= 75*1000,//75kHz
	}
};
static struct spi_board_info tegra_spi_devices[] __initdata = {
	{
		.modalias = "spi-cpucom",
		.bus_num = 0,
		.chip_select = 0,
		.mode = SPI_MODE_3,
		.max_speed_hz = 500*1000,//500kHz
		.platform_data = NULL,
		.irq = 0,
	},
	{
		.modalias = "spi-cpucom",
		.bus_num = 1,
		.chip_select = 0,
		.mode = SPI_MODE_3,
		.max_speed_hz = 500*1000,//500kHz
		.platform_data = NULL,
		.irq = 0,
	},
	{
		.modalias = "spi-ipod-auth",
		.bus_num = 4,
		.chip_select = 3,
		.mode = SPI_MODE_1,
		.max_speed_hz = 75*1000,//75kHz
		.platform_data = NULL,
		.irq = 0,
	},
};

static void __init e1853_register_spidev(void)
{
	spi_register_board_info(tegra_spi_devices,
			ARRAY_SIZE(tegra_spi_devices));
}
#else
#define e1853_register_spidev() do {} while (0)
#endif

struct spi_clk_parent spi_parent_clk[] = {
	[0] = {.name = "pll_p"},
	[1] = {.name = "pll_m"},
	[2] = {.name = "clk_m"},
};

static void e1853_spi_init(void)
{
	struct clk *c;
	int i;
	/* clock enable */
	writel(readl(CLK_RST_CONTROLLER_CLK_OUT_ENB_H_0)|CLK_ENB_SBC2|CLK_ENB_SBC1,CLK_RST_CONTROLLER_CLK_OUT_ENB_H_0);
	writel(readl(CLK_RST_CONTROLLER_CLK_OUT_ENB_V_0)|CLK_ENB_SBC5,CLK_RST_CONTROLLER_CLK_OUT_ENB_V_0);
	/* clock setting  CLK_M:12MHz is devided 24 = 500kHz  for cpu-com*/
	writel(CLK_M_CLK_M|12	,CLK_RST_CONTROLLER_CLK_SOURCE_SBC1);
	writel(CLK_M_CLK_M|12	,CLK_RST_CONTROLLER_CLK_SOURCE_SBC2);
	/* clock setting  CLK_M:12MHz is devided 160 = 75kHz for ipod auth*/
	writel(CLK_M_CLK_M|(160+1)	,CLK_RST_CONTROLLER_CLK_SOURCE_SBC5);

	/* SFIO setting on gpio						ball name	SFIO			GPIO */
	/* SPI1 for CPUCOM1 */
	tegra_gpio_disable(TEGRA_GPIO_PY3);		/* 	ULPI_STP	SPI1.A-CS0_N	GPIO-Y3 */
	tegra_gpio_disable(TEGRA_GPIO_PY2);		/* 	ULPI_NXT	SPI1.A-SCK		GPIO-Y2 */
	tegra_gpio_disable(TEGRA_GPIO_PY1);		/* 	ULPI_DIR	SPI1.A-MISO		GPIO-Y1 */
	tegra_gpio_disable(TEGRA_GPIO_PY0);		/* 	ULPI_CLK	SPI1.A-MOSI		GPIO-Y0 */
	/* SPI2 for CPUCOM2 */
	tegra_gpio_disable(TEGRA_GPIO_PO0);		/* 	ULPI_DATA7	SPI2.A-CS1_N	GPIO-O0 */
	tegra_gpio_disable(TEGRA_GPIO_PO7);		/* 	ULPI_DATA6	SPI2.A-SCK		GPIO-O7 */
	tegra_gpio_disable(TEGRA_GPIO_PO6);		/* 	ULPI_DATA5	SPI2.A-MISO		GPIO-O6 */
	tegra_gpio_disable(TEGRA_GPIO_PO5);		/* 	ULPI_DATA4	SPI2.A-MOSI		GPIO-O5 */
	/* SPI5 for ipod authentification */
	tegra_gpio_disable(TEGRA_GPIO_PW0);		/* 	LCD_CS1_N	SPI5.B-CS3_N	GPIO-W0 */
	tegra_gpio_disable(TEGRA_GPIO_PB2);		/* 	LCD_PWR0	SPI5.B-MOSI		GPIO-B2 */
	tegra_gpio_disable(TEGRA_GPIO_PC6);		/* 	LCD_PWR2	SPI5.B-MISO		GPIO-C6 */
	tegra_gpio_disable(TEGRA_GPIO_PZ3);		/* 	LCD_WR_N	SPI5.B-SCK		GPIO-Z3 */

	
	for (i = 0; i < ARRAY_SIZE(spi_parent_clk); ++i) {
		c = tegra_get_clock_by_name(spi_parent_clk[i].name);
		if (IS_ERR_OR_NULL(c)) {
			pr_err("Not able to get the clock for %s\n",spi_parent_clk[i].name);
			continue;
		}
		spi_parent_clk[i].parent_clk = c;
		spi_parent_clk[i].fixed_clk_rate = clk_get_rate(c);
	}
	for (i = 0; i < ARRAY_SIZE(e1853_spi_pdata); ++i) {
		e1853_spi_pdata[i].parent_clk_list = spi_parent_clk;
		e1853_spi_pdata[i].parent_clk_count = ARRAY_SIZE(spi_parent_clk);
	}

	tegra_spi_device1.dev.platform_data = &e1853_spi_pdata[0];
	tegra_spi_device2.dev.platform_data = &e1853_spi_pdata[1];
	tegra_spi_device5.dev.platform_data = &e1853_spi_pdata[2];

	
	platform_device_register(&tegra_spi_device1);
	platform_device_register(&tegra_spi_device2);
	platform_device_register(&tegra_spi_device5);
	e1853_register_spidev();
}

static struct platform_device tegra_camera = {
	.name = "tegra_camera",
	.id = -1,
};

/* Add @FCT 20130517 start */
#ifdef CONFIG_KEYBOARD_ADA_HWSW
#define GPIO_KEY(_id, _gpio, _iswake)		\
	{					\
		.code = _id,			\
		.gpio = TEGRA_GPIO_##_gpio,	\
		.active_low = 1,		\
		.desc = #_id,			\
		.type = EV_KEY,			\
		.wakeup = _iswake,		\
		.debounce_interval = 25,	\
	}

static struct gpio_keys_button e1853_keys[] = {
	[0] = GPIO_KEY(KEY_HOME, PV7, 0),
	[1] = GPIO_KEY(KEY_BACK, PN4, 0),
	[2] = GPIO_KEY(KEY_MENU, PZ4, 0),
	[3] = GPIO_KEY(KEY_VOLUMEUP, PZ2, 0),
	[4] = GPIO_KEY(KEY_VOLUMEDOWN, PN5, 0),

};

static struct gpio_keys_platform_data e1853_ada_hwsw_platform_data = {
	.buttons	= e1853_keys,
	.nbuttons	= ARRAY_SIZE(e1853_keys),
};

static struct platform_device e1853_ada_hwsw_device = {
	.name   = "ada-hwsw",
	.id     = 0,
	.dev    = {
		.platform_data  = &e1853_ada_hwsw_platform_data,
	},
};
#endif
/* Add @FCT 20130517 end */
static struct platform_device *e1853_devices[] __initdata = {
	&tegra_pmu_device,
#if defined(CONFIG_TEGRA_IOVMM_SMMU)
	&tegra_smmu_device,
#endif
#if defined(CONFIG_TEGRA_AVP)
	&tegra_avp_device,
#endif
#if defined(CONFIG_RTC_DRV_TEGRA)
	&tegra_rtc_device,
#endif
	&tegra_camera,
#if defined(CONFIG_BCM4329_RFKILL)
	&e1853_bcm4329_rfkill_device,
#endif

#if defined(CONFIG_CRYPTO_DEV_TEGRA_SE)
	&tegra_se_device,
#endif

/* Add @FCT 20130517 start */
#ifdef CONFIG_KEYBOARD_ADA_HWSW
	&e1853_ada_hwsw_device,
#endif
/* Add @FCT 20130517 end */
	&tegra_wdt_device
};

/* FUJITSU TEN:2012-09-18 delete touch function of the vibrante. start */
#if 0
/* FUJITSU TEN:2012-09-18 delete touch function of the vibrante. end */
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT

#define MXT_CONFIG_CRC  0xD62DE8
static const u8 config[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0x32, 0x0A, 0x00, 0x14, 0x14, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x00, 0x00,
	0x1B, 0x2A, 0x00, 0x20, 0x3C, 0x04, 0x05, 0x00,
	0x02, 0x01, 0x00, 0x0A, 0x0A, 0x0A, 0x0A, 0xFF,
	0x02, 0x55, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x64, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23,
	0x00, 0x00, 0x00, 0x05, 0x0A, 0x15, 0x1E, 0x00,
	0x00, 0x04, 0xFF, 0x03, 0x3F, 0x64, 0x64, 0x01,
	0x0A, 0x14, 0x28, 0x4B, 0x00, 0x02, 0x00, 0x64,
	0x00, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x08, 0x10, 0x3C, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define MXT_CONFIG_CRC_SKU2000  0xA24D9A
static const u8 config_sku2000[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0x32, 0x0A, 0x00, 0x14, 0x14, 0x19,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x00, 0x00,
	0x1B, 0x2A, 0x00, 0x20, 0x3A, 0x04, 0x05, 0x00, /* 23=thr 2 di */
	0x04, 0x04, 0x41, 0x0A, 0x0A, 0x0A, 0x0A, 0xFF,
	0x02, 0x55, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, /* 0A=limit */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23,
	0x00, 0x00, 0x00, 0x05, 0x0A, 0x15, 0x1E, 0x00,
	0x00, 0x04, 0x00, 0x03, 0x3F, 0x64, 0x64, 0x01,
	0x0A, 0x14, 0x28, 0x4B, 0x00, 0x02, 0x00, 0x64,
	0x00, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x08, 0x10, 0x3C, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static struct mxt_platform_data atmel_mxt_info = {
	.x_line         = 27,
	.y_line         = 42,
	.x_size         = 768,
	.y_size         = 1366,
	.blen           = 0x20,
	.threshold      = 0x3C,
	.voltage        = 3300000,              /* 3.3V */
	.orient         = 5,
	.config         = config,
	.config_length  = 157,
	.config_crc     = MXT_CONFIG_CRC,
	.irqflags       = IRQF_TRIGGER_FALLING,
	.read_chg       = NULL,
};

static struct i2c_board_info __initdata atmel_i2c_info[] = {
	{
		I2C_BOARD_INFO("atmel_mxt_ts", 0x5A),
		.irq = TEGRA_GPIO_TO_IRQ(TOUCH_GPIO_IRQ_ATMEL_T9),
		.platform_data = &atmel_mxt_info,
	}
};

static __initdata struct tegra_clk_init_table spi_clk_init_table[] = {
	/* name         parent          rate            enabled */
	{ "sbc1",       "pll_p",        52000000,       true},
	{ NULL,         NULL,           0,              0},
};

int __init e1853_touch_init(void)
{
	tegra_gpio_enable(TOUCH_GPIO_IRQ_ATMEL_T9);
	tegra_gpio_enable(TOUCH_GPIO_RST_ATMEL_T9);

	gpio_request(TOUCH_GPIO_IRQ_ATMEL_T9, "atmel-irq");
	gpio_direction_input(TOUCH_GPIO_IRQ_ATMEL_T9);

	gpio_request(TOUCH_GPIO_RST_ATMEL_T9, "atmel-reset");
	gpio_direction_output(TOUCH_GPIO_RST_ATMEL_T9, 0);
	usleep_range(1000, 2000);
	gpio_set_value(TOUCH_GPIO_RST_ATMEL_T9, 1);
	msleep(100);

	atmel_mxt_info.config = config_sku2000;
	atmel_mxt_info.config_crc = MXT_CONFIG_CRC_SKU2000;

	i2c_register_board_info(TOUCH_BUS_ATMEL_T9, atmel_i2c_info, 1);

	return 0;
}

#endif /* CONFIG_TOUCHSCREEN_ATMEL_MXT */
/* FUJITSU TEN:2012-09-18 delete touch function of the vibrante. start */
#endif
/* FUJITSU TEN:2012-09-18 delete touch function of the vibrante. end */

#if defined(CONFIG_USB_G_ANDROID)
static struct tegra_usb_platform_data tegra_udc_pdata = {
	.port_otg = false,
	.has_hostpc = true,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_DEVICE,
	.u_data.dev = {
		.vbus_pmu_irq = 0,
		.vbus_gpio = -1,
		.charging_supported = false,
		.remote_wakeup_supported = false,
	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.idle_wait_delay = 17,
		.elastic_limit = 16,
		.term_range_adj = 6,
		.xcvr_setup = 63,
		.xcvr_setup_offset = 6,
		.xcvr_use_fuses = 1,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_use_lsb = 1,
	},
};
#else
static struct tegra_usb_platform_data tegra_ehci1_utmi_pdata = {
	.port_otg = false,
	.has_hostpc = true,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.vbus_gpio = -1,
		.vbus_reg = NULL,
		.hot_plug = false,
		.remote_wakeup_supported = false,
		.power_off_on_suspend = true,
	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.idle_wait_delay = 17,
		.elastic_limit = 16,
		.term_range_adj = 6,
		.xcvr_setup = 63,
		.xcvr_setup_offset = 6,
		.xcvr_use_fuses = 1,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_use_lsb = 1,
	},
};
#endif
static struct tegra_usb_platform_data tegra_ehci2_utmi_pdata = {
	.port_otg = false,
	.has_hostpc = true,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.vbus_gpio = -1,
		.vbus_reg = NULL,
		.hot_plug = false,
		.remote_wakeup_supported = false,
		.power_off_on_suspend = true,
	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.idle_wait_delay = 17,
		.elastic_limit = 16,
		.term_range_adj = 6,
		.xcvr_setup = 63,
		.xcvr_setup_offset = 6,
		.xcvr_use_fuses = 1,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_use_lsb = 1,
	},
};

static struct tegra_usb_platform_data tegra_ehci3_utmi_pdata = {
	.port_otg = false,
	.has_hostpc = true,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.vbus_gpio = -1,
		.vbus_reg = NULL,
		.hot_plug = false,
		.remote_wakeup_supported = false,
		.power_off_on_suspend = true,
	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.idle_wait_delay = 17,
		.elastic_limit = 16,
		.term_range_adj = 6,
		.xcvr_setup = 63,
		.xcvr_setup_offset = 6,
		.xcvr_use_fuses = 1,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_use_lsb = 1,
	},
};

static void e1853_usb_init(void)
{
	/* Need to parse sku info to decide host/device mode */

	/* G_ANDROID require device mode */
#if defined(CONFIG_USB_G_ANDROID)
	tegra_udc_device.dev.platform_data = &tegra_udc_pdata;
	platform_device_register(&tegra_udc_device);
/* FUJITSU TEN:2013-01-07 USB automated recognition DELETE start */
//#else
//	tegra_ehci1_device.dev.platform_data = &tegra_ehci1_utmi_pdata;
//	platform_device_register(&tegra_ehci1_device);
/* FUJITSU TEN:2013-01-07 USB automated recognition DELETE end */
#endif
	tegra_ehci2_device.dev.platform_data = &tegra_ehci2_utmi_pdata;
	platform_device_register(&tegra_ehci2_device);

	tegra_ehci3_device.dev.platform_data = &tegra_ehci3_utmi_pdata;
	platform_device_register(&tegra_ehci3_device);
/* FUJITSU TEN:2013-01-07 USB automated recognition ADD start */
#if !defined(CONFIG_USB_G_ANDROID)
	tegra_ehci1_device.dev.platform_data = &tegra_ehci1_utmi_pdata;
	platform_device_register(&tegra_ehci1_device);
#endif
/* FUJITSU TEN:2013-01-07 USB automated recognition ADD end */
}

/* FUJITSU TEN:2012-11-07 Suspend Resume start */
#ifdef CONFIG_ADA_BUDET_DETECT
static struct platform_device Bu_DET = {
    .name = "budet_dev",
    .id = -1,
};

static void e1853_Bu_DET_init(void) {
    platform_device_register(&Bu_DET);

    /* set the corresponding pins as GPIO */
    /* set in the following source file */
    /* arch/arm/mach-tegra/board-p1852-pinmux.c */

    /* request the GPIO resource */
    gpio_request(TG_BU_DET_INT, "TG_BU_DET_INT");
    gpio_request(TG_WAKEUP_REQ, "TG_WAKEUP_REQ");
/* Design_document_PlanG_0829_FTEN Start */
    gpio_request(TG_WAKEUP1, "TG_WAKEUP1");
    gpio_request(TG_WAKEUP2, "TG_WAKEUP2");
/* Design_document_PlanG_0829_FTEN End */
    gpio_request(TG_WAKEUP3, "TG_WAKEUP3");
/* FUJITSU TEN:2013-07-01 HDMI NRESET start */
    gpio_request(TEGRA_GPIO_PV6, "TEGRA_GPIO_PV6");
/* FUJITSU TEN:2013-07-01 HDMI NRESET end   */
/* FUJITSU TEN:2012-10-30 Power Sequence TG_WAKEUP1/TG_VERUP set Low start */
    gpio_request(TG_VERUP, "TG_VERUP");
/* FUJITSU TEN:2012-10-30 Power Sequence TG_WAKEUP1/TG_VERUP set Low end   */
/* 20120914 Bu High Start */
    gpio_request(DEV_SW_BUDET, "DEV_SW_BUDET");
/* 20120914 Bu High End */

    /* FUJITSU TEN:2012-12-21 HDMI SUPPORT BEGIN */
    gpio_request(TEGRA_GPIO_PA0, "CLK32K_OUT");
    gpio_direction_output(TEGRA_GPIO_PA0, 1);
    gpio_request(TEGRA_GPIO_PCC3, "SDMMC4_RST_N");
    gpio_direction_output(TEGRA_GPIO_PCC3, 1);
    /* FUJITSU TEN:2012-12-21 HDMI SUPPORT END */

    tegra_gpio_enable(TG_BU_DET_INT);
    tegra_gpio_enable(TG_WAKEUP_REQ);
/* Design_document_PlanG_0829_FTEN Start */
    tegra_gpio_enable(TG_WAKEUP1);
    tegra_gpio_enable(TG_WAKEUP2);
/* Design_document_PlanG_0829_FTEN End */
    tegra_gpio_enable(TG_WAKEUP3);
/* FUJITSU TEN:2013-07-01 HDMI NRESET start */
    tegra_gpio_enable(TEGRA_GPIO_PV6);
/* FUJITSU TEN:2013-07-01 HDMI NRESET end   */
/* FUJITSU TEN:2012-10-30 Power Sequence TG_WAKEUP1/TG_VERUP set Low start */
    tegra_gpio_enable(TG_VERUP);
/* FUJITSU TEN:2012-10-30 Power Sequence TG_WAKEUP1/TG_VERUP set Low end   */
/* 20120914 Bu High Start */
    tegra_gpio_enable(DEV_SW_BUDET);
/* 20120914 Bu High End */

    /* set the GPIO direction */
    gpio_direction_input(TG_BU_DET_INT);
    gpio_direction_input(TG_WAKEUP_REQ);
/* Design_document_PlanG_0829_FTEN Start */
    gpio_direction_output(TG_WAKEUP1, 1);
    gpio_direction_output(TG_WAKEUP2, 1);

	/* 2013-03-07 changed : initial High to Low start */
    gpio_direction_output(TG_WAKEUP3, 0);
/* FUJITSU TEN:2013-07-05 HDMI NRESET start */
    gpio_direction_output(TEGRA_GPIO_PV6, 0);
/* FUJITSU TEN:2013-07-05 HDMI NRESET end   */
	/* 2013-03-07 changed : initial High to Low end */

/* FUJITSU TEN:2012-10-30 Power Sequence TG_WAKEUP1/TG_VERUP set Low start */
    if(ada_get_board_start_mode() == MODE_RECOVERY) {
        gpio_direction_output(TG_VERUP, 1);
        printk("TG_VERUP HIGH");
    }
    else {
        gpio_direction_output(TG_VERUP, 0);
        printk("TG_VERUP LOW");
    }
/* FUJITSU TEN:2012-10-30 Power Sequence TG_WAKEUP1/TG_VERUP set Low end   */
/* Design_document_PlanG_0829_FTEN End */

/* 20120914 Bu High Start */
    gpio_direction_output(DEV_SW_BUDET, 1);
/* 20120914 Bu High End */

    /* setting pullup/pulldown of GPIO is not needed as outside pullup. */
    gpio_export(TG_WAKEUP3,false);
/* FUJITSU TEN:2013-07-01 HDMI NRESET start */
    gpio_export(TEGRA_GPIO_PV6,false);
/* FUJITSU TEN:2013-07-01 HDMI NRESET end   */
/* FUJITSU TEN:2013-12-10 #54090 start */
    gpio_export(TEGRA_GPIO_PS0,false);  /* ACCOFF */
/* FUJITSU TEN:2013-12-10 #54090 end */

}
/* FUJITSU TEN:2012-11-07 Suspend Resume end */

/* FUJITSU TEN:2012-10-30 Power Sequence TG_WAKEUP1/TG_VERUP Low Start */
extern void tps6591x_power_off(void);
static void e1853_pm_power_off(void)
{
    /* inform SH of termination of TEGRA */
    gpio_direction_output(TG_VERUP, 0);
    /* FUJITSU TEN:2013-12-04 Stop PMU start */
    tps6591x_power_off();
    /* FUJITSU TEN:2013-12-04 Stop PMU end   */
    /* wait for Shutdown/Reboot from SH */
    local_irq_disable();
    while (1);
}

static void e1853_pm_power_off_init(void)
{
    pm_power_off = e1853_pm_power_off;
}

/* FUJITSU TEN:2012-12-11 Virtual device START */
static struct platform_device resume_notifier_pfdev = {
    .name = "resumenotifier",
    .id = -1,
};

static struct platform_device user_process_killer_pfdev = {
    .name = "userprocesskiller",
    .id = -1,
};

static void p1852_virtual_device_init(void)
{
    platform_device_register(&resume_notifier_pfdev);
    platform_device_register(&user_process_killer_pfdev);
}
/* FUJITSU TEN:2012-12-11 Virtual device END   */

#else
static void e1853_Bu_DET_init(void) {
	gpio_request(TG_WAKEUP1, "TG_WAKEUP1");
	gpio_request(TG_WAKEUP2, "TG_WAKEUP2");
	gpio_request(TG_WAKEUP3, "TG_WAKEUP3");

	/* Design_document_PlanG_0829_FTEN Start */
    tegra_gpio_enable(TG_WAKEUP1);
    tegra_gpio_enable(TG_WAKEUP2);
    /* Design_document_PlanG_0829_FTEN End */
    tegra_gpio_enable(TG_WAKEUP3);

    gpio_direction_output(TG_WAKEUP1, 1);
	gpio_direction_output(TG_WAKEUP2, 1);
	gpio_direction_output(TG_WAKEUP3, 1);
	printk("e1853_Bu_DET_init: Enable TG_WAKEUPx pins(for debug)\n");
}
#endif /* CONFIG_ADA_BUDET_DETECT */
/* FUJITSU TEN:2012-10-30 Power Sequence TG_WAKEUP1/TG_VERUP set Low End   */

// ADA LOG 2013-0418
#if 1
static struct platform_device SHRst_INTR = {
    .name = "shrstintr_dev",
    .id   = -1,
};

static void e1853_SHRst_init ( void ) {
    int rtn; 

    rtn = platform_device_register ( &SHRst_INTR );
    printk ( KERN_INFO "platform_device_register() devname = shrstintr_dev rtn = %d\n", rtn );

    rtn = gpio_request ( TG_E_STBY_REQ, "TG_E_STBY_REQ" );
    printk ( KERN_INFO "gpio_request() TG_E_STBY_REQ rtn = %d\n", rtn );

    tegra_gpio_enable ( TG_E_STBY_REQ );
    printk ( KERN_INFO "tegra_gpio_enable() TG_E_STBY_REQ\n" );

    rtn = gpio_direction_input ( TG_E_STBY_REQ );
    printk ( KERN_INFO "gpio_direction_input() TG_E_STBY_REQ rtn = %d\n", rtn );

    return;
}
#endif
// ADA LOG 2013-0418

/* FUJITSU TEN 2013-05-27 NOR-Timing Mod start */
#if 0
static struct tegra_nor_platform_data e1853_nor_data = {
	.flash = {
		.map_name = "cfi_probe",
		.width = 2,
	},
	.chip_parms = {
		.MuxMode = NorMuxMode_ADNonMux,
		.ReadMode = NorReadMode_Page,
		.PageLength = NorPageLength_8Word,
		.ReadyActive = NorReadyActive_WithData,
		/* FIXME: Need to use characterized value */
		.timing_default = {
			.timing0 = 0x30300263,
			.timing1 = 0x00030302,
		},
		.timing_read = {
			.timing0 = 0x30300263,
			.timing1 = 0x00030302,
		},
	},
	.gmi_rst_n_gpio = TEGRA_GPIO_PI4,
	.gmi_oe_n_gpio = TEGRA_GPIO_PI1,
};
#else
static struct tegra_nor_platform_data e1853_nor_data = {
    .flash = {
        .map_name = "cfi_probe",
        .width = 2,
    },
    .chip_parms = {
        .MuxMode = NorMuxMode_ADNonMux,
        .ReadMode = NorReadMode_Page,
        .PageLength = NorPageLength_8Word,
        .ReadyActive = NorReadyActive_WithData,
        /* FIXME: Need to use characterized value */
        .timing_default = {
            .timing0 = 0x30700273,
            .timing1 = 0x00030303,
        },
        .timing_read = {
            .timing0 = 0x30700273,
            .timing1 = 0x00030303,
        },
    },
    .gmi_rst_n_gpio = TEGRA_GPIO_PI4,
    .gmi_oe_n_gpio = TEGRA_GPIO_PI1,
};
#endif
/* FUJITSU TEN 2013-05-27 NOR-Timing Mod end */

static void e1853_nor_init(void)
{
	tegra_nor_device.resource[2].end = TEGRA_NOR_FLASH_BASE + SZ_64M - 1;
	tegra_nor_device.dev.platform_data = &e1853_nor_data;
	platform_device_register(&tegra_nor_device);
}

#ifdef CONFIG_SATA_AHCI_TEGRA
static struct tegra_ahci_platform_data ahci_plat_data = {
	.gen2_rx_eq = 7,
};

static void e1853_sata_init(void)
{
	tegra_sata_device.dev.platform_data = &ahci_plat_data;
	platform_device_register(&tegra_sata_device);
}
#else
static void e1853_sata_init(void) { }
#endif

/* FUJITSU TEN:2012-07-03 Bluetooth use UART start */
/* wl128x BT, FM, GPS connectivity chip */
#define TI_ST_UART_DEV_NAME "/dev/ttyHS4"

static unsigned long retry_suspend = 0;

int plat_kim_suspend(struct platform_device *pdev, pm_message_t state)
{
	int status = -1;

	do
	{
		struct kim_data_s *kim_gdata = NULL;
		struct st_data_s *core_data = NULL;

		kim_gdata = dev_get_drvdata(&pdev->dev);
		if ( ! kim_gdata )
		{
			break;
		}

		core_data = kim_gdata->core_data;
		if ( ! core_data )
		{
			break;
		}

		if (st_ll_getstate(core_data) != ST_LL_INVALID)
		{
			while(st_ll_getstate(core_data) != ST_LL_ASLEEP && (retry_suspend++ < 5))
			{
				break;
			}
		}

		status = 0;
	}
	while ( 0 );

	return status;
}

int plat_kim_resume(struct platform_device* pdev)
{
	retry_suspend = 0;
	return 0;
}

struct ti_st_plat_data wilink_pdata = {
	.nshutdown_gpio	= GPIO_BT_EN,
	.dev_name	= TI_ST_UART_DEV_NAME,
	.flow_cntrl	= 1,
	.baud_rate	= 3000000,
	.suspend	= plat_kim_suspend,
	.resume		= plat_kim_resume,
};

static struct platform_device wl128x_device = {
	.name		= "kim",
	.id		= -1,
	.dev.platform_data = &wilink_pdata,
};

static struct platform_device btwilink_device = {
	.name = "btwilink",
	.id = -1,
};

static noinline void __init e1853_wl12xx_init(void)
{
	pr_info("e1853_wl12xx_init in");

	platform_device_register(&wl128x_device);
	platform_device_register(&btwilink_device);
}
/* FUJITSU TEN:2012-07-03 Bluetooth use UART end */

/* FUJITSU TEN:2012-06-28 touch init add. start */
static u8 mxt_read_chg(void)
{
	MXT_TS_INFO_LOG( "start.\n");
	MXT_TS_DEBUG_LOG( "TOUCH_GPIO_IRQ_ATMEL_T9 : %d\n",
		gpio_get_value(TOUCH_GPIO_IRQ_ATMEL_T9));

	return gpio_get_value(TOUCH_GPIO_IRQ_ATMEL_T9);
}

static void mxt_exec_tsns_reset(int reset_time)
{
	MXT_TS_INFO_LOG( "start.\n");

	MXT_TS_DEBUG_LOG( "TOUCH_GPIO_RST_ATMEL_T9 : %d\n",
		gpio_get_value(TOUCH_GPIO_RST_ATMEL_T9));
	MXT_TS_DEBUG_LOG( "TOUCH_GPIO_IRQ_ATMEL_T9 : %d\n",
		gpio_get_value(TOUCH_GPIO_IRQ_ATMEL_T9));

	gpio_set_value( TOUCH_GPIO_RST_ATMEL_T9, 0 );

	MXT_TS_DEBUG_LOG( "TOUCH_GPIO_RST_ATMEL_T9 set low. : %d\n",
		gpio_get_value(TOUCH_GPIO_RST_ATMEL_T9));
	MXT_TS_DEBUG_LOG( "TOUCH_GPIO_IRQ_ATMEL_T9 : %d\n",
		gpio_get_value(TOUCH_GPIO_IRQ_ATMEL_T9));
	MXT_TS_DEBUG_LOG( "TSNS_RESET_TIME : %d\n", reset_time );
	msleep( reset_time );

	gpio_set_value( TOUCH_GPIO_RST_ATMEL_T9, 1 );

	MXT_TS_DEBUG_LOG( "TOUCH_GPIO_RST_ATMEL_T9 set high. : %d\n",
		gpio_get_value(TOUCH_GPIO_RST_ATMEL_T9));
	MXT_TS_DEBUG_LOG( "TOUCH_GPIO_IRQ_ATMEL_T9 : %d\n",
		gpio_get_value(TOUCH_GPIO_IRQ_ATMEL_T9));

	MXT_TS_DEBUG_LOG( "end.\n" );
}

static void mxt_low_tsns_reset(void)
{
	MXT_TS_INFO_LOG( "start.\n");

	gpio_set_value( TOUCH_GPIO_RST_ATMEL_T9, 0 );

	MXT_TS_DEBUG_LOG( "end.\n" );
}

static struct mxt_platform_data atmel_mxt_info = {
	.irqflags       = IRQF_TRIGGER_FALLING,
	.read_chg       = &mxt_read_chg,
	.tsns_reset	= &mxt_exec_tsns_reset,
	.low_tsns_reset	= &mxt_low_tsns_reset,
};

static struct i2c_board_info __initdata atmel_i2c_info[] = {
	{
		I2C_BOARD_INFO("atmel_mxt_ts", MXT540E_I2C_ADDR1),
		.irq = TEGRA_GPIO_TO_IRQ(TOUCH_GPIO_IRQ_ATMEL_T9),
		.platform_data = &atmel_mxt_info,
	}
};

static void __init e1853_touch_init(void)
{
	MXT_TS_INFO_LOG( "start.\n" );

	gpio_request(TOUCH_GPIO_IRQ_ATMEL_T9, "atmel-irq");
	gpio_request(TOUCH_GPIO_RST_ATMEL_T9, "atmel-reset");

	gpio_direction_output(TOUCH_GPIO_RST_ATMEL_T9, 1);
	gpio_direction_input(TOUCH_GPIO_IRQ_ATMEL_T9);

	tegra_gpio_enable(TOUCH_GPIO_IRQ_ATMEL_T9);
	tegra_gpio_enable(TOUCH_GPIO_RST_ATMEL_T9);

	gpio_set_value(TOUCH_GPIO_RST_ATMEL_T9, 1);
	msleep(100);

	i2c_register_board_info(TOUCH_BUS_ATMEL_T9, atmel_i2c_info, 1);

	MXT_TS_DEBUG_LOG( "end.\n" );
}
/* FUJITSU TEN:2012-06-28 touch init add. end */

/* FUJITSU TEN:2012-07-11 over current protection init add. start */
void set_usbocp_usben(int funcidx, int value)
{
	if (funcidx == USBOCP2_EN) {
		gpio_set_value(USBOCP_GPIO_USB2_EN, value);
	}
	else if (funcidx == USBOCP3_EN) {
		gpio_set_value(USBOCP_GPIO_USB3_EN, value);
	}
}
EXPORT_SYMBOL(set_usbocp_usben);

int get_usbocp_pgood(int funcidx)
{
       int usbocp = -1;

	if (funcidx == USBOCP2_PGOOD) {
		usbocp = gpio_get_value(USBOCP_GPIO_USB2_PGOOD);
	}
	else if (funcidx == USBOCP3_PGOOD) {
		usbocp = gpio_get_value(USBOCP_GPIO_USB3_PGOOD);
	}
	USBOCP_DEBUG_LOG("USB PGOOD(%d) is %d.\n", funcidx, usbocp);
	return usbocp;
}
EXPORT_SYMBOL(get_usbocp_pgood);

static void __init e1853_usbocp_init(void)
{
	USBOCP_DEBUG_LOG("start.\n");
	gpio_request(USBOCP_GPIO_USB2_PGOOD, "usb2_pgood");
	gpio_request(USBOCP_GPIO_USB3_PGOOD, "usb3_pgood");
	gpio_request(USBOCP_GPIO_USB2_EN, "usb2_en");
	gpio_request(USBOCP_GPIO_USB3_EN, "usb3_en");

	gpio_direction_input(USBOCP_GPIO_USB2_PGOOD);
	gpio_direction_input(USBOCP_GPIO_USB3_PGOOD);
	gpio_direction_output(USBOCP_GPIO_USB2_EN, 0);
	gpio_direction_output(USBOCP_GPIO_USB3_EN, 0);

	tegra_gpio_enable(USBOCP_GPIO_USB2_PGOOD);
	tegra_gpio_enable(USBOCP_GPIO_USB3_PGOOD);
	tegra_gpio_enable(USBOCP_GPIO_USB2_EN);
	tegra_gpio_enable(USBOCP_GPIO_USB3_EN);

	USBOCP_DEBUG_LOG("end.\n");
}
/* FUJITSU TEN:2012-07-11 over current protection init add. end */

/* FUJITSU TEN:2012-08-30 accelerometer & gyro init add. start */
static struct i2c_board_info __initdata lis331dlh_i2c_info[] = {
	{
		I2C_BOARD_INFO("lis331dlh", LIS331DLH_I2C_ADDR),
	}
};

static struct i2c_board_info __initdata tag206n_i2c_info[] = {
	{
		I2C_BOARD_INFO("tag206n", TAG206N_I2C_ADDR),
	}
};

static void __init e1853_lis331dlh_init(void)
{
	i2c_register_board_info(TEGRA_LIS331DLH_I2C_BUS, lis331dlh_i2c_info, 1);
}

static void __init e1853_tag206n_init(void)
{
	i2c_register_board_info(TEGRA_TAG206N_I2C_BUS, tag206n_i2c_info, 1);
}
/* FUJITSU TEN:2012-08-30 accelerometer & gyro init add. end */


/* FUJITSU TEN:2012-07-04 dsp init add. start */
#ifdef CONFIG_AK7736_DSP
static void set_ak_dsp_pdn(int val)
{
	gpio_set_value(TEGRA_GPIO_AK_DSP_PDN, val );
}

static void set_ak_adc_pdn(int val)
{
	gpio_set_value(TEGRA_GPIO_AK_ADC_PDN, val );
}
static int get_ak_dsp_err (void)
{
    return gpio_get_value(TEGRA_GPIO_AK_DSP_ERR);
}

static int  get_ext_mic_stat(void)
{
    return (gpio_get_value(TEGRA_EXT_MIC_OPEN) << 1) |
            gpio_get_value(TEGRA_EXT_MIC_SHORT);
}

static void set_ext_mic_power(int val)
{
    gpio_set_value(TEGRA_EXT_MIC_PWRON, val);
}
static void set_ext_mic_detoff(int val)
{
    gpio_set_value(TEGRA_EXT_MIC_DET_OFF, val);
}

static void set_ext_mic_mode(int val)
{
    gpio_set_value(TEGRA_EXT_MIC_MODE0,  val & 0x01);
    gpio_set_value(TEGRA_EXT_MIC_MODE1, (val & 0x02) >> 1);
}

static int get_ext_mic_mode(void)
{
    return ((gpio_get_value(TEGRA_EXT_MIC_MODE1) << 1) |
            gpio_get_value(TEGRA_EXT_MIC_MODE0));
}

static struct akdsp_platform_data ak7736_dsp_info = {
	.irqflags             = IRQF_TRIGGER_FALLING,
	.set_ak_dsp_pdn       = &set_ak_dsp_pdn,
	.set_ak_adc_pdn       = &set_ak_adc_pdn,
	.get_ak_dsp_err       = &get_ak_dsp_err,

	.get_ext_mic_stat     = &get_ext_mic_stat,
	.set_ext_mic_power    = &set_ext_mic_power,
	.set_ext_mic_detoff   = &set_ext_mic_detoff,
	.set_ext_mic_mode     = &set_ext_mic_mode,
	.get_ext_mic_mode     = &get_ext_mic_mode,
};

static struct i2c_board_info __initdata ak7736_i2c_info[] = {
	{
		I2C_BOARD_INFO("ak7736_dsp", AK7736_DSP_I2C_ADDR),
		.platform_data = &ak7736_dsp_info,
		.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_AK_DSP_ERR),
	}
};

static void __init e1853_ak7736_dsp_init(void)
{
	gpio_request(TEGRA_GPIO_AK_DSP_PDN, "ak7736-pdn");
	gpio_request(TEGRA_GPIO_AK_ADC_PDN, "ak5355-pdn");
	gpio_request(TEGRA_GPIO_AK_DSP_ERR, "ak7736-err");

	gpio_request(TEGRA_EXT_MIC_OPEN   , "ext-mic-open");
	gpio_request(TEGRA_EXT_MIC_PWRON  , "ext-mic-pwron");
	gpio_request(TEGRA_EXT_MIC_SHORT  , "ext-mic-short");
	gpio_request(TEGRA_EXT_MIC_MODE0  , "ext-mic-mode0");
	gpio_request(TEGRA_EXT_MIC_MODE1  , "ext-mic-mode1");
	gpio_request(TEGRA_EXT_MIC_DET_OFF, "ext-mic-detoff");

	gpio_direction_output(TEGRA_GPIO_AK_DSP_PDN, 0);
	gpio_direction_output(TEGRA_GPIO_AK_ADC_PDN, 0);
	gpio_direction_input (TEGRA_GPIO_AK_DSP_ERR);

	gpio_direction_input (TEGRA_EXT_MIC_OPEN);
	gpio_direction_output(TEGRA_EXT_MIC_PWRON  , 0);
	gpio_direction_input (TEGRA_EXT_MIC_SHORT);
	gpio_direction_output(TEGRA_EXT_MIC_MODE0  , 0);
	gpio_direction_output(TEGRA_EXT_MIC_MODE1  , 0);
	gpio_direction_output(TEGRA_EXT_MIC_DET_OFF, 1);

	tegra_gpio_enable(TEGRA_GPIO_AK_DSP_PDN);
	tegra_gpio_enable(TEGRA_GPIO_AK_ADC_PDN);
	tegra_gpio_enable(TEGRA_GPIO_AK_DSP_ERR);

	tegra_gpio_enable(TEGRA_EXT_MIC_OPEN   );
	tegra_gpio_enable(TEGRA_EXT_MIC_PWRON  );
	tegra_gpio_enable(TEGRA_EXT_MIC_SHORT  );
	tegra_gpio_enable(TEGRA_EXT_MIC_MODE0  );
	tegra_gpio_enable(TEGRA_EXT_MIC_MODE1  );
	tegra_gpio_enable(TEGRA_EXT_MIC_DET_OFF);

	i2c_register_board_info(TEGRA_AK_DSP_I2C_BUS, ak7736_i2c_info, 1);
}
#endif
/* FUJITSU TEN:2012-07-04 dsp init add. end */

/* FUJITSU TEN:2012-09-21 GPS Antenna init add. start */
u8 gps_ant_read_gps_ant_short(void)
{
	GPS_ANT_INFO_LOG( "start.\n");
	GPS_ANT_DEBUG_LOG( "/GPS_ANT_SHORT : %d\n",
		gpio_get_value(GPS_ANT_GPIO_GPS_ANT_SHORT));

	return gpio_get_value(GPS_ANT_GPIO_GPS_ANT_SHORT);
}
EXPORT_SYMBOL(gps_ant_read_gps_ant_short);

u8 gps_ant_read_gps_ant_open(void)
{
	GPS_ANT_INFO_LOG( "start.\n");
	GPS_ANT_DEBUG_LOG( "/GPS_ANT_OPEN : %d\n",
		gpio_get_value(GPS_ANT_GPIO_GPS_ANT_OPEN));

	return gpio_get_value(GPS_ANT_GPIO_GPS_ANT_OPEN);
}
EXPORT_SYMBOL(gps_ant_read_gps_ant_open);

static void __init e1853_gps_ant_init(void)
{
	GPS_ANT_INFO_LOG( "start.\n" );

	gpio_request(GPS_ANT_GPIO_GPS_ANT_SHORT, "gps_ant_short");
	gpio_request(GPS_ANT_GPIO_GPS_ANT_OPEN, "gps_ant_open");

	gpio_direction_input(GPS_ANT_GPIO_GPS_ANT_SHORT);
	gpio_direction_input(GPS_ANT_GPIO_GPS_ANT_OPEN);

	tegra_gpio_enable(GPS_ANT_GPIO_GPS_ANT_SHORT);
	tegra_gpio_enable(GPS_ANT_GPIO_GPS_ANT_OPEN);

	GPS_ANT_DEBUG_LOG( "end.\n" );
}
/* FUJITSU TEN:2012-09-21 GPS Antenna init add. end */

/* FUJITSU TEN:2012-11-13 TMP401 init add. start */
static struct i2c_board_info __initdata tmp401_i2c_info[] = {
	{
		I2C_BOARD_INFO("tmp401", TMP401_I2C_ADDR),
	}
};

static void __init e1853_tmp401_init(void)
{
	i2c_register_board_info(TMP401_I2C_BUS, tmp401_i2c_info, 1);
}

/* FUJITSU TEN:2012-11-13 TMP401 init add. end */

/* FUJITSU TEN:2012-12-05 TPS6591X init add. start */
static struct i2c_board_info __initdata TPS6591X_i2c_info[] = {
	{
		I2C_BOARD_INFO("TPS6591X", TPS6591X_I2C_ADDR),
	}
};

static void __init e1853_TPS6591X_init(void)
{
	i2c_register_board_info(TPS6591X_I2C_BUS, TPS6591X_i2c_info, 1);
}
/* FUJITSU TEN:2012-12-05 TPS6591X init add. end */

/* FUJITSU TEN:2013-1-17 Refresh init add. start */
u8 get_refresh_status(void)
{
	return (gpio_get_value(WAKEUP_STATE1) == gpio_get_value(WAKEUP_STATE2));
}
EXPORT_SYMBOL(get_refresh_status);

static void __init e1853_refresh_init(void)
{
        gpio_request(WAKEUP_STATE1, "WAKEUP_STATE1");
        gpio_request(WAKEUP_STATE2, "WAKEUP_STATE2");
        tegra_gpio_enable(WAKEUP_STATE1);
        tegra_gpio_enable(WAKEUP_STATE2);
        gpio_direction_input(WAKEUP_STATE1);
        gpio_direction_input(WAKEUP_STATE2);
        gpio_export(WAKEUP_STATE1, false);  // false or true
        gpio_export(WAKEUP_STATE2, false);  // false or true
}
/* FUJITSU TEN:2013-1-17 Refresh init add. end */


static void __init tegra_e1853_init(void)
{
	tegra_init_board_info();
	tegra_clk_init_from_table(e1853_clk_init_table);
	e1853_pinmux_init();
	e1853_i2c_init();
#ifdef CONFIG_SENSORS_TMON_TMP411
	register_therm_monitor(&e1853_therm_monitor_data);
#endif
	e1853_regulator_init();
	e1853_suspend_init();
	e1853_i2s_audio_init();
	e1853_gpio_init();
	e1853_uart_init();
	e1853_usb_init();
/* FUJITSU TEN:2012-07-11 over current protection init add. start */
	e1853_usbocp_init();
/* FUJITSU TEN:2012-07-11 over current protection init add. end */
	tegra_io_dpd_init();
	e1853_sdhci_init();
	e1853_spi_init();
	platform_add_devices(e1853_devices, ARRAY_SIZE(e1853_devices));
/* FUJITSU TEN:2012-09-18 delete touch function of the vibrante. start */
#if 0
/* FUJITSU TEN:2012-09-18 delete touch function of the vibrante. end */
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT
	e1853_touch_init();
#endif
/* FUJITSU TEN:2012-09-18 delete touch function of the vibrante. start */
#endif
/* FUJITSU TEN:2012-09-18 delete touch function of the vibrante. end */
	e1853_panel_init();
	e1853_nor_init();
//	e1853_pcie_init();
/* FUJITSU TEN:2013-01-31 #13582 start */
//	e1853_pca953x_init();
/* FUJITSU TEN:2013-01-31 #13582 end */
/* FUJITSU TEN:2012-07-03 Bluetooth use UART start */
	e1853_wl12xx_init();
/* FUJITSU TEN:2012-07-03 Bluetooth use UART end */

/* FUJITSU TEN:2012-06-28 touch init add. start */
	e1853_touch_init();
/* FUJITSU TEN:2012-06-28 touch init add. end */
/* FUJITSU TEN:2012-07-04 dsp init add. start */
#ifdef CONFIG_AK7736_DSP
	e1853_ak7736_dsp_init();
#endif
/* FUJITSU TEN:2012-07-04 dsp init add. end */
/* FUJITSU TEN:2012-08-30 accelerometer & gyro init add. start */
	e1853_lis331dlh_init();
	e1853_tag206n_init();
/* FUJITSU TEN:2012-08-30 accelerometer & gyro init add. end */
/* FUJITSU TEN:2012-09-21 GPS Antenna init add. start */
	e1853_gps_ant_init();
/* FUJITSU TEN:2012-09-21 GPS Antenna init add. end */
/* FUJITSU TEN:2012-11-13 TMP411 init add. start */
	e1853_tmp401_init();
/* FUJITSU TEN:2012-11-13 TMP411 init add. end */
/* FUJITSU TEN:2012-12-05 TPS6591X init add. start */
	e1853_TPS6591X_init();
/* FUJITSU TEN:2012-12-05 TPS6591X init add. end */

// ADA LOG 2013-0418 ADD
#if 1
    e1853_SHRst_init ( );
#endif
// ADA LOG 2013-0418 ADD

/* FUJITSU TEN:2012-11-07 Suspend Resume start */
//#ifdef CONFIG_ADA_BUDET_DETECT
    e1853_Bu_DET_init();
//#endif /* CONFIG_ADA_BUDET_DETECT */
/* FUJITSU TEN:2012-11-07 Suspend Resume end */

/* FUJITSU TEN:2012-10-30 Power Sequence TG_WAKEUP1/TG_VERUP set Low start */
#ifdef CONFIG_ADA_BUDET_DETECT 
    e1853_pm_power_off_init();
#endif /* CONFIG_ADA_BUDET_DETECT */
/* FUJITSU TEN:2012-10-30 Power Sequence TG_WAKEUP1/TG_VERUP set Low end   */

/* FUJITSU TEN:2012-12-11 Virtual device START */
#ifdef CONFIG_ADA_BUDET_DETECT 
    p1852_virtual_device_init();
#endif /* CONFIG_ADA_BUDET_DETECT */
/* FUJITSU TEN:2012-12-11 Virtual device END   */

/* FUJITSU TEN:2013-1-17 Refresh init add. start */
        e1853_refresh_init();
/* FUJITSU TEN:2013-1-17 Refresh init add. end */

/* FUJITSU TEN:2013-03-07 Disable SATA and KEY start */
//	e1853_sata_init();
//	e1853_keys_init();
/* FUJITSU TEN:2013-03-07 Disable SATA and KEY end */
}

static void __init tegra_e1853_reserve(void)
{
#if defined(CONFIG_NVMAP_CONVERT_CARVEOUT_TO_IOVMM)
	tegra_reserve(0, SZ_8M, SZ_8M);
#else
	tegra_reserve(SZ_128M, SZ_8M, SZ_8M);
#endif
}

MACHINE_START(E1853, "e1853")
	.boot_params    = 0x80000100,
	.init_irq       = tegra_init_irq,
	.init_early     = tegra_init_early,
	.init_machine   = tegra_e1853_init,
	.map_io         = tegra_map_common_io,
	.reserve        = tegra_e1853_reserve,
	.timer          = &tegra_timer,
MACHINE_END

/* FUJITSU TEN:2013-04-02 Correction for the reduced ability Start */
static enum start_mode board_start_mode = MODE_NORMAL;
enum start_mode ada_get_board_start_mode(void)
{
    return board_start_mode;
}

static int __init ada_board_start_mode(char* info)
{
    char* p = info;
    const char recovery_mode[] = "osupdatemode";
    if (!strncmp(p, recovery_mode, strlen(recovery_mode)))
        board_start_mode = MODE_RECOVERY;
    else
        board_start_mode = MODE_NORMAL;
    
    return 1;
}

__setup("mode=", ada_board_start_mode);
/* FUJITSU TEN:2013-04-02 Correction for the reduced ability end */

