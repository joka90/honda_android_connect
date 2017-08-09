/*
 * linux/drivers/input/misc/lis331dlh_priv.h
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
#ifndef LIS331DLH_INSIDE_HEADER
#define LIS331DLH_INSIDE_HEADER

#include <linux/i2c/lis331dlh.h>

#define LIS331DLH_DRIVER_NAME   "lis331dlh"
#define LIS331DLH_EVENT_NAME    "accelerometer"

/* lis331dlh chip id */
#define LIS331DLH_DEVICE_ID   0x32

/* lis331dlh gyroscope registers */
#define LIS331DLH_WHO_AM_I       0x0F
#define LIS331DLH_CTRL_REG1      0x20 /* PowerMode(PM2:PM1:PM0), DataRate(DR1:DR0), Zenable(Zen), Yenable(Yen), Xenable(Xen) */
#define LIS331DLH_CTRL_REG2      0x21 /* BOOT, Hipassmode(HPM1:HPM0), FilterDataSel(FDS), HPen2, HPen1, CutOfFrq(HPCF1:HPCF0) */
#define LIS331DLH_CTRL_REG3      0x22 /* Int active Hi/Lo(IHL), Push-pullOD(PP_OD), Latch int Req INT2_SRC(LIR2), */
                                      /*   Data signal on INT2 pad control(I2CFG1:I2CFG0), Latch int Req INT1_SRC(LIR1), */
                                      /*   Data signal on INT1 pad control(I1CFG1:I1CFG0) */
#define LIS331DLH_CTRL_REG4      0x23 /* BlockDataUpdate(BDU), BigLittleEndian(BLE), FullScale(FS1:FS0), 0, */
                                      /*   SelfTestsign(STsign), SPI serial interface selection(SIM) */
#define LIS331DLH_CTRL_REG5      0x24 /* 0, 0, 0, 0, 0, 0, Turn-on mode selection (TurnOn1:TurnOn0) */
                                  /*   high pass-filter. */
#define LIS331DLH_REG_REFERENCE  0x26 /* This reg. sets the acceleration value taken as a reference for the High pass output. */
#define LIS331DLH_REG_STATUS     0x27 /* ZYXOR, ZOR, YOR, XOR, ZYXDA, ZDA, YDA, ZDA */
#define LIS331DLH_REG_OUT_X_L    0x28 /* X-axis Lo data (2's complement) */
#define LIS331DLH_REG_OUT_X_H    0x29 /* X-axis Hi data (2's complement) */
#define LIS331DLH_REG_OUT_Y_L    0x2A /* Y-axis Lo data (2's complement) */
#define LIS331DLH_REG_OUT_Y_H    0x2B /* Y-axis Hi data (2's complement) */
#define LIS331DLH_REG_OUT_Z_L    0x2C /* Z-axis Lo data (2's complement) */
#define LIS331DLH_REG_OUT_Z_H    0x2D /* Z-axis Hi data (2's complement) */

/* fix data */
#define LIS331DLH_REG1_DATA   0x27
#define LIS331DLH_REG4_DATA   0x80

/* polling parameter */
#define LIS331DLH_POLLING_MIN          (  20 * 1000 * 1000)     /* ns */
#define LIS331DLH_POLLING_MAX          (2000 * 1000 * 1000)     /* ns */
#define LIS331DLH_DEF_POLLING_INTERVAL LIS331DLH_POLLING_MIN

/* wait time */
#define PRE_REGISTER_DUMP_WAIT_TIME     20      /* ms */
#define POST_REGISTER_DUMP_WAIT_TIME    100     /* ms */
#define REG_INIT_WAIT_TIME  10

/* lis331dlh retry */
#define LIS331DLH_FAIL_RETRY_NUM  2

/* lis331dlh status */
#define LIS331DLH_STATUS_INITIALIZE  10
#define LIS331DLH_STATUS_DISABLE     20
#define LIS331DLH_STATUS_ENABLE      30
#define LIS331DLH_STATUS_SUSPEND     40
#define LIS331DLH_STATUS_ERROR       60

#define LIS331DLH_STATUS_ERR1        61 /* device failed    */
#define LIS331DLH_STATUS_ERR2        62 /* i2cdevice failed */
/* lis331dlh REG size */
#define LIS331DLH_INIT_REG_SIZE   2
#define LIS331DLH_DATA_REG_SIZE   8

