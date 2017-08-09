/*
 * arch/arm/mach-tegra/board-e1853-sdhci.c
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
/*----------------------------------------------------------------------------*/
// COPYRIGHT(C) FUJITSU TEN LIMITED 2012,2013
/*----------------------------------------------------------------------------*/

#include <linux/resource.h>
#include <linux/platform_device.h>
#include <linux/wlan_plat.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/mmc/host.h>
/* FUJITSU TEN:2012-06-22 WiFi use Header. edit. start */
#include <linux/wl12xx.h>
/* FUJITSU TEN:2012-06-22 WiFi use Header. edit. end */
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
#include <linux/budet_interruptible.h>
#endif
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
#include <linux/nonvolatile.h>
#include <linux/nonvolatile_def.h>
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */

#include <asm/mach-types.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/sdhci.h>
/* FUJITSU TEN :2013-03-07 DriveRec start */
//#include <mach/board_id.h>
/* FUJITSU TEN :2013-03-07 DriveRec end */
/* FUJITSU TEN :2013-11-08 DriveRec start */
#include <linux/lites_trace_wireless.h>
/* FUJITSU TEN :2013-11-08 DriveRec end */
#include <linux/i2c.h>

#include "gpio-names.h"
#include "board.h"
#include "board-e1853.h"
#include "devices.h"

#define E1853_SD1_CD TEGRA_GPIO_PV2
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
#define SLEEP_WL_EN_ON_BEFORE   1200    //0x174
#define SLEEP_WL_EN_OFF_AFTER   10      //0x179
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe start */
#define NONVOL_VALCHK(val) (val!=0x00 && val!=0xFF)
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe end */

/* FUJITSU TEN:2012-06-22 WiFi use code insert edit. start */
// flag for card detection
static int f12arc_wifi_cd = 0;
// callback
static void (*wifi_status_cb)(int card_present, void *dev_id);
// devid of card
static void *wifi_status_cb_devid = NULL;
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
static unsigned int sleep_wl_en_on_before = SLEEP_WL_EN_ON_BEFORE;
static unsigned int sleep_wl_en_off_after = SLEEP_WL_EN_OFF_AFTER;
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */

/* FUJITSU TEN:2013-01-31 #13582 start */
#if 0
static void (*wifi_status_cb) (int card_present, void *dev_id);
static void *wifi_status_cb_devid;
static int e1853_wifi_status_register(void (*callback) (int, void *), void *);
static int e1853_wifi_reset(int on);
static int e1853_wifi_power(int on);
static int e1853_wifi_set_carddetect(int val);

static struct wifi_platform_data e1853_wifi_control = {
	.set_power = e1853_wifi_power,
	.set_reset = e1853_wifi_reset,
	.set_carddetect = e1853_wifi_set_carddetect,
};

static struct platform_device broadcom_wifi_device = {
	.name = "bcm4329_wlan",
	.id = 1,
	.dev = {
		.platform_data = &e1853_wifi_control,
		},
};

#ifdef CONFIG_MMC_EMBEDDED_SDIO
static struct embedded_sdio_data embedded_sdio_data2 = {
	.cccr = {
		 .sdio_vsn = 2,
		 .multi_block = 1,
		 .low_speed = 0,
		 .wide_bus = 0,
		 .high_power = 1,
		 .high_speed = 1,
		 },
	.cis = {
		.vendor = 0x02d0,
		.device = 0x4329,
		},
};
#endif

static int
e1853_wifi_status_register(void (*callback) (int card_present, void *dev_id),
			   void *dev_id)
{
	if (wifi_status_cb)
		return -EBUSY;
	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	return 0;
}

static int e1853_wifi_set_carddetect(int val)
{

	if (wifi_status_cb)
		wifi_status_cb(val, wifi_status_cb_devid);
	else
		pr_warning("%s: Nobody to notify\n", __func__);

	return 0;
}

static int e1853_wifi_power(int on)
{
	int ret;

	if (on) {
		/* Assert WiFi Enable GPIO */
		ret = gpio_request(MISCIO_WF_EN_GPIO, "wifi_en");
		if (ret < 0)
			goto fail;
		gpio_direction_output(MISCIO_WF_EN_GPIO, 1);
		gpio_free(MISCIO_WF_EN_GPIO);

		/* Deassert WiFi RST GPIO */
		ret = gpio_request(MISCIO_WF_RST_GPIO, "wifi_rst");
		if (ret < 0)
			goto fail;
		gpio_direction_output(MISCIO_WF_RST_GPIO, 1);
		gpio_free(MISCIO_WF_RST_GPIO);
	}
	return 0;

fail:
	printk(KERN_ERR "%s: gpio_request failed(%d)\r\n", __func__, ret);
	return ret;
}

