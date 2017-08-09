/*
 * drivers/input/misc/tag206n_priv.h
 *
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


#ifndef TAG206N_INSIDE_HEADER
#define TAG206N_INSIDE_HEADER

#include <linux/i2c/tag206n.h>
#define TAG206N_DRIVER_NAME     "gyro"

/* nv offset */
#define TAG206N_NVOFFSET_POLLING_DEF_INTERVAL   FTEN_NONVOL_GYRO_POLL_DEFAULT
#define TAG206N_NVOFFSET_HEALTH_CHECK_INTERVAL  FTEN_NONVOL_GYRO_POLL_HEALTH_CHECK
#define TAG206N_NVOFFSET_UNLOCK_PROTECT_WAIT    FTEN_NONVOL_GYRO_WAIT_UNLOCKWP
#define TAG206N_NVOFFSET_STAT_UPDATE_WAIT       FTEN_NONVOL_GYRO_WAIT_STAT_UPDATE
#define TAG206N_NVOFFSET_SYSTEM_RESET_WAIT      FTEN_NONVOL_GYRO_WAIT_SYSTEM_RESET


/* tag206n gyroscope registers */
#define TAG206N_REG_STATCNT     0x00    /* Status / Count */
#define TAG206N_REG_STATUS2     0x01    /* fault-diagnosis status in a sensor */
#define TAG206N_REG_GYRODATA_HI 0x02    /* Gyro data Hi */
#define TAG206N_REG_GYRODATA_LO 0x03    /* Gyro data Lo */
#define TAG206N_REG_GYROCRC     0x04    /* Gyro data CRC-7 */
#define TAG206N_REG_TEMP_HI     0x06    /* temperature Hi byte data */
#define TAG206N_REG_TEMP_LO     0x07    /* temperature Lo byte data */
#define TAG206N_REG_TEMPCRC     0x08    /* temperature CRC-7 */
#define TAG206N_REG_WPROTECT    0x10    /* write protect */ 
#define TAG206N_REG_SYSRESET    0x11    /* system reset */

#define BIT_AC      (1 << 6)    /* register alive-count bit */
#define BIT_STS     (1 << 2)    /* status-register fault diagnosis bit */
#define BIT_STA8    (1 << 1)    /* status-register STA8 bit */
#define BIT_STA7    (1 << 0)    /* status-register STA7 bit */

#define TAG206N_DATA_REG_SIZE  3
#define TAG206N_DATA_Z_SIZE    2
#define TAG206N_DATA_TEMPERATER_SIZE  TAG206N_DATA_Z_SIZE
#define TAG206N_GYRO_MASK      0x7f

/* fix data */
#define TAG206N_WPROTECT_DATA       0x08
#define TAG206N_RELEASE_CHECK_DATA  0x0804

/* i2c read parameter */
#define TAG206N_READ_TO_GYRODATA            0
#define TAG206N_READ_TO_TEMPDATA            1

/* polling parameter */
#define TAG206N_POLLING_MIN                  (  10 * 1000 * 1000UL)  /* ns */
#define TAG206N_POLLING_MAX                  (2000 * 1000 * 1000UL)  /* ns */
#define TAG206N_POLLING_HEALTH_CHECK_MAX     (4000 * 1000 * 1000UL)/* ns */
#define TAG206N_POLLING_DEF_INTERVAL TAG206N_POLLING_MIN
#define TAG206N_POLLING_1S                   (1000 * 1000 * 1000UL)  /* ns */

/* wait time */
#define TAG206N_WRITE_PROTECT_REG_CANCEL_WAIT_TIME  5   /* ms */
#define TAG206N_STATUS_COUNT_REG_UPDATE_WAIT_TIME   5   /* ms */
#define TAG206N_SENSOR_SYSTEM_RESET_WAIT_TIME       1   /* 1sec */
#define TAG206N_PRE_REGISTER_DUMP_WAIT_TIME        20   /* ms */
#define TAG206N_WAIT_TIME_MIN                       1
#define TAG206N_WAIT_TIME_MAX                   10000


/* tag206n retry */
#define TAG206N_FAIL_RETRY_NUM   2
#define TAG206N_RESET_RETRY_NUM  2

/* tag206n status */
#define TAG206N_STATUS_INITIALIZE 10
#define TAG206N_STATUS_DISABLE    20
#define TAG206N_STATUS_ENABLE     30
#define TAG206N_STATUS_SUSPEND    40
#define TAG206N_STATUS_ERROR      60

#define TAG206N_STATUS_ERR1       61 /* device failed    */
#define TAG206N_STATUS_ERR2       62 /* i2cdevice failed */

#define TAG206N_CRC_INIT    0x7f

#define TAG206N_LITES_FAILSAFE_ERROR 0x8000
#define TAG206N_ERROR_INFO_RETRY_OUT 0x00
#define TAG206N_ERROR_INFO_RESET     0x01

/*
 * tag206n gyroscope data
 * brief structure containing gyroscope values for yaw, pitch and roll in
 * signed short
 */
struct tag206n_t {
    s16 z_axle;
    s16 temperature;
};

struct tag206n_data {
    struct i2c_client *client;
    struct input_dev *input_dev;

    struct mutex lock;
    struct workqueue_struct *tag206n_wq;
    struct work_struct work;
    struct hrtimer timer;
    struct tag206n_t data;

    int  status;
    int  suspended;
    int  errdetail;

    bool enable;
    s64  polling;
    s64  polling_elapsed_time;
    s64  health_check_elapsed_time;
    s64  polling_delta;
    struct timeval elapsed_tv;

    int failsafe_counter;
    int init_check_flag;

    unsigned long interval_polling;
    unsigned long interval_health_check;
    unsigned long interval_unlock_protect;
    unsigned long interval_status_update;
    unsigned long interval_system_reset;

    struct early_device edev;

    u32 get_data_count;
    struct timeval get_data_tv;
    
};

#define TEGRA_I2C_TRANSFER_RETRY_MAX    2

#define __file__ (strrchr(__FILE__,'/' ) + 1 )

#define TAG206N_EMERG_LOG( format, ...) { \
        printk( KERN_EMERG "[%s:%d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define TAG206N_ERR_LOG( format, ...) { \
        printk( KERN_ERR "[%s:%d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define TAG206N_WARNING_LOG( format, ...) { \
        printk( KERN_WARNING "[%s:%d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#if defined(CONFIG_TAG206N_LOG_NOTICE) || defined(CONFIG_TAG206N_LOG_INFO) \
        || defined(CONFIG_TAG206N_LOG_DEBUG)
#define TAG206N_NOTICE_LOG( format, ...) { \
        printk( KERN_NOTICE "[%s:%d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define TAG206N_NOTICE_LOG( format, ... )
#endif

#if defined(CONFIG_TAG206N_LOG_INFO) || defined(CONFIG_TAG206N_LOG_DEBUG)
#define TAG206N_INFO_LOG( format, ...) { \
        printk( KERN_INFO "[%s:%d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define TAG206N_INFO_LOG( format, ... )
#endif

#ifdef CONFIG_TAG206N_LOG_DEBUG
#define TAG206N_DEBUG_LOG( format, ...) { \
        printk( KERN_DEBUG "[%s:%d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define TAG206N_DEBUG_LOG( format, ... )
#endif

/* for debug */
#define TAG206N_DEBUG_SYSFS
#define TAG206N_DEBUG_MODE
/* for debug(test mode) */
#endif
