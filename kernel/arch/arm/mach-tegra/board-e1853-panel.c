/*
 * arch/arm/mach-tegra/board-e1853-panel.c
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

#include <linux/resource.h>
#include <asm/mach-types.h>
#include <linux/platform_device.h>
#include <linux/nvhost.h>
#include <linux/nvmap.h>
#include <linux/i2c/ds90uh925q_ser.h>
#include <mach/irqs.h>
#include <mach/iomap.h>
#include <mach/dc.h>
#include <mach/fb.h>
#include <mach/board_id.h>
/* FUJITSU TEN:2012-12-21 HDMI SUPPORT BEGIN */
#include <linux/delay.h>
/* FUJITSU TEN:2012-12-21 HDMI SUPPORT END */

#include "board.h"
#include "devices.h"
#include "gpio-names.h"
//add
#include "board-e1853.h"

#define E1853_HDMI_HPD TEGRA_GPIO_PB2

//add
#define E1853_LVDS_ENA1 		TEGRA_GPIO_PV0
#define E1853_LVDS_SER1_ADDR	0xd

/* dc related */
static int e1853_lvds_enable(void)	//mod
{
/* FUJITSU TEN: No need for initialize BEGIN */
#if 0
	struct i2c_adapter *adapter;
	struct i2c_board_info info = {{0}};
	static struct i2c_client *client;
	struct i2c_msg msg[2];
	u8 cmd_buf[] = {0x4, 0};
	int status;

	/* Turn on serializer chip */
	gpio_set_value(E1853_LVDS_ENA1, 1);

	/* Program the serializer */
	adapter = i2c_get_adapter(3);
	if (!adapter)
		printk(KERN_WARNING "%s: adapter is null\n", __func__);
	else {
		info.addr = E1853_LVDS_SER1_ADDR;
		if (!client)
			client = i2c_new_device(adapter, &info);
		i2c_put_adapter(adapter);
		if (!client)
			printk(KERN_WARNING "%s: client is null\n", __func__);
		else {
			msg[0].addr = E1853_LVDS_SER1_ADDR;
			msg[0].flags = 0;
			msg[0].len = 1;
			msg[0].buf = &cmd_buf[0];

			status = i2c_transfer(client->adapter, msg, 1);

			msg[0].addr = E1853_LVDS_SER1_ADDR;
			msg[0].flags = 0;
			msg[0].len = 1;
			msg[0].buf = &cmd_buf[0];

			msg[1].addr = E1853_LVDS_SER1_ADDR;
			msg[1].flags = I2C_M_RD;
			msg[1].len = 1;
			msg[1].buf = &cmd_buf[1];

			status = i2c_transfer(client->adapter, msg, 2);

			cmd_buf[1] |= (1 << 2);
			cmd_buf[1] |= (1 << 3);

			msg[0].addr = E1853_LVDS_SER1_ADDR;
			msg[0].flags = 0;
			msg[0].len = 2;
			msg[0].buf = &cmd_buf[0];

			status = i2c_transfer(client->adapter, msg, 1);
		}
	}
#endif
/* FUJITSU TEN: No need for initialize  END */
	return 0;
}

static int e1853_lvds_disable(void)	//mod
{
/* FUJITSU TEN: No need for initialize BEGIN */
#if 0
	/* Turn off serializer chip */
	gpio_set_value(E1853_LVDS_ENA1, 0);
#endif
/* FUJITSU TEN: No need for initialize END */
	return 0;
}

/*
static struct ds90uh925q_platform_data lvds_ser_platform = {
	.has_lvds_en_gpio = false,
	.is_fpdlinkII  = true,
	.support_hdcp  = false,
	.clk_rise_edge = true,
};

static struct i2c_board_info __initdata lvds_ser_info[] = {
	{
		I2C_BOARD_INFO("ds90uh925q", 0xd),
		.platform_data = &lvds_ser_platform,
	}
};
*/

#if 0
static struct tegra_dc_mode e1853_CLAA101WB03_panel_modes[] = {
	{
		/* 1366x768@60Hz */
		.pclk = 74180000,
		.h_ref_to_sync = 1,
		.v_ref_to_sync = 1,
		.h_sync_width = 30,
		.v_sync_width = 5,
		.h_back_porch = 52,
		.v_back_porch = 20,
		.h_active = 1366,
		.v_active = 768,
		.h_front_porch = 64,
		.v_front_porch = 25,
	},
};

