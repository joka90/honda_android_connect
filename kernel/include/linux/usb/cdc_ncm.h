/*
 * USB CDC NCM (Network Control Model) definitions
 *
 * Copyright (C) 2012 MCCI Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LINUX_CDC_NCM_H_		/* prevent multiple includes */
#define __LINUX_CDC_NCM_H_


#ifndef _LINUX_NETDEVICE_H
# include <linux/netdevice.h>
#endif

#ifndef _LINUX_CTYPE_H
# include <linux/ctype.h>
#endif

#ifndef _LINUX_ETHTOOL_H
# include <linux/ethtool.h>
#endif

#ifndef _LINUX_WORKQUEUE_H
# include <linux/workqueue.h>
#endif

#ifndef __LINUX_MII_H__
# include <linux/mii.h>
#endif

#ifndef _LINUX_CRC32_H
# include <linux/crc32.h>
#endif

#ifndef __LINUX_USB_H
# include <linux/usb.h>
#endif

#ifndef _LINUX_TIMER_H
# include <linux/timer.h>
#endif

#ifndef __LINUX_SPINLOCK_H
# include <linux/spinlock.h>
#endif

#ifndef _LINUX_ATOMIC_H
# include <linux/atomic.h>
#endif

#ifndef __LINUX_USB_USBNET_H
# include <linux/usb/usbnet.h>
#endif

#ifndef __LINUX_USB_CDC_H
# include <linux/usb/cdc.h>
#endif

/****************************************************************************\
|
|	XDESCRIPTION
|
\****************************************************************************/

/* Maximum NTB length */
#define	CDC_NCM_NTB_MAX_SIZE_TX			32768	/* bytes */
#define	CDC_NCM_NTB_MAX_SIZE_RX			32768	/* bytes */

/*
 * Maximum amount of datagrams in NCM Datagram Pointer Table, not counting
 * the last NULL entry. Any additional datagrams in NTB would be discarded.
 */
#define	CDC_NCM_DPT_DATAGRAMS_MAX		32

#define USB_CDC_NCM_NDP16_LENGTH_MIN		0x10

/* Restart the timer, if amount of datagrams is less than given value */
#define	CDC_NCM_RESTART_TIMER_DATAGRAM_CNT	3

#define	CDC_NCM_MIN_TX_PKT			512	/* bytes */

/* Default value for MaxDatagramSize */
#define	CDC_NCM_MAX_DATAGRAM_SIZE		8192	/* bytes */

struct cdc_ncm_data
	{
	struct usb_cdc_ncm_nth16	nth16;
	struct usb_cdc_ncm_ndp16	ndp16;
	struct usb_cdc_ncm_dpe16	dpe16[CDC_NCM_DPT_DATAGRAMS_MAX + 1];
	};

struct cdc_ncm_ctx
	{
	struct cdc_ncm_data			rx_ncm;
	struct cdc_ncm_data			tx_ncm;
	struct usb_cdc_ncm_ntb_parameter	ncm_parm;
	struct timer_list			tx_timer;

	const struct usb_cdc_ncm_desc		*func_desc;
	const struct usb_cdc_header_desc	*header_desc;
	const struct usb_cdc_union_desc		*union_desc;
	const struct usb_cdc_ether_desc		*ether_desc;

	struct net_device			*netdev;
	struct usb_device			*udev;
	struct usb_host_endpoint		*in_ep;
	struct usb_host_endpoint		*out_ep;
	struct usb_host_endpoint		*status_ep;
	struct usb_interface			*intf;
	struct usb_interface			*control;
	struct usb_interface			*data;

	struct sk_buff				*tx_curr_skb;
	struct sk_buff				*tx_rem_skb;

	spinlock_t				mtx;

	u32					tx_timer_pending;
	u32					tx_curr_offset;
	u32					tx_curr_last_offset;
	u32					tx_curr_frame_num;
	u32					rx_speed;
	u32					tx_speed;
	u32					rx_max;
	u32					tx_max;
	u32					max_datagram_size;
	u16					tx_max_datagrams;
	u16					tx_remainder;
	u16					tx_modulus;
	u16					tx_ndp_modulus;
	u16					tx_seq;
	u16					connected;
	u8					data_claimed;
	u8					control_claimed;
	struct ncm_xf_fixup_switch *    pncm_xf_fixup_switch;
	void *					pLdkInternalInfo;
	u8					auto_mode;
	};

struct ncm_xf_fixup_switch
	{
	struct cdc_ncm_ctx *	pCdcNcmCtx;
	int     switchflag;

#define FLAG_XF_FIXUP_SWITCH_YES	0x0001
#define FLAG_XF_FIXUP_SWITCH_NO		0x0000

	/* fixup rx packet (strip framing) */
	int     (*rx_fixup)( void *, void *);

	/* fixup tx packet (add framing) */
	struct sk_buff  *(*tx_fixup)( void *, void *, unsigned);
	};

struct ncm_client_switch
    {
    int (*post_client_bind)(void *, void *);
    int (*post_client_unbind)(void *, void *);
    int (*rx_fixup)(void *, void *);
    struct sk_buff *(*tx_fixup)(void *, void *, unsigned);

    void (*usbnet_defer_kevent)(struct usbnet *, int);
    netdev_tx_t (*usbnet_start_xmit) (struct sk_buff *, struct net_device *);
    int (*usb_submit_urb)(struct urb *, gfp_t );
    void (*usbnet_skb_return) (struct usbnet *, struct sk_buff *);
    void (*usb_free_urb)(struct urb *);
    struct urb *(*usb_alloc_urb)(int, gfp_t);
    void (*usb_anchor_urb)(struct urb *, struct usb_anchor *);
    int (*usb_autopm_get_interface_async) (struct usb_interface * intf);
    void (*usb_autopm_put_interface_async) (struct usb_interface * intf);
    };

void *
mccildk_register_client_ncm_switch(
	void
	);

/**** end of cdc_ncm.h ****/
#endif /* __LINUX_CDC_NCM_H_ */
