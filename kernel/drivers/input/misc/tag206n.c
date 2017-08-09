/**
 * @file
 * drivers/input/misc/tag206n.c
 *
 * Copyright(C) 2012-2013 FUJITSU TEN LIMITED
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
#include <linux/nonvolatile.h>
#include <linux/lites_trace_drv.h>
#ifdef CONFIG_TAG206N_CRC_CHECK
 #include <linux/crc7.h>
#endif
#include "tag206n_priv.h"
#include <linux/time.h>

/** 
 * @brief         I2C read function.
 * @param[in]     client I2C client handle pointer
 * @param[in]     saddr I2C slave address
 * @param[in]     reg Register data (8bits sub address)
 * @param[in]     len Read block length
 * @param[out]    buf The pointer which stores the read data
 * @param[in]     retrycnt A specification of the count of the maximum retry
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EPERM Parameter error.
 * @retval        -EIO Device I/O error.
 * @retval        -ESHUTDOWN Fastboot executing.
 * @note          Returns negative errno, else the number of messages executed.
 */
int tag206n_read_transfer(struct i2c_client *client,
               unsigned char saddr, unsigned char reg,
               unsigned int  len, unsigned char *buf, unsigned char retrycnt)
{
    struct i2c_msg msgs[2];
    int ret = 0;
    unsigned char cnt = 0;
    struct tag206n_data *data;

    data = i2c_get_clientdata(client);

    if (retrycnt == 0) {
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
        ret = i2c_transfer(client->adapter, msgs, 2);
        if ((ret >= 0) && (ret < 2)) {
            ret = -EIO;
            TAG206N_NOTICE_LOG("tag206n_read_transfer() is retry.");
            continue;
        } else if (ret == 2) {
            ret = 0;
            break;
        } else {
            TAG206N_NOTICE_LOG("tag206n_read_transfer() failed.(0x%08x)\n", ret);
        }
    }

    return ret;

err_retrycnt:
    return ret;
err_suspended:
    return -ESHUTDOWN;
}

/** 
 * @brief         I2C write function.
 * @param[in]     client I2C client handle pointer
 * @param[in]     saddr I2C slave address
 * @param[in]     len Write Block length
 * @param[in]     buf The pointer which stores the write data
 * @param[in]     retrycnt A specification of the count of the maximum retry
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EPERM Parameter error.
 * @retval        -EIO Device I/O error.
 * @retval        -ESHUTDOWN Fastboot executing.
 * @note          Returns negative errno, else the number of messages executed.
 */
int tag206n_write_transfer(struct i2c_client *client,
                unsigned char saddr, unsigned int len, unsigned char const *buf, 
                                                        unsigned char retrycnt)
{
    struct i2c_msg msgs[1];
    int ret = 0;
    unsigned char cnt;
    struct tag206n_data *data;

    data = i2c_get_clientdata(client);

    if (retrycnt == 0) {
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
        ret = i2c_transfer(client->adapter, msgs, 1);
        if (ret == 0) {
            ret = -EIO;
            TAG206N_NOTICE_LOG("tag206n_write_transfer() is retry.");
            continue;
        } else if (ret == 1) {
            ret = 0;
            break;
        } else {
            TAG206N_NOTICE_LOG("tag206n_write_transfer() failed.(0x%08x)\n", ret);
        }
    }

    return ret;

err_retrycnt:
    return ret;
err_suspended:
    return -ESHUTDOWN;
}

/** 
 * @brief         I2C write word function.
 * @param[in]     client I2C Client handle pointer
 * @param[in]     saddr I2C slave address
 * @param[in]     reg write register address (8bits sub address)
 * @param[in]     data Write-in word data
 * @param[in]     retrycnt A specification of the count of the maximum retry
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        !=0 Error end.
 * @note          Returns negative errno, else the number of messages executed.
 */
int tag206n_write_word_transfer(struct i2c_client *client,
                unsigned char saddr, unsigned char reg, short int data, 
                                                        unsigned char retrycnt)
{
    char buff[3];

    buff[0] = reg;
    buff[1] = (data >> 8);
    buff[2] = data & 0xff;

    return tag206n_write_transfer(client, saddr, 3, buff, retrycnt);
}

/** 
 * @brief         msleep wrapper function.
 * @param[in]     time Set sleep time (msec).
 * @return        None.
 */
inline static void tag206n_msleep(unsigned int time)
{
    msleep(time);
}

/**
 * @brief         Hardware error polling timer start function.
 * @param[in,out] *data The structure of Gyro sensor driver information.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        !=0 Error end.
 */
static int tag206n__timer_start(struct tag206n_data *data)
{
    int rtn = 0;
    s64  polling;

    TAG206N_DEBUG_LOG("start");
    if (unlikely(data->suspended == 1))
        return 0;

    mutex_lock(&data->lock);
    if (unlikely(data->status == TAG206N_STATUS_ERROR))
        goto end_status;

    if (unlikely(data->status == TAG206N_STATUS_INITIALIZE)) {
        data->init_check_flag = true;
        data->status = TAG206N_STATUS_DISABLE;
        polling      = data->interval_system_reset * 1000000000;
    }else{
        polling = data->health_check_elapsed_time;
        if (likely(data->enable)) {
            if ( data->status != TAG206N_STATUS_ENABLE) {
                TAG206N_NOTICE_LOG("status change to ENABLE\n");
                TAG206N_DRC_TRC( "status change to ENABLE\n" );
                data->get_data_count = 0;
                memset( &data->get_data_tv, 0, sizeof( struct timeval ) );
            }

            data->status = TAG206N_STATUS_ENABLE;
            if ( data->health_check_elapsed_time > data->polling_elapsed_time ) {
                polling = data->polling_elapsed_time;
            }
        }else{
            if ( data->status != TAG206N_STATUS_DISABLE) {
                TAG206N_NOTICE_LOG("status change to DISABLE\n");
                TAG206N_DRC_TRC( "status change to DISABLE\n" );
            }
            data->status = TAG206N_STATUS_DISABLE;
        }
    }

    if ( unlikely(polling < 0) ) {
        polling = 0;
    }
    data->polling_delta = polling;
    TAG206N_INFO_LOG("timer interval = %lld\n", polling);
    rtn = hrtimer_start(&(data->timer),
                        ns_to_ktime(polling),
                        HRTIMER_MODE_REL);
    TAG206N_DEBUG_LOG("end");

end_status:
    mutex_unlock(&data->lock);
    return rtn;
}