static int e1853_wifi_reset(int on)
{
	/*
	 * FIXME: Implement wifi reset
	 */
	return 0;
}

int __init e1853_wifi_init(void)
{
	platform_device_register(&broadcom_wifi_device);
	return 0;
}

static struct tegra_sdhci_platform_data tegra_sdhci_platform_data1 = {
	.mmc_data = {
		     .register_status_notify = e1853_wifi_status_register,
#ifdef CONFIG_MMC_EMBEDDED_SDIO
		     .embedded_sdio = &embedded_sdio_data2,
#endif
		     .built_in = 0,
		     .ocr_mask = MMC_OCR_1V8_MASK,
		     },
#ifndef CONFIG_MMC_EMBEDDED_SDIO
	.pm_flags = MMC_PM_KEEP_POWER,
#endif
	.cd_gpio = -1,
	.wp_gpio = -1,
	.power_gpio = -1,
	.tap_delay = 0x0F,
	.ddr_clk_limit = 30000000,
	.is_8bit = false,
};

static struct tegra_sdhci_platform_data tegra_sdhci_platform_data2 = {
	.cd_gpio = -1,
	.wp_gpio = -1,
	.power_gpio = -1,
	.is_8bit = 1,
	.tap_delay = 0x06,
	.max_clk_limit = 52000000,
	.ddr_clk_limit = 30000000,
	.uhs_mask = MMC_UHS_MASK_DDR50,
	.mmc_data = {
		     .built_in = 1,
		     }
};

static struct tegra_sdhci_platform_data tegra_sdhci_platform_data3 = {
	.cd_gpio = TEGRA_GPIO_PN6,
	.wp_gpio = TEGRA_GPIO_PD4,
	.power_gpio = TEGRA_GPIO_PN7,
	.is_8bit = false,
	/* WAR: Operating SDR104 cards at upto 104 MHz, as signal errors are
	 * seen when they are operated at any higher frequency.
	 */
	.max_clk_limit = 104000000,
	.ddr_clk_limit = 30000000,
	.mmc_data = {
		.ocr_mask = MMC_OCR_2V8_MASK,
	},
	.cd_wakeup_incapable = true,
};

static struct tegra_sdhci_platform_data tegra_sdhci_platform_data4 = {
	.cd_gpio = -1,
	.wp_gpio = -1,
	.power_gpio = -1,
	.is_8bit = true,
	.tap_delay = 0x06,
	.max_clk_limit = 52000000,
	.ddr_clk_limit = 51000000,
	.uhs_mask = MMC_UHS_MASK_DDR50,
	.mmc_data = {
		.built_in = 1,
	}
};
#endif
/* FUJITSU TEN:2013-01-31 #13582 end */

int f12arc_wifi_power(int power_on)
{
	static int power_state;
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
	unsigned long remain;
#endif
/* FUJITSU TEN :2012-11-15 Support_BuDet end */

	if (power_on == power_state)
		return 0;

/* FUJITSU TEN :2013-01-25 Support_#10868 start */
	//pr_info("Powering %s wifi\n", (power_on ? "on" : "off"));
/* FUJITSU TEN :2013-01-25 Support_#10868 end */

	power_state = power_on;

	if (power_on) {
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
		remain = mdelay_budet_interruptible(sleep_wl_en_on_before);
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
		if (remain) {
			//ADA_LOG_WARNING(sdhci,
			//	"Budet Interrupt Occurred. remain :%ld",remain);
			DRC_WL_WIFI(EVT_BUDET+201, remain);
			return -EINTR;
		}
#else /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
		msleep(sleep_wl_en_on_before);
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
#endif /* CONFIG_TI_FASTBOOT */
		gpio_set_value(GPIO_WLAN_EN, 1);
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
	} else {
/* FUJITSU TEN :2013-01-25 Support_#10868 start */
#ifdef CONFIG_TI_FASTBOOT
		if ( get_budet_flag() )
		{
			//ADA_LOG_ERR(sdhci, "Budet Interrupted occured.");
			return -EINTR;
		}
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2013-01-25 Support_#10868 end */

		gpio_set_value(GPIO_WLAN_EN, 0);
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
		remain = mdelay_budet_interruptible(sleep_wl_en_off_after);
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
		if (remain) {
			//ADA_LOG_WARNING(sdhci,
			//	"Budet Interrupt Occurred. remain :%ld",remain);
			DRC_WL_WIFI(EVT_BUDET+202, remain);
			return -EINTR;
		}
#else /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
		msleep(sleep_wl_en_off_after);
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
#endif
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
	}

	return 0;
}
EXPORT_SYMBOL(f12arc_wifi_power);