static struct tegra_fb_data e1853_CLAA101WB03_fb_data = {
	.win        = 0,
	.xres       = 1366,
	.yres       = 768,
	.bits_per_pixel = 32,
};
#endif

//#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT
#if 0
static struct tegra_dc_mode e1853_panel_modes[] = {
	{
		/* 1366x768@60Hz */
		.pclk = 74180000,
		.h_ref_to_sync = 1,
		.v_ref_to_sync = 1,
		.h_sync_width = 30,
		.v_sync_width = 5,
		.h_back_porch = 52,
		.v_back_porch = 20,
		.h_active = 1366,
		.v_active = 768,
		.h_front_porch = 64,
		.v_front_porch = 25,
	},
};

static struct tegra_fb_data e1853_fb_data = {
	.win        = 0,
	.xres       = 1366,
	.yres       = 768,
	.bits_per_pixel = 32,
	.flags      = TEGRA_FB_HIDE_DC_WIN,
};

#else

static struct tegra_dc_mode e1853_panel_modes[] = {	//mod
	{
#if defined(CONFIG_FJFEAT_PRODUCT_ADA1S)
		/* 800x480@60 */
		.pclk = 32460000,
		.h_ref_to_sync = 1,
		.v_ref_to_sync = 1,
		.h_sync_width = 64,
		.v_sync_width = 3,
		.h_back_porch = 151,
		.v_back_porch = 36,
		.h_front_porch = 41,
		.v_front_porch = 6,
		.h_active = 800,
		.v_active = 480,
//#elif defined(CONFIG_FJFEAT_PRODUCT_ADA2S) // If you need append new configuration after 2S, add 'elif' section on this line.
#else
		/* 800x480@60 */
/* FUJITSU TEN:2013-07-25 Mod start (Adjust LCD clock) */
		.pclk				= 36125000,
		.h_ref_to_sync	=   1,
		.v_ref_to_sync	=   1,
		.h_sync_width		= 120,
		.v_sync_width		=   2,
		.h_back_porch		=  96,
		.v_back_porch		=  33,
		.h_front_porch	= 132,
		.v_front_porch	=  10,
		.h_active			= 800,
		.v_active			= 480,
/* FUJITSU TEN:2013-07-25 Mod end (Adjust LCD clock) */
#endif
	},
};

static struct tegra_fb_data e1853_fb_data = {
	.win		= 0,
	.xres		= 800,
	.yres		= 480,
	.bits_per_pixel	= 32,
//	.flags      = TEGRA_FB_HIDE_DC_WIN,
};

#endif

static struct tegra_dc_out_pin e1853_dc_out_pins[] = {
	{
		.name	= TEGRA_DC_OUT_PIN_H_SYNC,
		.pol	= TEGRA_DC_OUT_PIN_POL_LOW,
	},
	{
		.name	= TEGRA_DC_OUT_PIN_V_SYNC,
		.pol	= TEGRA_DC_OUT_PIN_POL_LOW,
	},
	{
		.name	= TEGRA_DC_OUT_PIN_PIXEL_CLOCK,
		.pol	= TEGRA_DC_OUT_PIN_POL_LOW,
	},
	{
		.name   = TEGRA_DC_OUT_PIN_DATA_ENABLE,
		.pol    = TEGRA_DC_OUT_PIN_POL_HIGH,
	},
};

static struct tegra_dc_out e1853_ser_out = {
	.align		= TEGRA_DC_ALIGN_MSB,
	.order		= TEGRA_DC_ORDER_RED_BLUE,
	.parent_clk	= "pll_d_out0",
	.type		= TEGRA_DC_OUT_RGB,
	.modes		= e1853_panel_modes,
	.n_modes	= ARRAY_SIZE(e1853_panel_modes),
	.enable		= e1853_lvds_enable,
	.disable	= e1853_lvds_disable,
	.out_pins	= e1853_dc_out_pins,
	.n_out_pins	= ARRAY_SIZE(e1853_dc_out_pins),
};