/**
 * @brief         Hardware error polling timer stop function.
 * @param[in,out] *data The structure of Gyro sensor driver information.
 * @return        None.
 */
inline static void tag206n__timer_stop(struct tag206n_data *data)
{
    hrtimer_cancel(&data->timer);
    return;
}

/**
 * @brief         Hardware error polling timer execute function.
 * @param[in]     *timer The structure of High-resolution timers.
 * @return        None.
 */
static void tag206n__queue_work(struct hrtimer *timer)
{
    struct tag206n_data *data; 

    TAG206N_DEBUG_LOG("start");
    data = container_of(timer, struct tag206n_data, timer);
    if (data->suspended == 1)
        return;
    queue_work(data->tag206n_wq, &data->work);
    TAG206N_DEBUG_LOG("end");
}

/**
 * @brief         Parity check function.
 * @param[in]     in Check data.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EIO Parity error.
 */
static int tag206n_parity_check(u8 in)
{
    TAG206N_DEBUG_LOG("start");
    in = in ^ (in >> 4);
    in = in ^ (in >> 2);
    in = in ^ (in >> 1);
    in = in & 0x01;
    if (in != 0x01) {
        return -EIO;
    }
    TAG206N_DEBUG_LOG("end");
    return 0;
}

/**
 * @brief         Check Gyro sensor device read data.
 * @param[in]     *buf_p Check data.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EIO Check data error.
 */
static int tag206n_gyro_data_check(u8 *buf_p)
{
    u32 temp1;
#ifdef CONFIG_TAG206N_CRC_CHECK
    u8  crc;
    u8  mx[3];
#endif

    TAG206N_DEBUG_LOG("start");
    /* top 4byte parity check */
    temp1 = *((u32 *)&buf_p[0]);
    temp1 = temp1 ^ (temp1 >> 4);
    temp1 = temp1 ^ (temp1 >> 2);
    temp1 = temp1 ^ (temp1 >> 1);
    temp1 = temp1 & 0x00010101;
    if (temp1 != 0x00010101) {
        TAG206N_NOTICE_LOG("FAILED parity check.");
        goto data_check_end;
    }

#ifdef CONFIG_TAG206N_CRC_CHECK
    /* data crc check*/
    mx[0] = ((buf_p[0] & 0x7F) << 1 ) | ((buf_p[1] & 0x7f) >> 6);
    mx[1] = ((buf_p[1] & 0x7F) << 2 ) | ((buf_p[2] & 0x7f) >> 5);
    mx[2] = ((buf_p[2] & 0x7f) << 3 );
    crc = TAG206N_CRC_INIT;
    crc = crc7(crc, mx, 3);
    if (crc != 0) {
        TAG206N_NOTICE_LOG("CRC Error calc_crc=0x%x)", crc);
        goto data_check_end;
    }
    TAG206N_DEBUG_LOG("PASS crc check. 0x%02x", crc);
#endif

    return 0;
data_check_end:
    TAG206N_DEBUG_LOG("error end");
    return -EIO;
}

/**
 * @brief         Send reset command for Gyro sensor device.
 * @param[in,out] *data The structure of Gyro sensor driver information.
 * @return        None.
 */
static void tag206n_command_reset(struct tag206n_data *data)
{
    struct i2c_msg msgs[2];
    int ret = 0;
    unsigned char reg;

    TAG206N_INFO_LOG("start");
    if (data->suspended == 1)
        return;

    msgs[0].addr  = 0x7f;
    msgs[0].flags = I2C_M_RD | I2C_M_IGNORE_NAK;
    msgs[0].buf   = &reg;
    msgs[0].len   = 1;

    msgs[1].addr  = TAG206N_I2C_ADDR;
    msgs[1].flags = I2C_M_RD; /* read (Slave to Master)*/
    msgs[1].buf   = &reg;
    msgs[1].len   = 1;

    ret = i2c_transfer(data->client->adapter, msgs, 2);
    TAG206N_NOTICE_LOG("Command reset result(0x%02x) %d\n", reg, ret);

    TAG206N_INFO_LOG("end");
    return;
}

/**
 * @brief         Send unlock write protect command for Gyro sensor device.
 * @param[in,out] *data The structure of Gyro sensor driver information.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -ESHUTDOWN Fastboot executing.
 */
static int tag206n_unlock_write_protect(struct tag206n_data *data)
{
    int count;
    int ret;
    u8  wprotect_data;
    char buff[2];

    TAG206N_INFO_LOG("start");
    for (count = 0; count < TAG206N_FAIL_RETRY_NUM; count++) {
        ret = 0;
        if (data->suspended == 1) {
            goto err_suspended;
        }

        /* request unlock write-protect */
        buff[0] = TAG206N_REG_WPROTECT;
        buff[1] = TAG206N_WPROTECT_DATA;
        ret = tag206n_write_transfer(data->client, TAG206N_I2C_ADDR, 
                            2, buff, TEGRA_I2C_TRANSFER_RETRY_MAX);
        if (ret < 0) {
            TAG206N_NOTICE_LOG("wp-reg write failed.(0x%02x)\n", ret);
            continue;
        }

        /* wait unlock */
        tag206n_msleep(data->interval_unlock_protect);

        /* check unlock complete */
        ret = tag206n_read_transfer(data->client, TAG206N_I2C_ADDR, 
                                        TAG206N_REG_WPROTECT, 1, &wprotect_data, 
                                        TEGRA_I2C_TRANSFER_RETRY_MAX);
        if (ret < 0) {
            TAG206N_NOTICE_LOG("wp-reg read fail.\n");
            continue;
        }

        ret = tag206n_parity_check(wprotect_data);
        if (ret < 0) {
            TAG206N_NOTICE_LOG("wp-reg parity check failed.\n");
            continue;
        }

        if (wprotect_data != TAG206N_WPROTECT_DATA) {
            TAG206N_NOTICE_LOG("wp-unlock failed.\n");
            ret = -EIO;
            continue;
        }

        /* unlock complete ! */
        break;
    }

    if (ret < 0) {
        TAG206N_NOTICE_LOG("write protect unlock failed.\n");
    }
    TAG206N_INFO_LOG("end");
    return ret;

err_suspended:
    return -ESHUTDOWN;
}