int f12arc_wifi_set_carddetect(int card_on)
{
	pr_info("Card %s wifi\n", (card_on ? "on" : "off"));

	f12arc_wifi_cd = card_on;
	if (wifi_status_cb)
	{
		wifi_status_cb(card_on, wifi_status_cb_devid);
	}

	return 0;
}
EXPORT_SYMBOL(f12arc_wifi_set_carddetect);

static int f12arc_wifi_status_register(
               void (*callback)(int card_present, void *dev_id),
               void *dev_id)
{
	pr_info("f12arc_wifi_status_register:in\n");
	if (wifi_status_cb) {
		pr_err("f12arc_wifi_status_register:%d\n", -EAGAIN);
		return -EAGAIN;
	}
	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	return 0;
}

unsigned int f12arc_wifi_status(struct device* dev)
{
	pr_info("CardStatus %s wifi\n", (f12arc_wifi_cd ? "on" : "off"));

	return f12arc_wifi_cd;
}
EXPORT_SYMBOL(f12arc_wifi_status);

/* FUJITSU TEN:2012-06-22 WiFi use code insert edit. end */


static struct tegra_sdhci_platform_data tegra_sdhci_platform_data1 = {
/* FUJITSU TEN:2012-06-22 WiFi insert code edit. start */
	.mmc_data = {
		.register_status_notify = f12arc_wifi_status_register,
		.embedded_sdio = NULL,
		.built_in = 1,
		.status = f12arc_wifi_status,
	},
/* FUJITSU TEN:2012-06-22 WiFi insert code edit. end */
/* FUJITSU TEN:2012-06-22 WiFi modify code edit. start */
//	.cd_gpio = E1853_SD1_CD,
	.cd_gpio = -1,
/* FUJITSU TEN:2012-06-22 WiFi modify code edit. end */
	.wp_gpio = -1,
	.power_gpio = -1,
/* FUJITSU TEN:2012-06-22 WiFi modify code edit. start */
//	.is_8bit = false,
/* FUJITSU TEN:2013-06-06 WiFi sdio max clock edit start */
//	.max_clk_limit = 25000000,
	.max_clk_limit = 24000000,
/* FUJITSU TEN:2013-06-06 WiFi sdio max clock edit end */
/* FUJITSU TEN:2012-06-22 WiFi modify code edit. end */
};

static struct tegra_sdhci_platform_data tegra_sdhci_platform_data2 = {
	.cd_gpio = -1,
	.wp_gpio = -1,
	.power_gpio = -1,
	.is_8bit = true,
};

static struct tegra_sdhci_platform_data tegra_sdhci_platform_data4 = {
	.cd_gpio = -1,
	.wp_gpio = -1,
	.power_gpio = -1,
	.is_8bit = true,
};

/* FUJITSU TEN:2012-06-22 WiFi use code insert edit. start */
static struct wl12xx_platform_data f12arc_wlan_data __initdata = {
	.irq = TEGRA_GPIO_TO_IRQ(GPIO_WLAN_IRQ),
	.board_ref_clock = WL12XX_REFCLOCK_26,
	.board_tcxo_clock = WL12XX_TCXOCLOCK_26,
	.platform_quirks = 0,
};

