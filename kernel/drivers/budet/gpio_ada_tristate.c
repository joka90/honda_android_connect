/*
 * Copyright(C) 2012 FUJITSU TEN LIMITED
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*----------------------------------------------------------------------------*/
// COPYRIGHT(C) FUJITSU TEN LIMITED 2012,2013
/*----------------------------------------------------------------------------*/

#include <mach/gpio.h>
#include <mach/pinmux.h>
#include "../../../arch/arm/mach-tegra/gpio-names.h"

/* for DEBUG */
#if 0
#define DEBUG_PRINT
#endif

int ada_tristate_early_device_pin[] = {
	TEGRA_GPIO_PN2,				/* TEGRA_PINGROUP_DAP1_DOUT		*/
	TEGRA_GPIO_PN0,				/* TEGRA_PINGROUP_DAP1_FS		*/
	TEGRA_GPIO_PN3,				/* TEGRA_PINGROUP_DAP1_SCLK		*/
	TEGRA_GPIO_PX6,				/* TEGRA_PINGROUP_SPI1_CS0_N	*/
#if 0
	TEGRA_GPIO_PX5,				/* TEGRA_PINGROUP_SPI1_SCK		*/ /* TG_VERUP */ 
#endif
	TEGRA_GPIO_PP2,				/* TEGRA_PINGROUP_DAP3_DOUT		*/
	TEGRA_GPIO_PP0,				/* TEGRA_PINGROUP_DAP3_FS		*/
	TEGRA_GPIO_PP3,				/* TEGRA_PINGROUP_DAP3_SCLK		*/
	TEGRA_GPIO_PV0,				/* TEGRA_PINGROUP_GPIO_PV0		*/
	TEGRA_GPIO_PV1,				/* TEGRA_PINGROUP_GPIO_PV1		*/
	TEGRA_GPIO_PY0,				/* TEGRA_PINGROUP_ULPI_CLK		*/
#if 0
	TEGRA_GPIO_PO1,				/* TEGRA_PINGROUP_ULPI_DATA0	*/
	TEGRA_GPIO_PO4,				/* TEGRA_PINGROUP_ULPI_DATA3	*/
#endif
	TEGRA_GPIO_PO5,				/* TEGRA_PINGROUP_ULPI_DATA4	*/
	TEGRA_GPIO_PO7,				/* TEGRA_PINGROUP_ULPI_DATA6	*/
	TEGRA_GPIO_PO0,				/* TEGRA_PINGROUP_ULPI_DATA7	*/
	TEGRA_GPIO_PY2,				/* TEGRA_PINGROUP_ULPI_NXT		*/
	TEGRA_GPIO_PY3,				/* TEGRA_PINGROUP_ULPI_STP		*/
	TEGRA_GPIO_PJ7,				/* TEGRA_PINGROUP_GMI_A16		*/
	TEGRA_GPIO_PK7,				/* TEGRA_PINGROUP_GMI_A19		*/
#if 0
	TEGRA_GPIO_PJ2,				/* TEGRA_PINGROUP_GMI_CS1_N		*/ /* TG_WAKEUP2 */
#endif
	TEGRA_GPIO_PK3,				/* TEGRA_PINGROUP_GMI_CS2_N		*/
	TEGRA_GPIO_PK4,				/* TEGRA_PINGROUP_GMI_CS3_N		*/
	TEGRA_GPIO_PK2,				/* TEGRA_PINGROUP_GMI_CS4_N		*/
	TEGRA_GPIO_PI3,				/* TEGRA_PINGROUP_GMI_CS6_N		*/
	TEGRA_GPIO_PI6,				/* TEGRA_PINGROUP_GMI_CS7_N		*/
#if 0
	TEGRA_GPIO_PI2,				/* TEGRA_PINGROUP_GMI_DQS		*/ /* TG_WAKEUP1 */
	TEGRA_GPIO_PI5,				/* TEGRA_PINGROUP_GMI_IORDY		*/ /* TG_WAKEUP3 */
#endif
	TEGRA_GPIO_PV6,				/* TEGRA_PINGROUP_CRT_HSYNC		*/
	TEGRA_GPIO_PV4,				/* TEGRA_PINGROUP_DDC_SCL		*/
	TEGRA_GPIO_PV5,				/* TEGRA_PINGROUP_DDC_SDA		*/
	TEGRA_GPIO_PN4,				/* TEGRA_PINGROUP_LCD_CS0_N		*/
	TEGRA_GPIO_PW0,				/* TEGRA_PINGROUP_LCD_CS1_N		*/
	TEGRA_GPIO_PE0,				/* TEGRA_PINGROUP_LCD_D0		*/
	TEGRA_GPIO_PE1,				/* TEGRA_PINGROUP_LCD_D1		*/
	TEGRA_GPIO_PF2,				/* TEGRA_PINGROUP_LCD_D10		*/
	TEGRA_GPIO_PF3,				/* TEGRA_PINGROUP_LCD_D11		*/
	TEGRA_GPIO_PF4,				/* TEGRA_PINGROUP_LCD_D12		*/
	TEGRA_GPIO_PF5,				/* TEGRA_PINGROUP_LCD_D13		*/
	TEGRA_GPIO_PF6,				/* TEGRA_PINGROUP_LCD_D14		*/
	TEGRA_GPIO_PF7,				/* TEGRA_PINGROUP_LCD_D15		*/
	TEGRA_GPIO_PM0,				/* TEGRA_PINGROUP_LCD_D16		*/
	TEGRA_GPIO_PM1,				/* TEGRA_PINGROUP_LCD_D17		*/
	TEGRA_GPIO_PM2,				/* TEGRA_PINGROUP_LCD_D18		*/
	TEGRA_GPIO_PM3,				/* TEGRA_PINGROUP_LCD_D19		*/
	TEGRA_GPIO_PE2,				/* TEGRA_PINGROUP_LCD_D2		*/
	TEGRA_GPIO_PM4,				/* TEGRA_PINGROUP_LCD_D20		*/
	TEGRA_GPIO_PM5,				/* TEGRA_PINGROUP_LCD_D21		*/
	TEGRA_GPIO_PM6,				/* TEGRA_PINGROUP_LCD_D22		*/
	TEGRA_GPIO_PM7,				/* TEGRA_PINGROUP_LCD_D23		*/
	TEGRA_GPIO_PE3,				/* TEGRA_PINGROUP_LCD_D3		*/
	TEGRA_GPIO_PE4,				/* TEGRA_PINGROUP_LCD_D4		*/
	TEGRA_GPIO_PE5,				/* TEGRA_PINGROUP_LCD_D5		*/
	TEGRA_GPIO_PE6,				/* TEGRA_PINGROUP_LCD_D6		*/
	TEGRA_GPIO_PE7,				/* TEGRA_PINGROUP_LCD_D7		*/
	TEGRA_GPIO_PF0,				/* TEGRA_PINGROUP_LCD_D8		*/
	TEGRA_GPIO_PF1,				/* TEGRA_PINGROUP_LCD_D9		*/
	TEGRA_GPIO_PN6,				/* TEGRA_PINGROUP_LCD_DC0		*/
	TEGRA_GPIO_PD2,				/* TEGRA_PINGROUP_LCD_DC1		*/
	TEGRA_GPIO_PJ1,				/* TEGRA_PINGROUP_LCD_DE		*/
	TEGRA_GPIO_PJ3,				/* TEGRA_PINGROUP_LCD_HSYNC		*/
	TEGRA_GPIO_PW1,				/* TEGRA_PINGROUP_LCD_M1		*/
	TEGRA_GPIO_PB3,				/* TEGRA_PINGROUP_LCD_PCLK		*/
	TEGRA_GPIO_PB2,				/* TEGRA_PINGROUP_LCD_PWR0		*/
	TEGRA_GPIO_PC1,				/* TEGRA_PINGROUP_LCD_PWR1		*/
	TEGRA_GPIO_PJ4,				/* TEGRA_PINGROUP_LCD_VSYNC		*/
	TEGRA_GPIO_PZ3,				/* TEGRA_PINGROUP_LCD_WR_N		*/
	TEGRA_GPIO_PDD2,			/* TEGRA_PINGROUP_PEX_L0_CLKREQ_N	*/
	TEGRA_GPIO_PDD0,			/* TEGRA_PINGROUP_PEX_L0_PRSNT_N	*/
	TEGRA_GPIO_PDD6,			/* TEGRA_PINGROUP_PEX_L1_CLKREQ_N	*/
	TEGRA_GPIO_PDD4,			/* TEGRA_PINGROUP_PEX_L1_PRSNT_N	*/
	TEGRA_GPIO_PDD5,			/* TEGRA_PINGROUP_PEX_L1_RST_N		*/
	TEGRA_GPIO_PCC7,			/* TEGRA_PINGROUP_PEX_L2_CLKREQ_N	*/
	TEGRA_GPIO_PW5,				/* TEGRA_PINGROUP_CLK2_OUT			*/
	TEGRA_GPIO_PCC5,			/* TEGRA_PINGROUP_CLK2_REQ			*/
	TEGRA_GPIO_PV2,				/* TEGRA_PINGROUP_GPIO_PV2			*/
	TEGRA_GPIO_PZ0,				/* TEGRA_PINGROUP_SDMMC1_CLK		*/
	TEGRA_GPIO_PZ1,				/* TEGRA_PINGROUP_SDMMC1_CMD		*/
	TEGRA_GPIO_PY7,				/* TEGRA_PINGROUP_SDMMC1_DAT0		*/
	TEGRA_GPIO_PY6,				/* TEGRA_PINGROUP_SDMMC1_DAT1		*/
	TEGRA_GPIO_PY5,				/* TEGRA_PINGROUP_SDMMC1_DAT2		*/
	TEGRA_GPIO_PY4,				/* TEGRA_PINGROUP_SDMMC1_DAT3		*/
	TEGRA_GPIO_PA6,				/* TEGRA_PINGROUP_SDMMC3_CLK		*/
	TEGRA_GPIO_PA7,				/* TEGRA_PINGROUP_SDMMC3_CMD		*/
	TEGRA_GPIO_PB7,				/* TEGRA_PINGROUP_SDMMC3_DAT0		*/
	TEGRA_GPIO_PB6,				/* TEGRA_PINGROUP_SDMMC3_DAT1		*/
	TEGRA_GPIO_PB5,				/* TEGRA_PINGROUP_SDMMC3_DAT2		*/
	TEGRA_GPIO_PB4,				/* TEGRA_PINGROUP_SDMMC3_DAT3		*/
	TEGRA_GPIO_PD1,				/* TEGRA_PINGROUP_SDMMC3_DAT4		*/
	TEGRA_GPIO_PD0,				/* TEGRA_PINGROUP_SDMMC3_DAT5		*/
	TEGRA_GPIO_PD4,				/* TEGRA_PINGROUP_SDMMC3_DAT7		*/
	TEGRA_GPIO_PCC4,			/* TEGRA_PINGROUP_SDMMC4_CLK		*/
	TEGRA_GPIO_PAA0,			/* TEGRA_PINGROUP_SDMMC4_DAT0		*/
	TEGRA_GPIO_PAA3,			/* TEGRA_PINGROUP_SDMMC4_DAT3		*/
	TEGRA_GPIO_PAA6,			/* TEGRA_PINGROUP_SDMMC4_DAT6		*/
	TEGRA_GPIO_PQ3,				/* TEGRA_PINGROUP_KB_COL3			*/
	TEGRA_GPIO_PQ5,				/* TEGRA_PINGROUP_KB_COL5			*/
	TEGRA_GPIO_PQ6,				/* TEGRA_PINGROUP_KB_COL6			*/
	TEGRA_GPIO_PQ7,				/* TEGRA_PINGROUP_KB_COL7			*/
	TEGRA_GPIO_PS2,				/* TEGRA_PINGROUP_KB_ROW10			*/
	TEGRA_GPIO_PS3,				/* TEGRA_PINGROUP_KB_ROW11			*/
	TEGRA_GPIO_PS4,				/* TEGRA_PINGROUP_KB_ROW12			*/
	TEGRA_GPIO_PS5,				/* TEGRA_PINGROUP_KB_ROW13			*/
	TEGRA_GPIO_PS6,				/* TEGRA_PINGROUP_KB_ROW14			*/
	TEGRA_GPIO_PS7,				/* TEGRA_PINGROUP_KB_ROW15			*/
	TEGRA_GPIO_PR6,				/* TEGRA_PINGROUP_KB_ROW6			*/
	TEGRA_GPIO_PR7,				/* TEGRA_PINGROUP_KB_ROW7			*/
	TEGRA_GPIO_PS1,				/* TEGRA_PINGROUP_KB_ROW9			*/
								/* TEGRA_PINGROUP_OWR				*/
	TEGRA_GPIO_PEE0,			/* TEGRA_PINGROUP_CLK3_OUT			*/
	TEGRA_GPIO_PEE1,			/* TEGRA_PINGROUP_CLK3_REQ			*/
	TEGRA_GPIO_PC2,				/* TEGRA_PINGROUP_UART2_TXD			*/
	TEGRA_GPIO_PD7,				/* TEGRA_PINGROUP_VI_HSYNC			*/
	TEGRA_GPIO_PT1,				/* TEGRA_PINGROUP_VI_MCLK			*/
	TEGRA_GPIO_PD6,				/* TEGRA_PINGROUP_VI_VSYNC			*/
								/* TEGRA_PINGROUP_JTAG_TRST_N		*/
								/* TEGRA_PINGROUP_JTAG_TDO			*/
								/* TEGRA_PINGROUP_JTAG_TMS			*/
								/* TEGRA_PINGROUP_JTAG_TCK			*/
								/* TEGRA_PINGROUP_JTAG_TD			*/
};	

