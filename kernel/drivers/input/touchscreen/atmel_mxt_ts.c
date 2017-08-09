/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @file atmel_mxt_ts.c
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
/*
 * Atmel maXTouch Touchscreen driver
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Copyright (C) 2011 Atmel Corporation
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
/*
 *----------------------------------------------------------------------------
 * COPYRIGHT(C) FUJITSU TEN LIMITED 2012-2014
 *----------------------------------------------------------------------------
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
/* FUJITSU TEN:2012-08-06 include header file add. start */
#include <linux/spinlock.h>
#include <linux/earlydevice.h>
#include <linux/hrtimer.h>
/* FUJITSU TEN:2012-08-06 include header file add. end */
/* FUJITSU TEN:2012-10-23 nonvolatile driver access add. start */
#include <linux/nonvolatile.h>
/* FUJITSU TEN:2012-10-23 nonvolatile driver access add. end */
/* FUJITSU TEN:2013-02-18 lites include header add. start */
#include <linux/lites_trace_drv.h>
/* FUJITSU TEN:2013-02-18 lites include header add. end */
/* FUJITSU TEN:2013-06-07 debug function add. start */
#include <linux/time.h>
/* FUJITSU TEN:2013-06-07 debug function add. end */
/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. start */
#include <linux/wait.h>
/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. end */

/* Family ID */
#define MXT224_ID		0x80
#define MXT768E_ID		0xA1
#define MXT1386_ID		0xA0

/* Version */
#define MXT_VER_20		20
#define MXT_VER_21		21
#define MXT_VER_22		22

/* Firmware files */
#define MXT_FW_NAME		"maxtouch.fw"
#define MXT_CFG_NAME		"maxtouch.cfg"
#define MXT_CFG_MAGIC		"OBP_RAW V1"

/* Registers */
#define MXT_FAMILY_ID		0x00
#define MXT_VARIANT_ID		0x01
#define MXT_VERSION		0x02
#define MXT_BUILD		0x03
#define MXT_MATRIX_X_SIZE	0x04
#define MXT_MATRIX_Y_SIZE	0x05
#define MXT_OBJECT_NUM		0x06
#define MXT_OBJECT_START	0x07

#define MXT_OBJECT_SIZE		6

#define MXT_MAX_BLOCK_WRITE	256

/* Object types */
#define MXT_DEBUG_DIAGNOSTIC_T37	37
#define MXT_GEN_MESSAGE_T5		5
#define MXT_GEN_COMMAND_T6		6
#define MXT_GEN_POWER_T7		7
#define MXT_GEN_ACQUIRE_T8		8
#define MXT_GEN_DATASOURCE_T53		53
#define MXT_TOUCH_MULTI_T9		9
#define MXT_TOUCH_KEYARRAY_T15		15
#define MXT_TOUCH_PROXIMITY_T23		23
#define MXT_TOUCH_PROXKEY_T52		52
#define MXT_PROCI_GRIPFACE_T20		20
#define MXT_PROCG_NOISE_T22		22
#define MXT_PROCI_ONETOUCH_T24		24
#define MXT_PROCI_TWOTOUCH_T27		27
#define MXT_PROCI_GRIP_T40		40
#define MXT_PROCI_PALM_T41		41
#define MXT_PROCI_TOUCHSUPPRESSION_T42	42
#define MXT_PROCI_STYLUS_T47		47
#define MXT_PROCG_NOISESUPPRESSION_T48	48
#define MXT_SPT_COMMSCONFIG_T18		18
#define MXT_SPT_GPIOPWM_T19		19
#define MXT_SPT_SELFTEST_T25		25
#define MXT_SPT_CTECONFIG_T28		28
#define MXT_SPT_USERDATA_T38		38
#define MXT_SPT_DIGITIZER_T43		43
#define MXT_SPT_MESSAGECOUNT_T44	44
#define MXT_SPT_CTECONFIG_T46		46

/* MXT_GEN_MESSAGE_T5 object */
#define MXT_RPTID_NOMSG		0xff
#define MXT_MSG_MAX_SIZE	9

/* MXT_GEN_COMMAND_T6 field */
#define MXT_COMMAND_RESET	0
#define MXT_COMMAND_BACKUPNV	1
#define MXT_COMMAND_CALIBRATE	2
#define MXT_COMMAND_REPORTALL	3
#define MXT_COMMAND_DIAGNOSTIC	5

/* MXT_GEN_POWER_T7 field */
#define MXT_POWER_IDLEACQINT	0
#define MXT_POWER_ACTVACQINT	1
#define MXT_POWER_ACTV2IDLETO	2

#define MXT_POWER_CFG_RUN	0
#define MXT_POWER_CFG_DEEPSLEEP	1
/* FUJITSU TEN:2012-09-03 DEEPSLEEP mode setting for initialize add. start */
#define MXT_POWER_CFG_RUN_DATA	0xff
/* FUJITSU TEN:2012-09-03 DEEPSLEEP mode setting for initialize add. end */

/* MXT_GEN_ACQUIRE_T8 field */
#define MXT_ACQUIRE_CHRGTIME	0
#define MXT_ACQUIRE_TCHDRIFT	2
#define MXT_ACQUIRE_DRIFTST	3
#define MXT_ACQUIRE_TCHAUTOCAL	4
#define MXT_ACQUIRE_SYNC	5
#define MXT_ACQUIRE_ATCHCALST	6
#define MXT_ACQUIRE_ATCHCALSTHR	7

/* MXT_TOUCH_MULTI_T9 field */
#define MXT_TOUCH_CTRL		0
#define MXT_TOUCH_XORIGIN	1
#define MXT_TOUCH_YORIGIN	2
#define MXT_TOUCH_XSIZE		3
#define MXT_TOUCH_YSIZE		4
#define MXT_TOUCH_BLEN		6
#define MXT_TOUCH_TCHTHR	7
#define MXT_TOUCH_TCHDI		8
#define MXT_TOUCH_ORIENT	9
#define MXT_TOUCH_MOVHYSTI	11
#define MXT_TOUCH_MOVHYSTN	12
#define MXT_TOUCH_NUMTOUCH	14
#define MXT_TOUCH_MRGHYST	15
#define MXT_TOUCH_MRGTHR	16
#define MXT_TOUCH_AMPHYST	17
#define MXT_TOUCH_XRANGE_LSB	18
#define MXT_TOUCH_XRANGE_MSB	19
#define MXT_TOUCH_YRANGE_LSB	20
#define MXT_TOUCH_YRANGE_MSB	21
#define MXT_TOUCH_XLOCLIP	22
#define MXT_TOUCH_XHICLIP	23
#define MXT_TOUCH_YLOCLIP	24
#define MXT_TOUCH_YHICLIP	25
#define MXT_TOUCH_XEDGECTRL	26
#define MXT_TOUCH_XEDGEDIST	27
#define MXT_TOUCH_YEDGECTRL	28
#define MXT_TOUCH_YEDGEDIST	29
#define MXT_TOUCH_JUMPLIMIT	30

/* MXT_PROCI_GRIPFACE_T20 field */
#define MXT_GRIPFACE_CTRL	0
#define MXT_GRIPFACE_XLOGRIP	1
#define MXT_GRIPFACE_XHIGRIP	2
#define MXT_GRIPFACE_YLOGRIP	3
#define MXT_GRIPFACE_YHIGRIP	4
#define MXT_GRIPFACE_MAXTCHS	5
#define MXT_GRIPFACE_SZTHR1	7
#define MXT_GRIPFACE_SZTHR2	8
#define MXT_GRIPFACE_SHPTHR1	9
#define MXT_GRIPFACE_SHPTHR2	10
#define MXT_GRIPFACE_SUPEXTTO	11

/* MXT_PROCI_NOISE field */
#define MXT_NOISE_CTRL		0
#define MXT_NOISE_OUTFLEN	1
#define MXT_NOISE_GCAFUL_LSB	3
#define MXT_NOISE_GCAFUL_MSB	4
#define MXT_NOISE_GCAFLL_LSB	5
#define MXT_NOISE_GCAFLL_MSB	6
#define MXT_NOISE_ACTVGCAFVALID	7
#define MXT_NOISE_NOISETHR	8
#define MXT_NOISE_FREQHOPSCALE	10
#define MXT_NOISE_FREQ0		11
#define MXT_NOISE_FREQ1		12
#define MXT_NOISE_FREQ2		13
#define MXT_NOISE_FREQ3		14
#define MXT_NOISE_FREQ4		15
#define MXT_NOISE_IDLEGCAFVALID	16

/* MXT_SPT_COMMSCONFIG_T18 */
#define MXT_COMMS_CTRL		0
#define MXT_COMMS_CMD		1

/* MXT_SPT_CTECONFIG_T28 field */
#define MXT_CTE_CTRL		0
#define MXT_CTE_CMD		1
#define MXT_CTE_MODE		2
#define MXT_CTE_IDLEGCAFDEPTH	3
#define MXT_CTE_ACTVGCAFDEPTH	4
#define MXT_CTE_VOLTAGE		5

#define MXT_VOLTAGE_DEFAULT	2700000
#define MXT_VOLTAGE_STEP	10000

/* Define for MXT_GEN_COMMAND_T6 */
#define MXT_BOOT_VALUE		0xa5
#define MXT_RESET_VALUE		0x01
#define MXT_BACKUP_VALUE	0x55
/* FUJITSU TEN:2013-01-14 failsafe function for calibration add. start */
#define MXT_CALIBRATE_VALUE	0xFF
/* FUJITSU TEN:2013-01-14 failsafe function for calibration add. end */

/* FUJITSU TEN:2013-04-05 show config version IF(sysfs) add. start */
/* Define for MXT_SPT_USERDATA_T38 */
#define MXT_CONFIG_VERSION_SIZE	0x06
#define MXT_VERSION_DATA_0	0x30
#define MXT_VERSION_DATA_9	0x39
#define MXT_VERSION_DATA_A	0x41
#define MXT_VERSION_DATA_Z	0x5A
/* FUJITSU TEN:2013-04-05 show config version IF(sysfs) add. end */

/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. start */
/* Define for MXT_SPT_SELFTEST_T25 */
#define MXT_SELFTEST_RESULT_SIZE	0x03
#define MXT_SELFTEST_CTRL		0x00
#define MXT_SELFTEST_CMD		0x01

#define MXT_SELFTEST_CTRL_DISABLE	0x00
#define MXT_SELFTEST_CTRL_ENABLE	0x01
#define MXT_SELFTEST_CTRL_TESTEXEC	0x03
#define MXT_SELFTEST_CMD_NONE		0x00
#define MXT_SELFTEST_CMD_RUNTEST	0x17

#define MXT_SELFTEST_MESSAGE_STATUS	0x00
#define MXT_SELFTEST_MESSAGE_TYPE_NUM	0x01
#define MXT_SELFTEST_MESSAGE_TYPE_INST	0x02

#define MXT_SELFTEST_RETRY_COUNT	0x03
#define MXT_SELFTEST_WAIT_TIME		1000
#define MXT_SELFTEST_RETRY_COUNT_MIN	0x01
#define MXT_SELFTEST_WAIT_TIME_MIN	5
/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. end */

/* Delay times */
/* FUJITSU TEN:2012-08-10 backup time edit. start */
#define MXT_BACKUP_TIME		200	/* msec */
/* FUJITSU TEN:2012-08-10 backup time edit. end */
#define MXT224_RESET_TIME	65	/* msec */
#define MXT768E_RESET_TIME	250	/* msec */
#define MXT1386_RESET_TIME	200	/* msec */
#define MXT_RESET_TIME		200	/* msec */
#define MXT_RESET_NOCHGREAD	400	/* msec */
#define MXT_FWRESET_TIME	1000	/* msec */

/* FUJITSU TEN:2012-06-25 reset waittime add. start */
#define MXT540E_TSNS_RESET_TIME	150	/* msec */
#define MXT540E_RESET_TIME	280	/* msec */
/* FUJITSU TEN:2012-06-25 reset waittime add. end */

/* Command to unlock bootloader */
#define MXT_UNLOCK_CMD_MSB	0xaa
#define MXT_UNLOCK_CMD_LSB	0xdc

/* Bootloader mode status */
#define MXT_WAITING_BOOTLOAD_CMD	0xc0	/* valid 7 6 bit only */
#define MXT_WAITING_FRAME_DATA	0x80	/* valid 7 6 bit only */
#define MXT_FRAME_CRC_CHECK	0x02
#define MXT_FRAME_CRC_FAIL	0x03
#define MXT_FRAME_CRC_PASS	0x04
#define MXT_APP_CRC_FAIL	0x40	/* valid 7 8 bit only */
#define MXT_BOOT_STATUS_MASK	0x3f
#define MXT_BOOT_EXTENDED_ID	(1 << 5)
#define MXT_BOOT_ID_MASK	0x1f

/* Command process status */
#define MXT_STATUS_CFGERROR	(1 << 3)
/* FUJITSU TEN:2012-08-09 config error define add. start */
#define MXT_STATUS_SIGERROR	(1 << 5)
#define MXT_STATUS_COMSERROR	(1 << 2)
#define MXT_STATUS_ERROR	(MXT_STATUS_CFGERROR | MXT_STATUS_SIGERROR | \
				MXT_STATUS_COMSERROR )
/* FUJITSU TEN:2012-08-09 config error define add. end */

/* Touch status */
#define MXT_SUPPRESS		(1 << 1)
#define MXT_AMP			(1 << 2)
#define MXT_VECTOR		(1 << 3)
#define MXT_MOVE		(1 << 4)
#define MXT_RELEASE		(1 << 5)
#define MXT_PRESS		(1 << 6)
#define MXT_DETECT		(1 << 7)

/* Touch orient bits */
#define MXT_XY_SWITCH		(1 << 0)
#define MXT_X_INVERT		(1 << 1)
#define MXT_Y_INVERT		(1 << 2)

/* Touchscreen absolute values */
#define MXT_MAX_AREA		0xff

/* FUJITSU TEN:2012-07-31 MXT_MAX_FINGER edit. 10 -> 5 start */
#define MXT_MAX_FINGER		5
/* FUJITSU TEN:2012-07-31 MXT_MAX_FINGER edit. 10 -> 5 end */

/* FUJITSU TEN:2012-07-03 Orient data set for BB. start */
#define MXT_ORIENT_BB_VALUE	0x07
/* FUJITSU TEN:2012-07-03 Orient data set for BB. end */

/* FUJITSU TEN:2012-07-31 Update config define add. start */
#define MXT_CONFIG_DATA_SIZE	1000
#define MXT_ERROR_RETRY_MAX	3
#define MXT_RESET_RETRY_MAX	3
#define MXT_I2C_RETRY_MAX	3
#define MXT_ERROR_RETRY_WAIT	100	/* msec */
#define MXT_UPDATE_TIME		200	/* msec */
#define MXT_IRQ_TIMEOUT_COUNT	100
#define MXT_IRQ_CHECK_INTERVAL	20	/* msec */
#define MXT_READ_CONFIG_BYTE	2
#define MXT_CONFIG_HEADER_SIZE	3
#define MXT_UPDATE_MODE_WRITE	0
#define MXT_UPDATE_MODE_CHECK	1
#define MXT_UPDATE_DEBUG_LOG	256
/* FUJITSU TEN:2012-07-31 Update config define add. end */

/* FUJITSU TEN:2012-08-09 eMMC access define add. start */
#define MXT_EMMC_WRITE_START	0x00
#define MXT_EMMC_WRITE_END	0xAA
#define MXT_DEBUG_PRINT_SIZE	8
/* FUJITSU TEN:2012-08-09 eMMC access define add. end */

/* FUJITSU TEN:2012-08-09 message object read stop flag value add. start */
#define MXT_READ_STOP		1
#define MXT_FLAG_CLEAR		0
/* FUJITSU TEN:2012-08-09 message object read stop flag value add. end */

/* FUJITSU TEN:2012-10-10 hardware error polling add. start */
#define MXT_ERROR_POLLING_TIME	1000
#define MXT_ERROR_POLLING_COUNT	3
#define MXT_ERROR_POLLING_EXEC	1
#define MXT_ERROR_POLLING_STOP	0
/* FUJITSU TEN:2012-10-10 hardware error polling add. end */

/* FUJITSU TEN:2012-11-19 msleep macro add. start */
#define MXT_SLEEP_ONETIME_VALUE	10
/* FUJITSU TEN:2012-11-19 msleep macro add. end */

/* FUJITSU TEN:2012-12-07 eMMC MAX and MIN value add. start */
#define MXT_TSNS_RESET_TIME_MIN		50
#define MXT_ERROR_POLLING_TIME_MIN	100
#define MXT_ERROR_POLLING_COUNT_MIN	1
/* FUJITSU TEN:2012-12-07 eMMC MAX and MIN value add. end */

/* FUJITSU TEN:2013-02-07 TSNS_RESET exec time eMMC edit add. start */
#define MXT_TSNS_RESET_EXEC_TIME_MIN	5
/* FUJITSU TEN:2013-02-07 TSNS_RESET exec time eMMC edit add. end */

/* FUJITSU TEN:2013-02-18 lites macro add. start */
#define MXT_LITES_UPDATE_CONFIG_FMT	0x0000
#define MXT_LITES_ERROR_POLLING		0x0001
#define MXT_LITES_TSNS_RESET_FS		0x0002
#define MXT_LITES_TSNS_RESET_FE		0x8000
#define MXT_LITES_UPDATE_CONFIG_HARD	0x8001
#define MXT_LITES_EMMC_ACCESS		0x8002
#define MXT_UPDATE_CONFIG_HARD_WRITE	0x00
#define MXT_UPDATE_CONFIG_HARD_BACKUP	0x01
#define MXT_UPDATE_CONFIG_HARD_RESET	0x02
/* FUJITSU TEN:2013-02-18 lites macro add. end */
/* FUJITSU TEN:2013-06-07 debug function add. start */
#define MXT_LITES_TRACE_DATA_SIZE	256
/* FUJITSU TEN:2013-06-07 debug function add. end */

/* FUJITSU TEN:2013-07-09 drive-rec log add. start */
#define MXT_SINFO_LOG_INTERVAL_TIME	((60)*(10))
#define MXT_SINFO_LOG_INTERVAL_STIME	((60)*(1))
#define MXT_SINFO_LOG_MASK		0x10000
#define MXT_SINFO_LOG_GPIO_L		'L'
#define MXT_SINFO_LOG_GPIO_H		'H'
/* FUJITSU TEN:2013-07-09 drive-rec log add. end */