static int __init f12arc_wifi_init(void)
{
	int rc;
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
	uint8_t val = 0;
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
	pr_info("f12arc_wifi_init in");
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
	rc = GetNONVOLATILE(&val, FTEN_NONVOL_TI_SLEEP_WL_EN_ON_BEFORE,
		FTEN_NONVOL_TI_SLEEP_WL_EN_ON_BEFORE_SIZE);
	if( rc != 0 ) {
		//ADA_LOG_WARNING(sdhci, "read failed(FTEN_NONVOL_TI_SLEEP_WL_EN_ON_BEFORE)");
		DRC_WL_SOFTERR(NVL_READ_ERR, 0x0011, rc);
	} else {
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe start */
		if (NONVOL_VALCHK(val)) {
			sleep_wl_en_on_before = val * 10;
		} else {
			//ADA_LOG_WARNING(sdhci, "read abnormal(FTEN_NONVOL_TI_SLEEP_WL_EN_ON_BEFORE)");
			DRC_WL_SOFTERR(NVL_READ_ERR, 0x0012, rc);
		}
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe end */
	}
	rc = GetNONVOLATILE(&val, FTEN_NONVOL_TI_SLEEP_WL_EN_OFF_AFTER,
		FTEN_NONVOL_TI_SLEEP_WL_EN_OFF_AFTER_SIZE);
	if( rc != 0 ) {
		//ADA_LOG_WARNING(sdhci, "read failed(FTEN_NONVOL_TI_SLEEP_WL_EN_OFF_AFTER)");
		DRC_WL_SOFTERR(NVL_READ_ERR, 0x0013, rc);
	} else {
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe start */
		if (NONVOL_VALCHK(val)) {
			sleep_wl_en_off_after = val * 10;
		} else {
			//ADA_LOG_WARNING(sdhci, "read abnormal(FTEN_NONVOL_TI_SLEEP_WL_EN_OFF_AFTER)");
			DRC_WL_SOFTERR(NVL_READ_ERR, 0x0014, rc);
		}
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe end */
	}
/* FUJITSU TEN :2013-02-21 DriveRec start */
	DRC_WL_DRVTRACE("nvw2:%d %d"
		,sleep_wl_en_on_before
		,sleep_wl_en_off_after);
/* FUJITSU TEN :2013-02-21 DriveRec end */
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */

	rc = gpio_request(GPIO_WLAN_EN, "wlan_en");
	if (rc)
		pr_err("WLAN_EN gpio request failed:%d\n", rc);

	rc = gpio_request(GPIO_WLAN_IRQ, "wlan_irq");
	if (rc)
		pr_err("WLAN_IRQ gpio request failed:%d\n", rc);

	rc = gpio_direction_output(GPIO_WLAN_EN, 0);
	if (rc)
		pr_err("WLAN_EN gpio direction configuration failed:%d\n", rc);

	rc = gpio_direction_input(GPIO_WLAN_IRQ);
	if (rc)
		pr_err("WLAN_IRQ gpio direction configuration failed:%d\n", rc);

	tegra_gpio_enable(GPIO_WLAN_EN);
	tegra_gpio_enable(GPIO_WLAN_IRQ);

	rc = wl12xx_set_platform_data(&f12arc_wlan_data);
	if (rc)
		pr_err("Error setting wl12xx data:%d\n", rc);

	pr_info("f12arc_wifi_init:f12arc_wlan_data.irq=%d", f12arc_wlan_data.irq);

	return 0;
}
/* FUJITSU TEN:2012-06-22 WiFi use code insert edit. end */


int __init e1853_sdhci_init(void)
{
/* FUJITSU TEN :2013-03-07 DriveRec start */
//	int is_e1860 = 0;
/* FUJITSU TEN :2013-03-07 DriveRec end */
	tegra_sdhci_device1.dev.platform_data = &tegra_sdhci_platform_data1;
	tegra_sdhci_device2.dev.platform_data = &tegra_sdhci_platform_data2;
/* FUJITSU TEN:2013-01-31 #13582 start */
//	tegra_sdhci_device3.dev.platform_data = &tegra_sdhci_platform_data3;
/* FUJITSU TEN:2013-01-31 #13582 end */
	tegra_sdhci_device4.dev.platform_data = &tegra_sdhci_platform_data4;

/* FUJITSU TEN :2013-03-07 DriveRec start */
//	is_e1860 = tegra_is_board(NULL, "61860", NULL, NULL, NULL);
//	if (is_e1860){
//		tegra_sdhci_platform_data3.mmc_data.ocr_mask = MMC_OCR_3V2_MASK;
//	}
/* FUJITSU TEN :2013-03-07 DriveRec end */

	platform_device_register(&tegra_sdhci_device4);
	platform_device_register(&tegra_sdhci_device2);
	platform_device_register(&tegra_sdhci_device1);
/* FUJITSU TEN:2013-01-31 #13582 start */
//	platform_device_register(&tegra_sdhci_device3);
/* FUJITSU TEN:2013-01-31 #13582 end */

/* FUJITSU TEN:2012-06-22 WiFi use code insert edit. start */
	f12arc_wifi_init();
/* FUJITSU TEN:2012-06-22 WiFi use code insert edit. end */

	return 0;
}