static struct tegra_dc_platform_data e1853_disp1_pdata = {
	.flags		= TEGRA_DC_FLAG_ENABLED,
	.default_out	= &e1853_ser_out,
	.emc_clk_rate	= 300000000,
	.fb		= &e1853_fb_data,
};

static int e1853_hdmi_enable(void)
{
	return 0;
}

static int e1853_hdmi_disable(void)
{
	return 0;
}

static struct tegra_fb_data e1853_hdmi_fb_data = {
	.win            = 0,
	.xres           = 800,
	.yres           = 480,
	.bits_per_pixel = 32,
	.flags          = TEGRA_FB_FLIP_ON_PROBE,
};

static struct tegra_dc_out e1853_hdmi_out = {
	.align		= TEGRA_DC_ALIGN_MSB,
	.order		= TEGRA_DC_ORDER_RED_BLUE,
	.parent_clk     = "pll_d2_out0",
	.type		= TEGRA_DC_OUT_HDMI,
	.flags          = TEGRA_DC_OUT_HOTPLUG_LOW |
			  TEGRA_DC_OUT_NVHDCP_POLICY_ON_DEMAND,	
	.max_pixclock   = KHZ2PICOS(148500),
	.hotplug_gpio   = E1853_HDMI_HPD, 	
	.enable		= e1853_hdmi_enable,
	.disable	= e1853_hdmi_disable,
	.dcc_bus        = 3,
};

static struct tegra_dc_platform_data e1853_hdmi_pdata = {
	.flags           = 0,
	.default_out     = &e1853_hdmi_out,
	.emc_clk_rate    = 300000000,
	.fb              = &e1853_hdmi_fb_data,
};

static struct nvmap_platform_carveout e1853_carveouts[] = {
	[0] = {
		.name		= "iram",
		.usage_mask	= NVMAP_HEAP_CARVEOUT_IRAM,
		.base		= TEGRA_IRAM_BASE + TEGRA_RESET_HANDLER_SIZE,
		.size		= TEGRA_IRAM_SIZE - TEGRA_RESET_HANDLER_SIZE,
		.buddy_size	= 0, /* no buddy allocation for IRAM */
	},
	[1] = {
		.name		= "generic-0",
		.usage_mask	= NVMAP_HEAP_CARVEOUT_GENERIC,
		.base		= 0,	/* Filled in by e1853_panel_init() */
		.size		= 0,	/* Filled in by e1853_panel_init() */
		.buddy_size	= SZ_32K,
	},
};

static struct nvmap_platform_data e1853_nvmap_data = {
	.carveouts	= e1853_carveouts,
	.nr_carveouts	= ARRAY_SIZE(e1853_carveouts),
};

static struct platform_device *e1853_gfx_devices[] __initdata = {
	&tegra_nvmap_device,
};

#if 0
static void e1853_config_CLAA101WB03_lcd(void)
{
	e1853_disp1_pdata.default_out->modes =
		e1853_CLAA101WB03_panel_modes;
	e1853_disp1_pdata.default_out->n_modes =
		ARRAY_SIZE(e1853_CLAA101WB03_panel_modes);
	e1853_disp1_pdata.default_out->depth = 18;
	e1853_disp1_pdata.default_out->dither = TEGRA_DC_ORDERED_DITHER;
	e1853_disp1_pdata.fb = &e1853_CLAA101WB03_fb_data;
}
#endif

