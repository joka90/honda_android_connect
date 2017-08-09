/*
 * Copyright(C) 2013 FUJITSU TEN LIMITED
 */

#ifndef _LIB_MODEL_PARAM_ACCESS_H
#define _LIB_MODEL_PARAM_ACCESS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <sys/types.h>
#include <linux/model_param_def.h>

int get_model_parameter_data(uint8_t *, unsigned int, unsigned int);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* _LIB_MODEL_PARAM_ACCESS_H */
