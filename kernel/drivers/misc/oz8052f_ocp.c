/**
 * @file oz8052f_ocp.c
 *
 * USB over-current protection driver
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

#include <linux/module.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/hrtimer.h>
#include <linux/earlydevice.h>
#include <linux/slab.h>
#include <linux/usb/ocp.h>
#include <linux/nonvolatile.h>
#include <linux/lites_trace_drv.h>

static struct poll_info {
	int	usbidx;
	int	valid;
	int	count;
	int	enable;
	int	pgood;
	int	btn;
} poll_prop[] = {
	{ USBOCP_USBOCP2, USBOCP_USB_DISABLE, 0, USBOCP2_EN, USBOCP2_PGOOD, USBOCP_USBOCP2},
	{ USBOCP_USBOCP3, USBOCP_USB_DISABLE, 0, USBOCP3_EN, USBOCP3_PGOOD, USBOCP_USBOCP3},
};

static struct usbocp_info {
	struct input_dev *in_dev;
	struct mutex lock;
	struct workqueue_struct *usbocp_wq;
	struct work_struct work;
	int	fast_shutdown;
	int	first_polling;
	int	polling_enable_mutex_lock;
	struct hrtimer timer;
	struct poll_info *ports;
	struct early_device edev;
	unsigned long polling_default;
	unsigned long polling_delay;
	unsigned char	count_over;
} *ocpdrv;

extern int set_usbocp_usben(int funcidx, int value);
extern int get_usbocp_pgood(int funcidx);
extern u8 get_refresh_status(void);

int oz8052f_early_resume(struct device* dev);
void oz8052f_early_suspend(struct device* dev);
static int oz8052f_resumeflag_atomic_callback(bool flag, void* param);

/**
 * @brief       USB over-current check timer start.
 * @param[in]   polling_span Polling interval. (msec)
 * @return      0 Normal return.
 */
static int usbocp_timer_start(unsigned long polling_span)
{
	int	rtn;

	USBOCP_DEBUG_LOG("ENTRY.\n!");
	rtn = hrtimer_start(&(ocpdrv->timer),
				ns_to_ktime(polling_span*1000*1000), HRTIMER_MODE_REL);
	USBOCP_DEBUG_LOG("EXIT.\n");
	return rtn;
}

/**
 * @brief       USB over-current check timer stop
 * @param       None.
 * @return      0 Normal return.
 */
static int usbocp_timer_stop(void)
{
	int	rtn;

	USBOCP_DEBUG_LOG("ENTRY.\n");
	rtn = hrtimer_cancel(&(ocpdrv->timer));
	USBOCP_DEBUG_LOG("EXIT.\n");
	return rtn;
}

/**
 * @brief       Sysfs interface for USB over-current information.
 * @param[in]   *dev USB over-current device.
 * @param[in]   *attr Device attribute structure.
 * @param[out]  *buf Show USB over-current data.
 * @return      Size of buf.
 */
static ssize_t oz8052f_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int i;
	int printed_size = 0;
	int show_size = 0;
	char *cur_buf = buf;
	size_t max_size = PAGE_SIZE;

	USBOCP_DEBUG_LOG("ENTRY.\n");
	for (i = 0; i < ARRAY_SIZE(poll_prop); i++) {
		printed_size = snprintf(cur_buf, max_size,
								"USBOCP:USB%d info.\n\tvalid=%d\n" \
								"\tcount=%d\n\tpgood=%d\n",
								poll_prop[i].usbidx,
								poll_prop[i].valid,
								poll_prop[i].count,
								get_usbocp_pgood(poll_prop[i].pgood));
		if (printed_size <= 0) {
			USBOCP_DEBUG_LOG("ERR EXIT(%d).\n", printed_size);
			return printed_size;
		}
		show_size += printed_size;
		max_size -= printed_size;
		cur_buf = buf + show_size;
	}

	USBOCP_DEBUG_LOG("EXIT(%d).\n", show_size);
	return show_size;
}
static DEVICE_ATTR(usbocp, S_IRUGO | S_IWUSR | S_IWGRP, oz8052f_show, NULL);

/**
 * @brief       USB over-current monitoring Resume.
 * @param[in]   *timer Timer information.
 * @return      HRTIMER_NORESTART.
 */
