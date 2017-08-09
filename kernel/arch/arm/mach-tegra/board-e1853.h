/*
 * arch/arm/mach-tegra/e1853/board-e1853.h
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
 */

/*
 *----------------------------------------------------------------------------
 * COPYRIGHT(C) FUJITSU TEN LIMITED 2011-2013
 *----------------------------------------------------------------------------
 */

#ifndef _MACH_TEGRA_BOARD_E1853_H
#define _MACH_TEGRA_BOARD_E1853_H

#include <mach/gpio.h>
#include <linux/mfd/tps6591x.h>
#include <mach/irqs.h>
#include <mach/gpio.h>

int e1853_sdhci_init(void);
int e1853_pinmux_init(void);
/* FUJITSU TEN:2012-09-18 delete touch function of the vibrante. start */
//int e1853_touch_init(void);
/* FUJITSU TEN:2012-09-18 delete touch function of the vibrante. end */
int e1853_panel_init(void);
int e1853_gpio_init(void);
int e1853_pins_state_init(void);
int e1853_suspend_init(void);
int e1853_regulator_init(void);
/* FUJITSU TEN:2013-01-31 #13582 start */
//int e1853_wifi_init(void);
/* FUJITSU TEN:2013-01-31 #13582 end */
int e1853_pca953x_init(void);
int e1853_keys_init(void);
/* External peripheral act as gpio */
/* TPS6591x GPIOs */
#define TPS6591X_GPIO_BASE	TEGRA_NR_GPIOS
#define TPS6591X_GPIO_0		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP0)
#define TPS6591X_GPIO_1		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP1)
#define TPS6591X_GPIO_2		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP2)
#define TPS6591X_GPIO_3		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP3)
#define TPS6591X_GPIO_4		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP4)
#define TPS6591X_GPIO_5		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP5)
#define TPS6591X_GPIO_6		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP6)
#define TPS6591X_GPIO_7		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP7)
#define TPS6591X_GPIO_8		(TPS6591X_GPIO_BASE + TPS6591X_GPIO_GP8)
#define TPS6591X_GPIO_END	(TPS6591X_GPIO_BASE + TPS6591X_GPIO_NR)

#define TPS6591X_IRQ_BASE	TEGRA_NR_IRQS
#define TPS6591X_IRQ_END	(TPS6591X_IRQ_BASE + 18)

/* TPS6591x interrupt line */
#define TPS6591X_IRQ	TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PR4)

/* PCA953X - MISC SYSTEM IO */
#define PCA953X_MISCIO_GPIO_BASE	TPS6591X_GPIO_END
#define MISCIO_BT_RST_GPIO			(PCA953X_MISCIO_GPIO_BASE + 0)
#define MISCIO_GPS_RST_GPIO			(PCA953X_MISCIO_GPIO_BASE + 1)
#define MISCIO_GPS_EN_GPIO			(PCA953X_MISCIO_GPIO_BASE + 2)
#define MISCIO_WF_EN_GPIO			(PCA953X_MISCIO_GPIO_BASE + 3)
#define MISCIO_WF_RST_GPIO			(PCA953X_MISCIO_GPIO_BASE + 4)
#define MISCIO_BT_EN_GPIO			(PCA953X_MISCIO_GPIO_BASE + 5)
/* GPIO6 is not used */
#define MISCIO_NOT_USED0			(PCA953X_MISCIO_GPIO_BASE + 6)
#define MISCIO_BT_WAKEUP_GPIO		(PCA953X_MISCIO_GPIO_BASE + 7)
#define MISCIO_FAN_SEL_GPIO			(PCA953X_MISCIO_GPIO_BASE + 8)
#define MISCIO_EN_MISC_BUF_GPIO		(PCA953X_MISCIO_GPIO_BASE + 9)
#define MISCIO_EN_MSATA_GPIO		(PCA953X_MISCIO_GPIO_BASE + 10)
#define MISCIO_EN_SDCARD_GPIO		(PCA953X_MISCIO_GPIO_BASE + 11)
/* GPIO12 is not used */
#define MISCIO_NOT_USED1			(PCA953X_MISCIO_GPIO_BASE + 12)
#define MISCIO_ABB_RST_GPIO			(PCA953X_MISCIO_GPIO_BASE + 13)
#define MISCIO_USER_LED2_GPIO		(PCA953X_MISCIO_GPIO_BASE + 14)
#define MISCIO_USER_LED1_GPIO		(PCA953X_MISCIO_GPIO_BASE + 15)
#define PCA953X_MISCIO_GPIO_END		(PCA953X_MISCIO_GPIO_BASE + 16)

