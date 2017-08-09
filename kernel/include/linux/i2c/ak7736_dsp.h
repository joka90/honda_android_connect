/* 
 * AK7736 DSP Control driver header
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

#ifndef AK7736_DSP_HEADER
#define AK7736_DSP_HEADER


/* for debug */
#define AK7736_DSP_DEBUG_STAT
#define AK7736_DSP_DEBUG_SYSFS
//#define AK7736_DSP_DEBUG_REGISTER_DUMP
//#define AK7736_DSP_FORCE_CRAM_VERIFY
//#define CONFIG_AK7736_PSEUDO_NONVOLATILE_USE

/* default value define */
//#define AK7736_DSP_CLKO_DEFAULT_ON
#define AK7736_DSP_DEFAULT_EXTERNAL_MIC


/* I2C Slave address */
#define AK7736_DSP_I2C_ADDR 0x18

/* platform data */
struct akdsp_platform_data {
    unsigned long         irqflags;
    void(*set_ak_dsp_pdn) (int val);
    void(*set_ak_adc_pdn) (int val);
    int(*get_ak_dsp_err)  (void);

    int(*get_ext_mic_stat)    (void);
    void(*set_ext_mic_power)  (int val);
    void(*set_ext_mic_detoff) (int val);
    void(*set_ext_mic_mode)   (int val);
    int(*get_ext_mic_mode)    (void);
};

#define AK7736_DSP_DEVICE_NAME "ak7736_dsp"



struct ak7736_dspvol {
    int send_vol;
    int recv_vol;
};

struct ak7736_status {
    int status;
    int error_stat;
};

struct ak7736_micstatus {
    int mic_type;
    int open_detect;
    int short_detect;
};

struct ak7736_miclevel {
    int mic_type;
    int lch_level;
    int rch_level; 
};

struct ak7736_level_vol {
    int level;
    int vol;
};

#define IOCTL_AK7736DSP_MAGIC 'd'

#define IOCTL_AK7736DSP_GET_STATUS     _IOR(IOCTL_AK7736DSP_MAGIC , 0x10, struct ak7736_status)
#define IOCTL_AK7736DSP_SET_CONTROL    _IOW(IOCTL_AK7736DSP_MAGIC , 0x11, int)
#define IOCTL_AK7736DSP_RESTART        _IO( IOCTL_AK7736DSP_MAGIC , 0x12)
#define IOCTL_AK7736DSP_SET_FSMODE     _IOW(IOCTL_AK7736DSP_MAGIC , 0x13, int)
#define IOCTL_AK7736DSP_SET_CLKOUT     _IOW(IOCTL_AK7736DSP_MAGIC , 0x14, int) // T.B.D
#define IOCTL_AK7736DSP_START_HF       _IOW(IOCTL_AK7736DSP_MAGIC , 0x15, struct ak7736_dspvol)
#define IOCTL_AK7736DSP_SET_HFVOL      _IOW(IOCTL_AK7736DSP_MAGIC , 0x16, struct ak7736_dspvol)
#define IOCTL_AK7736DSP_STOP_HF        _IO( IOCTL_AK7736DSP_MAGIC , 0x17)
#define IOCTL_AK7736DSP_START_VR       _IO( IOCTL_AK7736DSP_MAGIC , 0x18)
#define IOCTL_AK7736DSP_STOP_VR        _IO( IOCTL_AK7736DSP_MAGIC , 0x19)
#define IOCTL_AK7736DSP_SET_ECNCMODE   _IO( IOCTL_AK7736DSP_MAGIC , 0x1a)
#define IOCTL_AK7736DSP_CHECK_DEVICE   _IO( IOCTL_AK7736DSP_MAGIC , 0x1b)
#define IOCTL_AK7736DSP_START_SIRI     _IOW(IOCTL_AK7736DSP_MAGIC , 0x1c, struct ak7736_dspvol)
#define IOCTL_AK7736DSP_STOP_SIRI      _IO( IOCTL_AK7736DSP_MAGIC , 0x1d)
#define IOCTL_AK7736DSP_SET_DIAGMODE   _IOW(IOCTL_AK7736DSP_MAGIC , 0x30, int)
#define IOCTL_AK7736DSP_GET_MICLEVEL   _IOR(IOCTL_AK7736DSP_MAGIC , 0x31, int)
#define IOCTL_AK7736DSP_SET_SENDLEVEL  _IOW(IOCTL_AK7736DSP_MAGIC , 0x32, int)
#define IOCTL_AK7736DSP_GET_SENDLEVEL  _IOR(IOCTL_AK7736DSP_MAGIC , 0x33, int)
#define IOCTL_AK7736DSP_GET_MICSTATUS  _IOR(IOCTL_AK7736DSP_MAGIC , 0x34, struct ak7736_micstatus)
#define IOCTL_AK7736DSP_GET_MICLEVEL_STEREO   _IOR(IOCTL_AK7736DSP_MAGIC, 0x35, struct ak7736_miclevel)
#define IOCTL_AK7736DSP_SET_SENDLEVEL_DEV     _IOW(IOCTL_AK7736DSP_MAGIC, 0x36, struct ak7736_level_vol)
#define IOCTL_AK7736DSP_GET_SENDLEVEL_DEV     _IOR(IOCTL_AK7736DSP_MAGIC, 0x37, struct ak7736_level_vol)
#define IOCTL_AK7736DSP_SET_VRLEVEL_DEV       _IOW(IOCTL_AK7736DSP_MAGIC, 0x38, int)
#define IOCTL_AK7736DSP_GET_VRLEVEL_DEV       _IOR(IOCTL_AK7736DSP_MAGIC, 0x39, int)
#define IOCTL_AK7736DSP_GET_MICMODE           _IOR(IOCTL_AK7736DSP_MAGIC, 0x3a, int)
#define IOCTL_AK7736DSP_SET_MICMODE           _IOW(IOCTL_AK7736DSP_MAGIC, 0x3b, int)


