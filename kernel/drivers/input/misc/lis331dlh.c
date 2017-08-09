/*
 * drivers/input/misc/lis331dlh.c
 *
 * Copyright (c) 2012, NVIDIA Corporation.
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
 */
/*
 *----------------------------------------------------------------------------
 * COPYRIGHT(C) FUJITSU TEN LIMITED 2012
 *----------------------------------------------------------------------------
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <asm/div64.h>
#include <linux/delay.h>
#include <linux/earlydevice.h>

#include "lis331dlh_priv.h"

#ifdef CONFIG_LIS331DLH_LOG_DEBUG
static int  lis331dlh_debug_count = -1;
#endif

/** 
 * @brief     tegra_i2c_transfer - execute a single or combined I2C message
 * @param[in] adap  : Handle to I2C bus
 * @param[in] msgs  : One or more messages to execute before STOP is issued to
 *                    terminate the operation; each message begins with a START.
 * @param[in] num   : Number of messages to be executed.
 *
 * @retval    >=0   : the number of messages executed.
 * @retval    <0    : error
 * @note      Returns negative errno, else the number of messages executed.
 */
int tegra_i2c_transfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    return i2c_transfer(adap, msgs, num);
}

/** 
 * @brief     I2C read block
 * @param[in] client : I2C client handle pointer
 * @param[in] saddr  : I2C slave address
 * @param[in] reg    : Register data (8bits sub address)
 * @param[in] len    : Read block length
 * @param[out] data  : The pointer which stores the read data
 * @param[in] retrycnt : A specification of the count of the maximum retry
 *
 * @retval    0     : ok
 * @retval    -EIO  : error
 * @note      Returns negative errno, else the number of messages executed.
 */
int tegra_i2c_read_blk_transfer(struct i2c_client *client,
               unsigned char saddr, unsigned char reg,
               unsigned int  len, unsigned char *data, unsigned char retrycnt)
{
    struct i2c_msg msgs[2];
    int ret = 0;
    int err = 0;
    unsigned char cnt = 0;

    if (retrycnt == 0) {
        LIS331DLH_NOTICE_LOG("retrycnt is specifying one or more\n");
        err = -EPERM;
        goto err_retrycnt;
    }

    msgs[0].addr  = saddr;
    msgs[0].flags = 0;          /* reg write */
    msgs[0].buf   = &reg;
    msgs[0].len   = 1;

    msgs[1].addr  = saddr;
    msgs[1].flags = I2C_M_RD;   /* read (Slave to Master)*/
    msgs[1].buf   = data;
    msgs[1].len   = len;

    for (cnt = retrycnt; cnt > 0; cnt--) {
        ret = tegra_i2c_transfer(client->adapter, msgs, 2);
#ifdef LIS331DLH_TEST_TEGRA_I2C_TRANSFER_FAILED
        ret = 0;
#endif
        if ((ret >= 0) && (ret < 2)) {
            LIS331DLH_WARNING_LOG(
            "The returned value is mismatching.(ret=0x%04x,cnt=%d)",
            ret, cnt);
            err = -EIO;
            continue;
        } else {
            err = 0;
            break;
        }
    }

    if (err == 0) {
        return ret;
    }

err_retrycnt:
    return err;
}
EXPORT_SYMBOL(tegra_i2c_read_blk_transfer);

/** 
 * @brief     I2C read byte
 * @param[in] client : I2C client handle pointer
 * @param[in] saddr  : I2C slave address
 * @param[in] reg    : read register address (8bits sub address)
 * @param[out] data  : The pointer which stores the read data
 * @param[in] retrycnt : A specification of the count of the maximum retry
 *
 * @retval    0     : ok
 * @retval    -EIO  : error
 * @note      Returns negative errno, else the number of messages executed.
 */
int tegra_i2c_read_byte_transfer(struct i2c_client *client, unsigned char saddr, 
                unsigned char reg, char *data, unsigned char retrycnt)
{
    int ret;
    ret = tegra_i2c_read_blk_transfer(client, saddr, reg, 1, data, retrycnt);
    return 0;
}
EXPORT_SYMBOL(tegra_i2c_read_byte_transfer);

/** 
 * @brief     I2C read word
 * @param[in] client : I2C client handle pointer
 * @param[in] saddr  : I2C slave address
 * @param[in] reg    : read register address (8bits sub address)
 * @param[out] data  : The pointer which stores the read data
 * @param[in] retrycnt : A specification of the count of the maximum retry
 *
 * @retval    0     : ok
 * @retval    -EIO  : error
 * @note      Returns negative errno, else the number of messages executed.
 */
int tegra_i2c_read_word_transfer(struct i2c_client *client,
                unsigned char saddr, unsigned char reg, unsigned short int *data, 
                                                        unsigned char retrycnt)
{
    int ret;

    ret = tegra_i2c_read_blk_transfer(client, saddr, reg, 2, 
                                            (unsigned char *)data, retrycnt);
    *data = swab16(*data);
    return ret;
}
EXPORT_SYMBOL(tegra_i2c_read_word_transfer);

/** 
 * @brief     I2C write block
 * @param[in] client : I2C Client handle pointer
 * @param[in] saddr  : I2C slave address
 * @param[in] len    : Write block length
 * @param[in] data   : The pointer of a write-in data
 * @param[in] retrycnt : A specification of the count of the maximum retry
 *
 * @retval    !0    : ok
 * @retval    -EIO  : error
 * @note      Returns negative errno, else the number of messages executed.
 */