/**
 * @brief         Execute Gyro sensor device reset.
 * @param[in]     *data The structure of Gyro sensor driver information.
 * @param[in]     *io_reset Execute io reset.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        !=0 Error end.
 */
static int tag206n_reset_device(struct tag206n_data *data, int io_reset)
{
    int ret;

    TAG206N_INFO_LOG("start");
    /* tag206n i2c-io reset*/
    if (io_reset)
        tag206n_command_reset(data);

    /* unlock write protect */
    ret = tag206n_unlock_write_protect(data);
    if (ret < 0) {
        goto err;
    }

    /* system reset */
    ret = tag206n_write_word_transfer(data->client, TAG206N_I2C_ADDR, 
                                      TAG206N_REG_WPROTECT,
                                      TAG206N_RELEASE_CHECK_DATA, 
                                      TEGRA_I2C_TRANSFER_RETRY_MAX);
    if (ret < 0) {
        goto err;
    }
    TAG206N_INFO_LOG("end");
    return ret;

err:
    TAG206N_NOTICE_LOG("reset_device error.\n");
    return ret;
}

/**
 * @brief         Check status register in Gyro sensor device.
 * @param[in]     *data The structure of Gyro sensor driver information.
 * @param[out]    *stat Read status data.
 * @param[in]     count Retry count.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EIO Device I/O error.
 */
static int tag206n_check_status_register(struct tag206n_data *data, u8 *stat, int count)
{
    int ret = 0;
    unsigned char statusdata = 0;

    TAG206N_DEBUG_LOG("start");
    while(count--) {
        /* read status register */
        ret = tag206n_read_transfer(data->client, TAG206N_I2C_ADDR,
                                    TAG206N_REG_STATCNT,
                                    1,
                                    &statusdata,
                                    TEGRA_I2C_TRANSFER_RETRY_MAX);
        if (ret < 0) {
            TAG206N_NOTICE_LOG("stat-reg read failed. (0x%x)\n", ret);
            goto retry;
        }

        /* parity check */
        ret = tag206n_parity_check(statusdata);
        if (ret < 0) {
            TAG206N_NOTICE_LOG("stat-reg parity check failed. \n");
            goto retry;
        }

        /* update check */
        if ((statusdata & BIT_AC) == 0) {
            *stat = statusdata;
            break;
        }
        ret = -EIO;
retry:
        tag206n_msleep(data->interval_status_update);
    }

    TAG206N_DEBUG_LOG("end");
    return ret;
}

/**
 * @brief         Device health check function.
 * @param[in]     stat Check data.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EIO Health check error.
 */
static int tag206n_device_health_check(u8 stat)
{
    int ret = 0;

    if ( (stat & BIT_STS)  == 0 &&
         (stat & BIT_STA8) == 0 &&
         (stat & BIT_STA7) == 0 ) {
        /* health ok! */
        TAG206N_INFO_LOG("device health check ok. (0x%02x)\n", stat);
    } else {
        ret = -EIO;
        TAG206N_NOTICE_LOG("device error detect. (0x%02x)\n", stat);
    }

    return ret;
}

/**
 * @brief         Execute health check for Gyro sensor device.
 * @param[in,out] *data The structure of Gyro sensor driver information.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        !=0 Error end.
 */
static int tag206n_execute_device_health_check(struct tag206n_data *data)
{
    int count;
    int ret;
    u8  stat;

    TAG206N_DEBUG_LOG("start");
    data->health_check_elapsed_time -= data->polling_delta;
    if (likely(data->health_check_elapsed_time > 0)) {
        return 0;
    }
    data->health_check_elapsed_time = data->interval_health_check;

    for (count = 0; count < TAG206N_FAIL_RETRY_NUM; count++) {
        ret = tag206n_check_status_register(data, &stat, TAG206N_FAIL_RETRY_NUM);
        if (ret) {
            continue;
        }

        ret = tag206n_device_health_check(stat);
        if (ret) {
            data->errdetail = TAG206N_STATUS_ERR1;
            continue;
        }
        break;
    }
    if (unlikely(data->init_check_flag == true ) &&
          likely(ret == 0                      )) {
        data->init_check_flag = false;
        TAG206N_NOTICE_LOG("status change to DISABLE\n");
        TAG206N_DRC_TRC( "status change to DISABLE\n" );
        TAG206N_NOTICE_LOG("device init done\n");
    }
    TAG206N_DEBUG_LOG("end");
    return ret;
}

/**
 * @brief         Read register data for Gyro sensor device.
 * @param[in]     *data The structure of Gyro sensor driver information.
 * @param[in]     read_reg Read register address offset.
 * @param[out]    *outdata Read data.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        !=0 Error end.
 */
static int tag206n_read_data_values(struct tag206n_data *data,
                                    u8   read_reg,
                                    s16 *outdata)
{
    int ret;
    u8  data_gyro[TAG206N_DATA_REG_SIZE];

    TAG206N_DEBUG_LOG("start");

    ret = tag206n_read_transfer(data->client, TAG206N_I2C_ADDR,
                                read_reg,
                                3,
                                (unsigned char *)data_gyro,
                                TEGRA_I2C_TRANSFER_RETRY_MAX);
    if (ret < 0) {
        goto err;
    }

    ret = tag206n_gyro_data_check(data_gyro);
    if (ret < 0) {
        goto err;
    }

    *outdata = (data_gyro[1] & 0x7F) | ((u16)(data_gyro[0] & 0x7F) << 7);
    TAG206N_DEBUG_LOG("end %04x", *outdata);
err:
    return ret;
}

