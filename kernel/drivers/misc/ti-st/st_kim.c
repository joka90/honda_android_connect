/*
 *  Shared Transport Line discipline driver Core
 *	Init Manager module responsible for GPIO control
 *	and firmware download
 *  Copyright (C) 2009-2010 Texas Instruments
 *  Author: Pavan Savoy <pavan_savoy@ti.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*----------------------------------------------------------------------------*/
// COPYRIGHT(C) FUJITSU TEN LIMITED 2012,2013
/*----------------------------------------------------------------------------*/

#define pr_fmt(fmt) "(stk) :" fmt
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/sysfs.h>
#include <linux/tty.h>

#include <linux/skbuff.h>
#include <linux/ti_wilink_st.h>
#include <linux/serial_core.h>

/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
#include <linux/earlydevice.h>
#include <linux/budet_interruptible.h>
#endif /* #ifdef CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
#include <linux/nonvolatile.h>
#include <linux/nonvolatile_def.h>
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */

/* FUJITSU TEN :2013-08-29 Log HCI_HardErr start */
#define HCI_HARDERR_BACKUP  6
#define HCI_HARDERR_CMDMAX  300
/* FUJITSU TEN :2013-08-29 Log HCI_HardErr end */
#define MAX_ST_DEVICES	3	/* Imagine 1 on each UART for now */
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
#define SLEEP_BT_ON_BTEN_OFF   1200   //0x175
#define SLEEP_BT_ON_BTEN_ON    10     //0x176
#define SLEEP_BT_OFF_BTEN_OFF  10     //0x177
#define SLEEP_BT_OFF_BTEN_ON   10     //0x178
#define WAIT_BAUDRATE_STABLE   100
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe start */
#define NONVOL_VALCHK(val) (val!=0x00 && val!=0xFF)
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe end */

static struct platform_device *st_kim_devices[MAX_ST_DEVICES];
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
/* FUJITSU TEN :2012-12-12 Support_DeviceReady_Flag start */
//static volatile int st_device_ready = 0;
static volatile int st_device_ready = 1;
/* FUJITSU TEN :2012-12-12 Support_DeviceReady_Flag end */
#endif /* #ifdef CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
static unsigned int sleep_bt_on_bten_off  = SLEEP_BT_ON_BTEN_OFF;
static unsigned int sleep_bt_on_bten_on   = SLEEP_BT_ON_BTEN_ON;
static unsigned int sleep_bt_off_bten_off = SLEEP_BT_OFF_BTEN_OFF;
static unsigned int sleep_bt_off_bten_on  = SLEEP_BT_OFF_BTEN_ON;
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */


/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
int st_is_device_ready() {
	return st_device_ready;
}
EXPORT_SYMBOL(st_is_device_ready);

int st_update_flag(bool flag, void* param) {
	int retval = -EINTR;
	if (!flag) {
		ADA_LOG_ST_INFO("device_ready (%d -> 1)", st_device_ready);
		DRC_ST_EVT_DBG(EVT_BUDET+1, st_device_ready);
		st_device_ready = 1;
		retval = 0;
	}
/* FUJITSU TEN :2013-02-21 DriveRec start */
	DRC_ST_DRVTRACE("st_update_flag:%d %d %d", flag, retval, st_device_ready);
/* FUJITSU TEN :2013-02-21 DriveRec end */
	return retval;
}

int st_up_device_ready(void) {
	int retval = early_device_invoke_with_flag_irqsave(
		RNF_BUDET_ACCOFF_INTERRUPT_MASK, st_update_flag, NULL);
	ADA_LOG_ST_INFO("retval=%d", retval);
	DRC_ST_EVT_DBG(EVT_BUDET+2, retval);
	return retval;
}
/* FUJITSU TEN :2012-12-12 Support_DeviceReady_Flag start */
EXPORT_SYMBOL(st_up_device_ready);
/* FUJITSU TEN :2012-12-12 Support_DeviceReady_Flag end */
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */

/**********************************************************************/
/* internal functions */

/**
 * st_get_plat_device -
 *	function which returns the reference to the platform device
 *	requested by id. As of now only 1 such device exists (id=0)
 *	the context requesting for reference can get the id to be
 *	requested by a. The protocol driver which is registering or
 *	b. the tty device which is opened.
 */
static struct platform_device *st_get_plat_device(int id)
{
	return st_kim_devices[id];
}

/**
 * validate_firmware_response -
 *	function to return whether the firmware response was proper
 *	in case of error don't complete so that waiting for proper
 *	response times out
 */
void validate_firmware_response(struct kim_data_s *kim_gdata)
{
	struct sk_buff *skb = kim_gdata->rx_skb;
	if (unlikely(skb->data[5] != 0)) {
		pr_err("no proper response during fw download");
		pr_err("data6 %x", skb->data[5]);
		kfree_skb(skb);
		return;		/* keep waiting for the proper response */
	}
	/* becos of all the script being downloaded */
	complete_all(&kim_gdata->kim_rcvd);
	kfree_skb(skb);
}

/* check for data len received inside kim_int_recv
 * most often hit the last case to update state to waiting for data
 */
static inline int kim_check_data_len(struct kim_data_s *kim_gdata, int len)
{
	register int room = skb_tailroom(kim_gdata->rx_skb);

	pr_debug("len %d room %d", len, room);

	if (!len) {
		validate_firmware_response(kim_gdata);
	} else if (len > room) {
		/* Received packet's payload length is larger.
		 * We can't accommodate it in created skb.
		 */
		pr_err("Data length is too large len %d room %d", len,
			   room);
		kfree_skb(kim_gdata->rx_skb);
	} else {
		/* Packet header has non-zero payload length and
		 * we have enough space in created skb. Lets read
		 * payload data */
		kim_gdata->rx_state = ST_W4_DATA;
		kim_gdata->rx_count = len;
		return len;
	}

	/* Change ST LL state to continue to process next
	 * packet */
	kim_gdata->rx_state = ST_W4_PACKET_TYPE;
	kim_gdata->rx_skb = NULL;
	kim_gdata->rx_count = 0;

	return 0;
}