/* FUJITSU TEN:2013-01-14 failsafe function for calibration add. start */
#define MXT_CALIBRATE_INTERVAL_TIME	((30)*(1))
/* FUJITSU TEN:2013-01-14 failsafe function for calibration add. end */

/* FUJITSU TEN:2012-07-31 global variable add. start */
static spinlock_t	mxt_config_lock;
unsigned char		mxt_config_status;
unsigned char		mxt_read_stop;
/* FUJITSU TEN:2012-07-31 global variable add. end */

struct mxt_info {
	u8 family_id;
	u8 variant_id;
	u8 version;
	u8 build;
	u8 matrix_xsize;
	u8 matrix_ysize;
	u8 object_num;
};

/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxt_early_suspend(struct early_suspend *es);
static void mxt_late_resume(struct early_suspend *es);
#endif
#endif

struct mxt_object {
	u8 type;
	u16 start_address;
	u16 size;
	u16 instances;
	u8 num_report_ids;

	/* to map object and message */
	u8 min_reportid;
	u8 max_reportid;
};

struct mxt_message {
	u8 reportid;
	u8 message[MXT_MSG_MAX_SIZE - 2];
};

struct mxt_finger {
	int status;
	int x;
	int y;
	int area;
	int pressure;
};

enum mxt_device_state { INIT, APPMODE, BOOTLOADER, FAILED };

/* Each client has this additional data */
struct mxt_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	const struct mxt_platform_data *pdata;
	enum mxt_device_state state;
	struct mxt_object *object_table;
	u16 mem_size;
	struct mxt_info info;
	struct mxt_finger finger[MXT_MAX_FINGER];
	unsigned int irq;
	unsigned int max_x;
	unsigned int max_y;
	struct bin_attribute mem_access_attr;
	bool debug_enabled;
	bool driver_paused;
	/* FUJITSU TEN:2012-07-31 mxt_data member add. start */
	struct bin_attribute update_attr;
	/* FUJITSU TEN:2012-07-31 mxt_data member add. end */
	u8 bootloader_addr;
	u8 actv_cycle_time;
	u8 idle_cycle_time;
	u8 is_stopped;
	u8 max_reportid;
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
	/* FUJITSU TEN:2012-09-18 fastboot edit. start */
	struct early_device edev;
	u8 misc_count;
	/* FUJITSU TEN:2012-09-18 fastboot edit. end */
	/* FUJITSU TEN:2012-10-10 hardware error polling add. start */
	struct hrtimer timer;
	struct workqueue_struct *wq;
	struct work_struct work;
	u8 hwerror_count;
	u8 error_polling_flag;
	/* FUJITSU TEN:2012-10-10 hardware error polling add. end */
	/* FUJITSU TEN:2012-11-19 read parameter for eMMC. start */
	u16 tsns_reset_time;
	u64 hwerror_polling_time;
	u8 hwerror_count_max;
	u16 tsns_reset_exec_time;
	/* FUJITSU TEN:2012-11-19 read parameter for eMMC. end */
	/* FUJITSU TEN:2013-06-07 debug function add. start */
	u32 intr_count;
	struct timeval intr_tv;
	u32 touch_count;
	struct timeval touch_tv;
	/* FUJITSU TEN:2013-06-07 debug function add. end */
	/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. start */
	u16 test_wait_time;
	u8 test_retry_count;
	wait_queue_head_t wait;
	u8 test_flag;
	u8 test_data[MXT_SELFTEST_RESULT_SIZE]; 
	/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. end */
	/* FUJITSU TEN:2013-07-09 drive-rec log add. start */
#ifdef CONFIG_TOUCH_STATISTICAL_INFO_LOG
	u32 slog_count;
	u32 last_intr_count;
	u32 last_touch_count;
#endif
	/* FUJITSU TEN:2013-07-09 drive-rec log add. end */
	/* FUJITSU TEN:2014-01-15 failsafe function for calibration add. start */
	u32 last_intr_count_fs;
	u32 last_touch_count_fs;
	u32 fs_count;
	/* FUJITSU TEN:2014-01-15 failsafe function for calibration add. end */
};

/* I2C slave address pairs */
struct mxt_i2c_address_pair {
	u8 bootloader;
	u8 application;
};

static const struct mxt_i2c_address_pair mxt_i2c_addresses[] = {
	{ 0x24, 0x4a },
	{ 0x25, 0x4b },
	{ 0x26, 0x4c },
	{ 0x27, 0x4d },
	{ 0x34, 0x5a },
	{ 0x35, 0x5b },
};

/* FUJITSU TEN:2012-08-07 struct emmc add. start */
struct mxt_emmc_header {
	u16 mem_size;
	u8 max_reportid;
	u8 write_flag;
};
/* FUJITSU TEN:2012-08-07 struct emmc add. end */

/* FUJITSU TEN:2012-08-13 i2c error retry add. start */
#define MXT_I2C_TRANSFER( retry_count, return_code, adap, msgs, num )	\
	for( retry_count = 0; retry_count <= MXT_I2C_RETRY_MAX; retry_count++ ) { \
		if ( mxt_read_stop ) { \
			MXT_TS_DEBUG_LOG( "fastboot:i2c_transter loop stop.\n" ); \
			break; \
		} \
		if (i2c_transfer(adap, msgs, num) != num) { \
			MXT_TS_ERR_LOG( "i2c_transfer error\n" ); \
			return_code = -EIO; \
			continue; \
		} \
		else { \
			return_code = 0; \
			break; \
		} \
	}

#define MXT_I2C_MASTER_SEND( retry_count, return_code, client, buf, count )	\
	for( retry_count = 0; retry_count <= MXT_I2C_RETRY_MAX; retry_count++ ) { \
		if ( mxt_read_stop ) { \
			MXT_TS_DEBUG_LOG( "fastboot:i2c_master_send loop stop.\n" );\
			break; \
		} \
		if (i2c_master_send(client, buf, count) != count) { \
			MXT_TS_ERR_LOG( "i2c_master_send error\n" ); \
			return_code = -EIO; \
			continue; \
		} \
		else { \
			return_code = 0; \
			break; \
		} \
	}
/* FUJITSU TEN:2012-08-13 i2c error retry add. end */
 
/* FUJITSU TEN:2012-08-14 dev_XXX macro add start */
#define dev_dbg( dev, format, ... ) {	\
	MXT_TS_DEBUG_LOG( format, ##__VA_ARGS__ ); \
}
#define dev_info( dev, format, ... ) {	\
	MXT_TS_INFO_LOG( format, ##__VA_ARGS__ ); \
}
#define dev_err( dev, format, ... ) {	\
	MXT_TS_ERR_LOG( format, ##__VA_ARGS__ ); \
}
/* FUJITSU TEN:2012-08-14 dev_XXX macro add end */

/* FUJITSU TEN:2012-11-19 msleep macro add. start */
#ifdef MXT_DEVIDE_MSLEEP
#define MXT_MSLEEP( value ) {	\
	int tmp_time; \
	tmp_time = value; \
	while( tmp_time && !( mxt_read_stop ) ) { \
		if( tmp_time >= MXT_SLEEP_ONETIME_VALUE ) { \
			msleep( MXT_SLEEP_ONETIME_VALUE ); \
			tmp_time -= MXT_SLEEP_ONETIME_VALUE; \
		} else { \
			msleep( tmp_time );\
			tmp_time = 0;\
		} \
	} \
}
#else
#define MXT_MSLEEP( value )	msleep( value )
#endif
/* FUJITSU TEN:2012-11-19 msleep macro add. end */

/* FUJITSU TEN:2013-08-04 prototype add. start */
static int mxt_read_message(struct mxt_data *data,
	struct mxt_message *message);
/* FUJITSU TEN:2013-08-04 prototype add. end */

/* FUJITSU TEN:2012-06-19 mxt_tsns_reset add. start */
/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         TSNS_RESET execute function.
 * @param[in]     *data The structure of Touch panel driver information.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EIO Device I/O error.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_tsns_reset(struct mxt_data *data)
{
	int ret = 0;
	int timeout_counter = 0;
	int reset_count = 0;
	struct mxt_message message;
	u8 reportid;
	int error;

	MXT_TS_INFO_LOG( "start.\n" );
	MXT_TS_INFO_LOG( "Resetting chip TSNS_RESET\n");

	for( reset_count = 0; reset_count <= MXT_RESET_RETRY_MAX; reset_count++ ) {
		timeout_counter = 0;

		if ( mxt_read_stop ) {
			break;
		}

		data->pdata->tsns_reset(data->tsns_reset_exec_time);

		msleep(data->tsns_reset_time);

		while (((timeout_counter++ <= MXT_IRQ_TIMEOUT_COUNT)
			&& (data->pdata->read_chg()==0)) && !( mxt_read_stop ) )
			msleep(MXT_IRQ_CHECK_INTERVAL);

		if (timeout_counter > MXT_IRQ_TIMEOUT_COUNT) {
			if( reset_count == 0 ) {
				MXT_TS_ERR_LOG( "TSNS_RESET failsafe start.\n" );
				MXT_DRC_ERR( MXT_LITES_TSNS_RESET_FS, 0, 0, 0, 0 );
			}

			disable_irq( data->irq );
			do {
				if ( mxt_read_stop ) {
					break;
				}
				error = mxt_read_message(data, &message);
				if (error) {
					MXT_TS_ERR_LOG( "Failed to read message\n" );
					ret = error;
					break;
				}
				reportid = message.reportid;
			} while (reportid != MXT_RPTID_NOMSG );
			enable_irq( data->irq );

			msleep(MXT_IRQ_CHECK_INTERVAL);
			if( data->pdata->read_chg()!=0) {
				break;
			}

			MXT_TS_ERR_LOG( "TSNS_RESET error. retry_count=%d\n",
				reset_count );
		}
		else {
			break;
		}
	}

	if( reset_count >= MXT_RESET_RETRY_MAX && !( mxt_read_stop ) ) {
		MXT_TS_ERR_LOG( "TSNS_RESET retry out.\n" );
		MXT_DRC_HARD_ERR( MXT_LITES_TSNS_RESET_FE, 0, 0, 0, 0 );
		ret = -EIO;
	}
	else if( mxt_read_stop ) {
		MXT_TS_DEBUG_LOG( "fastboot:TSNS_RESET loop stop.\n" );
		ret = -ESHUTDOWN;
	}

	MXT_TS_DEBUG_LOG( "end.\n");

	return ret;
}
/* FUJITSU TEN:2012-06-19 mxt_tsns_reset add. end */

static int mxt_fw_read(struct mxt_data *data, u8 *val, unsigned int count)
{
	struct i2c_msg msg;

	msg.addr = data->bootloader_addr;
	msg.flags = data->client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.buf = val;

	return i2c_transfer(data->client->adapter, &msg, 1);
}

static int mxt_fw_write(struct mxt_data *data, const u8 * const val,
	unsigned int count)
{
	struct i2c_msg msg;

	msg.addr = data->bootloader_addr;
	msg.flags = data->client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = (u8 *)val;

	return i2c_transfer(data->client->adapter, &msg, 1);
}

static int mxt_get_bootloader_address(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int i;

	for (i = 0; i < ARRAY_SIZE(mxt_i2c_addresses); i++) {
		if (mxt_i2c_addresses[i].application == client->addr) {
			data->bootloader_addr = mxt_i2c_addresses[i].bootloader;

			dev_info(&client->dev, "Bootloader i2c adress: 0x%02x",
				data->bootloader_addr);

			return 0;
		}
	}

	dev_err(&client->dev, "Address 0x%02x not found in address table",
		client->addr);
	return -EINVAL;
}

static int mxt_probe_bootloader(struct mxt_data *data)
{
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct device *dev = &data->client->dev;
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
	int ret;
	u8 val;

	ret = mxt_get_bootloader_address(data);
	if (ret)
		return ret;

	if (mxt_fw_read(data, &val, 1) != 1) {
		dev_err(dev, "%s: i2c recv failed\n", __func__);
		return -EIO;
	}

	/* FUJITSU TEN:2012-08-14 dev_err macro add. start */
	if (val & MXT_BOOT_STATUS_MASK) {
		dev_err(dev, "Application CRC failure\n");
	}
	else {
		dev_err(dev, "Device in bootloader mode\n");
	}
	/* FUJITSU TEN:2012-08-14 dev_err macro add. end */

	return 0;
}

static int mxt_get_bootloader_version(struct mxt_data *data, u8 val)
{
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct device *dev = &data->client->dev;
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
	u8 buf[3];

	if (val | MXT_BOOT_EXTENDED_ID) {
		dev_dbg(dev, "Getting extended mode ID information");

		if (mxt_fw_read(data, &buf[0], 3) != 3) {
			dev_err(dev, "%s: i2c failure\n", __func__);
			return -EIO;
		}

		dev_info(dev, "Bootloader ID:%d Version:%d", buf[1], buf[2]);

		return buf[0];
	} else {
		dev_info(dev, "Bootloader ID:%d", val & MXT_BOOT_ID_MASK);

		return val;
	}
}

