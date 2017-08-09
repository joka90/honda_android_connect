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

#ifndef _NONVOLATILE_DEF_H
#define _NONVOLATILE_DEF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// 固定領域
#define FTEN_NONVOL_VERSION                         (0x0)
#define FTEN_NONVOL_THERM_FAN_T1                    (0x32)
#define FTEN_NONVOL_THERM_FAN_T2                    (0x33)
#define FTEN_NONVOL_THERM_ALERT_T1                  (0x34)
#define FTEN_NONVOL_THERM_ALERT_T2                  (0x35)
#define FTEN_NONVOL_THERM_SHUTDOWN_T1               (0x36)
#define FTEN_NONVOL_THERM_SHUTDOWN_T2               (0x37)
#define FTEN_NONVOL_TI_SLEEP_WIFI_ON                (0x14A)
#define FTEN_NONVOL_TI_SLEEP_WIFI_OFF               (0x14B)
#define FTEN_NONVOL_TI_SLEEP_WLEN_INSMOD            (0x14C)
#define FTEN_NONVOL_TI_SLEEP_SDMMC_INSMOD           (0x14D)
#define FTEN_NONVOL_TI_SLEEP_WLEN_RMMOD             (0x14E)
#define FTEN_NONVOL_TI_SLEEP_SDMMC_RMMOD            (0x14F)
#define FTEN_NONVOL_AK7736_DELAY_COLD_BOOT          (0x150)
#define FTEN_NONVOL_AK7736_DELAY_FAST_BOOT          (0x152)
#define FTEN_NONVOL_AK7736_RETRY_FAILSAFE           (0x154)
#define FTEN_NONVOL_AK7736_SENDVOL                  (0x155)
#define FTEN_NONVOL_GYRO_POLL_DEFAULT               (0x158)
#define FTEN_NONVOL_GYRO_POLL_HEALTH_CHECK          (0x15C)
#define FTEN_NONVOL_GYRO_WAIT_UNLOCKWP              (0x160)
#define FTEN_NONVOL_GYRO_WAIT_STAT_UPDATE           (0x162)
#define FTEN_NONVOL_GYRO_WAIT_SYSTEM_RESET          (0x164)
#define FTEN_NONVOL_TOUCH_WAIT_SYSTEM_RESET         (0x168)
#define FTEN_NONVOL_TOUCH_POLL_DEFAULT              (0x16A)
#define FTEN_NONVOL_TOUCH_POLL_THRESHOLD            (0x16C)
#define FTEN_NONVOL_USBOCP_POLL_DEFAULT             (0x170)
#define FTEN_NONVOL_USBOCP_POLL_THRESHOLD           (0x172)
#define FTEN_NONVOL_USBOCP_POLL_DELAY               (0x173)
#define FTEN_NONVOL_TI_SLEEP_WL_EN_ON_BEFORE        (0x174)
#define FTEN_NONVOL_TI_SLEEP_BT_ON_BTEN_OFF         (0x175)
#define FTEN_NONVOL_TI_SLEEP_BT_ON_BTEN_ON          (0x176)
#define FTEN_NONVOL_TI_SLEEP_BT_OFF_BTEN_OFF        (0x177)
#define FTEN_NONVOL_TI_SLEEP_BT_OFF_BTEN_ON         (0x178)
#define FTEN_NONVOL_TI_SLEEP_WL_EN_OFF_AFTER        (0x179)
#define FTEN_NONVOL_TI_WAIT_BAUDRATE_STABLE         (0x17A)
#define FTEN_NONVOL_FDR                             (0x180)
#define FTEN_NONVOL_TMP401_CONVERSION_RATE          (0x181)
#define FTEN_NONVOL_VOLATILEUPDATE                  (0x182)
#define FTEN_NONVOL_TOUCH_MMAP_HEADER               (0x184)
#define FTEN_NONVOL_TOUCH_MMAP_DATA                 (0x188)
#define FTEN_NONVOL_TOUCH_EXEC_SYSTEM_RESET         (0x2DA)
#define FTEN_NONVOL_TOUCH_SELFTEST_WAIT_TIME        (0x2DC)
#define FTEN_NONVOL_TOUCH_SELFTEST_EXEC_COUNT       (0x2DE)
#define FTEN_NONVOL_COLOR_VARIATION_INIT            (0x2E0)

