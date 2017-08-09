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

/*
 *	Display color adjust driver
 */

#include <linux/module.h> 
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <asm/uaccess.h>

#include "display_color_drv.h"

#define	INF_MSGLVL			KERN_WARNING
#define	WORN_MSGLVL		KERN_WARNING
#define	ERR_MSGLVL			KERN_ERR

#define DISPLAY_COLOR_DEV_NAME			"color"
#define DISPLAY_COLOR_DEV_MINOR_MAX	1
#define WINDOW_A_SELECT				 	(1 << 4)

static int					devmajor = 0;
static int					devminor = 0;
static struct cdev			displaycolorcdev;
static struct class		*displaycolorclass;
struct task_struct			*kthread_tsk;

void set_csc_reg(struct DISP_COLOR_CSC_PARAM *csc_param)
{
	int cnt;
	unsigned long ret;

	for (cnt = 0; cnt < 3; cnt++) {
		writel(WINDOW_A_SELECT << cnt, 	 0xFE900000 + 0x42 * 4);
		writel(csc_param->yof   & 0x00ff,0xFE900000 + 0x611 * 4);
		writel(csc_param->kyrgb & 0x03ff,0xFE900000 + 0x612 * 4);
		writel(csc_param->kur   & 0x07ff,0xFE900000 + 0x613 * 4);
		writel(csc_param->kvr	& 0x07ff,0xFE900000 + 0x614 * 4);
		writel(csc_param->kug	& 0x03ff,0xFE900000 + 0x615 * 4);
		writel(csc_param->kvg	& 0x03ff,0xFE900000 + 0x616 * 4);
		writel(csc_param->kub	& 0x07ff,0xFE900000 + 0x617 * 4);
		writel(csc_param->kvb	& 0x07ff,0xFE900000 + 0x618 * 4);
	}

}

void set_pal_reg(struct DISP_COLOR_PALETTE_PARAM *pal_param)
{
	unsigned long	regval;
	int				ctr;
	int				cnt;

#if 0
	for (cnt = 0; cnt < 3; cnt++) {
		writel(WINDOW_A_SELECT << cnt, 	 0xFE900000 + 0x42 * 4);
		for(ctr = 0; ctr < 256; ++ctr) {
			regval = pal_param->red  [ctr] |
					(pal_param->green[ctr] << 8) |
					(pal_param->blue [ctr] << 16);
			writel(regval, 0xFE900000 + ( 0x500 + ctr ) * 4);
//			writel(regval, IO_TO_VIRT(0x54200000) + 0x500 + ctr);
		}
	}
#else

		writel(WINDOW_A_SELECT << 0, 	 0xFE900000 + 0x42 * 4);
		for(ctr = 0; ctr < 256; ++ctr) {
			regval = pal_param->red  [ctr] |
					(pal_param->green[ctr] << 8) |
					(pal_param->blue [ctr] << 16);
			writel(regval, 0xFE900000 + ( 0x500 + ctr ) * 4);
		}

		writel(WINDOW_A_SELECT << 1, 	 0xFE900000 + 0x42 * 4);
		for(ctr = 0; ctr < 256; ++ctr) {
			regval = pal_param->red  [ctr] |
					(pal_param->green[ctr] << 8) |
					(pal_param->blue [ctr] << 16);
			writel(regval, 0xFE900000 + ( 0x500 + ctr ) * 4);
		}

		writel(WINDOW_A_SELECT << 2, 	 0xFE900000 + 0x42 * 4);
		for(ctr = 0; ctr < 256; ++ctr) {
			regval = ctr |
					(ctr << 8) |
					(ctr << 16);
			writel(regval, 0xFE900000 + ( 0x500 + ctr ) * 4);
		}
#endif
}

static int display_color_drv_open(	struct inode	*inode,
									struct file		*file)
{
	return 0;
}

static int display_color_drv_close(	struct inode	*inode,
									struct file		*file)
{
	return 0;
}

static long display_color_drv_ioctl(struct file		*file,
									unsigned int	reqcode, 
									unsigned long	msg)
{
	struct DISP_COLOR_PALETTE_PARAM	pal_param;
	struct DISP_COLOR_CSC_PARAM		csc_param;

	switch(reqcode) {
	case DISP_COLOR_REQ_SET_CSC:
		if(copy_from_user(&csc_param, (void __user *)msg, sizeof(csc_param))) {
			return -EFAULT;
		}
		set_csc_reg(&csc_param);
		break;
	case DISP_COLOR_REQ_SET_PAL:
		if(copy_from_user(&pal_param, (void __user *)msg, sizeof(pal_param))) {
			return -EFAULT;
		}
		set_pal_reg(&pal_param);
		break;
	default:
		return -EINVAL;
		break;
	}

	return 0;
}

static const struct file_operations displaycolor_drv_fops = {
	.owner			= THIS_MODULE,
	.open			= display_color_drv_open,
	.release		= display_color_drv_close,
	.unlocked_ioctl	= display_color_drv_ioctl,
};

static int __init display_color_drv_init_module(void)
{
	int		ret;
	dev_t	devno;

	kthread_tsk	= NULL;

	printk(KERN_NOTICE "Display color adjust driver active\n");

	if(devmajor) {
		devno	= MKDEV(devmajor, devminor);
		ret		= register_chrdev_region(devno,
						DISPLAY_COLOR_DEV_MINOR_MAX, DISPLAY_COLOR_DEV_NAME);
	} else {
		ret		= alloc_chrdev_region(&devno, devminor, 
						DISPLAY_COLOR_DEV_MINOR_MAX, DISPLAY_COLOR_DEV_NAME);
		devmajor= MAJOR(devno);
	}

	if(ret < 0) {
		printk(KERN_ERR "register_chrdev %d, %s failed, error code: %d\n",
    					devmajor, DISPLAY_COLOR_DEV_NAME, ret);
		return ret;
	}

	cdev_init(&displaycolorcdev, &displaycolor_drv_fops);
	displaycolorcdev.owner = THIS_MODULE;
	ret = cdev_add(&displaycolorcdev, devno, DISPLAY_COLOR_DEV_MINOR_MAX);
	if(ret < 0) {
		printk(KERN_ERR "cdev_add %s failed %d\n",
							DISPLAY_COLOR_DEV_NAME, ret);
		device_destroy(displaycolorclass, MKDEV(devmajor, devminor));
		return ret;
	}

	displaycolorclass = class_create(THIS_MODULE, "colordev");
	if (IS_ERR(displaycolorclass)) {
		printk(KERN_ERR "class_create failed %d\n", ret);
		cdev_del(&displaycolorcdev);
		return ret;
    }

	if (!device_create(displaycolorclass, NULL, MKDEV(devmajor, devminor),
					NULL, "%s", DISPLAY_COLOR_DEV_NAME)) 
	{
    	printk(KERN_ERR "class_device_create failed %s %d\n", 
												DISPLAY_COLOR_DEV_NAME, ret);
        class_destroy(displaycolorclass);
		return ret;
    }

	return 0;
}
	
static void __exit display_color_drv_exit_module(void)
{
	printk(KERN_NOTICE "Display color adjust driver exit\n");
	device_destroy(displaycolorclass, MKDEV(devmajor, devminor));
	class_destroy(displaycolorclass);
	cdev_del(&displaycolorcdev);
	unregister_chrdev_region(MKDEV(devmajor, devminor),
									DISPLAY_COLOR_DEV_MINOR_MAX);

	return;
}

module_init(display_color_drv_init_module);
module_exit(display_color_drv_exit_module);

MODULE_LICENSE("GPL");