/**
 * kim_int_recv - receive function called during firmware download
 *	firmware download responses on different UART drivers
 *	have been observed to come in bursts of different
 *	tty_receive and hence the logic
 */
void kim_int_recv(struct kim_data_s *kim_gdata,
	const unsigned char *data, long count)
{
	const unsigned char *ptr;
	int len = 0, type = 0;
	unsigned char *plen;

	pr_debug("%s", __func__);
	/* Decode received bytes here */
	ptr = data;
	if (unlikely(ptr == NULL)) {
		pr_err(" received null from TTY ");
		return;
	}

	while (count) {
		if (kim_gdata->rx_count) {
			len = min_t(unsigned int, kim_gdata->rx_count, count);
			memcpy(skb_put(kim_gdata->rx_skb, len), ptr, len);
			kim_gdata->rx_count -= len;
			count -= len;
			ptr += len;

			if (kim_gdata->rx_count)
				continue;

			/* Check ST RX state machine , where are we? */
			switch (kim_gdata->rx_state) {
				/* Waiting for complete packet ? */
			case ST_W4_DATA:
				pr_debug("Complete pkt received");
				validate_firmware_response(kim_gdata);
				kim_gdata->rx_state = ST_W4_PACKET_TYPE;
				kim_gdata->rx_skb = NULL;
				continue;
				/* Waiting for Bluetooth event header ? */
			case ST_W4_HEADER:
				plen =
				(unsigned char *)&kim_gdata->rx_skb->data[1];
				pr_debug("event hdr: plen 0x%02x\n", *plen);
				kim_check_data_len(kim_gdata, *plen);
				continue;
			}	/* end of switch */
		}		/* end of if rx_state */
		switch (*ptr) {
			/* Bluetooth event packet? */
		case 0x04:
			kim_gdata->rx_state = ST_W4_HEADER;
			kim_gdata->rx_count = 2;
			type = *ptr;
			/* FUJITSU TEN :2013-08-29 Log HCI_HardErr start */
			if (unlikely(ptr[1]==0x10 && ptr[2]==0x01)) {
				pr_err("HCI HardErr 0x%02x ",ptr[3]);
			}
			/* FUJITSU TEN :2013-08-29 Log HCI_HardErr end */
			break;
		default:
			pr_info("unknown packet");
			ptr++;
			count--;
			continue;
		}
		ptr++;
		count--;
		kim_gdata->rx_skb =
			alloc_skb(1024+8, GFP_ATOMIC);
		if (!kim_gdata->rx_skb) {
			pr_err("can't allocate mem for new packet");
			kim_gdata->rx_state = ST_W4_PACKET_TYPE;
			kim_gdata->rx_count = 0;
			return;
		}
		skb_reserve(kim_gdata->rx_skb, 8);
		kim_gdata->rx_skb->cb[0] = 4;
		kim_gdata->rx_skb->cb[1] = 0;

	}
	return;
}