/**
 * @brief         Read register data for Gyro sensor device.
 * @param[in]     *data The structure of Gyro sensor driver information.
 * @param[in]     read_reg Read register address offset.
 * @param[out]    *outdata Read data.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        !=0 Error end.
 */
static int tag206n_execute_read_gyro_data(struct tag206n_data *data)
{
    int count;
    int ret;
    u8  stat;

    TAG206N_DEBUG_LOG("start");
    if (!data->enable) {
        return 0;
    }

    data->polling_elapsed_time -= data->polling_delta;
    if (data->polling_elapsed_time > 0) {
        return 0;
    }

    data->polling_elapsed_time = data->polling;

    for (count = 0; count < TAG206N_FAIL_RETRY_NUM; count++) {
        ret = tag206n_check_status_register(data, &stat, 1);
        if (ret) {
            continue;
        }

        ret = tag206n_read_data_values(data,
                                       TAG206N_REG_GYRODATA_HI,
                                       &data->data.z_axle);
        if (ret < 0) {
            continue;
        }

        input_report_rel(data->input_dev, REL_Z, data->data.z_axle);
        input_sync(data->input_dev);
        data->get_data_count++;
        do_gettimeofday( &(data->get_data_tv) );

        TAG206N_INFO_LOG("gyro report. (%5d)\n", data->data.z_axle);
        break;
    }

    TAG206N_DEBUG_LOG("end");
    return ret;
}


/**
 * @brief         Hardware error polling timer execute handler.
 * @param[in]     *timer The structure of High-resolution timers.
 * @return        Result.
 * @retval        HRTIMER_NORESTERT Timer function end.
 */
static enum hrtimer_restart tag206n_timer_func(struct hrtimer *timer)
{
    tag206n__queue_work(timer);
    return HRTIMER_NORESTART;
}

/**
 * @brief         Hardware error polling work queue handler.
 * @param[in,out] *work The structure of work queue.
 * @return        None.
 */
static void tag206n_work_func(struct work_struct *work)
{
    int ret;
    unsigned int error_code = 0;

    struct tag206n_data *data;

    TAG206N_INFO_LOG("start");
    data = container_of(work, struct tag206n_data, work);
    if (unlikely(data->suspended == 1) ||
        unlikely(data->status    == TAG206N_STATUS_ERROR)){
        return;
    }

    ret = tag206n_execute_read_gyro_data(data);
    if (unlikely(ret < 0)) {
        goto failsafe;
    }

    ret = tag206n_execute_device_health_check(data);
    if (unlikely(ret < 0)) {
        goto failsafe;
    }

    data->failsafe_counter = 0;
    tag206n__timer_start(data);

    TAG206N_INFO_LOG("end");
    return;

failsafe:
    data->failsafe_counter++;

    if (!data->errdetail) {
        data->errdetail = TAG206N_STATUS_ERR2;
    }

    if (data->failsafe_counter >= TAG206N_RESET_RETRY_NUM) {
	error_code = TAG206N_ERROR_INFO_RETRY_OUT;
        goto critical_err;
    }

    ret = tag206n_reset_device(data, true);
    if (ret) {
	error_code = TAG206N_ERROR_INFO_RESET;
        goto critical_err;
    }

    data->status    = TAG206N_STATUS_INITIALIZE;
    data->errdetail = 0;
    TAG206N_NOTICE_LOG("status change to INITIALIZE\n");
    TAG206N_DRC_TRC( "status change to INITIALIZE\n" );
    tag206n__timer_start(data);
    TAG206N_DEBUG_LOG("failsafe");
    return;

critical_err:
    data->status = TAG206N_STATUS_ERROR;
    tag206n__timer_stop(data);
    TAG206N_NOTICE_LOG("status change to ERROR\n");
    TAG206N_DRC_TRC( "status change to ERROR\n" );
    TAG206N_ERR_LOG("tag206n device error detect.\n");
    TAG206N_DRC_HARD_ERR( TAG206N_LITES_FAILSAFE_ERROR,
	error_code, 0, 0, 0 );
    return;
}

/**
 * @brief         Sysfs interface for show enable data.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr The structure of device attribute.
 * @param[out]    *buf Show enable data.
 * @return        Read buffer size.
 */
static ssize_t tag206n_show_enable(struct device *dev,
            struct device_attribute *attr, char *buf)
{
    struct tag206n_data *data;
    ssize_t size;

    TAG206N_INFO_LOG("start");
    data  = dev_get_drvdata(dev);
    size = sprintf(buf, "%d\n", data->enable);
    TAG206N_INFO_LOG("end\n");
    return size;
}

/**
 * @brief         Sysfs interface for set enable data.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr The structure of device attribute.
 * @param[in]     *buf Set enable data.
 * @param[in]     size Set data size.
 * @return        Write buffer size.
 * @retval        <=0 Normal end.
 * @retval        -EINVAL Parameter error.
 */
static ssize_t tag206n_set_enable(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    int err = 0;
    struct tag206n_data *data;
    bool new_enable;

    TAG206N_INFO_LOG("start");
    data  = dev_get_drvdata(dev);

    if ((data->suspended == 1) ||
        (data->status    == TAG206N_STATUS_ERROR) ||
        (data->status    == TAG206N_STATUS_INITIALIZE))
        return -EINVAL;

    if (sysfs_streq(buf, "1")) {
        new_enable = true;
    } else if (sysfs_streq(buf, "0")) {
        new_enable = false;
    } else {
        TAG206N_NOTICE_LOG("sysfs_streq() invalid data\n");
        return -EINVAL;
    }
    if (new_enable == data->enable) {
        TAG206N_INFO_LOG("There is no change.");
        return size;
    }
    data->enable               = new_enable;
    data->polling_elapsed_time = data->polling;
    tag206n__timer_start(data);
    TAG206N_NOTICE_LOG("PASS set enable(new_enable=%d)\n", new_enable);

    TAG206N_INFO_LOG("end");
    return err ? err : size;
}