/* for debug */
#define LIS331DLH_DEBUG_SYSFS
#define LIS331DLH_DEBUG_MODE
/* test mode */
#if 0
#define LIS331DLH_TEST_KZALLOC_FAILED                   100
#define LIS331DLH_TEST_DEVICE_IDENTIFICATION_FAILED     101
#define LIS331DLH_TEST_ALLOC_INPUT_DEVICE_FAILED        102    /* allocate input_device */
#define LIS331DLH_TEST_INPUT_REGISTER_DEVICE_FAILED     103    /* register input_device */
#define LIS331DLH_TEST_CREATE_WORK_QUEUE_FAILED         104    /* cleate work queue */

#define LIS331DLH_TEST_CREATE_SYSFS1_FAILED             110     /* cleate sysfs */
#define LIS331DLH_TEST_CREATE_SYSFS2_FAILED             111     /* cleate sysfs */
#define LIS331DLH_TEST_CREATE_SYSFS3_FAILED             112     /* cleate sysfs */
#endif
#define LIS331DLH_TEST_I2C_READ_STATUS_REG_RETRY_FAILED         200
#define LIS331DLH_TEST_I2C_READ_STATUS_REG_RETRY_OK             201

#define LIS331DLH_TEST_I2C_READ_STATUS_REG_DATA_RETRY_FAILED    202
#define LIS331DLH_TEST_I2C_READ_STATUS_REG_DATA_RETRY_OK        203

#define LIS331DLH_TEST_I2C_READ_DATA_REG_RETRY_FAILED           204
#define LIS331DLH_TEST_I2C_READ_DATA_REG_RETRY_OK               205

#define LIS331DLH_TEST_INIT_DEVICE_RETRY_FAILED                 206
#define LIS331DLH_TEST_INIT_DEVICE_RETRY_OK                     207

#define LIS331DLH_TEST_I2C_READ_TRANSFER_RETRY_FAILED           208
#define LIS331DLH_TEST_I2C_READ_TRANSFER_RETRY_OK               209
#define LIS331DLH_TEST_I2C_WRITE_TRANSFER_RETRY_FAILED          210
#define LIS331DLH_TEST_I2C_WRITE_TRANSFER_RETRY_OK              211
/* Mark calculation & CRC */
#define MSB_INT16   0x8000
#define MSN_INT16   0xF000

/* accelerometer data readout */
#define BIT_ZYXDA      (1 << 3)    /* STATUS REG(ZYXDA) */
#define BIT_ZYXOR      (1 << 7)    /* STATUS REG(ZYXOR) */

#define LIS331DLH_DATA_LEN  (LIS331DLH_REG_OUT_Z_H - LIS331DLH_REG_STATUS) + 1
/*
 * LIS331DLH gyroscope data
 * brief structure containing gyroscope values for yaw, pitch and roll in
 * signed short
 */
struct lis331dlh_t {
    s16 x_axle;
    s16 y_axle;
    s16 z_axle;
};

struct lis331dlh_data {
    struct i2c_client *client;
    struct input_dev *input_dev;
    struct mutex lock;
    struct workqueue_struct *lis331dlh_wq;
    struct work_struct work;
    struct hrtimer timer;
    struct lis331dlh_t data;
    bool enable;
    s64  polling;
    int  status;
    int  suspended;
    int  errdetail;
    struct early_device edev;
};

#define TEGRA_I2C_TRANSFER_RETRY_MAX    2

#define LIS331DLH_EMERG_LOG( format, ...) { \
        printk( KERN_EMERG "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define LIS331DLH_ERR_LOG( format, ...) { \
        printk( KERN_ERR "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define LIS331DLH_WARNING_LOG( format, ...) { \
        printk( KERN_WARNING "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#if defined(CONFIG_LIS331DLH_LOG_NOTICE) || defined(CONFIG_LIS331DLH_LOG_INFO) \
        || defined(CONFIG_LIS331DLH_LOG_DEBUG)
#define LIS331DLH_NOTICE_LOG( format, ...) { \
        printk( KERN_NOTICE "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define LIS331DLH_NOTICE_LOG( format, ... )
#endif

#if defined(CONFIG_LIS331DLH_LOG_INFO) || defined(CONFIG_LIS331DLH_LOG_DEBUG)
#define LIS331DLH_INFO_LOG( format, ...) { \
        printk( KERN_INFO "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define LIS331DLH_INFO_LOG( format, ... )
#endif

#ifdef CONFIG_LIS331DLH_LOG_DEBUG
#define LIS331DLH_DEBUG_LOG( format, ...) { \
        printk( KERN_DEBUG "[%s:%d] %s " format, __FILE__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define LIS331DLH_DEBUG_LOG( format, ... )
#endif

#endif
