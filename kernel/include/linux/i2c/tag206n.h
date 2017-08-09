/*
 * include/linux/i2c/tag206n.h
 *
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


#ifndef TAG206N_HEADER
#define TAG206N_HEADER

#if  defined(CONFIG_FJFEAT_PRODUCT_ADA1S)
 #define TEGRA_TAG206N_I2C_BUS 0
 #define TAG206N_I2C_ADDR 0x4C
#else
 #define TEGRA_TAG206N_I2C_BUS 1
 #define TAG206N_I2C_ADDR 0x4D
#endif


#define TAG206N_CSTATUS_INITIALIZE   "10"
#define TAG206N_CSTATUS_DISABLE      "20"
#define TAG206N_CSTATUS_ENABLE       "30"
#define TAG206N_CSTATUS_SUSPEND      "40"
#define TAG206N_CSTATUS_ERROR1       "61"
#define TAG206N_CSTATUS_ERROR2       "62"


#endif
