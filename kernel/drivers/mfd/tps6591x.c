/*
 * driver/mfd/tps6591x.c
 *
 * Core driver for TI TPS6591x PMIC family
 *
 * Copyright (c) 2011-2013, NVIDIA CORPORATION.  All rights reserved.
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
 *
 */
/*
 *----------------------------------------------------------------------------
 * COPYRIGHT(C) FUJITSU TEN LIMITED 2012,2013
 *----------------------------------------------------------------------------
 */
 
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/i2c.h>

#include <linux/mfd/core.h>
#include <linux/mfd/tps6591x.h>
#include <linux/lites_trace.h>

/* device control registers */
#define TPS6591X_DEVCTRL	0x3F
#define DEVCTRL_PWR_OFF_SEQ	(1 << 7)
#define DEVCTRL_DEV_ON		(1 << 2)
#define DEVCTRL_DEV_SLP		(1 << 1)
#define DEVCTRL_DEV_OFF		(1 << 0)	/* FUJITSU TEN: 2013-10-17 add */
#define TPS6591X_DEVCTRL2	0x40

/* device sleep on registers */
#define TPS6591X_SLEEP_KEEP_ON	0x42
#define SLEEP_KEEP_ON_THERM	(1 << 7)
#define SLEEP_KEEP_ON_CLKOUT32K	(1 << 6)
#define SLEEP_KEEP_ON_VRTC	(1 << 5)
#define SLEEP_KEEP_ON_I2CHS	(1 << 4)

/* interrupt status registers */
#define TPS6591X_INT_STS	0x50
#define TPS6591X_INT_STS2	0x52
#define TPS6591X_INT_STS3	0x54

/* interrupt mask registers */
#define TPS6591X_INT_MSK	0x51
#define TPS6591X_INT_MSK2	0x53
#define TPS6591X_INT_MSK3	0x55

/* GPIO register base address */
#define TPS6591X_GPIO_BASE_ADDR	0x60

/* silicon version number */
#define TPS6591X_VERNUM		0x80

/* FUJITSU TEN:2012-12-05 thermal address add. start */
#define TPS6591X_THERMAL 0x38
#define DEFAULT_KEEP_LDO_ON_REG 0x40
#define DEFAULT_KEEP_RES_ON_REG 0x60
#define DEFAULT_SET_LDO_OFF_REG 0xBF
#define DEFAULT_SET_RES_OFF_REG 0x0A
#define KEEP_LDO_ON_REG_ADDR 0x41
#define KEEP_RES_ON_REG_ADDR 0x42
#define SET_LDO_OFF_REG_ADDR 0x43
#define SET_RES_OFF_REG_ADDR 0x44
#define TPS6591X_VERSION 0x6A
/* FUJITSU TEN:2012-12-05 thermal address add. end */

#define TPS6591X_GPIO_SLEEP	7
#define TPS6591X_GPIO_PDEN	3
#define TPS6591X_GPIO_DIR	2

/* FUJITSU TEN:2013-03-13 DRC add. start */
// Module ID
#define ID_PMU_DRIVER     4057

// エラーレベル
#define LV_DIAG    -1
#define LV_EMERG    0
#define LV_ALERT    1
#define LV_CRIT     2
#define LV_ERR      3
#define LV_WARNING  4
#define LV_NOTICE   5
#define LV_INFO     6
#define LV_DEBUG    7

// エラーID種別
#define ID_CREATE_FILE_ERROR 1
#define ID_IO_ERROR          2
#define ID_PMU_DISABLE       3
#define ID_ILLEGAL_STATUS    4

// ファイルID種別
#define ID_TPS6591X          1

// エラー情報種別
#define TYPE_NOT_USE         0 // 付加情報を利用しない

// エラー情報1種別
#define TYPE_CLASS_FAILED    1 // class"pmu"作成失敗
#define TYPE_TEMP_FAILED     2 // sysfs"temperature"作成失敗
#define TYPE_STATUS_FAILED   3 // sysfs"status"作成失敗
#define TYPE_READ_ERROR     11 // read error

struct ST_DRC_PMU_ERR_TRACE1{
    unsigned int err_id;
    unsigned int file_id;
    unsigned int line;
    unsigned int param1;
    unsigned int param2;
};
/* FUJITSU TEN:2013-03-13 DRC add. end */


enum irq_type {
	EVENT,
	GPIO,
};

struct tps6591x_irq_data {
	u8		mask_reg;
	u8		mask_pos;
	enum irq_type	type;
};

#define TPS6591X_IRQ(_reg, _mask_pos, _type)	\
	{					\
		.mask_reg	= (_reg),	\
		.mask_pos	= (_mask_pos),	\
		.type		= (_type),	\
	}

static const struct tps6591x_irq_data tps6591x_irqs[] = {
	[TPS6591X_INT_PWRHOLD_F]	= TPS6591X_IRQ(0, 0, EVENT),
	[TPS6591X_INT_VMBHI]		= TPS6591X_IRQ(0, 1, EVENT),
	[TPS6591X_INT_PWRON]		= TPS6591X_IRQ(0, 2, EVENT),
	[TPS6591X_INT_PWRON_LP]		= TPS6591X_IRQ(0, 3, EVENT),
	[TPS6591X_INT_PWRHOLD_R]	= TPS6591X_IRQ(0, 4, EVENT),
	[TPS6591X_INT_HOTDIE]		= TPS6591X_IRQ(0, 5, EVENT),
	[TPS6591X_INT_RTC_ALARM]	= TPS6591X_IRQ(0, 6, EVENT),
	[TPS6591X_INT_RTC_PERIOD]	= TPS6591X_IRQ(0, 7, EVENT),
	[TPS6591X_INT_GPIO0]		= TPS6591X_IRQ(1, 0, GPIO),
	[TPS6591X_INT_GPIO1]		= TPS6591X_IRQ(1, 2, GPIO),
	[TPS6591X_INT_GPIO2]		= TPS6591X_IRQ(1, 4, GPIO),
	[TPS6591X_INT_GPIO3]		= TPS6591X_IRQ(1, 6, GPIO),
	[TPS6591X_INT_GPIO4]		= TPS6591X_IRQ(2, 0, GPIO),
	[TPS6591X_INT_GPIO5]		= TPS6591X_IRQ(2, 2, GPIO),
	[TPS6591X_INT_WTCHDG]		= TPS6591X_IRQ(2, 4, EVENT),
	[TPS6591X_INT_VMBCH2_H]		= TPS6591X_IRQ(2, 5, EVENT),
	[TPS6591X_INT_VMBCH2_L]		= TPS6591X_IRQ(2, 6, EVENT),
	[TPS6591X_INT_PWRDN]		= TPS6591X_IRQ(2, 7, EVENT),
};

