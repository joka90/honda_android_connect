/*  ----------------------------------------------------------------------

    Copyright (C) 2012 Joe Wei, HTC

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    ---------------------------------------------------------------------- */
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <asm/uaccess.h>
#include <linux/usb.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/mm.h>
#include <linux/completion.h>
#include <linux/version.h>

#ifdef CONFIG_HIBERNATION
#include <linux/freezer.h>
#endif


#if 1 /* ADA_CUSTOMIZE */
#include <linux/usb/hsml_client.h>
#include <linux/cdev.h>
#else
#include "hsml_client.h"
#endif/* ADA_CUSTOMIZE */

//#define DEBUG_VERBOSE
#ifdef DEBUG_VERBOSE
#define HDEBUG(x...) printk("[HSML] "x)
#else
#define HDEBUG(x...)
#endif

/* Define these values to match your devices */
#define CLASS		0XFF
#define SUBCLASS	0XFF
#define PROTOCOL	0XFF

#ifdef ADA_CUSTOMIZE
#define HSML_USB_TIMEOUT 10000
#endif/* ADA_CUSTOMIZE */

/* Get a minor range for your devices from the usb maintainer */
#ifdef ADA_CUSTOMIZE
#ifdef CONFIG_USB_DYNAMIC_MINORS
#define USB_HSML_MINOR_BASE	0
#else
#define USB_HSML_MINOR_BASE	192
#endif
#else
#define USB_HSML_MINOR_BASE	192
#endif  /* ADA_CUSTOMIZE */

#define NUM_FB				3

#define PIXEL_FORMAT_RGB565					(1 << 0)

#define HSML_SIGNATURE_OFFSET		0

enum {
	HSML_FB_READER_STATE_READ_PAYLOAD = 0,
	HSML_FB_READER_STATE_READ_HEADER,
	HSML_FB_READER_STATE_RECOVERY,

	HSML_FB_READER_STATE_COUNT
};

/*----------------------------------------------------------------
 * HSML Client struct
 * ----------------------------------------------------------------*/
struct fb_infox {
    int framesize;
    int height;
    int width;
};

/* Structure to hold all of our device specific stuff */
struct usb_hsml_client {
	struct usb_device *udev;			/* the usb device for this device */
	struct usb_interface *interface;		/* the interface for this device */
	size_t			bulk_in_size;		/* the size of the receive buffer */
#ifdef ADA_CUSTOMIZE
	size_t			bulk_out_size;		/* the size of the receive buffer */
#endif /* ADA_CUSTOMIZE */
	__u8			bulk_in_endpointAddr;	/* the address of the bulk in endpoint */
#ifdef ADA_CUSTOMIZE
	__u8			bulk_out_endpointAddr;	/* the address of the bulk out endpoint */
#endif /* ADA_CUSTOMIZE */
	__u8 			can_out_endpointAddr;
	int			errors;			/* the last request tanked */
	int			open_count;		/* count the number of openers */
	spinlock_t		err_lock;		/* lock for errors */
	struct kref		kref;
	struct rw_semaphore	io_rwsem;		/* synchronize I/O with disconnect */
	struct fb_infox  fb_info_data;
	server_info_type server_info_data;
	struct work_struct display_work;
	int recv_fb_size;

	struct work_struct read_work;
	struct usb_anchor read_submitted;
#ifdef ADA_CUSTOMIZE
	struct usb_anchor send_submitted;
#endif /* ADA_CUSTOMIZE */
	u8 fb_reader_state;
	int remaining_size;

	struct hsml_header fb_header;

	struct urb *rx_header_urb;
	struct urb *rx_urb[NUM_FB];
	void *rx_urb_buf[NUM_FB];
#ifdef ADA_CUSTOMIZE
	void *kmalloc_buf[NUM_FB];
	struct urb *tx_urb;
#endif /* ADA_CUSTOMIZE */
	int buf_index[NUM_FB];
	spinlock_t buf_lock;
	struct completion buf_comp;

	int is_new;
	int buf_num;
};


#define to_hsml_client_dev(d) container_of(d, struct usb_hsml_client, kref)

static atomic_t recv_fb_enable;		//0: stop, 1: start, 2: prepare to stop

static struct usb_driver hsml_client_driver;
static void hsml_client_draw_down(struct usb_hsml_client *dev);
static struct workqueue_struct *wq_frame_reader;

static const char hsml_sig[] = {
	0xFF, 0xFF, 0x48, 0x53, 0x4D, 0x4C, 0xFF, 0xFF
};

#ifdef ADA_CUSTOMIZE
static const unsigned int hsml_devs = 1; /* device count */
static unsigned int hsml_major = 0;
static struct cdev hsml_cdev;
static struct class	 *hsml_class;
static dev_t dev_id;
#endif /* ADA_CUSTOMIZE */

/**
 * Called when there is no reference to hsml client device
 *
 * @kerf: kobject reference
 */