static enum hrtimer_restart usbocp_timer_func(struct hrtimer *timer)
{
	struct usbocp_info *pusbocp_info;

	USBOCP_DEBUG_LOG("ENTRY.\n");
	pusbocp_info = container_of(timer, struct usbocp_info, timer);
	queue_work(pusbocp_info->usbocp_wq, &pusbocp_info->work);
	USBOCP_DEBUG_LOG("EXIT.\n");
	return HRTIMER_NORESTART;
}

/**
 * @brief       USB over-current action.
 * @param[in]   *in_dev Input device.
 * @param[in]   *port USB port information.
 * @param[in]   val Not used.
 * @return      None.
 */
static void usbocp_check_current_over(struct input_dev *in_dev,
								struct poll_info *port, int val)
{
	USBOCP_DEBUG_LOG("ENTRY.\n");
	if (val) {
		USBOCP_DEBUG_LOG("USB%02d:Overcurrent recovery\n",
		                  port->usbidx );
		port->count = 0; /* counter reset */
		goto end;;
	}

	/* detect current count up */
	port->count++;
	USBOCP_INFO_LOG("USB%02d:Overcurrent detect %d\n",
	                port->usbidx, port->count );

	if ( ocpdrv->count_over <= port->count) {
		USBOCP_ERR_LOG("USBOCP:USB%02d:Overcurrent. USB DISABLE.\n", port->usbidx);
		set_usbocp_usben(port->enable, USBOCP_USB_DISABLE);
		input_event(in_dev, EV_MSC, MSC_SCAN, port->btn);
		input_sync(in_dev);
		port->valid = USBOCP_USB_DISABLE;
		USBOCP_DRC_HARD_ERR( USBOCP_ERRORID_OVERCURRENT, port->usbidx,
			port->valid, 0, 0 );
	}
end:
	USBOCP_DEBUG_LOG("EXIT.\n");
}


/**
 * @brief       Disable in USB port
 * @param[in]   *port USB port information.
 * @return      None.
 */
static void usbocp_disable_usb_en(struct poll_info *port)
{
	USBOCP_DEBUG_LOG("ENTRY.\n");
	if (port->enable) {
		set_usbocp_usben(port->enable, USBOCP_USB_DISABLE);
		USBOCP_DEBUG_LOG("USB%02d:USB DISABLE.\n", port->usbidx);
	}
	USBOCP_DEBUG_LOG("EXIT.\n");
}


/**
 * @brief       USB over-current timer function.(work queue)
 * @param[in]   *work work queue..
 * @return      None.
 */
