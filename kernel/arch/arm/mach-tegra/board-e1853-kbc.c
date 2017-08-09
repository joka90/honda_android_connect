/*
 * arch/arm/mach-tegra/board-e1853-kbc.c
 * Wake key configuration for Nvidia tegra3 e1853 platform.
 *
 * Copyright (c) 2013, NVIDIA CORPORATION.  All rights reserved.
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
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>

#include <mach/irqs.h>
#include <mach/io.h>
#include <mach/iomap.h>
#include <mach/kbc.h>
#include "board.h"
#include "board-e1853.h"

#include "gpio-names.h"
#include "devices.h"

#define GPIO_IKEY(_id, _irq, _iswake, _deb)	\
	{					\
		.code = _id,			\
		.gpio = -1,			\
		.irq = _irq,			\
		.desc = #_id,			\
		.type = EV_KEY,			\
		.wakeup = _iswake,		\
		.debounce_interval = _deb,	\
	}

static struct gpio_keys_button e1853_wake_key[] = {
	[0] = GPIO_IKEY(KEY_POWER, WAKE_BUTTON_IRQ, 1, 100),
};

static struct gpio_keys_platform_data e1853_wake_key_pdata = {
	.buttons	= e1853_wake_key,
	.nbuttons	= ARRAY_SIZE(e1853_wake_key),
};

static struct platform_device e1853_wake_key_device = {
	.name	= "gpio-keys",
	.id	= 0,
	.dev	= {
		.platform_data = &e1853_wake_key_pdata,
	},
};

int __init e1853_keys_init(void)
{
	pr_info("Registering gpio keys\n");
	platform_device_register(&e1853_wake_key_device);
	return 0;
}
