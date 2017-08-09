/* 
 * AK7736 DSP Control driver local header
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

#ifndef AK7736_DSP_LOCAL_HEADER
#define AK7736_DSP_LOCAL_HEADER

#include <linux/nonvolatile_def.h>
#include <linux/lites_trace_drv.h>

/* timing */
#define AK7736_DSP_SYSTEM_RESET_WAIT                1 // 1msec
#define AK7736_DSP_SYSTEM_INIT_WAIT_COLDBOOT     8000 // 8000msec
#define AK7736_DSP_SYSTEM_INIT_WAIT_FASTBOOT        0 // 0msec
#define AK7736_DSP_DIAG_MIC_LEVEL_SAMPLING_TIME    50 // 50msec
#define AK7736_DSP_DIAG_MIC_LEVEL_TIMEOUT        1000 // 1000msec
#define AK7736_DSP_MINIMUM_WAIT_RESOLUTION         10 // 10msec

/* for failsafe */
#define AK7736_DSP_I2C_RETRY_COUNT        3
#define AK7736_DSP_INIT_RETRY_COUNT       3
#define AK7736_DSP_INIT_RETRY_COUNT_MIN   1
#define AK7736_DSP_INIT_RETRY_COUNT_MAX   10

/* AK7736 registers */
#define AK7736_DSP_REG_WRITE_MASK    0x80

#define AK7736_DSP_REG_CONT00        0x40
#define AK7736_DSP_REG_CONT01        0x41
#define AK7736_DSP_REG_CONT02        0x42
#define AK7736_DSP_REG_CONT03        0x43
#define AK7736_DSP_REG_CONT04        0x44
#define AK7736_DSP_REG_CONT05        0x45
#define AK7736_DSP_REG_CONT06        0x46
#define AK7736_DSP_REG_CONT07        0x47
#define AK7736_DSP_REG_CONT08        0x48
#define AK7736_DSP_REG_CONT09        0x49
#define AK7736_DSP_REG_CONT0A        0x4a
#define AK7736_DSP_REG_CONT0B        0x4b
#define AK7736_DSP_REG_CONT0C        0x4c
#define AK7736_DSP_REG_CONT0D        0x4d
#define AK7736_DSP_REG_CONT0E        0x4e
#define AK7736_DSP_REG_CONT0F        0x4f
#define AK7736_DSP_REG_IDENTIFY      0x60
#define AK7736_DSP_REG_DSPERROR      0x70
#define AK7736_DSP_REG_CRC           0x72

#define AK7736_DSP_REG_MIR1          0x76
#define AK7736_DSP_REG_MIR2          0x78
#define AK7736_DSP_REG_MIR3          0x7A

#define AK7736_DSP_REG_PRAM          0x38
#define AK7736_DSP_REG_CRAM          0x34

#define AK7736_DSP_REG_CRAM_UPDATE   0xA4


/* Control register definition for bitmap operation */
#define AK7736_DSP_BIT_CKRESETN      0x01
#define AK7736_DSP_BIT_SRESETN       0x80
#define AK7736_DSP_BIT_DLRDY         0x01
#define AK7736_DSP_BIT_LR1DOWN       0x40
#define AK7736_DSP_BIT_CLKO          0xe0
#define AK7736_DSP_BIT_CRCE          0x20

#define AK7736_DSP_MIR_VALIDITY_MASK 0x0000000f

#define AK7736_DSP_RESET_HIGH        1
#define AK7736_DSP_RESET_LOW         0

#define AK7736_DSP_DLRDY_ON          1
#define AK7736_DSP_DLRDY_OFF         0

#define AK7736_DSP_LR1DOWN_FS8       1
#define AK7736_DSP_LR1DOWN_FS16      0

#define AK7736_DSP_CLKO_ON           1
#define AK7736_DSP_CLKO_OFF          0


/* Control register default value */
#define AK7736_DSP_VALUE_CONT00      0x02
#define AK7736_DSP_VALUE_CONT01      0xc8
#define AK7736_DSP_VALUE_CONT02      0x11
#define AK7736_DSP_VALUE_CONT03      0x14
#define AK7736_DSP_VALUE_CONT04      0x23
#define AK7736_DSP_VALUE_CONT05      0x20
#define AK7736_DSP_VALUE_CONT06      0x00
#define AK7736_DSP_VALUE_CONT07      0x00
#define AK7736_DSP_VALUE_CONT08      0x00
#define AK7736_DSP_VALUE_CONT09      0x00
#ifdef AK7736_DSP_CIRCUIT_TYPE_ENABLE_OUT3E
 #ifdef AK7736_DSP_CLKO_DEFAULT_ON
  #define AK7736_DSP_VALUE_CONT0A      0xeb
 #else
  #define AK7736_DSP_VALUE_CONT0A      0x0b
 #endif