int pm_late_tristate_pin[] =
{
	TEGRA_GPIO_PEE2,			/* TEGRA_PINGROUP_CLK1_REQ			*/
	TEGRA_GPIO_PA4,				/* TEGRA_PINGROUP_DAP2_DIN			*/
	TEGRA_GPIO_PA5,				/* TEGRA_PINGROUP_DAP2_DOUT			*/
	TEGRA_GPIO_PA2,				/* TEGRA_PINGROUP_DAP2_FS			*/
	TEGRA_GPIO_PA3,				/* TEGRA_PINGROUP_DAP2_SCLK			*/
	TEGRA_GPIO_PX7,				/* TEGRA_PINGROUP_SPI1_MISO			*/
	TEGRA_GPIO_PX4,				/* TEGRA_PINGROUP_SPI1_MOSI			*/
	TEGRA_GPIO_PX3,				/* TEGRA_PINGROUP_SPI2_CS0_N		*/
	TEGRA_GPIO_PX1,				/* TEGRA_PINGROUP_SPI2_MISO			*/
	TEGRA_GPIO_PX0,				/* TEGRA_PINGROUP_SPI2_MOSI			*/
	TEGRA_GPIO_PX2,				/* TEGRA_PINGROUP_SPI2_SCK			*/
	TEGRA_GPIO_PBB1,			/* TEGRA_PINGROUP_CAM_I2C_SCL		*/
	TEGRA_GPIO_PBB2,			/* TEGRA_PINGROUP_CAM_I2C_SDA		*/
	TEGRA_GPIO_PCC0,			/* TEGRA_PINGROUP_CAM_MCLK			*/
	TEGRA_GPIO_PBB0,			/* TEGRA_PINGROUP_GPIO_PBB0			*/
	TEGRA_GPIO_PBB3,			/* TEGRA_PINGROUP_GPIO_PBB3			*/
	TEGRA_GPIO_PBB4,			/* TEGRA_PINGROUP_GPIO_PBB4			*/
	TEGRA_GPIO_PBB5,			/* TEGRA_PINGROUP_GPIO_PBB5			*/
	TEGRA_GPIO_PBB6,			/* TEGRA_PINGROUP_GPIO_PBB6			*/
	TEGRA_GPIO_PBB7,			/* TEGRA_PINGROUP_GPIO_PBB7			*/
	TEGRA_GPIO_PCC1,			/* TEGRA_PINGROUP_GPIO_PCC1			*/
	TEGRA_GPIO_PCC2,			/* TEGRA_PINGROUP_GPIO_PCC2			*/
	TEGRA_GPIO_PT5,				/* TEGRA_PINGROUP_GEN2_I2C_SCL		*/
	TEGRA_GPIO_PT6,				/* TEGRA_PINGROUP_GEN2_I2C_SDA		*/
	TEGRA_GPIO_PG0,				/* TEGRA_PINGROUP_GMI_AD0			*/
	TEGRA_GPIO_PG1,				/* TEGRA_PINGROUP_GMI_AD1			*/
	TEGRA_GPIO_PH2,				/* TEGRA_PINGROUP_GMI_AD10			*/
	TEGRA_GPIO_PH3,				/* TEGRA_PINGROUP_GMI_AD11			*/
	TEGRA_GPIO_PH4,				/* TEGRA_PINGROUP_GMI_AD12			*/
	TEGRA_GPIO_PH5,				/* TEGRA_PINGROUP_GMI_AD13			*/
	TEGRA_GPIO_PH6,				/* TEGRA_PINGROUP_GMI_AD14			*/
	TEGRA_GPIO_PH7,				/* TEGRA_PINGROUP_GMI_AD15			*/
	TEGRA_GPIO_PG2,				/* TEGRA_PINGROUP_GMI_AD2			*/
	TEGRA_GPIO_PG3,				/* TEGRA_PINGROUP_GMI_AD3			*/
	TEGRA_GPIO_PG4,				/* TEGRA_PINGROUP_GMI_AD4			*/
	TEGRA_GPIO_PG5,				/* TEGRA_PINGROUP_GMI_AD5			*/
	TEGRA_GPIO_PG6,				/* TEGRA_PINGROUP_GMI_AD6			*/
	TEGRA_GPIO_PG7,				/* TEGRA_PINGROUP_GMI_AD7			*/
	TEGRA_GPIO_PH0,				/* TEGRA_PINGROUP_GMI_AD8			*/
	TEGRA_GPIO_PH1,				/* TEGRA_PINGROUP_GMI_AD9			*/
	TEGRA_GPIO_PK0,				/* TEGRA_PINGROUP_GMI_ADV_N			*/
	TEGRA_GPIO_PK1,				/* TEGRA_PINGROUP_GMI_CLK			*/
	TEGRA_GPIO_PJ0,				/* TEGRA_PINGROUP_GMI_CS0_N			*/
	TEGRA_GPIO_PI1,				/* TEGRA_PINGROUP_GMI_OE_N			*/
	TEGRA_GPIO_PI4,				/* TEGRA_PINGROUP_GMI_RST_N			*/
	TEGRA_GPIO_PI7,				/* TEGRA_PINGROUP_GMI_WAIT			*/
	TEGRA_GPIO_PC7,				/* TEGRA_PINGROUP_GMI_WP_N			*/
	TEGRA_GPIO_PI0,				/* TEGRA_PINGROUP_GMI_WR_N			*/
	TEGRA_GPIO_PDD7,			/* TEGRA_PINGROUP_PEX_L2_PRSNT_N	*/
	TEGRA_GPIO_PDD3,			/* TEGRA_PINGROUP_PEX_WAKE_N		*/
/* FUJITSU TEN:2013-01-30 exclude PMU device. start */
//	TEGRA_GPIO_PZ6,				/* TEGRA_PINGROUP_PWR_I2C_SCL		*/
//	TEGRA_GPIO_PZ7,				/* TEGRA_PINGROUP_PWR_I2C_SDA		*/
/* FUJITSU TEN:2013-01-30 exclude PMU device. end */
	TEGRA_GPIO_PP5,				/* TEGRA_PINGROUP_DAP4_DIN			*/
	TEGRA_GPIO_PP6,				/* TEGRA_PINGROUP_DAP4_DOUT			*/
	TEGRA_GPIO_PP4,				/* TEGRA_PINGROUP_DAP4_FS			*/
	TEGRA_GPIO_PP7,				/* TEGRA_PINGROUP_DAP4_SCLK			*/
	TEGRA_GPIO_PC4,				/* TEGRA_PINGROUP_GEN1_I2C_SCL		*/
	TEGRA_GPIO_PC5,				/* TEGRA_PINGROUP_GEN1_I2C_SDA		*/
	TEGRA_GPIO_PU0,				/* TEGRA_PINGROUP_GPIO_PU0			*/
	TEGRA_GPIO_PU1,				/* TEGRA_PINGROUP_GPIO_PU1			*/
	TEGRA_GPIO_PU2,				/* TEGRA_PINGROUP_GPIO_PU2			*/
	TEGRA_GPIO_PU3,				/* TEGRA_PINGROUP_GPIO_PU3			*/
	TEGRA_GPIO_PU4,				/* TEGRA_PINGROUP_GPIO_PU4			*/
	TEGRA_GPIO_PU5,				/* TEGRA_PINGROUP_GPIO_PU5			*/
	TEGRA_GPIO_PU6,				/* TEGRA_PINGROUP_GPIO_PU6			*/
	TEGRA_GPIO_PJ5,				/* TEGRA_PINGROUP_UART2_CTS_N		*/
	TEGRA_GPIO_PJ6,				/* TEGRA_PINGROUP_UART2_RTS_N		*/
	TEGRA_GPIO_PA1,				/* TEGRA_PINGROUP_UART3_CTS_N		*/
	TEGRA_GPIO_PC0,				/* TEGRA_PINGROUP_UART3_RTS_N		*/
	TEGRA_GPIO_PW7,				/* TEGRA_PINGROUP_UART3_RXD			*/
	TEGRA_GPIO_PW6,				/* TEGRA_PINGROUP_UART3_TXD			*/
	TEGRA_GPIO_PU7,				/* TEGRA_PINGROUP_JTAG_RTCK			*/
};


