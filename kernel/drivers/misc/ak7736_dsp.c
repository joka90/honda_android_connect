/**
 * @file ak7736_dsp.c
 *
 * AK7736 DSP Control driver
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

#include <asm/uaccess.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/crc-itu-t.h>
#include <linux/earlydevice.h>
#include <linux/i2c/ak7736_dsp.h>
#include "ak7736_dsp_priv.h"

#ifdef CONFIG_AK7736_PSEUDO_NONVOLATILE_USE
#include "pseudo_nonvolatile.h"
#else
#include <linux/nonvolatile.h>
#endif



static int device_power = 1;

static const struct i2c_device_id ak7736_dsp_id[] = {
    {AK7736_DSP_DEVICE_NAME, 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, ak7736_dsp_id);


/**
 * @brief       Writing the DSP data to I2C.
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[in]   len Data length.
 * @param[in]   *data Writing data.
 * @param[in]   retry Writing data count (but, error occurrsed).
 * @retval      0 Normal return.
 * @retval      -EIO I2C access error.
 */
static int ak7736_dsp_write(struct i2c_client *client,
                            unsigned int len, unsigned char const *data,
                            int retry)
{
    struct i2c_msg msgs[1];
    int ret = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    msgs[0].addr  = AK7736_DSP_I2C_ADDR;
    msgs[0].flags = 0; /* write (Master to Slave) */
    msgs[0].buf   = (unsigned char *) data;
    msgs[0].len   = len;

    while ( device_power && (retry--)) {
        ret = i2c_transfer(client->adapter, msgs, 1);
        if (ret == 0) {
            ret = -EIO;
        } else if (ret == 1) {
            ret = 0; /* success*/
            break;
        }
    }

    if (!device_power) ret = -ESHUTDOWN;

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Reading the DSP data from I2C.
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[in]   reg I2C register (AK7736 DSP).
 * @param[in]   len Data length.
 * @param[out]  *data Reading data.
 * @param[in]   retry Reading data count (but, error occurrsed).
 * @retval      0 Normal return.
 * @retval      -EIO I2C access error.
 */
static int ak7736_dsp_read(struct i2c_client *client,
                           unsigned char reg,
                           int   len,
                           char *data,
                           int   retry)
{
    struct i2c_msg msgs[2];
    int ret = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    msgs[0].addr  = AK7736_DSP_I2C_ADDR;
    msgs[0].flags = 0; /* reg write */
    msgs[0].buf   = &reg;
    msgs[0].len   = 1;

    msgs[1].addr  = AK7736_DSP_I2C_ADDR;
    msgs[1].flags = I2C_M_RD; /* read (Slave to Master)*/
    msgs[1].buf   = data;
    msgs[1].len   = len;

    while ( device_power && (retry--)) {
        ret = i2c_transfer(client->adapter, msgs, 2);
        if ((ret >= 0) && (ret < 2)) {
            ret = -EIO;
        } else if (ret == 2) {
            ret = 0; /* success*/
            break;
        }
    }

    if (!device_power) ret = -ESHUTDOWN;

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Dumping the DSP data to I2C.
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[in]   reg I2C register (AK7736 DSP).
 * @param[in]   addr I2C register address.
 * @param[in]   len Data length.
 * @param[in]   *data Dumping data.
 * @param[in]   retry Dumping data count (but, error occurrsed).
 * @retval      0 Normal return.
 * @retval      -EIO I2C access error.
 */
static int ak7736_dsp_dump(struct i2c_client *client,
                           unsigned char  reg,
                           unsigned short addr,
                           int   len,
                           char *data,
                           int   retry)
{
    struct i2c_msg msgs[2];
    char buff[3];
    int ret = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    buff[0] = reg;
    buff[1] = addr >> 8;
    buff[2] = addr & 0xff;

    msgs[0].addr  = AK7736_DSP_I2C_ADDR;
    msgs[0].flags = 0; /* reg write */
    msgs[0].buf   = buff;
    msgs[0].len   = 3;

    msgs[1].addr  = AK7736_DSP_I2C_ADDR;
    msgs[1].flags = I2C_M_RD; /* read (Slave to Master)*/
    msgs[1].buf   = data;
    msgs[1].len   = len;

    while ( device_power && (retry--)) {
        ret = i2c_transfer(client->adapter, msgs, 2);
        if ((ret >= 0) && (ret < 2)) {
            ret = -EIO;
        } else if (ret == 2) {
            ret = 0; /* success*/
            break;
        }
    }

    if (!device_power) ret = -ESHUTDOWN;

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}


/**
 * @brief       Writing the DSP data (1Byte) to I2C. 
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[in]   reg I2C register (AK7736 DSP)
 * @param[in]   *data Writing data.
 * @return      ak7736_dsp_write function return.
 */
static int ak7736_dsp_contreg_write_byte(struct i2c_client *client,
                                         unsigned char reg, char data)
{
    char buff[2];

    AK7736_DEBUG_FUNCTION_ENTER_ONLY
    buff[0] = reg | AK7736_DSP_REG_WRITE_MASK;
    buff[1] = data;

    return ak7736_dsp_write(client, 2, buff, AK7736_DSP_I2C_RETRY_COUNT);
}

/**
 * @brief       Reading the DSP data (1Byte) from I2C. 
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[in]   reg I2C register (AK7736 DSP)
 * @param[in]   *data Reading data.
 * @return      ak7736_dsp_read function return.
 */
static int ak7736_dsp_contreg_read_byte(struct i2c_client *client,
                                        unsigned char reg, char *data)
{
    AK7736_DEBUG_FUNCTION_ENTER_ONLY
    return ak7736_dsp_read(client, reg, 1, data, AK7736_DSP_I2C_RETRY_COUNT);
}

#if 0
/**
 * @brief       Writing the DSP data (2Byte) to I2C. 
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[in]   reg I2C register (AK7736 DSP)
 * @param[in]   *data Writing data.
 * @return      ak7736_dsp_write function return.
 */
static int ak7736_dsp_contreg_write_word(struct i2c_client *client,
                                         unsigned char reg, short int data)
{
    char buff[3];

    AK7736_DEBUG_FUNCTION_ENTER_ONLY

    buff[0] = reg | AK7736_DSP_REG_WRITE_MASK;
    buff[1] = (data >> 8);
    buff[2] = data & 0xff;

    return ak7736_dsp_write(client, 3, buff, AK7736_DSP_I2C_RETRY_COUNT);
}
#endif

/**
 * @brief       Reading the DSP data (1Byte) from I2C. 
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[in]   reg I2C register (AK7736 DSP)
 * @param[in]   *data Reading data.
 * @return      ak7736_dsp_read function return.
 */
static int ak7736_dsp_contreg_read_word(struct i2c_client *client,
                                        unsigned char reg,
                                        unsigned short int *data)
{
    int ret;

    AK7736_DEBUG_FUNCTION_ENTER

    ret = ak7736_dsp_read(client, reg, 2, (unsigned char *)data,
                          AK7736_DSP_I2C_RETRY_COUNT);
    *data = swab16(*data);

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Sleep function for AK7736 DSP driver
 * @param[in]   time Sleep time
 * @retval      0 Normal return.
 * @retval      -ESHUTDOWN Suspend return.
 */
static inline int ak7736_dsp_msleep(unsigned int time) {
    int ret = 0;
    int elapsed_time = time;
    int calc_time;

    AK7736_DEBUG_FUNCTION_ENTER
    while(elapsed_time > 0) {
        if (!device_power) {
            ret = -ESHUTDOWN;
            break;
        }
        if (elapsed_time < AK7736_DSP_MINIMUM_WAIT_RESOLUTION) {
            calc_time = elapsed_time;
        }else{
            calc_time = AK7736_DSP_MINIMUM_WAIT_RESOLUTION;
        }
        msleep(calc_time);
        elapsed_time -= calc_time;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Set AK7736 DSP status.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   status DSP status.
 * @param[in]   error_status DSP error status.
 * @return      None.
 */
static void ak7736_dsp_set_status(struct akdsp_private_data *priv,
                                  int status,
                                  int error_status)
{
    AK7736_DEBUG_FUNCTION_ENTER
    char msg[32];

    priv->status     = status;
    priv->error_stat = error_status;
    priv->stat_unread++;
    wake_up_interruptible(&priv->statq);

    AK7736_NOTICE_LOG("status change %d %d %d\n",
                      status,
                      error_status,
                      priv->stat_unread);
    snprintf( msg, sizeof(msg), "dsp status change %d %d %d\n",
              status,
              error_status,
              priv->stat_unread);
    AK7736_DRC_TRC( msg );

    AK7736_DEBUG_FUNCTION_EXIT
    return;
}

/**
 * @brief       Get AK7736 DSP status.
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[out]  *stat DSP status pointer.
 * @return      rget_ak_dsp_err return.
 */
static int ak7736_dsp_get_status(struct i2c_client *client,
                                 unsigned char *stat)
{
    struct akdsp_platform_data *pdata = client->dev.platform_data;
    int ret;

    AK7736_DEBUG_FUNCTION_ENTER

    *stat = 0;
    ret = pdata->get_ak_dsp_err();
    ak7736_dsp_contreg_read_byte(client, AK7736_DSP_REG_DSPERROR, stat);

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Convert error number to error state.
 * @param[in]   errno Error number
 * @return      error state
 */
static int ak7736_dsp_errno2errorstat(int errno)
{
    int ret;

    AK7736_DEBUG_FUNCTION_ENTER

    switch (errno) {
    case -ENOMEM:
        ret = AK7736_DSP_ERRSTAT_NOMEM;
        break;

    case -ENOENT:
        ret = AK7736_DSP_ERRSTAT_NONVOLATILE;
        break;

    case -EIO:
    default:
        ret = AK7736_DSP_ERRSTAT_DSP;
        break;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Reset AK7736 DSP.
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @return      None.
 */
static void ak7736_dsp_reset(struct i2c_client *client)
{
    struct akdsp_platform_data *pdata = client->dev.platform_data;

    AK7736_DEBUG_FUNCTION_ENTER

    AK7736_NOTICE_LOG("dsp reset\n");
    AK7736_DRC_TRC( "dsp reset\n" );
    pdata->set_ak_adc_pdn(AK7736_DSP_RESET_LOW);
    msleep(1);
    pdata->set_ak_dsp_pdn(AK7736_DSP_RESET_LOW);
    msleep(AK7736_DSP_SYSTEM_RESET_WAIT);

    pdata->set_ak_dsp_pdn(AK7736_DSP_RESET_HIGH);
#ifdef AK7736_DSP_CLKO_DEFAULT_ON
    pdata->set_ak_adc_pdn(AK7736_DSP_RESET_HIGH);
#endif
    msleep(1);
    AK7736_DEBUG_FUNCTION_EXIT
}

/**
 * @brief       Setup AK7736 DSP register.
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @return      ak7736_dsp_contreg_write_bye return.
 */
static int ak7736_dsp_register_setup(struct i2c_client *client)
{
    int reg;
    int ret;

    AK7736_DEBUG_FUNCTION_ENTER

    /* register setup */
    for (reg = 0 ; reg <= 0x0f ; reg++) {
        ret = ak7736_dsp_contreg_write_byte(client,
                                            AK7736_DSP_REG_CONT00 + reg,
                                            ak7736_dsp_control_register[reg] );
        if (unlikely(ret)) {
            goto error_exit;
        }
    }

    msleep(20);

error_exit:
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}


/* dump for debug */
#ifdef AK7736_DSP_DEBUG_REGISTER_DUMP
/**
 * @brief       Dumping AK7736 DSP register.
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @return      ak7736_dsp_contreg_read_byte return.
 */
static int ak7736_dsp_register_dump(struct i2c_client *client)
{
    unsigned char data;
    int reg;
    int ret;

    AK7736_DEBUG_FUNCTION_ENTER

    /* register dump for debug */
    printk(KERN_INFO "ak7736_dsp: control register dump ");
    for (reg = AK7736_DSP_REG_CONT00 ; reg <= AK7736_DSP_REG_CONT0F ; reg++) {
        ret = ak7736_dsp_contreg_read_byte(client, reg, &data );
        if (unlikely(ret)) {
            goto error_exit;
        }
        printk( "%02x ",data);
    }

    printk( "\n");

error_exit:
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}
#endif


/**
 * @brief       Write a bit, including the reset to DSP register.
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[in]   cont I2C register (AK7736 DSP)
 * @param[in]   bitmap Bit value in DSP register.
 * @param[in]   reset_stat If set to true, reset the DSP register.
 * @retval      0 Normal return.
 * @retval      !=0 Abnormal return.
 */
static int ak7736_dsp_register_bitop(struct i2c_client *client,
                                     unsigned char cont,
                                     unsigned char bitmap,
                                     int reset_stat)
{
    unsigned char data;
    int ret;

    AK7736_DEBUG_FUNCTION_ENTER

    ret = ak7736_dsp_contreg_read_byte( client, cont, &data);
    if (unlikely(ret)) {
        return ret;
    }

    if (reset_stat) {
        data |= bitmap;
    } else {
        data &= ~(bitmap);
    }

    ret = ak7736_dsp_contreg_write_byte(client, cont,  data);
    if (unlikely(ret)) {
        return ret;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return 0;
}

/**
 * @brief       DSP clock reset.
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[in]   reset_stat If set to true, reset the DSP register.
 * @return      ak7736_dsp_register_bitop return.
 */
static inline int ak7736_dsp_ckresetn_setup(struct i2c_client *client, int reset_stat)
{
    AK7736_DEBUG_FUNCTION_ENTER_ONLY
    return ak7736_dsp_register_bitop(client, AK7736_DSP_REG_CONT01,
                                     AK7736_DSP_BIT_CKRESETN, reset_stat);
}

/**
 * @brief       DSP system reset.
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[in]   reset_stat If set to true, reset the DSP register.
 * @return      ak7736_dsp_register_bitop return.
 */
static inline int ak7736_dsp_sresetn_setup(struct i2c_client *client, int reset_stat)
{
    AK7736_DEBUG_FUNCTION_ENTER_ONLY
    return ak7736_dsp_register_bitop(client, AK7736_DSP_REG_CONT0F,
                                     AK7736_DSP_BIT_SRESETN , reset_stat);
}


/**
 * @brief       Writing DSP program
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[in]   *data DSP program data pointer.
 * @param[in]   size DSP program data size.
 * @param[in]   dest_reg Program type. (CRAM/PRAM)
 * @param[in]   addr Writing DSP address.
 * @param[in]   alignment Program alignment.
 * @retval      0 Normal return.
 * @retval      -EINVAL Parameter error.
 * @retval      -ENOMEM Memory allocation error.
 * @retval      -EIO DSP error.
 */
static int ak7736_dsp_program_write(struct i2c_client *client,
                                    const char        *data,
                                    unsigned int       size,
                                    unsigned char      dest_reg,
                                    unsigned short int addr,
                                    int                alignment)
{
    int          ret = 0;
    const char  *dspram = data;
    char        *buffer;
    unsigned int buffer_size ,write_size;
    unsigned int offset = 0;

    AK7736_DEBUG_FUNCTION_ENTER
    /* input data check */
    if (unlikely(size == 0)) {
        return -EINVAL;
    }
    if (unlikely((size%alignment) != 0)) {
        return -EINVAL;
    }
    if (unlikely(data == NULL)) {
        return -EINVAL;
    }

    /* setup download buffer */
    if ((size+AK7736_DSP_DL_DATA_HEADER_SIZE) > AK7736_DSP_DL_BUFFER_SIZE ) {
        buffer_size = ((AK7736_DSP_DL_BUFFER_SIZE -
                        AK7736_DSP_DL_DATA_HEADER_SIZE) /
                         alignment) * alignment;
    } else {
        buffer_size = size;
    }
    buffer = kmalloc(buffer_size + AK7736_DSP_DL_DATA_HEADER_SIZE, GFP_DMA);
    if (buffer == NULL) {
        AK7736_NOTICE_LOG("kmalloc error\n");
        return -ENOMEM;
    }

    AK7736_NOTICE_LOG("dsp firmware download to %x %d bytes (buffer %d) \n",
                      dest_reg | AK7736_DSP_REG_WRITE_MASK,
                      size, buffer_size);

    /* download loop */
    while (size > offset) {
        unsigned short int calc_crc, read_crc;

        /* calculate write size */
        if ((size-offset) >= buffer_size) {
            write_size = buffer_size;
        } else {
            write_size = size - offset;
        }

        /* setup buffer */
        buffer[0] = (char)(dest_reg | AK7736_DSP_REG_WRITE_MASK);
        buffer[1] = (char)(((addr + offset)/alignment) >> 8);
        buffer[2] = (char)(((addr + offset)/alignment) & 0xff);

        memcpy(&buffer[3], dspram, write_size);

        /* calculate crc */
        calc_crc = crc_itu_t(0x0, buffer,
                             write_size + AK7736_DSP_DL_DATA_HEADER_SIZE);

        /* data write */
        ret = ak7736_dsp_write(client,
                               write_size + AK7736_DSP_DL_DATA_HEADER_SIZE,
                               buffer,
                               AK7736_DSP_I2C_RETRY_COUNT);
        if (unlikely(ret)) {
            goto error_exit;
        }

        /* check crc */
        ret = ak7736_dsp_contreg_read_word(client, AK7736_DSP_REG_CRC, &read_crc);
        if (unlikely(ret)) {
            goto error_exit;
        }

        AK7736_NOTICE_LOG("0x%04x: %5d byte write : calc crc %04x/ read crc %04x\n",
                          addr + offset, write_size,calc_crc,read_crc);
        if ( read_crc != calc_crc) {
            if( dest_reg == AK7736_DSP_REG_CRAM ) {
                AK7736_DRC_HARD_ERR( AK7736_ERRORID_DSP, AK7736_ERROR_PATTERN_CRAM, 0, 0, 0 );
            } else {
                AK7736_DRC_HARD_ERR( AK7736_ERRORID_DSP, AK7736_ERROR_PATTERN_PRAM, 0, 0, 0 );
            }
            AK7736_NOTICE_LOG("crc error detect!\n");
            ret = -EIO;
            goto error_exit;
        }

        /* update address */
        offset += write_size;
        dspram += write_size;
    }

error_exit:
    kfree(buffer);

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}


/**
 * @brief       Reading AK7736 DSP data from Nonvolatile driver.
 * @param[in]   part_offset Nonvolatile data offset.
 * @param[in]   part_size Nonvolatile data size.
 * @param[in]   *magic Magic Data (Check Data)
 * @param[out]  *data AK7736 DSP data.
 * @param[in]   *size AK7736 DSP data size.
 * @retval      0 Normal return.
 * @retval      -ENOENT Data check error.
 */
static int ak7736_dsp_nonvolatile_read(unsigned int  part_offset,
                                       unsigned int  part_size,
                                       const char   *magic,
                                       char         *data,
                                       unsigned int *size)
{
    int ret = 0;
    unsigned short int crc;

    AK7736_DEBUG_FUNCTION_ENTER
    AK7736_NOTICE_LOG("read nonvolatile[%s]\n",magic);

    if (!device_power) return -ESHUTDOWN;

    ret = GetNONVOLATILE(data, part_offset, part_size);
    if (unlikely(ret)) goto error_exit;

    if (unlikely(memcmp(data, magic, AK7736_DSP_NVROM_MAGIC_SIZE))) {
        ret = -ENOENT;
        AK7736_DRC_ERR( AK7736_ERRORID_NONVOLATILE,
                        part_offset, AK7736_ERROR_DETAIL_MAGIC, 0, 0 );
        goto error_exit;
    }

    *size = *(unsigned int *) &data[4];
    if (unlikely(part_size < (*size + AK7736_DSP_NVROM_HEADER_SIZE))) {
        ret = -ENOENT;
        AK7736_DRC_ERR( AK7736_ERRORID_NONVOLATILE,
                        part_offset, AK7736_ERROR_DETAIL_SIZE, 0, 0 );
        goto error_exit;
    }

    crc = crc_itu_t(0x0, &data[AK7736_DSP_NVROM_HEADER_SIZE], *size);
    if (crc != *(unsigned short int *)&data[AK7736_DSP_NVROM_CRC_OFFSET]) {
        ret = -ENOENT;
        AK7736_DRC_ERR( AK7736_ERRORID_NONVOLATILE,
                        part_offset, AK7736_ERROR_DETAIL_CRC, 0, 0 );
        goto error_exit;
    }

error_exit:
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Download the program in AK7736 DSP (PRAM/CRAM)
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[in]   type Program type.
 * @retval      0 Normal return.
 * @retval      -ENOENT Program type error.
 * @retval      -ENOMEM Memory allocation error.
 */
static int ak7736_dsp_program_download( struct i2c_client *client, int type)
{
    unsigned int       part_size;
    unsigned int       part_offset;
    char              *nvdata;
    const char        *magic;

    unsigned int       size;
    unsigned short int addr;
    unsigned char      dest_reg;
    int                alignment;

    int ret = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    switch (type){
    case AK7736_DSP_PROGRAM_TYPE_PRAM:
        /* firmware download (pram) */
        dest_reg    = AK7736_DSP_REG_PRAM;
        addr        = AK7736_DSP_RAM_START_ADDRESS;
        alignment   = AK7736_DSP_PRAM_ALIGNMENT;
        part_offset = AK7736_DSP_NVROM_PRAM_OFFSET;
        part_size   = AK7736_DSP_NVROM_PRAM_SIZE;
        magic       = ak7736_dsp_pram_magic;
        break;

    case AK7736_DSP_PROGRAM_TYPE_CRAM:
        /* firmware download (cram) */
        dest_reg    = AK7736_DSP_REG_CRAM;
        addr        = AK7736_DSP_RAM_START_ADDRESS;
        alignment   = AK7736_DSP_CRAM_ALIGNMENT;
        part_offset = AK7736_DSP_NVROM_CRAM_OFFSET;
        part_size   = AK7736_DSP_NVROM_CRAM_SIZE;
        magic       = ak7736_dsp_cram_magic;
        break;

    default:
        return -ENOENT;
        break;
    }

    nvdata = kzalloc(part_size, GFP_KERNEL);
    if (unlikely(nvdata == NULL)) {
        return -ENOMEM;
    }

    ret = ak7736_dsp_nonvolatile_read(part_offset,
                                      part_size,
                                      magic,
                                      nvdata,
                                      &size);
    if (unlikely(ret)) goto error_exit;

    ret = ak7736_dsp_program_write(client,
                                   &nvdata[AK7736_DSP_NVROM_HEADER_SIZE],
                                   size,
                                   dest_reg,
                                   addr,
                                   alignment);

error_exit:
    kfree(nvdata);
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Get AK7736 DSP Send level and volume information.
 * @param[out]  *priv AK7736 DSP private data pointer.
 * @retval      0 Normal return.
 */
static int ak7736_dsp_get_send_level_vol_info(struct akdsp_private_data *priv)
{
    int count = 0;
    char send_vol[3] = {0};
    unsigned int offset;

    AK7736_DEBUG_FUNCTION_ENTER

    for (count = 0; count < (AK7736_NV_DATA_FES_OUT_ATT_MAX+1); count++) {
        offset = AK7736_NV_OFFSET_FES_OUT_ATT + count;
        memcpy(send_vol,
               priv->cram_parameter_table +
                 AK7736_DSP_NVROM_HEADER_SIZE +
                 offset * AK7736_DSP_CRAM_ALIGNMENT,
               AK7736_DSP_CRAM_ALIGNMENT);
        priv->send_level_vol[count] = (send_vol[0] << 16) | (send_vol[1] << 8) | send_vol[2];
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return 0;
}

/**
 * @brief       Get AK7736 DSP parameter table
 * @param[out]  *priv AK7736 DSP private data pointer.
 * @retval      0 Normal return.
 * @retval      -ENOMEM Memory allocation error.
 */
static int ak7736_dsp_get_cram_parameter_table(struct akdsp_private_data *priv)
{
    char              *nvdata;
    unsigned int       size;
    int ret = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    nvdata = kzalloc(AK7736_DSP_NVROM_PARAM_SIZE, GFP_KERNEL);
    if (unlikely(nvdata == NULL)) {
        return -ENOMEM;
    }

    ret = ak7736_dsp_nonvolatile_read(AK7736_DSP_NVROM_PARAM_OFFSET,
                                      AK7736_DSP_NVROM_PARAM_SIZE,
                                      ak7736_dsp_param_magic,
                                      nvdata,
                                      &size);
    if (unlikely(ret)) goto error_exit;

    priv->cram_parameter_table = nvdata;
    return 0;

error_exit:
    kfree(nvdata);
    AK7736_NOTICE_LOG("cram parameter table load fail.\n");
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Update CRAM data clock on
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[in]   addr Writing DSP address.
 * @param[in]   *data Writing DSP data.
 * @return      ak7736_dsp_write return.
 */
static int ak7736_dsp_cram_update_clkon( struct i2c_client *client,
                                   unsigned short int addr,
                                   const char *data)
{
    int  ret   = 0;
    int  retry = AK7736_DSP_I2C_RETRY_COUNT;
    char wbuffer[AK7736_DSP_DL_DATA_HEADER_SIZE+AK7736_DSP_CRAM_ALIGNMENT];
    char ubuffer[] = {AK7736_DSP_REG_CRAM_UPDATE, 0x00, 0x00};
#ifdef AK7736_DSP_FORCE_CRAM_VERIFY
    char rbuffer[AK7736_DSP_CRAM_ALIGNMENT];
#endif

    AK7736_DEBUG_FUNCTION_ENTER
    AK7736_NOTICE_LOG("cram update[c%04x]=%02x %02x %02x\n",
                      addr, data[0], data[1], data[2] );

    wbuffer[0] = (char)(AK7736_DSP_REG_WRITE_MASK);
    wbuffer[1] = (char)(addr >> 8);
    wbuffer[2] = (char)(addr & 0xff);
    memcpy(&wbuffer[3], data, AK7736_DSP_CRAM_ALIGNMENT);

    while (device_power && retry--) {
        /* write buffer */
        ret = ak7736_dsp_write(client,
                               AK7736_DSP_DL_DATA_HEADER_SIZE +
                                 AK7736_DSP_CRAM_ALIGNMENT,
                               wbuffer,
                               AK7736_DSP_I2C_RETRY_COUNT);
        if (unlikely(ret)) continue;

        /* cram update exec */
        ret = ak7736_dsp_write(client,
                               sizeof(ubuffer),
                               ubuffer,
                               AK7736_DSP_I2C_RETRY_COUNT);
        if (unlikely(ret)) continue;

#ifdef AK7736_DSP_FORCE_CRAM_VERIFY
        /* read data */
        ret = ak7736_dsp_dump (client,
                               AK7736_DSP_REG_CRAM,
                               addr,
                               AK7736_DSP_CRAM_ALIGNMENT,
                               rbuffer,
                               AK7736_DSP_I2C_RETRY_COUNT);
        if (unlikely(ret)) continue;

        if (unlikely(memcmp(data, rbuffer, AK7736_DSP_CRAM_ALIGNMENT))) {
            ret = -EIO;
            continue;
        }
#endif
        break;
    }

    if (!device_power) ret = -ESHUTDOWN;

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}


/**
 * @brief       Update CRAM data clock off
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[in]   addr Writing DSP address.
 * @param[in]   *data Writing DSP data.
 * @retval      0 Normal return.
 * @retval      ak7736_dsp_write return.
 */
static int ak7736_dsp_cram_update_clkoff( struct i2c_client *client,
                                   unsigned short int addr,
                                   const char *data)
{
    int  ret   = 0;
    int  retry = AK7736_DSP_I2C_RETRY_COUNT;
    char wbuffer[AK7736_DSP_DL_DATA_HEADER_SIZE+AK7736_DSP_CRAM_ALIGNMENT];

    AK7736_DEBUG_FUNCTION_ENTER
    AK7736_NOTICE_LOG("cram update clk off[c%04x]=%02x %02x %02x\n",
                      addr, data[0], data[1], data[2] );

    wbuffer[0] = (char)(AK7736_DSP_REG_CRAM | AK7736_DSP_REG_WRITE_MASK);
    wbuffer[1] = (char)(addr >> 8);
    wbuffer[2] = (char)(addr & 0xff);
    memcpy(&wbuffer[3], data, AK7736_DSP_CRAM_ALIGNMENT);

    while (device_power && retry--) {
        /* write buffer */
        ret = ak7736_dsp_write(client,
                               AK7736_DSP_DL_DATA_HEADER_SIZE +
                                 AK7736_DSP_CRAM_ALIGNMENT,
                               wbuffer,
                               AK7736_DSP_I2C_RETRY_COUNT);
        if (unlikely(ret)) continue;

        break;
    }

    if (!device_power) ret = -ESHUTDOWN;

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Set CRAM parameter address from CRAM address
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   cram_addr CRAM address.
 * @param[out]  *data CRAM parameter address.
 * @retval      0 Normal return.
 * @retval      -ENOENT CRAM parameter table not set.
 * @retval      -EINVAL Abnormal CRAM address.
 */
static int ak7736_dsp_cram_parameter(struct akdsp_private_data *priv,
                                     unsigned short cram_addr,
                                     char *data)
{
    unsigned int offset;

    AK7736_DEBUG_FUNCTION_ENTER
    if (unlikely(!priv->cram_parameter_table)) {
        return -ENOENT;
    }

    switch (cram_addr) {
    case AK7736_DSP_CRAM_FES_OUT_ATT_STEP:
        offset = AK7736_NV_OFFSET_FES_OUT_ATT_STEP;
        break;

    case AK7736_DSP_CRAM_FES_OUT_ATT:
        offset = AK7736_NV_OFFSET_FES_OUT_ATT;
        break;

    case AK7736_DSP_CRAM_FES_IN_ATT_STEP:
        offset = AK7736_NV_OFFSET_FES_IN_ATT_STEP;
        break;

    case AK7736_DSP_CRAM_FES_IN_ATT:
        offset = AK7736_NV_OFFSET_FES_IN_ATT;
        break;

    case AK7736_DSP_CRAM_HF_OFF_ATT:
        offset = AK7736_NV_OFFSET_HF_OFF_ATT;
        break;

    case AK7736_DSP_CRAM_MIC_L_COMPENSATION_ATT:
        offset = AK7736_NV_OFFSET_MIC_L_COMPENSATION_ATT;
        break;

    case AK7736_DSP_CRAM_MIC_R_COMPENSATION_ATT:
        offset = AK7736_NV_OFFSET_MIC_R_COMPENSATION_ATT;
        break;

    case AK7736_DSP_CRAM_ATTWAIT:
        offset = AK7736_NV_OFFSET_ATTWAIT;
        break;

    default:
        return -EINVAL;
        break;
    }

    memcpy(data,
           priv->cram_parameter_table +
             AK7736_DSP_NVROM_HEADER_SIZE +
             offset * AK7736_DSP_CRAM_ALIGNMENT,
           AK7736_DSP_CRAM_ALIGNMENT);

    AK7736_DEBUG_FUNCTION_EXIT
    return 0;
}

/**
 * @brief       Set Send volume address from send volume.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   send_level_vol Volume level transmitter.
 * @param[out]  *data CRAM parameter address.
 * @retval      0 Normal return.
 * @retval      -ENOENT CRAM parameter table not set.
 */
static int ak7736_dsp_cram_sendvolindex_table(struct akdsp_private_data *priv,
                                              int send_level_vol,
                                              char *data)
{
    unsigned int offset;

    AK7736_DEBUG_FUNCTION_ENTER
    if (unlikely(!priv->cram_parameter_table)) {
        return -ENOENT;
    }

    if ((send_level_vol < AK7736_NV_DATA_SEND_VOL_INDEX_MIN) ||
        (send_level_vol > AK7736_NV_DATA_SEND_VOL_INDEX_MAX )) {
        return -EINVAL;
    }

    offset = AK7736_NV_OFFSET_SEND_VOL_INDEX +
             AK7736_NV_DATA_SEND_VOL_INDEX_MAX - send_level_vol;

    memcpy(data,
           priv->cram_parameter_table +
             AK7736_DSP_NVROM_HEADER_SIZE +
             offset * AK7736_DSP_CRAM_ALIGNMENT,
           AK7736_DSP_CRAM_ALIGNMENT);

    AK7736_DEBUG_FUNCTION_EXIT
    return 0;
}

/**
 * @brief       Set CRAM parameter address from Received volume
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   recv_vol Received volume.
 * @param[out]  *data CRAM parameter address.
 * @retval      0 Normal return.
 * @retval      -ENOENT CRAM parameter table not set.
 */
static int ak7736_dsp_cram_spkvolindex_table(struct akdsp_private_data *priv,
                                             int recv_vol,
                                             char *data)
{
    unsigned int offset;

    AK7736_DEBUG_FUNCTION_ENTER
    if (unlikely(!priv->cram_parameter_table)) {
        return -ENOENT;
    }

    offset = AK7736_NV_OFFSET_SPK_VOL_INDEX +
             AK7736_NV_DATA_SPK_VOL_INDEX_MAX - recv_vol ;

    memcpy(data,
           priv->cram_parameter_table +
             AK7736_DSP_NVROM_HEADER_SIZE +
             offset * AK7736_DSP_CRAM_ALIGNMENT,
           AK7736_DSP_CRAM_ALIGNMENT);

    AK7736_DEBUG_FUNCTION_EXIT
    return 0;
}

/**
 * @brief       Set CRAM parameter address from Mic level
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   miclevel Microphone level.
 * @retval      CRAM parameter of mic level.
 */
static int ak7736_dsp_cram_miclevel_table(struct akdsp_private_data *priv,
                                          unsigned int miclevel)
{
    int level;
    unsigned char data[3];
    unsigned int offset;
    unsigned int table_level;

    AK7736_DEBUG_FUNCTION_ENTER
    for (level = 0; level < AK7736_NV_DATA_MICLEVEL_MAX ; level++) {
        offset = AK7736_DSP_NVROM_HEADER_SIZE +
                   ( AK7736_NV_OFFSET_MICLEVEL + level ) *
                   AK7736_DSP_CRAM_ALIGNMENT;

        memcpy(data,
               priv->cram_parameter_table + offset,
               AK7736_DSP_CRAM_ALIGNMENT);

        table_level = ( data[0] << 16 ) + ( data[1] << 8 ) + data[2];
        if (miclevel < table_level) {
            break;
        }
    }
    AK7736_DEBUG_FUNCTION_EXIT
    return level;
}

/**
 * @brief       Get Att wait time.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @return      att wait time.
 */
static int ak7736_dsp_get_att_wait(struct akdsp_private_data *priv)
{
    char data[3] = {0};
    int  att_wait;

    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_ATTWAIT,
                              data);

    att_wait = (data[0] << 16) | (data[1] << 8) | data[2];
    AK7736_NOTICE_LOG("att wait time %d\n", att_wait);

    return att_wait;
}

/**
 * @brief       AK7736 DSP fail safe process
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @return      att wait time.
 */
static void ak7736_dsp_start_failsafe(struct akdsp_private_data *priv)
{
    AK7736_DEBUG_FUNCTION_ENTER
    AK7736_NOTICE_LOG("start failsafe\n");
    AK7736_DRC_ERR( AK7736_ERRORID_FAILSAFE, 0, 0, 0, 0 );
    if (!device_power) return;
    down(&priv->stat_lock);
    /* check under apl control */
    if (priv->status == AK7736_DSP_STAT_READY) {
        ak7736_dsp_set_status(priv,
                              AK7736_DSP_STAT_FAIL,
                              AK7736_DSP_ERRSTAT_NONE);
        up(&priv->stat_lock);
        return;
    }

    /* check status */
    if ((priv->status == AK7736_DSP_STAT_INIT) ||
        (priv->status == AK7736_DSP_STAT_ERROR)){
        up(&priv->stat_lock);
        return;
    }

    /* failsafe start */
    ak7736_dsp_set_status(priv,
                          AK7736_DSP_STAT_INIT,
                          AK7736_DSP_ERRSTAT_NONE);
    queue_delayed_work(priv->initwq, &priv->work, 0);
    up(&priv->stat_lock);

    AK7736_DEBUG_FUNCTION_EXIT
}

/**
 * @brief       External mic detection
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @return      None.
 */
static void ak7736_dsp_mic_detect(struct akdsp_private_data *priv)
{
    AK7736_DEBUG_FUNCTION_ENTER
    int est_time = AK7736_EXTMIC_DETECT_EXPIRE_TIME;
    int open_count  = 0;
    int short_count = 0;
    int ret;

    /* init */
    priv->mic_stat.open_detect  = AK7736_DSP_MICSTAT_OK;
    priv->mic_stat.short_detect = AK7736_DSP_MICSTAT_OK;

    /* MIC Power on & detect start */
    priv->pdata->set_ext_mic_power(1);
    priv->pdata->set_ext_mic_detoff(0);
    priv->pdata->set_ext_mic_mode(AK7736_DSP_EXTMIC_HFT);

    ak7736_dsp_msleep(AK7736_EXTMIC_DETECT_DELAY);
    est_time -= AK7736_EXTMIC_DETECT_DELAY;

    while(device_power) {
        est_time -= AK7736_EXTMIC_DETECT_INTERVAL;
        if (est_time < 0) {
            break;
        }
        ret = priv->pdata->get_ext_mic_stat();

        short_count = (ret & 0x01) ? (short_count+1) : 0;
        open_count  = (ret & 0x02) ? (open_count +1) : 0;
        ak7736_dsp_msleep(AK7736_EXTMIC_DETECT_INTERVAL);
    }

    if (short_count >= AK7736_EXTMIC_DETECT_TIMES) {
        priv->mic_stat.short_detect = AK7736_DSP_MICSTAT_NG;
    }

    if (open_count >= AK7736_EXTMIC_DETECT_TIMES) {
        priv->mic_stat.open_detect = AK7736_DSP_MICSTAT_NG;
    }

    if (device_power) {
        priv->mic_detect_done_flag = 1;
    }

    /* MIC detect stop */
    priv->pdata->set_ext_mic_detoff(1);
    AK7736_NOTICE_LOG("mic stat report %d %d\n",
                      priv->mic_stat.open_detect,
                      priv->mic_stat.short_detect);

    AK7736_DEBUG_FUNCTION_EXIT
    return;
}

/**
 * @brief       Set mode of external mic 
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   micmode external mic mode.
 * @return      None.
 */
static int ak7736_dsp_mic_setmode(struct akdsp_private_data *priv, int micmode)
{
    AK7736_DEBUG_FUNCTION_ENTER

    if ( priv->have_external_mic == false ) {
        return 0; // not connect external mic
    }

    if ((micmode != AK7736_DSP_EXTMIC_VOICETAG) &&
        (micmode != AK7736_DSP_EXTMIC_HFT)      &&
        (micmode != AK7736_DSP_EXTMIC_SIRI)) {
        return -EINVAL;
    }

    priv->pdata->set_ext_mic_mode(micmode);
    AK7736_NOTICE_LOG("set mic mode %d.\n",micmode);
    AK7736_DEBUG_FUNCTION_EXIT
    return 0;
}

/**
 * @brief       AK7736 DSP device identify
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @retval      0 Normal return.
 * @retval      -EIO Abnormal return.
 */
static int ak7736_dsp_device_identify(struct akdsp_private_data *priv)
{
    int ret;
    unsigned char value;
    AK7736_DEBUG_FUNCTION_ENTER

    ret = ak7736_dsp_contreg_read_byte(priv->client,
                                       AK7736_DSP_REG_IDENTIFY,
                                       &value);

    if ( (ret != 0) || (value != AK7736_DSP_VALUE_IDENTIFY) ){
        ret = -EIO;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Initialize the AK7736 DSP hardware device
 * @param[in]   *work Work queue.
 * @retval      0 Normal return.
 * @retval      -EIO Abnormal return.
 */
static void ak7736_dsp_init_hw(struct work_struct *work)
{
    struct delayed_work *dwork      = to_delayed_work(work);
    struct akdsp_private_data *priv = container_of(dwork,
                                                   struct akdsp_private_data,
                                                   work);
    struct i2c_client *client       = priv->client;

    int ret = 0;

    AK7736_DEBUG_FUNCTION_ENTER
    AK7736_NOTICE_LOG("dsp init queue start.\n");
    AK7736_DRC_TRC( "dsp init queue start\n" );
    disable_irq(priv->client->irq);
    priv->clko = AK7736_DSP_CLKO_OFF;

    if (( priv->have_external_mic == true ) &&
        (!priv->mic_detect_done_flag )) {
        ak7736_dsp_mic_detect(priv);
    }

    /* init loop */
    while (device_power){
        priv->error_count++;
        if (priv->error_count > priv->retry_failsafe) {
            if (!ret) ret = -EIO;
            break;
        }

        if (priv->cram_parameter_table == NULL) {
            ret = ak7736_dsp_get_cram_parameter_table(priv);
            if (unlikely(ret)) {
                AK7736_DRC_HARD_ERR( AK7736_ERRORID_FAILSAFE_FAILED,
                                     AK7736_ERROR_PATTERN_NV, 0, 0, 0 );
                continue;
            }
        }

        /* send volume initialize */
        ret = ak7736_dsp_get_send_level_vol_info(priv);
        if (unlikely(ret)) {
            AK7736_DRC_HARD_ERR( AK7736_ERRORID_FAILSAFE_FAILED,
                                 AK7736_ERROR_PATTERN_NV, 0, 0, 0 );
            AK7736_ERR_LOG("error. ak7736_dsp_get_send_level_vol_info failed\n");
            continue;
        }
        /* vr volume initialize */
        priv->vr_vol = 0;
        ret = ak7736_dsp_cram_parameter(priv,
                                        AK7736_DSP_CRAM_HF_OFF_ATT,
                                        priv->vr_vol_address);
        if (unlikely(ret)) {
            AK7736_DRC_HARD_ERR( AK7736_ERRORID_FAILSAFE_FAILED,
                                 AK7736_ERROR_PATTERN_NV, 0, 0, 0 );
            AK7736_ERR_LOG("error. vr_volume get failed\n");
            continue;
        }

        ak7736_dsp_reset(client);

        ret = ak7736_dsp_device_identify(priv);

        if (unlikely(ret)) {
            AK7736_NOTICE_LOG("error. device not found.");
            ret = -EIO;
            continue;
        }
        AK7736_INFO_LOG("ak7736_dsp: device found.\n");

        ret = ak7736_dsp_register_setup(client);
        if (unlikely(ret)) {
            AK7736_DRC_HARD_ERR( AK7736_ERRORID_FAILSAFE_FAILED,
                                 AK7736_ERROR_PATTERN_I2C, 0, 0, 0 );
            continue;
        }

        ret = ak7736_dsp_ckresetn_setup(client, AK7736_DSP_RESET_HIGH);
        if (unlikely(ret)) continue;
        msleep(10);

        ret = ak7736_dsp_program_download(client,
                                          AK7736_DSP_PROGRAM_TYPE_PRAM);
        if (unlikely(ret)) {
            AK7736_DRC_HARD_ERR( AK7736_ERRORID_FAILSAFE_FAILED,
                                 AK7736_ERROR_PATTERN_NV, 0, 0, 0 );
            continue;
        }

        ret = ak7736_dsp_program_download(client,
                                          AK7736_DSP_PROGRAM_TYPE_CRAM);
        if (unlikely(ret)) {
            AK7736_DRC_HARD_ERR( AK7736_ERRORID_FAILSAFE_FAILED,
                                 AK7736_ERROR_PATTERN_NV, 0, 0, 0 );
            continue;
        }

#ifdef AK7736_DSP_CIRCUIT_TYPE_ENABLE_OUT3E
        ret = ak7736_dsp_cram_update_clkoff(priv->client,
                                     AK7736_DSP_CRAM_DOUT3L_SEL,
                                     ak7736_dsp_cram_out3e);
        if (unlikely(ret)) continue;
#endif

#ifdef AK7736_DSP_CLKO_DEFAULT_ON
        ret = ak7736_dsp_sresetn_setup(client, AK7736_DSP_RESET_HIGH);
        if (unlikely(ret)) continue;
#endif

        down(&priv->stat_lock);
        enable_irq(priv->client->irq);
        if (!priv->pdata->get_ak_dsp_err()) {
            ret = -EIO;
            disable_irq(priv->client->irq);
            up(&priv->stat_lock);
            continue;
        }
        ak7736_dsp_set_status(priv,
                              AK7736_DSP_STAT_STANDBY,
                              AK7736_DSP_ERRSTAT_NONE);
#ifdef AK7736_DSP_CLKO_DEFAULT_ON
        priv->clko = AK7736_DSP_CLKO_ON;
#endif
        priv->error_count = 0;
        up(&priv->stat_lock);

#if defined(CONFIG_AK7736_DSP_LOG_NOTICE) || defined(CONFIG_AK7736_DSP_LOG_INFO) \
        || defined(CONFIG_AK7736_DSP_LOG_DEBUG)
        {
            unsigned char stat;
            AK7736_NOTICE_LOG("status get %d / code %02x\n",
                              ak7736_dsp_get_status(client, &stat),
                              stat);
#ifdef AK7736_DSP_DEBUG_REGISTER_DUMP
            ak7736_dsp_register_dump(client);
#endif
        }
#endif
        break;
    }

    if (device_power && ret) {
        int err_stat = ak7736_dsp_errno2errorstat(ret);
        down(&priv->stat_lock);
        ak7736_dsp_set_status(priv,
                              AK7736_DSP_STAT_ERROR,
                              err_stat);
        enable_irq(priv->client->irq);
        up(&priv->stat_lock);
        AK7736_ERR_LOG("dsp error.(%d)\n", ret);
        AK7736_DRC_HARD_ERR( AK7736_ERRORID_FAILSAFE_FAILED,
                             AK7736_ERROR_PATTERN_DSP, 0, 0, 0 );
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return;
}

/**
 * @brief       AK7736 DSP interrupt control
 * @param[in]   irq IRQ number.
 * @param[in]   *dev_id Device id.
 * @return      IRQ_HANDLED return.
 */
static irqreturn_t ak7736_dsp_interrupt(int irq, void *dev_id)
{
    struct akdsp_private_data *priv = dev_id;

    AK7736_DEBUG_FUNCTION_ENTER

    /* check device off */
    if (!device_power) {
        return IRQ_HANDLED;
    }
    AK7736_NOTICE_LOG("interrupt det\n");

    if (!priv->pdata->get_ak_dsp_err()) {
        AK7736_DRC_ERR( AK7736_ERRORID_FAILSAFE_START, 0, 0, 0, 0 );
        ak7736_dsp_start_failsafe(priv);
    }
    AK7736_DEBUG_FUNCTION_EXIT
    return IRQ_HANDLED;
}

/**
 * @brief       Get state of AK7736 DSP driver
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   *dev_id Device id.
 * @retval      0 Normal return.
 * @retval      -EFAULT Abnormal return.
 
*/
static int ak7736_dsp_ioctl_get_status(struct akdsp_private_data *priv,
                                      unsigned long arg)
{
    int ret = 0;
    struct ak7736_status out_stat;

    AK7736_DEBUG_FUNCTION_ENTER

    down(&priv->stat_lock);
    out_stat.status     = priv->status;
    out_stat.error_stat = priv->error_stat;
    priv->stat_unread  = 0;
    up(&priv->stat_lock);
    if (out_stat.status == AK7736_DSP_STAT_INIT_FAIL) {
        out_stat.status = AK7736_DSP_STAT_ERROR;
    }

    if (copy_to_user((void __user *)arg,
                     &out_stat,
                     sizeof(struct ak7736_status))) {
        ret = -EFAULT;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Set clock of AK7736 DSP
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   clko Clock type
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal clock type.
 */
static int ak7736_dsp_set_clko(struct akdsp_private_data *priv,
                               unsigned long clko)
{
    int ret = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    if (( clko       == AK7736_DSP_CLKO_ON ) &&
        ( priv->clko == AK7736_DSP_CLKO_OFF)){
        ret = ak7736_dsp_register_bitop(priv->client,
                                        AK7736_DSP_REG_CONT0A,
                                        AK7736_DSP_BIT_CLKO,
                                        AK7736_DSP_CLKO_ON);
        if (unlikely(ret)) goto fail_exit;

        ret = ak7736_dsp_sresetn_setup(priv->client, AK7736_DSP_RESET_HIGH);
        if (unlikely(ret)) goto fail_exit;

        msleep(1);

        priv->pdata->set_ak_adc_pdn(AK7736_DSP_RESET_HIGH);
        priv->clko = AK7736_DSP_CLKO_ON;
        AK7736_NOTICE_LOG("CLKO on\n");
    } else if (( clko       == AK7736_DSP_CLKO_OFF) &&
               ( priv->clko == AK7736_DSP_CLKO_ON )){
        priv->pdata->set_ak_adc_pdn(AK7736_DSP_RESET_LOW);

        msleep(1);

        ret = ak7736_dsp_sresetn_setup(priv->client, AK7736_DSP_RESET_LOW);
        if (unlikely(ret)) goto fail_exit;

        ret = ak7736_dsp_register_bitop(priv->client,
                                        AK7736_DSP_REG_CONT0A,
                                        AK7736_DSP_BIT_CLKO,
                                        AK7736_DSP_CLKO_OFF);
        if (unlikely(ret)) goto fail_exit;
        priv->clko = AK7736_DSP_CLKO_OFF;
        AK7736_NOTICE_LOG("CLKO off\n");
    } else {
        ret = -EINVAL;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;

fail_exit:
    ak7736_dsp_start_failsafe(priv);
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       AK7736 DSP configuration control authority
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   control Control authority type.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal Control authority type.
 */
static int ak7736_dsp_ioctl_set_control(struct akdsp_private_data *priv,
                                        unsigned long control)
{
    int ret = 0;

    AK7736_DEBUG_FUNCTION_ENTER

        AK7736_NOTICE_LOG("SET CONTROL %ld\n",control);

    down(&priv->stat_lock);
    if (( control == AK7736_DSP_CONTROL_ON ) &&
        (priv->status == AK7736_DSP_STAT_STANDBY)) {
        ak7736_dsp_set_status(priv,
                              AK7736_DSP_STAT_READY,
                              AK7736_DSP_ERRSTAT_NONE);
        up(&priv->stat_lock);
#ifndef AK7736_DSP_CLKO_DEFAULT_ON
        ak7736_dsp_set_clko(priv, AK7736_DSP_CLKO_ON);
#endif
    } else if ( control == AK7736_DSP_CONTROL_OFF ) {
        if (priv->status == AK7736_DSP_STAT_READY) {
            ak7736_dsp_set_status(priv,
                                  AK7736_DSP_STAT_STANDBY,
                                  AK7736_DSP_ERRSTAT_NONE);
            up(&priv->stat_lock);
#ifndef AK7736_DSP_CLKO_DEFAULT_ON
            ak7736_dsp_set_clko(priv, AK7736_DSP_CLKO_OFF);
#endif
        } else if (priv->status == AK7736_DSP_STAT_FAIL){
            ak7736_dsp_set_status(priv,
                                  AK7736_DSP_STAT_INIT,
                                  AK7736_DSP_ERRSTAT_NONE);
            queue_delayed_work(priv->initwq, &priv->work, 0);
            up(&priv->stat_lock);
        } else {
            ret = -EINVAL;
            up(&priv->stat_lock);
        }
    } else {
        ret = -EINVAL;
        up(&priv->stat_lock);
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       AK7736 DSP reset control
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal status.
 */
static int ak7736_dsp_ioctl_restart(struct akdsp_private_data *priv)
{
    int ret = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    down(&priv->stat_lock);
    if (priv->status != AK7736_DSP_STAT_FAIL) {
        up(&priv->stat_lock);
        return -EINVAL;
    }

    ak7736_dsp_set_status(priv,
                          AK7736_DSP_STAT_INIT,
                          AK7736_DSP_ERRSTAT_NONE);
    queue_delayed_work(priv->initwq, &priv->work, 0);
    up(&priv->stat_lock);

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       AK7736 DSP Frequency setting mode
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   fsmode Frequency setting mode.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal status.
 */
static int ak7736_dsp_ioctl_set_fsmode(struct akdsp_private_data *priv,
                                       unsigned long fsmode)
{
    int   ret = 0;
    int   lr1down  = AK7736_DSP_LR1DOWN_FS8;
    const char *band_sel = ak7736_dsp_cram_off;
    const char *hf_onoff = ak7736_dsp_cram_off;

    AK7736_DEBUG_FUNCTION_ENTER

    /* check status & clko */
    AK7736_NOTICE_LOG("SET FSMODE %ld\n",fsmode);

    if ((priv->status != AK7736_DSP_STAT_READY) ||
        (priv->clko   == AK7736_DSP_CLKO_OFF  )) {
        return -EINVAL;
    }

    /* check input parameter */
    if (( fsmode != AK7736_DSP_FSMODE_HF_NB ) &&
        ( fsmode != AK7736_DSP_FSMODE_HF_WB ) &&
        ( fsmode != AK7736_DSP_FSMODE_VR_NB ) &&
        ( fsmode != AK7736_DSP_FSMODE_VR_WB )) {
        return -EINVAL;
    }

    /* check wide band(16khz) */
    if (( fsmode == AK7736_DSP_FSMODE_HF_WB ) ||
        ( fsmode == AK7736_DSP_FSMODE_VR_WB ) ) {
        lr1down  = AK7736_DSP_LR1DOWN_FS16;
        band_sel = ak7736_dsp_cram_on;
    }

#ifndef AK7736_DSP_CIRCUIT_TYPE_ENABLE_OUT3E
    /* check vr */
    if (( fsmode == AK7736_DSP_FSMODE_VR_NB ) ||
        ( fsmode == AK7736_DSP_FSMODE_VR_WB ) ) {
        hf_onoff = ak7736_dsp_cram_on;
    }
#endif

    /* system reset low(assert) */
    ret = ak7736_dsp_sresetn_setup(priv->client, AK7736_DSP_RESET_LOW);
    if (unlikely(ret)) goto fail_exit;

    /* LR1DOWN register setup */
    ret = ak7736_dsp_register_bitop(priv->client, AK7736_DSP_REG_CONT01,
                                    AK7736_DSP_BIT_LR1DOWN,
                                    lr1down);
    if (unlikely(ret)) goto fail_exit;

    /* CRAM:BAND_SEL setup */
    ret = ak7736_dsp_cram_update_clkoff(priv->client,
                                        AK7736_DSP_CRAM_BAND_SEL,
                                        band_sel);
    if (unlikely(ret)) goto fail_exit;

    /* CRAM:HF_ON/OFF setup */
    /* When ENABLE_OUT3E is set, this value always sets up zero */
    ret = ak7736_dsp_cram_update_clkoff(priv->client,
                                        AK7736_DSP_CRAM_HF_ON_OFF,
                                        hf_onoff);
    if (unlikely(ret)) goto fail_exit;

    /* system reset high */
    ret = ak7736_dsp_sresetn_setup(priv->client, AK7736_DSP_RESET_HIGH);
    if (unlikely(ret)) goto fail_exit;

    AK7736_DEBUG_FUNCTION_EXIT
    return 0;


fail_exit:
    ak7736_dsp_start_failsafe(priv);
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       AK7736 DSP clock control
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   clko Clock type
 * @return      ak7736_dsp_set_clko return.
 */
static int ak7736_dsp_ioctl_set_clko(struct akdsp_private_data *priv,
                                     unsigned long clko)
{
    return ak7736_dsp_set_clko(priv, clko);
}


/**
 * @brief       Check volume paramer of AK7736 DSP
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   *vol Volume parameter.
 * @param[in]   *recv_vol Earpiece volume.
 * @param[in]   *send_vol Send volume.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal volume and state.
 */
static int ak7736_dsp_check_volume_parameter(struct akdsp_private_data *priv,
                                           struct ak7736_dspvol *vol,
                                           char *recv_vol,
                                           char *send_vol)
{
    int ret;
    AK7736_DEBUG_FUNCTION_ENTER

    if ( (vol->send_vol == AK7736_DSP_VOL_SENDMUTE_ON ) ||
         (vol->send_vol == AK7736_DSP_VOL_SENDMUTE_OFF )){
        memcpy(send_vol, ak7736_dsp_cram_off, sizeof(ak7736_dsp_cram_off));
        if (vol->send_vol == AK7736_DSP_VOL_SENDMUTE_OFF) {
            ak7736_dsp_cram_sendvolindex_table(priv,
                                              priv->send_level_vol[priv->send_level],
                                              send_vol);
        }
    } else if (vol->send_vol != AK7736_DSP_VOL_NOTSET ) {
        return -EINVAL;
    }

    if (( vol->recv_vol <= AK7736_NV_DATA_SPK_VOL_INDEX_MAX ) &&
        ( vol->recv_vol >= AK7736_NV_DATA_SPK_VOL_INDEX_MIN )) {
        ret = ak7736_dsp_cram_spkvolindex_table(priv,
                                                vol->recv_vol,
                                                recv_vol);
        if (unlikely(ret)) return -EINVAL;
    } else if (vol->recv_vol != AK7736_DSP_VOL_NOTSET ) {
        return -EINVAL;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return 0;
}

/**
 * @brief       Bluetooth hands-free start (internal)
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   arg Transmitter-receiver volume information pointer address.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal volume and state.
 * @retval      -EFAULT Abnormal return.
 */
static int ak7736_dsp_start_hf_exec(struct akdsp_private_data *priv,
                                    unsigned long arg)
{
    int ret;
    char data[3], recv_vol[3], send_vol[3];
    struct ak7736_dspvol vol;

    AK7736_DEBUG_FUNCTION_ENTER
    AK7736_NOTICE_LOG("START HF\n");

    if (priv->status != AK7736_DSP_STAT_READY) {
        return -EINVAL;
    }

    if (copy_from_user(&vol, (void __user *)arg, sizeof(vol)))
    {
        return -EFAULT;
    }

    ret = ak7736_dsp_check_volume_parameter(priv, &vol, recv_vol, send_vol);
    if (unlikely(ret)) return ret;

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_HF_OFF_ATT,
                                 ak7736_dsp_cram_off);
    if (unlikely(ret)) goto fail_exit;

    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_MIC_L_COMPENSATION_ATT,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_MIC_L_COMPENSATION_ATT,
                                 data);
    if (unlikely(ret)) goto fail_exit;

    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_MIC_R_COMPENSATION_ATT,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_MIC_R_COMPENSATION_ATT,
                                 data);
    if (unlikely(ret)) goto fail_exit;


    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_FES_OUT_ATT_STEP,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FES_OUT_ATT_STEP,
                                 data);
    if (unlikely(ret)) goto fail_exit;

    if (vol.send_vol != AK7736_DSP_VOL_NOTSET ) {
        ret = ak7736_dsp_cram_update_clkon(priv->client,
                                     AK7736_DSP_CRAM_FES_OUT_ATT,
                                     send_vol);
        if (unlikely(ret)) goto fail_exit;
    }

    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_FES_IN_ATT_STEP,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FES_IN_ATT_STEP,
                                 data);
    if (unlikely(ret)) goto fail_exit;

    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_FES_IN_ATT,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FES_IN_ATT,
                                 data);
    if (unlikely(ret)) goto fail_exit;


    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FORCE_INIT,
                                 ak7736_dsp_cram_on);
    if (unlikely(ret)) goto fail_exit;

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FES_OUT_ATT_STEP,
                                 ak7736_dsp_cram_on);
    if (unlikely(ret)) goto fail_exit;

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FES_IN_ATT_STEP,
                                 ak7736_dsp_cram_in_att_default);
    if (unlikely(ret)) goto fail_exit;


    if (vol.recv_vol != AK7736_DSP_VOL_NOTSET ) {
        ret = ak7736_dsp_cram_update_clkon(priv->client,
                                     AK7736_DSP_CRAM_SPK_VOL_INDEX,
                                     recv_vol);
        if (unlikely(ret)) goto fail_exit;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return 0;

fail_exit:
    ak7736_dsp_start_failsafe(priv);
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Bluetooth hands-free start
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   arg Transmitter-receiver volume information.
 * @return      ak7736_dsp_start_hf_exec return.
 */
static int ak7736_dsp_ioctl_start_hf(struct akdsp_private_data *priv,
                                     unsigned long arg)
{
    int ret;

    AK7736_DEBUG_FUNCTION_ENTER
    ret = ak7736_dsp_start_hf_exec(priv ,arg);
    ak7736_dsp_mic_setmode(priv, AK7736_DSP_EXTMIC_HFT);

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Speech Interpretation and Recognition Interface start
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   arg Transmitter-receiver volume information.
 * @return      ak7736_dsp_start_hf_exec return.
 */
static int ak7736_dsp_ioctl_start_siri(struct akdsp_private_data *priv)
{
    int ret;
    char data[3];
    int sleep_time;

    AK7736_DEBUG_FUNCTION_ENTER
    AK7736_NOTICE_LOG("START SIRI\n");

    if (priv->status != AK7736_DSP_STAT_READY) {
        return -EINVAL;
    }

    ak7736_dsp_mic_setmode(priv, AK7736_DSP_EXTMIC_SIRI);

    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_FES_OUT_ATT_STEP,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FES_OUT_ATT_STEP,
                                 data);
    if (unlikely(ret)) goto fail_exit;

    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_FES_OUT_ATT,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FES_OUT_ATT,
                                 data);
    if (unlikely(ret)) goto fail_exit;

    sleep_time = ak7736_dsp_get_att_wait(priv);
    if (sleep_time) {
        ak7736_dsp_msleep(sleep_time);
    }

    /* Receiver volume mute release */
    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_FES_IN_ATT,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FES_IN_ATT,
                                 data);
    if (unlikely(ret)) goto fail_exit;

    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_MIC_L_COMPENSATION_ATT,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_MIC_L_COMPENSATION_ATT,
                                 data);
    if (unlikely(ret)) goto fail_exit;

    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_MIC_R_COMPENSATION_ATT,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_MIC_R_COMPENSATION_ATT,
                                 data);
    if (unlikely(ret)) goto fail_exit;

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_HF_OFF_ATT,
                                 priv->vr_vol_address);
    if (unlikely(ret)) goto fail_exit;

    AK7736_DEBUG_FUNCTION_EXIT
    return 0;

fail_exit:
    ak7736_dsp_start_failsafe(priv);
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Change volume in bluetooth hands-free execution
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   arg Transmitter-receiver volume information.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal state.
 * @retval      -EFAULT Abnormal return.
 */
static int ak7736_dsp_ioctl_set_hfvol(struct akdsp_private_data *priv,
                                      unsigned long arg)
{
    int ret = 0;
    char data[3];
    char recv_vol[3], send_vol[3];
    struct ak7736_dspvol vol;
    int sleep_time;

    AK7736_DEBUG_FUNCTION_ENTER

    if (priv->status != AK7736_DSP_STAT_READY) {
        return -EINVAL;
    }

    if (copy_from_user(&vol, (void __user *)arg, sizeof(vol)))
    {
        return -EFAULT;
    }

    ret = ak7736_dsp_check_volume_parameter(priv, &vol, recv_vol, send_vol);
    if (unlikely(ret)) return ret;

    if (vol.send_vol != AK7736_DSP_VOL_NOTSET ) {
        ak7736_dsp_cram_parameter(priv,
                                  AK7736_DSP_CRAM_FES_OUT_ATT_STEP,
                                  data);

        ret = ak7736_dsp_cram_update_clkon(priv->client,
                                     AK7736_DSP_CRAM_FES_OUT_ATT_STEP,
                                     data);
        if (unlikely(ret)) goto fail_exit;

        ret = ak7736_dsp_cram_update_clkon(priv->client,
                                     AK7736_DSP_CRAM_FES_OUT_ATT,
                                     send_vol);
        if (unlikely(ret)) goto fail_exit;

        sleep_time = ak7736_dsp_get_att_wait(priv);
        if (sleep_time) {
            ak7736_dsp_msleep(sleep_time);
        }

        ret = ak7736_dsp_cram_update_clkon(priv->client,
                                     AK7736_DSP_CRAM_FES_OUT_ATT_STEP,
                                     ak7736_dsp_cram_on);
        if (unlikely(ret)) goto fail_exit;
    }

    if (vol.recv_vol != AK7736_DSP_VOL_NOTSET ) {
        ret = ak7736_dsp_cram_update_clkon(priv->client,
                                     AK7736_DSP_CRAM_SPK_VOL_INDEX,
                                     recv_vol);
        if (unlikely(ret)) goto fail_exit;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;

fail_exit:
    AK7736_ERR_LOG("ak7736_dsp_ioctl_set_hfvol error:%d\n", ret);
    ak7736_dsp_start_failsafe(priv);
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Bluetooth hands-free stop
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @return      ak7736_dsp_cram_update_clkon return.
 */
static int ak7736_dsp_ioctl_stop_hf(struct akdsp_private_data *priv)
{
    int ret;
    char data[3];
    int sleep_time;

    AK7736_DEBUG_FUNCTION_ENTER

    AK7736_NOTICE_LOG("STOP HF\n");
    if (priv->status != AK7736_DSP_STAT_READY) {
        return -EINVAL;
    }

    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_FES_OUT_ATT_STEP,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FES_OUT_ATT_STEP,
                                 data);
    if (unlikely(ret)) goto fail_exit;

    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_FES_IN_ATT_STEP,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FES_IN_ATT_STEP,
                                 data);
    if (unlikely(ret)) goto fail_exit;

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FES_OUT_ATT,
                                 ak7736_dsp_cram_off);
    if (unlikely(ret)) goto fail_exit;

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FES_IN_ATT,
                                 ak7736_dsp_cram_in_att_default);
    if (unlikely(ret)) goto fail_exit;

    sleep_time = ak7736_dsp_get_att_wait(priv);
    if (sleep_time) {
        ak7736_dsp_msleep(sleep_time);
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;

fail_exit:
    ak7736_dsp_start_failsafe(priv);
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}


/**
 * @brief       Voice recognition and apps cooperation start
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal volume and state.
 */
static int ak7736_dsp_ioctl_start_vr(struct akdsp_private_data *priv)
{
    int ret;
    char data[3];
    int sleep_time;

    AK7736_DEBUG_FUNCTION_ENTER
    AK7736_NOTICE_LOG("START VR\n");

    if (priv->status != AK7736_DSP_STAT_READY) {
        return -EINVAL;
    }

    ak7736_dsp_mic_setmode(priv, AK7736_DSP_EXTMIC_VOICETAG);

    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_FES_OUT_ATT_STEP,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FES_OUT_ATT_STEP,
                                 data);
    if (unlikely(ret)) goto fail_exit;

    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_FES_OUT_ATT,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FES_OUT_ATT,
                                 data);
    if (unlikely(ret)) goto fail_exit;

    sleep_time = ak7736_dsp_get_att_wait(priv);
    if (sleep_time) {
        ak7736_dsp_msleep(sleep_time);
    }

    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_MIC_L_COMPENSATION_ATT,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_MIC_L_COMPENSATION_ATT,
                                 data);
    if (unlikely(ret)) goto fail_exit;

    ak7736_dsp_cram_parameter(priv,
                              AK7736_DSP_CRAM_MIC_R_COMPENSATION_ATT,
                              data);

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_MIC_R_COMPENSATION_ATT,
                                 data);
    if (unlikely(ret)) goto fail_exit;

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_HF_OFF_ATT,
                                 priv->vr_vol_address);
    if (unlikely(ret)) goto fail_exit;

    AK7736_DEBUG_FUNCTION_EXIT
    return 0;

fail_exit:
    ak7736_dsp_start_failsafe(priv);
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Voice recognition and apps cooperation stop
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal state.
 */
static int ak7736_dsp_ioctl_stop_vr(struct akdsp_private_data *priv)
{
    int ret;

    AK7736_DEBUG_FUNCTION_ENTER
    AK7736_NOTICE_LOG("STOP VR\n");

    if (priv->status != AK7736_DSP_STAT_READY) {
        return -EINVAL;
    }

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_HF_OFF_ATT,
                                 ak7736_dsp_cram_off);

    if (unlikely(ret)) {
        ak7736_dsp_start_failsafe(priv);
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Speech Interpretation and Recognition Interface stop
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal state.
 */
static int ak7736_dsp_ioctl_stop_siri(struct akdsp_private_data *priv)
{
    int ret;

    AK7736_DEBUG_FUNCTION_ENTER
    AK7736_NOTICE_LOG("STOP SIRI\n");

    if (priv->status != AK7736_DSP_STAT_READY) {
        return -EINVAL;
    }
    /** Receiver volume setting mute */
    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_FES_IN_ATT,
                                 ak7736_dsp_cram_off);
    if (unlikely(ret)) goto fail_exit;

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_HF_OFF_ATT,
                                 ak7736_dsp_cram_off);
    if (unlikely(ret)) goto fail_exit;

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;

fail_exit:
    ak7736_dsp_start_failsafe(priv);
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Setting ECNC engine
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   mode ECNC engine mode.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal mode and state.
 */
static int ak7736_dsp_ioctl_set_ecncmode(struct akdsp_private_data *priv,
                                         unsigned long mode)
{
    int   ret = 0;
    const char *data = ak7736_dsp_cram_off;

    AK7736_DEBUG_FUNCTION_ENTER

    if (priv->status != AK7736_DSP_STAT_READY) {
        return -EINVAL;
    }

    if ((mode != AK7736_DSP_ECNC_ON ) &&
        (mode != AK7736_DSP_ECNC_OFF)){
        return -EINVAL;
    }

    if (mode == AK7736_DSP_ECNC_OFF) {
        data = ak7736_dsp_cram_on;
    }

    ret = ak7736_dsp_cram_update_clkon(priv->client,
                                 AK7736_DSP_CRAM_HF_ON_OFF,
                                 data);
    if (unlikely(ret)) goto fail_exit;

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;

fail_exit:
    ak7736_dsp_start_failsafe(priv);
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       AK7736 DSP device alive monitoring
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @return      ak7736_dsp_device_identify return.
 */
static int ak7736_dsp_ioctl_check_device(struct akdsp_private_data *priv)
{
    int ret;
    AK7736_DEBUG_FUNCTION_ENTER

    ret = ak7736_dsp_device_identify(priv);

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}


/**
 * @brief       Change in DIAG mode of AK7736 DSP driver.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   onoff Presence or absence of state DIAG.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal state.
 */
static int ak7736_dsp_ioctl_set_diagmode(struct akdsp_private_data *priv,
                                         unsigned long onoff)
{
    int ret = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    down(&priv->stat_lock);
    if (( onoff == AK7736_DSP_DIAG_ON ) &&
        (priv->status == AK7736_DSP_STAT_STANDBY)) {
        ak7736_dsp_set_status(priv,
                              AK7736_DSP_STAT_DIAG,
                              AK7736_DSP_ERRSTAT_NONE);
        up(&priv->stat_lock);

#ifndef AK7736_DSP_CLKO_DEFAULT_ON
        ak7736_dsp_set_clko(priv, AK7736_DSP_CLKO_ON);
#endif
        ret = ak7736_dsp_cram_update_clkon(priv->client,
                                     AK7736_DSP_CRAM_TEST_MODE,
                                     ak7736_dsp_cram_on);
        if (unlikely(ret)) goto fail_exit;

        msleep(1);

    } else if (( onoff == AK7736_DSP_DIAG_OFF) &&
        (priv->status == AK7736_DSP_STAT_DIAG)) {
        ak7736_dsp_set_status(priv,
                              AK7736_DSP_STAT_STANDBY,
                              AK7736_DSP_ERRSTAT_NONE);
        up(&priv->stat_lock);

        ret = ak7736_dsp_cram_update_clkon(priv->client,
                                     AK7736_DSP_CRAM_TEST_MODE,
                                     ak7736_dsp_cram_off);
        if (unlikely(ret)) goto fail_exit;
#ifndef AK7736_DSP_CLKO_DEFAULT_ON
            ak7736_dsp_set_clko(priv, AK7736_DSP_CLKO_OFF);
#endif
    } else {
        up(&priv->stat_lock);
        ret = -EINVAL;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;

fail_exit:
    ak7736_dsp_start_failsafe(priv);
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Get the microphone level from AK7736 DSP (internal)
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[out]  miclevel_l Microphone level on the left.
 * @param[out]  miclevel_r Microphone level on the right.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal state.
 */
static int ak7736_dsp_get_miclevel(struct akdsp_private_data *priv,
                                   int *miclevel_l,
                                   int *miclevel_r)
{
    int ret;
    unsigned int mir2 = 0x00;
    unsigned int mir3 = 0x00;
    int cnt;
    int retry = (AK7736_DSP_DIAG_MIC_LEVEL_TIMEOUT/AK7736_DSP_DIAG_MIC_LEVEL_SAMPLING_TIME);

    AK7736_DEBUG_FUNCTION_ENTER

    if (priv->status != AK7736_DSP_STAT_DIAG) {
        return -EINVAL;
    }

    for ( cnt = 0; cnt < retry; cnt++ ) {
        ret = ak7736_dsp_cram_update_clkon(priv->client,
                                     AK7736_DSP_CRAM_PEAKHOLD_MICR2,
                                     ak7736_dsp_cram_off);
        if (unlikely(ret)) goto fail_exit;

        ret = ak7736_dsp_cram_update_clkon(priv->client,
                                     AK7736_DSP_CRAM_PEAKHOLD_MICR3,
                                     ak7736_dsp_cram_off);
        if (unlikely(ret)) goto fail_exit;

        ak7736_dsp_msleep(AK7736_DSP_DIAG_MIC_LEVEL_SAMPLING_TIME);

        ret = ak7736_dsp_read(priv->client,
                              AK7736_DSP_REG_MIR2,
                              sizeof(unsigned int),
                              (char *)&mir2,
                              AK7736_DSP_I2C_RETRY_COUNT);

        ret = ak7736_dsp_read(priv->client,
                              AK7736_DSP_REG_MIR3,
                              sizeof(unsigned int),
                              (char *)&mir3,
                              AK7736_DSP_I2C_RETRY_COUNT);

        mir2 = swab32(mir2);
        mir3 = swab32(mir3);
        if (mir2 & AK7736_DSP_MIR_VALIDITY_MASK) {
            continue;
        }
        if (mir3 & AK7736_DSP_MIR_VALIDITY_MASK) {
            continue;
        }

        break;
    }
    if (cnt == retry) {
        return -EIO;
    }

    mir2 = mir2 >> 8;
    mir3 = mir3 >> 8;
    *miclevel_l = ak7736_dsp_cram_miclevel_table(priv, mir2);
    *miclevel_r = ak7736_dsp_cram_miclevel_table(priv, mir3);

    AK7736_DEBUG_FUNCTION_EXIT
    return 0;

fail_exit:
    ak7736_dsp_start_failsafe(priv);
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Set the External microphone mode.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   arg External microphone mode.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal state.
 */
static int ak7736_dsp_ioctl_set_micmode(struct akdsp_private_data *priv,
                                        unsigned long arg)
{
    AK7736_DEBUG_FUNCTION_ENTER

    if((arg != AK7736_DSP_EXTMIC_HFT) &&
       (arg != AK7736_DSP_EXTMIC_SIRI) &&
       (arg != AK7736_DSP_EXTMIC_VOICETAG) &&
       (arg != AK7736_DSP_EXTMIC_UNKNOWN)){
        return -EINVAL;
    }

    priv->pdata->set_ext_mic_mode(arg);
    AK7736_DEBUG_FUNCTION_EXIT;
    return 0;
}

/**
 * @brief       Get the External microphone mode.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[out]  arg External microphone mode.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal state.
 */
static int ak7736_dsp_ioctl_get_micmode(struct akdsp_private_data *priv,
                                        unsigned long arg)
{
    int micmode = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    micmode = priv->pdata->get_ext_mic_mode();
    if (put_user(micmode, (int __user *)arg) ) {
        return -EFAULT;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return 0;
}

/**
 * @brief       Get the microphone level from AK7736 DSP.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[out]  arg Microphone level on the left data pointer.
 * @retval     0 Normal return.
 * @retval      -EINVAL Abnormal state.
 * @retval      -EFAULT Abnormal return.
 */
static int ak7736_dsp_ioctl_get_miclevel(struct akdsp_private_data *priv,
                                         unsigned long arg)
{
    int ret;
    int miclevel_l, miclevel_r;

    AK7736_DEBUG_FUNCTION_ENTER

    if (priv->status != AK7736_DSP_STAT_DIAG) {
        return -EINVAL;
    }

    ret = ak7736_dsp_get_miclevel(priv, &miclevel_l, &miclevel_r);
    if (ret) return ret;

    if (put_user(miclevel_l, (int __user *)arg) ) {
        ret = -EFAULT;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Get the microphone level (Stereo) from AK7736 DSP.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[out]  arg Microphone level data pointer.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal state.
 * @retval      -EFAULT Abnormal return.
 */
static int ak7736_dsp_ioctl_get_miclevel_stereo(struct akdsp_private_data *priv,
                                                unsigned long arg)
{
    int ret;
    struct ak7736_miclevel miclevel;

    AK7736_DEBUG_FUNCTION_ENTER

    if (priv->status != AK7736_DSP_STAT_DIAG) {
        return -EINVAL;
    }

    ret = ak7736_dsp_get_miclevel(priv,
                                  &miclevel.lch_level,
                                  &miclevel.rch_level);
    if (ret) return ret;
    AK7736_NOTICE_LOG("MICLEVEL LCH  = %d\n", miclevel.lch_level);
    AK7736_NOTICE_LOG("MICLEVEL RCH  = %d\n", miclevel.rch_level);

    if (priv->have_external_mic == true ) {
        miclevel.rch_level = 0;
        miclevel.mic_type = AK7736_DSP_MIC_TYPE_EXTERNAL;
    }else{
        miclevel.mic_type = AK7736_DSP_MIC_TYPE_INTERNAL;
    }

    if (copy_to_user((void __user *)arg,
                     &miclevel,
                     sizeof(struct ak7736_miclevel))) {
        ret = -EFAULT;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}


/**
 * @brief       Set volume level transmitter
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   arg Transmitter level data pointer.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal state and transmitter level.
 * @retval      -EFAULT Abnormal return.
 */
static int ak7736_dsp_ioctl_set_sendlevel(struct akdsp_private_data *priv,
                                          unsigned long arg)
{
    int  ret;
    unsigned char write_data;

    AK7736_DEBUG_FUNCTION_ENTER

    if (priv->status != AK7736_DSP_STAT_DIAG) {
        return -EINVAL;
    }

    if (( arg < AK7736_NV_DATA_FES_OUT_ATT_MIN ) ||
        ( arg > AK7736_NV_DATA_FES_OUT_ATT_MAX )) {
        return -EINVAL;
    }

    write_data = (unsigned char)arg;

    ret = SetNONVOLATILE(&write_data, AK7736_DSP_NVROM_DATA_OFFSET_SENDLEVEL, 1);
    if (ret == 0) {
        priv->send_level = arg;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Get volume level transmitter
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   arg Transmitter level data pointer.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal state and transmitter level.
 * @retval      -EFAULT Abnormal return.
 */
static int ak7736_dsp_ioctl_get_sendlevel(struct akdsp_private_data *priv,
                                          unsigned long arg)
{
    int ret = 0;
    AK7736_DEBUG_FUNCTION_ENTER

    if (priv->status != AK7736_DSP_STAT_DIAG) {
        return -EINVAL;
    }

    if (put_user(priv->send_level, (int __user *)arg) ) {
        ret = -EFAULT;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Get microphone level
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[out]  arg Microphone level data pointer.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal state.
 * @retval      -EFAULT Abnormal return.
 */
static int ak7736_dsp_ioctl_get_mic_status(struct akdsp_private_data *priv,
                                           unsigned long arg)
{
    int ret = 0;
    struct ak7736_micstatus mic_stat;

    AK7736_DEBUG_FUNCTION_ENTER

    if (priv->status != AK7736_DSP_STAT_DIAG) {
        return -EINVAL;
    }

    mic_stat.mic_type     = priv->have_external_mic;
    mic_stat.open_detect  = priv->mic_stat.open_detect;
    mic_stat.short_detect = priv->mic_stat.short_detect;

    if (copy_to_user((void __user *)arg,
                     &mic_stat,
                     sizeof(struct ak7736_micstatus))) {
        ret = -EFAULT;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Set volume of Transmitter-level
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[out]  arg Send level data pointer.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal state.
 * @retval      -EFAULT Abnormal return.
 */
static int ak7736_dsp_ioctl_set_sendlevel_dev(struct akdsp_private_data *priv,
                                              unsigned long arg)
{
    int ret = 0;
    struct ak7736_level_vol level_vol;

    AK7736_DEBUG_FUNCTION_ENTER

    if (priv->status != AK7736_DSP_STAT_DIAG) {
        AK7736_NOTICE_LOG("SET_SENDLEVEL_DEV diag mode only. %d\n", priv->status);
        return -EINVAL;
    }

    if (copy_from_user(&level_vol,
                       (void __user *)arg, sizeof(level_vol))) {
        AK7736_ERR_LOG("SET_SENDLEVEL_DEV copy from user error\n");
        return -EFAULT;
    }

    if (level_vol.level < AK7736_NV_DATA_FES_OUT_ATT_MIN ||
        level_vol.level > AK7736_NV_DATA_FES_OUT_ATT_MAX) {
        AK7736_ERR_LOG("SET_SENDLEVEL_DEV send level is illegal(%d)\n", level_vol.level);
        return -EINVAL;
    }
    if (level_vol.vol < AK7736_NV_DATA_SEND_VOL_INDEX_MIN ||
        level_vol.vol > AK7736_NV_DATA_SEND_VOL_INDEX_MAX) {
        AK7736_ERR_LOG("SET_SENDLEVEL_DEV send volume of send level is illegal(%d)\n", level_vol.vol);
        return -EINVAL;
    }
	AK7736_INFO_LOG("SET_SENDLEVEL_DEV send volume change of send level(%d) %d -> %d\n",
                    level_vol.level, priv->send_level_vol[level_vol.level], level_vol.vol);
    priv->send_level_vol[level_vol.level] = level_vol.vol;

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Get volume of Transmitter-level
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[out]  arg Send level data pointer.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal state.
 * @retval      -EFAULT Abnormal return.
 */
static int ak7736_dsp_ioctl_get_sendlevel_dev(struct akdsp_private_data *priv,
                                              unsigned long arg)
{
    int ret = 0;
    struct ak7736_level_vol level_vol;

    AK7736_DEBUG_FUNCTION_ENTER

    if (priv->status != AK7736_DSP_STAT_DIAG) {
        AK7736_NOTICE_LOG("GET_SENDLEVEL_DEV diag mode only. %d\n", priv->status);
        return -EINVAL;
    }

    if (copy_from_user(&level_vol,
                       (void __user *)arg, sizeof(level_vol))) {
        AK7736_ERR_LOG("GET_SENDLEVEL_DEV copy from user error\n");
        return -EFAULT;
    }

    if (level_vol.level < AK7736_NV_DATA_FES_OUT_ATT_MIN ||
        level_vol.level > AK7736_NV_DATA_FES_OUT_ATT_MAX) {
        AK7736_ERR_LOG("SET_SENDLEVEL_DEV send level is illegal(%d)\n", level_vol.level);
        return -EINVAL;
    }

	AK7736_INFO_LOG("SET_SENDLEVEL_DEV send volume of send level(%d) is %d\n",
                    level_vol.level, priv->send_level_vol[level_vol.level]);
    level_vol.vol = priv->send_level_vol[level_vol.level];

    if (copy_to_user((void __user *)arg,
                     &level_vol, sizeof(level_vol))) {
        AK7736_ERR_LOG("GET_SENDLEVEL_DEV copy to user error\n");
        ret = -EFAULT;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Set volume of VR
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[out]  arg VR volume.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal state.
 * @retval      -EFAULT Abnormal return.
 */
static int ak7736_dsp_ioctl_set_vrlevel_dev(struct akdsp_private_data *priv,
                                              unsigned long arg)
{
    int ret = 0;
    int vr_vol = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    if (priv->status != AK7736_DSP_STAT_DIAG) {
        AK7736_NOTICE_LOG("SET_VRLEVEL_DEV diag mode only. %d\n", priv->status);
        return -EINVAL;
    }

    vr_vol = (int)arg;

    if (vr_vol < AK7736_NV_DATA_SEND_VOL_INDEX_MIN ||
        vr_vol > AK7736_NV_DATA_SEND_VOL_INDEX_MAX) {
        AK7736_ERR_LOG("SET_VRLEVEL_DEV vr volume is illegal(%d)\n", vr_vol);
        return -EINVAL;
    }

    AK7736_INFO_LOG("SET_VRLEVEL_DEV vr volume change %d -> %d\n",
                    priv->vr_vol, vr_vol);
    priv->vr_vol = vr_vol;
    ret = ak7736_dsp_cram_sendvolindex_table(priv,
                                             priv->vr_vol,
                                             priv->vr_vol_address);
    if (ret != 0) {
        AK7736_ERR_LOG("SET_VRLEVEL_DEV cram vr volume address error %d\n", ret);
        priv->vr_vol = 0;
        ak7736_dsp_cram_parameter(priv,
                                  AK7736_DSP_CRAM_HF_OFF_ATT,
                                  priv->vr_vol_address);
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Get volume of VR
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[out]  arg VR volume.
 * @retval      0 Normal return.
 * @retval      -EINVAL Abnormal state.
 * @retval      -EFAULT Abnormal return.
 */
static int ak7736_dsp_ioctl_get_vrlevel_dev(struct akdsp_private_data *priv,
                                              unsigned long arg)
{
    int ret = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    if (priv->status != AK7736_DSP_STAT_DIAG) {
        AK7736_NOTICE_LOG("GET_VRLEVEL_DEV diag mode only. %d\n", priv->status);
        return -EINVAL;
    }
    if (put_user(priv->vr_vol, (int __user *)arg) ) {
        AK7736_ERR_LOG("GET_VRLEVEL_DEV copy to user error\n");
        ret = -EFAULT;
    }
    AK7736_INFO_LOG("GET_VRLEVEL_DEV vr volume is %d\n", priv->vr_vol);

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Show AK7736 DSP version information
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   *attr AK7736 DSP device attribute.
 * @param[out]  *buf AK7736 DSP version information data pointer.
 * @return      AK7736 DSP version information message length.
 */
static int ak7736_dsp_version_show(struct device *dev,
                                   struct device_attribute *attr, char *buf)
{
    const unsigned long nv_address_set[3] = {AK7736_DSP_NVROM_PRAM_OFFSET,
                                             AK7736_DSP_NVROM_CRAM_OFFSET,
                                             AK7736_DSP_NVROM_PARAM_OFFSET};
    char nv_header[AK7736_DSP_NVROM_HEADER_SIZE+1];
    int ret;
    int len = 0;
    int i;

    AK7736_DEBUG_FUNCTION_ENTER

    for ( i = 0; i < 3; i++){
        ret = GetNONVOLATILE(nv_header, nv_address_set[i], AK7736_DSP_NVROM_HEADER_SIZE);
        if (unlikely(ret)) continue;
        nv_header[AK7736_DSP_NVROM_SIZE_OFFSET] = 0x00;
        nv_header[AK7736_DSP_NVROM_HEADER_SIZE] = 0x00;
        len += sprintf(&buf[len],
                       "%s: %s\n",
                       &nv_header[AK7736_DSP_NVROM_MAGIC_OFFSET],
                       &nv_header[AK7736_DSP_NVROM_VERSION_OFFSET]);
    }
    AK7736_DEBUG_FUNCTION_EXIT

    return len;
}
static DEVICE_ATTR(dsp_version, S_IRUGO | S_IWUSR | S_IWGRP,
                   ak7736_dsp_version_show, NULL);

/**
 * @brief       Initialize a reference counter IOCTL, and DSP restart
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   *attr AK7736 DSP device attribute.
 * @param[in]   *buf Not used.
 * @param[in]   size Not used.
 * @return      Size in parameter.
 */
static int ak7736_dsp_ioctl_free_set(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t size)
{
    struct akdsp_private_data *priv = dev_get_drvdata(dev);

    AK7736_DEBUG_FUNCTION_ENTER
    down(&priv->stat_lock);
    priv->open_count = 0;
    ak7736_dsp_set_status(priv,
                          AK7736_DSP_STAT_INIT,
                          AK7736_DSP_ERRSTAT_NONE);
    queue_delayed_work(priv->initwq, &priv->work, 0);
    up(&priv->stat_lock);
    AK7736_DEBUG_FUNCTION_EXIT

    return size;
}
static DEVICE_ATTR(force_free_ioctl, S_IRUGO | S_IWUSR | S_IWGRP,
                   NULL, ak7736_dsp_ioctl_free_set);




/**
 * @brief       Show AK7736 DSP status.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   *attr AK7736 DSP device attribute.
 * @param[out]  *buf AK7736 DSP status data pointer.
 * @return      AK7736 DSP status message length.
 */
static ssize_t ak7736_dsp_stat_show(struct device *dev,
                                    struct device_attribute *attr, char *buf)
{
    struct akdsp_private_data *priv = dev_get_drvdata(dev);
    unsigned char stat;
    int ret;
    int len;

    AK7736_DEBUG_FUNCTION_ENTER

    ret = ak7736_dsp_get_status(priv->client, &stat);
    len = sprintf(buf, "%2d %02x %2d %2d\n",
                  ret,stat,
                  priv->status,priv->error_stat);
    AK7736_DEBUG_FUNCTION_EXIT
    return len;
}

static DEVICE_ATTR(stat, S_IRUGO | S_IWUSR | S_IWGRP,
                   ak7736_dsp_stat_show, NULL);


#ifdef AK7736_DSP_DEBUG_SYSFS
/**
 * @brief       Show AK7736 DSP register.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   *attr AK7736 DSP device attribute.
 * @param[out]  *buf AK7736 DSP register data pointer.
 * @return      AK7736 DSP register message length.
 */
static ssize_t ak7736_dsp_register_show(struct device *dev,
                                        struct device_attribute *attr,
                                        char *buf)
{
    struct akdsp_private_data *priv = dev_get_drvdata(dev);
    unsigned char data;
    int reg;
    int ret;
    int len = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    for (reg = AK7736_DSP_REG_CONT00 ; reg <= AK7736_DSP_REG_CONT0F ; reg++) {
        ret = ak7736_dsp_contreg_read_byte(priv->client, reg, &data );
        if (unlikely(ret)) {
            len += sprintf(&buf[len], "\nI2C ERROR");
            break;
        }
        len += sprintf(&buf[len], "%02x ",data);
    }
    len += sprintf(&buf[len], "\n");

    AK7736_DEBUG_FUNCTION_EXIT
    return len;
}

static DEVICE_ATTR(dump_reg, S_IRUGO | S_IWUSR | S_IWGRP,
                   ak7736_dsp_register_show, NULL);

static unsigned short ak7736_dsp_cram_dump_address = 0x0;
/**
 * @brief       Show AK7736 DSP program (CRAM)
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   *attr AK7736 DSP device attribute.
 * @param[out]  *buf AK7736 DSP program data pointer.
 * @return      AK7736 DSP program message length.
 */
static ssize_t ak7736_dsp_cram_show(struct device *dev,
                                    struct device_attribute *attr, char *buf)
{
    struct akdsp_private_data *priv = dev_get_drvdata(dev);
    char data[3];
    int len = 0;
    int ret;

    AK7736_DEBUG_FUNCTION_ENTER

    ret = ak7736_dsp_dump (priv->client,
                           AK7736_DSP_REG_CRAM,
                           ak7736_dsp_cram_dump_address,
                           AK7736_DSP_CRAM_ALIGNMENT,
                           data,
                           AK7736_DSP_I2C_RETRY_COUNT);
    if (unlikely(ret)) {
        len += sprintf(&buf[len], "I2C ERROR %d\n", ret);
    } else {
        len += sprintf(&buf[len], "%04x: %02x %02x %02x\n",
                        ak7736_dsp_cram_dump_address,
                        data[0], data[1], data[2]);
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return len;
}

/**
 * @brief       Set AK7736 DSP program dump address (CRAM)
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   *attr AK7736 DSP device attribute.
 * @param[out]  *buf AK7736 DSP program dump address.
 * @param[in]   size Not used.
 * @return      Size in parameter.
 */
static ssize_t ak7736_dsp_cram_set(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t size)
{
    AK7736_DEBUG_FUNCTION_ENTER
    sscanf(buf,"%hx",&ak7736_dsp_cram_dump_address);
    AK7736_DEBUG_FUNCTION_EXIT
    return size;
}

static DEVICE_ATTR(dump_cram, S_IRUGO | S_IWUSR | S_IWGRP,
                   ak7736_dsp_cram_show, ak7736_dsp_cram_set);

/**
 * @brief       Set AK7736 DSP clock control
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   *attr AK7736 DSP device attribute.
 * @param[in]   *buf AK7736 DSP clock type data pointer.
 * @param[in]   size Not used.
 * @return      Size in parameter.
 */
static ssize_t ak7736_dsp_clko_set(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t size)
{
    struct akdsp_private_data *priv = dev_get_drvdata(dev);
    int onoff;
    int save_status = priv->status;

    AK7736_DEBUG_FUNCTION_ENTER

    if (sysfs_streq(buf, "1"))
    onoff = AK7736_DSP_CLKO_ON;
    else if (sysfs_streq(buf, "0"))
    onoff = AK7736_DSP_CLKO_OFF;
    else {
        return -EINVAL;
    }

    priv->status = AK7736_DSP_STAT_READY;
    ak7736_dsp_ioctl_set_clko(priv, onoff);
    priv->status = save_status;

    AK7736_DEBUG_FUNCTION_EXIT
    return size;
}

static DEVICE_ATTR(clko, S_IRUGO | S_IWUSR | S_IWGRP,
                   NULL, ak7736_dsp_clko_set);

/**
 * @brief       Test Voice recognition and apps cooperation
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   *attr AK7736 DSP device attribute.
 * @param[in]   *buf Not used.
 * @param[in]   size Not used.
 * @return      Size in parameter.
 */
static ssize_t ak7736_dsp_testvr_set(struct device *dev,
                                     struct device_attribute *attr,
                                     const char *buf, size_t size)
{
    struct akdsp_private_data *priv = dev_get_drvdata(dev);
    int ret;

    AK7736_DEBUG_FUNCTION_ENTER

    printk("ak7736_dsp_ioctl_set_control");
    ret = ak7736_dsp_ioctl_set_control(priv, 1);
    printk(" = %d\n",ret);

    printk("ak7736_dsp_ioctl_set_fsmode");
    ret = ak7736_dsp_ioctl_set_fsmode( priv, 3);
    printk(" = %d\n",ret);

    printk("ak7736_dsp_ioctl_start_vr");
    ret = ak7736_dsp_ioctl_start_vr(priv);
    printk(" = %d\n",ret);

    AK7736_DEBUG_FUNCTION_EXIT
    return size;
}

static DEVICE_ATTR(testvr, S_IRUGO | S_IWUSR | S_IWGRP,
                   NULL, ak7736_dsp_testvr_set);

/**
 * @brief       Test AK7736 DSP error
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   *attr AK7736 DSP device attribute.
 * @param[in]   *buf Not used.
 * @param[in]   size Not used.
 * @return      Size in parameter.
 */
static ssize_t ak7736_dsp_testdsperror_set(struct device *dev,
                                           struct device_attribute *attr,
                                           const char *buf, size_t size)
{
    struct akdsp_private_data *priv = dev_get_drvdata(dev);

    AK7736_DEBUG_FUNCTION_ENTER

    ak7736_dsp_register_bitop(priv->client,
                              AK7736_DSP_REG_CONT03,
                              AK7736_DSP_BIT_CRCE,
                              1);
    AK7736_DEBUG_FUNCTION_EXIT
    return size;
}

static DEVICE_ATTR(testdsperror, S_IRUGO | S_IWUSR | S_IWGRP,
                   NULL, ak7736_dsp_testdsperror_set);


static void ak7736_dsp_early_device_off(struct device *);
static int  ak7736_dsp_re_init(struct device *);
/**
 * @brief       Test driver suspend.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   *attr AK7736 DSP device attribute.
 * @param[in]   *buf Not used.
 * @param[in]   size Not used.
 * @return      Size in parameter.
 */
static ssize_t ak7736_dsp_testbudet_set(struct device *dev,
                                        struct device_attribute *attr,
                                        const char *buf, size_t size)
{
    AK7736_DEBUG_FUNCTION_ENTER
    ak7736_dsp_early_device_off(dev);
    AK7736_DEBUG_FUNCTION_EXIT
    return size;
}

static DEVICE_ATTR(testbudet, S_IRUGO | S_IWUSR | S_IWGRP,
                   NULL, ak7736_dsp_testbudet_set);

/**
 * @brief       Test driver reinitialize.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @param[in]   *attr AK7736 DSP device attribute.
 * @param[in]   *buf Not used.
 * @param[in]   size Not used.
 * @return      Size in parameter.
 */
static ssize_t ak7736_dsp_testreinit_set(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf, size_t size)
{
    AK7736_DEBUG_FUNCTION_ENTER
    ak7736_dsp_re_init(dev);
    AK7736_DEBUG_FUNCTION_EXIT
    return size;
}

static DEVICE_ATTR(testreinit, S_IRUGO | S_IWUSR | S_IWGRP,
                   NULL, ak7736_dsp_testreinit_set);


/**
 * @brief       Create debug sysfs file of AK7736 DSP driver.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @return      0 Normal return.
 */
static int ak7736_dsp_debug_sysfs_init(struct akdsp_private_data *priv)
{
    int ret = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    ret = device_create_file(&priv->client->dev, &dev_attr_dump_reg);
    if (unlikely(ret)) goto err_device_create_file;

    ret = device_create_file(&priv->client->dev, &dev_attr_dump_cram);
    if (unlikely(ret)) goto err_device_create_file;

    ret = device_create_file(&priv->client->dev, &dev_attr_clko);
    if (unlikely(ret)) goto err_device_create_file;

    ret = device_create_file(&priv->client->dev, &dev_attr_testvr);
    if (unlikely(ret)) goto err_device_create_file;

    ret = device_create_file(&priv->client->dev, &dev_attr_testdsperror);
    if (unlikely(ret)) goto err_device_create_file;

    ret = device_create_file(&priv->client->dev, &dev_attr_testbudet);
    if (unlikely(ret)) goto err_device_create_file;

    ret = device_create_file(&priv->client->dev, &dev_attr_testreinit);
    if (unlikely(ret)) goto err_device_create_file;
#ifdef CONFIG_AK7736_PSEUDO_NONVOLATILE_USE
    ak7736_dsp_nonvol_test_sysfsreg(priv);
#endif

err_device_create_file:
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Remove debug sysfs file of AK7736 DSP driver.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @return      0 Normal return.
 */
static void ak7736_dsp_debug_sysfs_exit(struct akdsp_private_data *priv)
{
    AK7736_DEBUG_FUNCTION_ENTER
    device_remove_file(&priv->client->dev, &dev_attr_stat);
    device_remove_file(&priv->client->dev, &dev_attr_dump_reg);
    device_remove_file(&priv->client->dev, &dev_attr_dump_cram);
    device_remove_file(&priv->client->dev, &dev_attr_clko);
    device_remove_file(&priv->client->dev, &dev_attr_testvr);
    device_remove_file(&priv->client->dev, &dev_attr_testdsperror);
    device_remove_file(&priv->client->dev, &dev_attr_testbudet);
    device_remove_file(&priv->client->dev, &dev_attr_testreinit);
    device_remove_file(&priv->client->dev, &dev_attr_dsp_version);
#ifdef CONFIG_AK7736_PSEUDO_NONVOLATILE_USE
    ak7736_dsp_nonvol_test_sysfsunreg(priv);
#endif

    AK7736_DEBUG_FUNCTION_EXIT
    return;
}
#endif


/**
 * @brief       Create sysfs file of AK7736 DSP driver.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @return      0 Normal return.
 */
static int ak7736_dsp_sysfs_init(struct akdsp_private_data *priv)
{
    int ret = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    ret = device_create_file(&priv->client->dev, &dev_attr_dsp_version);
    if (unlikely(ret)) goto err_device_create_file;

    ret = device_create_file(&priv->client->dev, &dev_attr_force_free_ioctl);
    if (unlikely(ret)) goto err_device_create_file;

    ret = device_create_file(&priv->client->dev, &dev_attr_stat);
    if (unlikely(ret)) goto err_device_create_file;

err_device_create_file:
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Remove sysfs file of AK7736 DSP driver.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @return      0 Normal return.
 */
static void ak7736_dsp_sysfs_exit(struct akdsp_private_data *priv)
{
    AK7736_DEBUG_FUNCTION_ENTER
    device_remove_file(&priv->client->dev, &dev_attr_dsp_version);
    device_remove_file(&priv->client->dev, &dev_attr_force_free_ioctl);
    device_remove_file(&priv->client->dev, &dev_attr_stat);

    AK7736_DEBUG_FUNCTION_EXIT
    return;
}

/**
 * @brief       AK7736 DSP poll function
 * @param[in]   *file File information.
 * @param[in]   *wait Polling wait time.
 * @return      Interrupt status.
 */
static unsigned int ak7736_dsp_poll(struct file *file, poll_table *wait)
{
    struct akdsp_private_data *priv = file->private_data;
    unsigned int mask = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    poll_wait(file, &priv->statq, wait);

    if (priv->stat_unread) {
        mask |= POLLIN | POLLRDNORM;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return mask;
}

/**
 * @brief       AK7736 DSP device open function.
 * @param[in]   *inode inode.
 * @param[in]   *file File information.
 * @return      0 Normal return.
 */
static int ak7736_dsp_open(struct inode *inode, struct file *file)
{
    struct akdsp_private_data *priv = container_of(inode->i_cdev,
                                                   struct akdsp_private_data,
                                                   cdev);
    AK7736_DEBUG_FUNCTION_ENTER

    file->private_data = priv;
    down(&priv->stat_lock);
    if (priv->open_count) {
        up(&priv->stat_lock);
        return -EALREADY;
    }
    priv->open_count++;
    up(&priv->stat_lock);

    AK7736_DEBUG_FUNCTION_EXIT
    return 0;
}

/**
 * @brief       AK7736 DSP device close function.
 * @param[in]   *inode inode.
 * @param[in]   *file File information.
 * @return      0 Normal return.
 */
static int ak7736_dsp_release(struct inode *inode, struct file *file)
{
    struct akdsp_private_data *priv = file->private_data;

    AK7736_DEBUG_FUNCTION_ENTER
    down(&priv->stat_lock);
    priv->open_count = 0;
    if (priv->status == AK7736_DSP_STAT_READY) {
        ak7736_dsp_set_clko(priv, AK7736_DSP_CLKO_OFF);
        ak7736_dsp_set_status(priv,
                              AK7736_DSP_STAT_STANDBY,
                              AK7736_DSP_ERRSTAT_NONE);
    } else if (priv->status == AK7736_DSP_STAT_FAIL) {
        ak7736_dsp_set_status(priv,
                              AK7736_DSP_STAT_INIT,
                              AK7736_DSP_ERRSTAT_NONE);
        queue_delayed_work(priv->initwq, &priv->work, 0);
    }
    up(&priv->stat_lock);
    AK7736_DEBUG_FUNCTION_EXIT
    return 0;
}

/**
 * @brief       AK7736 DSP device I/O control.
 * @param[in]   *file File information.
 * @param[in]   cmd IOCTL command.
 * @param[in/out] IOCTL command parameter.
 * @retval      0 Normal return.
 * @retval      -EINTR IOCTL interrupt.
 * @retval      -EINVAL Abnormal IOCTL command.
 * @retval      -ESHUTDOWN AK7736 driver mode is suspend.
 */
static long ak7736_dsp_ioctl(struct file *file,
                             unsigned int cmd,
                             unsigned long arg)
{
    struct akdsp_private_data *priv = file->private_data;
    int ret;

    AK7736_DEBUG_FUNCTION_ENTER

    if (!device_power) {
        return -ESHUTDOWN;
    }

    if (down_interruptible(&priv->user_lock)) {
        AK7736_NOTICE_LOG("ioctl interrupt\n");
        return -EINTR;
    }

    switch (cmd) {
    case IOCTL_AK7736DSP_GET_STATUS:
        ret = ak7736_dsp_ioctl_get_status(priv, arg);
        break;

    case IOCTL_AK7736DSP_SET_CONTROL:
        ret = ak7736_dsp_ioctl_set_control(priv, arg);
        break;

    case IOCTL_AK7736DSP_RESTART:
        ret = ak7736_dsp_ioctl_restart(priv);
        break;

    case IOCTL_AK7736DSP_SET_FSMODE:
        ret = ak7736_dsp_ioctl_set_fsmode(priv, arg);
        break;

    case IOCTL_AK7736DSP_START_HF:
        ret = ak7736_dsp_ioctl_start_hf(priv, arg);
        break;

    case IOCTL_AK7736DSP_SET_HFVOL:
        ret = ak7736_dsp_ioctl_set_hfvol(priv, arg);
        break;

    case IOCTL_AK7736DSP_STOP_HF:
        ret = ak7736_dsp_ioctl_stop_hf(priv);
        break;

    case IOCTL_AK7736DSP_START_VR:
        ret = ak7736_dsp_ioctl_start_vr(priv);
        break;

    case IOCTL_AK7736DSP_STOP_VR:
        ret = ak7736_dsp_ioctl_stop_vr(priv);
        break;

    case IOCTL_AK7736DSP_START_SIRI:
        ret = ak7736_dsp_ioctl_start_siri(priv);
        break;

    case IOCTL_AK7736DSP_STOP_SIRI:
        ret = ak7736_dsp_ioctl_stop_siri(priv);
        break;

    case IOCTL_AK7736DSP_SET_ECNCMODE:
        ret = ak7736_dsp_ioctl_set_ecncmode(priv, arg);
        break;

    case IOCTL_AK7736DSP_CHECK_DEVICE:
        ret = ak7736_dsp_ioctl_check_device(priv);
        break;



    case IOCTL_AK7736DSP_SET_DIAGMODE:
        ret = ak7736_dsp_ioctl_set_diagmode(priv, arg);
        break;

    case IOCTL_AK7736DSP_GET_MICLEVEL:
        ret = ak7736_dsp_ioctl_get_miclevel(priv, arg);
        break;

    case IOCTL_AK7736DSP_GET_MICLEVEL_STEREO:
        ret = ak7736_dsp_ioctl_get_miclevel_stereo(priv, arg);
        break;

    case IOCTL_AK7736DSP_SET_SENDLEVEL:
        ret = ak7736_dsp_ioctl_set_sendlevel(priv, arg);
        break;

    case IOCTL_AK7736DSP_GET_SENDLEVEL:
        ret = ak7736_dsp_ioctl_get_sendlevel(priv, arg);
        break;

    case IOCTL_AK7736DSP_GET_MICSTATUS:
        ret = ak7736_dsp_ioctl_get_mic_status(priv, arg);
        break;

    case IOCTL_AK7736DSP_SET_SENDLEVEL_DEV:
        ret = ak7736_dsp_ioctl_set_sendlevel_dev(priv, arg);
        break;

    case IOCTL_AK7736DSP_GET_SENDLEVEL_DEV:
        ret = ak7736_dsp_ioctl_get_sendlevel_dev(priv, arg);
        break;

    case IOCTL_AK7736DSP_SET_VRLEVEL_DEV:
        ret = ak7736_dsp_ioctl_set_vrlevel_dev(priv, arg);
        break;

    case IOCTL_AK7736DSP_GET_VRLEVEL_DEV:
        ret = ak7736_dsp_ioctl_get_vrlevel_dev(priv, arg);
        break;

    case IOCTL_AK7736DSP_GET_MICMODE:
        ret = ak7736_dsp_ioctl_get_micmode(priv, arg);
        break;

    case IOCTL_AK7736DSP_SET_MICMODE:
        ret = ak7736_dsp_ioctl_set_micmode(priv, arg);
        break;

    default:
        ret = -EINVAL;
        break;
    }

    up(&priv->user_lock);
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       Reading AK7736 DSP data (2Byte) from Nonvolatile driver.
 * @param[in]   addr Nonvolatile data offset.
 * @retval      0 AK7736 DSP data (2Byte).
 * @retval      0xFFFF Nonvolatile access error.
 */
static unsigned short ak7736_dsp_nonvolatile_short_read(unsigned int addr)
{
    int ret;
    unsigned char  data[2];
    unsigned short nv_output;

    AK7736_DEBUG_FUNCTION_ENTER
    ret = GetNONVOLATILE(data, addr, sizeof(short));
    if (ret) {
        return 0xffff;
    }

    nv_output = (data[0] << 8) | data[1];
    AK7736_DEBUG_FUNCTION_EXIT
    return nv_output;
}

/**
 * @brief       Reading AK7736 DSP data (1Byte) from Nonvolatile driver.
 * @param[in]   addr Nonvolatile data offset.
 * @retval      0 AK7736 DSP data (1Byte).
 * @retval      0xFF Nonvolatile access error.
 */
static unsigned char ak7736_dsp_nonvolatile_byte_read(unsigned int addr)
{
    int ret;
    unsigned char nv_output;

    AK7736_DEBUG_FUNCTION_ENTER
    ret = GetNONVOLATILE(&nv_output, addr, sizeof(unsigned char));
    if (ret) {
        return 0xff;
    }

    AK7736_DEBUG_FUNCTION_EXIT
    return nv_output;
}


/**
 * @brief       Reading AK7736 DSP parameter data from Nonvolatile driver.
 * @param[in]   *priv AK7736 DSP private data pointer.
 * @return      None.
 */
static void ak7736_dsp_nonvolatile_parameter_read(struct akdsp_private_data *priv)
{
    int ret;
    AK7736_DEBUG_FUNCTION_ENTER
    ret = ak7736_dsp_nonvolatile_byte_read(AK7736_DSP_NVROM_DATA_OFFSET_EXTMIC);
    if       (ret == 0x00) {
        priv->have_external_mic = true;
    }else if (ret == 0x01) {
        priv->have_external_mic = false;
    }else{
#ifdef AK7736_DSP_DEFAULT_EXTERNAL_MIC
        priv->have_external_mic = true;
#else
        priv->have_external_mic = false;
#endif
        AK7736_WARNING_LOG("EXTMIC nvdata error. default value set.\n");
    }
    AK7736_NOTICE_LOG("MIC TYPE = %d\n", priv->have_external_mic);

    priv->wq_interval_coldboot = ak7736_dsp_nonvolatile_short_read(AK7736_DSP_NVROM_DATA_OFFSET_DELAYCOLD);
    if (priv->wq_interval_coldboot == 0xffff) {
        priv->wq_interval_coldboot = AK7736_DSP_SYSTEM_INIT_WAIT_COLDBOOT;
        AK7736_WARNING_LOG("DELAYCOLDBOOT nvdata error. default value set. %08x\n",
                           priv->wq_interval_coldboot);
    }
    AK7736_NOTICE_LOG("COLDBOOT = %d\n", priv->wq_interval_coldboot);

    priv->wq_interval_fastboot = ak7736_dsp_nonvolatile_short_read(AK7736_DSP_NVROM_DATA_OFFSET_DELAYFAST);
    if (priv->wq_interval_fastboot == 0xffff) {
        priv->wq_interval_fastboot = AK7736_DSP_SYSTEM_INIT_WAIT_FASTBOOT;
        AK7736_WARNING_LOG("DELAYFASTBOOT nvdata error. default value set. %08x\n",
                           priv->wq_interval_fastboot);
    }
    AK7736_NOTICE_LOG("FASTBOOT = %d\n", priv->wq_interval_fastboot);

    priv->retry_failsafe = ak7736_dsp_nonvolatile_byte_read(AK7736_DSP_NVROM_DATA_OFFSET_FAILRETRY);
    if (( priv->retry_failsafe == 0xff )  ||
        ( priv->retry_failsafe <  AK7736_DSP_INIT_RETRY_COUNT_MIN ) ||
        ( priv->retry_failsafe >  AK7736_DSP_INIT_RETRY_COUNT_MAX )) {
        AK7736_WARNING_LOG("RETRYCOUNT nvdata error. default value set. %08x\n",
                           priv->retry_failsafe);
        priv->retry_failsafe = AK7736_DSP_INIT_RETRY_COUNT;
    }
    AK7736_NOTICE_LOG("FAILSAFE = %d\n", priv->retry_failsafe);

    priv->send_level = ak7736_dsp_nonvolatile_byte_read(AK7736_DSP_NVROM_DATA_OFFSET_SENDLEVEL);
    if (( priv->send_level == 0xff ) ||
        ( priv->send_level < AK7736_NV_DATA_FES_OUT_ATT_MIN ) ||
        ( priv->send_level > AK7736_NV_DATA_FES_OUT_ATT_MAX )){
        priv->send_level = AK7736_DSP_SENDLEVEL_MID;
        AK7736_WARNING_LOG("SENDLV nvdata error. default value set. %08x\n",
                           priv->send_level);
    }
    AK7736_NOTICE_LOG("SENDLV   = %d\n", priv->send_level);
    AK7736_DEBUG_FUNCTION_EXIT
    return;
}


/**
 * @brief       AK7736 DSP driver suspend. (early device off).
 * @param[in]   *dev AK7736 DSP device.
 * @return      None.
 */
static void ak7736_dsp_early_device_off(struct device *dev)
{
    AK7736_DEBUG_FUNCTION_ENTER
    device_power = 0;
    AK7736_DEBUG_FUNCTION_EXIT
}

/**
 * @brief       Check the resume flag of resume.
 * @param[in]   flag Suspend flag.
 * @param[in]   *param AK7736 DSP device information.
 * @return      0 Normal return.
 */
static int ak7736_dsp_resumeflag_atomic_callback(bool flag, void* param)
{
    if ( flag == false ) {
        device_power = 1;
    }
    return 0;
}

/**
 * @brief       AK7736 DSP driver reinitialize.
 * @param[in]   *dev AK7736 DSP device information.
 * @return      0 Normal return.
 */
static int ak7736_dsp_re_init(struct device *dev)
{
    struct akdsp_private_data *priv = dev_get_drvdata(dev);
    AK7736_DEBUG_FUNCTION_ENTER

    if (priv->status == AK7736_DSP_STAT_INIT_FAIL) {
        device_power = 1;
        return 0;
    }

    cancel_delayed_work_sync(&priv->work);

    early_device_invoke_with_flag_irqsave(EARLY_DEVICE_BUDET_INTERRUPTION_MASKED,
                                          ak7736_dsp_resumeflag_atomic_callback,
                                          priv);

    if (!device_power) {
        AK7736_NOTICE_LOG("re_init called in bu-det\n");
        return 0;
    }
    priv->mic_detect_done_flag = 0;

    ak7736_dsp_nonvolatile_parameter_read(priv);

    down(&priv->stat_lock);
    priv->error_count = 0;
    ak7736_dsp_set_status(priv,
                          AK7736_DSP_STAT_INIT,
                          AK7736_DSP_ERRSTAT_NONE);
    queue_delayed_work(priv->initwq, &priv->work,
                       msecs_to_jiffies(priv->wq_interval_fastboot));
    up(&priv->stat_lock);
    AK7736_DEBUG_FUNCTION_EXIT

    return 0;
}




static const struct file_operations ak7736_dsp_fops = {
    .owner		= THIS_MODULE,
    .poll		= ak7736_dsp_poll,
    .open		= ak7736_dsp_open,
    .release		= ak7736_dsp_release,
    .unlocked_ioctl	= ak7736_dsp_ioctl,
};

/**
 * @brief       AK7736 DSP driver device initialize.
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @param[in]   *id I2C devices ID.
 * @retval      0 Normal return.
 * @retval      -EIO Abnormal return.
 */
static int __devinit ak7736_dsp_probe(struct i2c_client *client,
                                      const struct i2c_device_id *id)
{
    struct akdsp_private_data *priv;
    int ret = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    priv = kzalloc(sizeof(struct akdsp_private_data), GFP_KERNEL);
    if (priv == NULL) goto probe_fail;

    if (client->dev.platform_data) {
        priv->pdata = client->dev.platform_data;
    } else {
        AK7736_WARNING_LOG("ak7736_dsp: error. platform data unavailable. check your configuration\n");
        ret = -EIO;
        goto probe_fail_kzalloc;
    }

    priv->client = client;
    i2c_set_clientdata(client, priv);
    dev_set_drvdata(&client->dev, priv);
    sema_init(&priv->stat_lock, 1);
    sema_init(&priv->user_lock, 1);
    init_waitqueue_head( &priv->statq );
    priv->status     = AK7736_DSP_STAT_INIT;
    priv->error_stat = AK7736_DSP_ERRSTAT_NONE;
    priv->mic_detect_done_flag = 0;

    priv->edev.dev                    = &client->dev;
    priv->edev.reinit                 = ak7736_dsp_re_init;
    priv->edev.sudden_device_poweroff = ak7736_dsp_early_device_off;
    ret = early_device_register(&priv->edev);
    if ( ret < 0 ){
        goto probe_fail_kzalloc;
    }

    ret = alloc_chrdev_region(&priv->devno, 0, 1, AK7736_DSP_DEVICE_NAME);
    if ( ret < 0 ){
        goto probe_fail_kzalloc;
    }

    cdev_init(&priv->cdev, &ak7736_dsp_fops);
    priv->cdev.owner = THIS_MODULE;
    priv->cdev.ops   = &ak7736_dsp_fops;
    ret = cdev_add (&priv->cdev, priv->devno, 1);
    if (unlikely(ret)) {
        goto probe_fail_alloc_chrdev_region;
    }

    priv->devclass = class_create( THIS_MODULE, AK7736_DSP_DEVICE_NAME );
    if (priv->devclass == NULL) {
        ret = 0;
        priv->status     = AK7736_DSP_STAT_INIT_FAIL;
        priv->error_stat = AK7736_DSP_ERRSTAT_NOMEM;
        goto probe_ok;
    }
    device_create( priv->devclass, NULL,
                   priv->devno, NULL,
                   AK7736_DSP_DEVICE_NAME);

#ifdef AK7736_DSP_DEBUG_SYSFS
    ak7736_dsp_debug_sysfs_init(priv);
#endif
    ret = ak7736_dsp_sysfs_init(priv);
    if (unlikely(ret)) {
        ret = 0;
        priv->status     = AK7736_DSP_STAT_INIT_FAIL;
        priv->error_stat = AK7736_DSP_ERRSTAT_NOMEM;
        goto probe_ok;
    }

    ret = request_threaded_irq(client->irq, NULL, ak7736_dsp_interrupt,
                               priv->pdata->irqflags,
                               client->dev.driver->name,
                               priv);
    if (unlikely(ret)) {
        ret = 0;
        priv->status     = AK7736_DSP_STAT_INIT_FAIL;
        priv->error_stat = AK7736_DSP_ERRSTAT_NOMEM;
        goto probe_ok;
    }

    priv->initwq = create_singlethread_workqueue("k"AK7736_DSP_DEVICE_NAME);
    if (priv->initwq == NULL) {
        ret = 0;
        priv->status     = AK7736_DSP_STAT_INIT_FAIL;
        priv->error_stat = AK7736_DSP_ERRSTAT_NOMEM;
        goto probe_ok;
    }

    ak7736_dsp_nonvolatile_parameter_read(priv);

    INIT_DELAYED_WORK(&priv->work, ak7736_dsp_init_hw);

    queue_delayed_work(priv->initwq, &priv->work,
                       msecs_to_jiffies(priv->wq_interval_coldboot));

probe_ok:
    AK7736_DEBUG_FUNCTION_EXIT
    return 0;


probe_fail_alloc_chrdev_region:
    unregister_chrdev_region(priv->devno, 1);

probe_fail_kzalloc:
    kfree(priv);

probe_fail:
    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}

/**
 * @brief       AK7736 DSP driver device remove processing.
 * @param[in]   *client I2C devices (AK7736 DSP)
 * @return      0 Normal return.
 */
static int __devexit ak7736_dsp_remove(struct i2c_client *client)
{
    struct akdsp_platform_data *pdata = client->dev.platform_data;
    struct akdsp_private_data *priv;
    int ret = 0;

    AK7736_DEBUG_FUNCTION_ENTER

    priv = i2c_get_clientdata(client);
    disable_irq(priv->client->irq);

    if (priv->initwq != NULL) {
        flush_delayed_work_sync(&priv->work);
        destroy_workqueue(priv->initwq);
    }

    /* adc powerdown */
    pdata->set_ak_adc_pdn(AK7736_DSP_RESET_LOW);
    /* Reset line set to Low */
    msleep(1);

    /* Clock reset & system reset */
    ak7736_dsp_ckresetn_setup(client, AK7736_DSP_RESET_LOW);
    ak7736_dsp_sresetn_setup (client, AK7736_DSP_RESET_LOW);

    /* dsp powerdown */
    pdata->set_ak_dsp_pdn(AK7736_DSP_RESET_LOW);
    msleep(1);

    early_device_unregister(&priv->edev);
#ifdef AK7736_DSP_DEBUG_SYSFS
    ak7736_dsp_debug_sysfs_exit(priv);
#endif
    ak7736_dsp_sysfs_exit(priv);

    device_destroy( priv->devclass , priv->devno );
    class_destroy( priv->devclass  );

    free_irq(priv->client->irq, priv);
    cdev_del(&priv->cdev);
    unregister_chrdev_region(priv->devno,1);
    if (priv->cram_parameter_table) {
        kfree(priv->cram_parameter_table);
    }
    kfree(priv);

    AK7736_DEBUG_FUNCTION_EXIT
    return ret;
}


static struct i2c_driver ak7736_dsp_driver = {
    .driver = {
        .name = AK7736_DSP_DEVICE_NAME,
        .owner = THIS_MODULE
    },
    .probe    = ak7736_dsp_probe,
    .remove   = __devexit_p(ak7736_dsp_remove),
    .id_table = ak7736_dsp_id,
};

/**
 * @brief       AK7736 DSP driver initialize.
 * @param       None.
 * @return      i2c_add_driver return.
 */
static int __init ak7736_dsp_init(void)
{
    AK7736_DEBUG_FUNCTION_ENTER_ONLY
    return i2c_add_driver(&ak7736_dsp_driver);
}
subsys_initcall(ak7736_dsp_init);

/**
 * @brief       AK7736 DSP driver finish
 * @param       None.
 * @return      i2c_del_driver return.
 */
static void __exit ak7736_dsp_exit(void)
{
    AK7736_DEBUG_FUNCTION_ENTER_ONLY
    return i2c_del_driver(&ak7736_dsp_driver);
}
module_exit(ak7736_dsp_exit);

MODULE_DESCRIPTION("AK7736 DSP Control driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("i2c:ak7736_dsp");
