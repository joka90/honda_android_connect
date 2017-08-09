/*
 * include/linux/i2c/lis331dlh.h
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

#ifndef LIS331DLH_HEADER
#define LIS331DLH_HEADER

#ifdef CONFIG_SENSOR_TARGET_VERSION_2S
#define TEGRA_LIS331DLH_I2C_BUS 1
#else
#define TEGRA_LIS331DLH_I2C_BUS 0
#endif

#define LIS331DLH_I2C_ADDR 0x19


#define LIS331DLH_CSTATUS_INITIALIZE    "10"
#define LIS331DLH_CSTATUS_DISABLE       "20"
#define LIS331DLH_CSTATUS_ENABLE        "30"
#define LIS331DLH_CSTATUS_SUSPEND       "40"
#define LIS331DLH_CSTATUS_ERROR2        "62"


#endif
