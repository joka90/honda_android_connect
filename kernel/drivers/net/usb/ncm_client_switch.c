#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/ctype.h>
#include <linux/ethtool.h>
#include <linux/workqueue.h>
#include <linux/mii.h>
#include <linux/crc32.h>
#include <linux/usb.h>
#include <linux/version.h>
#include <linux/timer.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/usb/usbnet.h>
#include <linux/usb/cdc.h>

#include <linux/usb/cdc_ncm.h>

#define DRIVER_AUTHOR "MEENA"
#define DRIVER_DESC   "DUMMY NCM CLIENT SWITCH"

void *
mccildk_register_client_ncm_switch(
	void
	)
    {
    return NULL;
    }

EXPORT_SYMBOL(mccildk_register_client_ncm_switch);

MODULE_LICENSE("Non GPL");

MODULE_AUTHOR(DRIVER_AUTHOR);           /* Who wrote this module? */
MODULE_DESCRIPTION(DRIVER_DESC);        /* What does this module do? */