static void hsml_client_delete(struct kref *kref)
{
	struct usb_hsml_client *dev = to_hsml_client_dev(kref);
	int i;
#if 1 /* ADA_CUSTOMIZE */
	printk("[HSML] hsml_client_delete\n");
#endif/* ADA_CUSTOMIZE */
	usb_put_dev(dev->udev);

	for (i = 0; i < NUM_FB; i++) {
		if (dev->rx_urb_buf[i]) {
		#if LINUX_VERSION_CODE > 0x020620
			usb_free_coherent(dev->udev, dev->fb_info_data.framesize,
					dev->rx_urb_buf[i], dev->rx_urb[i]->transfer_dma);
		#else
			usb_buffer_free(dev->udev, dev->fb_info_data.framesize,
					dev->rx_urb_buf[i], dev->rx_urb[i]->transfer_dma);
		#endif
			dev->rx_urb_buf[i] = NULL;
		}

		if (dev->rx_urb[i]) {
			usb_free_urb(dev->rx_urb[i]);
			dev->rx_urb[i] = NULL;
		}
#ifdef ADA_CUSTOMIZE
		if (dev->kmalloc_buf[i]) {
			kfree(dev->kmalloc_buf[i]);
			dev->kmalloc_buf[i] = NULL;
		}
#endif /* ADA_CUSTOMIZE */
	}

	if (dev->rx_header_urb) {
		usb_free_urb(dev->rx_header_urb);
		dev->rx_header_urb = NULL;
	}

#ifdef ADA_CUSTOMIZE
	if (dev->tx_urb) {
		usb_free_urb(dev->tx_urb);
		dev->tx_urb = NULL;
	}
#endif /* ADA_CUSTOMIZE */

	kfree(dev);
}

/**
 * Called when a userland program opens the HSML Client device
 *
 * @inode:
 * @file:
 */
static int hsml_client_open(struct inode *inode, struct file *file)
{
	struct usb_hsml_client *dev;
	struct usb_interface *interface;
	int subminor;
	int retval = 0;
#if 1 /* ADA_CUSTOMIZE */
	printk("[HSML] hsml_client_open\n");
#endif/* ADA_CUSTOMIZE */
	subminor = iminor(inode);

	interface = usb_find_interface(&hsml_client_driver, subminor);
	if (!interface) {
		err ("%s - error, can't find device for minor %d",
		     __func__, subminor);
		retval = -ENODEV;
		goto exit;
	}

	dev = usb_get_intfdata(interface);

	if (!dev) {
		retval = -ENODEV;
		goto exit;
	}

	/* increment our usage count for the device */
	kref_get(&dev->kref);

	/* lock the device to allow correctly handling errors
	 * in resumption */
	down_write(&dev->io_rwsem);

	if (!dev->open_count++) {
		retval = usb_autopm_get_interface(interface);
		if (retval) {
			dev->open_count--;
			up_write(&dev->io_rwsem);
			kref_put(&dev->kref, hsml_client_delete);
			goto exit;
		}
	}

	/* save our object in the file's private structure */
	file->private_data = dev;
	up_write(&dev->io_rwsem);

exit:
	return retval;
}

/**
 * Called when a user-space program close the HSML Client device
 *
 * @inode:
 * @file:
 */
static int hsml_client_release(struct inode *inode, struct file *file)
{
	struct usb_hsml_client *dev;
#if 1 /* ADA_CUSTOMIZE */
	printk("[HSML] hsml_client_release\n");
#endif/* ADA_CUSTOMIZE */
	dev = (struct usb_hsml_client *)file->private_data;
	if (dev == NULL)
		return -ENODEV;

	/* allow the device to be autosuspended */
	down_write(&dev->io_rwsem);
	if (!--dev->open_count && dev->interface)
		usb_autopm_put_interface(dev->interface);
	up_write(&dev->io_rwsem);

	/* decrement the count on our device */
	kref_put(&dev->kref, hsml_client_delete);
	return 0;
}


/**
 * Called when a user-space program close/flush the HSML Client device
 *
 * @file:
 * @id:
 */
static int hsml_client_flush(struct file *file, fl_owner_t id)
{
	struct usb_hsml_client *dev;
	int res;
#if 1 /* ADA_CUSTOMIZE */
	printk("[HSML] hsml_client_flush\n");
#endif/* ADA_CUSTOMIZE */
	dev = (struct usb_hsml_client *)file->private_data;
	if (dev == NULL)
		return -ENODEV;

	/* wait for io to stop */
	down_write(&dev->io_rwsem);
	hsml_client_draw_down(dev);

	/* read out errors, leave subsequent opens a clean slate */
	spin_lock_irq(&dev->err_lock);
	res = dev->errors ? (dev->errors == -EPIPE ? -EPIPE : -EIO) : 0;
	dev->errors = 0;
	spin_unlock_irq(&dev->err_lock);

	up_write(&dev->io_rwsem);

	return res;
}


/**
 *	Return the start of framebuffer data
 *
 *	This function is called when data is corrupted.
 *
 *	@dev: HSML client device
 *	@src: data source
 *	@size: size of src
 *	@return the start of framebuffer data
 */