#define FTEN_NONVOL_VERSION_SIZE                    (4)
#define FTEN_NONVOL_THERM_FAN_T1_SIZE               (1)
#define FTEN_NONVOL_THERM_FAN_T2_SIZE               (1)
#define FTEN_NONVOL_THERM_ALERT_T1_SIZE             (1)
#define FTEN_NONVOL_THERM_ALERT_T2_SIZE             (1)
#define FTEN_NONVOL_THERM_SHUTDOWN_T1_SIZE          (1)
#define FTEN_NONVOL_THERM_SHUTDOWN_T2_SIZE          (1)
#define FTEN_NONVOL_TI_SLEEP_WIFI_ON_SIZE           (1)
#define FTEN_NONVOL_TI_SLEEP_WIFI_OFF_SIZE          (1)
#define FTEN_NONVOL_TI_SLEEP_WLEN_INSMOD_SIZE       (1)
#define FTEN_NONVOL_TI_SLEEP_SDMMC_INSMOD_SIZE      (1)
#define FTEN_NONVOL_TI_SLEEP_WLEN_RMMOD_SIZE        (1)
#define FTEN_NONVOL_TI_SLEEP_SDMMC_RMMOD_SIZE       (1)
#define FTEN_NONVOL_AK7736_DELAY_COLD_BOOT_SIZE     (2)
#define FTEN_NONVOL_AK7736_DELAY_FAST_BOOT_SIZE     (2)
#define FTEN_NONVOL_AK7736_RETRY_FAILSAFE_SIZE      (1)
#define FTEN_NONVOL_AK7736_SENDVOL_SIZE             (1)
#define FTEN_NONVOL_GYRO_POLL_DEFAULT_SIZE          (4)
#define FTEN_NONVOL_GYRO_POLL_HEALTH_CHECK_SIZE     (4)
#define FTEN_NONVOL_GYRO_WAIT_UNLOCKWP_SIZE         (2)
#define FTEN_NONVOL_GYRO_WAIT_STAT_UPDATE_SIZE      (2)
#define FTEN_NONVOL_GYRO_WAIT_SYSTEM_RESET_SIZE     (2)
#define FTEN_NONVOL_TOUCH_WAIT_SYSTEM_RESET_SIZE    (2)
#define FTEN_NONVOL_TOUCH_POLL_DEFAULT_SIZE         (2)
#define FTEN_NONVOL_TOUCH_POLL_THRESHOLD_SIZE       (1)
#define FTEN_NONVOL_USBOCP_POLL_DEFAULT_SIZE        (1)
#define FTEN_NONVOL_USBOCP_POLL_THRESHOLD_SIZE      (1)
#define FTEN_NONVOL_USBOCP_POLL_DELAY_SIZE          (1)
#define FTEN_NONVOL_TI_SLEEP_WL_EN_ON_BEFORE_SIZE   (1)
#define FTEN_NONVOL_TI_SLEEP_BT_ON_BTEN_OFF_SIZE    (1)
#define FTEN_NONVOL_TI_SLEEP_BT_ON_BTEN_ON_SIZE     (1)
#define FTEN_NONVOL_TI_SLEEP_BT_OFF_BTEN_OFF_SIZE   (1)
#define FTEN_NONVOL_TI_SLEEP_BT_OFF_BTEN_ON_SIZE    (1)
#define FTEN_NONVOL_TI_SLEEP_WL_EN_OFF_AFTER_SIZE   (1)
#define FTEN_NONVOL_TI_WAIT_BAUDRATE_STABLE_SIZE    (1)
#define FTEN_NONVOL_FDR_SIZE                        (1)
#define FTEN_NONVOL_TMP401_CONVERSION_RATE_SIZE     (1)
#define FTEN_NONVOL_VOLATILEUPDATE_SIZE             (1)
#define FTEN_NONVOL_TOUCH_MMAP_HEADER_SIZE          (4)
#define FTEN_NONVOL_TOUCH_MMAP_DATA_SIZE            (288)
#define FTEN_NONVOL_TOUCH_EXEC_SYSTEM_RESET_SIZE    (2)
#define FTEN_NONVOL_TOUCH_SELFTEST_WAIT_TIME_SIZE   (2)
#define FTEN_NONVOL_TOUCH_SELFTEST_EXEC_COUNT_SIZE  (1)
#define FTEN_NONVOL_COLOR_VARIATION_INIT_SIZE       (1)