/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      pm_set_tristate_pin                                                  */
/*  [DESCRIPTION]                                                            */
/*      early device tristate setting                                        */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      int       *set_state:        I       tristate setting                */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
void pm_set_tristate_pin(int set_state){
    int count = 0;

    /* Hi-Z setting of IO pins */
    for(count = 0; count < (sizeof(ada_tristate_early_device_pin)/sizeof(int)); count++){
#ifdef DEBUG_PRINT
		printk("Set tegra_gpio_set_tristate number(%d) pin(%d) \n",
					ada_tristate_early_device_pin[count], count);
#endif
        tegra_gpio_set_tristate(ada_tristate_early_device_pin[count], set_state);
#ifdef DEBUG_PRINT
		printk("End tegra_gpio_set_tristate number(%d) pin(%d) \n", 
					ada_tristate_early_device_pin[count], count);
#endif
    }
}

/*****************************************************************************/
/*  [SYMBOL]                                                                 */
/*      pm_set_late_tristate_pin                                             */
/*  [DESCRIPTION]                                                            */
/*      late device tristate setting                                         */
/*  [TYPE]                                                                   */
/*      void                                                                 */
/*  [PARAMETER]                                                              */
/*      int       *set_state:        I       tristate setting                */
/*  [RETURN]                                                                 */
/*      None                                                                 */
/*  [NOTE]                                                                   */
/*      None                                                                 */
/*****************************************************************************/
void pm_set_late_tristate_pin(int set_state){
    int count = 0;

    /* Hi-Z setting of IO pins */
    for(count = 0; count < (sizeof(pm_late_tristate_pin)/sizeof(int)); count++){
#ifdef DEBUG_PRINT
		printk("Set pm_late_tristate_pin number(%d) pin(%d) \n", 
							pm_late_tristate_pin[count], count);
#endif
        tegra_gpio_set_tristate(pm_late_tristate_pin[count], set_state);
#ifdef DEBUG_PRINT
		printk("End pm_late_tristate_pin number(%d) pin(%d) \n", 
							pm_late_tristate_pin[count], count);
#endif
    }
}