#if 0 //FTEN
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT
#define TOUCH_GPIO_IRQ_ATMEL_T9 TEGRA_GPIO_PEE1
#define TOUCH_GPIO_RST_ATMEL_T9 TEGRA_GPIO_PR2
#define TOUCH_BUS_ATMEL_T9  0
#endif
#endif //FTEN

/* PCA953X I2C IO expander bus addresses */
#define PCA953X_MISCIO_ADDR		0x75

/* Wake button interrupt line (PWR_INT) */
#define WAKE_BUTTON_IRQ	INT_EXTERNAL_PMU

/* Thermal monitor data */
#define DELTA_TEMP 4000
#define DELTA_TIME 2000
#define REMT_OFFSET 8000
#define I2C_ADDR_TMP411 0x4c
#define I2C_BUS_TMP411 4

/* FUJITSU TEN:2012-06-22 WiFi use GPIO-Address. PCC5 PV2 PV3 edit. start */
/* TI WiFi/GPS/BT module */
#define GPIO_BT_EN                             TEGRA_GPIO_PCC5
#define GPIO_WLAN_EN                   TEGRA_GPIO_PV2
#define GPIO_WLAN_IRQ                  TEGRA_GPIO_PV3
/* FUJITSU TEN:2012-06-22 WiFi use GPIO-Address. PCC5 PV2 PV3 edit. end */

/* FUJITSU TEN:2012-06-28 touch use GPIO / I2C2 bus add. start */
/* touch panel use GPIO and I2c bus */
#define TOUCH_GPIO_IRQ_ATMEL_T9		TEGRA_GPIO_PD3
#define TOUCH_GPIO_RST_ATMEL_T9		TEGRA_GPIO_PX7
#define TOUCH_BUS_ATMEL_T9		1
/* FUJITSU TEN:2012-06-28 touch use GPIO / I2C2 bus add. end */

/* FUJITSU TEN:2012-07-11 over current protection init add. start */
#define USBOCP_GPIO_USB2_PGOOD	TEGRA_GPIO_PR1
#define USBOCP_GPIO_USB3_PGOOD	TEGRA_GPIO_PEE3
#define USBOCP_GPIO_USB2_EN		TEGRA_GPIO_PDD5
#define USBOCP_GPIO_USB3_EN		TEGRA_GPIO_PDD0
/* FUJITSU TEN:2012-07-11 over current protection init add. end */

/* FUJITSU TEN:2012-07-04 dsp use GPIO add. start */
/* AK7736 dsp GPIO and I2C BUS */
#define TEGRA_GPIO_AK_DSP_PDN TEGRA_GPIO_PC1
/* FUJITSU TEN:2013-05-21 modify start */
#ifdef CONFIG_FJFEAT_PRODUCT_ADA3S
  #define TEGRA_GPIO_AK_DSP_ERR TEGRA_GPIO_PW4
#else
  #ifdef CONFIG_FJFEAT_PRODUCT_ADA1M
    #define TEGRA_GPIO_AK_DSP_ERR TEGRA_GPIO_PW4
  #else
    #define TEGRA_GPIO_AK_DSP_ERR TEGRA_GPIO_PZ4
  #endif
#endif
/* FUJITSU TEN:2013-05-21 modify end */
#define TEGRA_GPIO_AK_ADC_PDN TEGRA_GPIO_PK2
#define TEGRA_EXT_MIC_OPEN    TEGRA_GPIO_PQ2
#define TEGRA_EXT_MIC_PWRON   TEGRA_GPIO_PQ3
#define TEGRA_EXT_MIC_SHORT   TEGRA_GPIO_PQ4
#define TEGRA_EXT_MIC_MODE0   TEGRA_GPIO_PQ5
#define TEGRA_EXT_MIC_MODE1   TEGRA_GPIO_PQ6
#define TEGRA_EXT_MIC_DET_OFF TEGRA_GPIO_PD7
#define TEGRA_AK_DSP_I2C_BUS 0
/* FUJITSU TEN:2012-07-04 dsp use GPIO add. end */
// Micware Co.,Ltd. for SPI driver add start
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_H_0 		(IO_ADDRESS(TEGRA_CLK_RESET_BASE) + 0x14)
#define CLK_RST_CONTROLLER_CLK_OUT_ENB_V_0 		(IO_ADDRESS(TEGRA_CLK_RESET_BASE) + 0x360)
#define CLK_RST_CONTROLLER_CLK_SOURCE_SBC1 		(IO_ADDRESS(TEGRA_CLK_RESET_BASE) + 0x134)
#define CLK_RST_CONTROLLER_CLK_SOURCE_SBC2 		(IO_ADDRESS(TEGRA_CLK_RESET_BASE) + 0x118)
#define CLK_RST_CONTROLLER_CLK_SOURCE_SBC5 		(IO_ADDRESS(TEGRA_CLK_RESET_BASE) + 0x3C8)
#define CLK_ENB_SBC5							(1<<8)
#define CLK_ENB_SBC2							(1<<12)
#define CLK_ENB_SBC1							(1<< 9)
#define CLK_M_MASK								(3<<30)
#define CLK_M_PLLP_OUT0							(0<<30)
#define CLK_M_PLLC_OUT0							(1<<30)
#define CLK_M_PLLM_OUT0							(2<<30)
#define CLK_M_CLK_M								(3<<30)
// Micware Co.,Ltd. for SPI driver add end

