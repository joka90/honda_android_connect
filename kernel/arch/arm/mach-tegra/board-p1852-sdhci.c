/*
 * arch/arm/mach-tegra/board-p1852-sdhci.c
 *
 * Copyright (C) 2010 Google, Inc.
 *
 * Copyright (c) 2012-2013, NVIDIA CORPORATION.  All rights reserved.
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
/* FUJITSU TEN :2013-02-21 DriveRec start */
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/lites_trace.h>
/* FUJITSU TEN :2013-02-21 DriveRec end */

#include <asm/mach-types.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/sdhci.h>

#include "gpio-names.h"
#include "board.h"
#include "board-p1852.h"
#include "devices.h"


#define P1852_SD1_CD TEGRA_GPIO_PV2
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

/* FUJITSU TEN :2013-02-21 DriveRec start */

//#define LOCAL_DBG

#define WL128X_WIFI_DRIVER	26
#define DRVTRC_ERR_LVL		6

typedef struct _lites_drv_trace_err {
	unsigned char	msg[36];
} lites_drv_trace_err;

static int drc_drvtrace_native(const char *fmt, va_list args)
{
	lites_drv_trace_err errinfo;
	ssize_t size = 0;
	int result;
	int level = DRVTRC_ERR_LVL;
	struct lites_trace_header  trc_header;
	struct lites_trace_format  trc_format;
	unsigned int gid = REGION_TRACE_DRIVER;
	unsigned int tid = LITES_KNL_LAYER + WL128X_WIFI_DRIVER;
	unsigned int tno = LITES_KNL_LAYER + WL128X_WIFI_DRIVER;

	memset(errinfo.msg, 0, sizeof(errinfo.msg));
	size = vscnprintf(errinfo.msg, sizeof(errinfo.msg), fmt, args);

#ifdef LOCAL_DBG
	pr_err("sdhci_drvtracelog: size=%d gid=%d tid=%d tno=%d level=%d msg=%s",
		size, gid, tid, tno, level, errinfo.msg);
#endif
	trc_header.level = level;
	trc_header.trace_id = tid;
	trc_header.trace_no = tno;
	trc_header.format_id = 1;
	trc_format.trc_header = &trc_header;
	trc_format.count = size+1;
	trc_format.buf = &errinfo;

	result = lites_trace_write(gid, &trc_format);
#ifdef LOCAL_DBG
	pr_err("sdhci_drc = %d",result);
#endif
	return result;
}

static int drc_drvtrace(const char *fmt, ...)
{
	int ret;

	va_list args;
	va_start(args, fmt);
	ret = drc_drvtrace_native(fmt, args);
	va_end(args);
	return ret;
}
/* FUJITSU TEN :2013-02-21 DriveRec end */

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
			ADA_LOG_TRACE(sdhci,
				"Budet Interrupt Occurred. remain :%ld",remain);
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
			ADA_LOG_TRACE(sdhci,
				"Budet Interrupt Occurred. remain :%ld",remain);
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
//	.cd_gpio = P1852_SD1_CD,
	.cd_gpio = -1,
/* FUJITSU TEN:2012-06-22 WiFi modify code edit. end */
	.wp_gpio = -1,
	.power_gpio = -1,
/* FUJITSU TEN:2012-06-22 WiFi modify code edit. start */
//	.is_8bit = false,
	.max_clk_limit = 25000000,
/* FUJITSU TEN:2012-06-22 WiFi modify code edit. end */
};

static struct tegra_sdhci_platform_data tegra_sdhci_platform_data4 = {
	.cd_gpio = -1,
	.wp_gpio = -1,
	.power_gpio = -1,
	.is_8bit = true,
	.max_clk_limit = 52000000,
	.uhs_mask = MMC_UHS_MASK_DDR50,
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
		ADA_LOG_WARNING(sdhci, "read failed(FTEN_NONVOL_TI_SLEEP_WL_EN_ON_BEFORE)");
	} else {
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe start */
		if (NONVOL_VALCHK(val)) {
			sleep_wl_en_on_before = val * 10;
		} else {
			ADA_LOG_WARNING(sdhci, "read abnormal(FTEN_NONVOL_TI_SLEEP_WL_EN_ON_BEFORE)");
		}
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe end */
	}
	rc = GetNONVOLATILE(&val, FTEN_NONVOL_TI_SLEEP_WL_EN_OFF_AFTER,
		FTEN_NONVOL_TI_SLEEP_WL_EN_OFF_AFTER_SIZE);
	if( rc != 0 ) {
		ADA_LOG_WARNING(sdhci, "read failed(FTEN_NONVOL_TI_SLEEP_WL_EN_OFF_AFTER)");
	} else {
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe start */
		if (NONVOL_VALCHK(val)) {
			sleep_wl_en_off_after = val * 10;
		} else {
			ADA_LOG_WARNING(sdhci, "read abnormal(FTEN_NONVOL_TI_SLEEP_WL_EN_OFF_AFTER)");
		}
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe end */
	}
/* FUJITSU TEN :2013-02-21 DriveRec start */
	drc_drvtrace("nvw2:%d %d"
		,sleep_wl_en_on_before
		,sleep_wl_en_off_after);
/* FUJITSU TEN :2013-02-21 DriveRec end */
	ADA_LOG_TRACE(sdhci, "Get Nonvolatile(sdhci) = %d %d"
		,sleep_wl_en_on_before
		,sleep_wl_en_off_after);
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


int __init p1852_sdhci_init(void)
{
	tegra_sdhci_device1.dev.platform_data = &tegra_sdhci_platform_data1;
	tegra_sdhci_device4.dev.platform_data = &tegra_sdhci_platform_data4;

	platform_device_register(&tegra_sdhci_device1);
	platform_device_register(&tegra_sdhci_device4);

/* FUJITSU TEN:2012-06-22 WiFi use code insert edit. start */
	f12arc_wifi_init();
/* FUJITSU TEN:2012-06-22 WiFi use code insert edit. end */

	return 0;
}