// 可変領域
#define FTEN_NONVOL_MIC                             (0x1802)
#define FTEN_NONVOL_OSUPDATE_BOOTFLAG               (0x1810)
#define FTEN_NONVOL_OSUPDATE_WRITING                (0x1811)
#define FTEN_NONVOL_OSUPDATE_TYPE                   (0x1812)
#define FTEN_NONVOL_OSUPDATE_BOOT_RETRY             (0x1813)
#define FTEN_NONVOL_OSUPDATE_WRITING_RECOVERY       (0x1814)
#define FTEN_NONVOL_OSUPDATE_MAJOR_VERSION          (0x1815)
#define FTEN_NONVOL_OSUPDATE_MINOR_VERSION          (0x1816)
#define FTEN_NONVOL_DIAG_DEVELOPER_OPT              (0x181B)
#define FTEN_NONVOL_DATACLEAR_FLAG                  (0x181C)
#define FTEN_NONVOL_WIPE_DATA_FLAG                  (0x181D)
#define FTEN_NONVOL_BL_UPDATE_ERROR_ID              (0x181E)
#define FTEN_NONVOL_TI_WIFI_NVS_FILE                (0x182F)
#define FTEN_NONVOL_WIFI_MAC_ADDRESS                (0x1C88)
#define FTEN_NONVOL_SYSTEM_DAY_BRIGHTNESS           (0x1C98)
#define FTEN_NONVOL_SYSTEM_DAY_CONTRAST             (0x1C99)
#define FTEN_NONVOL_SYSTEM_DAY_BLACKLEVEL           (0x1C9A)
#define FTEN_NONVOL_SYSTEM_NIGHT_BRIGHTNESS         (0x1C9B)
#define FTEN_NONVOL_SYSTEM_NIGHT_CONTRAST           (0x1C9C)
#define FTEN_NONVOL_SYSTEM_NIGHT_BLACKLEVEL         (0x1C9D)
#define FTEN_NONVOL_AUDIO_DAY_BRIGHTNESS            (0x1C9E)
#define FTEN_NONVOL_AUDIO_DAY_CONTRAST              (0x1C9F)
#define FTEN_NONVOL_AUDIO_DAY_BLACKLEVEL            (0x1CA0)
#define FTEN_NONVOL_AUDIO_DAY_COLOR                 (0x1CA1)
#define FTEN_NONVOL_AUDIO_DAY_TINT                  (0x1CA2)
#define FTEN_NONVOL_AUDIO_NIGHT_BRIGHTNESS          (0x1CA3)
#define FTEN_NONVOL_AUDIO_NIGHT_CONTRAST            (0x1CA4)
#define FTEN_NONVOL_AUDIO_NIGHT_BLACKLEVEL          (0x1CA5)
#define FTEN_NONVOL_AUDIO_NIGHT_COLOR               (0x1CA6)
#define FTEN_NONVOL_AUDIO_NIGHT_TINT                (0x1CA7)
#define FTEN_NONVOL_REAR_DAY_BRIGHTNESS             (0x1CA8)
#define FTEN_NONVOL_REAR_DAY_CONTRAST               (0x1CA9)
#define FTEN_NONVOL_REAR_DAY_BLACKLEVEL             (0x1CAA)
#define FTEN_NONVOL_REAR_DAY_COLOR                  (0x1CAB)
#define FTEN_NONVOL_REAR_DAY_TINT                   (0x1CAC)
#define FTEN_NONVOL_REAR_NIGHT_BRIGHTNESS           (0x1CAD)
#define FTEN_NONVOL_REAR_NIGHT_CONTRAST             (0x1CAE)
#define FTEN_NONVOL_REAR_NIGHT_BLACKLEVEL           (0x1CAF)
#define FTEN_NONVOL_REAR_NIGHT_COLOR                (0x1CB0)
#define FTEN_NONVOL_REAR_NIGHT_TINT                 (0x1CB1)
#define FTEN_NONVOL_FRONT_DAY_BRIGHTNESS            (0x1CB2)
#define FTEN_NONVOL_FRONT_DAY_CONTRAST              (0x1CB3)
#define FTEN_NONVOL_FRONT_DAY_BLACKLEVEL            (0x1CB4)
#define FTEN_NONVOL_FRONT_DAY_COLOR                 (0x1CB5)
#define FTEN_NONVOL_FRONT_DAY_TINT                  (0x1CB6)
#define FTEN_NONVOL_FRONT_NIGHT_BRIGHTNESS          (0x1CB7)
#define FTEN_NONVOL_FRONT_NIGHT_CONTRAST            (0x1CB8)
#define FTEN_NONVOL_FRONT_NIGHT_BLACKLEVEL          (0x1CB9)
#define FTEN_NONVOL_FRONT_NIGHT_COLOR               (0x1CBA)
#define FTEN_NONVOL_FRONT_NIGHT_TINT                (0x1CBB)
#define FTEN_NONVOL_CORNER_DAY_BRIGHTNESS           (0x1CBC)
#define FTEN_NONVOL_CORNER_DAY_CONTRAST             (0x1CBD)
#define FTEN_NONVOL_CORNER_DAY_BLACKLEVEL           (0x1CBE)
#define FTEN_NONVOL_CORNER_DAY_COLOR                (0x1CBF)
#define FTEN_NONVOL_CORNER_DAY_TINT                 (0x1CC0)
#define FTEN_NONVOL_CORNER_NIGHT_BRIGHTNESS         (0x1CC1)
#define FTEN_NONVOL_CORNER_NIGHT_CONTRAST           (0x1CC2)
#define FTEN_NONVOL_CORNER_NIGHT_BLACKLEVEL         (0x1CC3)
#define FTEN_NONVOL_CORNER_NIGHT_COLOR              (0x1CC4)
#define FTEN_NONVOL_CORNER_NIGHT_TINT               (0x1CC5)
#define FTEN_NONVOL_MVC_DAY_BRIGHTNESS              (0x1CC6)
#define FTEN_NONVOL_MVC_DAY_CONTRAST                (0x1CC7)
#define FTEN_NONVOL_MVC_DAY_BLACKLEVEL              (0x1CC8)
#define FTEN_NONVOL_MVC_DAY_COLOR                   (0x1CC9)
#define FTEN_NONVOL_MVC_DAY_TINT                    (0x1CCA)
#define FTEN_NONVOL_MVC_NIGHT_BRIGHTNESS            (0x1CCB)
#define FTEN_NONVOL_MVC_NIGHT_CONTRAST              (0x1CCC)
#define FTEN_NONVOL_MVC_NIGHT_BLACKLEVEL            (0x1CCD)
#define FTEN_NONVOL_MVC_NIGHT_COLOR                 (0x1CCE)
#define FTEN_NONVOL_MVC_NIGHT_TINT                  (0x1CCF)
#define FTEN_NONVOL_LWATCH_DAY_BRIGHTNESS           (0x1CD0)
#define FTEN_NONVOL_LWATCH_DAY_CONTRAST             (0x1CD1)
#define FTEN_NONVOL_LWATCH_DAY_BLACKLEVEL           (0x1CD2)
#define FTEN_NONVOL_LWATCH_DAY_COLOR                (0x1CD3)
#define FTEN_NONVOL_LWATCH_DAY_TINT                 (0x1CD4)
#define FTEN_NONVOL_LWATCH_NIGHT_BRIGHTNESS         (0x1CD5)
#define FTEN_NONVOL_LWATCH_NIGHT_CONTRAST           (0x1CD6)
#define FTEN_NONVOL_LWATCH_NIGHT_BLACKLEVEL         (0x1CD7)
#define FTEN_NONVOL_LWATCH_NIGHT_COLOR              (0x1CD8)
#define FTEN_NONVOL_LWATCH_NIGHT_TINT               (0x1CD9)
#define FTEN_NONVOL_DIAG_FACEFLAG                   (0x1CDE)
#define FTEN_NONVOL_INFO_SKIN_CHANGE                (0x1CDF)
#define FTEN_NONVOL_MODEL_DISCRIMINANT_CHECKSUM     (0x1CE0)
#define FTEN_NONVOL_PARAMETER_TYPE                  (0x1CE2)
#define FTEN_NONVOL_DESTINATION                     (0x1CE3)
#define FTEN_NONVOL_TEN_PART_NUMBER                 (0x1CE4)
#define FTEN_NONVOL_MANUFACTUREES_PART_NUMBER       (0x1CF3)
#define FTEN_NONVOL_PROTOTYPE_CODE                  (0x1D05)
#define FTEN_NONVOL_MEMORIY_CAPACITY                (0x1D06)
#define FTEN_NONVOL_EMMC_CAPACITY                   (0x1D07)
#define FTEN_NONVOL_FACESWTYPE                      (0x1D08)
#define FTEN_NONVOL_HANDLE                          (0x1D09)
#define FTEN_NONVOL_AMP_TYPE                        (0x1D0A)
#define FTEN_NONVOL_BEEP_TYPE                       (0x1D0B)
#define FTEN_NONVOL_TYPE_OF_RADIO_FREQUENCY         (0x1D0C)
#define FTEN_NONVOL_ASEL                            (0x1D0D)
#define FTEN_NONVOL_DAB                             (0x1D0E)
#define FTEN_NONVOL_RDS_RDBS                        (0x1D0F)
#define FTEN_NONVOL_RDS_TMC                         (0x1D10)
#define FTEN_NONVOL_DECK                            (0x1D11)
#define FTEN_NONVOL_KEY_OFF                         (0x1D12)
#define FTEN_NONVOL_GYRO                            (0x1D13)
#define FTEN_NONVOL_TILT                            (0x1D14)
#define FTEN_NONVOL_FAN_MUTE                        (0x1D15)
#define FTEN_NONVOL_ANTITHEFT_LED                   (0x1D16)
#define FTEN_NONVOL_CAMERA                          (0x1D17)
#define FTEN_NONVOL_CAMERA_WIDE                     (0x1D18)
#define FTEN_NONVOL_CAMERA_CTA                      (0x1D19)
#define FTEN_NONVOL_CAMERA_BSI_CTA                  (0x1D1A)
#define FTEN_NONVOL_LAGUAGE_TYPE                    (0x1D1B)
#define FTEN_NONVOL_TIME_ZONE                       (0x1D1C)
#define FTEN_NONVOL_SUMMER_TIME                     (0x1D1D)
#define FTEN_NONVOL_GUIDELINES_TYPE                 (0x1D1E)
#define FTEN_NONVOL_SOUND_PARAMETER_TYPE            (0x1D1F)
#define FTEN_NONVOL_EC_NC_PARAMETER_TYPE            (0x1D20)
#define FTEN_NONVOL_IMID                            (0x1D21)
#define FTEN_NONVOL_COLOR_VARIATION_SETTINGS        (0x1D22)
#define FTEN_NONVOL_DESIGN_TYPE                     (0x1D23)
#define FTEN_NONVOL_NAVI                            (0x1D24)
#define FTEN_NONVOL_NAVI_MAP_TYPE                   (0x1D25)
#define FTEN_NONVOL_NAVI_PRESENT_LOCATION_TYPE      (0x1D26)
#define FTEN_NONVOL_PARKING_SENSOR                  (0x1D27)
#define FTEN_NONVOL_FUEL_EFFICIENT_DISPLAY_SCALE    (0x1D28)
#define FTEN_NONVOL_CALL_HISTORY_TYPE               (0x1D29)
#define FTEN_NONVOL_HFT_CARRIER_CODE_TYPE           (0x1D2A)
#define FTEN_NONVOL_OP_TYPE                         (0x1D2B)
#define FTEN_NONVOL_BT_PRODUCT_ID                   (0x1D2C)
#define FTEN_NONVOL_PI_PI_PON_VOL                   (0x1D2E)
#define FTEN_NONVOL_INT_VOL_DIR                     (0x1D2F)
#define FTEN_NONVOL_INT_VOL_BCD                     (0x1D3B)
#define FTEN_NONVOL_LOCK_END_HI                     (0x1D47)
#define FTEN_NONVOL_LOCK_END_LO                     (0x1D48)
#define FTEN_NONVOL_LOCK_END_RESERVED_1             (0x1D49)
#define FTEN_NONVOL_LOCK_END_RESERVED_2             (0x1D4A)
#define FTEN_NONVOL_FIX_TO_DYNAMIC_HI               (0x1D4B)
#define FTEN_NONVOL_FIX_TO_DYNAMIC_LO               (0x1D4C)
#define FTEN_NONVOL_DYNAMIC_TO_FIX_HI               (0x1D4D)
#define FTEN_NONVOL_DYNAMIC_TO_FIX_LO               (0x1D4E)
#define FTEN_NONVOL_REARCAMERA_GUIDELINE_VERSION    (0x20E0)
#define FTEN_NONVOL_REARCAMERA_GUIDELINE_VAL        (0x20E4)