#else
 #ifdef AK7736_DSP_CLKO_DEFAULT_ON
  #define AK7736_DSP_VALUE_CONT0A      0xe3
 #else
  #define AK7736_DSP_VALUE_CONT0A      0x03
 #endif
#endif
#define AK7736_DSP_VALUE_CONT0B      0x00
#define AK7736_DSP_VALUE_CONT0C      0x00
#define AK7736_DSP_VALUE_CONT0D      0x00
#define AK7736_DSP_VALUE_CONT0E      0x30
#define AK7736_DSP_VALUE_CONT0F      0x00
#define AK7736_DSP_VALUE_IDENTIFY    0x36

static const char ak7736_dsp_control_register[] = {
    AK7736_DSP_VALUE_CONT00,
    AK7736_DSP_VALUE_CONT01,
    AK7736_DSP_VALUE_CONT02,
    AK7736_DSP_VALUE_CONT03,
    AK7736_DSP_VALUE_CONT04,
    AK7736_DSP_VALUE_CONT05,
    AK7736_DSP_VALUE_CONT06,
    AK7736_DSP_VALUE_CONT07,
    AK7736_DSP_VALUE_CONT08,
    AK7736_DSP_VALUE_CONT09,
    AK7736_DSP_VALUE_CONT0A,
    AK7736_DSP_VALUE_CONT0B,
    AK7736_DSP_VALUE_CONT0C,
    AK7736_DSP_VALUE_CONT0D,
    AK7736_DSP_VALUE_CONT0E,
    AK7736_DSP_VALUE_CONT0F,
};


/* DSP firmware */
#define AK7736_DSP_DL_BUFFER_SIZE    1024*4
#define AK7736_DSP_RAM_START_ADDRESS 0x0000
#define AK7736_DSP_PRAM_ALIGNMENT    0x05
#define AK7736_DSP_CRAM_ALIGNMENT    0x03
#define AK7736_DSP_DL_DATA_HEADER_SIZE 3


/* CRAM PARAMETER */
#define AK7736_DSP_CRAM_BAND_SEL                0x0267
#define AK7736_DSP_CRAM_HF_ON_OFF               0x026a
#define AK7736_DSP_CRAM_FES_OUT_ATT_STEP        0x0276
#define AK7736_DSP_CRAM_FES_OUT_ATT             0x0277
#define AK7736_DSP_CRAM_FES_IN_ATT_STEP         0x0279
#define AK7736_DSP_CRAM_FES_IN_ATT              0x027a
#define AK7736_DSP_CRAM_SPK_VOL_INDEX           0x0290
#define AK7736_DSP_CRAM_FORCE_INIT              0x0291
#define AK7736_DSP_CRAM_HF_OFF_ATT              0x0283
#define AK7736_DSP_CRAM_MIC_L_COMPENSATION_ATT  0x034c
#define AK7736_DSP_CRAM_MIC_R_COMPENSATION_ATT  0x034d
#define AK7736_DSP_CRAM_PEAKHOLD                0x035c
#define AK7736_DSP_CRAM_DOUT3L_SEL              0x051e
#define AK7736_DSP_CRAM_PEAKHOLD_MICR2          0x0522
#define AK7736_DSP_CRAM_PEAKHOLD_MICR3          0x0523

#define AK7736_DSP_CRAM_ATTWAIT                 0xffff
#define AK7736_DSP_CRAM_TEST_MODE               0x0520

const char ak7736_dsp_cram_on [3] = {0x00, 0x01, 0x00};
const char ak7736_dsp_cram_off[3] = {0x00, 0x00, 0x00};
const char ak7736_dsp_cram_in_att_default[3] = {0x00, 0x10, 0x00};
const char ak7736_dsp_cram_out3e[3] = {0x00, 0x00, 0x01};