static void usbocp_work_func(struct work_struct *work)
{
	int i, pgood_value, valid_something = 0;

	USBOCP_DEBUG_LOG("ENTRY.\n");
	mutex_lock(&ocpdrv->lock);
	if (ocpdrv->first_polling) {
		ocpdrv->first_polling = USBOCP_POLLING_STOP;
		if( get_refresh_status() ){
			USBOCP_NOTICE_LOG("USBOCP:USB port intialized\n");
			USBOCP_DRC_TRC("USBOCP:USB port intialized\n");
			for (i = 0; i < ARRAY_SIZE(poll_prop); i++) {
				set_usbocp_usben(poll_prop[i].enable, USBOCP_USB_ENABLE);
				poll_prop[i].count = 0;
				poll_prop[i].valid = USBOCP_USB_ENABLE;
				USBOCP_NOTICE_LOG("USBOCP:USB%02d:USB ENABLE\n",poll_prop[i].usbidx);
			}
		} else {
			USBOCP_NOTICE_LOG("USBOCP:USB power supply stop for refresh mode.\n");
			USBOCP_DRC_TRC("USBOCP:USB power supply stop for refresh mode.\n");
			for (i = 0; i < ARRAY_SIZE(poll_prop); i++) {
				poll_prop[i].count = 0;
				poll_prop[i].valid = USBOCP_USB_DISABLE;
			}
			ocpdrv->polling_enable_mutex_lock = USBOCP_POLLING_DISABLED;
			mutex_unlock(&ocpdrv->lock);
			return;
		}
	}

	for (i = 0; i < ARRAY_SIZE(poll_prop); i++) {
		if (poll_prop[i].valid){
			pgood_value = get_usbocp_pgood(poll_prop[i].pgood);
			if (pgood_value < 0) {
				USBOCP_ERR_LOG("USBOCP:USB%02d:gpio read error. %d.\n",
				               poll_prop[i].usbidx, pgood_value);
				continue;
			}
			USBOCP_DEBUG_LOG("USB%02d:PGOOD=%d\n",
			                  poll_prop[i].usbidx, pgood_value);
			usbocp_check_current_over(ocpdrv->in_dev, &poll_prop[i],
										pgood_value);
			valid_something += poll_prop[i].valid;
		}
	}

	if (!valid_something) {
		ocpdrv->polling_enable_mutex_lock = USBOCP_POLLING_DISABLED;
		USBOCP_NOTICE_LOG("USBOCP:All usb is overcurrent. polling stop.\n");
	}

	if (ocpdrv->polling_enable_mutex_lock && !ocpdrv->fast_shutdown) {
		usbocp_timer_start(ocpdrv->polling_default);
		USBOCP_DEBUG_LOG("RESTART TIMER.\n");
	}
	else {
		USBOCP_DEBUG_LOG("POLLING STOPPED.\n");
		for (i = 0; i < ARRAY_SIZE(poll_prop); i++) {
			set_usbocp_usben(poll_prop[i].enable, USBOCP_USB_DISABLE);
			USBOCP_DEBUG_LOG("USB%02d:USB DISABLE.\n", poll_prop[i].usbidx);
		}
	}
	mutex_unlock(&ocpdrv->lock);
	USBOCP_DEBUG_LOG("EXIT.\n");
}


/**
 * @brief       USB over-current driver resume.
 * @param[in]   *dev USB over-current device.
 * @return      0 normal return.
 */
int oz8052f_early_resume(struct device* dev)
{
	USBOCP_DEBUG_LOG("ENTRY.\n");
	if (ocpdrv) {
		cancel_work_sync(&ocpdrv->work);
		early_device_invoke_with_flag_irqsave(
		    EARLY_DEVICE_BUDET_INTERRUPTION_MASKED,
		    oz8052f_resumeflag_atomic_callback, ocpdrv);
		if( ocpdrv->fast_shutdown == USBOCP_FAST_SHUTDOWN ) {
			USBOCP_NOTICE_LOG("USBOCP:re_init called in bu-det\n");
			return 0;
		}

		mutex_lock(&ocpdrv->lock);
		ocpdrv->polling_enable_mutex_lock = USBOCP_POLLING_ENABLED;
		ocpdrv->first_polling = USBOCP_POLLING_BUSY;
		usbocp_timer_start(ocpdrv->polling_delay);
		mutex_unlock(&ocpdrv->lock);
	}
	USBOCP_NOTICE_LOG("USBOCP:resume complete.\n");
	USBOCP_DEBUG_LOG("EXIT.\n");
	return 0;
}

/**
 * @brief       USB over-current driver suspend.
 * @param[in]   *dev USB over-current device.
 * @return      0 Normal return.
 */
void oz8052f_early_suspend(struct device* dev)
{
	USBOCP_DEBUG_LOG("ENTRY.\n");
	if (ocpdrv) {
		ocpdrv->fast_shutdown = USBOCP_FAST_SHUTDOWN;
	}
	USBOCP_INFO_LOG("USBOCP:suspend call.\n");
	USBOCP_DEBUG_LOG("EXIT.\n");
}

/**
 * @brief       Check the resume flag of resume.
 * @param[in]   flag Suspend flag.
 * @param[in]   *param USB device device information.
 * @return      0 Normal return.
 */
static int oz8052f_resumeflag_atomic_callback(bool flag, void* param)
{
	struct usbocp_info *data = (struct usbocp_info *) param;
	USBOCP_DEBUG_LOG("ENTRY.\n");
	if( flag == false ) {
		data->fast_shutdown = USBOCP_RUNNING;
	}
	USBOCP_DEBUG_LOG("EXIT.\n");
	return 0;
}

/**
 * @brief       USB over-current device open function.
 * @param[in]   *dev Input device.
 * @return      0 Normal return.
 */
