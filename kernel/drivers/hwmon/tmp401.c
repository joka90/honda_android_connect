/* FUJITSU TEN:2013-01-11 doxygen comment add. start */
/**
 * @file tmp401.c
 */
/* FUJITSU TEN:2013-01-11 doxygen comment add. end */
/* tmp401.c
 *
 * Copyright (C) 2007,2008 Hans de Goede <hdegoede@redhat.com>
 * Preliminary tmp411 support by:
 * Gabriel Konat, Sander Leget, Wouter Willems
 * Copyright (C) 2009 Andre Prendel <andre.prendel@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Driver for the Texas Instruments TMP401 SMBUS temperature sensor IC.
 *
 * Note this IC is in some aspect similar to the LM90, but it has quite a
 * few differences too, for example the local temp has a higher resolution
 * and thus has 16 bits registers for its value and limit instead of 8 bits.
 */
/*
 *----------------------------------------------------------------------------
 * COPYRIGHT(C) FUJITSU TEN LIMITED 2012-2013
 *----------------------------------------------------------------------------
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
/* FUJITSU TEN:2012-11-09 fastboot header add. start */
#include <linux/earlydevice.h>
/* FUJITSU TEN:2012-11-09 fastboot header add. end */
/* FUJITSU TEN:2012-12-07 nonvolatile driver access add. start */
#include <linux/nonvolatile.h>
/* FUJITSU TEN:2012-12-07 nonvolatile driver access add. end */

/* Addresses to scan */
static const unsigned short normal_i2c[] = { 0x4c, I2C_CLIENT_END };

enum chips { tmp401, tmp411 };

/*
 * The TMP401 registers, note some registers have different addresses for
 * reading and writing
 */
#define TMP401_STATUS				0x02
#define TMP401_CONFIG_READ			0x03
#define TMP401_CONFIG_WRITE			0x09
#define TMP401_CONVERSION_RATE_READ		0x04
#define TMP401_CONVERSION_RATE_WRITE		0x0A
#define TMP401_TEMP_CRIT_HYST			0x21
#define TMP401_CONSECUTIVE_ALERT		0x22
#define TMP401_MANUFACTURER_ID_REG		0xFE
#define TMP401_DEVICE_ID_REG			0xFF
#define TMP411_N_FACTOR_REG			0x18

static const u8 TMP401_TEMP_MSB[2]			= { 0x00, 0x01 };
static const u8 TMP401_TEMP_LSB[2]			= { 0x15, 0x10 };
static const u8 TMP401_TEMP_LOW_LIMIT_MSB_READ[2]	= { 0x06, 0x08 };
static const u8 TMP401_TEMP_LOW_LIMIT_MSB_WRITE[2]	= { 0x0C, 0x0E };
static const u8 TMP401_TEMP_LOW_LIMIT_LSB[2]		= { 0x17, 0x14 };
static const u8 TMP401_TEMP_HIGH_LIMIT_MSB_READ[2]	= { 0x05, 0x07 };
static const u8 TMP401_TEMP_HIGH_LIMIT_MSB_WRITE[2]	= { 0x0B, 0x0D };
static const u8 TMP401_TEMP_HIGH_LIMIT_LSB[2]		= { 0x16, 0x13 };
/* These are called the THERM limit / hysteresis / mask in the datasheet */
static const u8 TMP401_TEMP_CRIT_LIMIT[2]		= { 0x20, 0x19 };

static const u8 TMP411_TEMP_LOWEST_MSB[2]		= { 0x30, 0x34 };
static const u8 TMP411_TEMP_LOWEST_LSB[2]		= { 0x31, 0x35 };
static const u8 TMP411_TEMP_HIGHEST_MSB[2]		= { 0x32, 0x36 };
static const u8 TMP411_TEMP_HIGHEST_LSB[2]		= { 0x33, 0x37 };

/* Flags */
#define TMP401_CONFIG_RANGE		0x04
#define TMP401_CONFIG_SHUTDOWN		0x40
#define TMP401_STATUS_LOCAL_CRIT		0x01
#define TMP401_STATUS_REMOTE_CRIT		0x02
#define TMP401_STATUS_REMOTE_OPEN		0x04
#define TMP401_STATUS_REMOTE_LOW		0x08
#define TMP401_STATUS_REMOTE_HIGH		0x10
#define TMP401_STATUS_LOCAL_LOW		0x20
#define TMP401_STATUS_LOCAL_HIGH		0x40

/* Manufacturer / Device ID's */
#define TMP401_MANUFACTURER_ID			0x55
#define TMP401_DEVICE_ID			0x11
#define TMP411_DEVICE_ID			0x12

/* FUJITSU TEN:2012-11-30 conversion rate register value add. start */
#define TMP401_CONVERSION_RATE_VALUE		0x03
#define TMP401_CONVERSION_RATE_VALUE_MAX	0x0f
#define TMP401_FLAG_ON				0x01
#define TMP401_FLAG_OFF				0x00
#define TMP401_REMOTE_THERM_LIMIT_VALUE		0x6E
/* FUJITSU TEN:2012-11-30 conversion rate register value add. end */

/*
 * Driver data (common to all clients)
 */