struct tps6591x {
	struct mutex		lock;
	struct device		*dev;
	struct i2c_client	*client;

	struct gpio_chip	gpio;
	struct irq_chip		irq_chip;
	struct mutex		irq_lock;
	int			irq_base;
	int			irq_main;
	u32			irq_en;
	u8			mask_cache[3];
	u8			mask_reg[3];
};

/* FUJITSU TEN:2013-03-13 DRC add. start */
#define DRC_ERR_REC(g_id, e_level, e_id, file, line_num, buf1, buf2) { \
	struct lites_trace_header header = {0}; \
	struct lites_trace_format format = {0}; \
	struct ST_DRC_PMU_ERR_TRACE1 err_info = {0};\
\
	header.level = e_level; \
	header.trace_no = ID_PMU_DRIVER; \
	header.trace_id = 0; \
	header.format_id = 1; \
	format.trc_header = &header; \
\
	err_info.err_id = e_id; \
	err_info.file_id = file; \
	err_info.line = line_num; \
	err_info.param1 = buf1; \
	err_info.param2 = buf2; \
	format.buf = &err_info; \
	format.count = sizeof(err_info); \
\
	lites_trace_write(g_id, &format); \
}
/* FUJITSU TEN:2013-03-13 DRC add. end */

/* FUJITSU TEN:2012-12-05 logmacro add. start */