/**
 * @brief         Sysfs interface for show status data.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr The structure of device attribute.
 * @param[out]    *buf Show status data.
 * @return        Read buffer size.
 */
static ssize_t tag206n_show_status(struct device *dev,
            struct device_attribute *attr, char *buf)
{
    struct tag206n_data *data;
    ssize_t size;

    TAG206N_INFO_LOG("start");
    data  = dev_get_drvdata(dev);
    if (data->suspended == 1) {
        size = sprintf(buf, "%02d\n", TAG206N_STATUS_SUSPEND);
    }else{
        size = sprintf(buf, "%02d\n",
                       (data->status == TAG206N_STATUS_ERROR ?
                        data->errdetail : data->status));
    }
    TAG206N_INFO_LOG("end\n");
    return size;
}

/**
 * @brief         Sysfs interface for show temperature data.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr The structure of device attribute.
 * @param[out]    *buf Show temperature data.
 * @return        Read buffer size.
 */
static ssize_t tag206n_show_temperature(struct device *dev,
            struct device_attribute *attr, char *buf)
{
    struct tag206n_data *data;
    ssize_t size;
    int ret = 0;
    int count;

    TAG206N_INFO_LOG("start");
    data  = dev_get_drvdata(dev);

    if ((data->suspended == 1) ||
        (data->status    == TAG206N_STATUS_ERROR) ||
        (data->status    == TAG206N_STATUS_INITIALIZE))
        return -EINVAL;

    for (count = 0; count < TAG206N_FAIL_RETRY_NUM; count++) {
        u8  stat;
        ret = tag206n_check_status_register(data, &stat, 1);
        if (ret) {
            continue;
        }
        ret = tag206n_read_data_values(data,
                                       TAG206N_REG_TEMP_HI,
                                       &data->data.temperature);
        if (ret) {
            continue;
        }
        break;
    }
    if (ret) {
        return ret;
    }
    size = sprintf(buf, "0x%04x\n", data->data.temperature);
    TAG206N_INFO_LOG("end\n");
    return size;
}

/**
 * @brief         Sysfs interface for show polling data.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr The structure of device attribute.
 * @param[out]    *buf Show polling data.
 * @return        Read buffer size.
 */
static ssize_t tag206n_show_polling(struct device *dev,
            struct device_attribute *attr, char *buf)
{
    struct tag206n_data *data;
    u64 delay;
    ssize_t size;

    TAG206N_INFO_LOG("start");
    data  = dev_get_drvdata(dev);
    delay = data->polling;
    size  = sprintf(buf, "%lld\n", delay);
    TAG206N_INFO_LOG("end\n");
    return size;
}

/**
 * @brief         Sysfs interface for set polling data.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr The structure of device attribute.
 * @param[in]     *buf Set polling data.
 * @param[in]     size Set data size.
 * @return        Write buffer size.
 * @retval        <=0 Normal end.
 * @retval        -EINVAL Parameter error.
 */
static ssize_t tag206n_set_polling(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    struct tag206n_data *data;
    int res = 0;
    u64 delay_ns;
    u64 delay_ns_before;

    TAG206N_INFO_LOG("start");

    data  = dev_get_drvdata(dev);

    if ((data->suspended == 1) ||
        (data->status    == TAG206N_STATUS_ERROR) ||
        (data->status    == TAG206N_STATUS_INITIALIZE))
        return -EINVAL;

    res = strict_strtoll(buf, 10, &delay_ns);
    if (res < 0) {
        TAG206N_NOTICE_LOG("polling invalid data.\n");
        return res;
    }
    delay_ns_before = data->polling;
    if (delay_ns < TAG206N_POLLING_MIN) {
        TAG206N_NOTICE_LOG("polling data is little.");
        data->polling = TAG206N_POLLING_DEF_INTERVAL;
    } else if (delay_ns > TAG206N_POLLING_MAX){
        TAG206N_NOTICE_LOG("polling data is large.");
        data->polling = TAG206N_POLLING_MAX;
    } else {
        data->polling = delay_ns;
    }
    if (data->polling != delay_ns_before) {
        data->polling_elapsed_time = data->polling;
        tag206n__timer_start(data);
    }
    TAG206N_INFO_LOG("end");
    return size;
}

/**
 * @brief         Sysfs interface for show driver information.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr The structure of device attribute.
 * @param[out]    *buf Show temperature data.
 * @return        Read buffer size.
 */
static ssize_t tag206n_show_gyro_info(struct device *dev,
            struct device_attribute *attr, char *buf)
{
    struct tag206n_data *data;
    ssize_t size;
    int tmp_status;

    TAG206N_INFO_LOG("start");
    data  = dev_get_drvdata(dev);

    if (data->suspended == 1) {
        tmp_status =  TAG206N_STATUS_SUSPEND;
    }else{
        tmp_status = (data->status == TAG206N_STATUS_ERROR ?
            data->errdetail : data->status);
    }

    size = sprintf(buf,
        "driver status %d\nget Gyro data count 0x%x\n"
        "get Gyro data time 0x%lx\n",
        tmp_status, data->get_data_count,
	data->get_data_tv.tv_sec);

    TAG206N_INFO_LOG("end\n");
    return size;
}

#ifdef TAG206N_DEBUG_SYSFS
static ssize_t tag206n_test_sysfs_reinit(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf, size_t size);
static ssize_t tag206n_test_sysfs_suspend(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf, size_t size);
/* sysfs(debug) */
static DEVICE_ATTR(test_sysfs_reinit, S_IRUGO | S_IWUSR | S_IWGRP,
            NULL, tag206n_test_sysfs_reinit);
static DEVICE_ATTR(test_sysfs_suspend, S_IRUGO | S_IWUSR | S_IWGRP,
            NULL, tag206n_test_sysfs_suspend);

