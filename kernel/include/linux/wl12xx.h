/*
 * This file is part of wl12xx
 *
 * Copyright (C) 2009 Nokia Corporation
 *
 * Contact: Luciano Coelho <luciano.coelho@nokia.com>
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
/*----------------------------------------------------------------------------*/
// COPYRIGHT(C) FUJITSU TEN LIMITED 2012
/*----------------------------------------------------------------------------*/

#ifndef _LINUX_WL12XX_H
#define _LINUX_WL12XX_H

/* Reference clock values */
enum {
	WL12XX_REFCLOCK_19	= 0, /* 19.2 MHz */
	WL12XX_REFCLOCK_26	= 1, /* 26 MHz */
	WL12XX_REFCLOCK_38	= 2, /* 38.4 MHz */
	WL12XX_REFCLOCK_52	= 3, /* 52 MHz */
	WL12XX_REFCLOCK_38_XTAL = 4, /* 38.4 MHz, XTAL */
	WL12XX_REFCLOCK_26_XTAL = 5, /* 26 MHz, XTAL */
};


/* TCXO clock values */
enum {
	WL12XX_TCXOCLOCK_19_2	= 0, /* 19.2MHz */
	WL12XX_TCXOCLOCK_26	= 1, /* 26 MHz */
	WL12XX_TCXOCLOCK_38_4	= 2, /* 38.4MHz */
	WL12XX_TCXOCLOCK_52	= 3, /* 52 MHz */
	WL12XX_TCXOCLOCK_16_368	= 4, /* 16.368 MHz */
	WL12XX_TCXOCLOCK_32_736	= 5, /* 32.736 MHz */
	WL12XX_TCXOCLOCK_16_8	= 6, /* 16.8 MHz */
	WL12XX_TCXOCLOCK_33_6	= 7, /* 33.6 MHz */
};

struct wl12xx_platform_data {
// FUJITSU TEN:2012-06-22 add WiFi. start
#if 0
	int (*set_power)(int power_on);
	int (*set_carddetect)(int val);
#else
	// for spi only
	void (*set_power)(bool enable);
#endif
// FUJITSU TEN:2012-06-22 add WiFi. end

	/* SDIO only: IRQ number if WLAN_IRQ line is used, 0 for SDIO IRQs */
	int irq;
	bool use_eeprom;
	int board_ref_clock;
	int board_tcxo_clock;
	unsigned long platform_quirks;
};

/* Platform does not support level trigger interrupts */
#define WL12XX_PLATFORM_QUIRK_EDGE_IRQ	BIT(0)

#ifdef CONFIG_WL12XX_PLATFORM_DATA

int wl12xx_set_platform_data(const struct wl12xx_platform_data *data);

#else

static inline
int wl12xx_set_platform_data(const struct wl12xx_platform_data *data)
{
	return -ENOSYS;
}

#endif

const struct wl12xx_platform_data *wl12xx_get_platform_data(void);

/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
bool wl12xx_is_device_ready(void);
int  wl12xx_up_device_ready(void);
/** This func should be call in interrupt context */
void wl12xx_down_device_ready(void);

// LOG Definitions
#define ADA_LOG_WL_ERR(...)		ADA_LOG_ERR(wl12xx,__VA_ARGS__)
#define ADA_LOG_WL_WARNING(...)	ADA_LOG_WARNING(wl12xx,__VA_ARGS__)
#define ADA_LOG_WL_NOTICE(...)	ADA_LOG_NOTICE(wl12xx,__VA_ARGS__)
#define ADA_LOG_WL_INFO(...)	ADA_LOG_INFO(wl12xx,__VA_ARGS__)
#define ADA_LOG_WL_DEBUG(...)	ADA_LOG_DEBUG(wl12xx,__VA_ARGS__)
#define ADA_LOG_WL_TRACE(...)	ADA_LOG_TRACE(wl12xx,__VA_ARGS__)

#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
#endif