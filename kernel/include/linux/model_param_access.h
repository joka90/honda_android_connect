/*----------------------------------------------------------------------------*/
// COPYRIGHT(C) FUJITSU TEN LIMITED 2013
/*----------------------------------------------------------------------------*/

#ifndef _MODEL_PARAM_ACCESS_H
#define _MODEL_PARAM_ACCESS_H

#include <linux/nonvolatile.h>
#include <linux/nonvolatile_def.h>
#include <linux/model_param_def.h>

typedef struct {
    int ADR;
} NV_DATA;

NV_DATA MODELPARAMETER_ENABLED[] = {
    FTEN_NONVOL_MODEL_DISCRIMINANT_CHECKSUM,
    FTEN_NONVOL_PARAMETER_TYPE,
    FTEN_NONVOL_DESTINATION,
    FTEN_NONVOL_TEN_PART_NUMBER,
    FTEN_NONVOL_MANUFACTUREES_PART_NUMBER,
    FTEN_NONVOL_PROTOTYPE_CODE,
    FTEN_NONVOL_MEMORIY_CAPACITY,
    FTEN_NONVOL_EMMC_CAPACITY,
    FTEN_NONVOL_FACESWTYPE,
    FTEN_NONVOL_HANDLE,
    FTEN_NONVOL_AMP_TYPE,
    FTEN_NONVOL_BEEP_TYPE,
    FTEN_NONVOL_TYPE_OF_RADIO_FREQUENCY,
    FTEN_NONVOL_ASEL,
    FTEN_NONVOL_DAB,
    FTEN_NONVOL_RDS_RDBS,
    FTEN_NONVOL_RDS_TMC,
    FTEN_NONVOL_DECK,
    FTEN_NONVOL_KEY_OFF,
    FTEN_NONVOL_GYRO,
    FTEN_NONVOL_TILT,
    FTEN_NONVOL_FAN_MUTE,
    FTEN_NONVOL_ANTITHEFT_LED,
    FTEN_NONVOL_CAMERA,
    FTEN_NONVOL_CAMERA_WIDE,
    FTEN_NONVOL_CAMERA_CTA,
    FTEN_NONVOL_CAMERA_BSI_CTA,
    FTEN_NONVOL_LAGUAGE_TYPE,
    FTEN_NONVOL_TIME_ZONE,
    FTEN_NONVOL_SUMMER_TIME,
    FTEN_NONVOL_GUIDELINES_TYPE,
    FTEN_NONVOL_SOUND_PARAMETER_TYPE,
    FTEN_NONVOL_EC_NC_PARAMETER_TYPE,
    FTEN_NONVOL_IMID,
    FTEN_NONVOL_COLOR_VARIATION_SETTINGS,
    FTEN_NONVOL_DESIGN_TYPE,
    FTEN_NONVOL_NAVI,
    FTEN_NONVOL_NAVI_MAP_TYPE,
    FTEN_NONVOL_NAVI_PRESENT_LOCATION_TYPE,
    FTEN_NONVOL_PARKING_SENSOR,
    FTEN_NONVOL_FUEL_EFFICIENT_DISPLAY_SCALE,
    FTEN_NONVOL_CALL_HISTORY_TYPE,
    FTEN_NONVOL_HFT_CARRIER_CODE_TYPE,
    FTEN_NONVOL_OP_TYPE,
    FTEN_NONVOL_BT_PRODUCT_ID,
    FTEN_NONVOL_PI_PI_PON_VOL,
    FTEN_NONVOL_INT_VOL_DIR,
    FTEN_NONVOL_INT_VOL_BCD,
    FTEN_NONVOL_LOCK_END_HI,
    FTEN_NONVOL_LOCK_END_LO,
    FTEN_NONVOL_LOCK_END_RESERVED_1,
    FTEN_NONVOL_LOCK_END_RESERVED_2,
    FTEN_NONVOL_FIX_TO_DYNAMIC_HI,
    FTEN_NONVOL_FIX_TO_DYNAMIC_LO,
    FTEN_NONVOL_DYNAMIC_TO_FIX_HI,
    FTEN_NONVOL_DYNAMIC_TO_FIX_LO,
};

static inline int get_model_param_data(uint8_t* buff, unsigned int model_param, unsigned int size) {
    int result = 0;
    int nvdatanum = sizeof(MODELPARAMETER_ENABLED)/sizeof(NV_DATA);

    if(model_param >= nvdatanum) {
        return -EINVAL;
    }

    result = GetNONVOLATILE( buff, (unsigned int)MODELPARAMETER_ENABLED[model_param].ADR, size );

    return result;
}

#endif /* _MODEL_PARAM_ACCESS_H */