static long read_local_version(struct kim_data_s *kim_gdata, char *bts_scr_name)
{
	unsigned short version = 0, chip = 0, min_ver = 0, maj_ver = 0;
	const char read_ver_cmd[] = { 0x01, 0x01, 0x10, 0x00 };
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
	long remain;
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */

	pr_debug("%s", __func__);

/* FUJITSU TEN :2012-12-11 Fix Nvidia_Bluetooth_Bug start */
#ifdef CONFIG_TI_FASTBOOT
	remain = mdelay_budet_interruptible(WAIT_BAUDRATE_STABLE);
	if (remain) {
		ADA_LOG_ST_TRACE("Budet Interrupt occurred. remain :%ld", remain);
		DRC_ST_EVT(EVT_BUDET+3, remain);
		return -EINTR;
	}
#else /* CONFIG_TI_FASTBOOT */
	mdelay(WAIT_BAUDRATE_STABLE);
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-12-11 Fix Nvidia_Bluetooth_Bug end */

	INIT_COMPLETION(kim_gdata->kim_rcvd);
	if (4 != st_int_write(kim_gdata->core_data, read_ver_cmd, 4)) {
		pr_err("kim: couldn't write 4 bytes");
		return -EIO;
	}
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
	remain = wait_for_completion_budet_interruptible_timeout
			(&kim_gdata->kim_rcvd, msecs_to_jiffies(CMD_RESP_TIME));
	if (remain == -ERESTARTSYS) {
		ADA_LOG_ST_TRACE("Budet Interrput occurred. err :%ld", remain);
		DRC_ST_EVT(EVT_BUDET+4, remain);
		return -EINTR;
	} else if (!remain) {
#else /* CONFIG_TI_FASTBOOT */
        if (!wait_for_completion_timeout
		(&kim_gdata->kim_rcvd, msecs_to_jiffies(CMD_RESP_TIME))) {
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
		pr_err(" waiting for ver info- timed out ");
		return -ETIMEDOUT;
	}
	INIT_COMPLETION(kim_gdata->kim_rcvd);

	version =
		MAKEWORD(kim_gdata->resp_buffer[13],
				kim_gdata->resp_buffer[14]);
	chip = (version & 0x7C00) >> 10;
	min_ver = (version & 0x007F);
	maj_ver = (version & 0x0380) >> 7;

	if (version & 0x8000)
		maj_ver |= 0x0008;

	sprintf(bts_scr_name, "TIInit_%d.%d.%d.bts", chip, maj_ver, min_ver);

	/* to be accessed later via sysfs entry */
	kim_gdata->version.full = version;
	kim_gdata->version.chip = chip;
	kim_gdata->version.maj_ver = maj_ver;
	kim_gdata->version.min_ver = min_ver;

	pr_info("%s", bts_scr_name);
	return 0;
}

void skip_change_remote_baud(unsigned char **ptr, long *len)
{
	unsigned char *nxt_action, *cur_action;
	cur_action = *ptr;

	nxt_action = cur_action + sizeof(struct bts_action) +
		((struct bts_action *) cur_action)->size;

	if (((struct bts_action *) nxt_action)->type != ACTION_WAIT_EVENT) {
		pr_err("invalid action after change remote baud command");
	} else {
		*ptr = *ptr + sizeof(struct bts_action) +
			((struct bts_action *)cur_action)->size;
		*len = *len - (sizeof(struct bts_action) +
				((struct bts_action *)cur_action)->size);
		/* warn user on not commenting these in firmware */
		pr_warn("skipping the wait event of change remote baud");
	}
}

/**
 * download_firmware -
 *	internal function which parses through the .bts firmware
 *	script file intreprets SEND, DELAY actions only as of now
 */
static long download_firmware(struct kim_data_s *kim_gdata)
{
	long err = 0;
	long len = 0;
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
	unsigned long remain;
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
	unsigned char *ptr = NULL;
	unsigned char *action_ptr = NULL;
	unsigned char bts_scr_name[30] = { 0 };	/* 30 char long bts scr name? */
	int wr_room_space;
	int cmd_size;
	unsigned long timeout;
	/* FUJITSU TEN :2013-08-29 Log HCI_HardErr start */
	int i;
	int print_index = 0;
	unsigned char *precommand[HCI_HARDERR_BACKUP];
	int precommand_size[HCI_HARDERR_BACKUP];

	for (i=0; i<HCI_HARDERR_BACKUP; i++) {
		precommand[i] = NULL;
		precommand_size[i] = 0;
	}
	/* FUJITSU TEN :2013-08-29 Log HCI_HardErr end */

	err = read_local_version(kim_gdata, bts_scr_name);
	if (err != 0) {
		pr_err("kim: failed to read local ver");
		return err;
	}
	err =
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
		request_firmware_budet_interruptible(
			&kim_gdata->fw_entry, bts_scr_name,
			&kim_gdata->kim_pdev->dev);
	if (unlikely(err == -ERESTARTSYS)) {
		ADA_LOG_ST_TRACE("Bu-Det interrupted.");
		DRC_ST_EVT(EVT_BUDET+5, err);
		return err;
	}
#else
	    request_firmware(&kim_gdata->fw_entry, bts_scr_name,
			     &kim_gdata->kim_pdev->dev);
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */

	if (unlikely((err != 0) || (kim_gdata->fw_entry->data == NULL) ||
		     (kim_gdata->fw_entry->size == 0))) {
		pr_err(" request_firmware failed(errno %ld) for %s", err,
			   bts_scr_name);
		return -EINVAL;
	}
	ptr = (void *)kim_gdata->fw_entry->data;
	len = kim_gdata->fw_entry->size;
	/* bts_header to remove out magic number and
	 * version
	 */
	ptr += sizeof(struct bts_header);
	len -= sizeof(struct bts_header);

	while (len > 0 && ptr) {
		pr_debug(" action size %d, type %d ",
			   ((struct bts_action *)ptr)->size,
			   ((struct bts_action *)ptr)->type);

		switch (((struct bts_action *)ptr)->type) {
		case ACTION_SEND_COMMAND:	/* action send */
			pr_debug("S");
			action_ptr = &(((struct bts_action *)ptr)->data[0]);
			if (unlikely
			    (((struct hci_command *)action_ptr)->opcode ==
			     0xFF36)) {
				/* ignore remote change
				 * baud rate HCI VS command */
				pr_warn("change remote baud"
				    " rate command in firmware");
				skip_change_remote_baud(&ptr, &len);
				break;
			}
			/*
			 * Make sure we have enough free space in uart
			 * tx buffer to write current firmware command
			 */
			cmd_size = ((struct bts_action *)ptr)->size;
			timeout = jiffies + msecs_to_jiffies(CMD_WR_TIME);
			do {
				wr_room_space =
					st_get_uart_wr_room(kim_gdata->core_data);
				if (wr_room_space < 0) {
					pr_err("Unable to get free "
							"space info from uart tx buffer");
					release_firmware(kim_gdata->fw_entry);
					return wr_room_space;
				}
                mdelay(1); /* wait 1ms before checking room */
			} while ((wr_room_space < cmd_size) &&
					time_before(jiffies, timeout));

			/* Timeout happened ? */
			if (time_after_eq(jiffies, timeout)) {
				pr_err("Timeout while waiting for free "
						"free space in uart tx buffer");
				release_firmware(kim_gdata->fw_entry);
				return -ETIMEDOUT;
			}
			/* reinit completion before sending for the
			 * relevant wait
			 */
			INIT_COMPLETION(kim_gdata->kim_rcvd);

			/*
			 * Free space found in uart buffer, call st_int_write
			 * to send current firmware command to the uart tx
			 * buffer.
			 */
			err = st_int_write(kim_gdata->core_data,
			((struct bts_action_send *)action_ptr)->data,
					   ((struct bts_action *)ptr)->size);
			if (unlikely(err < 0)) {
				release_firmware(kim_gdata->fw_entry);
				return err;
			}
			/*
			 * Check number of bytes written to the uart tx buffer
			 * and requested command write size
			 */
			if (err != cmd_size) {
				pr_err("Number of bytes written to uart "
						"tx buffer are not matching with "
						"requested cmd write size");
				release_firmware(kim_gdata->fw_entry);
				return -EIO;
			}

			/* FUJITSU TEN :2013-08-29 Log HCI_HardErr start */
			for (i=HCI_HARDERR_BACKUP-1; i>=1; i--) {
				precommand[i] = precommand[i-1];
				precommand_size[i] = precommand_size[i-1];
			}
			precommand[0] = ((struct bts_action_send *)action_ptr)->data;
			precommand_size[0] = ((struct bts_action *)ptr)->size;
			/* FUJITSU TEN :2013-08-29 Log HCI_HardErr end */

			break;
		case ACTION_WAIT_EVENT:  /* wait */
			pr_debug("W");
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
			err = wait_for_completion_budet_interruptible_timeout
                                       (&kim_gdata->kim_rcvd,
                                         msecs_to_jiffies(CMD_RESP_TIME));

			if (err == -ERESTARTSYS) {
				ADA_LOG_ST_TRACE("Budet Interrput occurred. err :%ld",err);
				DRC_ST_EVT(EVT_BUDET+6, err);
				/* Interrupt occurred */
				release_firmware(kim_gdata->fw_entry);
				return -EINTR;
			} else if (!err) {
#else /* CONFIG_TI_FASTBOOT */
            if (!wait_for_completion_timeout
                        (&kim_gdata->kim_rcvd,
                         msecs_to_jiffies(CMD_RESP_TIME))) {
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDer end */
                pr_err("response timeout during fw download ");

				/* FUJITSU TEN :2013-08-29 Log HCI_HardErr start */
				for (i=0; i<HCI_HARDERR_BACKUP; i++) {
					if (precommand_size[i] > 0) {
						int cmdsize = precommand_size[i];
						unsigned char *cmdptr = precommand[i];
						pr_err("precommand#%d size(%d)",i,cmdsize);
						for (print_index=0; print_index<cmdsize && print_index<HCI_HARDERR_CMDMAX; print_index+=8) {
							pr_err("> %02x %02x %02x %02x %02x %02x %02x %02x "
								,print_index   < cmdsize ? cmdptr[print_index] : 0
								,print_index+1 < cmdsize ? cmdptr[print_index+1] : 0
								,print_index+2 < cmdsize ? cmdptr[print_index+2] : 0
								,print_index+3 < cmdsize ? cmdptr[print_index+3] : 0
								,print_index+4 < cmdsize ? cmdptr[print_index+4] : 0
								,print_index+5 < cmdsize ? cmdptr[print_index+5] : 0
								,print_index+6 < cmdsize ? cmdptr[print_index+6] : 0
								,print_index+7 < cmdsize ? cmdptr[print_index+7] : 0);
						}
					}
				}
				/* FUJITSU TEN :2013-08-29 Log HCI_HardErr end */

                /* timed out */
                release_firmware(kim_gdata->fw_entry);
                return -ETIMEDOUT;
            }
			INIT_COMPLETION(kim_gdata->kim_rcvd);
			break;
		case ACTION_DELAY:	/* sleep */
			pr_info("sleep command in scr");
			action_ptr = &(((struct bts_action *)ptr)->data[0]);
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
			remain = mdelay_budet_interruptible(
				((struct bts_action_delay *)action_ptr)->msec);
			if (remain) {
				ADA_LOG_ST_TRACE("Budet Interrupt occurred. remain :%ld",remain);
				DRC_ST_EVT(EVT_BUDET+7, remain);
				release_firmware(kim_gdata->fw_entry);
				return -EINTR;
			}
#else /* CONFIG_TI_FASTBOOT */
            mdelay(((struct bts_action_delay *)action_ptr)->msec);
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
			break;
		}
		len =
		    len - (sizeof(struct bts_action) +
			   ((struct bts_action *)ptr)->size);
		ptr =
		    ptr + sizeof(struct bts_action) +
		    ((struct bts_action *)ptr)->size;
	}
	/* fw download complete */
	release_firmware(kim_gdata->fw_entry);
	return 0;
}

/**********************************************************************/
/* functions called from ST core */
/* called from ST Core, when REG_IN_PROGRESS (registration in progress)
 * can be because of
 * 1. response to read local version
 * 2. during send/recv's of firmware download
 */
void st_kim_recv(void *disc_data, const unsigned char *data, long count)
{
	struct st_data_s	*st_gdata = (struct st_data_s *)disc_data;
	struct kim_data_s	*kim_gdata = st_gdata->kim_data;

	/* copy to local buffer */
	if (unlikely(data[4] == 0x01 && data[5] == 0x10 && data[0] == 0x04)) {
		/* must be the read_ver_cmd */
		memcpy(kim_gdata->resp_buffer, data, count);
		complete_all(&kim_gdata->kim_rcvd);
		return;
	} else {
		kim_int_recv(kim_gdata, data, count);
		/* either completes or times out */
	}
	return;
}

/* to signal completion of line discipline installation
 * called from ST Core, upon tty_open
 */
void st_kim_complete(void *kim_data)
{
	struct kim_data_s	*kim_gdata = (struct kim_data_s *)kim_data;
	complete(&kim_gdata->ldisc_installed);
}

/**
 * st_kim_start - called from ST Core upon 1st registration
 *	This involves toggling the chip enable gpio, reading
 *	the firmware version from chip, forming the fw file name
 *	based on the chip version, requesting the fw, parsing it
 *	and perform download(send/recv).
 */
long st_kim_start(void *kim_data)
{
	long err = 0;
	long retry = POR_RETRY_COUNT;
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
	unsigned long remain;
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
	struct ti_st_plat_data	*pdata;
	struct kim_data_s	*kim_gdata = (struct kim_data_s *)kim_data;

/* FUJITSU TEN :2013-02-21 DriveRec start */
	DRC_ST_DRVTRACE("st_kim_start");
/* FUJITSU TEN :2013-02-21 DriveRec end */
	pr_info(" %s", __func__);
	pdata = kim_gdata->kim_pdev->dev.platform_data;

/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
	remain = mdelay_budet_interruptible(sleep_bt_on_bten_off);
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
	if (remain) {
		ADA_LOG_ST_TRACE("Budet Interrupt occurred. remain :%ld", remain);
		DRC_ST_EVT(EVT_BUDET+8, remain);
		return -EINTR;
	}
#else /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
	mdelay(sleep_bt_on_bten_off);
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */

	do {
		/* platform specific enabling code here */
		wake_lock(&kim_gdata->core_data->st_wk_lock);
		/* Configure BT nShutdown to HIGH state */
/* FUJITSU TEN:2012-07-03 BT-EN signal inverted start */
#ifdef CONFIG_TI_ST_BT_EN_INVERT
		gpio_set_value(kim_gdata->nshutdown, GPIO_HIGH);
#else
		gpio_set_value(kim_gdata->nshutdown, GPIO_LOW);
#endif
/* FUJITSU TEN:2012-07-03 BT-EN signal inverted end */
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
		remain = mdelay_budet_interruptible(sleep_bt_on_bten_on);
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
		if (remain) {
			ADA_LOG_ST_TRACE("Budet Interrupt occurred. remain :%ld", remain);
			DRC_ST_EVT(EVT_BUDET+9, remain);
			return -EINTR;
		}
#else /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
		mdelay(sleep_bt_on_bten_on);
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */

/* FUJITSU TEN:2012-07-03 BT-EN signal inverted start */
#ifdef CONFIG_TI_ST_BT_EN_INVERT
		gpio_set_value(kim_gdata->nshutdown, GPIO_LOW);
#else
		gpio_set_value(kim_gdata->nshutdown, GPIO_HIGH);
#endif
/* FUJITSU TEN:2012-07-03 BT-EN signal inverted end */
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
		remain = mdelay_budet_interruptible(100);
		if (remain) {
			ADA_LOG_ST_TRACE("Budet Interrupt occurred. remain :%ld", remain);
			DRC_ST_EVT(EVT_BUDET+10, remain);
            return -EINTR;
        }
#else /* CONFIG_TI_FASTBOOT */
        mdelay(100);
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */

		/* re-initialize the completion */
		INIT_COMPLETION(kim_gdata->ldisc_installed);
		/* send notification to UIM */
		kim_gdata->ldisc_install = 1;
		pr_info("ldisc_install = 1");
		sysfs_notify(&kim_gdata->kim_pdev->dev.kobj,
				NULL, "install");
		/* wait for ldisc to be installed */
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
		err = wait_for_completion_budet_interruptible_timeout(
				&kim_gdata->ldisc_installed,
				msecs_to_jiffies(LDISC_TIME));
		if (err == -ERESTARTSYS) {
			ADA_LOG_ST_TRACE("Budet Interrupt occurred. err :%ld",err);
			DRC_ST_EVT(EVT_BUDET+11, err);
			return -EINTR;
		}
#else /* CONFIG_TI_FASTBOOT */
        err = wait_for_completion_timeout(&kim_gdata->ldisc_installed,
                        msecs_to_jiffies(LDISC_TIME));
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */

        if (!err) {     /* timeout */
/* FUJITSU TEN :2013-02-21 DriveRec start */
        	if (0 == retry) {
        		DRC_ST_HARDERR(HARD_ERR_BT_ON, ST_LDISC_INSTALL, err);
        	}
/* FUJITSU TEN :2013-02-21 DriveRec end */
			pr_err("line disc installation timed out ");
			kim_gdata->ldisc_install = 0;
			pr_info("ldisc_install = 0");
			sysfs_notify(&kim_gdata->kim_pdev->dev.kobj,
					NULL, "install");
			/* the following wait is never going to be completed,
			 * since the ldisc was never installed, hence serving
			 * as a mdelay of LDISC_TIME msecs */
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
			err = wait_for_completion_budet_interruptible_timeout
				(&kim_gdata->ldisc_installed,
				 msecs_to_jiffies(LDISC_TIME));
			if (err == -ERESTARTSYS) {
				ADA_LOG_ST_TRACE("Budet Interrupt occurred. err :%ld",err);
				DRC_ST_EVT(EVT_BUDET+12, err);
                        	return -EINTR;
			}
#else /* CONFIG_TI_FASTBOOT */
            err = wait_for_completion_timeout
                    (&kim_gdata->ldisc_installed,
                     msecs_to_jiffies(LDISC_TIME));
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
			err = -ETIMEDOUT;
			continue;
		} else {
			/* ldisc installed now */
			pr_info(" line discipline installed ");
			err = download_firmware(kim_gdata);
			if (err != 0) {
/* FUJITSU TEN :2013-02-21 DriveRec start */
        		if (0 == retry) {
					switch (err) {
					case -EIO:
					case -ETIMEDOUT:
					case -ENOENT:
						DRC_ST_HARDERR(HARD_ERR_BT_ON, ST_DOWNLOAD_FIRMWARE, err);
						break;
					default:
						DRC_ST_SOFTERR(SOFT_ERR_BT_ON, ST_DOWNLOAD_FIRMWARE, err);
						break;
					}
				}
/* FUJITSU TEN :2013-02-21 DriveRec end */
				pr_err("download firmware failed");
				kim_gdata->ldisc_install = 0;
				pr_info("ldisc_install = 0");
				sysfs_notify(&kim_gdata->kim_pdev->dev.kobj,
						NULL, "install");
				/* this wait might be completed, though in the
				 * tty_close() since the ldisc is already
				 * installed */
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
	            err = wait_for_completion_budet_interruptible_timeout
                            (&kim_gdata->ldisc_installed,
                            msecs_to_jiffies(LDISC_TIME));
                if (err == -ERESTARTSYS) {
					ADA_LOG_ST_TRACE("Budet Interrupt occurred. err :%ld",
						err);
					DRC_ST_EVT(EVT_BUDET+13, err);
                    return -EINTR;
                }
#else /* CONFIG_TI_FASTBOOT */
                err = wait_for_completion_timeout
                        (&kim_gdata->ldisc_installed,
                         msecs_to_jiffies(LDISC_TIME));
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
				err = -EINVAL;
				continue;
			} else {	/* on success don't retry */
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
/* FUJITSU TEN :2012-12-12 Support_DeviceReady_Flag DEL start */
//#ifdef CONFIG_TI_FASTBOOT
//				if (st_up_device_ready()) {
//					ADA_LOG_ST_TRACE("Budet Interrupted occured.");
//					return -EAGAIN;
//				}
//#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-12-12 Support_DeviceReady_Flag DEL end */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
				break;
			}
		}
	} while (retry--);
	return err;
}

/**
 * st_kim_stop - called from ST Core, on the last un-registration
 *	toggle low the chip enable gpio
 */
long st_kim_stop(void *kim_data)
{
	long err = 0;
	struct kim_data_s	*kim_gdata = (struct kim_data_s *)kim_data;

/* FUJITSU TEN :2013-02-21 DriveRec start */
	DRC_ST_DRVTRACE("st_kim_stop");
/* FUJITSU TEN :2013-02-21 DriveRec end */

	INIT_COMPLETION(kim_gdata->ldisc_installed);

	/* Flush any pending characters in the driver and discipline. */
	tty_ldisc_flush(kim_gdata->core_data->tty);
	tty_driver_flush_buffer(kim_gdata->core_data->tty);

	/* send uninstall notification to UIM */
	pr_info("ldisc_install = 0");
	kim_gdata->ldisc_install = 0;
	sysfs_notify(&kim_gdata->kim_pdev->dev.kobj, NULL, "install");

	/* wait for ldisc to be un-installed */
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
	err = wait_for_completion_budet_interruptible_timeout(
			&kim_gdata->ldisc_installed, msecs_to_jiffies(LDISC_TIME));
	if (err == -ERESTARTSYS) {
		ADA_LOG_ST_TRACE("Budet Interrupt occurred. err :%ld",err);
		DRC_ST_EVT(EVT_BUDET+14, err);
        return -EINTR;
	}
#else /* CONFIG_TI_FASTBOOT */
    err = wait_for_completion_timeout(&kim_gdata->ldisc_installed,
                        msecs_to_jiffies(LDISC_TIME));
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
    if (!err) {             /* timeout */
		pr_err(" timed out waiting for ldisc to be un-installed");
		return -ETIMEDOUT;
	}

	/* By default configure BT nShutdown to LOW state */
	gpio_set_value(kim_gdata->nshutdown, GPIO_LOW);

/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
	err = mdelay_budet_interruptible(sleep_bt_off_bten_off);
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
	if (err) {
		ADA_LOG_ST_TRACE("Budet Interrupt occurred. remain :%ld", err);
		DRC_ST_EVT(EVT_BUDET+15, err);
		return -EINTR;
	}
#else /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
	mdelay(sleep_bt_off_bten_off);
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */

	gpio_set_value(kim_gdata->nshutdown, GPIO_HIGH);

/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
	err = mdelay_budet_interruptible(sleep_bt_off_bten_on);
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
	if (err) {
		ADA_LOG_ST_TRACE("Budet Interrupt occurred. remain :%ld", err);
		DRC_ST_EVT(EVT_BUDET+16, err);
		return -EINTR;
	}
#else /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
	mdelay(sleep_bt_off_bten_on);
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */

	gpio_set_value(kim_gdata->nshutdown, GPIO_LOW);

	/* platform specific disable */
	wake_unlock(&kim_gdata->core_data->st_wk_lock);
	return err;
}

/**********************************************************************/
/* functions called from subsystems */
/* called when debugfs entry is read from */

static int show_version(struct seq_file *s, void *unused)
{
	struct kim_data_s *kim_gdata = (struct kim_data_s *)s->private;
	seq_printf(s, "%04X %d.%d.%d\n", kim_gdata->version.full,
			kim_gdata->version.chip, kim_gdata->version.maj_ver,
			kim_gdata->version.min_ver);
	return 0;
}

static int show_list(struct seq_file *s, void *unused)
{
	struct kim_data_s *kim_gdata = (struct kim_data_s *)s->private;
	kim_st_list_protocols(kim_gdata->core_data, s);
	return 0;
}

static ssize_t show_install(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct kim_data_s *kim_data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", kim_data->ldisc_install);
}

static ssize_t show_dev_name(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct kim_data_s *kim_data = dev_get_drvdata(dev);
	return sprintf(buf, "%s\n", kim_data->dev_name);
}

static ssize_t show_baud_rate(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct kim_data_s *kim_data = dev_get_drvdata(dev);
	return sprintf(buf, "%ld\n", kim_data->baud_rate);
}

static ssize_t show_flow_cntrl(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct kim_data_s *kim_data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", kim_data->flow_cntrl);
}

/* structures specific for sysfs entries */
static struct kobj_attribute ldisc_install =
__ATTR(install, 0444, (void *)show_install, NULL);

static struct kobj_attribute uart_dev_name =
__ATTR(dev_name, 0444, (void *)show_dev_name, NULL);

static struct kobj_attribute uart_baud_rate =
__ATTR(baud_rate, 0444, (void *)show_baud_rate, NULL);

static struct kobj_attribute uart_flow_cntrl =
__ATTR(flow_cntrl, 0444, (void *)show_flow_cntrl, NULL);

static struct attribute *uim_attrs[] = {
	&ldisc_install.attr,
	&uart_dev_name.attr,
	&uart_baud_rate.attr,
	&uart_flow_cntrl.attr,
	NULL,
};

static struct attribute_group uim_attr_grp = {
	.attrs = uim_attrs,
};

/**
 * st_kim_ref - reference the core's data
 *	This references the per-ST platform device in the arch/xx/
 *	board-xx.c file.
 *	This would enable multiple such platform devices to exist
 *	on a given platform
 */
void st_kim_ref(struct st_data_s **core_data, int id)
{
	struct platform_device	*pdev;
	struct kim_data_s	*kim_gdata;
	/* get kim_gdata reference from platform device */
	pdev = st_get_plat_device(id);
	if (!pdev) {
		*core_data = NULL;
		return;
	}
	kim_gdata = dev_get_drvdata(&pdev->dev);
	*core_data = kim_gdata->core_data;
}

static int kim_version_open(struct inode *i, struct file *f)
{
	return single_open(f, show_version, i->i_private);
}

static int kim_list_open(struct inode *i, struct file *f)
{
	return single_open(f, show_list, i->i_private);
}

static const struct file_operations version_debugfs_fops = {
	/* version info */
	.open = kim_version_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
static const struct file_operations list_debugfs_fops = {
	/* protocols info */
	.open = kim_list_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/**********************************************************************/
/* functions called from platform device driver subsystem
 * need to have a relevant platform device entry in the platform's
 * board-*.c file
 */

struct dentry *kim_debugfs_dir;
static int kim_probe(struct platform_device *pdev)
{
	long status;
	struct kim_data_s	*kim_gdata;
	struct ti_st_plat_data	*pdata = pdev->dev.platform_data;

	if ((pdev->id != -1) && (pdev->id < MAX_ST_DEVICES)) {
		/* multiple devices could exist */
		st_kim_devices[pdev->id] = pdev;
	} else {
		/* platform's sure about existence of 1 device */
		st_kim_devices[0] = pdev;
	}

	kim_gdata = kzalloc(sizeof(struct kim_data_s), GFP_ATOMIC);
	if (!kim_gdata) {
		pr_err("no mem to allocate");
		return -ENOMEM;
	}
	dev_set_drvdata(&pdev->dev, kim_gdata);

	status = st_core_init(&kim_gdata->core_data);
	if (status != 0) {
		pr_err(" ST core init failed");
		return -EIO;
	}
	/* refer to itself */
	kim_gdata->core_data->kim_data = kim_gdata;

	wake_lock_init(&kim_gdata->core_data->st_wk_lock, WAKE_LOCK_SUSPEND,
				"st_wake_lock");
	/* Claim the chip enable nShutdown gpio from the system */
	kim_gdata->nshutdown = pdata->nshutdown_gpio;
	status = gpio_request(kim_gdata->nshutdown, "kim");
	if (unlikely(status)) {
		pr_err(" gpio %ld request failed ", kim_gdata->nshutdown);
		return status;
	}

	/* Configure nShutdown GPIO as output=0 */
	status = gpio_direction_output(kim_gdata->nshutdown, 0);
	if (unlikely(status)) {
		pr_err(" unable to configure gpio %ld", kim_gdata->nshutdown);
		return status;
	}
	
/* FUJITSU TEN:2012-07-03 BT-EN register enable start */
	tegra_gpio_enable(kim_gdata->nshutdown);
/* FUJITSU TEN:2012-07-03 BT-EN register enable end */

	/* get reference of pdev for request_firmware
	 */
	kim_gdata->kim_pdev = pdev;
	init_completion(&kim_gdata->kim_rcvd);
	init_completion(&kim_gdata->ldisc_installed);

	status = sysfs_create_group(&pdev->dev.kobj, &uim_attr_grp);
	if (status) {
		pr_err("failed to create sysfs entries");
		return status;
	}

	/* copying platform data */
	strncpy(kim_gdata->dev_name, pdata->dev_name, UART_DEV_NAME_LEN);
	kim_gdata->flow_cntrl = pdata->flow_cntrl;
	kim_gdata->baud_rate = pdata->baud_rate;
	pr_info("sysfs entries created\n");

	kim_debugfs_dir = debugfs_create_dir("ti-st", NULL);
	if (IS_ERR(kim_debugfs_dir)) {
		pr_err(" debugfs entries creation failed ");
		kim_debugfs_dir = NULL;
		return -EIO;
	}

	debugfs_create_file("version", S_IRUGO, kim_debugfs_dir,
				kim_gdata, &version_debugfs_fops);
	debugfs_create_file("protocols", S_IRUGO, kim_debugfs_dir,
				kim_gdata, &list_debugfs_fops);
	pr_info(" debugfs entries created ");
	return 0;
}

static int kim_remove(struct platform_device *pdev)
{
	/* free the GPIOs requested */
	struct ti_st_plat_data	*pdata = pdev->dev.platform_data;
	struct kim_data_s	*kim_gdata;

	kim_gdata = dev_get_drvdata(&pdev->dev);

	/* Free the Bluetooth/FM/GPIO
	 * nShutdown gpio from the system
	 */
	
/* FUJITSU TEN:2012-07-03 BT-EN register enable start */
	tegra_gpio_disable(kim_gdata->nshutdown);
/* FUJITSU TEN:2012-07-03 BT-EN register enable end */
	
	gpio_free(pdata->nshutdown_gpio);
	pr_info("nshutdown GPIO Freed");

	debugfs_remove_recursive(kim_debugfs_dir);
	sysfs_remove_group(&pdev->dev.kobj, &uim_attr_grp);
	pr_info("sysfs entries removed");

	wake_lock_destroy(&kim_gdata->core_data->st_wk_lock);
	kim_gdata->kim_pdev = NULL;
	st_core_exit(kim_gdata->core_data);

	kfree(kim_gdata);
	kim_gdata = NULL;

	return 0;
}

int kim_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct kim_data_s *kim_gdata;
	struct st_data_s *core_data;
	struct uart_state *uart_state;
	struct uart_port *uport;

	kim_gdata = dev_get_drvdata(&pdev->dev);
	core_data = kim_gdata->core_data;

	if (st_ll_getstate(core_data) != ST_LL_INVALID) {
		uart_state = core_data->tty->driver_data;
		uport = uart_state->uart_port;
#ifdef CONFIG_BT_TIBLUESLEEP
		pr_info(" Bluesleep Start");
		bluesleep_start(uport);
#endif
	}

	return 0;
}

int kim_resume(struct platform_device *pdev)
{
	return 0;
}

/**********************************************************************/
/* entry point for ST KIM module, called in from ST Core */
static struct platform_driver kim_platform_driver = {
	.probe = kim_probe,
    .remove = kim_remove,
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
	.suspend = NULL,
	.resume = NULL,
#else /* CONFIG_TI_FASTBOOT */
        .suspend = kim_suspend,
	.resume = kim_resume,
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
	.driver = {
		.name = "kim",
		.owner = THIS_MODULE,
	},
};

/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT

static int st_reinit(struct device *dev) {
	/* FUJITSU TEN :2012-12-12 Support_DeviceReady_Flag start */
	struct st_data_s	*st_gdata;
	/* FUJITSU TEN :2012-12-12 Support_DeviceReady_Flag end */
	ADA_LOG_ST_INFO("called. dev=%p", dev);
	DRC_ST_EVT_DBG(EVT_BUDET+17, 0);
	
	/* FUJITSU TEN :2012-12-12 Support_DeviceReady_Flag start */
	st_kim_ref(&st_gdata, 0);

	if( !st_gdata || (0 == st_gdata->protos_registered) ) {
		if (st_up_device_ready()) {
			ADA_LOG_ST_INFO("Budet Interrupted occured.");
			DRC_ST_EVT_DBG(EVT_BUDET+18, 0);
/* FUJITSU TEN :2013-02-21 DriveRec start */
			DRC_ST_DRVTRACE("st_reinit:1");
		}
		else {
			ADA_LOG_ST_INFO("st_reinit() device ready UP \n");
			DRC_ST_DRVTRACE("st_reinit:0");
		}
	}
	else {
		DRC_ST_DRVTRACE("st_reinit:2");
	}
//			return 0;
//		}
//		pr_info("st_reinit() device ready UP \n");
//	}
/* FUJITSU TEN :2013-02-21 DriveRec end */
	/* FUJITSU TEN :2012-12-12 Support_DeviceReady_Flag end */
	
	return 0;
}

static void st_sudden_device_poweroff (struct device *dev) {
	ADA_LOG_ST_INFO("called. dev=%p", dev);
	ADA_LOG_ST_INFO("device_ready (%d -> 0)", st_device_ready);
/* FUJITSU TEN :2013-02-21 DriveRec start */
	DRC_ST_DRVTRACE("st_sudden_dev_poweroff:%d", st_device_ready);
/* FUJITSU TEN :2013-02-21 DriveRec end */
	st_device_ready = 0;
}

static struct early_device st_early_device = {
	.reinit = st_reinit,
	.sudden_device_poweroff = st_sudden_device_poweroff,
	.dev = NULL,
};
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */

static int __init st_kim_init(void)
{
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
	int ret = 0;
	uint8_t val = 0;
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
	early_device_register(&st_early_device);
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
	
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile start */
	ret = GetNONVOLATILE(&val, FTEN_NONVOL_TI_SLEEP_BT_ON_BTEN_OFF,
		FTEN_NONVOL_TI_SLEEP_BT_ON_BTEN_OFF_SIZE);
	if( ret != 0 ) {
		//ADA_LOG_ST_WARNING("read failed(FTEN_NONVOL_TI_SLEEP_BT_ON_BTEN_OFF)");
		DRC_ST_SOFTERR(NVL_READ_ERR, 0x0001, ret);
	} else {
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe start */
		if (NONVOL_VALCHK(val)) {
			sleep_bt_on_bten_off = val * 10;
		} else {
			//ADA_LOG_ST_WARNING("read abnormal(FTEN_NONVOL_TI_SLEEP_BT_ON_BTEN_OFF)");
			DRC_ST_SOFTERR(NVL_READ_ERR, 0x0002, ret);
		}
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe end */
	}
	ret = GetNONVOLATILE(&val, FTEN_NONVOL_TI_SLEEP_BT_ON_BTEN_ON,
		FTEN_NONVOL_TI_SLEEP_BT_ON_BTEN_ON_SIZE);
	if( ret != 0 ) {
		//ADA_LOG_ST_WARNING("read failed(FTEN_NONVOL_TI_SLEEP_BT_ON_BTEN_ON)");
		DRC_ST_SOFTERR(NVL_READ_ERR, 0x0003, ret);
	} else {
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe start */
		if (NONVOL_VALCHK(val)) {
			sleep_bt_on_bten_on = val * 10;
		} else {
			//ADA_LOG_ST_WARNING("read abnormal(FTEN_NONVOL_TI_SLEEP_BT_ON_BTEN_ON)");
			DRC_ST_SOFTERR(NVL_READ_ERR, 0x0004, ret);
		}
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe end */
	}
	
	ret = GetNONVOLATILE(&val, FTEN_NONVOL_TI_SLEEP_BT_OFF_BTEN_OFF,
		FTEN_NONVOL_TI_SLEEP_BT_OFF_BTEN_OFF_SIZE);
	if( ret != 0 ) {
		//ADA_LOG_ST_WARNING("read failed(FTEN_NONVOL_TI_SLEEP_BT_OFF_BTEN_OFF)");
		DRC_ST_SOFTERR(NVL_READ_ERR, 0x0005, ret);
	} else {
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe start */
		if (NONVOL_VALCHK(val)) {
			sleep_bt_off_bten_off = val * 10;
		} else {
			//ADA_LOG_ST_WARNING("read abnormal(FTEN_NONVOL_TI_SLEEP_BT_OFF_BTEN_OFF)");
			DRC_ST_SOFTERR(NVL_READ_ERR, 0x0006, ret);
		}
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe end */
	}
	
	ret = GetNONVOLATILE(&val, FTEN_NONVOL_TI_SLEEP_BT_OFF_BTEN_ON,
		FTEN_NONVOL_TI_SLEEP_BT_OFF_BTEN_ON_SIZE);
	if( ret != 0 ) {
		//ADA_LOG_ST_WARNING("read failed(FTEN_NONVOL_TI_SLEEP_BT_OFF_BTEN_ON)");
		DRC_ST_SOFTERR(NVL_READ_ERR, 0x0007, ret);
	} else {
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe start */
		if (NONVOL_VALCHK(val)) {
			sleep_bt_off_bten_on = val * 10;
		} else {
			//ADA_LOG_ST_WARNING("read abnormal(FTEN_NONVOL_TI_SLEEP_BT_OFF_BTEN_ON)");
			DRC_ST_SOFTERR(NVL_READ_ERR, 0x0008, ret);
		}
/* FUJITSU TEN :2012-12-18 Support_Nonvolatile_Failsafe end */
	}
/* FUJITSU TEN :2013-02-21 DriveRec start */
	DRC_ST_DRVTRACE("nvst:%d %d %d %d"
		,sleep_bt_on_bten_off
		,sleep_bt_on_bten_on
		,sleep_bt_off_bten_off
		,sleep_bt_off_bten_on);
/* FUJITSU TEN :2013-02-21 DriveRec end */
/* FUJITSU TEN :2012-12-11 Support_Nonvolatile end */
	return platform_driver_register(&kim_platform_driver);
}

static void __exit st_kim_deinit(void)
{
/* FUJITSU TEN :2012-11-15 Support_BuDet start */
#ifdef CONFIG_TI_FASTBOOT
	early_device_unregister(&st_early_device);
#endif /* CONFIG_TI_FASTBOOT */
/* FUJITSU TEN :2012-11-15 Support_BuDet end */
	platform_driver_unregister(&kim_platform_driver);
}

module_init(st_kim_init);
module_exit(st_kim_deinit);
MODULE_AUTHOR("Pavan Savoy <pavan_savoy@ti.com>");
MODULE_DESCRIPTION("Shared Transport Driver for TI BT/FM/GPS combo chips ");
MODULE_LICENSE("GPL");