int tegra_i2c_write_blk_transfer(struct i2c_client *client,
                unsigned char saddr, unsigned int len, unsigned char const *data, 
                                                        unsigned char retrycnt)
{
    struct i2c_msg msgs[1];
    int ret = 0;
    int err = 0;
    unsigned char cnt;

    if (retrycnt == 0) {
        LIS331DLH_NOTICE_LOG("retrycnt is specifying one or more\n");
        err = -EPERM;
        goto err_retrycnt;
    }

    msgs[0].addr  = saddr;
    msgs[0].flags = 0; /* write (Master to Slave) */
    msgs[0].buf   = (unsigned char *) data;
    msgs[0].len   = len;

    for (cnt = retrycnt; cnt > 0; cnt--) {
        ret = tegra_i2c_transfer(client->adapter, msgs, 1);
        if (ret == 0) {
            err = -EIO;
            LIS331DLH_NOTICE_LOG("tegra_i2c_transfer() is retry.");
            continue;
        } else if (ret == 1) {
            err = 0;
            ret = 0;
            break;
        } else {
            LIS331DLH_NOTICE_LOG("tegra_i2c_transfer() failed.(0x%08x)\n", ret);
        }
    }

    if (err != 0) {
        return ret;
    }

err_retrycnt:
    return err;
}
EXPORT_SYMBOL(tegra_i2c_write_blk_transfer);

/** 
 * @brief     I2C write byte
 * @param[in] client : I2C Client handle pointer
 * @param[in] saddr  : I2C slave address
 * @param[in] reg    : write register address (8bits sub address)
 * @param[in] data   : Write-in byte data
 * @param[in] retrycnt : A specification of the count of the maximum retry
 *
 * @retval    0     : ok
 * @retval    -EIO  : error
 * @note      Returns negative errno, else the number of messages executed.
 */
int tegra_i2c_write_byte_transfer(struct i2c_client *client,
                unsigned char saddr, unsigned char reg, char data, 
                                                        unsigned char retrycnt)
{
    char buff[2];

    buff[0] = reg;
    buff[1] = data;
    return tegra_i2c_write_blk_transfer(client, saddr, 2, buff, retrycnt);
}
EXPORT_SYMBOL(tegra_i2c_write_byte_transfer);

/** 
 * @brief     I2C write word
 * @param[in] client : I2C Client handle pointer
 * @param[in] saddr  : I2C slave address
 * @param[in] reg    : write register address (8bits sub address)
 * @param[in] data   : Write-in word data
 * @param[in] retrycnt : A specification of the count of the maximum retry
 *
 * @retval    0     : ok
 * @retval    -EIO  : error
 * @note      Returns negative errno, else the number of messages executed.
 */
int tegra_i2c_write_word_transfer(struct i2c_client *client,
                unsigned char saddr, unsigned char reg, short int data, 
                                                        unsigned char retrycnt)
{
    char buff[3];

    LIS331DLH_DEBUG_LOG("data=0x%04x\n", data);
    buff[0] = reg;
    LIS331DLH_DEBUG_LOG("buff[0]=0x%02x\n", buff[0]);
    buff[1] = (data >> 8);
    LIS331DLH_DEBUG_LOG("buff[1]=0x%02x\n", buff[1]);
    buff[2] = data & 0xff;
    LIS331DLH_DEBUG_LOG("buff[2]=0x%02x\n", buff[2]);

    return tegra_i2c_write_blk_transfer(client, saddr, 3, buff, retrycnt);
}
EXPORT_SYMBOL(tegra_i2c_write_word_transfer);

#ifdef LIS331DLH_DEBUG_MODE
static int  lis331dlh_debug_mode = 0;
static struct i2c_client *lis331dlh_debug_client; 
static void __exit lis331dlh_exit(void);
#endif

/** 
 * @brief     I2C read
 * @param[in] client : I2C client handle pointer
 * @param[in] saddr  : I2C slave address
 * @param[in] reg    : Register data (8bits sub address)
 * @param[in] len    : Read block length
 * @param[out] data  : The pointer which stores the read data
 * @param[in] retrycnt : A specification of the count of the maximum retry
 *
 * @retval    0     : ok
 * @retval    -EIO  : error
 * @note      Returns negative errno, else the number of messages executed.
 */
int lis331dlh_read_transfer(struct i2c_client *client,
               unsigned char saddr, unsigned char reg,
               unsigned int  len, unsigned char *buf, unsigned char retrycnt)
{
    struct i2c_msg msgs[2];
    int ret = 0;
    unsigned char cnt = 0;
    struct lis331dlh_data *data;
#ifdef LIS331DLH_DEBUG_MODE
    int  debug_count = 0;
#endif
    data = i2c_get_clientdata(client);

    if (retrycnt == 0) {
        LIS331DLH_NOTICE_LOG("retrycnt is specifying one or more\n");
        ret = -EPERM;
        goto err_retrycnt;
    }

    msgs[0].addr  = saddr;
    msgs[0].flags = 0;          /* reg write */
    msgs[0].buf   = &reg;
    msgs[0].len   = 1;

    msgs[1].addr  = saddr;
    msgs[1].flags = I2C_M_RD;   /* read (Slave to Master)*/
    msgs[1].buf   = buf;
    msgs[1].len   = len;

    for (cnt = retrycnt; cnt > 0; cnt--) {
        if (data->suspended == 1)
            goto err_suspended;
        ret = tegra_i2c_transfer(client->adapter, msgs, 2);
#ifdef LIS331DLH_DEBUG_MODE
        if (lis331dlh_debug_mode == LIS331DLH_TEST_I2C_READ_TRANSFER_RETRY_FAILED)
            ret = 0;
        if (lis331dlh_debug_mode == LIS331DLH_TEST_I2C_READ_TRANSFER_RETRY_OK) {
            if (debug_count == 0) {
                ret = 0;
                debug_count++;
            }
        }
#endif
        if ((ret >= 0) && (ret < 2)) {
            LIS331DLH_WARNING_LOG(
            "The returned value is mismatching.(ret=0x%04x,cnt=%d)",
            ret, cnt);
            ret = -EIO;
            LIS331DLH_NOTICE_LOG("read_transfer() is retry.");
            continue;
        } else {
            ret = 0;
            break;
        }
    }

    return ret;

err_retrycnt:
    return ret;
err_suspended:
    return -ESHUTDOWN;
}

