/*
 * Copyright(C) 2012 FUJITSU TEN LIMITED
 */

#ifndef _LIB_NONVOLATILE_H
#define _LIB_NONVOLATILE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <sys/types.h>
#include <linux/nonvolatile_def.h>

int GetNONVOLATILE(uint8_t *, unsigned int, unsigned int);
int SetNONVOLATILE(uint8_t *, unsigned int, unsigned int);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* _LIB_NONVOLATILE_H */
