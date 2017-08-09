#include "ak7736_dsp_pram.h"
#include "ak7736_dsp_cram_internal.h"
#include "ak7736_dsp_cram_external.h"
#include "ak7736_dsp_param.h"


#ifdef AK7736_DSP_DEFAULT_EXTERNAL_MIC
static int cram_select = 1;
#else
static int cram_select = 0;
#endif
static int nonvol_debug_mode  = 0;

const char ak7736_dsp_error_magic[] = "    ";

static int ak7736_dsp_re_init(struct device  *dev);
static int SetNONVOLATILE(unsigned char* buff,
                          uint offset,
                          uint outsize)
{
  return 0;
}
static int GetNONVOLATILE(unsigned char* buff,
                          uint offset,
                          uint outsize)
{
    const char    *pdata = NULL;
    unsigned int   size;

    // type check
    if (offset == AK7736_DSP_NVROM_PRAM_OFFSET) {
            pdata = dsp_pram_data;
            size  = sizeof(dsp_pram_data);
    }else if (offset == AK7736_DSP_NVROM_CRAM_OFFSET) {

      if (cram_select){
        pdata = dsp_cram_data_external;
        size  = sizeof(dsp_cram_data_external);
      }else{
        pdata = dsp_cram_data_internal;
        size  = sizeof(dsp_cram_data_internal);
      }
    }else if (offset == AK7736_DSP_NVROM_PARAM_OFFSET) {
            pdata = dsp_param_data;
            size  = sizeof(dsp_param_data);
    }else{
      return -1;
    }
    memcpy(&buff[0], pdata , outsize);

    return 0;
}

#if 0
static int GetNONVOLATILE(unsigned char* buff,
                          uint offset,
                          uint outsize)
{
    const char    *pdata = NULL;
    char           header[16];
    unsigned int   size;
    unsigned short crc = 0;
    const char    *magic;

    // type check
    memset(header, 0x00 , sizeof(header));
    if (offset == AK7736_DSP_NVROM_PRAM_OFFSET) {
        if ( nonvol_debug_mode == 1) {
            return -EINVAL;
        }else{
            pdata = dsp_pram_data;
        }
        if ( nonvol_debug_mode == 2) {
            size  = 0xf0ffffff;
        }else if ( nonvol_debug_mode == 10) {
            size  = 11;
        }else{
            size  = sizeof(dsp_pram_data);
        }
        if ( nonvol_debug_mode == 3) {
            magic = ak7736_dsp_error_magic;
        }else{
            magic = ak7736_dsp_pram_magic;
        }
        printk("pseudo_nonvolatile: pram image[ENCV36_S3_08A.obj]\n");
    }else if (offset == AK7736_DSP_NVROM_CRAM_OFFSET) {
        if ( nonvol_debug_mode == 4) {
            return -EINVAL;
        }else if ( nonvol_debug_mode == 5){
            size  = 0xf0ffffff;
        }else if ( nonvol_debug_mode == 11) {
            size  = 11;
        }else if (cram_select) {
            pdata = dsp_cram_data_internal;
            size  = sizeof(dsp_cram_data_internal);
            printk("pseudo_nonvolatile: cram image[ENCV36_S3_08A_IMic_01.cra]\n");
        }else {
            pdata = dsp_cram_data_external;
            size  = sizeof(dsp_cram_data_external);
            printk("pseudo_nonvolatile: cram image[ENCV36_S3_08A_AMic_01..cra]\n");
        }
        if ( nonvol_debug_mode == 6) {
            magic = ak7736_dsp_error_magic;
        }else{
            magic = ak7736_dsp_cram_magic;
        }
    }else if (offset == AK7736_DSP_NVROM_PARAM_OFFSET) {
        if ( nonvol_debug_mode == 7) {
            return -EINVAL;
        }else{
            pdata = dsp_param_data;
        }
        if ( nonvol_debug_mode == 8) {
            size  = 0xf0ffffff;
        }else{
            size  = sizeof(dsp_param_data);
        }
        if ( nonvol_debug_mode == 9) {
            magic = ak7736_dsp_error_magic;
        }else{
            magic = ak7736_dsp_param_magic;
        }
    }else{
      return -1;
    }

    // check size
    if ( outsize < (size + 16)) {
        return -1;
    }

    // make header
    memcpy(&header[0], magic,         4);
    memcpy(&header[4], (void *)&size, 4);
    if (pdata && size != 0xf0ffffff)
           crc = crc_itu_t(0x0, pdata, size);
    if (offset == AK7736_DSP_NVROM_PRAM_OFFSET) {
        if ( nonvol_debug_mode == 20) {
            crc = 0;
        }
    }else if (offset == AK7736_DSP_NVROM_CRAM_OFFSET) {
        if ( nonvol_debug_mode == 21) {
            crc = 0;
        }
    }else if (offset == AK7736_DSP_NVROM_PARAM_OFFSET) {
        if ( nonvol_debug_mode == 22) {
            crc = 0;
        }
    }
    memcpy(&header[8], &crc, 2);

    // copy data
    memcpy(&buff[ 0], header, 16);
    if (pdata && size != 0xf0ffffff)
    memcpy(&buff[16], pdata , size);

    printk("pseudo_nonvolatile: load ok. crc[%s]=0x%04x\n", magic, crc);

    return 0;
}
#endif