static const struct i2c_device_id tmp401_id[] = {
	{ "tmp401", tmp401 },
	{ "tmp411", tmp411 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tmp401_id);

/*
 * Client data (each client gets its own)
 */

struct tmp401_data {
	struct device *hwmon_dev;
	struct mutex update_lock;
	char valid; /* zero until following fields are valid */
	unsigned long last_updated; /* in jiffies */
	enum chips kind;

	/* register values */
	u8 status;
	u8 config;
	u16 temp[2];
	u16 temp_low[2];
	u16 temp_high[2];
	u8 temp_crit[2];
	u8 temp_crit_hyst;
	u16 temp_lowest[2];
	u16 temp_highest[2];
/* FUJITSU TEN:2012-11-09 variable add. start */
	int error;
	struct early_device edev;
/* FUJITSU TEN:2012-11-09 variable add. end */
/* FUJITSU TEN:2012-12-07 variable add. start */
	u8 conversion_rate;
/* FUJITSU TEN:2012-12-07 variable add. end */
};

/* FUJITSU TEN:2012-11-09 global variable add. start */
unsigned char		tmp401_stop_flag;
/* FUJITSU TEN:2012-11-09 global variable add. end */

/* FUJITSU TEN:2012-11-09 i2c access macro add. start */
#define TMP401_I2C_READ( value, client, address ) { \
	if( tmp401_stop_flag == TMP401_FLAG_ON ) { \
		value = -ESHUTDOWN; \
		goto ERR; \
	} \
	value = i2c_smbus_read_byte_data(client, address); \
	if( value < 0 ) { \
		TMP401_ERR_LOG( "i2c_smbus_read_byte_data error\n" ); \
		value = -EIO; \
		goto ERR; \
	} \
}

#define TMP401_I2C_WRITE( return_value, client, address, value ) { \
	if( tmp401_stop_flag == TMP401_FLAG_ON ) { \
		return_value = -ESHUTDOWN; \
		goto ERR; \
	} \
	return_value = i2c_smbus_write_byte_data(client, address, value ); \
	if( return_value < 0 ) { \
		TMP401_ERR_LOG( "i2c_smbus_write_byte_data error\n" ); \
		return_value = -EIO; \
		goto ERR; \
	} \
}
/* FUJITSU TEN:2012-11-09 i2c access macro add. end */