/** 
 * @brief     I2C write
 * @param[in] client : I2C Client handle pointer
 * @param[in] saddr  : I2C slave address
 * @param[in] len    : Write block length
 * @param[in] data   : The pointer of a write-in data
 * @param[in] retrycnt : A specification of the count of the maximum retry
 *
 * @retval    !0    : ok
 * @retval    -EIO  : error
 * @note      Returns negative errno, else the number of messages executed.
 */
int lis331dlh_write_transfer(struct i2c_client *client,
                unsigned char saddr, unsigned int len, unsigned char const *buf, 
                                                        unsigned char retrycnt)
{
    struct i2c_msg msgs[1];
    int ret = 0;
    unsigned char cnt;
    struct lis331dlh_data *data;
#ifdef LIS331DLH_DEBUG_MODE
    int  debug_count = 0;
#endif

    data = i2c_get_clientdata(client);

    if (retrycnt == 0) {
        LIS331DLH_NOTICE_LOG("retrycnt is specifying one or more\n");
        ret = -EPERM;
        goto err_retrycnt;
    }

    msgs[0].addr  = saddr;
    msgs[0].flags = 0; /* write (Master to Slave) */
    msgs[0].buf   = (unsigned char *) buf;
    msgs[0].len   = len;

    for (cnt = retrycnt; cnt > 0; cnt--) {
        if (data->suspended == 1)
            goto err_suspended;
        ret = tegra_i2c_transfer(client->adapter, msgs, 1);
#ifdef LIS331DLH_DEBUG_MODE
        if (lis331dlh_debug_mode == LIS331DLH_TEST_I2C_WRITE_TRANSFER_RETRY_FAILED)
            ret = 0;
        if (lis331dlh_debug_mode == LIS331DLH_TEST_I2C_WRITE_TRANSFER_RETRY_OK) {
            if (debug_count == 0) {
                ret = 0;
                debug_count++;
            }
        }
#endif
        if (ret == 0) {
            ret = -EIO;
            LIS331DLH_NOTICE_LOG("write_transfer() is retry.\n");
            continue;
        } else if (ret == 1) {
            ret = 0;
            break;
        } else {
            LIS331DLH_NOTICE_LOG("write_transfer() failed.(0x%08x)\n", ret);
        }
    }

    return ret;

err_retrycnt:
    return ret;
err_suspended:
    return -ESHUTDOWN;
}

static int lis331dlh__timer_start(struct lis331dlh_data *data)
{
    int rtn = 0;
    
    LIS331DLH_DEBUG_LOG("start");
#ifdef CONFIG_LIS331DLH_LOG_DEBUG
    if (lis331dlh_debug_count == -1)
        LIS331DLH_DEBUG_LOG("jiffies(%lu)\n", jiffies);
#endif
    if ((data->suspended == 1) ||
        (data->status    == LIS331DLH_STATUS_ERROR))
        return 0;

    mutex_lock(&data->lock);
    if (data->enable) {
        data->status = LIS331DLH_STATUS_ENABLE;
        rtn = hrtimer_start(&(data->timer),
                            ns_to_ktime(data->polling),
                            HRTIMER_MODE_REL);
    } else {
        data->status = LIS331DLH_STATUS_DISABLE;
    }
    LIS331DLH_DEBUG_LOG("end");
    mutex_unlock(&data->lock);
    return rtn;
}

inline static void lis331dlh__timer_stop(struct lis331dlh_data *data)
{
    LIS331DLH_INFO_LOG("start");
    hrtimer_cancel(&data->timer);
    LIS331DLH_DEBUG_LOG("end");
    return;
}
    
static void lis331dlh__queue_work(struct hrtimer *timer)
{
    struct lis331dlh_data *data; 

    LIS331DLH_DEBUG_LOG("start");
    data = container_of(timer, struct lis331dlh_data, timer);
    if (data->suspended == 1)
        return;
    queue_work(data->lis331dlh_wq, &data->work);
    LIS331DLH_DEBUG_LOG("end");
}

static int lis331dlh_read_accelerometer_values(struct i2c_client *client,
                struct lis331dlh_data *data)
{
    int ret;
    int err;
    int count;
    unsigned char reg_data;

    int loopcnt;
    unsigned char reg1arry[LIS331DLH_DATA_REG_SIZE] = {
        LIS331DLH_REG_OUT_X_L,   LIS331DLH_REG_OUT_X_H,   LIS331DLH_REG_OUT_Y_L,  
        LIS331DLH_REG_OUT_Y_H,   LIS331DLH_REG_OUT_Z_L,  LIS331DLH_REG_OUT_Z_H
    };
    u8 accelerometer[LIS331DLH_DATA_REG_SIZE];
#ifdef LIS331DLH_DEBUG_MODE
static int  debug_count = 0;
#endif
    
    LIS331DLH_DEBUG_LOG("start");