static int mxt_check_bootloader(struct mxt_data *data,
				unsigned int state)
{
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct device *dev = &data->client->dev;
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
	u8 val;

recheck:
	if (mxt_fw_read(data, &val, 1) != 1) {
		dev_err(dev, "%s: i2c recv failed\n", __func__);
		return -EIO;
	}

	switch (state) {
	case MXT_WAITING_BOOTLOAD_CMD:
		val = mxt_get_bootloader_version(data, val);
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_WAITING_FRAME_DATA:
	case MXT_APP_CRC_FAIL:
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_FRAME_CRC_PASS:
		if (val == MXT_FRAME_CRC_CHECK)
			goto recheck;
		if (val == MXT_FRAME_CRC_FAIL) {
			dev_err(dev, "Bootloader CRC fail\n");
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	if (val != state) {
		dev_err(dev, "Invalid bootloader mode state 0x%X\n", val);
		return -EINVAL;
	}

	return 0;
}

static int mxt_unlock_bootloader(struct mxt_data *data)
{
	u8 buf[2];

	buf[0] = MXT_UNLOCK_CMD_LSB;
	buf[1] = MXT_UNLOCK_CMD_MSB;

	if (mxt_fw_write(data, buf, 2) != 2) {
		dev_err(&data->client->dev, "%s: i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Read register function for device.
 * @param[in]     *client The structure of Touch panel driver use I2C information.
 * @param[in]     reg Read register address offset.
 * @param[in]     len Read register length.
 * @param[out]    *val Read data.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        !=0 Error end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int __mxt_read_reg(struct i2c_client *client,
			       u16 reg, u16 len, void *val)
{
	struct i2c_msg xfer[2];
	u8 buf[2];
	/* FUJITSU TEN:2012-08-13 i2c error retry add. start */
	int retry_count;
	int ret = 0;
	/* FUJITSU TEN:2012-08-13 i2c error retry add. end */

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start. reg = %x len = %x\n", reg, len );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	buf[0] = reg & 0xff;
	buf[1] = (reg >> 8) & 0xff;

	/* Write register */
	xfer[0].addr = client->addr;
	xfer[0].flags = 0;
	xfer[0].len = 2;
	xfer[0].buf = buf;

	/* Read data */
	xfer[1].addr = client->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = len;
	xfer[1].buf = val;

	/* FUJITSU TEN:2012-08-13 i2c error retry add. start */
	MXT_I2C_TRANSFER( retry_count, ret, client->adapter, xfer, 2 );

	MXT_TS_DEBUG_LOG( "end. ret = %d\n", ret );

	return ret;
	/* FUJITSU TEN:2012-08-13 i2c error retry add. end */
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Read register wrapper function.
 * @param[in]     *client The structure of Touch panel driver use I2C information.
 * @param[in]     reg Read register address offset.
 * @param[out]    *val Read data.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        !=0 Error end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_read_reg(struct i2c_client *client, u16 reg, u8 *val)
{
	/* FUJITSU TEN:2012-06-25 debug log add. start */
	int ret;

	MXT_TS_INFO_LOG( "start. reg = %x\n", reg );
	
	ret = __mxt_read_reg(client, reg, 1, val);

	MXT_TS_DEBUG_LOG( "end. val = %x\n", *val );

	return ret;
	/* FUJITSU TEN:2012-06-25 debug log add. end */
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Write register function for device.
 * @param[in]     *client The structure of Touch panel driver use I2C information.
 * @param[in]     reg Write register address offset.
 * @param[in]     *val Write data.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        !=0 Error end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
	u8 buf[3];
	/* FUJITSU TEN:2012-08-13 i2c error retry add. start */
	int retry_count;
	int ret = 0;
	/* FUJITSU TEN:2012-08-13 i2c error retry add. end */

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start. reg = %x, val = %x\n", reg, val );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	buf[0] = reg & 0xff;
	buf[1] = (reg >> 8) & 0xff;
	buf[2] = val;

	/* FUJITSU TEN:2012-08-13 i2c error retry add. start */
	MXT_I2C_MASTER_SEND( retry_count, ret, client, buf, 3 );

	MXT_TS_DEBUG_LOG( "end. ret = %d\n", ret );

	return ret;
	/* FUJITSU TEN:2012-08-13 i2c error retry add. end */
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Block write function for device.
 * @param[in]     *client The structure of Touch panel driver use I2C information.
 * @param[in]     addr Write register address offset.
 * @param[in]     length Write register length.
 * @param[in]     *value Write data.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EINVAL Parameter error.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
int mxt_write_block(struct i2c_client *client, u16 addr, u16 length, u8 *value)
{
	/* FUJITSU TEN:2012-08-13 i2c error retry add. start */
	int retry_count;
	int ret = 0;
	/* FUJITSU TEN:2012-08-13 i2c error retry add. end */
	struct {
		__le16 le_addr;
		u8  data[MXT_MAX_BLOCK_WRITE];
	} i2c_block_transfer;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start. addr = %x, length = %x\n", addr, length );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	/* FUJITSU TEN:2012-08-13 error log add. start */
	if (length > MXT_MAX_BLOCK_WRITE) {
		MXT_TS_ERR_LOG( "buffer size error. length = %x\n", length );
		return -EINVAL;
	}
	/* FUJITSU TEN:2012-08-13 error log add. end */
	memcpy(i2c_block_transfer.data, value, length);

	i2c_block_transfer.le_addr = cpu_to_le16(addr);

	/* FUJITSU TEN:2012-08-13 i2c error retry add. start */
	MXT_I2C_MASTER_SEND( retry_count, ret, client, 
		(u8 *) &i2c_block_transfer, ( length + 2 ) );

	MXT_TS_DEBUG_LOG( "end. ret = %d\n", ret );

	return ret;
	/* FUJITSU TEN:2012-08-13 i2c error retry add. end */
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief       Read object table (memorymap table) data for device.
 * @param[in]	*data The structure of Touch panel driver information.
 * @param[in]	reg Read register address offset.
 * @param[out]	*object_buf Read data.
 * @return	Result.
 * @retval	0 Normal end.
 * @retval	!=0 Error end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_read_object_table(struct mxt_data *data,
					u16 reg, u8 *object_buf)
{
	/* FUJITSU TEN:2012-08-10 variable add. start */
	int ret = 0;
	int error_count;
	/* FUJITSU TEN:2012-08-10 variable add. end */

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start. reg = %x\n", reg );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	/* FUJITSU TEN:2012-08-10 retry add. start */
	for( error_count = 0; error_count <= MXT_ERROR_RETRY_MAX; error_count++ ) {
		ret = __mxt_read_reg(data->client, reg, MXT_OBJECT_SIZE,
			object_buf);
		if( ret != 0 ) {
			MXT_TS_ERR_LOG( "__mxt_read_reg error.\n" );
		}
		else {
			goto END;
		}
		if( error_count != MXT_ERROR_RETRY_MAX ) {
			msleep(MXT_ERROR_RETRY_WAIT);
			MXT_TS_DEBUG_LOG( "error retry. count:%x\n",
				error_count );
		}
	}
END:
	MXT_TS_DEBUG_LOG( "end.\n" );
	return ret;
	/* FUJITSU TEN:2012-08-10 retry add. end */
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Read object table (memorymap table) data for driver data.
 * @param[in]     *data The structure of Touch panel driver information.
 * @param[in]     type Object table id (Txx).
 * @return        Object table (memorymap table) data.
 * @retval        NULL Object table id (Txx) error.
 * *retval        !=0 Object table (memorymap table) data pointer.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static struct mxt_object *mxt_get_object(struct mxt_data *data, u8 type)
{
	struct mxt_object *object;
	int i;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start. type = %x\n", type );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	for (i = 0; i < data->info.object_num; i++) {
		object = data->object_table + i;
		if (object->type == type)
			return object;
	}

	dev_err(&data->client->dev, "Invalid object type T%u\n", type);
	return NULL;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Check for T5 message length.
 * @param[in]     *data The structure of Touch panel driver information.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EINVAL T5 message object data error.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_check_message_length(struct mxt_data *data)
{
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct device *dev = &data->client->dev;
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
	struct mxt_object *object;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	object = mxt_get_object(data, MXT_GEN_MESSAGE_T5);
	if (!object)
		return -EINVAL;

	if (object->size > MXT_MSG_MAX_SIZE) {
		dev_err(dev, "MXT_MSG_MAX_SIZE exceeded");
		return -EINVAL;
	}

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return 0;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Read T5 message object.
 * @param[in]     *data The structure of Touch panel driver information.
 * @param[in]     *message Read T5 message data.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EINVAL T5 message object data error.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_read_message(struct mxt_data *data,
				 struct mxt_message *message)
{
	struct mxt_object *object;
	u16 reg;
	int ret;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	object = mxt_get_object(data, MXT_GEN_MESSAGE_T5);
	if (!object)
		return -EINVAL;

	reg = object->start_address;

	/* Do not read last byte which contains CRC */
	ret = __mxt_read_reg(data->client, reg,
			     object->size - 1, message);

	if (ret == 0 && message->reportid != MXT_RPTID_NOMSG
	    && data->debug_enabled)
		print_hex_dump(KERN_DEBUG, "MXT MSG:", DUMP_PREFIX_NONE, 16, 1,
			       message, object->size - 1, false);

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end. return_code : %d\n", ret );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return ret;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Read T5 message object for the same as specified report id.
 * @param[in]     *data The structure of Touch panel driver information.
 * @param[in]     *message Read T5 message data.
 * @param[in]     *reportid Check report id.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EINVAL T5 message object data error.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_read_message_reportid(struct mxt_data *data,
	struct mxt_message *message, u8 reportid)
{
	int try = 0;
	int error;
	int fail_count;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start. reportid = %x\n", reportid );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	fail_count = data->max_reportid * 2;

	while (++try < fail_count) {
		error = mxt_read_message(data, message);
		if (error)
			return error;

		if (message->reportid == 0xff)
			return -EINVAL;

		if (message->reportid == reportid)
			return 0;
	}

	return -EINVAL;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Read object table (memorymap table) data for table.
 * @param[in]     *data The structure of Touch panel driver information.
 * @param[in]     type Object table id (Txx).
 * @param[in]     offset Read register address offset.
 * @param[out]    *val Read data.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EINVAL Object table id (Txx) error.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_read_object(struct mxt_data *data,
				u8 type, u8 offset, u8 *val)
{
	struct mxt_object *object;
	u16 reg;
	/* FUJITSU TEN:2012-06-25 debug log add. start */
	int ret = 0;

	MXT_TS_INFO_LOG( "start. type=%x, offset=%x\n",type, offset );


	object = mxt_get_object(data, type);
	if (!object)
		return -EINVAL;

	reg = object->start_address;
	ret = __mxt_read_reg(data->client, reg + offset, 1, val);

	MXT_TS_DEBUG_LOG( "end. ret=%d, val=%x\n", ret, *val );
	return ret;
	/* FUJITSU TEN:2012-06-25 debug log add. end */
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Write object table (memorymap table) data for device.
 * @param[in]     *data The structure of Touch panel driver information.
 * @param[in]     type Object table id (Txx).
 * @param[in]     offset Write register address offset.
 * @param[in]     *val Write data.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EINVAL Object table id (Txx) error.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_write_object(struct mxt_data *data,
				 u8 type, u8 offset, u8 val)
{
	struct mxt_object *object;
	u16 reg;
	/* FUJITSU TEN:2012-06-25 debug log add. start */
	int ret = 0;

	MXT_TS_INFO_LOG( "start. type=%x, offset=%x, val=%x\n", type, offset, val );

	object = mxt_get_object(data, type);
	if (!object)
		return -EINVAL;

	if (offset >= object->size * object->instances) {
		dev_err(&data->client->dev, "Tried to write outside object T%d"
			" offset:%d, size:%d\n", type, offset, object->size);
		return -EINVAL;
	}

	reg = object->start_address;
	ret = mxt_write_reg(data->client, reg + offset, val);

	MXT_TS_DEBUG_LOG( "end. ret=%d\n", ret );
	return ret;
	/* FUJITSU TEN:2012-06-25 debug log add. end */
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Touch report data set for Input Sybsystem.
 * @param[in]     *data The structure of Touch panel driver information.
 * @param[in]     single_id Single touch report data id.
 * @return        None.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static void mxt_input_report(struct mxt_data *data, int single_id)
{
	struct mxt_finger *finger = data->finger;
	struct input_dev *input_dev = data->input_dev;
/* FUJITSU TEN:2012-06-25 comment out. start */
//	int status = finger[single_id].status;
/* FUJITSU TEN:2012-06-25 comment out. end */
	int finger_num = 0;
	int id;
	/* FUJITSU TEN:2013-06-07 debug function add. start */
	unsigned char trc_data[ MXT_LITES_TRACE_DATA_SIZE ] = { 0 };
	/* FUJITSU TEN:2013-06-07 debug function add. end */

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */
	for (id = 0; id < MXT_MAX_FINGER; id++) {
		if (!finger[id].status)
			continue;

		input_mt_slot(input_dev, id);
		input_mt_report_slot_state(input_dev, MT_TOOL_FINGER,
				finger[id].status != MXT_RELEASE);
		if (finger[id].status != MXT_RELEASE) {
			finger_num++;
			input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR,
					finger[id].area);
			input_report_abs(input_dev, ABS_MT_POSITION_X,
					finger[id].x);
			input_report_abs(input_dev, ABS_MT_POSITION_Y,
					finger[id].y);
			input_report_abs(input_dev, ABS_MT_PRESSURE,
					finger[id].pressure);
			/* FUJITSU TEN:2012-06-19 debug log add. start */
			MXT_TS_INFO_LOG(
			"### touch id %d : area=%x, x=%x, y=%x, pressure=%x\n",
				id, finger[id].area, finger[id].x, finger[id].y,
				finger[id].pressure );
			/* FUJITSU TEN:2013-06-07 debug function add. start */
			if( !( data->touch_count ) ) {
				sprintf( trc_data, "set input data id=%1d x=%3x y=%3x\n",
					id, finger[id].x, finger[id].y );
				MXT_DRC_TRC( trc_data );
			}
			data->touch_count++;
			do_gettimeofday( &(data->touch_tv) );
			/* FUJITSU TEN:2013-06-07 debug function add. end */

		} else {
			finger[id].status = 0;
			MXT_TS_INFO_LOG( "### touch id %d : release\n", id );
			/* FUJITSU TEN:2012-06-19 debug log add. end */
		}
	}

	input_report_key(input_dev, BTN_TOUCH, finger_num > 0);

/* FUJITSU TEN:2012-06-25 single touch not support. start */
#if 0
	if (status != MXT_RELEASE) {
		input_report_abs(input_dev, ABS_X, finger[single_id].x);
		input_report_abs(input_dev, ABS_Y, finger[single_id].y);
		input_report_abs(input_dev,
				 ABS_PRESSURE, finger[single_id].pressure);
		/* FUJITSU TEN:2012-06-19 debug log add. start */
		MXT_TS_DEBUG_LOG( "###single touch : x=%x, y=%x, pressure=%x\n",
			finger[single_id].x, finger[single_id].y,
			finger[single_id].pressure );
		/* FUJITSU TEN:2012-06-19 debug log add. end */
	}
#endif
/* FUJITSU TEN:2012-06-25 single touch not support. end */

	input_sync(input_dev);

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Read message change into the touch report.
 * @param[in,out] *data The structure of Touch panel driver information.
 * @param[in]     message Read T5 message object.
 * @param[in]     id Touch report data id.
 * @return        None.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static void mxt_input_touchevent(struct mxt_data *data,
				      struct mxt_message *message, int id)
{
	struct mxt_finger *finger = data->finger;
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct device *dev = &data->client->dev;
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
	u8 status = message->message[0];
	int x;
	int y;
	int area;
	int pressure;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	if (data->driver_paused)
		return;

	if (id > MXT_MAX_FINGER) {
		dev_err(dev, "MXT_MAX_FINGER exceeded!\n");
		return;
	}

	/* Check the touch is present on the screen */
	if (!(status & MXT_DETECT)) {
		if (status & MXT_SUPPRESS) {
			dev_dbg(dev, "[%d] suppressed\n", id);

			finger[id].status = MXT_RELEASE;
			mxt_input_report(data, id);
		} else if (status & MXT_RELEASE) {
			dev_dbg(dev, "[%d] released\n", id);

			finger[id].status = MXT_RELEASE;
			mxt_input_report(data, id);
		}
		return;
	}

	/* Check only AMP detection */
	if (!(status & (MXT_PRESS | MXT_MOVE)))
		return;

	x = (message->message[1] << 4) | ((message->message[3] >> 4) & 0xf);
	y = (message->message[2] << 4) | ((message->message[3] & 0xf));
	if (data->max_x < 1024)
		x = x >> 2;
	if (data->max_y < 1024)
		y = y >> 2;

	area = message->message[4];
	pressure = message->message[5];

	dev_dbg(dev, "[%d] %s x: %d, y: %d, area: %d\n", id,
		status & MXT_MOVE ? "moved" : "pressed",
		x, y, area);

	finger[id].status = status & MXT_MOVE ?
				MXT_MOVE : MXT_PRESS;
	finger[id].x = x;
	finger[id].y = y;
	finger[id].area = area;
	finger[id].pressure = pressure;

	mxt_input_report(data, id);

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Touch panel driver interrupt hander.
 * @param[in]     irq Irq No.
 * @param[in]     *dev_id Device id.
 * @return        irq return.
 * @retval        IRQ_HANDLED Get interrupt data.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static irqreturn_t mxt_interrupt(int irq, void *dev_id)
{
	struct mxt_data *data = dev_id;
	struct mxt_message message;
	struct mxt_object *object;
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct device *dev = &data->client->dev;
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
	int touchid;
	u8 reportid;
	/* FUJITSU TEN:2012-08-09 variable add. start */
	unsigned long flags = 0;
	/* FUJITSU TEN:2012-08-09 variable add. end */

	
	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	do {
		/* FUJITSU TEN:2012-08-10 force read stop add. start */
		if ( mxt_read_stop ) {
			MXT_TS_DEBUG_LOG( "message read stop.\n" );
			break;
		}
		/* FUJITSU TEN:2012-08-10 force read stop add. end */

		if (mxt_read_message(data, &message)) {
			dev_err(dev, "Failed to read message\n");
			goto end;
		}

		reportid = message.reportid;

		/* FUJITSU TEN:2012-06-25 debug log add. start */
		MXT_TS_DEBUG_LOG( "message object:"
			"%02x %02x %02x %02x %02x %02x %02x %02x \n",
			message.reportid, message.message[0], message.message[1],
			message.message[2], message.message[3], message.message[4],
			message.message[5], message.message[6] );
		/* FUJITSU TEN:2012-06-25 debug log add. end */

		object = mxt_get_object(data, MXT_TOUCH_MULTI_T9);
		if (!object)
			goto end;

		if (reportid >= object->min_reportid
			&& reportid <= object->max_reportid) {
			touchid = reportid - object->min_reportid;
			/* FUJITSU TEN:2013-06-07 debug function add. start */
			if( !( data->intr_count ) ) {
				MXT_DRC_TRC( "get interrupt touch data.\n" );
			}
			data->intr_count++;
			do_gettimeofday( &(data->intr_tv) );
			/* FUJITSU TEN:2013-06-07 debug function add. end */
			mxt_input_touchevent(data, &message, touchid);
		} else {
			/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. start */
			object = mxt_get_object(data, MXT_SPT_SELFTEST_T25);
			if (!object)
				goto end;

			if ( reportid == object->max_reportid ) {
				if( data->test_flag == 0 ) {
					memcpy( data->test_data, message.message,
						sizeof( u8 ) * MXT_SELFTEST_RESULT_SIZE );
					data->test_flag = 1;
				}
				wake_up_interruptible( &(data->wait) );
			}
			else {
				object = mxt_get_object(data, MXT_GEN_COMMAND_T6);
				if (!object)
					goto end;

				/* FUJITSU TEN:2012-08-09 config error check edit. start */
				if ( reportid != object->max_reportid ) {
					continue;
				}

				MXT_TS_DEBUG_LOG( "config error check.\n" );

				if (message.message[0] & MXT_STATUS_ERROR) {
					spin_lock_irqsave( &mxt_config_lock, flags );
					mxt_config_status =
						message.message[0] & MXT_STATUS_ERROR;
					spin_unlock_irqrestore( &mxt_config_lock, flags );
					dev_err(dev, "Configuration error %x\n",
						mxt_config_status );
				}
				/* FUJITSU TEN:2012-08-09 config error check edit. end */
			}
			/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. end */
		}
	} while (reportid != MXT_RPTID_NOMSG );

end:
	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return IRQ_HANDLED;
}

static int mxt_make_highchg(struct mxt_data *data)
{
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct device *dev = &data->client->dev;
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
	struct mxt_message message;
	int count;
	int error;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	/* If all objects report themselves then a number of messages equal to
	 * the number of report ids may be generated. Therefore a good safety
	 * heuristic is twice this number */
	count = data->max_reportid * 2;

	/* Read dummy message to make high CHG pin */
	do {
		error = mxt_read_message(data, &message);
		if (error)
			return error;
	} while (message.reportid != MXT_RPTID_NOMSG && --count);

	if (!count) {
		dev_err(dev, "CHG pin isn't cleared\n");
		return -EBUSY;
	}

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return 0;
}

static int mxt_read_current_crc(struct mxt_data *data, unsigned long *crc)
{
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct device *dev = &data->client->dev;
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
	int error;
	struct mxt_message message;
	struct mxt_object *object;

	object = mxt_get_object(data, MXT_GEN_COMMAND_T6);
	if (!object)
		return -EIO;

	/* Try to read the config checksum of the existing cfg */
	mxt_write_object(data, MXT_GEN_COMMAND_T6,
		MXT_COMMAND_REPORTALL, 1);

	msleep(30);

	/* Read message from command processor, which only has one report ID */
	error = mxt_read_message_reportid(data, &message, object->max_reportid);
	if (error) {
		dev_err(dev, "Failed to retrieve CRC\n");
		return error;
	}

	/* Bytes 1-3 are the checksum. */
	*crc = message.message[1] | (message.message[2] << 8) |
		(message.message[3] << 16);

	return 0;
}

int mxt_download_config(struct mxt_data *data, const char *fn)
{
	struct device *dev = &data->client->dev;
	struct mxt_info cfg_info;
	struct mxt_object *object;
	const struct firmware *cfg = NULL;
	int ret;
	int offset;
	int pos;
	int i;
	unsigned long current_crc, info_crc, config_crc;
	unsigned int type, instance, size;
	u8 val;
	u16 reg;

	ret = request_firmware(&cfg, fn, dev);
	if (ret < 0) {
		dev_err(dev, "Failure to request config file %s\n", fn);
		return 0;
	}

	ret = mxt_read_current_crc(data, &current_crc);
	if (ret)
		return ret;

	if (strncmp(cfg->data, MXT_CFG_MAGIC, strlen(MXT_CFG_MAGIC))) {
		dev_err(dev, "Unrecognised config file\n");
		ret = -EINVAL;
		goto release;
	}

	pos = strlen(MXT_CFG_MAGIC);

	/* Load information block and check */
	for (i = 0; i < sizeof(struct mxt_info); i++) {
		ret = sscanf(cfg->data + pos, "%hhx%n",
			     (unsigned char *)&cfg_info + i,
			     &offset);
		if (ret != 1) {
			dev_err(dev, "Bad format\n");
			ret = -EINVAL;
			goto release;
		}

		pos += offset;
	}

	if (cfg_info.family_id != data->info.family_id) {
		dev_err(dev, "Family ID mismatch!\n");
		ret = -EINVAL;
		goto release;
	}

	if (cfg_info.variant_id != data->info.variant_id) {
		dev_err(dev, "Variant ID mismatch!\n");
		ret = -EINVAL;
		goto release;
	}

	if (cfg_info.version != data->info.version)
		dev_err(dev, "Warning: version mismatch!\n");

	if (cfg_info.build != data->info.build)
		dev_err(dev, "Warning: build num mismatch!\n");

	ret = sscanf(cfg->data + pos, "%lx%n", &info_crc, &offset);
	if (ret != 1) {
		dev_err(dev, "Bad format\n");
		ret = -EINVAL;
		goto release;
	}
	pos += offset;

	/* Check config CRC */
	ret = sscanf(cfg->data + pos, "%lx%n", &config_crc, &offset);
	if (ret != 1) {
		dev_err(dev, "Bad format\n");
		ret = -EINVAL;
		goto release;
	}
	pos += offset;

	if (current_crc == config_crc) {
		dev_info(dev,
		"Config CRC 0x%X: OK\n", (unsigned int) current_crc);
		ret = 0;
		goto release;
	} else {
		dev_info(dev, "Config CRC 0x%X: does not match 0x%X, "
			"writing config\n",
			(unsigned int) current_crc,
			(unsigned int) config_crc);
	}

	while (pos < cfg->size) {
		/* Read type, instance, length */
		ret = sscanf(cfg->data + pos, "%x %x %x%n",
			     &type, &instance, &size, &offset);
		if (ret == 0) {
			/* EOF */
			ret = 1;
			goto release;
		} else if (ret < 0) {
			dev_err(dev, "Bad format\n");
			ret = -EINVAL;
			goto release;
		}
		pos += offset;

		object = mxt_get_object(data, type);
		if (!object) {
			ret = -EINVAL;
			goto release;
		}

		if (size > object->size) {
			dev_err(dev, "Object length exceeded!\n");
			ret = -EINVAL;
			goto release;
		}

		if (instance >= object->instances) {
			dev_err(dev, "Object instances exceeded!\n");
			ret = -EINVAL;
			goto release;
		}

		reg = object->start_address + object->size * instance;

		for (i = 0; i < size; i++) {
			ret = sscanf(cfg->data + pos, "%hhx%n",
				     &val,
				     &offset);
			if (ret != 1) {
				dev_err(dev, "Bad format\n");
				ret = -EINVAL;
				goto release;
			}

			ret = mxt_write_reg(data->client, reg + i, val);
			if (ret)
				goto release;

			pos += offset;
		}

		/* If firmware is upgraded, new bytes may be added to end of
		 * objects. It is generally forward compatible to zero these
		 * bytes - previous behaviour will be retained. However
		 * this does invalidate the CRC and will force a config
		 * download every time until the configuration is updated */
		if (size < object->size) {
			dev_info(dev, "Warning: zeroing %d byte(s) in T%d\n",
				 object->size - size, type);

			for (i = size + 1; i < object->size; i++) {
				ret = mxt_write_reg(data->client, reg + i, 0);
				if (ret)
					goto release;
			}
		}
	}

release:
	release_firmware(cfg);
	return ret;
}

static int mxt_soft_reset(struct mxt_data *data, u8 value)
{
	int timeout_counter = 0;
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct device *dev = &data->client->dev;
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */

	/* FUJITSU TEN:2012-06-25 debug log add. start */	
	MXT_TS_INFO_LOG( "start. value=%x\n", value );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	dev_info(dev, "Resetting chip\n");

	mxt_write_object(data, MXT_GEN_COMMAND_T6,
			MXT_COMMAND_RESET, value);

	msleep(MXT540E_RESET_TIME);
	timeout_counter = 0;
/* FUJITSU TEN:2012-06-19 reset interrput edit. start */
#if 0
	while ((timeout_counter++ <= 100) && (data->pdata->read_chg()==0))
		msleep(20);
	if (timeout_counter > 100) {
		dev_err(dev, "No response after reset!\n");
		return -EIO;
	}
#endif
/* FUJITSU TEN:2012-06-19 reset interrput edit. end */

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n");
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return 0;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Set power config for Touch panel device.
 * @param[in,out] *data The structure of Touch panel driver information.
 * @param[in]     mode Set power mode (RUN/DEEPSLEEP).
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EINVAL Parameter error.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_set_power_cfg(struct mxt_data *data, u8 mode)
{
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct device *dev = &data->client->dev;
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
	int error;
	u8 actv_cycle_time;
	u8 idle_cycle_time;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	if (data->state != APPMODE) {
		dev_err(dev, "Not in APPMODE\n");
		return -EINVAL;
	}

	switch (mode) {
	case MXT_POWER_CFG_DEEPSLEEP:
		actv_cycle_time = 0;
		idle_cycle_time = 0;
		break;
	case MXT_POWER_CFG_RUN:
	default:
		actv_cycle_time = data->actv_cycle_time;
		idle_cycle_time = data->idle_cycle_time;
		break;
	}

	error = mxt_write_object(data, MXT_GEN_POWER_T7, MXT_POWER_ACTVACQINT,
	actv_cycle_time);
	if (error)
		goto i2c_error;

	error = mxt_write_object(data, MXT_GEN_POWER_T7, MXT_POWER_IDLEACQINT,
				idle_cycle_time);
	if (error)
		goto i2c_error;

	dev_dbg(dev, "Set ACTV %d, IDLE %d", actv_cycle_time, idle_cycle_time);

	data->is_stopped = (mode == MXT_POWER_CFG_DEEPSLEEP) ? 1 : 0;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. start */

	return 0;

i2c_error:
	dev_err(dev, "Failed to set power cfg");

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end. error:%d\n", error );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return error;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief       Read power config for Touch panel device.
 * @param[in]   *data The structure of Touch panel driver information.
 * @param[out]  *actv_cycle_time Read ACTVACQINT data.
 * @param[out]  *idle_cycle_time Read IDLEACQINT data.
 * @return      Result.
 * @retval      0 Normal end.
 * @retval      !=0 Error end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_read_power_cfg(struct mxt_data *data, u8 *actv_cycle_time,
				u8 *idle_cycle_time)
{
	int error;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	error = mxt_read_object(data, MXT_GEN_POWER_T7,
				MXT_POWER_ACTVACQINT,
				actv_cycle_time);
	if (error)
		return error;

	error = mxt_read_object(data, MXT_GEN_POWER_T7,
				MXT_POWER_IDLEACQINT,
				idle_cycle_time);
	if (error)
		return error;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end. actv_cycle_time=%x, idle_cycle_time=%x\n",
		*actv_cycle_time, *idle_cycle_time );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return 0;
}

static int mxt_check_power_cfg_post_reset(struct mxt_data *data)
{
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct device *dev = &data->client->dev;
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
	int error;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	error = mxt_read_power_cfg(data, &data->actv_cycle_time,
				   &data->idle_cycle_time);
	if (error)
		return error;

	/* Power config is zero, select free run */
	if (data->actv_cycle_time == 0 || data->idle_cycle_time == 0) {
		dev_dbg(dev, "Overriding power cfg to free run\n");
		data->actv_cycle_time = 255;
		data->idle_cycle_time = 255;

		error = mxt_set_power_cfg(data, MXT_POWER_CFG_RUN);
		if (error)
			return error;
	}

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return 0;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Store power config data in Touch panel driver information.
 * @param[in,out] *data The structure of Touch panel driver information.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        !=0 Error end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_probe_power_cfg(struct mxt_data *data)
{
/* FUJITSU TEN:2012-09-03 DEEPSLEEP mode setting for initialize add. start */
#ifdef CONFIG_TOUCH_INIT_DEEPSLEEP_SETTING
	int error;
#endif
/* FUJITSU TEN:2012-08-09 comment out. end */

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

/* FUJITSU TEN:2012-09-03 DEEPSLEEP mode setting for initialize add. start */
#ifdef CONFIG_TOUCH_INIT_DEEPSLEEP_SETTING
	error = mxt_read_power_cfg(data, &data->actv_cycle_time,
				   &data->idle_cycle_time);
	if (error)
		return error;

/* FUJITSU TEN:2012-08-09 comment out. start */
#if 0	
	/* If in deep sleep mode, attempt reset */
	if (data->actv_cycle_time == 0 || data->idle_cycle_time == 0) {

		error = mxt_soft_reset( data, MXT_RESET_VALUE );
		if (error)
			return error;

		error = mxt_check_power_cfg_post_reset(data);
		if (error)
			return error;
	}
#endif
/* FUJITSU TEN:2012-08-09 comment out. end */
#else
	data->actv_cycle_time = MXT_POWER_CFG_RUN_DATA;
	data->idle_cycle_time = MXT_POWER_CFG_RUN_DATA;
#endif
/* FUJITSU TEN:2012-09-03 DEEPSLEEP mode setting for initialize add. end */

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return 0;
}

static int mxt_check_reg_init(struct mxt_data *data)
{
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct device *dev = &data->client->dev;
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
	int timeout_counter = 0;
	int ret;
	u8 command_register;

	ret = mxt_download_config(data, MXT_CFG_NAME);
	if (ret < 0)
		return ret;
	else if (ret == 0)
		/* CRC matched, or no config file, no need to reset */
		return 0;

	/* Backup to memory */
	mxt_write_object(data, MXT_GEN_COMMAND_T6,
			MXT_COMMAND_BACKUPNV,
			MXT_BACKUP_VALUE);
	msleep(MXT_BACKUP_TIME);
	do {
		ret =  mxt_read_object(data, MXT_GEN_COMMAND_T6,
					MXT_COMMAND_BACKUPNV,
					&command_register);
		if (ret)
			return ret;
		msleep(20);
	} while ((command_register != 0) && (timeout_counter++ <= 100));
	if (timeout_counter > 100) {
		dev_err(dev, "No response after backup!\n");
		return -EIO;
	}

	ret = mxt_soft_reset(data, MXT_RESET_VALUE );
	if (ret)
		return ret;

	ret = mxt_check_power_cfg_post_reset(data);
	if (ret)
		return ret;

	return 0;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Store device information in Touch panel driver information.
 * @param[in,out] *data The structure of Touch panel driver information.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        !=0 Error end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_get_info(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	struct mxt_info *info = &data->info;
	int error;
	u8 val;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	error = mxt_read_reg(client, MXT_FAMILY_ID, &val);
	if (error)
		/* FUJITSU TEN:2012-08-06 goto add. start */
		goto ERR;
		/* FUJITSU TEN:2012-08-06 goto add. end */
	info->family_id = val;

	error = mxt_read_reg(client, MXT_VARIANT_ID, &val);
	if (error)
		/* FUJITSU TEN:2012-08-06 goto add. start */
		goto ERR;
		/* FUJITSU TEN:2012-08-06 goto add. end */
	info->variant_id = val;

	error = mxt_read_reg(client, MXT_VERSION, &val);
	if (error)
		/* FUJITSU TEN:2012-08-06 goto add. start */
		goto ERR;
		/* FUJITSU TEN:2012-08-06 goto add. end */
	info->version = val;

	error = mxt_read_reg(client, MXT_BUILD, &val);
	if (error)
		/* FUJITSU TEN:2012-08-06 goto add. start */
		goto ERR;
		/* FUJITSU TEN:2012-08-06 goto add. end */
	info->build = val;

	error = mxt_read_reg(client, MXT_OBJECT_NUM, &val);
	if (error)
		/* FUJITSU TEN:2012-08-06 goto add. start */
		goto ERR;
		/* FUJITSU TEN:2012-08-06 goto add. end */
	info->object_num = val;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return 0;

/* FUJITSU TEN:2012-08-06 error log add. start */
ERR:
	MXT_TS_ERR_LOG( "Information Block register data get error. %x\n", error );
	return error;	
/* FUJITSU TEN:2012-08-06 error log add. start */
}

/* FUJITSU TEN:2012-08-08 mxt_read/write_mmap add. start */
/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Read eMMC data for Nonvolatile driver.
 * @param[in]     *data The structure of Touch panel driver information.
 * @param[in]     offset Read eMMC data address offset.
 * @param[in]     size Read eMMC data size.
 * @param[out]    *buf Read eMMC data.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EINVAL Parameter error.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_read_mmap(struct mxt_data *data,
	unsigned int offset, size_t size, unsigned char *buf )
{
	int ret = 0;
	int error_count;
#ifdef CONFIG_TOUCH_LOG_DEBUG
	int pos = 0, copy_size, tmp_offset;
	unsigned char printbuf[ MXT_DEBUG_PRINT_SIZE ];
	unsigned char *tmp_buf;
#endif

	MXT_TS_INFO_LOG( "start. offset=%x, size=%x\n", offset, size );

	if( buf == NULL ) {
		MXT_TS_ERR_LOG( "parameter error.\n" );
		ret = -EINVAL;
		goto END;
	}
	for( error_count = 0; error_count <= MXT_ERROR_RETRY_MAX; error_count++ ) {
		ret = GetNONVOLATILE( buf, offset, size );
		if( ret != 0 ) {
			MXT_TS_ERR_LOG( "GetNONVOLATILE error. ret=%x\n", ret );
			continue;
		}
		break;
	}
	if( ret != 0 ) {
		/* error save */
		MXT_DRC_HARD_ERR( MXT_LITES_EMMC_ACCESS, ret, 0, 0, 0 );
	}
#ifdef CONFIG_TOUCH_LOG_DEBUG
	else {
		pos = size;
		tmp_buf = buf;
		tmp_offset = offset;
		memset( printbuf, 0, MXT_DEBUG_PRINT_SIZE );
		while( pos > 0 ) {
			if( pos >= MXT_DEBUG_PRINT_SIZE ) {
				copy_size = MXT_DEBUG_PRINT_SIZE;
				pos -= MXT_DEBUG_PRINT_SIZE;
			}
			else {
				copy_size = pos % MXT_DEBUG_PRINT_SIZE;
				pos -= copy_size;
			}
			memcpy( printbuf, tmp_buf, copy_size );
			MXT_TS_DEBUG_LOG( "offset=%04x, "
				"data=%02x%02x%02x%02x%02x%02x%02x%02x\n",
				tmp_offset, printbuf[0], printbuf[1], printbuf[2],
				printbuf[3], printbuf[4], printbuf[5],printbuf[6],
				printbuf[7] );
			tmp_buf += copy_size;
			tmp_offset += copy_size;
		}
	}
#endif
	
END:
	MXT_TS_DEBUG_LOG( "end. ret=%d\n", ret );
	return ret;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Write eMMC data for Nonvolatile driver.
 * @param[in]     *data The structure of Touch panel driver information.
 * @param[in]     offset Write eMMC data address offset.
 * @param[in]     size Write eMMC data size.
 * @param[in]     *buf Write eMMC data.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EINVAL Parameter error.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_write_mmap(struct mxt_data *data,
	unsigned int offset, size_t size, unsigned char *buf )
{
	int ret = 0;
	int error_count;
#ifdef CONFIG_TOUCH_LOG_DEBUG
	int pos = 0, copy_size, tmp_offset;
	unsigned char printbuf[ MXT_DEBUG_PRINT_SIZE ];
	unsigned char *tmp_buf;
#endif

	MXT_TS_INFO_LOG( "start. offset=%x, size=%x\n", offset, size );

	if( buf == NULL ) {
		MXT_TS_ERR_LOG( "parameter error.\n" );
		ret = -EINVAL;
		goto END;
	}
	for( error_count = 0; error_count <= MXT_ERROR_RETRY_MAX; error_count++ ) {
		ret = SetNONVOLATILE( buf, offset, size );
		if( ret != 0 ) {
			MXT_TS_ERR_LOG( "SetNONVOLATILE error. ret=%x\n", ret );
			continue;
		}
		break;
	}
	if( ret != 0 ) {
		/* error save */
		MXT_DRC_HARD_ERR( MXT_LITES_EMMC_ACCESS, ret, 0, 0, 0 );
	}
#ifdef CONFIG_TOUCH_LOG_DEBUG
	else {
		pos = size;
		tmp_buf = buf;
		tmp_offset = offset;
		memset( printbuf, 0, MXT_DEBUG_PRINT_SIZE );
		while( pos > 0 ) {
			if( pos >= MXT_DEBUG_PRINT_SIZE ) {
				copy_size = MXT_DEBUG_PRINT_SIZE;
				pos -= MXT_DEBUG_PRINT_SIZE;
			}
			else {
				copy_size = pos % MXT_DEBUG_PRINT_SIZE;
				pos -= copy_size;
			}
			memcpy( printbuf, tmp_buf, copy_size );
			MXT_TS_DEBUG_LOG( "offset=%04x, "
				"data=%02x%02x%02x%02x%02x%02x%02x%02x\n",
				tmp_offset, printbuf[0], printbuf[1], printbuf[2],
				printbuf[3], printbuf[4], printbuf[5],printbuf[6],
				printbuf[7] );
			tmp_buf += copy_size;
			tmp_offset += copy_size;
		}
	}
#endif
END:
	MXT_TS_DEBUG_LOG( "end. ret=%d\n", ret );
	return ret;
}
/* FUJITSU TEN:2012-08-08 mxt_read/write_mmap add. end */
/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Read or Create object table (memorymap table ).
 * @param[in,out] *data The structure of Touch panel driver information.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        !=0 Error end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_get_object_table(struct mxt_data *data)
{
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct device *dev = &data->client->dev;
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
	int error;
	int i;
	u16 reg;
	u16 end_address;
	u8 reportid = 0;
	u8 buf[MXT_OBJECT_SIZE];
	/* FUJITSU TEN:2012-08-06 variable add. start */
	struct mxt_emmc_header header;
	/* FUJITSU TEN:2012-08-06 variable add. end */
	data->mem_size = 0;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	/* FUJITSU TEN:2012-08-06 eMMC access add. start */
	/* eMMC data check */
	error = mxt_read_mmap( data, FTEN_NONVOL_TOUCH_MMAP_HEADER,
		sizeof( struct mxt_emmc_header ),
		( unsigned char* )&header );
	if( error < 0 ) {
		MXT_TS_ERR_LOG( "mxt_read_mmap error. : header data\n");
	}

	/* read object data for eMMC */
	if( error == 0 && ( header.write_flag == MXT_EMMC_WRITE_END ) ) {
		MXT_TS_INFO_LOG( "object memory map data read eMMC.\n");

		error = mxt_read_mmap( data, FTEN_NONVOL_TOUCH_MMAP_DATA,
			sizeof( struct mxt_object ) * data->info.object_num,
			( unsigned char* )( data->object_table ) );

		if( error == 0 ) {
			data->mem_size = header.mem_size;
			data->max_reportid = header.max_reportid;
			goto END;
		}
		else {
			MXT_TS_ERR_LOG( "mxt_read_mmap error. : object data\n");
		}	
	}
	/* FUJITSU TEN:2012-08-06 eMMC access add. end */

	/* FUJITSU TEN:2012-08-06 comment add. start */
	/* read object data for mxT540E register */
	/* FUJITSU TEN:2012-08-06 comment add. end */
	MXT_TS_INFO_LOG( "object memory map data read register.\n");
	for (i = 0; i < data->info.object_num; i++) {
		struct mxt_object *object = data->object_table + i;

		reg = MXT_OBJECT_START + MXT_OBJECT_SIZE * i;
		/* FUJITSU TEN:2012-08-10 parameter edit. start */
		error = mxt_read_object_table(data, reg, buf);
		/* FUJITSU TEN:2012-08-10 parameter edit. end */
		/* FUJITSU TEN:2012-08-08 error log add. start */
		if (error) {
			MXT_TS_ERR_LOG( "mxt_read_object_table error.\n");
			goto ERR;
		}
		/* FUJITSU TEN:2012-08-08 error log add. end */

		object->type = buf[0];
		object->start_address = (buf[2] << 8) | buf[1];
		object->size = buf[3] + 1;
		object->instances = buf[4] + 1;
		object->num_report_ids = buf[5];

		if (object->num_report_ids) {
			reportid += object->num_report_ids * object->instances;
			object->max_reportid = reportid;
			object->min_reportid = object->max_reportid -
				object->instances * object->num_report_ids + 1;
		}

		end_address = object->start_address
			+ object->size * object->instances - 1;

		if (end_address >= data->mem_size)
			data->mem_size = end_address + 1;

		/* FUJITSU TEN:2012-08-14 debug log edit. start */
		dev_dbg(dev, "T%u, start:0x%04x size:%u instances:%u "
		/* FUJITSU TEN:2012-08-14 debug log edit. end */
			"min_reportid:%u max_reportid:%u\n",
			object->type, object->start_address, object->size,
			object->instances,
			object->min_reportid, object->max_reportid);
	}

	/* Store maximum reportid */
	data->max_reportid = reportid;

	/* FUJITSU TEN:2012-08-06 eMMC access add. start */
	/* header and object data write */
	MXT_TS_INFO_LOG( "object memory map data write eMMC.\n");

	if( WARN_ON( ( sizeof(struct mxt_object) * data->info.object_num ) >
		FTEN_NONVOL_TOUCH_MMAP_DATA_SIZE ) ) {
		MXT_TS_ERR_LOG( "mxT540E memory map data size error.\n" );
		MXT_TS_ERR_LOG( "eMMC size:%d(0x%04x), write size:%d(0x%04x)\n",
			FTEN_NONVOL_TOUCH_MMAP_DATA_SIZE,
			FTEN_NONVOL_TOUCH_MMAP_DATA_SIZE,
			sizeof(struct mxt_object) * data->info.object_num,
			sizeof(struct mxt_object) * data->info.object_num );
		goto END;
	}

	header.mem_size = MXT_EMMC_WRITE_START;
	header.max_reportid = MXT_EMMC_WRITE_START;
	header.write_flag = MXT_EMMC_WRITE_START;
	error = mxt_write_mmap( data, FTEN_NONVOL_TOUCH_MMAP_HEADER,
		sizeof(struct mxt_emmc_header ), ( unsigned char* )&header );
	if( error < 0 ) {
		MXT_TS_ERR_LOG( "mxt_write_mmap error.\n");
		MXT_TS_ERR_LOG( "memory map header start data don't write eMMC.\n");
		goto END;
	}

	error = mxt_write_mmap( data, FTEN_NONVOL_TOUCH_MMAP_DATA,
		sizeof(struct mxt_object) * data->info.object_num,
		( unsigned char* )( data->object_table ) );
	if( error < 0 ) {
		MXT_TS_ERR_LOG( "mxt_write_mmap error.\n");
		MXT_TS_ERR_LOG( "memory map data don't write eMMC.\n");
		goto END;
	}

	header.mem_size = data->mem_size;
	header.max_reportid = data->max_reportid;
	header.write_flag = MXT_EMMC_WRITE_END;
	error = mxt_write_mmap( data, FTEN_NONVOL_TOUCH_MMAP_HEADER,
		sizeof(struct mxt_emmc_header ), ( unsigned char* )&header );
	if( error < 0 ) {
		MXT_TS_ERR_LOG( "mxt_write_mmap error.\n");
		MXT_TS_ERR_LOG( "memory map header end data don't write eMMC.\n");
		goto END;
	}

	/* FUJITSU TEN:2012-08-06 eMMC access add. end */
END:
	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return 0;

/* FUJITSU TEN:2012-08-08 error end add. start */
ERR:
	MXT_TS_DEBUG_LOG( "error. ret = %d\n", error );

	return error;
/* FUJITSU TEN:2012-08-08 error end add. end */
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Read X/Y Range and Orient for Touch panel device.
 * @param[in,out] *data The structure of Touch panel driver information.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        !=0 Error end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_read_resolution(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int error;
	unsigned int x_range, y_range;
	unsigned char orient;
	unsigned char val;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	/* Update matrix size in info struct */
	error = mxt_read_reg(client, MXT_MATRIX_X_SIZE, &val);
	if (error)
		/* FUJITSU TEN:2012-08-06 goto add. start */
		goto ERR;
		/* FUJITSU TEN:2012-08-06 goto add. end */
	data->info.matrix_xsize = val;

	error = mxt_read_reg(client, MXT_MATRIX_Y_SIZE, &val);
	if (error)
		/* FUJITSU TEN:2012-08-06 goto add. start */
		goto ERR;
		/* FUJITSU TEN:2012-08-06 goto add. end */
	data->info.matrix_ysize = val;

	/* Read X/Y size of touchscreen */
	error =  mxt_read_object(data, MXT_TOUCH_MULTI_T9,
			       MXT_TOUCH_XRANGE_MSB, &val);
	if (error)
		/* FUJITSU TEN:2012-08-06 goto add. start */
		goto ERR;
		/* FUJITSU TEN:2012-08-06 goto add. end */
	x_range = val << 8;

	error =  mxt_read_object(data, MXT_TOUCH_MULTI_T9,
			       MXT_TOUCH_XRANGE_LSB, &val);
	if (error)
		/* FUJITSU TEN:2012-08-06 goto add. start */
		goto ERR;
		/* FUJITSU TEN:2012-08-06 goto add. end */
	x_range |= val;

	error =  mxt_read_object(data, MXT_TOUCH_MULTI_T9,
			       MXT_TOUCH_YRANGE_MSB, &val);
	if (error)
		/* FUJITSU TEN:2012-08-06 goto add. start */
		goto ERR;
		/* FUJITSU TEN:2012-08-06 goto add. end */
	y_range = val << 8;

	error =  mxt_read_object(data, MXT_TOUCH_MULTI_T9,
			       MXT_TOUCH_YRANGE_LSB, &val);
	if (error)
		/* FUJITSU TEN:2012-08-06 goto add. start */
		goto ERR;
		/* FUJITSU TEN:2012-08-06 goto add. end */
	y_range |= val;

	error =  mxt_read_object(data, MXT_TOUCH_MULTI_T9,
			       MXT_TOUCH_ORIENT, &orient);
	if (error)
		/* FUJITSU TEN:2012-08-06 goto add. start */
		goto ERR;
		/* FUJITSU TEN:2012-08-06 goto add. end */

/* FUJITSU TEN:2012-07-03 Orient data set for BB. start */
#ifdef CONFIG_TOUCH_ORIENT_WRITE_BB
	orient = MXT_ORIENT_BB_VALUE;
	error =  mxt_write_object(data, MXT_TOUCH_MULTI_T9,
			       MXT_TOUCH_ORIENT, orient);
	if (error)
		/* FUJITSU TEN:2012-08-06 goto add. start */
		goto ERR;
		/* FUJITSU TEN:2012-08-06 goto add. end */

	error =  mxt_read_object(data, MXT_TOUCH_MULTI_T9,
			       MXT_TOUCH_ORIENT, &orient);
	if (error)
		/* FUJITSU TEN:2012-08-06 goto add. start */
		goto ERR;
		/* FUJITSU TEN:2012-08-06 goto add. end */
#endif
/* FUJITSU TEN:2012-07-03 Orient data set for BB. end */

	/* Handle default values */
	if (x_range == 0)
		x_range = 1023;

	if (y_range == 0)
		y_range = 1023;

	if (orient & MXT_XY_SWITCH) {
		data->max_x = y_range;
		data->max_y = x_range;
	} else {
		data->max_x = x_range;
		data->max_y = y_range;
	}

	/* FUJITSU TEN:2012-07-03 orient data add. start */
	dev_info(&client->dev,
			"Matrix Size X%uY%u Touchscreen size X%uY%u Orient %x\n",
			data->info.matrix_xsize, data->info.matrix_ysize,
			data->max_x, data->max_y, orient );
	/* FUJITSU TEN:2012-07-03 orient data add. end */

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return 0;
/* FUJITSU TEN:2012-08-06 error log add. start */
ERR:
	MXT_TS_ERR_LOG( "Information Block register data get error. %x\n", error );
	return error;	
/* FUJITSU TEN:2012-08-06 error log add. start */
}

/* FUJITSU TEN:2012-11-19 read parameter for eMMC add. start */
/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Read driver parameter for Nonvolatile driver.
 * @param[in,out] *data The structure of Touch panel driver information.
 * @return        None.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static void mxt_read_parameter(struct mxt_data *data)
{
	int error;

	MXT_TS_INFO_LOG( "start.\n" );

	error = GetNONVOLATILE( ( unsigned char* )&data->tsns_reset_time,
		FTEN_NONVOL_TOUCH_WAIT_SYSTEM_RESET, sizeof( u16 ) );
	if (error) {
		MXT_TS_ERR_LOG( "GetNONVOLATILE error : TSNS_RESET time.\n" );
		data->tsns_reset_time = MXT540E_TSNS_RESET_TIME;
	}
	else if (data->tsns_reset_time < MXT_TSNS_RESET_TIME_MIN) {
		MXT_TS_ERR_LOG( "eMMC TSNS_RESET time error. value:%d\n",
			data->tsns_reset_time );
		data->tsns_reset_time = MXT540E_TSNS_RESET_TIME;
	}
	MXT_TS_DEBUG_LOG( "TSNS_RESET time set value : %d\n",
		data->tsns_reset_time );

	error = GetNONVOLATILE( ( unsigned char* )&data->tsns_reset_exec_time,
		FTEN_NONVOL_TOUCH_EXEC_SYSTEM_RESET, sizeof( u16 ) );
	if (error) {
		MXT_TS_ERR_LOG( "GetNONVOLATILE error : TSNS_RESET exec time.\n" );
		data->tsns_reset_exec_time = MXT_TSNS_RESET_EXEC_TIME;
	}
	else if (data->tsns_reset_exec_time < MXT_TSNS_RESET_EXEC_TIME_MIN) {
		MXT_TS_ERR_LOG( "eMMC TSNS_RESET exec time error. value:%d\n",
			data->tsns_reset_exec_time );
		data->tsns_reset_exec_time = MXT_TSNS_RESET_EXEC_TIME;
	}
	MXT_TS_DEBUG_LOG( "TSNS_RESET exec time set value : %d\n",
		data->tsns_reset_exec_time );

	error = GetNONVOLATILE( ( unsigned char* )&data->hwerror_polling_time,
		FTEN_NONVOL_TOUCH_POLL_DEFAULT, sizeof( u16 ) );
	if (error) {
		MXT_TS_ERR_LOG( "GetNONVOLATILE error : Error polling time.\n" );
		data->hwerror_polling_time = MXT_ERROR_POLLING_TIME;
	}
	else if (data->hwerror_polling_time < MXT_ERROR_POLLING_TIME_MIN) {
		MXT_TS_ERR_LOG( "eMMC Error polling time error. value:%d\n",
			( u16 )data->hwerror_polling_time );
		data->hwerror_polling_time = MXT_ERROR_POLLING_TIME;
	}
	MXT_TS_DEBUG_LOG( "hardware error polling time set value : %d\n",
		( u16 )data->hwerror_polling_time );

	error = GetNONVOLATILE( ( unsigned char* )&data->hwerror_count_max,
		FTEN_NONVOL_TOUCH_POLL_THRESHOLD, sizeof( u8 ) );
	if (error) {
		MXT_TS_ERR_LOG( "GetNONVOLATILE error : Error polling count.\n" );
		data->hwerror_count_max = MXT_ERROR_POLLING_COUNT;
	}
	else if (data->hwerror_count_max < MXT_ERROR_POLLING_COUNT_MIN) {
		MXT_TS_ERR_LOG( "eMMC Error polling count error. value:%d\n",
			data->hwerror_count_max );
		data->hwerror_count_max = MXT_ERROR_POLLING_COUNT;
	}
	MXT_TS_DEBUG_LOG( "hardware error polling count set value : %d\n", 
		data->hwerror_count_max );

	error = GetNONVOLATILE( ( unsigned char* )&data->test_wait_time,
		FTEN_NONVOL_TOUCH_SELFTEST_WAIT_TIME, sizeof( u16 ) );
	if (error) {
		MXT_TS_ERR_LOG( "GetNONVOLATILE error : Self test execute wait time.\n" );
		data->test_wait_time = MXT_SELFTEST_WAIT_TIME;
	}
	else if (data->test_wait_time < MXT_SELFTEST_WAIT_TIME_MIN) {
		MXT_TS_ERR_LOG( "eMMC Self test execute wait time error. value:%d\n",
			data->test_wait_time );
		data->test_wait_time = MXT_SELFTEST_WAIT_TIME;
	}
	MXT_TS_DEBUG_LOG( " Self test execute wait time : %d\n", 
		data->test_wait_time );

	error = GetNONVOLATILE( ( unsigned char* )&data->test_retry_count,
		FTEN_NONVOL_TOUCH_SELFTEST_EXEC_COUNT, sizeof( u8 ) );
	if (error) {
		MXT_TS_ERR_LOG( "GetNONVOLATILE error : Self test retry count.\n" );
		data->test_retry_count = MXT_SELFTEST_RETRY_COUNT;
	}
	else if (data->test_retry_count < MXT_SELFTEST_RETRY_COUNT_MIN) {
		MXT_TS_ERR_LOG( "eMMC Self test retry count error. value:%d\n",
			data->test_retry_count );
		data->test_retry_count = MXT_SELFTEST_RETRY_COUNT;
	}
	MXT_TS_DEBUG_LOG( " Self test retry count : %d\n", 
		data->test_retry_count );

	MXT_TS_DEBUG_LOG( "end.\n" );

	return;
}
/* FUJITSU TEN:2012-11-19 read parameter for eMMC add. end */

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Initialize Touch panel driver device.
 * @param[in,out] *data The structure of Touch panel driver information.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -ENOMEM No memory.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_initialize(struct mxt_data *data)
{
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct i2c_client *client = data->client;a
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
	struct mxt_info *info = &data->info;
	int error;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */
	
	error = mxt_get_info(data);
	if (error) {
/* FUJITSU TEN:2012-06-19 comment out. start */
#if 0
		error = mxt_probe_bootloader(data);

		if (error) {
#endif
/* FUJITSU TEN:2012-06-19 comment out. end */
			return error;
/* FUJITSU TEN:2012-06-19 comment out. start */
#if 0
		} else {
			data->state = BOOTLOADER;
			return 0;
		}
#endif
/* FUJITSU TEN:2012-06-19 comment out. start */
	}

	dev_info(&client->dev,
		"Family ID: %d Variant ID: %d Version: %d.%d "
		"Build: 0x%02X Object Num: %d\n",
		info->family_id, info->variant_id,
		info->version >> 4, info->version & 0xf,
		info->build, info->object_num);

	data->state = APPMODE;

	data->object_table = kcalloc(info->object_num,
				     sizeof(struct mxt_object),
				     GFP_KERNEL);
	if (!data->object_table) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		return -ENOMEM;
	}

	/* Get object table information */
	error = mxt_get_object_table(data);
	if (error) {
		dev_err(&client->dev, "Error %d reading object table\n", error);
		return error;
	}

	error = mxt_check_message_length(data);
	if (error)
		return error;

	error = mxt_probe_power_cfg(data);
	if (error) {
		dev_err(&client->dev, "Failed to initialize power cfg\n");
		return error;
	}
/* FUJITSU TEN:2012-06-19 comment out. start */
#if 0
	/* Check register init values */
	error = mxt_check_reg_init(data);
	if (error) {
		dev_err(&client->dev, "Failed to initialize config\n");
		return error;
	}
#endif
/* FUJITSU TEN:2012-06-19 comment out. end */
	error = mxt_read_resolution(data);
	if (error) {
		dev_err(&client->dev, "Failed to initialize screen size\n");
		return error;
	}

/* FUJITSU TEN:2012-06-19 initialize DEEPSLEEP mode add. start */
#ifdef CONFIG_TOUCH_INIT_DEEPSLEEP_SETTING
	error = mxt_set_power_cfg(data, MXT_POWER_CFG_DEEPSLEEP);
	if (error) {
		dev_err(&client->dev, "Error %d power mode change for deepsleep.\n",
			error);
		return error;
	}
#else
	data->is_stopped = 1;
#endif
/* FUJITSU TEN:2012-06-19 initialize DEEPSLEEP mode add. end */

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return 0;
}

static int mxt_load_fw(struct device *dev, const char *fn)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	const struct firmware *fw = NULL;
	unsigned int frame_size;
	unsigned int pos = 0;
	unsigned int retry = 0;
	int ret;

	ret = request_firmware(&fw, fn, dev);
	if (ret < 0) {
		dev_err(dev, "Unable to open firmware %s\n", fn);
		return ret;
	}

	if (data->state != BOOTLOADER) {
		/* Change to the bootloader mode */
		ret = mxt_soft_reset(data, MXT_BOOT_VALUE);
		if (ret)
			goto release_firmware;

		ret = mxt_get_bootloader_address(data);
		if (ret)
			goto release_firmware;

		data->state = BOOTLOADER;
	}

	ret = mxt_check_bootloader(data, MXT_WAITING_BOOTLOAD_CMD);
	if (ret) {
		/* Bootloader may still be unlocked from previous update
		 * attempt */
		ret = mxt_check_bootloader(data, MXT_WAITING_FRAME_DATA);

		if (ret) {
			data->state = FAILED;
			goto release_firmware;
		}
	} else {
		dev_info(dev, "Unlocking bootloader\n");

		/* Unlock bootloader */
		mxt_unlock_bootloader(data);
	}

	while (pos < fw->size) {
		ret = mxt_check_bootloader(data, MXT_WAITING_FRAME_DATA);
		if (ret) {
			data->state = FAILED;
			goto release_firmware;
		}

		frame_size = ((*(fw->data + pos) << 8) | *(fw->data + pos + 1));

		/* Take account of CRC bytes */
		frame_size += 2;

		/* Write one frame to device */
		mxt_fw_write(data, fw->data + pos, frame_size);

		ret = mxt_check_bootloader(data, MXT_FRAME_CRC_PASS);
		if (ret) {
			retry++;

			/* Back off by 20ms per retry */
			msleep(retry * 20);

			if (retry > 20) {
				data->state = FAILED;
				goto release_firmware;
			}
		} else {
			retry = 0;
			pos += frame_size;
			dev_info(dev, "Updated %d/%zd bytes\n", pos, fw->size);
		}
	}

	data->state = APPMODE;

release_firmware:
	release_firmware(fw);
	return ret;
}

static ssize_t mxt_update_fw_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int error;

	disable_irq(data->irq);

	error = mxt_load_fw(dev, MXT_FW_NAME);
	if (error) {
		dev_err(dev, "The firmware update failed(%d)\n", error);
		count = error;
	} else {
		dev_info(dev, "The firmware update succeeded\n");

		/* Wait for reset */
		msleep(MXT_FWRESET_TIME);

		data->state = INIT;
		kfree(data->object_table);
		data->object_table = NULL;

		mxt_initialize(data);
	}

	if (data->state == APPMODE) {
		enable_irq(data->irq);

		error = mxt_make_highchg(data);
		if (error)
			return error;
	}

	return count;
}

static ssize_t mxt_pause_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	ssize_t count;
	char c;

	c = data->driver_paused ? '1' : '0';
	count = sprintf(buf, "%c\n", c);

	return count;
}

static ssize_t mxt_pause_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int i;

	if (sscanf(buf, "%u", &i) == 1 && i < 2) {
		data->driver_paused = (i == 1);
		dev_dbg(dev, "%s\n", i ? "paused" : "unpaused");
		return count;
	} else {
		dev_dbg(dev, "pause_driver write error\n");
		return -EINVAL;
	}
}

static ssize_t mxt_debug_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int count;
	char c;

	c = data->debug_enabled ? '1' : '0';
	count = sprintf(buf, "%c\n", c);

	return count;
}

static ssize_t mxt_debug_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int i;

	if (sscanf(buf, "%u", &i) == 1 && i < 2) {
		data->debug_enabled = (i == 1);

		dev_dbg(dev, "%s\n", i ? "debug enabled" : "debug disabled");
		return count;
	} else {
		dev_dbg(dev, "debug_enabled write error\n");
		return -EINVAL;
	}
}

static int mxt_check_mem_access_params(struct mxt_data *data, loff_t off,
				       size_t *count)
{
	if (data->state != APPMODE) {
		dev_err(&data->client->dev, "Not in APPMODE\n");
		return -EINVAL;
	}

	if (off >= data->mem_size)
		return -EIO;

	if (off + *count > data->mem_size)
		*count = data->mem_size - off;

	if (*count > MXT_MAX_BLOCK_WRITE)
		*count = MXT_MAX_BLOCK_WRITE;

	return 0;
}

static ssize_t mxt_mem_access_read(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off, size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct mxt_data *data = dev_get_drvdata(dev);
	int ret = 0;

	ret = mxt_check_mem_access_params(data, off, &count);
	if (ret < 0)
		return ret;

	if (count > 0)
		ret = __mxt_read_reg(data->client, off, count, buf);

	return ret == 0 ? count : ret;
}

static ssize_t mxt_mem_access_write(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off,
	size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct mxt_data *data = dev_get_drvdata(dev);
	int ret = 0;

	ret = mxt_check_mem_access_params(data, off, &count);
	if (ret < 0)
		return ret;

	if (count > 0)
		ret = mxt_write_block(data->client, off, count, buf);

	return ret == 0 ? count : 0;
}

/* FUJITSU TEN:2012-10-10 hardware error polling add. start */
/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Hardware error polling timer start function.
 * @param[in,out] *data The structure of Touch panel driver information.
 * @return        None.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static void mxt_timer_start( struct mxt_data *data ) {

	MXT_TS_INFO_LOG( "start.\n" );

	hrtimer_start( &( data->timer ),
		ns_to_ktime( ( data->hwerror_polling_time)*1000*1000 ),
		HRTIMER_MODE_REL );

	MXT_TS_DEBUG_LOG( "end.\n" );

}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Hardware error polling timer stop function.
 * @param[in,out] *data The structure of Touch panel driver information.
 * @return        None.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static void mxt_timer_stop( struct mxt_data *data ) {

	MXT_TS_INFO_LOG( "start.\n" );

	hrtimer_cancel( &( data->timer ) );

	MXT_TS_DEBUG_LOG( "end.\n" );

}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Hardware error polling timer execute handler.
 * @param[in]     *timer The structure of High-resolution timers.
 * @return        Result.
 * @retval        HRTIMER_NORESTERT Timer function end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static enum hrtimer_restart mxt_timer_func( struct hrtimer *timer ) {
	struct mxt_data *data;

	MXT_TS_INFO_LOG( "start.\n" );
	data = container_of( timer, struct mxt_data, timer );
	queue_work( data->wq, &data->work );

	MXT_TS_DEBUG_LOG( "end.\n" );
	return HRTIMER_NORESTART;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Hardware error polling work queue handler.
 * @param[in,out] *work The structure of work queue.
 * @return        None.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static void mxt_work_func( struct work_struct *work ) {
	struct mxt_data *data;
	unsigned int error;
	u8 val;
#ifdef CONFIG_TOUCH_STATISTICAL_INFO_LOG
	unsigned char trc_data[ MXT_LITES_TRACE_DATA_SIZE ] = { 0 };
#endif

	MXT_TS_INFO_LOG( "start.\n" );
	data = container_of( work, struct mxt_data, work );

	MXT_TS_DEBUG_LOG( "execute hardware error polling.\n" );
	error = mxt_read_reg(data->client, MXT_FAMILY_ID, &val);

	if (error) {
		if( mxt_read_stop == MXT_READ_STOP ) {
			goto SKIP;
		}
		data->hwerror_count++;
		MXT_TS_ERR_LOG( "get hardware error. count=%d\n",
			data->hwerror_count );
		if( data->hwerror_count == data->hwerror_count_max ) {
			MXT_TS_ERR_LOG( "hardware error polling stop.\n" );
			MXT_DRC_ERR( MXT_LITES_ERROR_POLLING, 0, 0, 0, 0 );
		}
	}
	else {
		data->hwerror_count = 0;
		MXT_TS_DEBUG_LOG( "not get hardware error. count=%d\n",
			data->hwerror_count );
	}

	if( ( ( data->fs_count % MXT_CALIBRATE_INTERVAL_TIME ) == 0 ) &&
		data->fs_count != 0 ){
		if( ( data->intr_count == data->last_intr_count_fs ) &&
			( data->touch_count == data->last_touch_count_fs ) ) {
			mxt_write_object(data, MXT_GEN_COMMAND_T6,
				MXT_COMMAND_CALIBRATE, MXT_CALIBRATE_VALUE );
		}
		else {
			data->last_intr_count_fs = data->intr_count;
			data->last_touch_count_fs = data->touch_count;
		}
	}
	data->fs_count += ( (u32)data->hwerror_polling_time / 1000 );

#ifdef CONFIG_TOUCH_STATISTICAL_INFO_LOG
	if( ( data->slog_count % MXT_SINFO_LOG_INTERVAL_TIME ) == 0 ) {
		sprintf( trc_data, "touch:I TS=%c intr=%x set=%x\n",
			( ( data->pdata->read_chg() ) ?
				MXT_SINFO_LOG_GPIO_H :
				MXT_SINFO_LOG_GPIO_L ),
			( data->intr_count % MXT_SINFO_LOG_MASK ),
			( data->touch_count % MXT_SINFO_LOG_MASK ) );
		MXT_DRC_TRC( trc_data );
		data->last_intr_count = data->intr_count;
		data->last_touch_count = data->touch_count;
	} else if( ( data->slog_count % MXT_SINFO_LOG_INTERVAL_STIME ) == 0 ) {
		if( ( data->intr_count != data->last_intr_count ) ||
			( data->touch_count != data->last_touch_count ) ) {
			sprintf( trc_data, "touch:D TS=%c intr=%x set=%x\n",
				( ( data->pdata->read_chg() ) ?
					MXT_SINFO_LOG_GPIO_H :
					MXT_SINFO_LOG_GPIO_L ),
				( data->intr_count % MXT_SINFO_LOG_MASK ),
				( data->touch_count % MXT_SINFO_LOG_MASK ) );
			MXT_DRC_TRC( trc_data );
			data->last_intr_count = data->intr_count;
			data->last_touch_count = data->touch_count;
		}	
	}
	data->slog_count += ( (u32)data->hwerror_polling_time / 1000 );
#endif
	if( data->hwerror_count < data->hwerror_count_max &&
		data->error_polling_flag == MXT_ERROR_POLLING_EXEC ) {
		mxt_timer_start( data );
	}
SKIP:
	MXT_TS_DEBUG_LOG( "end.\n" );
}

/* FUJITSU TEN:2012-10-10 hardware error polling add. end */

/* FUJITSU TEN:2012-07-31 update config add. start */
/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Write config data for Touch panel device.
 * @param[in,out] *data The structure of Touch panel driver information.
 * @param[in]     *buf Set config data.
 * @param[in]     count Set config data size
 * @param[in]     mode Exec mode (Write or Check).
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EINVAL Config data format error.
 * @retval        -EIO Config data compare check error.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_write_config_data(struct mxt_data *data, char *buf,
	size_t count, char mode )
{
	struct mxt_object *object;
	int pos;
	int i;
	unsigned int type, instance, size;
	int val;
	u16 reg;
	int ret = 0;
	char write_buf[MXT_MAX_BLOCK_WRITE];
	char read_buf[MXT_MAX_BLOCK_WRITE];
#ifdef CONFIG_TOUCH_LOG_DEBUG
	char log_buf[MXT_UPDATE_DEBUG_LOG];
	int j;
#endif
	pos = 0;

	MXT_TS_INFO_LOG( "start.\n" );

	while (pos < count) {
		MXT_TS_DEBUG_LOG( "data position %x : data count %x\n",
			pos, count );

		/* Read type, instance, length */
		if( ( pos + MXT_CONFIG_HEADER_SIZE ) >= count ) {
			MXT_TS_ERR_LOG( "Bad config data format.\n");
			ret = -EINVAL;
			goto ERR;
		}
		type = *( buf + pos );
		instance = *( buf + pos + 1);
		size = *(buf + pos + 2 );

		pos += MXT_CONFIG_HEADER_SIZE;

		object = mxt_get_object(data, type);
		if (!object) {
			ret = -EINVAL;
			goto ERR;
		}

		if (size > object->size) {
			MXT_TS_ERR_LOG( "Object length error.\n" );
			ret = -EINVAL;
			goto ERR;
		}

		if (instance >= object->instances) {
			MXT_TS_ERR_LOG( "Object instances error.\n");
			ret = -EINVAL;
			goto ERR;
		}

		MXT_TS_DEBUG_LOG( "config data : T%d, instance %x, size %x",
			type, instance, size );

		reg = object->start_address + object->size * instance;

		if( ( pos + size ) > count ) {
			MXT_TS_ERR_LOG( "Bad config data format.\n");
			ret = -EINVAL;
			goto ERR;
		}

		memset( write_buf, 0, MXT_MAX_BLOCK_WRITE );
		for (i = 0; i < object->size; i++) {
			if( i < size ) {
				val = *( buf + pos );
				pos++;
			}
			else {
				val = 0;
			}
			write_buf[ i ] = val;
		}

#ifdef CONFIG_TOUCH_LOG_DEBUG
			j = 0;
			MXT_TS_DEBUG_LOG( "### write config data dump ###\n" );
			while( j < object->size ) {
				memset( log_buf, 0, MXT_UPDATE_DEBUG_LOG );
				for (i = 0; i < MXT_DEBUG_PRINT_SIZE; i++, j++ ) {
					sprintf( log_buf, "%s%02x", log_buf,
						write_buf[ j ] );
				}
				MXT_TS_DEBUG_LOG( "offset:%02x, data:%s\n",
					j - MXT_DEBUG_PRINT_SIZE, log_buf );
			}
#endif

		if( mode == MXT_UPDATE_MODE_WRITE ) {
			ret = mxt_write_block(data->client, reg,
				object->size, write_buf);
			if( ret != 0 ) {
				MXT_TS_ERR_LOG( "write error. reg=%x\n", reg );
				goto ERR;
			}
		}
		else {
			memset( read_buf, 0, MXT_MAX_BLOCK_WRITE );
			ret = __mxt_read_reg(data->client, reg, object->size,
				read_buf);
			if( ret != 0 ) {
				MXT_TS_ERR_LOG( "read error. reg=%x\n", reg );
				goto ERR;
			}

#ifdef CONFIG_TOUCH_LOG_DEBUG
			j = 0;
			MXT_TS_DEBUG_LOG( "### read config data dump ###\n" );
			while( j < object->size ) {
				memset( log_buf, 0, MXT_UPDATE_DEBUG_LOG );
				for (i = 0; i < MXT_DEBUG_PRINT_SIZE; i++, j++ ) {
					sprintf( log_buf, "%s%02x", log_buf,
						read_buf[ j ] );
				}
				MXT_TS_DEBUG_LOG( "offset:%02x, data:%s\n",
					j - MXT_DEBUG_PRINT_SIZE, log_buf );
			}
#endif
			if( memcmp( write_buf, read_buf, object->size ) ) {
				MXT_TS_ERR_LOG( "config data compare check error\n");
				ret = -EIO;
				goto ERR;
			}
#ifdef CONFIG_TOUCH_LOG_DEBUG
			else {
				MXT_TS_DEBUG_LOG( "config data compare check OK\n" );
			}
#endif
		}
	}

ERR:
	MXT_TS_DEBUG_LOG( "end. ret=%d\n", ret );

	return ret;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Execute config data update for Touch panel device.
 * @param[in,out] *data The structure of Touch panel driver information.
 * @param[in]     *buf Set config data.
 * @param[in]     count Set config data size
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EINVAL Parameter error.
 * @retval        -EIO Device I/O error.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_exec_update_config(struct mxt_data *data, char *buf,
	size_t count)
{
	int ret = 0;
	int reset_count = 0, error_count;
	int timeout_counter = 0;
	unsigned int error_code = 0;

	MXT_TS_INFO_LOG( "start.\n" );

	if( buf == NULL || count > MXT_CONFIG_DATA_SIZE ) {
		MXT_TS_ERR_LOG( "update config data error.\n" );
		MXT_DRC_ERR( MXT_LITES_UPDATE_CONFIG_FMT, 0, 0, 0, 0 );
		ret = -EINVAL;
		goto ERR;
	}

	while( reset_count <= MXT_RESET_RETRY_MAX ) {
		/* Write update config data */
		for( error_count = 0; error_count <= MXT_ERROR_RETRY_MAX;
			error_count++ ) {

			ret = mxt_write_config_data( data, buf, count,
				MXT_UPDATE_MODE_WRITE );

			if( ret == -EINVAL ) {
				MXT_TS_ERR_LOG( "update config stop.\n" );
				MXT_DRC_ERR( MXT_LITES_UPDATE_CONFIG_FMT,
					0, 0, 0, 0 );
				MXT_TS_DEBUG_LOG( "### error reset.\n" );
				if( mxt_tsns_reset(data) ) {
					MXT_TS_ERR_LOG( "error reset error.\n" );
				}
				goto ERR;
			}

			spin_lock_irq( &mxt_config_lock );
			if( mxt_config_status  || ret != 0 ) {
				if( ret == 0 ) {
					ret = -EIO;
				}
				mxt_config_status = MXT_FLAG_CLEAR;
				spin_unlock_irq( &mxt_config_lock );
				MXT_TS_ERR_LOG( "config write error.\n" );
				error_code = MXT_UPDATE_CONFIG_HARD_WRITE;
				/* error retry */
				if( error_count < MXT_ERROR_RETRY_MAX ) {
					msleep(MXT_ERROR_RETRY_WAIT);
					MXT_TS_DEBUG_LOG(
						"error retry. count:%x\n",
						error_count );
					continue;
				}
				/* reset retry */
				else {
					goto RESET_RETRY;
				}
			}
			spin_unlock_irq( &mxt_config_lock );

			break;
		}

		/* Backup to memory */
		for( error_count = 0; error_count <= MXT_ERROR_RETRY_MAX;
			error_count++ ) {
			ret = mxt_write_object(data, MXT_GEN_COMMAND_T6,
					MXT_COMMAND_BACKUPNV,
					MXT_BACKUP_VALUE);
			if( ret == 0 ) {
				msleep(MXT_BACKUP_TIME);
				while (((timeout_counter++ <= MXT_IRQ_TIMEOUT_COUNT)
					&& (data->pdata->read_chg()==0))
					&& !( mxt_read_stop ) )
					msleep(MXT_IRQ_CHECK_INTERVAL);
				if (timeout_counter > MXT_IRQ_TIMEOUT_COUNT) {
					MXT_TS_ERR_LOG( "BACKUPNV timeout.\n" );
					ret = -EIO;
				}
			}

			spin_lock_irq( &mxt_config_lock );
			if( mxt_config_status || ret != 0 ) {
				if( ret == 0 ) {
					ret = -EIO;
				}
				mxt_config_status = MXT_FLAG_CLEAR;
				spin_unlock_irq( &mxt_config_lock );
				MXT_TS_ERR_LOG( "BACKUPNV error.\n" );
				error_code = MXT_UPDATE_CONFIG_HARD_BACKUP;
				/* error retry */
				if( count < MXT_ERROR_RETRY_MAX ) {
					msleep( MXT_ERROR_RETRY_WAIT );
					MXT_TS_DEBUG_LOG(
						"error retry. count:%x\n",
						error_count );
					continue;
				}
				/* reset retry */
				else {
					goto RESET_RETRY;
				}
			}
			spin_unlock_irq( &mxt_config_lock );
			break;
		}

		/* WRITE check */
		ret = mxt_write_config_data( data, buf, count,
			MXT_UPDATE_MODE_CHECK );
		if( ret != 0 ) {
			MXT_TS_ERR_LOG( "config write data error.\n" );
			error_code = MXT_UPDATE_CONFIG_HARD_WRITE;
			/* reset retry */
			goto RESET_RETRY;
		}
		
		/* TSNS_RESET */
		ret = mxt_tsns_reset(data);
		if( ret != 0 ) {
			MXT_TS_ERR_LOG( "TSNS_RESET error.\n" );
			MXT_DRC_HARD_ERR( MXT_LITES_UPDATE_CONFIG_HARD,
				MXT_UPDATE_CONFIG_HARD_RESET, 0, 0, 0 );
			goto ERR;
		}

		/* finish config update */
		break;

RESET_RETRY:
		if( mxt_tsns_reset(data) ) {
			MXT_TS_ERR_LOG( "reset retry error.\n" );
			MXT_DRC_HARD_ERR( MXT_LITES_UPDATE_CONFIG_HARD,
				MXT_UPDATE_CONFIG_HARD_RESET, 0, 0, 0 );
			goto ERR;
		}
		reset_count++;
#ifdef CONFIG_TOUCH_LOG_DEBUG
		if( reset_count <= MXT_RESET_RETRY_MAX ) {
			MXT_TS_DEBUG_LOG( "### reset retry. count:%x\n",
				reset_count );
		}
		else {
			MXT_TS_DEBUG_LOG( "### error reset.\n" );
		}
#endif
	}
	if( reset_count == MXT_RESET_RETRY_MAX ) {
		/* error save */
		MXT_DRC_HARD_ERR( MXT_LITES_UPDATE_CONFIG_HARD,
			error_code, 0, 0, 0 );
	}
ERR:
	MXT_TS_DEBUG_LOG( "end.\n" );
	return ret;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Sysfs interface for Touch panel device update config data.
 * @param[in]     *filp File pointer.
 * @param[in]     *kobj Touch panel driver kobject.
 * @param[in]     *bin_attr The structure of binary attribute.
 * @param[in]     *buf Set config data.
 * @param[in]     off Set config data offset.
 * @param[in]     count Set config data size
 * @return        Write buffer size.
 * @retval        <=0 Normal end.
 * @retval        >0 Error end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static ssize_t mxt_update_config_write(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off,
	size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct mxt_data *data = dev_get_drvdata(dev);
	int ret = 0;

	MXT_TS_INFO_LOG( "start.\n" );

	data->error_polling_flag = MXT_ERROR_POLLING_STOP;
	mxt_timer_stop( data );
	cancel_work_sync( &data->work );

	if( count > 0 ) {
		ret = mxt_exec_update_config(data, buf, count);
		if( ret ) {
			MXT_TS_ERR_LOG( "mxt_exec_update_config error ret=%d\n",
				ret);
			goto END;
		}
	}
	else {
		MXT_TS_INFO_LOG( "config data size zero.\n" );
		goto END;
	}

	ret = mxt_probe_power_cfg(data);
	if( ret ) {
		MXT_TS_ERR_LOG( "Failed to initialize power cfg.\n");
		goto END;
	}

	ret = mxt_read_resolution(data);
	if( ret ) {
		MXT_TS_ERR_LOG( "Failed to initialize screen size\n");
		goto END;
	}

#ifdef CONFIG_TOUCH_INIT_DEEPSLEEP_SETTING
	/* reset power mode setting */
	if (data->is_stopped) {
		ret = mxt_set_power_cfg(data, MXT_POWER_CFG_DEEPSLEEP);
		if( ret ) {
			MXT_TS_ERR_LOG(
				"Error %d power mode change for deepsleep.\n", ret);
			goto END;
		}
	}
#else
	if ( !(data->is_stopped) ) {
		ret = mxt_set_power_cfg(data, MXT_POWER_CFG_RUN);
		if( ret ) {
			MXT_TS_ERR_LOG(
				"Error %d power mode change for run.\n", ret);
			goto END;
		}
	}
#endif

	ret = count;
END:
	MXT_TS_DEBUG_LOG( "end. ret=%d\n", ret );

	return ret;
}
/* FUJITSU TEN:2012-07-31 update config add. end */

/* FUJITSU TEN:2013-04-05 show config version IF(sysfs) add. start */
/**
 * @brief         Sysfs interface for show config version.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr The structure of device attribute.
 * @param[in]     *buf Set polling data.
 * @param[in]     count Set data size.
 * @return        Write buffer size.
 * @retval        <=0 Normal end.
 * @retval        -EIO I2c access error.
 * @retval        -ESHUTDOWN Fastboot executing.
 * @retval        -EBADMSG Version data error.
 */
static ssize_t mxt_config_version_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int i;
	int ret = 0;
	int error;
	unsigned char read_data[ MXT_CONFIG_VERSION_SIZE ] = {0};
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	MXT_TS_INFO_LOG( "start.\n" );

	for( i = 0; i < MXT_CONFIG_VERSION_SIZE; i++ ) {

		error =  mxt_read_object(data, MXT_SPT_USERDATA_T38,
			i, &read_data[ i ] );
		if( error ) {
			MXT_TS_ERR_LOG(
				"mxt_read_object error. %d\n", error);
			ret = error;
			break;
		}
		else if( mxt_read_stop ) {
			ret = -ESHUTDOWN;
			break;
		}
		if( ! ( ( ( read_data[ i ] >= MXT_VERSION_DATA_0 ) &&
			( read_data[ i ] <= MXT_VERSION_DATA_9 ) ) ||
			( ( read_data[ i ] >= MXT_VERSION_DATA_A ) &&
			( read_data[ i ] <= MXT_VERSION_DATA_Z ) ) ) ) {
			MXT_TS_ERR_LOG( "read data error. 0x%02x\n",
				read_data[ i ] );
			ret = -EBADMSG;
			break;
		}
	}
	MXT_TS_DEBUG_LOG( "end. ret=%d\n", ret );

	if( ret ) {
		return ret;
	}
	else {
		return sprintf(buf, "%c%c%c%c%c%c\n",
			read_data[ 0 ], read_data[ 1 ], read_data[ 2 ],
			read_data[ 3 ], read_data[ 4 ], read_data[ 5 ] );
	}
}

static DEVICE_ATTR(version, 0664, mxt_config_version_show, NULL );
/* FUJITSU TEN:2013-04-05 show config version IF(sysfs) add. end */

/* FUJITSU TEN:2013-06-07 debug function add. start */
/**
 * @brief         Sysfs interface for debug function execute.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr The structure of device attribute.
 * @param[in]     *buf Set polling data.
 * @return        read buffer size.
 * @retval        <=0 Normal end.
 * @retval        -EINVAL Parameter error.
 */
static ssize_t mxt_debug_touch_info_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int count;

	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	MXT_TS_INFO_LOG( "start.\n" );


	count = sprintf(buf,
		"touch interrupt count 0x%x\nlast interrupt time 0x%lx\n"
		"touch input set count 0x%x\nlast input set time 0x%lx\n",
		data->intr_count, data->intr_tv.tv_sec,
		data->touch_count, data->touch_tv.tv_sec);


	MXT_TS_DEBUG_LOG( "end.\n" );
	return count;
}

static DEVICE_ATTR(touch_info, 0444, mxt_debug_touch_info_show, NULL );
/* FUJITSU TEN:2013-06-07 debug function add. endt */

/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. start */
/**
 * @brief         Sysfs interface for execute self test.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr The structure of device attribute.
 * @param[in]     *buf Set polling data.
 * @param[in]     count Set data size.
 * @return        Write buffer size.
 * @retval        <=0 Normal end.
 * @retval        -EAGAIN touch device not RUN mode.
 * @retval        -EIO I2c access error.
 * @retval        -ENOEXEC TSNS_STATUS No change.
 * @retval        -ESHUTDOWN Fastboot executing.
 */
static ssize_t mxt_self_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int i;
	int ret = 0;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	MXT_TS_INFO_LOG( "start.\n" );

	data->test_flag = 0;
	memset( data->test_data, 0, sizeof( u8 ) * MXT_SELFTEST_RESULT_SIZE );

	if (data->is_stopped) {
		MXT_TS_ERR_LOG( "touch panel device is DEEPSLEEP mode.\n" );
		ret = -EAGAIN;
		goto END;
	}

	error = mxt_write_object( data, MXT_SPT_SELFTEST_T25,
		MXT_SELFTEST_CTRL, MXT_SELFTEST_CTRL_DISABLE );
	if( error ) {
		MXT_TS_ERR_LOG(
			"mxt_write_object error : CTRL DISABLE. %d\n", error);
		ret = error;
		goto END;
	}

	for( i = 0; i < data->test_retry_count; i++ ) {

		error = mxt_write_object( data, MXT_SPT_SELFTEST_T25,
			MXT_SELFTEST_CMD, MXT_SELFTEST_CMD_RUNTEST );
		if( error ) {
			MXT_TS_ERR_LOG(
				"mxt_write_object error. CMD_RUNTEST. %d\n",
				error);
			ret = error;
			goto ERR;
		}

		error = mxt_write_object( data, MXT_SPT_SELFTEST_T25,
			MXT_SELFTEST_CTRL, MXT_SELFTEST_CTRL_TESTEXEC );
		if( error ) {
			MXT_TS_ERR_LOG(
				"mxt_write_object error. CTRL_TESTEXEC. %d\n",
				error);
			ret = error;
			goto ERR;
		}

		wait_event_interruptible_timeout( data->wait,
			( mxt_read_stop || data->test_flag ),
			msecs_to_jiffies( data->test_wait_time ) );

		if( mxt_read_stop ) {
			ret = -ESHUTDOWN;
			goto END;
		}
		
		if( data->test_flag ) {
			break;
		}
	}
	if( i == MXT_SELFTEST_RETRY_COUNT ) {
		ret = -ENOEXEC;
	}
ERR:
	error = mxt_write_object( data, MXT_SPT_SELFTEST_T25,
		MXT_SELFTEST_CMD, MXT_SELFTEST_CMD_NONE );
	if( error ) {
		MXT_TS_ERR_LOG( "mxt_write_object error. CMD_NONE. %d\n",
			error);
		ret = error;
	}

	error = mxt_write_object( data, MXT_SPT_SELFTEST_T25,
		MXT_SELFTEST_CTRL, MXT_SELFTEST_CTRL_DISABLE );
	if( error ) {
		MXT_TS_ERR_LOG( "mxt_write_object error. CMD_DISABLE. %d\n",
			error);
		ret = error;
	}

	error = mxt_write_object( data, MXT_SPT_SELFTEST_T25,
		MXT_SELFTEST_CTRL, MXT_SELFTEST_CTRL_ENABLE );
	if( error ) {
		MXT_TS_ERR_LOG( "mxt_write_object error. CMD_ENABLE. %d\n",
			error);
		ret = error;
	}

END:
	if( mxt_read_stop ) {
		ret = -ESHUTDOWN;
	}

	MXT_TS_DEBUG_LOG( "end. ret=%d\n", ret );

	if( ret == 0 && data->test_flag ) {
		memcpy( buf, data->test_data,
			sizeof( u8 ) * MXT_SELFTEST_RESULT_SIZE );
		return sizeof( u8 ) * MXT_SELFTEST_RESULT_SIZE;
	}
	else {
		return ret;
	}
}

static DEVICE_ATTR(test, 0664, mxt_self_test_show, NULL );
/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. end */

static DEVICE_ATTR(update_fw, 0664, NULL, mxt_update_fw_store);
static DEVICE_ATTR(debug_enable, 0664, mxt_debug_enable_show,
		   mxt_debug_enable_store);
static DEVICE_ATTR(pause_driver, 0664, mxt_pause_show, mxt_pause_store);

static struct attribute *mxt_attrs[] = {
	&dev_attr_update_fw.attr,
	&dev_attr_debug_enable.attr,
	&dev_attr_pause_driver.attr,
	NULL
};

static const struct attribute_group mxt_attr_group = {
	.attrs = mxt_attrs,
};

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Start to touch operation (device RUN mode).
 * @param[in,out] *data The structure of Touch panel driver information.
 * @return        None.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static void mxt_start(struct mxt_data *data)
{
	int error;
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct device *dev = &data->client->dev;
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	if (data->is_stopped == 0)
		return;

	/* FUJITSU TEN:2013-03-05 TSNS_RESET add. start */
	error = mxt_tsns_reset(data);
	if( error ) {
		MXT_TS_ERR_LOG( "mxt_tsns_reset error.\n" );
		return;
	}
	/* FUJITSU TEN:2013-03-05 TSNS_RESET add. end */

	error = mxt_set_power_cfg(data, MXT_POWER_CFG_RUN);

	/* FUJITSU TEN:2013-02-18 lites macro add. start */
	if (error)
		return;
	/* FUJITSU TEN:2013-02-18 lites macro add. start */
	/* FUJITSU TEN:2012-12-17 log edit for notice. start */
	MXT_TS_NOTICE_LOG( "touch started.\n");
	/* FUJITSU TEN:2012-12-17 log edit for notice. end */
	/* FUJITSU TEN:2013-02-18 lites macro add. start */
	MXT_DRC_TRC( "touch started.\n" );
	/* FUJITSU TEN:2013-02-18 lites macro add. end */

	/* FUJITSU TEN:2012-10-12 hardware error polling add. start */
	if( data->hwerror_count < data->hwerror_count_max ) {
		data->error_polling_flag = MXT_ERROR_POLLING_EXEC;
		mxt_timer_start( data );
	}
	/* FUJITSU TEN:2012-10-12 hardware error polling add. end */

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Stop to touch operation (device DEEPSLEEP mode).
 * @param[in,out] *data The structure of Touch panel driver information.
 * @return        None.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static void mxt_stop(struct mxt_data *data)
{
	int error;
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	struct device *dev = &data->client->dev;
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	if (data->is_stopped)
		return;

	error = mxt_set_power_cfg(data, MXT_POWER_CFG_DEEPSLEEP);

	/* FUJITSU TEN:2013-02-18 lites macro add. start */
	if (!error) {
		/* FUJITSU TEN:2012-12-17 log edit for notice. start */
		MXT_TS_NOTICE_LOG( "touch stopped.\n");
		/* FUJITSU TEN:2012-12-17 log edit for notice. end */
		MXT_DRC_TRC( "touch stoppend.\n" );
	}
	/* FUJITSU TEN:2013-02-18 lites macro add. end */

	/* FUJITSU TEN:2012-10-12 hardware error polling add. start */
	data->error_polling_flag = MXT_ERROR_POLLING_STOP;
	mxt_timer_stop( data );
	cancel_work_sync( &data->work );
	/* FUJITSU TEN:2012-10-12 hardware error polling add. end */

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Touch panel driver Input Subsystem open hander.
 * @param[in]     *dev The structure of Input device.
 * @return        Result.
 * @retval        0 Normal end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_input_open(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	mxt_start(data);

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return 0;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Touch panel driver Input Subsystem close hander.
 * @param[in]     *dev The structure of Input device.
 * @return        Result.
 * @retval        0 Normal end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static void mxt_input_close(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	mxt_stop(data);

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */
}

/* FUJITSU TEN:2012-11-26 BU-DET flag function add. start */
/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Fastboot flag down function.
 * @param[in]     *flag Fastboot flag of system global variable.
 * @param[in]     *param Not use.
 * @return        Result.
 * @retval        0 Normal end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_fastboot_flag_down(bool flag, void* param)
{
	if ( flag == false ) {
		mxt_read_stop = MXT_FLAG_CLEAR;
	}
	return 0;
}
/* FUJITSU TEN:2012-11-26 BU-DET flag function add. start */

/* FUJITSU TEN:2012-08-10 early device off. start */
/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Suspend handler function.
 * @param[in]     *dev Device information structure.
 * @return        Result.
 * @retval        0 Normal end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	MXT_TS_DEBUG_LOG( "start.\n" );

	disable_irq( data->irq );

	mxt_read_stop = MXT_READ_STOP;
	wake_up_interruptible( &(data->wait) );

	data->pdata->low_tsns_reset();

	data->error_polling_flag = MXT_ERROR_POLLING_STOP;

	MXT_TS_DEBUG_LOG( "end.\n" );

	return 0;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Resume handler function.
 * @param[in]     *dev Device information structure.
 * @return        Result.
 * @retval        0 Normal end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_resume(struct device *dev)
{
	MXT_TS_NOTICE_LOG( "start. \n" );

	MXT_TS_NOTICE_LOG( "end. \n" );

	return 0;
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Reinit handler function.
 * @param[in]     *dev Device information structure.
 * @return        Result.
 * @retval        0 Normal end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int mxt_reinit(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);
	struct input_dev *input_dev = data->input_dev;
	int ret = 0;

	MXT_TS_NOTICE_LOG( "start. \n" );

	enable_irq( data->irq );

	mxt_timer_stop( data );
	cancel_work_sync( &data->work );

	early_device_invoke_with_flag_irqsave(EARLY_DEVICE_BUDET_INTERRUPTION_MASKED,
		mxt_fastboot_flag_down, data );

	if( mxt_read_stop == MXT_READ_STOP ) {
		MXT_TS_NOTICE_LOG( "reinit called in BU-DET.\n" );
		return 0;
	}

	data->is_stopped = 1;

	data->intr_count = 0;
	memset( &data->intr_tv, 0, sizeof( struct timeval ) );
	data->touch_count = 0;
	memset( &data->touch_tv, 0, sizeof( struct timeval ) );
#ifdef CONFIG_TOUCH_STATISTICAL_INFO_LOG
	data->slog_count = 0;
	data->last_intr_count = 0;
	data->last_touch_count = 0;
#endif
	data->fs_count = 0;
	data->last_intr_count_fs = 0;
	data->last_touch_count_fs = 0;

	data->misc_count++;
	input_report_abs(input_dev, ABS_MISC, ( data->misc_count ) % 2 );
	input_sync(input_dev);

	MXT_TS_NOTICE_LOG( "end. \n" );

	return ret;
}
/* FUJITSU TEN:2012-08-10 early device off add. end */

/* FUJITSU TEN:2012-08-14 debug early device off add. start */
#ifdef CONFIG_TOUCH_DEBUG_FASTBOOT
/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Sysfs interface for debug fastboot execute.
 * @param[in]     *dev Device information structure.
 * @param[in]     *attr The structure of device attribute.
 * @param[in]     *buf Set polling data.
 * @param[in]     count Set data size.
 * @return        Write buffer size.
 * @retval        <=0 Normal end.
 * @retval        -EINVAL Parameter error.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static ssize_t mxt_debug_fastboot_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int i;
	int ret;

	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);

	MXT_TS_INFO_LOG( "start.\n" );

	if (sscanf(buf, "%u", &i) == 1 ) {
		switch( i ) {
		case 0:
			ret = mxt_suspend( dev );
			break;
		case 1:
			ret = mxt_resume( dev );
			break;
		default :
			ret = mxt_reinit( dev );
			break;
		}
	} else {
		MXT_TS_ERR_LOG( "debug_fastboot write error\n");
		return -EINVAL;
	}
	MXT_TS_DEBUG_LOG( "end. ret=%d\n", ret );
	return count;
}

static DEVICE_ATTR(debug_fastboot, 0664, NULL,
	mxt_debug_fastboot_store );
#endif

/* FUJITSU TEN:2012-08-14 debug early device off add. end */

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Touch panel driver probe function.
 * @param[in]     *client Touch panel driver use I2C information.
 * @param[in]     *id Not use.
 * @return        Result.
 * @retval        0 Normal end.
 * @retval        -EINVAL Parameter error.
 * @retval        -ENOMEM No memory.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int __devinit mxt_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	const struct mxt_platform_data *pdata = client->dev.platform_data;
	struct mxt_data *data;
	struct input_dev *input_dev;
	int error;

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	if (!pdata)
		return -EINVAL;

	/* FUJITSU TEN:2012-08-06 initialize variable add. start */
	/* Initialize variable */
	spin_lock_init( &mxt_config_lock );
	mxt_config_status = MXT_FLAG_CLEAR;
	mxt_read_stop = MXT_FLAG_CLEAR;
	/* FUJITSU TEN:2012-08-06 initialize variable add. end */

	data = kzalloc(sizeof(struct mxt_data), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!data || !input_dev) {
		dev_err(&client->dev, "Failed to allocate memory\n");
		error = -ENOMEM;
		goto err_free_mem;
	}

	data->state = INIT;

	data->client = client;
	data->input_dev = input_dev;
	data->pdata = pdata;
	data->irq = client->irq;
	/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. start */
	init_waitqueue_head( &(data->wait) );
	/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. end */

	/* FUJITSU TEN:2012-11-19 read parameter for eMMC add. start */
	mxt_read_parameter(data);
	/* FUJITSU TEN:2012-11-19 read parameter for eMMC add. end */

	/* Initialize i2c device */
	error = mxt_initialize(data);
	if (error)
		goto err_free_object;

	/* Initialize input device */
	/* FUJITSU TEN:2012-07-02 device name edit. start */
	input_dev->name = "atmel-maxtouch";
	/* FUJITSU TEN:2012-07-02 device name edit. end */
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;
	input_dev->open = mxt_input_open;
	input_dev->close = mxt_input_close;

	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);

/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
	/* For single touch */
	input_set_abs_params(input_dev, ABS_X,
			     0, data->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_Y,
			     0, data->max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE,
			     0, 255, 0, 0);
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */

	/* For multi touch */
	input_mt_init_slots(input_dev, MXT_MAX_FINGER);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
			     0, MXT_MAX_AREA, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
			     0, data->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
			     0, data->max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE,
			     0, 255, 0, 0);

	/* FUJITSU TEN:2012-08-30 fastboot data add. start */
	input_set_abs_params(input_dev, ABS_MISC,
			     0, 1, 0, 0);
	/* FUJITSU TEN:2012-08-30 fastboot data add. end */

	input_set_drvdata(input_dev, data);
	i2c_set_clientdata(client, data);

	error = request_threaded_irq(client->irq, NULL, mxt_interrupt,
			pdata->irqflags, client->dev.driver->name, data);
	if (error) {
		dev_err(&client->dev, "Error %d registering irq\n", error);
		goto err_free_object;
	}
	
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = mxt_early_suspend;
	data->early_suspend.resume = mxt_late_resume;
	register_early_suspend(&data->early_suspend);
#endif
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */

/* FUJITSU TEN:2012-08-09 comment out. start */
#if 0
	if (data->state == APPMODE) {
		error = mxt_make_highchg(data);
		if (error)
			goto err_free_irq;
	}
#endif
/* FUJITSU TEN:2012-08-09 comment out. end */
/* FUJITSU TEN:2012-08-09 TSNS_RESET add. start */
	error = mxt_tsns_reset(data);
	if (error) {
		MXT_TS_ERR_LOG( "mxt_tsns_reset error.\n" );
		goto err_free_irq;
	}
/* FUJITSU TEN:2012-08-09 TSNS_RESET add. end */

	error = input_register_device(input_dev);
	if (error) {
		dev_err(&client->dev, "Error %d registering input device\n",
			error);
		goto err_free_irq;
	}

/* FUJITSU TEN:2012-06-28 comment out. start */
#if 0
	error = sysfs_create_group(&client->dev.kobj, &mxt_attr_group);
	if (error) {
		dev_err(&client->dev, "Failure %d creating sysfs group\n",
			error);
		goto err_unregister_device;
	}

	sysfs_bin_attr_init(&data->mem_access_attr);
	data->mem_access_attr.attr.name = "mem_access";
	data->mem_access_attr.attr.mode = S_IRUGO | S_IWUSR;
	data->mem_access_attr.read = mxt_mem_access_read;
	data->mem_access_attr.write = mxt_mem_access_write;
	data->mem_access_attr.size = data->mem_size;

	if (sysfs_create_bin_file(&client->dev.kobj,
				  &data->mem_access_attr) < 0) {
		dev_err(&client->dev, "Failed to create %s\n",
			data->mem_access_attr.attr.name);
		goto err_remove_sysfs_group;
	}
#endif
/* FUJITSU TEN:2012-06-28 comment out. end */
/* FUJITSU TEN:2012-07-31 config update IF(sysfs) add. start */
	sysfs_bin_attr_init(&data->update_attr);
	data->update_attr.attr.name = "update";
/* FUJITSU TEN:2014-02-17 change permission from 666 to 664 start */
	data->update_attr.attr.mode = 0664;
/* FUJITSU TEN:2014-02-17 change permission from 666 to 664 end */
	data->update_attr.write = mxt_update_config_write;
	data->update_attr.size = MXT_CONFIG_DATA_SIZE;

	error = sysfs_create_bin_file(&input_dev->dev.kobj, &data->update_attr);
	if ( error ) {
		MXT_TS_ERR_LOG( "Failed to create %s\n",
			data->update_attr.attr.name);
		goto err_unregister_device;
	}
/* FUJITSU TEN:2012-07-31 config update IF(sysfs) add. end */

/* FUJITSU TEN:2013-04-05 show config version IF(sysfs) add. start */
	error = device_create_file(&input_dev->dev, &dev_attr_version );
 	if ( error ) {
		MXT_TS_ERR_LOG( "Failed to create %s\n",
			dev_attr_version.attr.name );
		goto err_remove_sysfs;
	}
/* FUJITSU TEN:2013-04-05 show config version IF(sysfs) add. end */

/* FUJITSU TEN:2013-06-07 debug function add. start */
	error = device_create_file(&input_dev->dev, &dev_attr_touch_info );
	if ( error ) {
		MXT_TS_ERR_LOG( "Failed to create %s\n",
			dev_attr_touch_info.attr.name );
		goto err_remove_sysfs2;
	}
/* FUJITSU TEN:2013-06-07 debug function add. end */

/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. start */
	error = device_create_file(&input_dev->dev, &dev_attr_test );
	if ( error ) {
		MXT_TS_ERR_LOG( "Failed to create %s\n",
			dev_attr_test.attr.name );
		goto err_remove_sysfs3;
	}
/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. end */

/* FUJITSU TEN:2012-09-19 fastboot edit. start */
	data->edev.reinit = mxt_reinit;
	data->edev.dev = &data->client->dev;
	error = early_device_register( &data->edev );
	if( error ) {
		MXT_TS_ERR_LOG( "Failed to early device register\n" );
		goto err_remove_sysfs4;
	}
/* FUJITSU TEN:2012-09-19 fastboot edit. end */

/* FUJITSU TEN:2012-08-14 debug early device off add. start */
#ifdef CONFIG_TOUCH_DEBUG_FASTBOOT
	error = device_create_file(&input_dev->dev,
		&dev_attr_debug_fastboot );
	if( error ) {
		MXT_TS_ERR_LOG( "Failed to create %s\n",
			dev_attr_debug_fastboot.attr.name);
		goto err_remove_sysfs_debug;
	}
#endif
/* FUJITSU TEN:2012-08-14 debug early device off add. end */

/* FUJITSU TEN:2012-10-10 hardware error polling add. start */
	hrtimer_init( &data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	data->timer.function = mxt_timer_func;
	data->wq = create_singlethread_workqueue( "atmel-maxtouch" );
	if( !( data->wq ) ) {
		MXT_TS_ERR_LOG( "Failed to create_singlethread_workqueue\n" );
		goto err_unregister_edev;
	}
	INIT_WORK(&data->work, mxt_work_func);
/* FUJITSU TEN:2012-10-10 hardware error polling add. end */
	/* FUJITSU TEN:2012-12-17 log edit for notice. start */
	MXT_TS_NOTICE_LOG( "mxT540E chip initialize finished.\n");
	/* FUJITSU TEN:2012-12-17 log edit for notice. end */
	/* FUJITSU TEN:2013-02-18 lites macro add. start */
	MXT_DRC_TRC( "mxT540E chip initialize finished.\n" );
	/* FUJITSU TEN:2013-02-18 lites macro add. end */

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return 0;

/* FUJITSU TEN:2012-06-28 comment out. start */
#if 0
err_remove_sysfs_group:
	sysfs_remove_group(&client->dev.kobj, &mxt_attr_group);
#endif
/* FUJITSU TEN:2012-06-28 comment out. end */

/* FUJITSU TEN:2012-10-10 hardware error polling add. start */
err_unregister_edev:
#ifdef CONFIG_TOUCH_DEBUG_FASTBOOT
	sysfs_remove_file( &client->dev.kobj,
		(const struct attribute *)&dev_attr_debug_fastboot );
/* FUJITSU TEN:2012-10-10 hardware error polling add. end */
/* FUJITSU TEN:2012-08-14 debug early device off add. start */
err_remove_sysfs_debug:
#endif
	early_device_unregister( &data->edev );
/* FUJITSU TEN:2012-08-14 debug early device off add. end */
/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. start */
err_remove_sysfs4:
	sysfs_remove_file( &client->dev.kobj,
		(const struct attribute *)&dev_attr_test );
/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. end */
/* FUJITSU TEN:2013-06-07 debug function add. start */
err_remove_sysfs3:
	sysfs_remove_file( &client->dev.kobj,
		(const struct attribute *)&dev_attr_touch_info );
/* FUJITSU TEN:2013-06-07 debug function add. end */
/* FUJITSU TEN:2012-09-19 fastboot edit. start */
/* FUJITSU TEN:2013-04-05 show config version IF(sysfs) add. start */
err_remove_sysfs2:
	sysfs_remove_file( &client->dev.kobj,
		(const struct attribute *)&dev_attr_version );
/* FUJITSU TEN:2013-04-05 show config version IF(sysfs) add. end */
err_remove_sysfs:
	sysfs_remove_bin_file(&client->dev.kobj, &data->update_attr);
/* FUJITSU TEN:2012-09-19 fastboot edit. end */
err_unregister_device:
	input_unregister_device(input_dev);
	input_dev = NULL;
err_free_irq:
	free_irq(client->irq, data);
err_free_object:
	kfree(data->object_table);
err_free_mem:
	input_free_device(input_dev);
	kfree(data);

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end. error:%d\n", error );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return error;
}


/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Touch panel driver remove function.
 * @param[in]     *client Touch panel driver use I2C information.
 * @return        Result.
 * @retval        0 Normal end.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int __devexit mxt_remove(struct i2c_client *client)
{
	struct mxt_data *data = i2c_get_clientdata(client);

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_INFO_LOG( "start.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

/* FUJITSU TEN:2012-06-28 comment out. start */
#if 0
	sysfs_remove_bin_file(&client->dev.kobj, &data->mem_access_attr);
	sysfs_remove_group(&client->dev.kobj, &mxt_attr_group);
#endif
/* FUJITSU TEN:2012-06-28 comment out. end */

/* FUJITSU TEN:2012-10-10 hardware error polling add. start */
	cancel_work_sync( &data->work );
	destroy_workqueue( data->wq );
/* FUJITSU TEN:2012-10-10 hardware error polling add. end */

/* FUJITSU TEN:2012-08-14 debug early device off add. start */
#ifdef CONFIG_TOUCH_DEBUG_FASTBOOT
	sysfs_remove_file( &client->dev.kobj,
		(const struct attribute *)&dev_attr_debug_fastboot );
#endif
/* FUJITSU TEN:2012-08-14 debug early device off add. start */

/* FUJITSU TEN:2012-09-19 fastboot edit. start */
	early_device_unregister( &data->edev );
/* FUJITSU TEN:2012-09-19 fastboot edit. end */

/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. start */
	sysfs_remove_file( &client->dev.kobj,
		(const struct attribute *)&dev_attr_test );
/* FUJITSU TEN:2013-06-17 exec self test T25 IF(sysfs) add. end */

/* FUJITSU TEN:2013-06-07 debug function add. start */
	sysfs_remove_file( &client->dev.kobj,
		(const struct attribute *)&dev_attr_touch_info );
/* FUJITSU TEN:2013-06-07 debug function add. end */

/* FUJITSU TEN:2013-04-05 show config version IF(sysfs) add. start */
	sysfs_remove_file( &client->dev.kobj,
		(const struct attribute *)&dev_attr_version );
/* FUJITSU TEN:2013-04-05 show config version IF(sysfs) add. end */

/* FUJITSU TEN:2012-07-31 config update IF(sysfs) add. start */
	sysfs_remove_bin_file(&client->dev.kobj, &data->update_attr);
/* FUJITSU TEN:2012-07-31 config update IF(sysfs) add. end */
	free_irq(data->irq, data);
	input_unregister_device(data->input_dev);
/* FUJITSU TEN:2012-08-14 comment out. start */
#if 0
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif
#endif
/* FUJITSU TEN:2012-08-14 comment out. end */
	kfree(data->object_table);
	kfree(data);

	/* FUJITSU TEN:2012-06-25 debug log add. start */
	MXT_TS_DEBUG_LOG( "end.\n" );
	/* FUJITSU TEN:2012-06-25 debug log add. end */

	return 0;
}

/* FUJITSU TEN:2012-08-10 comment out. start */
#if 0
#ifdef CONFIG_PM
static int mxt_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);
	struct input_dev *input_dev = data->input_dev;
	mutex_lock(&input_dev->mutex);

	if (input_dev->users)
		mxt_stop(data);

	mutex_unlock(&input_dev->mutex);

	return 0;
}

static int mxt_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);
	struct input_dev *input_dev = data->input_dev;

	/* Soft reset */
	mxt_soft_reset(data, MXT_RESET_VALUE);

	mutex_lock(&input_dev->mutex);

	if (input_dev->users)
		mxt_start(data);

	mutex_unlock(&input_dev->mutex);
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxt_early_suspend(struct early_suspend *es)
{
	struct mxt_data *mxt;
	mxt = container_of(es, struct mxt_data, early_suspend);

	if (mxt_suspend(&mxt->client->dev) != 0)
		dev_err(&mxt->client->dev, "%s: failed\n", __func__);
}

static void mxt_late_resume(struct early_suspend *es)
{
	struct mxt_data *mxt;
	mxt = container_of(es, struct mxt_data, early_suspend);

	if (mxt_resume(&mxt->client->dev) != 0)
		dev_err(&mxt->client->dev, "%s: failed\n", __func__);
}
#endif

#endif
#endif
/* FUJITSU TEN:2012-08-10 comment out. end */

static const struct dev_pm_ops mxt_pm_ops = {
	.suspend	= mxt_suspend,
	.resume		= mxt_resume,
};
/* FUJITSU TEN:2012-08-10 comment out. start */
//#endif
/* FUJITSU TEN:2012-08-10 comment out. end */

static const struct i2c_device_id mxt_id[] = {
	/* FUJITSU TEN:2012-06-25 device_id edit. start */
	{ "atmel_mxt_ts", 0 },
	/* FUJITSU TEN:2012-06-25 device_id edit. end */
	{ }
};
MODULE_DEVICE_TABLE(i2c, mxt_id);

static struct i2c_driver mxt_driver = {
	.driver = {
		.name	= "atmel_mxt_ts",
		.owner	= THIS_MODULE,
/* FUJITSU TEN:2012-08-10 comment out. start */
//#ifdef CONFIG_PM
/* FUJITSU TEN:2012-08-10 comment out. end */
		.pm	= &mxt_pm_ops,
/* FUJITSU TEN:2012-08-10 comment out. start */
//#endif
/* FUJITSU TEN:2012-08-10 comment out. end */
	},
	.probe		= mxt_probe,
	.remove		= __devexit_p(mxt_remove),
	.id_table	= mxt_id,
};

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Touch panel driver initialize function.
 * @param         None.
 * @return        i2c_add_driver result data.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static int __init mxt_init(void)
{
	return i2c_add_driver(&mxt_driver);
}

/* FUJITSU TEN:2013-01-15 doxygen comment add. start */
/**
 * @brief         Touch panel driver exit function.
 * @param         None.
 * @return        None.
 */
/* FUJITSU TEN:2013-01-15 doxygen comment add. end */
static void __exit mxt_exit(void)
{
	i2c_del_driver(&mxt_driver);
}

module_init(mxt_init);
module_exit(mxt_exit);

/* Module information */
MODULE_AUTHOR("Joonyoung Shim <jy0922.shim@samsung.com>");
MODULE_DESCRIPTION("Atmel maXTouch Touchscreen driver");
MODULE_LICENSE("GPL");