static int hsml_client_get_offset(struct usb_hsml_client *dev, char *src, u32 size)
{
	char *ptr = src;
	int copy_size = 0;
	size_t sig_size = sizeof(hsml_sig);
	u8 found = 0;

	while (((src + size) - ptr) >= sig_size) {
		if (!memcmp(ptr, hsml_sig, sig_size)) {
			printk(KERN_INFO "%s: Found header signature.\n", __func__);
			/* Move to the framebuffer data */
			ptr += (sizeof(dev->fb_header) - HSML_SIGNATURE_OFFSET);
			if (ptr >= (src + size)) {
				pr_err("%s: Remaining size is not large enough.\n", __func__);
				return -ERANGE;
			}
			found = 1;
			break;
		}

		ptr++;
	}

	if (found) {
		char *dst = NULL;

		copy_size = (src + size) - ptr;
		dst = kmalloc(copy_size, GFP_ATOMIC);
		if (!dst)
			return -ENOMEM;
		memcpy(dst, ptr, copy_size);
		memcpy(src, dst, copy_size);

		kfree(dst);

		return (size - copy_size);
	}

    return 0;
}

/**
 * Check whether the header is corrupted
 *
 * @fb_header: Framebuffer header
 *
 * Return 0 on success, != 0 otherwise
 */
static int check_fb_header(struct hsml_header *fb_header)
{
	if (strncmp(fb_header->signature, hsml_sig, sizeof(fb_header->signature)))
		return -ECOMM;
#if 0 /* DEBUG */
	if (be16_to_cpu(fb_header->num_context_info) > 0) {
		struct context_info info;
		memcpy(&info, fb_header->reserved, sizeof(info));

		printk(KERN_INFO "frameBufferDataSize=%d\n", be32_to_cpu(fb_header->framebuffer_size));
		printk(KERN_INFO "num_context_info=%d\n", be16_to_cpu(fb_header->num_context_info));
		printk(KERN_INFO "x=%d\n", info.x);
		printk(KERN_INFO "y=%d\n", info.y);
		printk(KERN_INFO "width=%d\n", be16_to_cpu(info.width));
		printk(KERN_INFO "height=%d\n", be16_to_cpu(info.height));
		printk(KERN_INFO "app_id=0x%08X\n", be32_to_cpu(info.app_id));
		printk(KERN_INFO "app_category_trust_level=0x%04X\n", be16_to_cpu(info.app_category_trust_level));
		printk(KERN_INFO "content_category_trust_level=0x%04X\n", be16_to_cpu(info.content_category_trust_level));
		printk(KERN_INFO "app_category=0x%08X\n", be32_to_cpu(info.app_category));
		printk(KERN_INFO "content_category=0x%08X\n", be32_to_cpu(info.content_category));
		printk(KERN_INFO "content_rules=0x%08X\n", be32_to_cpu(info.content_rules));
	}
#endif /* DEBUG */
	return 0;
}


#define HSML_BACK_BUFFER	0
#define HSML_MIDDLE_BUFFER	1
#define HSML_FRONT_BUFFER 	2

/**
 * Swap the framebuffers
 *
 * @dev: HSML client device
 * @dst: the index of destination buffer
 * @src: the index of source buffer
 * Return 0 on success, != 0 otherwise
 */
static void hsml_client_swap_buffer(struct usb_hsml_client *dev, int dst, int src)
{
	unsigned long flags;
	int temp_index;

	spin_lock_irqsave(&dev->buf_lock, flags);

	temp_index = dev->buf_index[dst];
	dev->buf_index[dst] = dev->buf_index[src]; 
	dev->buf_index[src] = temp_index;
	
	spin_unlock_irqrestore(&dev->buf_lock, flags);
}


/**
 * URB callback function for Framebuffer Reader
 *
 * @urb:
 */
static void hsml_client_readfb_callback(struct urb *urb)
{
	struct usb_hsml_client *dev = urb->context;
	int  read_next = 1;
	HDEBUG("hsml_client_readfb_callback \n");
	/* sync/async unlink faults aren't errors */
	if (urb->status) {
		pr_err("%s - nonzero write bulk status received: %d, actual=%d, transfer=%d",
			__func__, urb->status, urb->actual_length,
			urb->transfer_buffer_length);

		if (urb->status == -EOVERFLOW) {
			dev->remaining_size = urb->transfer_buffer_length - urb->actual_length;
			if (dev->remaining_size < 0)
				dev->remaining_size = 0;
		} else {
			read_next = 0;
			spin_lock(&dev->err_lock);
			dev->errors = urb->status;
			spin_unlock(&dev->err_lock);
		}
	} else {
		if (urb->actual_length < 0 || urb->actual_length != urb->transfer_buffer_length)
			pr_err("%s: error actual_len=%d transfer_buffer_len=%d\n",
					__func__, urb->actual_length,
					urb->transfer_buffer_length);

		switch (dev->fb_reader_state) {
		case HSML_FB_READER_STATE_READ_PAYLOAD:
			dev->fb_reader_state = HSML_FB_READER_STATE_READ_HEADER;
			hsml_client_swap_buffer(dev, HSML_MIDDLE_BUFFER, HSML_BACK_BUFFER);
			dev->is_new = 1;
			break;

		case HSML_FB_READER_STATE_READ_HEADER:
			if (check_fb_header(&dev->fb_header)) {
				pr_err("%s: checksum error, starts recovery mechanism.\n", __func__);
				dev->fb_reader_state = HSML_FB_READER_STATE_RECOVERY;
			} else
				dev->fb_reader_state = HSML_FB_READER_STATE_READ_PAYLOAD;
			break;

		case HSML_FB_READER_STATE_RECOVERY:
			dev->remaining_size = hsml_client_get_offset(dev,
					(char *)urb->transfer_buffer, urb->actual_length);
			if (dev->remaining_size < 0)
				dev->remaining_size = 0;
			dev->fb_reader_state = HSML_FB_READER_STATE_READ_PAYLOAD;
			break;
		default:
			pr_err("%s: Invalid usb state: %d\n", __func__, dev->fb_reader_state);
			break;
		}
	}

	if (read_next && (atomic_read(&recv_fb_enable) == 1)) {
		queue_work(wq_frame_reader, &dev->read_work);
		complete(&dev->buf_comp);
	}
}