/* FUJITSU TEN:2012-11-07 Suspend Resume start */
#define TG_BU_DET_INT       TEGRA_GPIO_PR2
#define TG_WAKEUP_REQ       TEGRA_GPIO_PS0
/* Design_document_PlanG_0829_FTEN Start */
#define DEV_SW_BUDET        TEGRA_GPIO_PR4
#define TG_WAKEUP2          TEGRA_GPIO_PJ2
#define TG_WAKEUP1          TEGRA_GPIO_PI2
/* Design_document_PlanG_0829_FTEN End */
/* FUJITSU TEN:2012-12-12 TG_WAKEUP3 supprt end */
#define TG_WAKEUP3          TEGRA_GPIO_PI5
/* FUJITSU TEN:2012-12-12 TG_WAKEUP3 supprt end */
/* FUJITSU TEN:2012-11-07 Suspend Resume end */

/* FUJITSU TEN:2012-10-30 Power Sequence TG_WAKEUP1/TG_VERUP set Low start */
#define TG_VERUP            TEGRA_GPIO_PX5
/* FUJITSU TEN:2012-10-30 Power Sequence TG_WAKEUP1/TG_VERUP set Low end   */

/* FUJITSU TEN:2012-09-21 GPS Antenna use GPIO add. start */
/* GPS Antenna use GPIO */
#define GPS_ANT_GPIO_GPS_ANT_SHORT	TEGRA_GPIO_PQ0
#define GPS_ANT_GPIO_GPS_ANT_OPEN	TEGRA_GPIO_PQ1
/* FUJITSU TEN:2012-09-21 GPS Antenna use GPIO add. end */
/* FUJITSU TEN:2012-11-14 TMP401 value add. start */
#define TMP401_I2C_BUS			3
#define TMP401_I2C_ADDR			0x4c
/* FUJITSU TEN:2012-11-14 TMP401 value add. end */
/* FUJITSU TEN:2012-12-05 TPS6591X value add. start */
#define TPS6591X_I2C_BUS		0
#define TPS6591X_I2C_ADDR		0x2D
/* FUJITSU TEN:2012-12-05 TPS6591X value add. end */

/* FUJITSU TEN:2013-1-17 Refresh use GPIO add. start */
#define WAKEUP_STATE1          TEGRA_GPIO_PDD1
#define WAKEUP_STATE2          TEGRA_GPIO_PCC6
/* FUJITSU TEN:2013-1-17 Refresh use GPIO add. end */

/* FUJITSU TEN:2012-09-18 delete touch function of the vibrante. start */
#if 0
/* FUJITSU TEN:2012-09-18 delete touch function of the vibrante. end */
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT
#define TOUCH_GPIO_IRQ_ATMEL_T9 TEGRA_GPIO_PEE1
#define TOUCH_GPIO_RST_ATMEL_T9 TEGRA_GPIO_PW2
#define TOUCH_BUS_ATMEL_T9  0
#endif
/* FUJITSU TEN:2012-09-18 delete touch function of the vibrante. start */
#endif
/* FUJITSU TEN:2012-09-18 delete touch function of the vibrante. end */
/* FUJITSU TEN:2012-06-22 WiFi use GPIO-Address. PCC5 PV2 PV3 edit. start */
/* TI WiFi/GPS/BT module */
#define GPIO_BT_EN				TEGRA_GPIO_PCC5
#define GPIO_WLAN_EN			TEGRA_GPIO_PV2
#define GPIO_WLAN_IRQ			TEGRA_GPIO_PV3
/* FUJITSU TEN:2012-06-22 WiFi use GPIO-Address. PCC5 PV2 PV3 edit. end */

// ADA LOG 2013-0418 ADD
#if 1
#define TG_E_STBY_REQ           TEGRA_GPIO_PR3
#endif
// ADA LOG 2013-0418 ADD

#endif