/* FUJITSU TEN:2012-11-09 logmacro add. start */
#define TMP401_EMERG_LOG( format, ...) { \
	printk( KERN_EMERG "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define TMP401_ERR_LOG( format, ...) { \
	printk( KERN_ERR "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define TMP401_WARNING_LOG( format, ...) { \
	printk( KERN_WARNING "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#if defined(CONFIG_SENSORS_TMP401_LOG_NOTICE) || defined(CONFIG_SENSORS_TMP401_LOG_INFO) \
	|| defined(CONFIG_SENSORS_TMP401_LOG_DEBUG)
#define TMP401_NOTICE_LOG( format, ...) { \
	printk( KERN_NOTICE "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define TMP401_NOTICE_LOG( format, ... )
#endif

#if defined(CONFIG_SENSORS_TMP401_LOG_INFO) || defined(CONFIG_SENSORS_TMP401_LOG_DEBUG)
#define TMP401_INFO_LOG( format, ...) { \
	printk( KERN_INFO "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define TMP401_INFO_LOG( format, ... )
#endif

#ifdef CONFIG_SENSORS_TMP401_LOG_DEBUG 
#define TMP401_DEBUG_LOG( format, ...) { \
	printk( KERN_DEBUG "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define TMP401_DEBUG_LOG( format, ... )
#endif
/* FUJITSU TEN:2012-11-09 logmacro add. end */

/*
 * Sysfs attr show / store functions
 */

/* FUJITSU TEN:2013-01-11 doxygen comment add. start */
/**
 * @brief         Read data is changed into thermal data.
 * @param[in]     reg Read data from the device.
 * @param[in]     config Read config data from the device.
 * @return        Changed thermal data.
 */
/* FUJITSU TEN:2013-01-11 doxygen comment add. end */
static int tmp401_register_to_temp(u16 reg, u8 config)
{
	int temp = reg;

	if (config & TMP401_CONFIG_RANGE)
		temp -= 64 * 256;

	return (temp * 625 + 80) / 160;
}

static u16 tmp401_temp_to_register(long temp, u8 config)
{
	if (config & TMP401_CONFIG_RANGE) {
		temp = SENSORS_LIMIT(temp, -64000, 191000);
		temp += 64000;
	} else
		temp = SENSORS_LIMIT(temp, 0, 127000);

	return (temp * 160 + 312) / 625;
}

static int tmp401_crit_register_to_temp(u8 reg, u8 config)
{
	int temp = reg;

	if (config & TMP401_CONFIG_RANGE)
		temp -= 64;

	return temp * 1000;
}

static u8 tmp401_crit_temp_to_register(long temp, u8 config)
{
	if (config & TMP401_CONFIG_RANGE) {
		temp = SENSORS_LIMIT(temp, -64000, 191000);
		temp += 64000;
	} else
		temp = SENSORS_LIMIT(temp, 0, 127000);

	return (temp + 500) / 1000;
}

/* FUJITSU TEN:2013-01-11 doxygen comment add. start */
/**
 * @brief         Get 16bit register data for thermal sensor device.
 * @param[in]     *client Thermal sensor driver use I2C information.
 * @param[in,out] *data The structure which stored read data.
 * @return        The structure which further stored read data.
 */
/* FUJITSU TEN:2013-01-11 doxygen comment add. end */
static struct tmp401_data *tmp401_update_device_reg16(
	struct i2c_client *client, struct tmp401_data *data)
{
/* FUJITSU TEN:2012-11-09 comment out & fastboot edit. start */
#if 0
	int i;

	for (i = 0; i < 2; i++) {
		/*
		 * High byte must be read first immediately followed
		 * by the low byte
		 */
		data->temp[i] = i2c_smbus_read_byte_data(client,
			TMP401_TEMP_MSB[i]) << 8;
		data->temp[i] |= i2c_smbus_read_byte_data(client,
			TMP401_TEMP_LSB[i]);
		data->temp_low[i] = i2c_smbus_read_byte_data(client,
			TMP401_TEMP_LOW_LIMIT_MSB_READ[i]) << 8;
		data->temp_low[i] |= i2c_smbus_read_byte_data(client,
			TMP401_TEMP_LOW_LIMIT_LSB[i]);
		data->temp_high[i] = i2c_smbus_read_byte_data(client,
			TMP401_TEMP_HIGH_LIMIT_MSB_READ[i]) << 8;
		data->temp_high[i] |= i2c_smbus_read_byte_data(client,
			TMP401_TEMP_HIGH_LIMIT_LSB[i]);
		data->temp_crit[i] = i2c_smbus_read_byte_data(client,
			TMP401_TEMP_CRIT_LIMIT[i]);

		if (data->kind == tmp411) {
			data->temp_lowest[i] = i2c_smbus_read_byte_data(client,
				TMP411_TEMP_LOWEST_MSB[i]) << 8;
			data->temp_lowest[i] |= i2c_smbus_read_byte_data(
				client, TMP411_TEMP_LOWEST_LSB[i]);

			data->temp_highest[i] = i2c_smbus_read_byte_data(
				client, TMP411_TEMP_HIGHEST_MSB[i]) << 8;
			data->temp_highest[i] |= i2c_smbus_read_byte_data(
				client, TMP411_TEMP_HIGHEST_LSB[i]);
		}
	}
#else
	int value;

	TMP401_INFO_LOG( "start.\n" );

	data->error = 0;
	data->temp[0] = 0;
	data->temp[1] = 0;
	data->temp_low[0] = 0;
	data->temp_low[1] = 0;
	data->temp_high[0] = 0;
	data->temp_high[1] = 0;
	data->temp_crit[0] = 0;
	data->temp_crit[1] = 0;
	data->temp_lowest[0] = 0;
	data->temp_lowest[1] = 0;
	data->temp_highest[0] = 0;
	data->temp_highest[1] = 0;

	TMP401_I2C_READ( value, client, TMP401_TEMP_MSB[1] );
	data->temp[1] = value << 8;

	TMP401_I2C_READ( value, client, TMP401_TEMP_LSB[1] );
	data->temp[1] |= value;

ERR:
	if( value < 0 ) {
		data->error = value;
	}
	TMP401_DEBUG_LOG( "end.\n" );
#endif
/* FUJITSU TEN:2012-11-09 comment out & fastboot edit. end */
	return data;
}

/* FUJITSU TEN:2013-01-11 doxygen comment add. start */
/**
 * @brief         Get 8bit register data for thermal sensor device.
 * @param[in,out] *dev Device information structure.
 * @return        The structure which stored read data.
 */
/* FUJITSU TEN:2013-01-11 doxygen comment add. end */
static struct tmp401_data *tmp401_update_device(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tmp401_data *data = i2c_get_clientdata(client);
	/* FUJITSU TEN:2012-11-09 variable add. start */
	int value = 0;
	/* FUJITSU TEN:2012-11-09 variable add. end */

	/* FUJITSU TEN:2012-11-09 debug log add. start */
	TMP401_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-11-09 debug log add. end */
	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated + HZ) || !data->valid) {

/* FUJITSU TEN:2012-11-09 fastboot edit. start */
#if 0
		data->status = i2c_smbus_read_byte_data(client, TMP401_STATUS);
		data->config = i2c_smbus_read_byte_data(client,
						TMP401_CONFIG_READ);
#else
		data->error = 0;
		TMP401_I2C_READ( value, client, TMP401_STATUS );
		data->status = value;
		TMP401_I2C_READ( value, client, TMP401_CONFIG_READ );
		data->config = value;
#endif
/* FUJITSU TEN:2012-11-09 fastboot edit. end */
		tmp401_update_device_reg16(client, data);

/* FUJITSU TEN:2012-11-09 comment out. start */
#if 0
		data->temp_crit_hyst = i2c_smbus_read_byte_data(client,
						TMP401_TEMP_CRIT_HYST);
#else
		data->temp_crit_hyst = 0;
#endif
/* FUJITSU TEN:2012-11-09 comment out. end */

		data->last_updated = jiffies;
		data->valid = 1;
	}

/* FUJITSU TEN:2012-11-09 fastboot edit. start */
ERR:
	if( value < 0 ) {
		data->error = value;
	}
/* FUJITSU TEN:2012-11-09 fastboot edit. end */
	mutex_unlock(&data->update_lock);
	/* FUJITSU TEN:2012-11-09 debug log add. start */
	TMP401_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-11-09 debug log add. end */
	return data;
}

/* FUJITSU TEN:2013-01-11 doxygen comment add. start */
/**
 * @brief         Sysfs interface for temp2_input data show.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr Device attribute structure.
 * @param[out]    *buf Show gpio data.
 * @return        Size of buf.
 */
/* FUJITSU TEN:2013-01-11 doxygen comment add. end */
static ssize_t show_temp_value(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int index = to_sensor_dev_attr(devattr)->index;
	/* FUJITSU TEN:2012-11-09 fastboot edit. start */
	struct tmp401_data *data;

	TMP401_INFO_LOG( "start.\n" );

	data = tmp401_update_device(dev);

	TMP401_DEBUG_LOG( "end. error=%d\n", data->error );

	if( data->error < 0 ) {
		return data->error;
	}
	else {
		return sprintf(buf, "%d\n",
			tmp401_register_to_temp(data->temp[index], data->config));
	}

	/* FUJITSU TEN:2012-11-09 fastboot edit. end */
}

static ssize_t show_temp_min(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	return sprintf(buf, "%d\n",
		tmp401_register_to_temp(data->temp_low[index], data->config));
}

static ssize_t show_temp_max(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	return sprintf(buf, "%d\n",
		tmp401_register_to_temp(data->temp_high[index], data->config));
}

static ssize_t show_temp_crit(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	return sprintf(buf, "%d\n",
			tmp401_crit_register_to_temp(data->temp_crit[index],
							data->config));
}

static ssize_t show_temp_crit_hyst(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int temp, index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	mutex_lock(&data->update_lock);
	temp = tmp401_crit_register_to_temp(data->temp_crit[index],
						data->config);
	temp -= data->temp_crit_hyst * 1000;
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d\n", temp);
}

static ssize_t show_temp_lowest(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	return sprintf(buf, "%d\n",
		tmp401_register_to_temp(data->temp_lowest[index],
					data->config));
}

static ssize_t show_temp_highest(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	return sprintf(buf, "%d\n",
		tmp401_register_to_temp(data->temp_highest[index],
					data->config));
}

static ssize_t show_status(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int mask = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);

	if (data->status & mask)
		return sprintf(buf, "1\n");
	else
		return sprintf(buf, "0\n");
}

static ssize_t store_temp_min(struct device *dev, struct device_attribute
	*devattr, const char *buf, size_t count)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);
	long val;
	u16 reg;

	if (strict_strtol(buf, 10, &val))
		return -EINVAL;

	reg = tmp401_temp_to_register(val, data->config);

	mutex_lock(&data->update_lock);

	i2c_smbus_write_byte_data(to_i2c_client(dev),
		TMP401_TEMP_LOW_LIMIT_MSB_WRITE[index], reg >> 8);
	i2c_smbus_write_byte_data(to_i2c_client(dev),
		TMP401_TEMP_LOW_LIMIT_LSB[index], reg & 0xFF);

	data->temp_low[index] = reg;

	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t store_temp_max(struct device *dev, struct device_attribute
	*devattr, const char *buf, size_t count)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);
	long val;
	u16 reg;

	if (strict_strtol(buf, 10, &val))
		return -EINVAL;

	reg = tmp401_temp_to_register(val, data->config);

	mutex_lock(&data->update_lock);

	i2c_smbus_write_byte_data(to_i2c_client(dev),
		TMP401_TEMP_HIGH_LIMIT_MSB_WRITE[index], reg >> 8);
	i2c_smbus_write_byte_data(to_i2c_client(dev),
		TMP401_TEMP_HIGH_LIMIT_LSB[index], reg & 0xFF);

	data->temp_high[index] = reg;

	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t store_temp_crit(struct device *dev, struct device_attribute
	*devattr, const char *buf, size_t count)
{
	int index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);
	long val;
	u8 reg;

	if (strict_strtol(buf, 10, &val))
		return -EINVAL;

	reg = tmp401_crit_temp_to_register(val, data->config);

	mutex_lock(&data->update_lock);

	i2c_smbus_write_byte_data(to_i2c_client(dev),
		TMP401_TEMP_CRIT_LIMIT[index], reg);

	data->temp_crit[index] = reg;

	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t store_temp_crit_hyst(struct device *dev, struct device_attribute
	*devattr, const char *buf, size_t count)
{
	int temp, index = to_sensor_dev_attr(devattr)->index;
	struct tmp401_data *data = tmp401_update_device(dev);
	long val;
	u8 reg;

	if (strict_strtol(buf, 10, &val))
		return -EINVAL;

	if (data->config & TMP401_CONFIG_RANGE)
		val = SENSORS_LIMIT(val, -64000, 191000);
	else
		val = SENSORS_LIMIT(val, 0, 127000);

	mutex_lock(&data->update_lock);
	temp = tmp401_crit_register_to_temp(data->temp_crit[index],
						data->config);
	val = SENSORS_LIMIT(val, temp - 255000, temp);
	reg = ((temp - val) + 500) / 1000;

	i2c_smbus_write_byte_data(to_i2c_client(dev),
		TMP401_TEMP_CRIT_HYST, reg);

	data->temp_crit_hyst = reg;

	mutex_unlock(&data->update_lock);

	return count;
}

/*
 * Resets the historical measurements of minimum and maximum temperatures.
 * This is done by writing any value to any of the minimum/maximum registers
 * (0x30-0x37).
 */
static ssize_t reset_temp_history(struct device *dev,
	struct device_attribute	*devattr, const char *buf, size_t count)
{
	long val;

	if (strict_strtol(buf, 10, &val))
		return -EINVAL;

	if (val != 1) {
		dev_err(dev, "temp_reset_history value %ld not"
			" supported. Use 1 to reset the history!\n", val);
		return -EINVAL;
	}
	i2c_smbus_write_byte_data(to_i2c_client(dev),
		TMP411_TEMP_LOWEST_MSB[0], val);

	return count;
}


/* FUJITSU TEN:2013-01-27 error polling interface add. start */
/**
 * @brief         Sysfs interface for error polling status data show.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr Device attribute structure.
 * @param[out]    *buf Show status data.
 * @return        Size of buf.
 */
static ssize_t show_error_status(struct device *dev,
	struct device_attribute *devattr, char *buf)
{
	int value;
	struct i2c_client *client = to_i2c_client(dev);
	struct tmp401_data *data = i2c_get_clientdata(client);

	TMP401_INFO_LOG( "start.\n" );

	mutex_lock(&data->update_lock);

	TMP401_I2C_READ( value, client, TMP401_CONVERSION_RATE_READ );
	TMP401_DEBUG_LOG( "Conversion rate value 0x%02x : read 0x%02x\n",
		TMP401_CONVERSION_RATE_VALUE, value );
	if( value != TMP401_CONVERSION_RATE_VALUE ) {
		TMP401_ERR_LOG( "Conversion rate data error. %02x\n",
			value );
		value = -EBADMSG;
		goto ERR;
	}

	TMP401_I2C_READ( value, client, TMP401_TEMP_CRIT_LIMIT[1] );
	TMP401_DEBUG_LOG( "Remote /THERM Limit value 0x%02x : read 0x%02x\n",
		TMP401_REMOTE_THERM_LIMIT_VALUE, value );
	if( value != TMP401_REMOTE_THERM_LIMIT_VALUE ) {
		TMP401_ERR_LOG( "Remote /THERM Limit data error. %02x\n",
			value );
		value = -EBADMSG;
		goto ERR;
	}

	value = 0;
ERR:
	mutex_unlock(&data->update_lock);

	TMP401_DEBUG_LOG( "end. error=%d\n", value );

	if( value < 0 ) {
		return value;
	}
	else {
		return sprintf(buf, "0\n");
	}
}
/* FUJITSU TEN:2012-12-27 error polling interface add. end */

static struct sensor_device_attribute tmp401_attr[] = {
	SENSOR_ATTR(temp1_input, S_IRUGO, show_temp_value, NULL, 0),
	SENSOR_ATTR(temp1_min, S_IWUSR | S_IRUGO, show_temp_min,
		    store_temp_min, 0),
	SENSOR_ATTR(temp1_max, S_IWUSR | S_IRUGO, show_temp_max,
		    store_temp_max, 0),
	SENSOR_ATTR(temp1_crit, S_IWUSR | S_IRUGO, show_temp_crit,
		    store_temp_crit, 0),
	SENSOR_ATTR(temp1_crit_hyst, S_IWUSR | S_IRUGO, show_temp_crit_hyst,
		    store_temp_crit_hyst, 0),
	SENSOR_ATTR(temp1_min_alarm, S_IRUGO, show_status, NULL,
		    TMP401_STATUS_LOCAL_LOW),
	SENSOR_ATTR(temp1_max_alarm, S_IRUGO, show_status, NULL,
		    TMP401_STATUS_LOCAL_HIGH),
	SENSOR_ATTR(temp1_crit_alarm, S_IRUGO, show_status, NULL,
		    TMP401_STATUS_LOCAL_CRIT),
	SENSOR_ATTR(temp2_input, S_IRUGO, show_temp_value, NULL, 1),
	SENSOR_ATTR(temp2_min, S_IWUSR | S_IRUGO, show_temp_min,
		    store_temp_min, 1),
	SENSOR_ATTR(temp2_max, S_IWUSR | S_IRUGO, show_temp_max,
		    store_temp_max, 1),
	SENSOR_ATTR(temp2_crit, S_IWUSR | S_IRUGO, show_temp_crit,
		    store_temp_crit, 1),
	SENSOR_ATTR(temp2_crit_hyst, S_IRUGO, show_temp_crit_hyst, NULL, 1),
	SENSOR_ATTR(temp2_fault, S_IRUGO, show_status, NULL,
		    TMP401_STATUS_REMOTE_OPEN),
	SENSOR_ATTR(temp2_min_alarm, S_IRUGO, show_status, NULL,
		    TMP401_STATUS_REMOTE_LOW),
	SENSOR_ATTR(temp2_max_alarm, S_IRUGO, show_status, NULL,
		    TMP401_STATUS_REMOTE_HIGH),
	SENSOR_ATTR(temp2_crit_alarm, S_IRUGO, show_status, NULL,
		    TMP401_STATUS_REMOTE_CRIT),
	/* FUJITSU TEN:2012-12-27 error polling interface add. start */
	SENSOR_ATTR(status, S_IRUGO, show_error_status, NULL, 0 ),
	/* FUJITSU TEN:2012-12-27 error polling interface add. end */

};

/*
 * Additional features of the TMP411 chip.
 * The TMP411 stores the minimum and maximum
 * temperature measured since power-on, chip-reset, or
 * minimum and maximum register reset for both the local
 * and remote channels.
 */
static struct sensor_device_attribute tmp411_attr[] = {
	SENSOR_ATTR(temp1_highest, S_IRUGO, show_temp_highest, NULL, 0),
	SENSOR_ATTR(temp1_lowest, S_IRUGO, show_temp_lowest, NULL, 0),
	SENSOR_ATTR(temp2_highest, S_IRUGO, show_temp_highest, NULL, 1),
	SENSOR_ATTR(temp2_lowest, S_IRUGO, show_temp_lowest, NULL, 1),
	SENSOR_ATTR(temp_reset_history, S_IWUSR, NULL, reset_temp_history, 0),
};

/*
 * Begin non sysfs callback code (aka Real code)
 */

/* FUJITSU TEN:2012-12-07 read parameter for eMMC add. start */
/* FUJITSU TEN:2013-01-11 doxygen comment add. start */
/**
 * @brief         Read use parameter data for eMMC.
 * @param[in,out] *data The structure which stored read data.
 * @return        None.
 */
/* FUJITSU TEN:2013-01-11 doxygen comment add. end */
static void tmp401_read_parameter(struct tmp401_data *data)
{
	int error;

	TMP401_INFO_LOG( "start.\n" );

	error = GetNONVOLATILE( ( unsigned char* )&data->conversion_rate,
		FTEN_NONVOL_TMP401_CONVERSION_RATE, sizeof( u8 ) );
	if (error) {
		TMP401_ERR_LOG( "GetNONVOLATILE error : Conversion rate value.\n" );
		data->conversion_rate = TMP401_CONVERSION_RATE_VALUE;
	}
	else if (data->conversion_rate > TMP401_CONVERSION_RATE_VALUE_MAX) {
		TMP401_ERR_LOG( "eMMC Conversion rate error. value:0x%02x\n",
			data->conversion_rate );
		data->conversion_rate = TMP401_CONVERSION_RATE_VALUE;
	}
	TMP401_DEBUG_LOG( "Conversion rate set value : 0x%02x\n",
		data->conversion_rate );

	TMP401_DEBUG_LOG( "end.\n" );

	return;
}
/* FUJITSU TEN:2012-12-07 read parameter for eMMC add. end */

/* FUJITSU TEN:2013-01-11 doxygen comment add. start */
/**
 * @brief         Initialize thermal sensor device.
 * @param[in]     *client Thermal sensor driver use I2C information.
 * @return        None.
 */
/* FUJITSU TEN:2013-01-11 doxygen comment add. end */
static void tmp401_init_client(struct i2c_client *client)
{
	/* FUJITSU TEN:2012-11-27 BU-DET flag function add. start */
	int config = 0, config_orig = 0;
	int ret = 0;
	struct tmp401_data *data = i2c_get_clientdata(client);
	/* FUJITSU TEN:2012-11-27 BU-DET flag function add. end */

	/* Set the conversion rate to 2 Hz */
	/* FUJITSU TEN:2012-11-27 BU-DET flag function add. start */
	TMP401_I2C_WRITE( ret, client, TMP401_CONVERSION_RATE_WRITE,
		data->conversion_rate );
	/* FUJITSU TEN:2012-11-27 BU-DET flag function add. end */

	/* Start conversions (disable shutdown if necessary) */
	/* FUJITSU TEN:2012-11-27 BU-DET flag function add. start */
	TMP401_I2C_READ( config, client, TMP401_CONFIG_READ );
	/* FUJITSU TEN:2012-11-27 BU-DET flag function add. end */

	config_orig = config;
	config &= ~TMP401_CONFIG_SHUTDOWN;

	if (config != config_orig)
/* FUJITSU TEN:2012-11-27 BU-DET flag function add. start */
		TMP401_I2C_WRITE( ret, client, TMP401_CONFIG_WRITE, config);

	return;
ERR:
	TMP401_ERR_LOG( "Initialization failed!\n");
	return;
/* FUJITSU TEN:2012-11-27 BU-DET flag function add. end */
}

/* FUJITSU TEN:2012-11-27 BU-DET flag function add. start */
/* FUJITSU TEN:2013-01-11 doxygen comment add. start */
/**
 * @brief         Fastboot flag down function.
 * @param[in]     flag Fastboot flag of system global variable.
 * @param[in]     *param Not use.
 * @return        Result.
 * @retval        0 Normal end.
 */
/* FUJITSU TEN:2013-01-11 doxygen comment add. end */
static int tmp401_fastboot_flag_down(bool flag, void* param)
{
	if ( flag == false ) {
		tmp401_stop_flag = TMP401_FLAG_OFF;
	}
	return 0;
}
/* FUJITSU TEN:2012-11-27 BU-DET flag function add. start */

/* FUJITSU TEN:2012-11-09 fastboot handler add. start */
/* FUJITSU TEN:2013-01-11 doxygen comment add. start */
/**
 * @brief         Sudden device poweroff hander function.
 * @param[in]     *dev Device information structure.
 * @return        None.
 */
/* FUJITSU TEN:2013-01-11 doxygen comment add. end */
static void tmp401_sudden_device_poweroff(struct device *dev)
{
	TMP401_DEBUG_LOG( "start. \n" );

	tmp401_stop_flag = TMP401_FLAG_ON;

	TMP401_DEBUG_LOG( "end. \n" );
}

/* FUJITSU TEN:2013-01-11 doxygen comment add. start */
/**
 * @brief         Reinit hander function.
 * @param[in]     *dev Device information structure.
 * @return        Result.
 * @retval        0 Normal end.
 */
/* FUJITSU TEN:2013-01-11 doxygen comment add. end */
static int tmp401_reinit(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tmp401_data *data = i2c_get_clientdata(client);

	TMP401_NOTICE_LOG( "start. \n" );

	early_device_invoke_with_flag_irqsave(EARLY_DEVICE_BUDET_INTERRUPTION_MASKED,
		tmp401_fastboot_flag_down, data );

	if( tmp401_stop_flag == TMP401_FLAG_ON ) {
		TMP401_NOTICE_LOG( "reinit called in BU-DET\n" );
		return 0;
	}

	mutex_lock(&data->update_lock);

	tmp401_init_client(client);

	mutex_unlock(&data->update_lock);

	TMP401_NOTICE_LOG( "end. \n" );
	return 0;
}
/* FUJITSU TEN:2012-11-09 fastboot handler add. end */

static int tmp401_detect(struct i2c_client *client,
			 struct i2c_board_info *info)
{
	enum chips kind;
	struct i2c_adapter *adapter = client->adapter;
	u8 reg;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	/* Detect and identify the chip */
	reg = i2c_smbus_read_byte_data(client, TMP401_MANUFACTURER_ID_REG);
	if (reg != TMP401_MANUFACTURER_ID)
		return -ENODEV;

	reg = i2c_smbus_read_byte_data(client, TMP401_DEVICE_ID_REG);

	switch (reg) {
	case TMP401_DEVICE_ID:
		kind = tmp401;
		break;
	case TMP411_DEVICE_ID:
		kind = tmp411;
		break;
	default:
		return -ENODEV;
	}

	reg = i2c_smbus_read_byte_data(client, TMP401_CONFIG_READ);
	if (reg & 0x1b)
		return -ENODEV;

	reg = i2c_smbus_read_byte_data(client, TMP401_CONVERSION_RATE_READ);
	/* Datasheet says: 0x1-0x6 */
	if (reg > 15)
		return -ENODEV;

	strlcpy(info->type, tmp401_id[kind].name, I2C_NAME_SIZE);

	return 0;
}

/* FUJITSU TEN:2013-01-11 doxygen comment add. start */
/**
 * @brief         Thermal sensor driver remove function.
 * @param[in]     *client Thermal sensor driver use I2C information.
 * @return        Result.
 * @retval        0 Normal end.
 */
/* FUJITSU TEN:2013-01-11 doxygen comment add. end */
static int tmp401_remove(struct i2c_client *client)
{
	struct tmp401_data *data = i2c_get_clientdata(client);
	int i;

	/* FUJITSU TEN:2012-11-09 debug log add. start */
	TMP401_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-11-09 debug log add. end */

	/* FUJITSU TEN:2012-11-09 fastboot add. start */
	if (data->edev.dev)
		early_device_unregister( &data->edev );
	/* FUJITSU TEN:2012-11-09 fastboot add. end */

	if (data->hwmon_dev)
		hwmon_device_unregister(data->hwmon_dev);

	for (i = 0; i < ARRAY_SIZE(tmp401_attr); i++)
		device_remove_file(&client->dev, &tmp401_attr[i].dev_attr);

	if (data->kind == tmp411) {
		for (i = 0; i < ARRAY_SIZE(tmp411_attr); i++)
			device_remove_file(&client->dev,
					   &tmp411_attr[i].dev_attr);
	}

	kfree(data);
	/* FUJITSU TEN:2012-11-09 debug log add. start */
	TMP401_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-11-09 debug log add. end */
	return 0;
}

/* FUJITSU TEN:2013-01-11 doxygen comment add. start */
/**
 * @brief         Thermal sensor driver probe function.
 * @param[in]     *client Thermal sensor driver use I2C information.
 * @param[in]     *i2c_device_id Not use.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -ENOMEM No memory.
 * @retval        -ENODEV Device not found.
 */
/* FUJITSU TEN:2013-01-11 doxygen comment add. end */
static int tmp401_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int i, err = 0;
	struct tmp401_data *data;
	const char *names[] = { "TMP401", "TMP411" };
	/* FUJITSU TEN:2012-11-13 variable add. start */
	u8 reg;
	enum chips kind;
	/* FUJITSU TEN:2012-11-13 variable add. end */

	/* FUJITSU TEN:2012-11-09 debug log add. start */
	TMP401_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-11-09 debug log add. end */

	data = kzalloc(sizeof(struct tmp401_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);
/* FUJITSU TEN:2012-11-14 comment out. start */
#if 0
	data->kind = id->driver_data;
#endif
/* FUJITSU TEN:2012-11-14 comment out. end */

	/* FUJITSU TEN:2012-11-19 read parameter for eMMC add. start */
	tmp401_read_parameter(data);
	/* FUJITSU TEN:2012-11-19 read parameter for eMMC add. end */

	/* Initialize the TMP401 chip */
	tmp401_init_client(client);

	/* FUJITSU TEN:2012-11-14 move detect setting. start */
	reg = i2c_smbus_read_byte_data(client, TMP401_DEVICE_ID_REG);

	switch (reg) {
	case TMP401_DEVICE_ID:
		kind = tmp401;
		break;
	case TMP411_DEVICE_ID:
		kind = tmp411;
		break;
	default:
		TMP401_ERR_LOG( "Failed to device id data read.\n" );
		err = -ENODEV;
		goto exit_remove;
	}
	data->kind = kind;
	/* FUJITSU TEN:2012-11-14 move detect setting. end */

	/* Register sysfs hooks */
	for (i = 0; i < ARRAY_SIZE(tmp401_attr); i++) {
		err = device_create_file(&client->dev,
					 &tmp401_attr[i].dev_attr);
		if (err)
			goto exit_remove;
	}

	/* Register aditional tmp411 sysfs hooks */
	if (data->kind == tmp411) {
		for (i = 0; i < ARRAY_SIZE(tmp411_attr); i++) {
			err = device_create_file(&client->dev,
						 &tmp411_attr[i].dev_attr);
			if (err)
				goto exit_remove;
		}
	}

	data->hwmon_dev = hwmon_device_register(&client->dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		data->hwmon_dev = NULL;
		goto exit_remove;
	}

	/* FUJITSU TEN:2012-11-09 fastboot handler add. start */
	data->edev.sudden_device_poweroff = tmp401_sudden_device_poweroff;
	data->edev.reinit = tmp401_reinit;
	data->edev.dev = &client->dev;
	err = early_device_register( &data->edev );
	if( err ) {
		TMP401_ERR_LOG( "Failed to early device register\n" );
		data->edev.dev = NULL;
		goto exit_remove;
	}
	/* FUJITSU TEN:2012-11-09 fastboot handler add. end */

	/* FUJITSU TEN:2012-12-17 log edit for notice. start */
	TMP401_NOTICE_LOG( "TI %s chip initialize finished.\n", names[data->kind]);
	/* FUJITSU TEN:2012-12-17 log edit for notice. end */

	/* FUJITSU TEN:2012-11-09 debug log add. start */
	TMP401_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-11-09 debug log add. end */

	return 0;

exit_remove:
	tmp401_remove(client); /* will also free data for us */

	/* FUJITSU TEN:2012-11-09 debug log add. start */
	TMP401_DEBUG_LOG( "end. %d\n", err );
	/* FUJITSU TEN:2012-11-09 debug log add. end */

	return err;
}

static struct i2c_driver tmp401_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "tmp401",
	},
	.probe		= tmp401_probe,
	.remove		= tmp401_remove,
	.id_table	= tmp401_id,
/* FUJITSU TEN:2012-11-09 debug log add. start */
#if 0
	.detect		= tmp401_detect,
#endif
/* FUJITSU TEN:2012-11-09 debug log add. end */
	.address_list	= normal_i2c,
};

/* FUJITSU TEN:2013-01-11 doxygen comment add. start */
/**
 * @brief         Thermal sensor driver initialize function.
 * @param         None.
 * @return        i2c_add_driver result data.
 */
/* FUJITSU TEN:2013-01-11 doxygen comment add. end */
static int __init tmp401_init(void)
{
	return i2c_add_driver(&tmp401_driver);
}

/* FUJITSU TEN:2013-01-11 doxygen comment add. start */
/**
 * @brief         Thermal sensor driver exit function.
 * @param         None.
 * @return        None.
 */
/* FUJITSU TEN:2013-01-11 doxygen comment add. end */
static void __exit tmp401_exit(void)
{
	i2c_del_driver(&tmp401_driver);
}

MODULE_AUTHOR("Hans de Goede <hdegoede@redhat.com>");
MODULE_DESCRIPTION("Texas Instruments TMP401 temperature sensor driver");
MODULE_LICENSE("GPL");

module_init(tmp401_init);
module_exit(tmp401_exit);