    for (count = 0, err = 0; count < LIS331DLH_FAIL_RETRY_NUM; count++) {
        if (data->suspended == 1) {
            goto err_suspended;
        }
        ret = lis331dlh_read_transfer(client, LIS331DLH_I2C_ADDR, 
                                                LIS331DLH_REG_STATUS, 
                                                1,
                                                &reg_data, 
                                                TEGRA_I2C_TRANSFER_RETRY_MAX);
#ifdef LIS331DLH_DEBUG_MODE
        if (lis331dlh_debug_mode == LIS331DLH_TEST_I2C_READ_STATUS_REG_RETRY_FAILED)
            ret = -EIO;
        if (lis331dlh_debug_mode == LIS331DLH_TEST_I2C_READ_STATUS_REG_RETRY_OK) {
            if (debug_count == 0) {
                ret = -EIO;
                debug_count++;
            }
        }
#endif
        if (ret < 0) {
            err = ret;
            continue;
        }
#ifdef LIS331DLH_DEBUG_MODE
        if (lis331dlh_debug_mode == LIS331DLH_TEST_I2C_READ_STATUS_REG_DATA_RETRY_FAILED)
            reg_data = 0;
        if (lis331dlh_debug_mode == LIS331DLH_TEST_I2C_READ_STATUS_REG_DATA_RETRY_OK) {
            if (debug_count == 0) {
                reg_data = 0;
                debug_count++;
            }
        }
#endif
        if ((reg_data & BIT_ZYXDA) == 0) {
            LIS331DLH_NOTICE_LOG("retry.(BIT_ZYXDA is 0)");
            err = -EIO;
            continue;
        }
        if ((reg_data & BIT_ZYXOR) == 1) {
            LIS331DLH_DEBUG_LOG("BIT_ZYXOR = 1");
        }
        for (loopcnt = 0; loopcnt < LIS331DLH_DATA_REG_SIZE; loopcnt++) {
            if (data->suspended == 1) {
                goto err_suspended;
            }
            ret = lis331dlh_read_transfer(client, LIS331DLH_I2C_ADDR, 
                                                reg1arry[loopcnt], 
                                                1,
                                                &accelerometer[loopcnt],
                                                TEGRA_I2C_TRANSFER_RETRY_MAX);
#ifdef LIS331DLH_DEBUG_MODE
            if (lis331dlh_debug_mode == LIS331DLH_TEST_I2C_READ_DATA_REG_RETRY_FAILED)
                ret = -EIO;
            if (lis331dlh_debug_mode == LIS331DLH_TEST_I2C_READ_DATA_REG_RETRY_OK) {
                if (debug_count == 0) {
                    ret = -EIO;
                    debug_count++;
                }
            }
#endif
            if (ret < 0) {
                LIS331DLH_NOTICE_LOG("read_transfer() failed. (0x%04x)\n", ret);
                err = ret;
                break;
            }
        }
        if (loopcnt != LIS331DLH_DATA_REG_SIZE) {
            continue;
        }
        if (data->suspended == 1) {
            goto err_suspended;
        }
        data->data.x_axle = ((accelerometer[1] << 8) | accelerometer[0]);
//        if (data->data.x_axle & MSB_INT16) {
//            data->data.x_axle |= MSN_INT16;
//        }
        data->data.y_axle = ((accelerometer[3] << 8) | accelerometer[2]);
//        if (data->data.y_axle & MSB_INT16) {
//            data->data.y_axle |= MSN_INT16;
//        }
        data->data.z_axle = ((accelerometer[5] << 8) | accelerometer[4]);
//        if (data->data.z_axle & MSB_INT16) {
//            data->data.z_axle |= MSN_INT16;
//        }
        LIS331DLH_DEBUG_LOG("X_L/X_H=0x%x/0x%x", accelerometer[0], accelerometer[1]);
        LIS331DLH_DEBUG_LOG("X_L/X_H=0x%x/0x%x", accelerometer[2], accelerometer[3]);
        LIS331DLH_DEBUG_LOG("X_L/X_H=0x%x/0x%x", accelerometer[4], accelerometer[5]);
        LIS331DLH_DEBUG_LOG("X,Y,Z:%d,%d,%d", 
            data->data.x_axle, data->data.y_axle, data->data.z_axle);
        break;
    }
    if (count == LIS331DLH_FAIL_RETRY_NUM) {
        LIS331DLH_NOTICE_LOG("timeout.");
        goto err_timeout;
    }
    LIS331DLH_DEBUG_LOG("end");
    return 0;
err_timeout:
    LIS331DLH_INFO_LOG("error end");
err_suspended:
    return err;
}

static int lis331dlh_report_accelerometer_values(struct lis331dlh_data *data)
{
    int res;
    
    LIS331DLH_INFO_LOG("start");
    res = lis331dlh_read_accelerometer_values(data->client, data);
    if (res < 0) {
        return res;
    }
    input_report_abs(data->input_dev, ABS_X, data->data.x_axle >> 4);
    input_report_abs(data->input_dev, ABS_Y, data->data.y_axle >> 4);
    input_report_abs(data->input_dev, ABS_Z, data->data.z_axle >> 4);
    input_sync(data->input_dev);

//printk("%04x %04x %04x\n", data->data.x_axle >> 4, data->data.y_axle >> 4, data->data.z_axle >> 4);
    LIS331DLH_INFO_LOG("end");

    return res;
}