#define FTEN_NONVOL_MIC_SIZE                        (1)
#define FTEN_NONVOL_OSUPDATE_BOOTFLAG_SIZE          (1)
#define FTEN_NONVOL_OSUPDATE_WRITING_SIZE           (1)
#define FTEN_NONVOL_OSUPDATE_TYPE_SIZE              (1)
#define FTEN_NONVOL_OSUPDATE_BOOT_RETRY_SIZE        (1)
#define FTEN_NONVOL_OSUPDATE_WRITING_RECOVERY_SIZE  (1)
#define FTEN_NONVOL_OSUPDATE_MAJOR_VERSION_SIZE     (1)
#define FTEN_NONVOL_OSUPDATE_MINOR_VERSION_SIZE     (1)
#define FTEN_NONVOL_DIAG_DEVELOPER_OPT_SIZE         (1)
#define FTEN_NONVOL_DATACLEAR_FLAG_SIZE             (1)
#define FTEN_NONVOL_WIPE_DATA_FLAG_SIZE             (1)
#define FTEN_NONVOL_BL_UPDATE_ERROR_ID_SIZE         (1)
#define FTEN_NONVOL_TI_WIFI_NVS_FILE_SIZE           (1113)
#define FTEN_NONVOL_WIFI_MAC_ADDRESS_SIZE           (6)
#define FTEN_NONVOL_SYSTEM_DAY_BRIGHTNESS_SIZE      (1)
#define FTEN_NONVOL_SYSTEM_DAY_CONTRAST_SIZE        (1)
#define FTEN_NONVOL_SYSTEM_DAY_BLACKLEVEL_SIZE      (1)
#define FTEN_NONVOL_SYSTEM_NIGHT_BRIGHTNESS_SIZE    (1)
#define FTEN_NONVOL_SYSTEM_NIGHT_CONTRAST_SIZE      (1)
#define FTEN_NONVOL_SYSTEM_NIGHT_BLACKLEVEL_SIZE    (1)
#define FTEN_NONVOL_AUDIO_DAY_BRIGHTNESS_SIZE       (1)
#define FTEN_NONVOL_AUDIO_DAY_CONTRAST_SIZE         (1)
#define FTEN_NONVOL_AUDIO_DAY_BLACKLEVEL_SIZE       (1)
#define FTEN_NONVOL_AUDIO_DAY_COLOR_SIZE            (1)
#define FTEN_NONVOL_AUDIO_DAY_TINT_SIZE             (1)
#define FTEN_NONVOL_AUDIO_NIGHT_BRIGHTNESS_SIZE     (1)
#define FTEN_NONVOL_AUDIO_NIGHT_CONTRAST_SIZE       (1)
#define FTEN_NONVOL_AUDIO_NIGHT_BLACKLEVEL_SIZE     (1)
#define FTEN_NONVOL_AUDIO_NIGHT_COLOR_SIZE          (1)
#define FTEN_NONVOL_AUDIO_NIGHT_TINT_SIZE           (1)
#define FTEN_NONVOL_REAR_DAY_BRIGHTNESS_SIZE        (1)
#define FTEN_NONVOL_REAR_DAY_CONTRAST_SIZE          (1)
#define FTEN_NONVOL_REAR_DAY_BLACKLEVEL_SIZE        (1)
#define FTEN_NONVOL_REAR_DAY_COLOR_SIZE             (1)
#define FTEN_NONVOL_REAR_DAY_TINT_SIZE              (1)
#define FTEN_NONVOL_REAR_NIGHT_BRIGHTNESS_SIZE      (1)
#define FTEN_NONVOL_REAR_NIGHT_CONTRAST_SIZE        (1)
#define FTEN_NONVOL_REAR_NIGHT_BLACKLEVEL_SIZE      (1)
#define FTEN_NONVOL_REAR_NIGHT_COLOR_SIZE           (1)
#define FTEN_NONVOL_REAR_NIGHT_TINT_SIZE            (1)
#define FTEN_NONVOL_FRONT_DAY_BRIGHTNESS_SIZE       (1)
#define FTEN_NONVOL_FRONT_DAY_CONTRAST_SIZE         (1)
#define FTEN_NONVOL_FRONT_DAY_BLACKLEVEL_SIZE       (1)
#define FTEN_NONVOL_FRONT_DAY_COLOR_SIZE            (1)
#define FTEN_NONVOL_FRONT_DAY_TINT_SIZE             (1)
#define FTEN_NONVOL_FRONT_NIGHT_BRIGHTNESS_SIZE     (1)
#define FTEN_NONVOL_FRONT_NIGHT_CONTRAST_SIZE       (1)
#define FTEN_NONVOL_FRONT_NIGHT_BLACKLEVEL_SIZE     (1)
#define FTEN_NONVOL_FRONT_NIGHT_COLOR_SIZE          (1)
#define FTEN_NONVOL_FRONT_NIGHT_TINT_SIZE           (1)
#define FTEN_NONVOL_CORNER_DAY_BRIGHTNESS_SIZE      (1)
#define FTEN_NONVOL_CORNER_DAY_CONTRAST_SIZE        (1)
#define FTEN_NONVOL_CORNER_DAY_BLACKLEVEL_SIZE      (1)
#define FTEN_NONVOL_CORNER_DAY_COLOR_SIZE           (1)
#define FTEN_NONVOL_CORNER_DAY_TINT_SIZE            (1)
#define FTEN_NONVOL_CORNER_NIGHT_BRIGHTNESS_SIZE    (1)
#define FTEN_NONVOL_CORNER_NIGHT_CONTRAST_SIZE      (1)
#define FTEN_NONVOL_CORNER_NIGHT_BLACKLEVEL_SIZE    (1)
#define FTEN_NONVOL_CORNER_NIGHT_COLOR_SIZE         (1)
#define FTEN_NONVOL_CORNER_NIGHT_TINT_SIZE          (1)
#define FTEN_NONVOL_MVC_DAY_BRIGHTNESS_SIZE         (1)
#define FTEN_NONVOL_MVC_DAY_CONTRAST_SIZE           (1)
#define FTEN_NONVOL_MVC_DAY_BLACKLEVEL_SIZE         (1)
#define FTEN_NONVOL_MVC_DAY_COLOR_SIZE              (1)
#define FTEN_NONVOL_MVC_DAY_TINT_SIZE               (1)
#define FTEN_NONVOL_MVC_NIGHT_BRIGHTNESS_SIZE       (1)
#define FTEN_NONVOL_MVC_NIGHT_CONTRAST_SIZE         (1)
#define FTEN_NONVOL_MVC_NIGHT_BLACKLEVEL_SIZE       (1)
#define FTEN_NONVOL_MVC_NIGHT_COLOR_SIZE            (1)
#define FTEN_NONVOL_MVC_NIGHT_TINT_SIZE             (1)
#define FTEN_NONVOL_LWATCH_DAY_BRIGHTNESS_SIZE      (1)
#define FTEN_NONVOL_LWATCH_DAY_CONTRAST_SIZE        (1)
#define FTEN_NONVOL_LWATCH_DAY_BLACKLEVEL_SIZE      (1)
#define FTEN_NONVOL_LWATCH_DAY_COLOR_SIZE           (1)
#define FTEN_NONVOL_LWATCH_DAY_TINT_SIZE            (1)
#define FTEN_NONVOL_LWATCH_NIGHT_BRIGHTNESS_SIZE    (1)
#define FTEN_NONVOL_LWATCH_NIGHT_CONTRAST_SIZE      (1)
#define FTEN_NONVOL_LWATCH_NIGHT_BLACKLEVEL_SIZE    (1)
#define FTEN_NONVOL_LWATCH_NIGHT_COLOR_SIZE         (1)
#define FTEN_NONVOL_LWATCH_NIGHT_TINT_SIZE          (1)
#define FTEN_NONVOL_DIAG_FACEFLAG_SIZE              (1)
#define FTEN_NONVOL_INFO_SKIN_CHANGE_SIZE           (1)
#define FTEN_NONVOL_MODEL_DISCRIMINANT_CHECKSUM_SIZE (2)
#define FTEN_NONVOL_PARAMETER_TYPE_SIZE             (1)
#define FTEN_NONVOL_DESTINATION_SIZE                (1)
#define FTEN_NONVOL_TEN_PART_NUMBER_SIZE            (15)
#define FTEN_NONVOL_MANUFACTUREES_PART_NUMBER_SIZE  (18)
#define FTEN_NONVOL_PROTOTYPE_CODE_SIZE             (1)
#define FTEN_NONVOL_MEMORIY_CAPACITY_SIZE           (1)
#define FTEN_NONVOL_EMMC_CAPACITY_SIZE              (1)
#define FTEN_NONVOL_FACESWTYPE_SIZE                 (1)
#define FTEN_NONVOL_HANDLE_SIZE                     (1)
#define FTEN_NONVOL_AMP_TYPE_SIZE                   (1)
#define FTEN_NONVOL_BEEP_TYPE_SIZE                  (1)
#define FTEN_NONVOL_TYPE_OF_RADIO_FREQUENCY_SIZE    (1)
#define FTEN_NONVOL_ASEL_SIZE                       (1)
#define FTEN_NONVOL_DAB_SIZE                        (1)
#define FTEN_NONVOL_RDS_RDBS_SIZE                   (1)
#define FTEN_NONVOL_RDS_TMC_SIZE                    (1)
#define FTEN_NONVOL_DECK_SIZE                       (1)
#define FTEN_NONVOL_KEY_OFF_SIZE                    (1)
#define FTEN_NONVOL_GYRO_SIZE                       (1)
#define FTEN_NONVOL_TILT_SIZE                       (1)
#define FTEN_NONVOL_FAN_MUTE_SIZE                   (1)
#define FTEN_NONVOL_ANTITHEFT_LED_SIZE              (1)
#define FTEN_NONVOL_CAMERA_SIZE                     (1)
#define FTEN_NONVOL_CAMERA_WIDE_SIZE                (1)
#define FTEN_NONVOL_CAMERA_CTA_SIZE                 (1)
#define FTEN_NONVOL_CAMERA_BSI_CTA_SIZE             (1)
#define FTEN_NONVOL_LAGUAGE_TYPE_SIZE               (1)
#define FTEN_NONVOL_TIME_ZONE_SIZE                  (1)
#define FTEN_NONVOL_SUMMER_TIME_SIZE                (1)
#define FTEN_NONVOL_GUIDELINES_TYPE_SIZE            (1)
#define FTEN_NONVOL_SOUND_PARAMETER_TYPE_SIZE       (1)
#define FTEN_NONVOL_EC_NC_PARAMETER_TYPE_SIZE       (1)
#define FTEN_NONVOL_IMID_SIZE                       (1)
#define FTEN_NONVOL_COLOR_VARIATION_SETTINGS_SIZE   (1)
#define FTEN_NONVOL_DESIGN_TYPE_SIZE                (1)
#define FTEN_NONVOL_NAVI_SIZE                       (1)
#define FTEN_NONVOL_NAVI_MAP_TYPE_SIZE              (1)
#define FTEN_NONVOL_NAVI_PRESENT_LOCATION_TYPE_SIZE (1)
#define FTEN_NONVOL_PARKING_SENSOR_SIZE             (1)
#define FTEN_NONVOL_FUEL_EFFICIENT_DISPLAY_SCALE_SIZE (1)
#define FTEN_NONVOL_CALL_HISTORY_TYPE_SIZE          (1)
#define FTEN_NONVOL_HFT_CARRIER_CODE_TYPE_SIZE      (1)
#define FTEN_NONVOL_OP_TYPE_SIZE                    (1)
#define FTEN_NONVOL_BT_PRODUCT_ID_SIZE              (2)
#define FTEN_NONVOL_PI_PI_PON_VOL_SIZE              (1)
#define FTEN_NONVOL_INT_VOL_DIR_SIZE                (12)
#define FTEN_NONVOL_INT_VOL_BCD_SIZE                (12)
#define FTEN_NONVOL_LOCK_END_HI_SIZE                (1)
#define FTEN_NONVOL_LOCK_END_LO_SIZE                (1)
#define FTEN_NONVOL_LOCK_END_RESERVED_1_SIZE        (1)
#define FTEN_NONVOL_LOCK_END_RESERVED_2_SIZE        (1)
#define FTEN_NONVOL_FIX_TO_DYNAMIC_HI_SIZE          (1)
#define FTEN_NONVOL_FIX_TO_DYNAMIC_LO_SIZE          (1)
#define FTEN_NONVOL_DYNAMIC_TO_FIX_HI_SIZE          (1)
#define FTEN_NONVOL_DYNAMIC_TO_FIX_LO_SIZE          (1)
#define FTEN_NONVOL_REARCAMERA_GUIDELINE_VERSION_SIZE (4)
#define FTEN_NONVOL_REARCAMERA_GUIDELINE_VAL_SIZE   (3192)


// 固定領域（AK7736-PRAM）
#define FTEN_NONVOL_AK7736_DSP_PRAM                 (0x3000)

#define FTEN_NONVOL_AK7736_DSP_PRAM_SIZE            (31232)


// 可変領域（AK7736-CRAM）
#define FTEN_NONVOL_AK7736_DSP_CRAM                 (0xAA00)

#define FTEN_NONVOL_AK7736_DSP_CRAM_SIZE            (12800)


// 可変領域（AK7736-REG）
#define FTEN_NONVOL_AK7736_DSP_PARAMETER            (0xDC00)

#define FTEN_NONVOL_AK7736_DSP_PARAMETER_SIZE       (1024)


// Refresh領域
#define FTEN_NONVOL_REFRESH                         (0x100000)

#define FTEN_NONVOL_REFRESH_SIZE                    (30720)



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _NONVOLATILE_DEF_H */