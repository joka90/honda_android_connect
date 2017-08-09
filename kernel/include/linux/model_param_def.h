/*
 * Copyright(C) 2013 FUJITSU TEN LIMITED
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

#ifndef _MODEL_PARAM_DEF_H
#define _MODEL_PARAM_DEF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MODEL_PARAM_MODEL_DISCRIMINANT_CHECKSUM          (0)
#define MODEL_PARAM_PARAMETER_TYPE                       (1)
#define MODEL_PARAM_DESTINATION                          (2)
#define MODEL_PARAM_TEN_PART_NUMBER                      (3)
#define MODEL_PARAM_MANUFACTUREES_PART_NUMBER            (4)
#define MODEL_PARAM_PROTOTYPE_CODE                       (5)
#define MODEL_PARAM_MEMORIY_CAPACITY                     (6)
#define MODEL_PARAM_EMMC_CAPACITY                        (7)
#define MODEL_PARAM_FACESWTYPE                           (8)
#define MODEL_PARAM_HANDLE                               (9)
#define MODEL_PARAM_AMP_TYPE                             (10)
#define MODEL_PARAM_BEEP_TYPE                            (11)
#define MODEL_PARAM_TYPE_OF_RADIO_FREQUENCY              (12)
#define MODEL_PARAM_ASEL                                 (13)
#define MODEL_PARAM_DAB                                  (14)
#define MODEL_PARAM_RDS_RDBS                             (15)
#define MODEL_PARAM_RDS_TMC                              (16)
#define MODEL_PARAM_DECK                                 (17)
#define MODEL_PARAM_KEY_OFF                              (18)
#define MODEL_PARAM_GYRO                                 (19)
#define MODEL_PARAM_TILT                                 (20)
#define MODEL_PARAM_FAN_MUTE                             (21)
#define MODEL_PARAM_ANTITHEFT_LED                        (22)
#define MODEL_PARAM_CAMERA                               (23)
#define MODEL_PARAM_CAMERA_WIDE                          (24)
#define MODEL_PARAM_CAMERA_CTA                           (25)
#define MODEL_PARAM_CAMERA_BSI_CTA                       (26)
#define MODEL_PARAM_LAGUAGE_TYPE                         (27)
#define MODEL_PARAM_TIME_ZONE                            (28)
#define MODEL_PARAM_SUMMER_TIME                          (29)
#define MODEL_PARAM_GUIDELINES_TYPE                      (30)
#define MODEL_PARAM_SOUND_PARAMETER_TYPE                 (31)
#define MODEL_PARAM_EC_NC_PARAMETER_TYPE                 (32)
#define MODEL_PARAM_IMID                                 (33)
#define MODEL_PARAM_COLOR_VARIATION_SETTINGS             (34)
#define MODEL_PARAM_DESIGN_TYPE                          (35)
#define MODEL_PARAM_NAVI                                 (36)
#define MODEL_PARAM_NAVI_MAP_TYPE                        (37)
#define MODEL_PARAM_NAVI_PRESENT_LOCATION_TYPE           (38)
#define MODEL_PARAM_PARKING_SENSOR                       (39)
#define MODEL_PARAM_FUEL_EFFICIENT_DISPLAY_SCALE         (40)
#define MODEL_PARAM_CALL_HISTORY_TYPE                    (41)
#define MODEL_PARAM_HFT_CARRIER_CODE_TYPE                (42)
#define MODEL_PARAM_OP_TYPE                              (43)
#define MODEL_PARAM_BT_PRODUCT_ID                        (44)
#define MODEL_PARAM_PI_PI_PON_VOL                        (45)
#define MODEL_PARAM_INT_VOL_DIR                          (46)
#define MODEL_PARAM_INT_VOL_BCD                          (47)
#define MODEL_PARAM_LOCK_END_HI                          (48)
#define MODEL_PARAM_LOCK_END_LO                          (49)
#define MODEL_PARAM_LOCK_END_RESERVED_1                  (50)
#define MODEL_PARAM_LOCK_END_RESERVED_2                  (51)
#define MODEL_PARAM_FIX_TO_DYNAMIC_HI                    (52)
#define MODEL_PARAM_FIX_TO_DYNAMIC_LO                    (53)
#define MODEL_PARAM_DYNAMIC_TO_FIX_HI                    (54)
#define MODEL_PARAM_DYNAMIC_TO_FIX_LO                    (55)

/* size */
#define MODEL_PARAM_MODEL_DISCRIMINANT_CHECKSUM_SIZE     (2)
#define MODEL_PARAM_PARAMETER_TYPE_SIZE                  (1)
#define MODEL_PARAM_DESTINATION_SIZE                     (1)
#define MODEL_PARAM_TEN_PART_NUMBER_SIZE                 (15)
#define MODEL_PARAM_MANUFACTUREES_PART_NUMBER_SIZE       (18)
#define MODEL_PARAM_PROTOTYPE_CODE_SIZE                  (1)
#define MODEL_PARAM_MEMORIY_CAPACITY_SIZE                (1)
#define MODEL_PARAM_EMMC_CAPACITY_SIZE                   (1)
#define MODEL_PARAM_FACESWTYPE_SIZE                      (1)
#define MODEL_PARAM_HANDLE_SIZE                          (1)
#define MODEL_PARAM_AMP_TYPE_SIZE                        (1)
#define MODEL_PARAM_BEEP_TYPE_SIZE                       (1)
#define MODEL_PARAM_TYPE_OF_RADIO_FREQUENCY_SIZE         (1)
#define MODEL_PARAM_ASEL_SIZE                            (1)
#define MODEL_PARAM_DAB_SIZE                             (1)
#define MODEL_PARAM_RDS_RDBS_SIZE                        (1)
#define MODEL_PARAM_RDS_TMC_SIZE                         (1)
#define MODEL_PARAM_DECK_SIZE                            (1)
#define MODEL_PARAM_KEY_OFF_SIZE                         (1)
#define MODEL_PARAM_GYRO_SIZE                            (1)
#define MODEL_PARAM_TILT_SIZE                            (1)
#define MODEL_PARAM_FAN_MUTE_SIZE                        (1)
#define MODEL_PARAM_ANTITHEFT_LED_SIZE                   (1)
#define MODEL_PARAM_CAMERA_SIZE                          (1)
#define MODEL_PARAM_CAMERA_WIDE_SIZE                     (1)
#define MODEL_PARAM_CAMERA_CTA_SIZE                      (1)
#define MODEL_PARAM_CAMERA_BSI_CTA_SIZE                  (1)
#define MODEL_PARAM_LAGUAGE_TYPE_SIZE                    (1)
#define MODEL_PARAM_TIME_ZONE_SIZE                       (1)
#define MODEL_PARAM_SUMMER_TIME_SIZE                     (1)
#define MODEL_PARAM_GUIDELINES_TYPE_SIZE                 (1)
#define MODEL_PARAM_SOUND_PARAMETER_TYPE_SIZE            (1)
#define MODEL_PARAM_EC_NC_PARAMETER_TYPE_SIZE            (1)
#define MODEL_PARAM_IMID_SIZE                            (1)
#define MODEL_PARAM_COLOR_VARIATION_SETTINGS_SIZE        (1)
#define MODEL_PARAM_DESIGN_TYPE_SIZE                     (1)
#define MODEL_PARAM_NAVI_SIZE                            (1)
#define MODEL_PARAM_NAVI_MAP_TYPE_SIZE                   (1)
#define MODEL_PARAM_NAVI_PRESENT_LOCATION_TYPE_SIZE      (1)
#define MODEL_PARAM_PARKING_SENSOR_SIZE                  (1)
#define MODEL_PARAM_FUEL_EFFICIENT_DISPLAY_SCALE_SIZE    (1)
#define MODEL_PARAM_CALL_HISTORY_TYPE_SIZE               (1)
#define MODEL_PARAM_HFT_CARRIER_CODE_TYPE_SIZE           (1)
#define MODEL_PARAM_OP_TYPE_SIZE                         (1)
#define MODEL_PARAM_BT_PRODUCT_ID_SIZE                   (2)
#define MODEL_PARAM_PI_PI_PON_VOL_SIZE                   (1)
#define MODEL_PARAM_INT_VOL_DIR_SIZE                     (12)
#define MODEL_PARAM_INT_VOL_BCD_SIZE                     (12)
#define MODEL_PARAM_LOCK_END_HI_SIZE                     (1)
#define MODEL_PARAM_LOCK_END_LO_SIZE                     (1)
#define MODEL_PARAM_LOCK_END_RESERVED_1_SIZE             (1)
#define MODEL_PARAM_LOCK_END_RESERVED_2_SIZE             (1)
#define MODEL_PARAM_FIX_TO_DYNAMIC_HI_SIZE               (1)
#define MODEL_PARAM_FIX_TO_DYNAMIC_LO_SIZE               (1)
#define MODEL_PARAM_DYNAMIC_TO_FIX_HI_SIZE               (1)
#define MODEL_PARAM_DYNAMIC_TO_FIX_LO_SIZE               (1)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _MODEL_PARAM_DEF_H */