#ifdef CONFIG_LIS331DLH_LOG_DEBUG
static int lis331dlh_reg_dump(struct i2c_client *client)
{
    unsigned char data;

    LIS331DLH_DEBUG_LOG("start");
    msleep(PRE_REGISTER_DUMP_WAIT_TIME);

    /* register dump for debug */
    LIS331DLH_DEBUG_LOG("lis331dlh: Register dump ");
    lis331dlh_read_transfer(client, LIS331DLH_I2C_ADDR,
                                    LIS331DLH_CTRL_REG1,
                                    1,
                                    &data,
                                    TEGRA_I2C_TRANSFER_RETRY_MAX);
    LIS331DLH_DEBUG_LOG("%02x ",data);
    lis331dlh_read_transfer(client, LIS331DLH_I2C_ADDR, 
                                    LIS331DLH_CTRL_REG4,
                                    1,
                                    &data,
                                    TEGRA_I2C_TRANSFER_RETRY_MAX);
    LIS331DLH_DEBUG_LOG("%02x \n",data);
    LIS331DLH_DEBUG_LOG("end");

    return 0;
}
#endif

static int lis331dlh_init_device(struct lis331dlh_data *data)
{
    int ret;
    int loopcnt;
    unsigned char reg[LIS331DLH_INIT_REG_SIZE] = {
        LIS331DLH_CTRL_REG1,    LIS331DLH_CTRL_REG4
    };
    unsigned char regdata[LIS331DLH_INIT_REG_SIZE] = {
        LIS331DLH_REG1_DATA,    LIS331DLH_REG4_DATA
    };
    char buff[2];
#ifdef LIS331DLH_DEBUG_MODE
static int  debug_count = 0;
#endif

    LIS331DLH_INFO_LOG("start");
    for (loopcnt = 0; loopcnt < LIS331DLH_INIT_REG_SIZE; loopcnt++) {
        buff[0] = reg[loopcnt];
        buff[1] = regdata[loopcnt];
        ret = lis331dlh_write_transfer(data->client, LIS331DLH_I2C_ADDR, 
                                                     2, 
                                                     buff, 
                                                     TEGRA_I2C_TRANSFER_RETRY_MAX);
#ifdef LIS331DLH_DEBUG_MODE
        if (lis331dlh_debug_mode == LIS331DLH_TEST_INIT_DEVICE_RETRY_FAILED)
            ret = -EIO;
        if (lis331dlh_debug_mode == LIS331DLH_TEST_INIT_DEVICE_RETRY_OK) {
            if (debug_count == 0) {
                ret = -EIO;
                debug_count++;
            }
        }
#endif
        if (ret < 0) {
            LIS331DLH_NOTICE_LOG("write_transfer() failed.(0x%04x)\n",
                                                                        ret);
            goto err_write_reg;
        }
    }
    msleep(REG_INIT_WAIT_TIME);
#ifdef CONFIG_LIS331DLH_LOG_DEBUG
    lis331dlh_reg_dump(data->client);
#endif
    LIS331DLH_INFO_LOG("end");
    return 0;

err_write_reg:
    LIS331DLH_INFO_LOG("error end");
    return ret;
}

static enum hrtimer_restart lis331dlh_timer_func(struct hrtimer *timer)
{
    LIS331DLH_DEBUG_LOG("start");
#ifdef CONFIG_LIS331DLH_LOG_DEBUG
    if ((lis331dlh_debug_count == 100) || (lis331dlh_debug_count == -1)) {
        LIS331DLH_DEBUG_LOG("jiffies(%lu)\n", jiffies);
        lis331dlh_debug_count = 0;
    }
    lis331dlh_debug_count++;
#endif
    lis331dlh__queue_work(timer);

    LIS331DLH_DEBUG_LOG("end");
    return HRTIMER_NORESTART;
}

static void lis331dlh_work_func(struct work_struct *work)
{
    int res;
    struct lis331dlh_data *data;

    LIS331DLH_DEBUG_LOG("start");
    data = container_of(work, struct lis331dlh_data, work);

    if ((data->suspended == 1) ||
        (data->status    == LIS331DLH_STATUS_ERROR) ||
        (data->status    == LIS331DLH_STATUS_DISABLE)){
        return;
    }

    res = lis331dlh_report_accelerometer_values(data);
    if (res < 0) {
        data->status = LIS331DLH_STATUS_ERROR;
        data->errdetail = LIS331DLH_STATUS_ERR2;
        lis331dlh__timer_stop(data);
        LIS331DLH_ERR_LOG("lis331dlh failure. \n");
        goto err_report;
    }

    lis331dlh__timer_start(data);
err_report:
    LIS331DLH_DEBUG_LOG("end");
}

static ssize_t lis331dlh_show_enable(struct device *dev,
            struct device_attribute *attr, char *buf)
{
    struct lis331dlh_data *data;
    ssize_t size;

    LIS331DLH_INFO_LOG("start");
    data  = dev_get_drvdata(dev);
    LIS331DLH_INFO_LOG("end\n");
    size =  sprintf(buf, "%d\n", data->enable);
    return size;
}

static ssize_t lis331dlh_set_enable(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    int err = 0;
    struct lis331dlh_data *data;
    bool new_enable;

    LIS331DLH_INFO_LOG("start");

    data  = dev_get_drvdata(dev);

    if ((data->suspended == 1) ||
        (data->status    == LIS331DLH_STATUS_ERROR) ||
        (data->status    == LIS331DLH_STATUS_INITIALIZE))
        return -EINVAL;

    if (sysfs_streq(buf, "1")) {
        new_enable = true;
    }
    else if (sysfs_streq(buf, "0")) {
        new_enable = false;
    } else {
        LIS331DLH_NOTICE_LOG("sysfs_streq() invalid data\n");
        return -EINVAL;
    }
    if (new_enable == data->enable) {
        LIS331DLH_INFO_LOG("There is no change.");
        return size;
    }

    data->enable = new_enable;
    lis331dlh__timer_start(data);
    LIS331DLH_INFO_LOG("PASS set enable(new_enable=%d)\n", new_enable);

    LIS331DLH_INFO_LOG("end");
    return err ? err : size;
}