#define TPS6591X_EMERG_LOG( format, ...) { \
	printk( KERN_EMERG "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define TPS6591X_ERR_LOG( format, ...) { \
	printk( KERN_ERR "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define TPS6591X_WARNING_LOG( format, ...) { \
	printk( KERN_WARNING "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#if defined(CONFIG_SENSORS_TPS6591X_LOG_NOTICE) || defined(CONFIG_SENSORS_TPS6591X_LOG_INFO) \
	|| defined(CONFIG_SENSORS_TPS6591X_LOG_DEBUG)
#define TPS6591X_NOTICE_LOG( format, ...) { \
	printk( KERN_NOTICE "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define TPS6591X_NOTICE_LOG( format, ... )
#endif

#if defined(CONFIG_SENSORS_TPS6591X_LOG_INFO) || defined(CONFIG_SENSORS_TPS6591X_LOG_DEBUG)
#define TPS6591X_INFO_LOG( format, ...) { \
	printk( KERN_INFO "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define TPS6591X_INFO_LOG( format, ... )
#endif

#ifdef CONFIG_SENSORS_TPS6591X_LOG_DEBUG 
#define TPS6591X_DEBUG_LOG( format, ...) { \
	printk( KERN_DEBUG "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define TPS6591X_DEBUG_LOG( format, ... )
#endif
/* FUJITSU TEN:2012-12-05 logmacro add. end */

/* FUJITSU TEN:2012-12-05 therm_register add. start */
struct tps6591x_therm_reg {
    int therm_state;// thermal module status(0:disable 1:enable)
    int rsvd1;      // reserved bit(2bit)
    int therm_hdsel;// temperature selection for Hot Die detector(00:low 11:high)
    int therm_ts;   // thermal shutdown detector(0:not reached 1:reached)
    int therm_hd;   // hot die detector(0:not reached 1:reached)
    int reserved;   // reserved bit(2bit)
};
/* FUJITSU TEN:2012-12-05 therm_register add. end */

/* FUJITSU TEN:2012-12-05 sleep_register add. start */
struct tps6591x_sleep_reg {
    int keep_ldo_on_reg;
    int keep_res_on_reg;
    int set_ldo_off_reg;
    int set_res_off_reg;
};
/* FUJITSU TEN:2012-12-05 sleep_register add. end */

static inline int __tps6591x_read(struct i2c_client *client,
				  int reg, uint8_t *val)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading at 0x%02x\n", reg);
		return ret;
	}

	*val = (uint8_t)ret;

	return 0;
}

static inline int __tps6591x_reads(struct i2c_client *client, int reg,
				int len, uint8_t *val)
{
	int ret;

	ret = i2c_smbus_read_i2c_block_data(client, reg, len, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed reading from 0x%02x\n", reg);
		return ret;
	}

	return 0;
}

static inline int __tps6591x_write(struct i2c_client *client,
				 int reg, uint8_t val)
{
	int ret;
	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writing 0x%02x to 0x%02x\n",
				val, reg);
		return ret;
	}

	return 0;
}

static inline int __tps6591x_writes(struct i2c_client *client, int reg,
				  int len, uint8_t *val)
{
	int ret;

	ret = i2c_smbus_write_i2c_block_data(client, reg, len, val);
	if (ret < 0) {
		dev_err(&client->dev, "failed writings to 0x%02x\n", reg);
		return ret;
	}

	return 0;
}

int tps6591x_write(struct device *dev, int reg, uint8_t val)
{
	struct tps6591x *tps6591x = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&tps6591x->lock);
	ret = __tps6591x_write(to_i2c_client(dev), reg, val);
	mutex_unlock(&tps6591x->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(tps6591x_write);

int tps6591x_writes(struct device *dev, int reg, int len, uint8_t *val)
{
	struct tps6591x *tps6591x = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&tps6591x->lock);
	ret = __tps6591x_writes(to_i2c_client(dev), reg, len, val);
	mutex_unlock(&tps6591x->lock);

	return ret;
}
EXPORT_SYMBOL_GPL(tps6591x_writes);

int tps6591x_read(struct device *dev, int reg, uint8_t *val)
{
	return __tps6591x_read(to_i2c_client(dev), reg, val);
}
EXPORT_SYMBOL_GPL(tps6591x_read);

int tps6591x_reads(struct device *dev, int reg, int len, uint8_t *val)
{
	return __tps6591x_reads(to_i2c_client(dev), reg, len, val);
}
EXPORT_SYMBOL_GPL(tps6591x_reads);

int tps6591x_set_bits(struct device *dev, int reg, uint8_t bit_mask)
{
	struct tps6591x *tps6591x = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&tps6591x->lock);

	ret = __tps6591x_read(to_i2c_client(dev), reg, &reg_val);
	if (ret)
		goto out;

	if ((reg_val & bit_mask) != bit_mask) {
		reg_val |= bit_mask;
		ret = __tps6591x_write(to_i2c_client(dev), reg, reg_val);
	}
out:
	mutex_unlock(&tps6591x->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(tps6591x_set_bits);

int tps6591x_clr_bits(struct device *dev, int reg, uint8_t bit_mask)
{
	struct tps6591x *tps6591x = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&tps6591x->lock);

	ret = __tps6591x_read(to_i2c_client(dev), reg, &reg_val);
	if (ret)
		goto out;

	if (reg_val & bit_mask) {
		reg_val &= ~bit_mask;
		ret = __tps6591x_write(to_i2c_client(dev), reg, reg_val);
	}
out:
	mutex_unlock(&tps6591x->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(tps6591x_clr_bits);

int tps6591x_update(struct device *dev, int reg, uint8_t val, uint8_t mask)
{
	struct tps6591x *tps6591x = dev_get_drvdata(dev);
	uint8_t reg_val;
	int ret = 0;

	mutex_lock(&tps6591x->lock);

	ret = __tps6591x_read(tps6591x->client, reg, &reg_val);
	if (ret)
		goto out;

	if ((reg_val & mask) != val) {
		reg_val = (reg_val & ~mask) | (val & mask);
		ret = __tps6591x_write(tps6591x->client, reg, reg_val);
	}
out:
	mutex_unlock(&tps6591x->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(tps6591x_update);

static struct i2c_client *tps6591x_i2c_client;
void tps6591x_power_off(void)
{
	struct device *dev = NULL;

	if (!tps6591x_i2c_client)
		return;

	dev = &tps6591x_i2c_client->dev;

	/* FUJITSU TEN:2013-10-17 change begin [DEVCTRL bit:0 Low->High]  */
	if (tps6591x_set_bits(dev, TPS6591X_DEVCTRL, DEVCTRL_DEV_OFF) < 0)
		return;
	/* FUJITSU TEN:2013-10-17 change end [DEVCTRL bit:0 Low->High]  */

	if (tps6591x_set_bits(dev, TPS6591X_DEVCTRL, DEVCTRL_PWR_OFF_SEQ) < 0)
		return;

	tps6591x_clr_bits(dev, TPS6591X_DEVCTRL, DEVCTRL_DEV_ON);
}

static int tps6591x_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	struct tps6591x *tps6591x = container_of(gc, struct tps6591x, gpio);
	uint8_t val;
	int ret;

	ret = __tps6591x_read(tps6591x->client, TPS6591X_GPIO_BASE_ADDR +
			offset,	&val);
	if (ret)
		return ret;

	if (val & 0x4)
		return val & 0x1;
	else
		return (val & 0x2) ? 1 : 0;
}

static void tps6591x_gpio_set(struct gpio_chip *chip, unsigned offset,
			int value)
{

	struct tps6591x *tps6591x = container_of(chip, struct tps6591x, gpio);

	tps6591x_update(tps6591x->dev, TPS6591X_GPIO_BASE_ADDR + offset,
			value, 0x1);
}

static int tps6591x_gpio_input(struct gpio_chip *gc, unsigned offset)
{
	struct tps6591x *tps6591x = container_of(gc, struct tps6591x, gpio);
	uint8_t reg_val;
	int ret;

	ret = __tps6591x_read(tps6591x->client, TPS6591X_GPIO_BASE_ADDR +
			offset,	&reg_val);
	if (ret)
		return ret;

	reg_val &= ~0x4;
	return __tps6591x_write(tps6591x->client, TPS6591X_GPIO_BASE_ADDR +
			offset,	reg_val);
}

static int tps6591x_gpio_output(struct gpio_chip *gc, unsigned offset,
				int value)
{
	struct tps6591x *tps6591x = container_of(gc, struct tps6591x, gpio);
	uint8_t reg_val, val;
	int ret;

	ret = __tps6591x_read(tps6591x->client, TPS6591X_GPIO_BASE_ADDR +
			offset,	&reg_val);
	if (ret)
		return ret;

	reg_val &= ~0x1;
	val = (value & 0x1) | 0x4;
	reg_val = reg_val | val;
	return __tps6591x_write(tps6591x->client, TPS6591X_GPIO_BASE_ADDR +
			offset,	reg_val);
}

static int tps6591x_gpio_to_irq(struct gpio_chip *gc, unsigned off)
{
	struct tps6591x *tps6591x;
	tps6591x = container_of(gc, struct tps6591x, gpio);

	if ((off >= 0) && (off <= TPS6591X_INT_GPIO5 - TPS6591X_INT_GPIO0))
		return tps6591x->irq_base + TPS6591X_INT_GPIO0 + off;

	return -EIO;
}

static void tps6591x_gpio_init(struct tps6591x *tps6591x,
			struct tps6591x_platform_data *pdata)
{
	int ret;
	int gpio_base = pdata->gpio_base;
	int i;
	u8 gpio_reg;
	struct tps6591x_gpio_init_data *ginit;

	if (gpio_base <= 0)
		return;

	for (i = 0; i < pdata->num_gpioinit_data; ++i) {
		ginit = &pdata->gpio_init_data[i];
		if (!ginit->init_apply)
			continue;
		gpio_reg = (ginit->sleep_en << TPS6591X_GPIO_SLEEP) |
				(ginit->pulldn_en << TPS6591X_GPIO_PDEN) |
				(ginit->output_mode_en << TPS6591X_GPIO_DIR);

		if (ginit->output_mode_en)
			gpio_reg |= ginit->output_val;

		ret =  __tps6591x_write(tps6591x->client,
				TPS6591X_GPIO_BASE_ADDR + i, gpio_reg);
		if (ret < 0)
			dev_err(&tps6591x->client->dev, "Gpio %d init "
				"configuration failed: %d\n", i, ret);
	}

	tps6591x->gpio.owner		= THIS_MODULE;
	tps6591x->gpio.label		= tps6591x->client->name;
	tps6591x->gpio.dev		= tps6591x->dev;
	tps6591x->gpio.base		= gpio_base;
	tps6591x->gpio.ngpio		= TPS6591X_GPIO_NR;
	tps6591x->gpio.can_sleep	= 1;

	tps6591x->gpio.direction_input	= tps6591x_gpio_input;
	tps6591x->gpio.direction_output	= tps6591x_gpio_output;
	tps6591x->gpio.set		= tps6591x_gpio_set;
	tps6591x->gpio.get		= tps6591x_gpio_get;
	tps6591x->gpio.to_irq		= tps6591x_gpio_to_irq;

	ret = gpiochip_add(&tps6591x->gpio);
	if (ret)
		dev_warn(tps6591x->dev, "GPIO registration failed: %d\n", ret);
}

static int __remove_subdev(struct device *dev, void *unused)
{
	platform_device_unregister(to_platform_device(dev));
	return 0;
}

static int tps6591x_remove_subdevs(struct tps6591x *tps6591x)
{
	return device_for_each_child(tps6591x->dev, NULL, __remove_subdev);
}

static void tps6591x_irq_lock(struct irq_data *data)
{
	struct tps6591x *tps6591x = irq_data_get_irq_chip_data(data);

	mutex_lock(&tps6591x->irq_lock);
}

static void tps6591x_irq_mask(struct irq_data *irq_data)
{
	struct tps6591x *tps6591x = irq_data_get_irq_chip_data(irq_data);
	unsigned int __irq = irq_data->irq - tps6591x->irq_base;
	const struct tps6591x_irq_data *data = &tps6591x_irqs[__irq];

	if (data->type == EVENT)
		tps6591x->mask_reg[data->mask_reg] |= (1 << data->mask_pos);
	else
		tps6591x->mask_reg[data->mask_reg] |= (3 << data->mask_pos);

	tps6591x->irq_en &= ~(1 << __irq);
}

static void tps6591x_irq_unmask(struct irq_data *irq_data)
{
	struct tps6591x *tps6591x = irq_data_get_irq_chip_data(irq_data);

	unsigned int __irq = irq_data->irq - tps6591x->irq_base;
	const struct tps6591x_irq_data *data = &tps6591x_irqs[__irq];

	if (data->type == EVENT) {
		tps6591x->mask_reg[data->mask_reg] &= ~(1 << data->mask_pos);
		tps6591x->irq_en |= (1 << __irq);
	}
}

static void tps6591x_irq_sync_unlock(struct irq_data *data)
{
	struct tps6591x *tps6591x = irq_data_get_irq_chip_data(data);
	int i;

	for (i = 0; i < ARRAY_SIZE(tps6591x->mask_reg); i++) {
		if (tps6591x->mask_reg[i] != tps6591x->mask_cache[i]) {
			if (!WARN_ON(tps6591x_write(tps6591x->dev,
						TPS6591X_INT_MSK + 2*i,
						tps6591x->mask_reg[i])))
				tps6591x->mask_cache[i] = tps6591x->mask_reg[i];
		}
	}

	mutex_unlock(&tps6591x->irq_lock);
}

static int tps6591x_irq_set_type(struct irq_data *irq_data, unsigned int type)
{
	struct tps6591x *tps6591x = irq_data_get_irq_chip_data(irq_data);

	unsigned int __irq = irq_data->irq - tps6591x->irq_base;
	const struct tps6591x_irq_data *data = &tps6591x_irqs[__irq];

	if (data->type == GPIO) {
		if (type & IRQ_TYPE_EDGE_FALLING)
			tps6591x->mask_reg[data->mask_reg]
				&= ~(1 << data->mask_pos);
		else
			tps6591x->mask_reg[data->mask_reg]
				|= (1 << data->mask_pos);

		if (type & IRQ_TYPE_EDGE_RISING)
			tps6591x->mask_reg[data->mask_reg]
				&= ~(2 << data->mask_pos);
		else
			tps6591x->mask_reg[data->mask_reg]
				|= (2 << data->mask_pos);

		tps6591x->irq_en |= (1 << __irq);
	}

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int tps6591x_irq_set_wake(struct irq_data *irq_data, unsigned int on)
{
	struct tps6591x *tps6591x = irq_data_get_irq_chip_data(irq_data);
	return irq_set_irq_wake(tps6591x->irq_main, on);
}
#else
#define tps6591x_irq_set_wake NULL
#endif


static irqreturn_t tps6591x_irq(int irq, void *data)
{
	struct tps6591x *tps6591x = data;
	int ret = 0;
	u8 tmp[3];
	u8 int_ack;
	u32 acks, mask = 0;
	int i;

	for (i = 0; i < 3; i++) {
		ret = tps6591x_read(tps6591x->dev, TPS6591X_INT_STS + 2*i,
				&tmp[i]);
		if (ret < 0) {
			dev_err(tps6591x->dev,
				"failed to read interrupt status\n");
			return IRQ_NONE;
		}
		if (tmp[i]) {
			/* Ack only those interrupts which are enabled */
			int_ack = tmp[i] & (~(tps6591x->mask_cache[i]));
			ret = tps6591x_write(tps6591x->dev,
					TPS6591X_INT_STS + 2*i,	int_ack);
			if (ret < 0) {
				dev_err(tps6591x->dev,
					"failed to write interrupt status\n");
				return IRQ_NONE;
			}
		}
	}

	acks = (tmp[2] << 16) | (tmp[1] << 8) | tmp[0];

	for (i = 0; i < ARRAY_SIZE(tps6591x_irqs); i++) {
		if (tps6591x_irqs[i].type == GPIO)
			mask = (3 << (tps6591x_irqs[i].mask_pos
					+ tps6591x_irqs[i].mask_reg*8));
		else if (tps6591x_irqs[i].type == EVENT)
			mask = (1 << (tps6591x_irqs[i].mask_pos
					+ tps6591x_irqs[i].mask_reg*8));

		if ((acks & mask) && (tps6591x->irq_en & (1 << i)))
			handle_nested_irq(tps6591x->irq_base + i);
	}
	return IRQ_HANDLED;
}

static int __devinit tps6591x_irq_init(struct tps6591x *tps6591x, int irq,
				int irq_base)
{
	int i, ret;

	if (!irq_base) {
		dev_warn(tps6591x->dev, "No interrupt support on IRQ base\n");
		return -EINVAL;
	}

	mutex_init(&tps6591x->irq_lock);

	tps6591x->mask_reg[0] = 0xFF;
	tps6591x->mask_reg[1] = 0xFF;
	tps6591x->mask_reg[2] = 0xFF;
	for (i = 0; i < 3; i++) {
		tps6591x->mask_cache[i] = tps6591x->mask_reg[i];
		tps6591x_write(tps6591x->dev, TPS6591X_INT_MSK + 2*i,
				 tps6591x->mask_cache[i]);
	}

	for (i = 0; i < 3; i++)
		tps6591x_write(tps6591x->dev, TPS6591X_INT_STS + 2*i, 0xff);

	tps6591x->irq_base = irq_base;
	tps6591x->irq_main = irq;

	tps6591x->irq_chip.name = "tps6591x";
	tps6591x->irq_chip.irq_mask = tps6591x_irq_mask;
	tps6591x->irq_chip.irq_unmask = tps6591x_irq_unmask;
	tps6591x->irq_chip.irq_bus_lock = tps6591x_irq_lock;
	tps6591x->irq_chip.irq_bus_sync_unlock = tps6591x_irq_sync_unlock;
	tps6591x->irq_chip.irq_set_type = tps6591x_irq_set_type;
	tps6591x->irq_chip.irq_set_wake = tps6591x_irq_set_wake;

	for (i = 0; i < ARRAY_SIZE(tps6591x_irqs); i++) {
		int __irq = i + tps6591x->irq_base;
		irq_set_chip_data(__irq, tps6591x);
		irq_set_chip_and_handler(__irq, &tps6591x->irq_chip,
					 handle_simple_irq);
		irq_set_nested_thread(__irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(__irq, IRQF_VALID);
#endif
	}

	ret = request_threaded_irq(irq, NULL, tps6591x_irq, IRQF_ONESHOT,
				"tps6591x", tps6591x);
	if (!ret) {
		device_init_wakeup(tps6591x->dev, 1);
		enable_irq_wake(irq);
	}

	return ret;
}

static int __devinit tps6591x_add_subdevs(struct tps6591x *tps6591x,
					  struct tps6591x_platform_data *pdata)
{
	struct tps6591x_subdev_info *subdev;
	struct platform_device *pdev;
	int i, ret = 0;

	for (i = 0; i < pdata->num_subdevs; i++) {
		subdev = &pdata->subdevs[i];

		pdev = platform_device_alloc(subdev->name, subdev->id);

		pdev->dev.parent = tps6591x->dev;
		pdev->dev.platform_data = subdev->platform_data;

		ret = platform_device_add(pdev);
		if (ret)
			goto failed;
	}
	return 0;

failed:
	tps6591x_remove_subdevs(tps6591x);
	return ret;
}
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/seq_file.h>
static void print_regs(const char *header, struct seq_file *s,
		struct i2c_client *client, int start_offset,
		int end_offset)
{
	uint8_t reg_val;
	int i;
	int ret;

	seq_printf(s, "%s\n", header);
	for (i = start_offset; i <= end_offset; ++i) {
		ret = __tps6591x_read(client, i, &reg_val);
		if (ret >= 0)
			seq_printf(s, "Reg 0x%02x Value 0x%02x\n", i, reg_val);
	}
	seq_printf(s, "------------------\n");
}

static int dbg_tps_show(struct seq_file *s, void *unused)
{
	struct tps6591x *tps = s->private;
	struct i2c_client *client = tps->client;

	seq_printf(s, "TPS6591x Registers\n");
	seq_printf(s, "------------------\n");

	print_regs("Timing Regs",    s, client, 0x0, 0x6);
	print_regs("Alarm Regs",     s, client, 0x8, 0xD);
	print_regs("RTC Regs",       s, client, 0x10, 0x16);
	print_regs("BCK Regs",       s, client, 0x17, 0x1B);
	print_regs("PUADEN Regs",    s, client, 0x18, 0x18);
	print_regs("REF Regs",       s, client, 0x1D, 0x1D);
	print_regs("VDD Regs",       s, client, 0x1E, 0x29);
	print_regs("LDO Regs",       s, client, 0x30, 0x37);
	print_regs("THERM Regs",     s, client, 0x38, 0x38);
	print_regs("BBCH Regs",      s, client, 0x39, 0x39);
	print_regs("DCDCCNTRL Regs", s, client, 0x3E, 0x3E);
	print_regs("DEV_CNTRL Regs", s, client, 0x3F, 0x40);
	print_regs("SLEEP Regs",     s, client, 0x41, 0x44);
	print_regs("EN1 Regs",       s, client, 0x45, 0x48);
	print_regs("INT Regs",       s, client, 0x50, 0x55);
	print_regs("GPIO Regs",      s, client, 0x60, 0x68);
	print_regs("WATCHDOG Regs",  s, client, 0x69, 0x69);
	print_regs("VMBCH Regs",     s, client, 0x6A, 0x6B);
	print_regs("LED_CTRL Regs",  s, client, 0x6c, 0x6D);
	print_regs("PWM_CTRL Regs",  s, client, 0x6E, 0x6F);
	print_regs("SPARE Regs",     s, client, 0x70, 0x70);
	print_regs("VERNUM Regs",    s, client, 0x80, 0x80);
	return 0;
}

static int dbg_tps_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_tps_show, inode->i_private);
}

static const struct file_operations debug_fops = {
	.open		= dbg_tps_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void __init tps6591x_debuginit(struct tps6591x *tps)
{
	(void)debugfs_create_file("tps6591x", S_IRUGO, NULL,
			tps, &debug_fops);
}
#else
static void __init tps6591x_debuginit(struct tps6591x *tpsi)
{
	return;
}
#endif

static int __init tps6591x_sleepinit(struct tps6591x *tpsi,
					struct tps6591x_platform_data *pdata)
{
	struct device *dev = NULL;
	int ret = 0;

	dev = tpsi->dev;

	if (!pdata->dev_slp_en)
		goto no_err_return;

	/* pmu dev_slp_en is set. Make sure slp_keepon is available before
	 * allowing SLEEP device state */
	if (!pdata->slp_keepon) {
		dev_err(dev, "slp_keepon_data required for slp_en\n");
		goto err_sleep_init;
	}

	/* enabling SLEEP device state */
	if (!pdata->dev_slp_delayed)
		ret = tps6591x_set_bits(dev, TPS6591X_DEVCTRL, DEVCTRL_DEV_SLP);
	else
		ret = tps6591x_clr_bits(dev, TPS6591X_DEVCTRL, DEVCTRL_DEV_SLP);
	if (ret < 0) {
		dev_err(dev, "set dev_slp failed: %d\n", ret);
		goto err_sleep_init;
	}

	if (pdata->slp_keepon->therm_keepon) {
		ret = tps6591x_set_bits(dev, TPS6591X_SLEEP_KEEP_ON,
						SLEEP_KEEP_ON_THERM);
		if (ret < 0) {
			dev_err(dev, "set therm_keepon failed: %d\n", ret);
			goto disable_dev_slp;
		}
	}

	if (pdata->slp_keepon->clkout32k_keepon) {
		ret = tps6591x_set_bits(dev, TPS6591X_SLEEP_KEEP_ON,
						SLEEP_KEEP_ON_CLKOUT32K);
		if (ret < 0) {
			dev_err(dev, "set clkout32k_keepon failed: %d\n", ret);
			goto disable_dev_slp;
		}
	}


	if (pdata->slp_keepon->vrtc_keepon) {
		ret = tps6591x_set_bits(dev, TPS6591X_SLEEP_KEEP_ON,
						SLEEP_KEEP_ON_VRTC);
		if (ret < 0) {
			dev_err(dev, "set vrtc_keepon failed: %d\n", ret);
			goto disable_dev_slp;
		}
	}

	if (pdata->slp_keepon->i2chs_keepon) {
		ret = tps6591x_set_bits(dev, TPS6591X_SLEEP_KEEP_ON,
						SLEEP_KEEP_ON_I2CHS);
		if (ret < 0) {
			dev_err(dev, "set i2chs_keepon failed: %d\n", ret);
			goto disable_dev_slp;
		}
	}

no_err_return:
	return 0;

disable_dev_slp:
	tps6591x_clr_bits(dev, TPS6591X_DEVCTRL, DEVCTRL_DEV_SLP);

err_sleep_init:
	return ret;
}

/* FUJITSU TEN:2012-12-05 show temperature add. start */
static ssize_t tps6591x_temperature_show(struct device *dev, struct device_attribute *attr, char *buf)
{
        TPS6591X_DEBUG_LOG("start.\n");
        uint8_t val;
        int ret = -1;
        int bin_array[8];
        int mask = 1;
        int counter = 0;
        struct tps6591x_therm_reg therm_now_status = {0, 0, 0, 0, 0, 0};
        
        /* read temperature */
        ret = __tps6591x_read(tps6591x_i2c_client, TPS6591X_THERMAL, &val);
        if(ret < 0){
                TPS6591X_ERR_LOG("read failed.\n");
                DRC_ERR_REC(REGION_TRACE_ERROR, LV_ERR, ID_IO_ERROR, ID_TPS6591X, __LINE__, TYPE_READ_ERROR, TYPE_NOT_USE);
                return -EIO;
        }
        TPS6591X_DEBUG_LOG("temperature read success.\n");

        /* read data change to bit data */
        for(counter = 0; counter < 8; counter++){
            bin_array[counter] = (val & mask);
            val = val >> 1;
         }
        
        /* save read data */
        therm_now_status.therm_state = bin_array[0];
        therm_now_status.rsvd1 = bin_array[1];
        therm_now_status.therm_hdsel = (bin_array[3] << 1)| bin_array[2];
        therm_now_status.therm_ts = bin_array[4];
        therm_now_status.therm_hd = bin_array[5];
        therm_now_status.reserved = (bin_array[7] << 1)| bin_array[6];
         
        TPS6591X_DEBUG_LOG("therm_state=%d\n", therm_now_status.therm_state);
        TPS6591X_DEBUG_LOG("rsvd1=%d\n", therm_now_status.rsvd1);
        TPS6591X_DEBUG_LOG("therm_hdsel=%d\n", therm_now_status.therm_hdsel);
        TPS6591X_DEBUG_LOG("therm_ts=%d\n", therm_now_status.therm_ts);
        TPS6591X_DEBUG_LOG("therm_hd=%d\n", therm_now_status.therm_hd);
        TPS6591X_DEBUG_LOG("reserved=%d\n", therm_now_status.reserved);

        /* check status */
        if(therm_now_status.therm_state == 0){
            TPS6591X_ERR_LOG("thermal status is Disable.\n");
            DRC_ERR_REC(REGION_TRACE_ERROR, LV_ERR, ID_PMU_DISABLE, ID_TPS6591X, __LINE__, TYPE_NOT_USE, TYPE_NOT_USE);
        }

        TPS6591X_DEBUG_LOG("end.\n");
        return sprintf(buf, "%d\n", therm_now_status.therm_hd);
}
/* FUJITSU TEN:2012-12-05 show temperature add. end */

/* FUJITSU TEN:2012-12-05 show status add. start */
static ssize_t tps6591x_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
        TPS6591X_DEBUG_LOG("start.\n");

        uint8_t val[4];
        int illegal_flag = 0;
        int ret;
        int counter = 0;
        struct tps6591x_sleep_reg tps6591x_now_status = {0, 0, 0, 0};
        int default_reg_array[4] = {
                                    DEFAULT_KEEP_LDO_ON_REG,
                                    DEFAULT_KEEP_RES_ON_REG,
                                    DEFAULT_SET_LDO_OFF_REG,
                                    DEFAULT_SET_RES_OFF_REG
                                    };
        int read_addr_array[4] = {
                                    KEEP_LDO_ON_REG_ADDR,
                                    KEEP_RES_ON_REG_ADDR,
                                    SET_LDO_OFF_REG_ADDR,
                                    SET_RES_OFF_REG_ADDR
                                    };

        /* read status */
        for(counter = 0; counter < 4; counter++){
            ret = __tps6591x_read(tps6591x_i2c_client, read_addr_array[counter], &val[counter]);
            TPS6591X_DEBUG_LOG("addr=0x%02x -> ret=%d, val[%d]=0x%02x.\n",
                    read_addr_array[counter], ret, counter, val[counter]);
            if(ret < 0){
                TPS6591X_ERR_LOG("read failed.\n");
                DRC_ERR_REC(REGION_TRACE_ERROR, LV_ERR, ID_IO_ERROR, ID_TPS6591X, __LINE__, TYPE_READ_ERROR, TYPE_NOT_USE);
                return -EIO;
            }
        }
        TPS6591X_DEBUG_LOG("status read success.\n");

        /* compere to default status */
        for(counter = 0; counter < 4; counter++){
            if(val[counter] != default_reg_array[counter]){
                TPS6591X_ERR_LOG("val[%d] is not Default.\n", counter);
                DRC_ERR_REC(REGION_TRACE_ERROR, LV_ERR, ID_ILLEGAL_STATUS, ID_TPS6591X, __LINE__, read_addr_array[counter], TYPE_NOT_USE);
                illegal_flag = 1;
            }
        }

        TPS6591X_DEBUG_LOG("end.\n");
        return sprintf(buf, "%d\n", illegal_flag);
}
/* FUJITSU TEN:2012-12-05 show status add. end */

/* FUJITSU TEN:2012-12-05 show version add. start */
static ssize_t tps6591x_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
        TPS6591X_DEBUG_LOG("start.\n");
        uint8_t val;
        int ret = -1;
        
        /* read version */
        ret = __tps6591x_read(tps6591x_i2c_client, TPS6591X_VERSION, &val);
        if(ret < 0){
                TPS6591X_ERR_LOG("read failed.\n");
                DRC_ERR_REC(REGION_TRACE_ERROR, LV_ERR, ID_IO_ERROR, ID_TPS6591X, __LINE__, TYPE_READ_ERROR, TYPE_NOT_USE);
                return -EIO;
        }
        TPS6591X_DEBUG_LOG("version read success.\n");
        
        TPS6591X_DEBUG_LOG("end.\n");
        return sprintf(buf, "%x\n", val);
}
/* FUJITSU TEN:2012-12-05 show version add. end */

/* FUJITSU TEN:2012-12-05 create sysfs add. start */
struct class *pmu_class;
static struct class_attribute pmu_temperature_file = __ATTR(temperature,  
                                        S_IRUGO  ,tps6591x_temperature_show, NULL);
static struct class_attribute pmu_status_file = __ATTR(status,  
                                        S_IRUGO  ,tps6591x_status_show, NULL);
static struct class_attribute pmu_version_file = __ATTR(version,  
                                        S_IRUGO  ,tps6591x_version_show, NULL);

static int create_pmu_class(void)
{
    TPS6591X_INFO_LOG("start.\n");

    int err;

    /* create class */
    pmu_class = class_create(THIS_MODULE, "pmu");  
    if(IS_ERR(pmu_class)){
        TPS6591X_ERR_LOG("Failed to create class 'pmu'.\n");
        DRC_ERR_REC(REGION_TRACE_ERROR, LV_ERR, ID_CREATE_FILE_ERROR, ID_TPS6591X, __LINE__, TYPE_CLASS_FAILED, TYPE_NOT_USE);
        return 1;
    }
    TPS6591X_NOTICE_LOG("success create class 'pmu'.\n");

    /* create temperature file */
    err = class_create_file(pmu_class, &pmu_temperature_file);
    if(err){
        TPS6591X_ERR_LOG("Failed to create file 'temperature'.\n");
        DRC_ERR_REC(REGION_TRACE_ERROR, LV_ERR, ID_CREATE_FILE_ERROR, ID_TPS6591X, __LINE__, TYPE_TEMP_FAILED, TYPE_NOT_USE);
        class_destroy(pmu_class);
        return 1;  
    }
    TPS6591X_NOTICE_LOG("success create file 'temperature'.\n");

    /* create status file */
    err = class_create_file(pmu_class, &pmu_status_file);
    if(err){
        TPS6591X_ERR_LOG("Failed to create file 'status'.\n");
        DRC_ERR_REC(REGION_TRACE_ERROR, LV_ERR, ID_CREATE_FILE_ERROR, ID_TPS6591X, __LINE__, TYPE_STATUS_FAILED, TYPE_NOT_USE);
        class_destroy(pmu_class);
        return 1;  
    }
    TPS6591X_NOTICE_LOG("success create file 'status'.\n");

    /* create version file */
    err = class_create_file(pmu_class, &pmu_version_file);
    if(err){
        TPS6591X_ERR_LOG("Failed to create file 'version'.\n");
        class_destroy(pmu_class);
        return 1;  
    }
    TPS6591X_NOTICE_LOG("success create file 'version'.\n");
    
    TPS6591X_INFO_LOG("end.\n");
    return 0;
}
/* FUJITSU TEN:2012-12-05 create sysfs add. end */

static int __devinit tps6591x_i2c_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
/* FUJITSU TEN:2012-12-05 log add. start */
	TPS6591X_DEBUG_LOG("start.\n");
/* FUJITSU TEN:2012-12-05 log add. end */
	struct tps6591x_platform_data *pdata = client->dev.platform_data;
	struct tps6591x *tps6591x;
	int ret;

	if (!pdata) {
		dev_err(&client->dev, "tps6591x requires platform data\n");
		return -ENOTSUPP;
	}

	ret = i2c_smbus_read_byte_data(client, TPS6591X_VERNUM);
	if (ret < 0) {
		dev_err(&client->dev, "Silicon version number read"
				" failed: %d\n", ret);
		return -EIO;
	}

	dev_info(&client->dev, "VERNUM is %02x\n", ret);

	tps6591x = kzalloc(sizeof(struct tps6591x), GFP_KERNEL);
	if (tps6591x == NULL)
		return -ENOMEM;

	tps6591x->client = client;
	tps6591x->dev = &client->dev;
	i2c_set_clientdata(client, tps6591x);

	mutex_init(&tps6591x->lock);

	if (client->irq) {
		ret = tps6591x_irq_init(tps6591x, client->irq,
					pdata->irq_base);
		if (ret) {
			dev_err(&client->dev, "IRQ init failed: %d\n", ret);
			goto err_irq_init;
		}
	}

	ret = tps6591x_add_subdevs(tps6591x, pdata);
	if (ret) {
		dev_err(&client->dev, "add devices failed: %d\n", ret);
		goto err_add_devs;
	}

	tps6591x_gpio_init(tps6591x, pdata);

	tps6591x_debuginit(tps6591x);

	tps6591x_sleepinit(tps6591x, pdata);

	if (pdata->use_power_off && !pm_power_off)
		pm_power_off = tps6591x_power_off;

	tps6591x_i2c_client = client;

/* FUJITSU TEN:2012-12-05 init sysfs add. start */
	ret = create_pmu_class();
    if(ret) {
        TPS6591X_ERR_LOG("Failed to create sysfs file.\n");
        DRC_ERR_REC(REGION_TRACE_ERROR, LV_ERR, ID_CREATE_FILE_ERROR, ID_TPS6591X, __LINE__, TYPE_CLASS_FAILED, TYPE_NOT_USE);
        goto err_create_class;
    }
    TPS6591X_NOTICE_LOG("success create sysfs files.\n");
/* FUJITSU TEN:2012-12-05 init sysfs add. end */
/* FUJITSU TEN:2012-12-05 log add. start */
	TPS6591X_DEBUG_LOG("end.\n");
/* FUJITSU TEN:2012-12-05 log add. end */
	return 0;

/* FUJITSU TEN:2012-12-05 err add. start */
err_create_class:
/* FUJITSU TEN:2012-12-05 err add. end */
err_add_devs:
	if (client->irq)
		free_irq(client->irq, tps6591x);
err_irq_init:
	kfree(tps6591x);
	return ret;
}

static int __devexit tps6591x_i2c_remove(struct i2c_client *client)
{
	struct tps6591x *tps6591x = i2c_get_clientdata(client);

	if (client->irq)
		free_irq(client->irq, tps6591x);

	if (gpiochip_remove(&tps6591x->gpio) < 0)
		dev_err(&client->dev, "Error in removing the gpio driver\n");

	kfree(tps6591x);
	return 0;
}
#ifdef CONFIG_PM
static int tps6591x_i2c_suspend(struct i2c_client *client, pm_message_t state)
{
	struct device *dev = &(client->dev);
	struct tps6591x_platform_data *pdata = client->dev.platform_data;
	int ret = 0;

	if (pdata->dev_slp_delayed) {
		ret = tps6591x_set_bits(dev, TPS6591X_DEVCTRL, DEVCTRL_DEV_SLP);
		if (ret) {
			dev_err(&client->dev, "Error in setting PMU's sleep bit; bailing out\n");
			return ret;
		}
	}

	if (client->irq)
		disable_irq(client->irq);
	return ret;
}

static int tps6591x_i2c_resume(struct i2c_client *client)
{
	struct device *dev = &(client->dev);
	struct tps6591x_platform_data *pdata = client->dev.platform_data;
	int ret = 0;

	if (client->irq)
		enable_irq(client->irq);
	if (pdata->dev_slp_delayed) {
		ret = tps6591x_clr_bits(dev, TPS6591X_DEVCTRL, DEVCTRL_DEV_SLP);
		if (ret)
			dev_err(&client->dev, "Error in Clearing PMU's sleep bit\n");
	}

	return ret;
}
#endif


static const struct i2c_device_id tps6591x_id_table[] = {
	{ "tps6591x", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, tps6591x_id_table);

static struct i2c_driver tps6591x_driver = {
	.driver	= {
		.name	= "tps6591x",
		.owner	= THIS_MODULE,
	},
	.probe		= tps6591x_i2c_probe,
	.remove		= __devexit_p(tps6591x_i2c_remove),
#ifdef CONFIG_PM
	.suspend	= tps6591x_i2c_suspend,
	.resume		= tps6591x_i2c_resume,
#endif
	.id_table	= tps6591x_id_table,
};

static int __init tps6591x_init(void)
{
	return i2c_add_driver(&tps6591x_driver);
}
subsys_initcall(tps6591x_init);

static void __exit tps6591x_exit(void)
{
	i2c_del_driver(&tps6591x_driver);
}
module_exit(tps6591x_exit);

MODULE_DESCRIPTION("TPS6591X core driver");
MODULE_LICENSE("GPL");