static ssize_t ak7736_dsp_cramsel_show(struct device *dev,
                                    struct device_attribute *attr, char *buf)
{
    int len = 0;

    AK7736_DEBUG_FUNCTION_ENTER
    len += sprintf(&buf[len], "%d\n", cram_select);
    AK7736_DEBUG_FUNCTION_EXIT
    return len;
}


static ssize_t ak7736_dsp_cramsel_set(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t size)
{

    if (sysfs_streq(buf, "1"))
    cram_select = 1;
    else if (sysfs_streq(buf, "0"))
    cram_select = 0;
    else {
        return -EINVAL;
    }

    ak7736_dsp_re_init(dev);

    return size;
}

static DEVICE_ATTR(cram, S_IRUGO | S_IWUSR | S_IWGRP,
                   ak7736_dsp_cramsel_show, ak7736_dsp_cramsel_set);


static ssize_t ak7736_dsp_nonvol_set(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t size)
{
    sscanf(buf, "%d", &nonvol_debug_mode);
    return size;
}

static DEVICE_ATTR(debug_nonvol, S_IRUGO | S_IWUSR | S_IWGRP,
                   NULL, ak7736_dsp_nonvol_set);


static ssize_t ak7736_dsp_clear_para_set(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t size)
{
    struct akdsp_private_data *priv = dev_get_drvdata(dev);
    if (priv->cram_parameter_table) {
        kfree(priv->cram_parameter_table);
        priv->cram_parameter_table = NULL;
    }
    return size;
}

static DEVICE_ATTR(debug_paraclr, S_IRUGO | S_IWUSR | S_IWGRP,
                   NULL, ak7736_dsp_clear_para_set);




static void ak7736_dsp_nonvol_test_sysfsreg(struct akdsp_private_data *priv)
{
    int ret;
    ret = device_create_file(&priv->client->dev, &dev_attr_cram);
    ret = device_create_file(&priv->client->dev, &dev_attr_debug_nonvol);
    ret = device_create_file(&priv->client->dev, &dev_attr_debug_paraclr);
}

static void ak7736_dsp_nonvol_test_sysfsunreg(struct akdsp_private_data *priv)
{
    device_remove_file(&priv->client->dev, &dev_attr_cram);
    device_remove_file(&priv->client->dev, &dev_attr_debug_nonvol);
    device_remove_file(&priv->client->dev, &dev_attr_debug_paraclr);
}