static ssize_t lis331dlh_show_status(struct device *dev,
            struct device_attribute *attr, char *buf)
{
    struct lis331dlh_data *data;
    ssize_t size;

    LIS331DLH_INFO_LOG("start");
    data  = dev_get_drvdata(dev);
    if (data->suspended == 1) {
        size = sprintf(buf, "%02d\n", LIS331DLH_STATUS_SUSPEND);
    }else{
        size = sprintf(buf, "%02d\n",
                       (data->status == LIS331DLH_STATUS_ERROR ?
                        data->errdetail : data->status));
    }
    LIS331DLH_INFO_LOG("end\n");
    return size;
}

static ssize_t lis331dlh_show_polling(struct device *dev,
            struct device_attribute *attr, char *buf)
{
    struct lis331dlh_data *data;
    u64 delay;
    ssize_t size;

    LIS331DLH_INFO_LOG("start");
    data  = dev_get_drvdata(dev);
    delay = data->polling;
    LIS331DLH_INFO_LOG("end\n");
    size = sprintf(buf, "%lld\n", delay);
    return size;
}

static ssize_t lis331dlh_set_polling(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    struct lis331dlh_data *data;
    int res = 0;
    u64 delay_ns;
    u64 delay_ns_before;

    LIS331DLH_INFO_LOG("start");


    data  = dev_get_drvdata(dev);

    if ((data->suspended == 1) ||
        (data->status    == LIS331DLH_STATUS_ERROR) ||
        (data->status    == LIS331DLH_STATUS_INITIALIZE))
        return -EINVAL;

    res = strict_strtoll(buf, 10, &delay_ns);
    if (res < 0) {
        LIS331DLH_NOTICE_LOG("polling invalid data.\n");
        return res;
    }
    delay_ns_before = data->polling;
    if (delay_ns < LIS331DLH_POLLING_MIN) {
        LIS331DLH_NOTICE_LOG("polling data is little.");
        data->polling = LIS331DLH_DEF_POLLING_INTERVAL;
    } else if (delay_ns > LIS331DLH_POLLING_MAX) {
        LIS331DLH_NOTICE_LOG("polling data is large.");
        data->polling = LIS331DLH_POLLING_MAX;
    } else {
        data->polling = delay_ns;
    }

    if (data->polling != delay_ns_before) {
        lis331dlh__timer_start(data);
    }
    LIS331DLH_INFO_LOG("end");

    return size;
}

#ifdef LIS331DLH_DEBUG_SYSFS
static ssize_t lis331dlh_test_sysfs(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf, size_t size);
static ssize_t lis331dlh_test_sysfs_reinit(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf, size_t size);
static ssize_t lis331dlh_test_sysfs_suspend(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf, size_t size);
/* sysfs(debug) */
static DEVICE_ATTR(test_sysfs, S_IRUGO | S_IWUSR | S_IWGRP,
            NULL, lis331dlh_test_sysfs);
static DEVICE_ATTR(test_sysfs_reinit, S_IRUGO | S_IWUSR | S_IWGRP,
            NULL, lis331dlh_test_sysfs_reinit);
static DEVICE_ATTR(test_sysfs_suspend, S_IRUGO | S_IWUSR | S_IWGRP,
            NULL, lis331dlh_test_sysfs_suspend);

static void lis331dlh_debug_sysfs_init(struct lis331dlh_data *data)
{
    int err;
    err = device_create_file(&(data->input_dev)->dev, &dev_attr_test_sysfs);
    err = device_create_file(&(data->input_dev)->dev, &dev_attr_test_sysfs_reinit);
    err = device_create_file(&(data->input_dev)->dev, &dev_attr_test_sysfs_suspend);
    return;
}

static void lis331dlh_debug_sysfs_exit(struct lis331dlh_data *data)
{
    device_remove_file(&data->client->dev, &dev_attr_test_sysfs);
    device_remove_file(&data->client->dev, &dev_attr_test_sysfs_reinit);
    device_remove_file(&data->client->dev, &dev_attr_test_sysfs_suspend);
    return;
}
#endif

/* sysfs */
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
            lis331dlh_show_enable, lis331dlh_set_enable);
static DEVICE_ATTR(status, S_IRUGO | S_IWUSR | S_IWGRP,
            lis331dlh_show_status, NULL);
static DEVICE_ATTR(polling, S_IRUGO | S_IWUSR | S_IWGRP,
            lis331dlh_show_polling,  lis331dlh_set_polling);



static int lis331dlh_suspend(struct device *dev)
{
    struct lis331dlh_data *data;
    struct i2c_client *client;
    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);
    data->suspended = 1;

    return 0;
}

static int lis331dlh_re_init(struct device *dev)
{
    int ret = 0;
    struct i2c_client *client;
    struct lis331dlh_data *data;
    int count;
    
    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);

    lis331dlh__timer_stop(data);
    cancel_work_sync(&data->work);

    mutex_lock(&data->lock);
    data->suspended = 0;
    data->status = LIS331DLH_STATUS_INITIALIZE;
    mutex_unlock(&data->lock);

    for (count = 0; count < LIS331DLH_FAIL_RETRY_NUM; count++) {
        ret = lis331dlh_init_device(data);
        if (ret < 0) {
            continue;
        }

        break;
    }
    if (count == LIS331DLH_FAIL_RETRY_NUM) {
        data->status = LIS331DLH_STATUS_ERROR;
        data->errdetail = LIS331DLH_STATUS_ERR1;
        LIS331DLH_ERR_LOG("lis331dlh failure. \n");
        goto err_wakeup;
    }
    LIS331DLH_NOTICE_LOG("reinit done");
    lis331dlh__timer_start(data);