static int __init e1853_ada_panel_init(void)	//add
{
	int err;
	struct resource *res;

	/* ADA: Single display only.(NO HDMI) */
	e1853_carveouts[1].base = tegra_carveout_start;
	e1853_carveouts[1].size = tegra_carveout_size;
	tegra_nvmap_device.dev.platform_data = &e1853_nvmap_data;
	/* sku 8 has primary RGB out and secondary HDMI out */
	tegra_disp1_device.dev.platform_data = &e1853_disp1_pdata;
	//FTEN: No needed for HDMI Interface
	//tegra_disp2_device.dev.platform_data = &e1853_hdmi_pdata;

#ifdef CONFIG_TEGRA_GRHOST
	err = nvhost_device_register(&tegra_grhost_device);
	if (err)
		return err;
#endif

	err = platform_add_devices(e1853_gfx_devices,
				ARRAY_SIZE(e1853_gfx_devices));

	res = nvhost_get_resource_byname(&tegra_disp1_device,
					 IORESOURCE_MEM, "fbmem");
	if (res) {
		res->start = tegra_fb_start;
		res->end = tegra_fb_start + tegra_fb_size - 1;
	}

	if (!err)
		err = nvhost_device_register(&tegra_disp1_device);

	//FTEN: No needed for HDMI Interface
	/*
	res = nvhost_get_resource_byname(&tegra_disp2_device,
					 IORESOURCE_MEM, "fbmem");
	if (res) {
		res->start = tegra_fb2_start;
		res->end = tegra_fb2_start + tegra_fb2_size - 1;
	}

	if (!err)
		err = nvhost_device_register(&tegra_disp2_device);
	 */

        /* FUJITSU TEN:2012-12-21 HDMI SUPPORT BEGIN */
	gpio_set_value(TEGRA_GPIO_PCC3, 1);
	mdelay(5);
	gpio_set_value(TEGRA_GPIO_PA0, 1);
	//gpio_set_value(TEGRA_GPIO_PR5, 1);
        /* FUJITSU TEN:2012-12-21 HDMI SUPPORT END */

#if defined(CONFIG_TEGRA_GRHOST) && defined(CONFIG_TEGRA_NVAVP)
	if (!err)
		err = nvhost_device_register(&nvavp_device);
#endif
	return err;
}

int __init e1853_panel_init(void)
{
#if 0
	bool has_ebb = false;
	struct nvhost_device *pdisp1_dev;
	int err;
	struct resource *res;

	if (tegra_is_board(NULL, "61861", NULL, NULL, NULL)) {
		has_ebb = true;
		if (tegra_is_board(NULL, "61227", NULL, NULL, NULL)) {
			e1853_config_CLAA101WB03_lcd();
			e1853_touch_init();
		}
	}

	e1853_carveouts[1].base = tegra_carveout_start;
	e1853_carveouts[1].size = tegra_carveout_size;
	tegra_nvmap_device.dev.platform_data = &e1853_nvmap_data;

	if (has_ebb) {
		pdisp1_dev = &tegra_disp1_device;
		pdisp1_dev->dev.platform_data = &e1853_disp1_pdata;
		tegra_disp2_device.dev.platform_data = &e1853_hdmi_pdata;
	} else {
		pdisp1_dev = &tegra_disp1_hdmi_device;
		pdisp1_dev->dev.platform_data = &e1853_hdmi_pdata;
	}

#ifdef CONFIG_TEGRA_GRHOST
	err = nvhost_device_register(&tegra_grhost_device);
	if (err)
		return err;
#endif

	err = platform_add_devices(e1853_gfx_devices,
				ARRAY_SIZE(e1853_gfx_devices));

#if defined(CONFIG_TEGRA_GRHOST) && defined(CONFIG_TEGRA_DC)
	res = nvhost_get_resource_byname(pdisp1_dev,
					 IORESOURCE_MEM, "fbmem");
	if (res) {
		res->start = tegra_fb_start;
		res->end = tegra_fb_start + tegra_fb_size - 1;
	}

	if (!err)
		err = nvhost_device_register(pdisp1_dev);

	if (has_ebb) {
		res = nvhost_get_resource_byname(&tegra_disp2_device,
						 IORESOURCE_MEM, "fbmem");
		if (res) {
			res->start = tegra_fb2_start;
			res->end = tegra_fb2_start + tegra_fb2_size - 1;
		}

		if (!err)
			err = nvhost_device_register(&tegra_disp2_device);
	}
#endif

#if defined(CONFIG_TEGRA_GRHOST) && defined(CONFIG_TEGRA_NVAVP)
	if (!err)
		err = nvhost_device_register(&nvavp_device);
#endif

	if (has_ebb) {
		if (!err)
			i2c_register_board_info(1, &lvds_ser_info, 1);
	}

	return err;
#else
	return e1853_ada_panel_init();
#endif
}