static int oz8052f_input_open(struct input_dev *dev)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(poll_prop); i++) {
        if (poll_prop[i].valid == USBOCP_USB_DISABLE){
            input_event(dev, EV_MSC, MSC_SCAN, poll_prop[i].btn);
            input_sync(dev);
        }
    }
    return 0;
}

/**
 * @brief       USB over-current driver initialize
 * @param       None.
 * @retval      0 Normal return.
 * @retval      -1 Abnormal return.
 */
static int __init oz8052f_init(void)
{
	int err = 0;
	unsigned char polling_default = 0;
	unsigned char polling_delay = 0;

	USBOCP_DEBUG_LOG("ENTRY.\n");
	ocpdrv = kzalloc(sizeof(struct usbocp_info), GFP_KERNEL);
	if (ocpdrv == NULL) {
		USBOCP_ERR_LOG("USBOCP:memory allocation failed.\n");
		err = -ENOMEM;
		goto exit;
	}

	// Nonvolatile data access
	if( ( GetNONVOLATILE( &ocpdrv->count_over,
	                      FTEN_NONVOL_USBOCP_POLL_THRESHOLD,
	                      FTEN_NONVOL_USBOCP_POLL_THRESHOLD_SIZE ) != 0 ) ||
	    ( ocpdrv->count_over < THRESHOLD_USB_CURRENT_OVER_MIN ) ){
			USBOCP_ERR_LOG("USBOCP:overcurrent is default\n");
			ocpdrv->count_over = THRESHOLD_USB_CURRENT_OVER;
	}
	if( ( GetNONVOLATILE( &polling_default,
	                      FTEN_NONVOL_USBOCP_POLL_DEFAULT,
	                      FTEN_NONVOL_USBOCP_POLL_DEFAULT_SIZE ) != 0 ) ||
	    ( polling_default < USBOCP_POLLING_INTERVAL_MIN ) ) {
		USBOCP_ERR_LOG("USBOCP:Polling time is default\n");
		ocpdrv->polling_default = USBOCP_POLLING_INTERVAL;
	} else {
		ocpdrv->polling_default = (unsigned long)polling_default * 10;
	}
	if( ( GetNONVOLATILE( &polling_delay,
	                      FTEN_NONVOL_USBOCP_POLL_DELAY,
	                      FTEN_NONVOL_USBOCP_POLL_DELAY_SIZE ) != 0 ) ||
	    ( polling_delay < USBOCP_START_POLLING_INTERVAL_MIN ) ) {
		USBOCP_ERR_LOG("USBOCP:First polling time is default\n");
		ocpdrv->polling_delay = USBOCP_START_POLLING_INTERVAL;
	} else {
		ocpdrv->polling_delay = (unsigned long)polling_delay * 10;
	}
	USBOCP_NOTICE_LOG("USBOCP:Count of overcurrent is %d times\n",ocpdrv->count_over);
	USBOCP_NOTICE_LOG("USBOCP:Polling time is %ldmsec\n",ocpdrv->polling_default);
	USBOCP_NOTICE_LOG("USBOCP:First Polling time is %ldmsec\n",ocpdrv->polling_delay);

	ocpdrv->edev.reinit = oz8052f_early_resume;
	ocpdrv->edev.sudden_device_poweroff = oz8052f_early_suspend;
	ocpdrv->edev.dev = NULL;
	err = early_device_register(&ocpdrv->edev);
	if( err != 0 ) {
		USBOCP_ERR_LOG("USBOCP:could not early device register.\n");
		goto err_early_device_register;
	}

	mutex_init(&ocpdrv->lock);

	ocpdrv->in_dev = input_allocate_device();
	if (!ocpdrv->in_dev) {
		USBOCP_ERR_LOG("USBOCP:could not allocate input device.\n");
		err = -ENOMEM;
		goto err_input_allocate_device;
	}
	USBOCP_DEBUG_LOG("PASS input_allocate_device().\n");

	ocpdrv->ports = poll_prop;

	input_set_drvdata(ocpdrv->in_dev, ocpdrv);
	(ocpdrv->in_dev)->name = USB_OVER_CURRENT_PROTECTION;

	input_set_capability(ocpdrv->in_dev, EV_MSC, MSC_SCAN);
	USBOCP_DEBUG_LOG("PASS setting static values.\n");

	ocpdrv->in_dev->open = oz8052f_input_open;

	err = input_register_device(ocpdrv->in_dev);
	if (err < 0) {
		USBOCP_ERR_LOG("USBOCP:could not register input device.\n");
		goto err_input_register_device;
	}
	USBOCP_DEBUG_LOG("PASS input_register_device().\n");

	hrtimer_init(&ocpdrv->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	ocpdrv->timer.function = usbocp_timer_func;

	ocpdrv->usbocp_wq =
				create_singlethread_workqueue(USB_OVER_CURRENT_PROTECTION);
	if (!ocpdrv->usbocp_wq) {
		err = -ENOMEM;
		USBOCP_ERR_LOG("USBOCP:could not create workqueue.\n");
		goto err_create_workqueue;
	}
	INIT_WORK(&ocpdrv->work, usbocp_work_func);
	USBOCP_DEBUG_LOG("PASS create_singlethread_workqueue().\n");

	if (device_create_file(&(ocpdrv->in_dev)->dev, &dev_attr_usbocp) < 0) {
		err = -ENOMEM;
		USBOCP_ERR_LOG("USBOCP:failed to create device file(%s).\n",
		               dev_attr_usbocp.attr.name);
		goto err_device_create_file;
	}

	mutex_lock(&ocpdrv->lock);
	ocpdrv->polling_enable_mutex_lock = USBOCP_POLLING_ENABLED;
	ocpdrv->first_polling = USBOCP_POLLING_BUSY;

	usbocp_timer_start(ocpdrv->polling_delay);

	mutex_unlock(&ocpdrv->lock);
	USBOCP_INFO_LOG("INITIALIZED.\n");
	return 0;

err_device_create_file:
	destroy_workqueue(ocpdrv->usbocp_wq);
err_create_workqueue:
	input_unregister_device(ocpdrv->in_dev);
err_input_register_device:
	input_free_device(ocpdrv->in_dev);
err_input_allocate_device:
	early_device_unregister(&ocpdrv->edev);
err_early_device_register:
	kfree(ocpdrv);
exit:
	USBOCP_ERR_LOG("USBOCP:INITIALIZE FAILED:(%d).\n", err);
	return err;
}
module_init(oz8052f_init);

/**
 * @brief       USB over-current driver finish
 * @param       None.
 * @return      None.
 */
static void __exit oz8052f_exit(void)
{
	int i;

	USBOCP_DEBUG_LOG("ENTRY.\n");
	if (ocpdrv) {
		early_device_unregister(&ocpdrv->edev);
		mutex_lock(&ocpdrv->lock);
		for (i = 0; i < ARRAY_SIZE(poll_prop); i++) {
			usbocp_disable_usb_en(&poll_prop[i]);
		}
		ocpdrv->polling_enable_mutex_lock = USBOCP_POLLING_DISABLED;
		usbocp_timer_stop();
		mutex_unlock(&ocpdrv->lock);
		USBOCP_DEBUG_LOG("timer stopped.\n");
		if (ocpdrv->usbocp_wq) {
			cancel_work_sync(&ocpdrv->work);
			destroy_workqueue(ocpdrv->usbocp_wq);
			USBOCP_DEBUG_LOG("PASS destroy_workqueue().\n");
		}
		if (ocpdrv->in_dev) {
			input_unregister_device(ocpdrv->in_dev);
			input_free_device(ocpdrv->in_dev);
			USBOCP_DEBUG_LOG("PASS input_unregister_device().\n");
		}

		mutex_destroy(&ocpdrv->lock);
		USBOCP_DEBUG_LOG("PASS mutex_destroy().\n");
		kfree(ocpdrv);
		USBOCP_DEBUG_LOG("PASS kfree().\n");
	}
	else {
		USBOCP_DEBUG_LOG("FAILED.\n");
	}
	USBOCP_DEBUG_LOG("EXIT.\n");
}
module_exit(oz8052f_exit);

MODULE_DESCRIPTION("oz8052f over current protection driver");
MODULE_LICENSE("GPL");