/**
 * @brief         Debug sysfs interface initialize function.
 * @param[in]     *data The structure of Gyro sensor driver information.
 * @return        None.
 */
static void tag206n_debug_sysfs_init(struct tag206n_data *data)
{
    int err;
    err = device_create_file(&(data->input_dev)->dev, &dev_attr_test_sysfs_reinit);
    err = device_create_file(&(data->input_dev)->dev, &dev_attr_test_sysfs_suspend);
    return;
}

/**
 * @brief         Debug sysfs interface exit function.
 * @param[in]     *data The structure of Gyro sensor driver information.
 * @return        None.
 */
static void tag206n_debug_sysfs_exit(struct tag206n_data *data)
{
    device_remove_file(&data->client->dev, &dev_attr_test_sysfs_reinit);
    device_remove_file(&data->client->dev, &dev_attr_test_sysfs_suspend);
    return;
}
#endif

/**
 * @brief         Read short(2byte) eMMC data for Nonvolatile driver.
 * @param[in]     addr Read eMMC data address offset.
 * @return        Read data.
 */
static unsigned short tag206n_nonvolatile_short_read(unsigned int addr)
{
    int ret;
    unsigned char  data[2];
    unsigned short nv_output;

    ret = GetNONVOLATILE(data, addr, sizeof(short));
    if (ret) {
        return 0xffff;
    }

    nv_output = (data[1] << 8) | data[0];
    return nv_output;
}

/**
 * @brief         Read long(4byte) eMMC data for Nonvolatile driver.
 * @param[in]     addr Read eMMC data address offset.
 * @return        Read data.
 */
static unsigned long tag206n_nonvolatile_long_read(unsigned int addr)
{
    int ret;
    unsigned char  data[4];
    unsigned long  nv_output;

    ret = GetNONVOLATILE(data, addr, sizeof(long));
    if (ret) {
        return 0xffffffff;
    }

    nv_output = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
    return nv_output;
}

/**
 * @brief         Read driver parameter for Nonvolatile driver.
 * @param[in,out] *data The structure of Gyro sensor driver information.
 * @return        None.
 */
static int tag206n_system_default_value_update(struct tag206n_data *data)
{
    data->interval_polling      = tag206n_nonvolatile_long_read(TAG206N_NVOFFSET_POLLING_DEF_INTERVAL);
    if ( ( data->interval_polling == 0xffffffff )          ||
         ( data->interval_polling >  TAG206N_POLLING_MAX ) ||
         ( data->interval_polling <  TAG206N_POLLING_MIN ) ) {
        TAG206N_WARNING_LOG("POLLING_DEF_INTERVAL nvdata error. default value set. %08x\n",
                            ( unsigned int )data->interval_polling);
        data->interval_polling = TAG206N_POLLING_DEF_INTERVAL;
    }
    TAG206N_NOTICE_LOG("POLLING_DEF_INTERVAL = %u\n", ( unsigned int )data->interval_polling);

    data->interval_health_check = tag206n_nonvolatile_long_read(TAG206N_NVOFFSET_HEALTH_CHECK_INTERVAL);
    if ( ( data->interval_health_check == 0xffffffff) ||
         ( data->interval_health_check >  TAG206N_POLLING_HEALTH_CHECK_MAX) ||
         ( data->interval_health_check <  TAG206N_POLLING_MIN) ){
        TAG206N_WARNING_LOG("POLLING_1S nvdata error. default value set. %08x\n",
                            ( unsigned int )data->interval_health_check);
        data->interval_health_check = TAG206N_POLLING_1S;
    }
    TAG206N_NOTICE_LOG("POLLING_1S = %u\n", ( unsigned int )data->interval_health_check);

    data->interval_unlock_protect = tag206n_nonvolatile_short_read(TAG206N_NVOFFSET_UNLOCK_PROTECT_WAIT);
    if ( ( data->interval_unlock_protect == 0xffff) ||
         ( data->interval_unlock_protect >  TAG206N_WAIT_TIME_MAX) ||
         ( data->interval_unlock_protect <  TAG206N_WAIT_TIME_MIN) ){
        TAG206N_WARNING_LOG("WRITE_PROTECT_REG_CANCEL_WAIT_TIME nvdata error. default value set. %08x\n",
                            ( unsigned int )data->interval_unlock_protect);
        data->interval_unlock_protect = TAG206N_WRITE_PROTECT_REG_CANCEL_WAIT_TIME;
    }
    TAG206N_NOTICE_LOG("WRITE_PROTECT_REG_CANCEL_WAIT_TIME = %u\n", ( unsigned int )data->interval_unlock_protect);

    data->interval_status_update = tag206n_nonvolatile_short_read(TAG206N_NVOFFSET_STAT_UPDATE_WAIT);
    if ( ( data->interval_status_update == 0xffff) ||
         ( data->interval_status_update >  TAG206N_WAIT_TIME_MAX) ||
         ( data->interval_status_update <  TAG206N_WAIT_TIME_MIN) ) {
        TAG206N_WARNING_LOG("STATUS_COUNT_REG_UPDATE_WAIT_TIME nvdata error. default value set. %08x\n",
                            ( unsigned int )data->interval_status_update);
        data->interval_status_update = TAG206N_STATUS_COUNT_REG_UPDATE_WAIT_TIME;
    }
    TAG206N_NOTICE_LOG("STATUS_COUNT_REG_UPDATE_WAIT_TIME = %u\n", ( unsigned int )data->interval_status_update);

    data->interval_system_reset = tag206n_nonvolatile_short_read(TAG206N_NVOFFSET_SYSTEM_RESET_WAIT);
    if ( ( data->interval_system_reset == 0xffff) ||
         ( data->interval_system_reset >  TAG206N_WAIT_TIME_MAX) ||
         ( data->interval_system_reset <  TAG206N_WAIT_TIME_MIN) ) {
        TAG206N_WARNING_LOG("SENSOR_SYSTEM_RESET_WAIT_TIME nvdata error. default value set. %08x\n",
                            ( unsigned int )data->interval_system_reset);
        data->interval_system_reset = TAG206N_SENSOR_SYSTEM_RESET_WAIT_TIME;
    }
    TAG206N_NOTICE_LOG("POLLING_DEF_INTERVALSENSOR_SYSTEM_RESET_WAIT_TIME = %u\n", ( unsigned int )data->interval_system_reset);

    return 0;
}