err_wakeup:
    return ret;
}

/*** The callback function from an I2C core driver ***/
static int lis331dlh_probe(struct i2c_client *client,
                   const struct i2c_device_id *devid)
{
    int ret;
    int err = 0;
    int count;
    struct lis331dlh_data *data;
    unsigned char bytebuf;

    LIS331DLH_INFO_LOG("start");

#ifdef LIS331DLH_DEBUG_MODE
    lis331dlh_debug_client = client; 
#endif

    data = kzalloc(sizeof(*data), GFP_KERNEL);
#ifdef LIS331DLH_TEST_KZALLOC_FAILED
    data = NULL;
#endif
    if (data == NULL) {
        LIS331DLH_ERR_LOG("failed to allocate memory for module data\n");
        err = -ENOMEM;
        goto probe_end;
    }
    LIS331DLH_DEBUG_LOG("kzalloc data=%p", data);
    data->status = LIS331DLH_STATUS_INITIALIZE;
    data->suspended = 0;
    data->client = client;

    data->edev.dev                    = &client->dev;
    data->edev.reinit                 = lis331dlh_re_init;
    data->edev.sudden_device_poweroff = NULL;
    err = early_device_register(&data->edev);
    if ( err < 0 ){
        goto err_read_reg;
    }

    i2c_set_clientdata(client, data);

    /* read chip id */
    ret = lis331dlh_read_transfer(client, LIS331DLH_I2C_ADDR, 
                                          LIS331DLH_WHO_AM_I,
                                          1,
                                          &bytebuf,
                                          TEGRA_I2C_TRANSFER_RETRY_MAX);
    if (ret != 0) {
        LIS331DLH_ERR_LOG("i2c for reading chip id failed. ret=%02x\n", ret);
        err = ret;
        goto err_early_device_register;
    }
#ifdef LIS331DLH_TEST_DEVICE_IDENTIFICATION_FAILED
    bytebuf = 0;
#endif
    if (bytebuf != LIS331DLH_DEVICE_ID) {
        LIS331DLH_ERR_LOG("Device identification failed. (0x%02x)\n", bytebuf);
        err = -ENODEV;
        goto err_early_device_register;
    } else {
        LIS331DLH_INFO_LOG("Device found. (ID=0x%02X)", bytebuf);
    }

    mutex_init(&data->lock);

    /* allocate input_device */
    data->input_dev = input_allocate_device();
#ifdef LIS331DLH_TEST_ALLOC_INPUT_DEVICE_FAILED
    data->input_dev = NULL;
#endif
    if (!data->input_dev) {
        LIS331DLH_ERR_LOG("input_allocate_device() failed.\n");
        err = -ENOMEM;
        goto err_alloc_device;
    }
    LIS331DLH_NOTICE_LOG("input_allocate_device=%p",data->input_dev);

    input_set_drvdata(data->input_dev, data);
    (data->input_dev)->name = LIS331DLH_EVENT_NAME;

    /* accelerometer */
    input_set_capability(data->input_dev, EV_ABS, ABS_X);
    input_set_abs_params(data->input_dev, ABS_X, -2048, 2047, 0, 0);
    input_set_capability(data->input_dev, EV_ABS, ABS_Y);
    input_set_abs_params(data->input_dev, ABS_Y, -2048, 2047, 0, 0);
    input_set_capability(data->input_dev, EV_ABS, ABS_Z);
    input_set_abs_params(data->input_dev, ABS_Z, -2048, 2047, 0, 0);
    LIS331DLH_DEBUG_LOG("PASS setting static values.");

    /* register input_device */
    err = input_register_device(data->input_dev);
#ifdef LIS331DLH_TEST_INPUT_REGISTER_DEVICE_FAILED
    err = -EIO;
#endif
    if (err < 0) {
        LIS331DLH_ERR_LOG("input_register_device() failed\n");
        input_free_device(data->input_dev);
        goto err_input_register_device;
    }
    LIS331DLH_DEBUG_LOG("input_register_device().");

    hrtimer_init(&data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

    data->timer.function = lis331dlh_timer_func;
    
    data->lis331dlh_wq = create_singlethread_workqueue(LIS331DLH_DRIVER_NAME);
#ifdef LIS331DLH_TEST_CREATE_WORK_QUEUE_FAILED
    data->lis331dlh_wq = NULL;
#endif
    if (!data->lis331dlh_wq) {
        err = -ENOMEM;
        LIS331DLH_ERR_LOG("create_singlethread_workqueue() failed\n");
        goto err_create_workqueue;
    }
    LIS331DLH_DEBUG_LOG("PASS create_singlethread_workqueue().\n");
    
    /* this is the thread function we run on the work queue */
    INIT_WORK(&data->work, lis331dlh_work_func);
    LIS331DLH_DEBUG_LOG("PASS INIT_WORK().\n");
    err = device_create_file(&(data->input_dev)->dev, &dev_attr_enable);
#ifdef LIS331DLH_TEST_CREATE_SYSFS1_FAILED
    err = -1;
#endif
    if (err  < 0) {
        LIS331DLH_ERR_LOG("device_create_file() failed(enable).\n");
        goto err_device_create_file;
    }
    LIS331DLH_DEBUG_LOG("PASS device_create_file() for enable.");
    err = device_create_file(&(data->input_dev)->dev, &dev_attr_polling);
#ifdef LIS331DLH_TEST_CREATE_SYSFS2_FAILED
    err = -1;
#endif
    if (err  < 0) {
        LIS331DLH_ERR_LOG("device_create_file() failed(polling).\n");
        goto err_device_create_file2;
    }
    LIS331DLH_DEBUG_LOG("PASS device_create_file() for polling.");
    err = device_create_file(&(data->input_dev)->dev, &dev_attr_status);
#ifdef LIS331DLH_TEST_CREATE_SYSFS2_FAILED
    err = -1;
#endif
    if (err  < 0) {
        LIS331DLH_ERR_LOG("device_create_file() failed(status).\n");
        goto err_device_create_file3;
    }
    LIS331DLH_DEBUG_LOG("PASS device_create_file() for status.");

    dev_set_drvdata(&(data->input_dev)->dev, data);

#ifdef LIS331DLH_DEBUG_SYSFS
    lis331dlh_debug_sysfs_init(data);
#endif
    for (count = 0; count < LIS331DLH_FAIL_RETRY_NUM; count++) {
        ret = lis331dlh_init_device(data);
        if (ret < 0) {
            continue;
        }

        break;
    }
    if (count == LIS331DLH_FAIL_RETRY_NUM) {
        LIS331DLH_ERR_LOG("initreg() failed. (ret=0x%04x)\n", ret);
        err = ret;
        goto err_device_create_file3;
    }
    
    data->enable = false;
    data->polling = LIS331DLH_DEF_POLLING_INTERVAL;
    data->status = LIS331DLH_STATUS_DISABLE;

    LIS331DLH_INFO_LOG("end");
    return 0;

err_device_create_file3:
    device_remove_file(&(data->input_dev)->dev, &dev_attr_polling);
err_device_create_file2:
    device_remove_file(&(data->input_dev)->dev, &dev_attr_enable);
err_device_create_file:
    destroy_workqueue(data->lis331dlh_wq);
    input_unregister_device(data->input_dev);
err_create_workqueue:
err_input_register_device:
err_early_device_register:
    early_device_unregister(&data->edev);
err_read_reg:
err_alloc_device:
    kfree(data);
probe_end:
    LIS331DLH_INFO_LOG("end");
    return err;
}

/*** Delete driver ***/
static int lis331dlh_remove(struct i2c_client *client)
{
    int err = 0;

    struct lis331dlh_data *data;
    LIS331DLH_INFO_LOG("start");

    data = i2c_get_clientdata(client);
    LIS331DLH_INFO_LOG("step-0");
    lis331dlh__timer_stop(data);
    LIS331DLH_INFO_LOG("step-1");
    cancel_work_sync(&data->work);
    LIS331DLH_INFO_LOG("step-2");
    destroy_workqueue(data->lis331dlh_wq);
    LIS331DLH_INFO_LOG("step-3");
#ifdef LIS331DLH_DEBUG_SYSFS
    lis331dlh_debug_sysfs_exit(data);
    LIS331DLH_INFO_LOG("step-4");
#endif

    early_device_unregister(&data->edev);

    input_unregister_device(data->input_dev);
    LIS331DLH_INFO_LOG("step-5");
    mutex_destroy(&data->lock);
    LIS331DLH_INFO_LOG("step-6");
    kfree(data);
    LIS331DLH_INFO_LOG("end");

    return err;
}

#ifdef LIS331DLH_DEBUG_SYSFS
static ssize_t lis331dlh_test_sysfs_suspend(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf, size_t size)
{
    LIS331DLH_NOTICE_LOG("dummy suspend.\n");
    lis331dlh_suspend(dev);
    return size;
}

static ssize_t lis331dlh_test_sysfs_reinit(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf, size_t size)
{
    LIS331DLH_NOTICE_LOG("dummy reinit.\n");
    lis331dlh_re_init(dev);
    return size;
}

static ssize_t lis331dlh_test_sysfs(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf, size_t size)
{
    u64 test_num;
    int ret;
    ret = strict_strtoll(buf, 10, &test_num);
    LIS331DLH_NOTICE_LOG("test_sysfs = %lld.\n", test_num);
    lis331dlh_debug_mode = test_num;

    if (test_num == 999) {
        lis331dlh_remove(lis331dlh_debug_client); 
        lis331dlh_exit();
        LIS331DLH_NOTICE_LOG("exit complete.\n");
    }

    return size;
}
#endif


static const struct i2c_device_id lis331dlh_id[] = {
    { LIS331DLH_DRIVER_NAME, 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, lis331dlh_id);

static const struct dev_pm_ops lis331dlh_pm_ops = {
    .suspend    = lis331dlh_suspend,
};

static struct i2c_driver lis331dlh_driver = {
    .probe   = lis331dlh_probe,
    .remove  = __devexit_p(lis331dlh_remove),
    .id_table = lis331dlh_id,
    .driver = {
        .owner = THIS_MODULE,
        .name  = LIS331DLH_DRIVER_NAME,
        .pm    = &lis331dlh_pm_ops,
    }
};

static int __init lis331dlh_init(void)
{
    LIS331DLH_INFO_LOG("start");
    return i2c_add_driver(&lis331dlh_driver);
}

static void __exit lis331dlh_exit(void)
{
    LIS331DLH_INFO_LOG("start");
    i2c_del_driver(&lis331dlh_driver);
}

module_init(lis331dlh_init);
module_exit(lis331dlh_exit);

MODULE_DESCRIPTION("LIS331DLH 3-axes linear Accelerometer Driver");
MODULE_AUTHOR("sakamoto QNET <@>");
MODULE_LICENSE("GPL");