/* nonvolatile map */
#define AK7736_DSP_NVROM_HEADER_SIZE    64
#define AK7736_DSP_NVROM_MAGIC_OFFSET    0
#define AK7736_DSP_NVROM_MAGIC_SIZE      4
#define AK7736_DSP_NVROM_SIZE_OFFSET     4
#define AK7736_DSP_NVROM_CRC_OFFSET      8
#define AK7736_DSP_NVROM_VERSION_OFFSET 16

#define AK7736_DSP_NVROM_PRAM_OFFSET    FTEN_NONVOL_AK7736_DSP_PRAM
#define AK7736_DSP_NVROM_CRAM_OFFSET    FTEN_NONVOL_AK7736_DSP_CRAM
#define AK7736_DSP_NVROM_PARAM_OFFSET   FTEN_NONVOL_AK7736_DSP_PARAMETER

#define AK7736_DSP_NVROM_PRAM_SIZE      FTEN_NONVOL_AK7736_DSP_PRAM_SIZE
#define AK7736_DSP_NVROM_CRAM_SIZE      FTEN_NONVOL_AK7736_DSP_CRAM_SIZE
#define AK7736_DSP_NVROM_PARAM_SIZE     FTEN_NONVOL_AK7736_DSP_PARAMETER_SIZE

#define AK7736_DSP_PROGRAM_TYPE_PRAM    0
#define AK7736_DSP_PROGRAM_TYPE_CRAM    1
#define AK7736_DSP_PROGRAM_TYPE_PARAM   2

const char ak7736_dsp_pram_magic[]  = "PRAM";
const char ak7736_dsp_cram_magic[]  = "CRAM";
const char ak7736_dsp_param_magic[] = "PARA";

#define AK7736_NV_OFFSET_FES_OUT_ATT_STEP        0x0000
#define AK7736_NV_OFFSET_FES_OUT_ATT_RESERVE     0x0001
#define AK7736_NV_OFFSET_FES_IN_ATT_STEP         0x0002
#define AK7736_NV_OFFSET_FES_IN_ATT              0x0003
#define AK7736_NV_OFFSET_HF_OFF_ATT              0x0004
#define AK7736_NV_OFFSET_MIC_L_COMPENSATION_ATT  0x0005
#define AK7736_NV_OFFSET_MIC_R_COMPENSATION_ATT  0x0006
#define AK7736_NV_OFFSET_ATTWAIT                 0x0007
#define AK7736_NV_OFFSET_FES_OUT_ATT             0x0010
#define AK7736_NV_OFFSET_MICLEVEL                0x0020
#define AK7736_NV_OFFSET_SPK_VOL_INDEX           0x0040
#define AK7736_NV_OFFSET_SEND_VOL_INDEX          0x00C0

#define AK7736_NV_DATA_FES_OUT_ATT_MAX           (3)
#define AK7736_NV_DATA_FES_OUT_ATT_MIN           (1)
#define AK7736_NV_DATA_MICLEVEL_MAX              (11)
#define AK7736_NV_DATA_SPK_VOL_INDEX_MAX         (0)
#define AK7736_NV_DATA_SPK_VOL_INDEX_MIN         (-99)
#define AK7736_NV_DATA_SEND_VOL_INDEX_MAX        (48)
#define AK7736_NV_DATA_SEND_VOL_INDEX_MIN        (-18)


#define AK7736_DSP_NVROM_DATA_OFFSET_DELAYCOLD   FTEN_NONVOL_AK7736_DELAY_COLD_BOOT
#define AK7736_DSP_NVROM_DATA_OFFSET_DELAYFAST   FTEN_NONVOL_AK7736_DELAY_FAST_BOOT
#define AK7736_DSP_NVROM_DATA_OFFSET_FAILRETRY   FTEN_NONVOL_AK7736_RETRY_FAILSAFE
#define AK7736_DSP_NVROM_DATA_OFFSET_SENDLEVEL   FTEN_NONVOL_AK7736_SENDVOL
#define AK7736_DSP_NVROM_DATA_OFFSET_EXTMIC      FTEN_NONVOL_MIC

/* ext mic setting */
#define AK7736_EXTMIC_DETECT_EXPIRE_TIME       200 // 200msec
#define AK7736_EXTMIC_DETECT_DELAY              70 // 70msec
#define AK7736_EXTMIC_DETECT_INTERVAL           30 // 30msec
#define AK7736_EXTMIC_DETECT_TIMES               3 // 3times

/* driver local resource */
struct akdsp_private_data {
    int device_off;