/**
 * Work function for framebuffer reader
 *
 * @work: work structure
 */
static void work_read_fb(struct work_struct *work)
{
	struct usb_hsml_client *dev = container_of(work, struct usb_hsml_client, read_work);
	int len = 0;
	char *ptr;
	int retval = 0;
	int index = dev->buf_index[HSML_BACK_BUFFER];
	struct urb *urb = NULL;

	if (index >= NUM_FB || index < 0) {
		pr_err("%s: index %d out of range\n", __func__, index);
		goto error;
	}

	if (!dev->fb_info_data.framesize) {
		pr_err("%s: fail. fb info should be read first.\n", __func__);
		goto error;
	}

	if (dev->fb_reader_state == HSML_FB_READER_STATE_READ_HEADER) {
		urb = dev->rx_header_urb;
		ptr = (char *)&dev->fb_header;
		len = sizeof(dev->fb_header);
	} else {
		urb = dev->rx_urb[index];
		urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
		ptr = (char *)dev->rx_urb_buf[index];
		len = dev->fb_info_data.framesize;
	}

	if (dev->remaining_size > 0) {
		ptr += (len - dev->remaining_size);
		len = dev->remaining_size;
	}

	down_read(&dev->io_rwsem);
	if (!dev->interface) {		/* disconnect() was called */
		up_read(&dev->io_rwsem);
		goto error;
	}

	usb_fill_bulk_urb(urb, dev->udev,
		  usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpointAddr),
		  ptr, len,
		  hsml_client_readfb_callback, dev);
	usb_anchor_urb(urb, &dev->read_submitted);
	retval = usb_submit_urb(urb, GFP_ATOMIC);
	if (retval) {
		pr_err("%s: failed to submit URB. (err=%d)\n", __func__, retval);
		up_read(&dev->io_rwsem);
		goto error_unanchor;
	}

	up_read(&dev->io_rwsem);
	return;

error_unanchor:
	usb_unanchor_urb(urb);
error:
	return;
}


/**
 * Enable/Disable the framebuffer reader
 *
 * @dev: Client HSML device
 * @enabled: 0 means to disable, otherwise to enable
 */
static void hsml_client_enable_fb_work(struct usb_hsml_client *dev, int enabled)
{
	if (enabled) {
		int i = 0;

		/* Reset the buffer index */
		for (i = 0; i < NUM_FB; i++)
			dev->buf_index[i] = i;

		atomic_set(&recv_fb_enable, 1);
	} else {
		atomic_set(&recv_fb_enable, 0);
	}
}


/**
 * HSML specific request handler
 *
 * @dev: Client HSML device
 * @request: The HSML request
 * @return > 0 on success, otherwise on failure.
 */
static int hsml_client_handle_request(
		struct usb_hsml_client *dev, struct hsml_request *request)
{
	int value;
#ifdef ADA_CUSTOMIZE
	int act_value;
#endif /* ADA_CUSTOMIZE */
	int schedule_read_work = 0;
	unsigned int pipe = 0;
	__u8 requesttype = USB_TYPE_VENDOR | USB_RECIP_INTERFACE;

	if (!dev)
		return -ENODEV;

	down_read(&dev->io_rwsem);
	if (!dev->interface) {
		pr_err("%s: usb is diconnected!\n", __func__);
		value = -ENODEV;
		goto exit;
	}

	value = request->len;
	switch (request->code) {
	case HSML_REQ_GET_SERVER_VERSION:
	case HSML_REQ_GET_SERVER_CONF:
	case HSML_REQ_GET_SERVER_DISP:
	case HSML_REQ_GET_NUM_COMPRESSION_SETTINGS:
		break;

	case HSML_REQ_GET_FB:
		HDEBUG("HSML_REQ_GET_FB \n");
		hsml_client_enable_fb_work(dev, true);
		schedule_read_work = 1;
		break;

	case HSML_REQ_STOP:
		flush_workqueue(wq_frame_reader);
		hsml_client_enable_fb_work(dev, false);
		break;

	case HSML_REQ_SET_SERVER_DISP:
	case HSML_REQ_SET_SERVER_CONF:
	case HSML_REQ_SET_MAX_FRAME_RATE:
		break;
#ifdef ADA_CUSTOMIZE
	case HSML_REQ_TOUCH_EVENT:
		break;
#endif /* ADA_CUSTOMIZE */
	default:
		pr_err("%s: Unidentified request code=%d\n",
			__func__, request->code);
		value = -EOPNOTSUPP;
	}