enum {
    AK7736_DSP_STAT_INIT = 0,
    AK7736_DSP_STAT_STANDBY,
    AK7736_DSP_STAT_READY,
    AK7736_DSP_STAT_FAIL,
    AK7736_DSP_STAT_ERROR,
    AK7736_DSP_STAT_DIAG,

    AK7736_DSP_STAT_INIT_FAIL,
};

enum{
    AK7736_DSP_ERRSTAT_NONE = 0,
    AK7736_DSP_ERRSTAT_NOMEM,
    AK7736_DSP_ERRSTAT_DSP,
    AK7736_DSP_ERRSTAT_NONVOLATILE,
};

#define AK7736_DSP_FSMODE_HF_NB        0
#define AK7736_DSP_FSMODE_HF_WB        1
#define AK7736_DSP_FSMODE_VR_NB        2
#define AK7736_DSP_FSMODE_VR_WB        3
#define AK7736_DSP_FSMODE_8KHZ         AK7736_DSP_FSMODE_HF_NB
#define AK7736_DSP_FSMODE_16KHZ        AK7736_DSP_FSMODE_HF_WB

#define AK7736_DSP_CLKO_ON             1
#define AK7736_DSP_CLKO_OFF            0

#define AK7736_DSP_CONTROL_ON          1
#define AK7736_DSP_CONTROL_OFF         0

#define AK7736_DSP_DIAG_ON             1
#define AK7736_DSP_DIAG_OFF            0

#define AK7736_DSP_VOL_SENDMUTE_ON     1
#define AK7736_DSP_VOL_SENDMUTE_OFF    0

#define AK7736_DSP_ECNC_ON             1
#define AK7736_DSP_ECNC_OFF            0

#define AK7736_DSP_EXTMIC_VOICETAG     0
#define AK7736_DSP_EXTMIC_SIRI         1
#define AK7736_DSP_EXTMIC_UNKNOWN      2
#define AK7736_DSP_EXTMIC_HFT          3

#define AK7736_DSP_MICSTAT_OK          1
#define AK7736_DSP_MICSTAT_NG          0

#define AK7736_DSP_MIC_TYPE_INTERNAL   0
#define AK7736_DSP_MIC_TYPE_EXTERNAL   1


#define AK7736_DSP_VOL_NOTSET      (-255)


#define AK7736_DSP_SENDLEVEL_NONE      0
#define AK7736_DSP_SENDLEVEL_LOW       1
#define AK7736_DSP_SENDLEVEL_MID       2
#define AK7736_DSP_SENDLEVEL_HIGH      3









#ifdef __KERNEL__
 #ifndef CONFIG_FJFEAT_PRODUCT_ADA1M
  #ifdef CONFIG_FJFEAT_PRODUCT_ADA2S
   #define AK7736_DSP_CIRCUIT_TYPE_ENABLE_OUT3E
  #endif
 #endif
#endif

#endif