/* sysfs */
static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
            tag206n_show_enable, tag206n_set_enable);
static DEVICE_ATTR(temperature, S_IRUGO | S_IWUSR | S_IWGRP,
            tag206n_show_temperature, NULL);
static DEVICE_ATTR(status, S_IRUGO | S_IWUSR | S_IWGRP,
            tag206n_show_status, NULL);
static DEVICE_ATTR(polling, S_IRUGO | S_IWUSR | S_IWGRP,
            tag206n_show_polling,  tag206n_set_polling);
static DEVICE_ATTR(gyro_info, S_IRUGO,
            tag206n_show_gyro_info,  NULL );

/**
 * @brief         Suspend handler function.
 * @param[in]     *dev Device information structure.
 * @return        Result.
 * @retval        0 Normal end.
 */
static int tag206n_suspend(struct device *dev)
{
    struct tag206n_data *data;
    struct i2c_client *client;
    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);
    data->suspended = 1;

    return 0;
}

/**
 * @brief         Fastboot flag down function.
 * @param[in]     *flag Fastboot flag of system global variable.
 * @param[in]     *param Not use.
 * @return        Result.
 * @retval        0 Normal end.
 */
static int tag206n_resumeflag_atomic_callback(bool flag, void* param)
{
    struct tag206n_data *data = (struct tag206n_data *) param;

    if ( flag == false ) {
        data->suspended = 0;
    }
    return 0;
}

/**
 * @brief         Reinit handler function.
 * @param[in]     *dev Device information structure.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        !=0 Error end.
 */
static int tag206n_re_init(struct device *dev)
{
    int ret = 0;
    struct i2c_client *client;
    struct tag206n_data *data;

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);

    tag206n__timer_stop(data);
    cancel_work_sync(&data->work);

    early_device_invoke_with_flag_irqsave(EARLY_DEVICE_BUDET_INTERRUPTION_MASKED,
                                          tag206n_resumeflag_atomic_callback,
                                          data);

    if (data->suspended == 1) {
        TAG206N_NOTICE_LOG("re_init called in bu-det\n");
        return 0;
    }

    mutex_lock(&(data->input_dev)->mutex);
    data->status = TAG206N_STATUS_INITIALIZE;
    data->failsafe_counter = 0;
    mutex_unlock(&(data->input_dev)->mutex);

    ret = tag206n_reset_device(data, false);
    if (ret) {
        data->status = TAG206N_STATUS_ERROR;
        TAG206N_NOTICE_LOG("status change to ERROR\n");
        TAG206N_DRC_TRC( "status change to ERROR\n" );
        TAG206N_ERR_LOG("tag206n(GYRO) not found\n");
        return 0;
    }
    TAG206N_NOTICE_LOG("status change to INITIALIZE\n");
    TAG206N_DRC_TRC( "status change to INITIALIZE\n" );
    tag206n__timer_start(data);
    TAG206N_NOTICE_LOG("re_init success\n");
    return ret;
}

/**
 * @brief         Gyro sensor driver probe function.
 * @param[in]     *client Gyro sensor driver use I2C information.
 * @param[in]     *devid Not use.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -ENOMEM No memory.
 */