	if (value >= 0) {
#ifdef ADA_CUSTOMIZE
		if( request->code == HSML_REQ_TOUCH_EVENT ) {
			down_read(&dev->io_rwsem);
			pipe = usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr);
			HDEBUG("HSML_REQ_TOUCH_EVENT \n");
			value = usb_bulk_msg(dev->udev, pipe,request->data, value, &act_value, HSML_USB_TIMEOUT);
			if( value < 0 )
			{
				pr_err("%s: Failed to usb_bulk_msg value = %d \n",__func__ ,value);
			}
			up_read(&dev->io_rwsem);
		}else{
#endif /* ADA_CUSTOMIZE */
		if (request->dir == HSML_REQ_DIR_OUT) {
			pipe = usb_sndctrlpipe(dev->udev, 0);
			requesttype |= USB_DIR_OUT;
		} else {
			pipe = usb_rcvctrlpipe(dev->udev, 0);
			requesttype |= USB_DIR_IN;
		}

		value = usb_control_msg(dev->udev, pipe,
			request->code, requesttype, request->value,
			dev->interface->cur_altsetting->desc.bInterfaceNumber,
			request->data, value, USB_CTRL_GET_TIMEOUT);
		if (value < 0)
			pr_err("%s: Failed to send request 0x%02X. (err=%d)\n",
				__func__, request->code, value);
		else
			if (schedule_read_work) {
				dev->fb_reader_state = HSML_FB_READER_STATE_READ_HEADER;
				queue_work(wq_frame_reader, &dev->read_work);
			}
#ifdef ADA_CUSTOMIZE
		}
#endif /* ADA_CUSTOMIZE */
	}

exit:
	up_read(&dev->io_rwsem);
	return value;
}


/**
 * IOCTL for HSML client
 *
 */
static long hsml_client_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct usb_hsml_client *dev = (struct usb_hsml_client *)filp->private_data;
	int error = 1;
	int i;
	struct hsml_request request;
#ifdef ADA_CUSTOMIZE
	int index = 0;
#endif/* ADA_CUSTOMIZE */
	if (dev == NULL)
		return -ENODEV;

	down_read(&dev->io_rwsem);
	if (!dev->interface) {		/* disconnect() was called */
		error = -ENODEV;
		goto exit;
	}

	switch (cmd) {
	case HSMLCLIENT_IOC_READ_FRONT_FB_INDEX:
		if (dev->is_new) {
			hsml_client_swap_buffer(dev, HSML_MIDDLE_BUFFER, HSML_FRONT_BUFFER);
			dev->is_new = 0;
#ifdef ADA_CUSTOMIZE			
			index = dev->buf_index[dev->buf_num - 1];
			memcpy(dev->kmalloc_buf[index], dev->rx_urb_buf[index], dev->fb_info_data.framesize);
#endif /* ADA_CUSTOMIZE */
		}
		error = put_user(dev->buf_index[dev->buf_num - 1],
				(int __user *)arg);
		break;

	case HSMLCLIENT_IOC_WRITE_SERVER_FB_INFO:
		error = copy_from_user(&dev->server_info_data, (server_info_type __user *) arg, sizeof(server_info_type));
#ifdef ADA_CUSTOMIZE
		if (dev->server_info_data.pixel_format & HSML_PIXEL_FORMAT_RGB565)
		{
			dev->fb_info_data.framesize = (dev->server_info_data.height * dev->server_info_data.width) * 2;
		}
		else if(dev->server_info_data.pixel_format & HSML_PIXEL_FORMAT_RGB888)
		{
			dev->fb_info_data.framesize = (dev->server_info_data.height * dev->server_info_data.width) * 4;
		}
		else
		{
			pr_err("%s: Unsupported pixel format.\n", __func__);
			error = -EINVAL;
			goto exit;
		}
#else
		if (dev->server_info_data.pixel_format & PIXEL_FORMAT_RGB565)
			dev->fb_info_data.framesize = (dev->server_info_data.height * dev->server_info_data.width) * 2;
		else {
			pr_err("%s: Unsupported pixel format.\n", __func__);
			error = -EINVAL;
			goto exit;
		}
#endif /* ADA_CUSTOMIZE */
		dev->fb_info_data.height = dev->server_info_data.height;
		dev->fb_info_data.width = dev->server_info_data.width;

		for (i = 0; i < NUM_FB; i++) {
			if (!dev->rx_urb_buf[i]) {
			#if LINUX_VERSION_CODE > 0x020620
				dev->rx_urb_buf[i] = usb_alloc_coherent(dev->udev,
						dev->fb_info_data.framesize, GFP_KERNEL, &dev->rx_urb[i]->transfer_dma);
			#else
				dev->rx_urb_buf[i] = usb_buffer_alloc(dev->udev,
						dev->fb_info_data.framesize, GFP_KERNEL, &dev->rx_urb[i]->transfer_dma);
			#endif
				if (!dev->rx_urb_buf[i]) {
					pr_err("%s: Failed to allocate rx_urb_buf[%d].\n", __func__, i);
					error = -ENOMEM;
					break;
				}
#ifdef ADA_CUSTOMIZE
				dev->kmalloc_buf[i] = kmalloc(dev->fb_info_data.framesize, GFP_KERNEL);
				if (!dev->kmalloc_buf[i])
					pr_err("%s: Failed to allocate kmalloc_buf[%d].\n", __func__, i);
#endif /* ADA_CUSTOMIZE */
			}
		}
		break;

	case HSMLCLIENT_IOC_PROTOCOL_TRANSFER:
		error = copy_from_user(&request, (char *)arg, sizeof(struct hsml_request));
		if (!error) {
			error = hsml_client_handle_request(dev, &request);
			if (error >= 0 && request.dir == HSML_REQ_DIR_IN && request.len > 0) {
				error = copy_to_user((struct hsml_request *)arg, &request,
						sizeof(struct hsml_request));
				if (error)
					printk(KERN_ERR "%s: Failed to copy data to userspace. (err=%d)\n",
						__func__, error);
			}
		} else {
			pr_err("%s: Failed to copy data from userspace. (err=%d)\n",
				__func__, error);
		}
		break;

	default:
		pr_err("%s: err\n", __func__);
		error = -EINVAL;
		break;

	}