    struct akdsp_platform_data *pdata;
    struct i2c_client *client;

    int open_count;
    int error_count;

    int status;
    int error_stat;
    int stat_unread;
    int clko;
    wait_queue_head_t statq;

    struct workqueue_struct *initwq;
    struct delayed_work work;
    struct cdev cdev;
    dev_t  devno;
    struct class *devclass;

    struct semaphore user_lock;
    struct semaphore stat_lock;

    int mic_detect_done_flag;
    struct ak7736_micstatus mic_stat;

    struct early_device edev;

/* nv data */
    char  *cram_parameter_table;
    int send_level_vol[AK7736_NV_DATA_FES_OUT_ATT_MAX+1];
    int vr_vol;
    char vr_vol_address[3];
    int have_external_mic;
    int send_level;
    int wq_interval_coldboot;
    int wq_interval_fastboot;
    int retry_failsafe;
};

/* Driver trace map */
#define AK7736_ERRORID_FAILSAFE_FAILED          0x0000
#define AK7736_ERRORID_DSP                      0x0001
#define AK7736_ERRORID_NONVOLATILE              0x8000
#define AK7736_ERRORID_FAILSAFE                 0x8001
#define AK7736_ERRORID_FAILSAFE_START           0x8002

/* Driver trace param (AK7736_ERRORID_DSP) */
#define AK7736_ERROR_PATTERN_CRAM               0x0000
#define AK7736_ERROR_PATTERN_PRAM               0x0001
/* Driver trace param (AK7736_ERRORID_NONVOLATILE) */
#define AK7736_ERROR_DETAIL_MAGIC               0x0000
#define AK7736_ERROR_DETAIL_SIZE                0x0001
#define AK7736_ERROR_DETAIL_CRC                 0x0002
/* Driver trace param (AK7736_ERRORID_FAILSAFE_FAILED) */
#define AK7736_ERROR_PATTERN_I2C                0x0000
#define AK7736_ERROR_PATTERN_DSP                0x0001
#define AK7736_ERROR_PATTERN_NV                 0x0002

#ifdef CONFIG_AK7736_DSP_LOG_DEBUG
#define CONFIG_AK7736_DEBUG_FUNCTION_INOUT
#endif


#define __file__ (strrchr(__FILE__,'/' ) + 1 )

#define AK7736_EMERG_LOG( format, ...) { \
        printk( KERN_EMERG "[%s:%4d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define AK7736_ERR_LOG( format, ...) { \
        printk( KERN_ERR "[%s:%4d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#define AK7736_WARNING_LOG( format, ...) { \
        printk( KERN_WARNING "[%s:%4d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}

#if defined(CONFIG_AK7736_DSP_LOG_NOTICE) || defined(CONFIG_AK7736_DSP_LOG_INFO) \
        || defined(CONFIG_AK7736_DSP_LOG_DEBUG)
#define AK7736_NOTICE_LOG( format, ...) { \
        printk( KERN_NOTICE "[%s:%4d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define AK7736_NOTICE_LOG( format, ... )
#endif

#if defined(CONFIG_AK7736_DSP_LOG_INFO) || defined(CONFIG_AK7736_DSP_LOG_DEBUG)
#define AK7736_INFO_LOG( format, ...) { \
        printk( KERN_INFO "[%s:%4d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define AK7736_INFO_LOG( format, ... )
#endif

#ifdef CONFIG_AK7736_DSP_LOG_DEBUG
#define AK7736_DEBUG_LOG( format, ...) { \
        printk( KERN_DEBUG "[%s:%4d] %s " format, __file__, __LINE__, __func__, ##__VA_ARGS__ ); \
}
#else
#define AK7736_DEBUG_LOG( format, ... )
#endif

#ifdef CONFIG_AK7736_DEBUG_FUNCTION_INOUT
#define AK7736_DEBUG_FUNCTION_ENTER      {AK7736_DEBUG_LOG("in\n");}
#define AK7736_DEBUG_FUNCTION_ENTER_ONLY {AK7736_DEBUG_LOG("in(out)\n");}
#define AK7736_DEBUG_FUNCTION_EXIT       {AK7736_DEBUG_LOG("out\n");}
#else
#define AK7736_DEBUG_FUNCTION_ENTER
#define AK7736_DEBUG_FUNCTION_ENTER_ONLY
#define AK7736_DEBUG_FUNCTION_EXIT
#endif

#endif