/*** The callback function from an I2C core driver ***/
static int tag206n_probe(struct i2c_client *client,
                   const struct i2c_device_id *devid)
{
    int err = 0;
    struct tag206n_data *data;

    TAG206N_INFO_LOG("start");

    data = kzalloc(sizeof(*data), GFP_KERNEL);
    if (data == NULL) {
        TAG206N_ERR_LOG("failed to allocate memory for module data\n");
        err = -ENOMEM;
        return err;
    }
    TAG206N_DEBUG_LOG("kzalloc data=%p", data);
    data->status = TAG206N_STATUS_INITIALIZE;
    data->suspended = 0;
    data->client = client;
    data->get_data_count = 0;
    memset( &data->get_data_tv, 0, sizeof( struct timeval ) );
    tag206n_system_default_value_update(data);

    data->edev.dev                    = &client->dev;
    data->edev.reinit                 = tag206n_re_init;
    data->edev.sudden_device_poweroff = NULL;
    err = early_device_register(&data->edev);
    if ( err < 0 ){
        goto err_alloc_device;
    }

    mutex_init(&data->lock);
    /* allocate input_device */
    data->input_dev = input_allocate_device();
    if (!data->input_dev) {
        TAG206N_ERR_LOG("input_allocate_device() failed\n");
        err = -ENOMEM;
        goto err_early_device_register;
    }

    input_set_drvdata(data->input_dev, data);
    (data->input_dev)->name = TAG206N_DRIVER_NAME;
    /* Gyro */
    input_set_capability(data->input_dev, EV_REL, REL_Z);
    input_set_abs_params(data->input_dev, REL_Z, 0, 16383, 0, 0);
#if 0
    input_set_capability(data->input_dev, EV_REL, REL_Y);
    input_set_abs_params(data->input_dev, REL_Y, 0, 16383, 0, 0);
#endif
    /* register input_device */
    err = input_register_device(data->input_dev);
    if (err < 0) {
        TAG206N_ERR_LOG("input_register_device() failed\n");
        input_free_device(data->input_dev);
        goto err_input_register_device;
    }

    hrtimer_init(&data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    data->timer.function = tag206n_timer_func;
    
    data->tag206n_wq = create_singlethread_workqueue("tag206n");
    if (!data->tag206n_wq) {
        err = -ENOMEM;
        TAG206N_ERR_LOG("create_singlethread_workqueue() failed.\n");
        goto err_create_workqueue;
    }

    INIT_WORK(&data->work, tag206n_work_func);

    i2c_set_clientdata(client, data);
    dev_set_drvdata(&(data->input_dev)->dev, data);

/* sysfs make */
    err = device_create_file(&(data->input_dev)->dev, &dev_attr_enable);
    if (err  < 0) {
        TAG206N_ERR_LOG("device_create_file() failed(enable).");
        goto err_device_create_file;
    }
    err = device_create_file(&(data->input_dev)->dev, &dev_attr_status);
    if (err  < 0) {
        TAG206N_ERR_LOG("device_create_file() failed(status).\n");
        goto err_device_create_file2;
    }
    err = device_create_file(&(data->input_dev)->dev, &dev_attr_polling);
    if (err  < 0) {
        TAG206N_ERR_LOG("device_create_file() failed(polling).\n");
        goto err_device_create_file3;
    }
    err = device_create_file(&(data->input_dev)->dev, &dev_attr_temperature);
    if (err  < 0) {
        TAG206N_ERR_LOG("device_create_file() failed(temperature).\n");
        goto err_device_create_file4;
    }
    err = device_create_file(&(data->input_dev)->dev, &dev_attr_gyro_info);
    if (err  < 0) {
        TAG206N_ERR_LOG("device_create_file() failed(gyro_info).\n");
        goto err_device_create_file5;
    }
#ifdef TAG206N_DEBUG_SYSFS
    tag206n_debug_sysfs_init(data);
#endif

    data->enable  = false;
    data->polling = data->interval_polling;
    err = tag206n_reset_device(data, false);
    if (err) {
        data->status = TAG206N_STATUS_ERROR;
        TAG206N_ERR_LOG("tag206n(GYRO) not found\n");
        return 0;
    }
    tag206n__timer_start(data);

    TAG206N_DRC_TRC( "Gyro initialized\n" );
    TAG206N_INFO_LOG("end");
    return 0;

err_device_create_file5:
    device_remove_file(&(data->input_dev)->dev, &dev_attr_temperature);
err_device_create_file4:
    device_remove_file(&(data->input_dev)->dev, &dev_attr_polling);
err_device_create_file3:
    device_remove_file(&(data->input_dev)->dev, &dev_attr_status);
err_device_create_file2:
    device_remove_file(&(data->input_dev)->dev, &dev_attr_enable);
err_device_create_file:
    destroy_workqueue(data->tag206n_wq);
    input_unregister_device(data->input_dev);
err_create_workqueue:
err_input_register_device:
err_early_device_register:
    early_device_unregister(&data->edev);
err_alloc_device:
    kfree(data);
    TAG206N_INFO_LOG("error end");
    return err;
}

/**
 * @brief         Gyro sensor driver remove function.
 * @param[in]     *client Gyro sensor driver use I2C information.
 * @return        Result.
 * @retval        0 Normal end.
 */
static int tag206n_remove(struct i2c_client *client)
{
    int ret = 0;
    struct tag206n_data *data;

    TAG206N_INFO_LOG("start");

    data = i2c_get_clientdata(client);
    tag206n__timer_stop(data);
    cancel_work_sync(&data->work);
    destroy_workqueue(data->tag206n_wq);
    device_remove_file(&(data->input_dev)->dev, &dev_attr_enable);
    device_remove_file(&(data->input_dev)->dev, &dev_attr_status);
    device_remove_file(&(data->input_dev)->dev, &dev_attr_polling);
    device_remove_file(&(data->input_dev)->dev, &dev_attr_temperature);
    device_remove_file(&(data->input_dev)->dev, &dev_attr_gyro_info);
#ifdef TAG206N_DEBUG_SYSFS
    tag206n_debug_sysfs_exit(data);
#endif

    early_device_unregister(&data->edev);
    input_unregister_device(data->input_dev);
    mutex_destroy(&data->lock);
    kfree(data);

    TAG206N_INFO_LOG("end");

    return ret;
}

#ifdef TAG206N_DEBUG_SYSFS
/**
 * @brief         Sysfs interface for debug suspend execute.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr The structure of device attribute.
 * @param[in]     *buf Not use.
 * @param[in]     size Set data size.
 * @return        Write buffer size.
 * @retval        <=0 Normal end.
 * @retval        >0 Error end.
 */
static ssize_t tag206n_test_sysfs_suspend(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf, size_t size)
{
    tag206n_suspend(dev);
    return size;
}
/**
 * @brief         Sysfs interface for debug reinit execute.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr The structure of device attribute.
 * @param[in]     *buf Not use.
 * @param[in]     size Set data size.
 * @return        Write buffer size.
 * @retval        <=0 Normal end.
 * @retval        >0 Error end.
 */
static ssize_t tag206n_test_sysfs_reinit(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf, size_t size)
{
    tag206n_re_init(dev);
    return size;
}
#endif

static const struct i2c_device_id tag206n_id[] = {
    { "tag206n", 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, tag206n_id);

static const struct dev_pm_ops tag206n_pm_ops = {
    .suspend    = tag206n_suspend,
};

static struct i2c_driver tag206n_driver = {
    .probe   = tag206n_probe,
    .remove  = __devexit_p(tag206n_remove),
    .id_table = tag206n_id,
    .driver = {
        .owner = THIS_MODULE,
        .name  = "tag206n",
        .pm    = &tag206n_pm_ops,
    },
};

/**
 * @brief         Gyro sensor driver initialize function.
 * @param         None.
 * @return        i2c_add_driver result data.
 */
static int __init tag206n_init(void)
{
    TAG206N_INFO_LOG("start");
    return i2c_add_driver(&tag206n_driver);
}

/**
 * @brief         Gyro sensor driver exit function.
 * @param         None.
 * @return        None.
 */
static void __exit tag206n_exit(void)
{
    TAG206N_INFO_LOG("start");
    i2c_del_driver(&tag206n_driver);
}

module_init(tag206n_init);
module_exit(tag206n_exit);

MODULE_DESCRIPTION("TAG206N0001 digital gyroscope driver");
MODULE_AUTHOR("");
MODULE_LICENSE("GPL");