exit:
	up_read(&dev->io_rwsem);
	return error;
}

#ifndef HAVE_COMPAT_IOCTL
static int hsml_client_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	return hsml_client_compat_ioctl(filp, cmd, arg);
}
#endif /* HAVE_COMPAT_IOCTL */

/*
 * Map the framebuffer buffer to the address space of the calling process.
 */
static int hsml_client_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct usb_hsml_client *dev = (struct usb_hsml_client *)filp->private_data;
	unsigned long pfn;
#ifdef ADA_CUSTOMIZE
	unsigned long offset;
#if 1 /* ADA_CUSTOMIZE */
	printk("[HSML] hsml_client_mmap\n");
#endif/* ADA_CUSTOMIZE */
	offset = vma->vm_pgoff;
#endif /* ADA_CUSTOMIZE */
	vma->vm_flags |= VM_RESERVED;
#ifdef ADA_CUSTOMIZE
pfn = virt_to_phys((char *)dev->kmalloc_buf[offset]) >> PAGE_SHIFT;
#else
	pfn = virt_to_phys((char *)dev->rx_urb_buf[vma->vm_pgoff]) >> PAGE_SHIFT;
#endif /* ADA_CUSTOMIZE */
	if (remap_pfn_range(vma, vma->vm_start, pfn, vma->vm_end - vma->vm_start,
			vma->vm_page_prot))
#ifdef ADA_CUSTOMIZE
			{
				pr_err("%s: remap_pfn_range error.\n", __func__);
				return -EAGAIN;
			}
#else
		return -EAGAIN;
#endif /* ADA_CUSTOMIZE */

	return 0;
}


static const struct file_operations hsml_client_fops = {
	.owner =	THIS_MODULE,
	.open =		hsml_client_open,
	.release =	hsml_client_release,
	.flush =	hsml_client_flush,
	.mmap = 	hsml_client_mmap,
#ifdef HAVE_COMPAT_IOCTL
	.compat_ioctl = hsml_client_compat_ioctl,
#endif

#ifdef HAVE_UNLOCKED_IOCTL
	.unlocked_ioctl = hsml_client_compat_ioctl,
#else
	.ioctl =    hsml_client_ioctl
#endif
};


/*
 * usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with the driver core
 */
static struct usb_class_driver hsml_client_class = {
#ifdef ADA_CUSTOMIZE
	.name =		HSML_DRIVER_NAME,
#else
	.name =		"hsml_client",
#endif /* ADA_CUSTOMIZE */
	.fops =		&hsml_client_fops,
	.minor_base =	USB_HSML_MINOR_BASE,
};


/**
 * Called when the HSML server is connected
 *
 */
static int hsml_client_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct usb_hsml_client *dev;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	size_t buffer_size;
	int i;
	int retval = -ENOMEM;
#if 1 /* ADA_CUSTOMIZE */
	printk("[HSML] hsml_client_probe\n");
#endif/* ADA_CUSTOMIZE */
	/* allocate memory for our device state and initialize it */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		err("Out of memory");
		goto error;
	}

	kref_init(&dev->kref);
	init_rwsem(&dev->io_rwsem);
	spin_lock_init(&dev->err_lock);

	dev->rx_header_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->rx_header_urb) {
		printk(KERN_ERR "%s: Failed to allocate memory for rx_header_urb\n",
				__func__);
		goto error;
	}

	for (i = 0; i < NUM_FB; i++) {
		dev->rx_urb[i] = usb_alloc_urb(0, GFP_KERNEL);
		if (!dev->rx_urb[i]) {
			printk(KERN_ERR "%s: Failed to allocate memory for rx_urb[%d]\n",
					__func__, i);
			goto error;
		}
		dev->rx_urb_buf[i] = NULL;
		dev->buf_index[i] = i;
	}

	dev->buf_num = NUM_FB;

#ifdef ADA_CUSTOMIZE
	dev->tx_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->tx_urb) {
		printk(KERN_ERR "%s: Failed to allocate memory for tx_urb\n",
				__func__);
		goto error;
	}
#endif /* ADA_CUSTOMIZE */

	INIT_WORK(&dev->read_work, work_read_fb);
	spin_lock_init(&dev->buf_lock);
	init_completion(&dev->buf_comp);
	init_usb_anchor(&dev->read_submitted);
#ifdef ADA_CUSTOMIZE
	init_usb_anchor(&dev->send_submitted);
#endif /* ADA_CUSTOMIZE */
	dev->udev = usb_get_dev(interface_to_usbdev(interface));
	dev->interface = interface;

	/* set up the endpoint information */
	iface_desc = interface->cur_altsetting;
#ifdef ADA_CUSTOMIZE
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;
		if (!dev->bulk_in_endpointAddr && usb_endpoint_is_bulk_in(endpoint)) {
			/* we found a bulk in endpoint */
			buffer_size = le16_to_cpu(endpoint->wMaxPacketSize);
			dev->bulk_in_size = buffer_size;
			dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
			HDEBUG("%s, we found a usb_endpoint_is_bulk_in, address = %x\n", __func__,  endpoint->bEndpointAddress);
			continue;
		}
		else if( !dev->bulk_out_endpointAddr && usb_endpoint_is_bulk_out(endpoint))
		{
			/* we found a bulk out endpoint */
			buffer_size = le16_to_cpu(endpoint->wMaxPacketSize);
			dev->bulk_out_size = buffer_size;
			dev->bulk_out_endpointAddr = endpoint->bEndpointAddress;
			HDEBUG("%s, we found a usb_endpoint_is_bulk_out, address = %x\n", __func__,  endpoint->bEndpointAddress);
			continue;
		}
	}

	if (!dev->bulk_in_endpointAddr) {
		err("Could not find bulk-in endpoint");
		goto error;
	}else if(!dev->bulk_out_endpointAddr) {
		err("Could not find bulk-out endpoint");
		goto error;
	}
#else
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;
		if (!dev->bulk_in_endpointAddr &&
		    usb_endpoint_is_bulk_in(endpoint)) {
			/* we found a bulk in endpoint */
			buffer_size = le16_to_cpu(endpoint->wMaxPacketSize);
			dev->bulk_in_size = buffer_size;
			dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
			HDEBUG("%s, we found a usb_endpoint_is_bulk_in, address = %x\n", __func__,  endpoint->bEndpointAddress);
			continue;
		}
	}

	if (!dev->bulk_in_endpointAddr) {
		err("Could not find bulk-in endpoint");
		goto error;
	}
#endif /* ADA_CUSTOMIZE */
	/* save our data pointer in this interface device */
	usb_set_intfdata(interface, dev);

	/* we can register the device now, as it is ready */
	retval = usb_register_dev(interface, &hsml_client_class);
	if (retval) {
		/* something prevented us from registering this driver */
		err("Not able to get a minor for this device.");
		usb_set_intfdata(interface, NULL);
		goto error;
	}

	/* let the user know what node this device is now attached to */
	printk(KERN_INFO "%s: USB hsml client device now attached to USB\n",
			__func__);
#ifdef ADA_CUSTOMIZE_DEBUG
	HDEBUG("USB hsml client device #%d now attached to major %d minor %d \n",
		(interface->minor - USB_HSML_MINOR_BASE), USB_MAJOR, interface->minor);
#endif/* ADA_CUSTOMIZE_DEBUG */

	return 0;

error:
	if (dev)
		/* this frees allocated memory */
		kref_put(&dev->kref, hsml_client_delete);
	return retval;
}

/**
 * Called when the HSML server is disconnected
 *
 */
static void hsml_client_disconnect(struct usb_interface *interface)
{
	struct usb_hsml_client *dev;
#if 1 /* ADA_CUSTOMIZE */
	printk("[HSML] hsml_client_disconnect\n");
#endif/* ADA_CUSTOMIZE */
	dev = usb_get_intfdata(interface);

	cancel_work_sync(&dev->read_work);

	usb_set_intfdata(interface, NULL);

	/* give back our minor */
	usb_deregister_dev(interface, &hsml_client_class);

	/* prevent more I/O from starting */
	down_write(&dev->io_rwsem);
	dev->interface = NULL;
	up_write(&dev->io_rwsem);

	usb_kill_anchored_urbs(&dev->read_submitted);
#ifdef ADA_CUSTOMIZE
	usb_kill_anchored_urbs(&dev->send_submitted);
#endif /* ADA_CUSTOMIZE */
	/* decrement our usage count */
	kref_put(&dev->kref, hsml_client_delete);

	atomic_set(&recv_fb_enable, 0);

	printk(KERN_INFO "%s\n", __func__);
}

static void hsml_client_draw_down(struct usb_hsml_client *dev)
{
	int time;
#if 1 /* ADA_CUSTOMIZE */
	printk("[HSML] hsml_client_draw_down\n");
#endif/* ADA_CUSTOMIZE */
	time = usb_wait_anchor_empty_timeout(&dev->read_submitted, 1000);
	if (!time)
		usb_kill_anchored_urbs(&dev->read_submitted);
		
#ifdef ADA_CUSTOMIZE
	time = usb_wait_anchor_empty_timeout(&dev->send_submitted, 1000);
	if (!time)
		usb_kill_anchored_urbs(&dev->send_submitted);
#endif /* ADA_CUSTOMIZE */
}

static int hsml_client_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct usb_hsml_client *dev = usb_get_intfdata(intf);
#if 1 /* ADA_CUSTOMIZE */
	printk("[HSML] hsml_client_suspend\n");
#endif/* ADA_CUSTOMIZE */
	if (!dev)
		return 0;
	hsml_client_draw_down(dev);
	return 0;
}

static int hsml_client_resume(struct usb_interface *intf)
{
	return 0;
}

static int hsml_client_pre_reset(struct usb_interface *intf)
{
	struct usb_hsml_client *dev = usb_get_intfdata(intf);
#if 1 /* ADA_CUSTOMIZE */
	printk("[HSML] hsml_client_pre_reset\n");
#endif/* ADA_CUSTOMIZE */
	down_write(&dev->io_rwsem);
	hsml_client_draw_down(dev);

	return 0;
}

static int hsml_client_post_reset(struct usb_interface *intf)
{
	struct usb_hsml_client *dev = usb_get_intfdata(intf);
#if 1 /* ADA_CUSTOMIZE */
	printk("[HSML] hsml_client_post_reset\n");
#endif/* ADA_CUSTOMIZE */
	/* we are sure no URBs are active - no locking needed */
	dev->errors = -EPIPE;
	up_write(&dev->io_rwsem);

	return 0;
}

/* table of devices that work with this driver */
static struct usb_device_id hsml_client_table [] = {
	{ USB_INTERFACE_INFO(CLASS, SUBCLASS, PROTOCOL) },
	{ }					/* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, hsml_client_table);

static struct usb_driver hsml_client_driver = {
#ifdef ADA_CUSTOMIZE
	.name =		HSML_DRIVER_NAME,
#else
	.name =		"hsml_client",
#endif /* ADA_CUSTOMIZE */
	.probe =	hsml_client_probe,
	.disconnect =	hsml_client_disconnect,
	.suspend =	hsml_client_suspend,
	.resume =	hsml_client_resume,
	.pre_reset =	hsml_client_pre_reset,
	.post_reset =	hsml_client_post_reset,
	.id_table =	hsml_client_table,
	.supports_autosuspend = 1,
};

/**
 * Called when the HSML client module is inserted into kernel.
 *
 */
static int __init usb_hsml_client_init(void)
{
	int result;
#ifdef ADA_CUSTOMIZE
	struct device *class_dev = NULL;
#endif/* ADA_CUSTOMIZE */
#if 1 /* ADA_CUSTOMIZE */
	printk("[HSML] usb_hsml_client_init\n");
#endif/* ADA_CUSTOMIZE */
	wq_frame_reader = create_workqueue("hsml");
	if (!wq_frame_reader) {
		pr_err("%s: Frame reader workqueue init fail!\n", __func__);
		result = -ENOMEM;
		goto error;
	}

	/* register this driver with the USB subsystem */
	result = usb_register(&hsml_client_driver);
	if (result) {
		err("usb_register failed. Error number %d", result);
		goto error_usb_register;
	}

	atomic_set(&recv_fb_enable,0);

#ifdef ADA_CUSTOMIZE
	dev_id = MKDEV(0, 0);	// For initialization of dynamic allocation?
	
	result = alloc_chrdev_region(&dev_id, 0, hsml_devs, HSML_DRIVER_NAME);
	
	if (result) {
		err( "ERROR! alloc_chrdev_region\n");
		goto error;
	}
	
	hsml_major = MAJOR(dev_id);
	
	// Relation of method
	cdev_init(&hsml_cdev, &hsml_client_fops);
	hsml_cdev.owner = THIS_MODULE;
	hsml_cdev.ops   = &hsml_client_fops;
	
	result = cdev_add(&hsml_cdev, MKDEV(hsml_major, USB_HSML_MINOR_BASE), 1);
	if (result) {
		err( "ERROR! cdev_add\n");
		goto error;
	}
	
	// register class
	hsml_class = class_create(THIS_MODULE, HSML_CLASS_NAME);
	if (IS_ERR(hsml_class)) {
		err( "ERROR! class_create\n");
		result = PTR_ERR(hsml_class);
		goto error;
	}
	
	// register class device
	class_dev = device_create( hsml_class, NULL, MKDEV(hsml_major, USB_HSML_MINOR_BASE), NULL, "%s", HSML_DEVICE_NAME);
	
#endif/* ADA_CUSTOMIZE */
	return 0;

error_usb_register:
	destroy_workqueue(wq_frame_reader);
error:
	return result;
}

/**
 * Called when the HSML client module is removed from kernel.
 *
 */
static void __exit usb_hsml_client_exit(void)
{
#if 1 /* ADA_CUSTOMIZE */
	printk("[HSML] usb_hsml_client_exit\n");
#endif/* ADA_CUSTOMIZE */
	usb_deregister(&hsml_client_driver);

	if (wq_frame_reader)
		destroy_workqueue(wq_frame_reader);
}

module_init(usb_hsml_client_init);
module_exit(usb_hsml_client_exit);
MODULE_LICENSE("GPL");
