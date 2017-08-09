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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdint.h>
#include <errno.h>

#include <glib.h>
#include <dbus/dbus.h>
#include <gdbus.h>

#include <sys/ioctl.h>
#include <sys/socket.h>


#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "log.h"
#include <utils/Log.h>

#include "error.h"
#include "hcicommand.h"
#include "dbus-common.h"
#include "plugin.h"
#include "recorder.h"

/* Source file ID */
#define BLUEZ_SRC_ID BLUEZ_SRC_ID_SRC_HCICOMMAND_C

char dev_id = 0;

static struct hci_dev_info di;
bdaddr_t gBdAddr;
#define BDADDR_SIZE 6
static DBusConnection *g_hci_connection = NULL;

static void print_dev_hdr(struct hci_dev_info *di);

extern int HCI_get_device_id();

static void print_link_policy(struct hci_dev_info *di)
{
	char *str;

	str = hci_lptostr(di->link_policy);
	if (str) {
		printf("\tLink policy: %s\n", str);
		bt_free(str);
	}
	BLUEZ_DEBUG_LOG("[HCI_CMD_019_001] print_link_policy ");
}

static char *get_minor_device_name(int major, int minor)
{
	switch (major) {
	case 0: /* misc */
		return "";
	case 1: /* computer */
		switch (minor) {
		case 0:
			return "Uncategorized";
		case 1:
			return "Desktop workstation";
		case 2:
			return "Server";
		case 3:
			return "Laptop";
		case 4:
			return "Handheld";
		case 5:
			return "Palm";
		case 6:
			return "Wearable";
		}
		break;
	case 2: /* phone */
		switch (minor) {
		case 0:
			return "Uncategorized";
		case 1:
			return "Cellular";
		case 2:
			return "Cordless";
		case 3:
			return "Smart phone";
		case 4:
			return "Wired modem or voice gateway";
		case 5:
			return "Common ISDN Access";
		case 6:
			return "Sim Card Reader";
		}
		break;
	case 3: /* lan access */
		if (minor == 0)
			return "Uncategorized";
		switch (minor / 8) {
		case 0:
			return "Fully available";
		case 1:
			return "1-17% utilized";
		case 2:
			return "17-33% utilized";
		case 3:
			return "33-50% utilized";
		case 4:
			return "50-67% utilized";
		case 5:
			return "67-83% utilized";
		case 6:
			return "83-99% utilized";
		case 7:
			return "No service available";
		}
		break;
	case 4: /* audio/video */
		switch (minor) {
		case 0:
			return "Uncategorized";
		case 1:
			return "Device conforms to the Headset profile";
		case 2:
			return "Hands-free";
			/* 3 is reserved */
		case 4:
			return "Microphone";
		case 5:
			return "Loudspeaker";
		case 6:
			return "Headphones";
		case 7:
			return "Portable Audio";
		case 8:
			return "Car Audio";
		case 9:
			return "Set-top box";
		case 10:
			return "HiFi Audio Device";
		case 11:
			return "VCR";
		case 12:
			return "Video Camera";
		case 13:
			return "Camcorder";
		case 14:
			return "Video Monitor";
		case 15:
			return "Video Display and Loudspeaker";
		case 16:
			return "Video Conferencing";
			/* 17 is reserved */
		case 18:
			return "Gaming/Toy";
		}
		break;
	case 5: /* peripheral */ {
		static char cls_str[48];

		cls_str[0] = '\0';

		switch (minor & 48) {
		case 16:
			strncpy(cls_str, "Keyboard", sizeof(cls_str));
			break;
		case 32:
			strncpy(cls_str, "Pointing device", sizeof(cls_str));
			break;
		case 48:
			strncpy(cls_str, "Combo keyboard/pointing device", sizeof(cls_str));
			break;
		}
		if ((minor & 15) && (strlen(cls_str) > 0))
			strcat(cls_str, "/");

		switch (minor & 15) {
		case 0:
			break;
		case 1:
			strncat(cls_str, "Joystick", sizeof(cls_str) - strlen(cls_str));
			break;
		case 2:
			strncat(cls_str, "Gamepad", sizeof(cls_str) - strlen(cls_str));
			break;
		case 3:
			strncat(cls_str, "Remote control", sizeof(cls_str) - strlen(cls_str));
			break;
		case 4:
			strncat(cls_str, "Sensing device", sizeof(cls_str) - strlen(cls_str));
			break;
		case 5:
			strncat(cls_str, "Digitizer tablet", sizeof(cls_str) - strlen(cls_str));
			break;
		case 6:
			strncat(cls_str, "Card reader", sizeof(cls_str) - strlen(cls_str));
			break;
		default:
			strncat(cls_str, "(reserved)", sizeof(cls_str) - strlen(cls_str));
			break;
		}
		if (strlen(cls_str) > 0)
			return cls_str;

		break;
	}
	case 6: /* imaging */
		if (minor & 4)
			return "Display";
		if (minor & 8)
			return "Camera";
		if (minor & 16)
			return "Scanner";
		if (minor & 32)
			return "Printer";
		break;
	case 7: /* wearable */
		switch (minor) {
		case 1:
			return "Wrist Watch";
		case 2:
			return "Pager";
		case 3:
			return "Jacket";
		case 4:
			return "Helmet";
		case 5:
			return "Glasses";
		}
		break;
	case 8: /* toy */
		switch (minor) {
		case 1:
			return "Robot";
		case 2:
			return "Vehicle";
		case 3:
			return "Doll / Action Figure";
		case 4:
			return "Controller";
		case 5:
			return "Game";
		}
		break;
	case 63:	/* uncategorised */
		return "";
	}
	return "Unknown (reserved) minor device class";
}

static int command_class(int ctl, uint8_t *status, uint32_t opt)
{
	int s = 0;

	BLUEZ_DEBUG_INFO("[HCI] command_class() Start");

	static const char *services[] = { "Positioning",
					"Networking",
					"Rendering",
					"Capturing",
					"Object Transfer",
					"Audio",
					"Telephony",
					"Information" };

	static const char *major_devices[] = { "Miscellaneous",
					"Computer",
					"Phone",
					"LAN Access",
					"Audio/Video",
					"Peripheral",
					"Imaging",
					"Uncategorized" };

	*status = 0x90;

	if (ioctl(ctl, HCIGETDEVINFO, (void *) &di)) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't get device info");
		BLUEZ_DEBUG_LOG("[HCI_CMD_004_001] command_class ioctl Err return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_CLASS, 0, 0);
		return 0;
	}

	s = hci_open_dev(dev_id);
	BLUEZ_DEBUG_LOG("[HCI_CMD_004_002] command_class hci_open_dev s = %d", s);

	if (s < 0) {
		BLUEZ_DEBUG_LOG("Can't open device hci%d: %s (%d)\n",
						dev_id, strerror(errno), errno);
		BLUEZ_DEBUG_LOG("[HCI_CMD_004_003] command_class return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_CLASS, 0, 0);
		return 0;
	}

	if (hci_write_class_of_dev2(s, status, opt, 2000) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't write local class of device on hci%d: %s (%d)\n",
					dev_id, strerror(errno), errno);
		BLUEZ_DEBUG_LOG("[HCI_CMD_004_004] command_class hci_write_class_of_dev2=0 return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_CLASS, dev_id, errno);
		hci_close_dev(s);
		return 0;
	}

	hci_close_dev(s);
	BLUEZ_DEBUG_LOG("[HCI_CMD_004_005] command_class return 1");
	BLUEZ_DEBUG_INFO("[HCI] command_class() End");
	return 1;
}

static int command_evtmask(int ctl, uint8_t *status, char *opt)
{
	*status = 0x90;
	int s = 0;

	BLUEZ_DEBUG_INFO("[HCI] command_evtmask() Start");

	if (ioctl(ctl, HCIGETDEVINFO, (void *) &di)) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't get device info");
		BLUEZ_DEBUG_LOG("[HCI_CMD_006_001] command_evtmask ioctl Err return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_EVTMASK, 0, 0);
		return 0;
	}

	s = hci_open_dev(dev_id);
	BLUEZ_DEBUG_LOG("[HCI_CMD_006_002] command_evtmask hci_open_dev s = %d", s);

	if (s < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open device hci%d: %s (%d)\n",
						dev_id, strerror(errno), errno);
		BLUEZ_DEBUG_LOG("[HCI_CMD_006_003] command_evtmask return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_EVTMASK, dev_id, errno);
		return 0;
	}
	if (opt) {

		if (hci_write_event_mask2(s, status, (uint8_t *)opt, 1000) < 0) {
			BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't write stored link key on hci%d: %s (%d)\n",
								dev_id, strerror(errno), errno);
			BLUEZ_DEBUG_LOG("[HCI_CMD_006_004] command_evtmask hci_write_event_mask2 return 0");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_EVTMASK, dev_id, errno);
			hci_close_dev(s);
			return 0;
		}
	} else {
		BLUEZ_DEBUG_LOG("[HCI_CMD_006_005] command_evtmask  return 0");
		hci_close_dev(s);
		return 0;
	}
	hci_close_dev(s);
	BLUEZ_DEBUG_LOG("[HCI_CMD_006_006] command_evtmask  return 1");
	BLUEZ_DEBUG_INFO("[HCI] command_evtmask() End");
	return 1;
}

static int command_name(int ctl, uint8_t *status, char *opt)
{
	int dd;
	char name[249];
	int i;

	BLUEZ_DEBUG_INFO("[HCI] command_name() Start");

	*status = 0x90;

	if (ioctl(ctl, HCIGETDEVINFO, (void *) &di)) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't get device info");
		BLUEZ_DEBUG_LOG("[HCI_CMD_008_001] command_name ioctl Err return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_NAME, 0, 0);
		return 0;
	}
	
	dd = hci_open_dev(dev_id);
	BLUEZ_DEBUG_LOG("[HCI_CMD_008_002] command_name hci_open_dev dd = %d", dd);
	if (dd < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open device hci%d: %s (%d)\n",
						dev_id, strerror(errno), errno);
		BLUEZ_DEBUG_LOG("[HCI_CMD_008_003] command_name return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_NAME, dev_id, errno);
		return 0;
	}

	if (opt) {
		if (hci_write_local_name2(dd, status, opt, 2000) < 0) {
			BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't change local name on hci%d: %s (%d)\n",
						dev_id, strerror(errno), errno);
			BLUEZ_DEBUG_LOG("[HCI_CMD_008_004] command_name hci_write_local_name2 return 0");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_NAME, dev_id, errno);
			hci_close_dev(dd);
			return 0;
		}
	} else {
		if (hci_read_local_name2(dd, status, sizeof(name), name, 1000) < 0) {
			BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't read local name on hci%d: %s (%d)\n",
						dev_id, strerror(errno), errno);
			BLUEZ_DEBUG_LOG("[HCI_CMD_008_005] command_name hci_read_local_name2 return 0");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_NAME, dev_id, errno);
			hci_close_dev(dd);
			return 0;
		}

		for (i = 0; i < 248 && name[i]; i++) {
			if ((unsigned char) name[i] < 32 || name[i] == 127)
			{
				name[i] = '.';
				BLUEZ_DEBUG_LOG("[HCI_CMD_008_006] command_name name[%d]", i);
			}
		}

		name[248] = '\0';

		print_dev_hdr(&di);
		BLUEZ_DEBUG_LOG("[HCI_CMD_008_007] command_name print_dev_hdr");
		printf("\tName: '%s'\n", name);
		BLUEZ_DEBUG_LOG("\tName: '%s'\n", name);
	}

	hci_close_dev(dd);
	BLUEZ_DEBUG_LOG("[HCI_CMD_008_008] command_name hci_close_dev");
	BLUEZ_DEBUG_LOG("[HCI_CMD_008_009] command_name return 1");
	BLUEZ_DEBUG_INFO("[HCI] command_name() End");
	return 1;
}

static int command_version(int ctl, uint8_t *status, uint16_t *manufacturer, 
						   uint8_t *hci_ver, uint16_t *hci_rev, 
						   uint8_t *lmp_ver, uint16_t *lmp_subver)
{
	struct hci_version ver;
	char *hciver, *lmpver;
	int dd;

	BLUEZ_DEBUG_INFO("[HCI] command_version() Start");

	*status = 0x90;

	if (ioctl(ctl, HCIGETDEVINFO, (void *) &di)) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't get device info");
		BLUEZ_DEBUG_LOG("[HCI_CMD_010_001] command_version ioctl return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_VERSION, 0, 0);
		return 0;
	}
	
	dd = hci_open_dev(dev_id);
	BLUEZ_DEBUG_LOG("[HCI_CMD_010_002] command_version hci_open_dev dd = %d", dd);
	if (dd < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open device hci%d: %s (%d)\n",
						dev_id, strerror(errno), errno);
		BLUEZ_DEBUG_LOG("[HCI_CMD_010_003] command_version dd return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_VERSION, dev_id, errno);
		return 0;
	}

	if (hci_read_local_version2(dd, status, &ver, 1000) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't read version info hci%d: %s (%d)\n",
						dev_id, strerror(errno), errno);
		BLUEZ_DEBUG_LOG("[HCI_CMD_010_004] command_version hci_read_local_version2 return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_VERSION, dev_id, errno);
		hci_close_dev(dd);
		return 0;
	}

	hciver = hci_vertostr(ver.hci_ver);
	BLUEZ_DEBUG_LOG("[HCI_CMD_010_005] command_version hci_vertostr hciver = %s", hciver);
	lmpver = lmp_vertostr(ver.lmp_ver);
	BLUEZ_DEBUG_LOG("[HCI_CMD_010_006] command_version lmp_vertostr lmpver = %s", lmpver);

	print_dev_hdr(&di);
	BLUEZ_DEBUG_LOG("[HCI_CMD_010_007] command_version print_dev_hdr");

	*manufacturer = ver.manufacturer;
	*hci_ver	  = ver.hci_ver;
	*hci_rev	  = ver.hci_rev;
	*lmp_ver	  = ver.lmp_ver;
	*lmp_subver   = ver.lmp_subver;

	if (hciver)
	{
		bt_free(hciver);
		BLUEZ_DEBUG_LOG("[HCI_CMD_010_008] command_version bt_free");
	}
	if (lmpver)
	{
		bt_free(lmpver);
		BLUEZ_DEBUG_LOG("[HCI_CMD_010_009] command_version bt_free");
	}

	hci_close_dev(dd);
	BLUEZ_DEBUG_LOG("[HCI_CMD_010_010] command_version hci_close_dev");
	BLUEZ_DEBUG_LOG("[HCI_CMD_010_011] command_version return 1");
	BLUEZ_DEBUG_INFO("[HCI] command_version() End");
	return 1;
}

static int command_sspmode(int ctl, uint8_t *status, uint8_t opt)
{
	int dd;

	BLUEZ_DEBUG_INFO("[HCI] command_sspmode() Start");

	*status = 0x90;

	if (ioctl(ctl, HCIGETDEVINFO, (void *) &di)) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't get device info");
		BLUEZ_DEBUG_LOG("[HCI_CMD_012_001] command_sspmode ioctl return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_SSPMODE, 0, 0);
		return 0;
	}
	
	dd = hci_open_dev(dev_id);
	BLUEZ_DEBUG_LOG("[HCI_CMD_012_002] command_sspmode hci_open_dev dd = %d", dd);
	if (dd < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open device hci%d: %s (%d)\n",
						dev_id, strerror(errno), errno);
		BLUEZ_DEBUG_LOG("[HCI_CMD_012_003] command_sspmode hci_open_dev dd = %d", dd);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_SSPMODE, dev_id, errno);
		return 0;
	}

	if (hci_write_simple_pairing_mode2(dd, status, opt, 2000) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't set Simple Pairing mode on hci%d: %s (%d)",
				dev_id, strerror(errno), errno);
		BLUEZ_DEBUG_LOG("[HCI_CMD_012_004] command_sspmode hci_write_simple_pairing_mode2 return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_SSPMODE, dev_id, errno);
		hci_close_dev(dd);
		return 0;
	}

	hci_close_dev(dd);
	BLUEZ_DEBUG_LOG("[HCI_CMD_012_004] command_sspmode return 1");
	BLUEZ_DEBUG_INFO("[HCI] command_sspmode() End");
	return 1;
}

static int command_lp(int ctl, uint8_t *status, const short link_policy)
{
	struct hci_dev_req dr;

	BLUEZ_DEBUG_INFO("[HCI] command_lp() Start");

	*status = 0x90;

	if (ioctl(ctl, HCIGETDEVINFO, (void *) &di)) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't get device info");
		BLUEZ_DEBUG_LOG("[HCI_CMD_014_001] command_lp ioctl return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_LP, 0, 0);
		return 0;
	}
	
	dr.dev_id = dev_id;
	dr.dev_opt = (uint32_t)link_policy;

	if (ioctl(ctl, HCISETLINKPOL, (unsigned long) &dr) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't set link policy on hci%d: %s (%d)\n",
							dev_id, strerror(errno), errno);
		BLUEZ_DEBUG_LOG("[HCI_CMD_014_002] command_lp ioctl return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_LP, dev_id, errno);
		return 0;
	}

	*status = 0x00;
	BLUEZ_DEBUG_LOG("[HCI_CMD_014_005] command_lp return 1");
	BLUEZ_DEBUG_INFO("[HCI] command_lp() End");
	return 1;
}

static int command_addr(int ctl, uint8_t *status, bdaddr_t *opt)
{
	int dd;
	char addr[BDADDR_SIZE];
	int i;

	BLUEZ_DEBUG_INFO("[HCI] command_addr() Start");

	*status = 0x90;

	dd = hci_open_dev(dev_id);
	BLUEZ_DEBUG_LOG("[HCI_CMD_016_001] command_addr hci_open_dev dd = %d", dd);
	if (dd < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open device hci%d: %s (%d)\n",
						dev_id, strerror(errno), errno);
		BLUEZ_DEBUG_LOG("[HCI_CMD_016_002] command_addr dd return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_ADDR, 0, 0);
		return 0;
	}

	if (hci_read_bd_addr2(dd, status, (bdaddr_t*)&addr[0], 1000) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't read BD_Addr on hci%d: %s (%d)\n",
					dev_id, strerror(errno), errno);
		BLUEZ_DEBUG_LOG("[HCI_CMD_016_003] command_addr hci_read_bd_addr2 return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_ADDR, dev_id, errno);
		hci_close_dev(dd);
		return 0;
	}

	print_dev_hdr(&di);
	BLUEZ_DEBUG_LOG("[HCI_CMD_016_004] command_addr print_dev_hdr");
	BLUEZ_DEBUG_LOG("[HCI] tBD Address: [ %02x:%02x:%02x:%02x:%02x:%02x ]", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0] );
	
	memcpy( opt, addr, sizeof(bdaddr_t) ) ;
	
	hci_close_dev(dd);
	BLUEZ_DEBUG_LOG("[HCI_CMD_016_005] command_addr hci_close_dev");
	BLUEZ_DEBUG_LOG("[HCI_CMD_016_006] command_addr return 1");
	BLUEZ_DEBUG_INFO("[HCI] command_addr() End");
	return 1;
}

static int command_dc(int ctl, uint16_t handle, uint8_t reason)
{
	int dd;

	BLUEZ_DEBUG_INFO("[HCI] command_dc() Start");

	if (ioctl(ctl, HCIGETDEVINFO, (void *) &di)) {
		BLUEZ_DEBUG_ERROR(" [Bluez - HCI] Can't get device info");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_DC, 0, 0);
		return 0;
	}

	dd = hci_open_dev(dev_id);
	if (dd < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open device hci%d: %s (%d)\n",
						dev_id, strerror(errno), errno);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_DC, dev_id, errno);
		return 0;
	}

	if (hci_devinfo(dev_id, &di) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't get device info for hci%d: %s (%d)\n",
						dev_id, strerror(errno), errno);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_DC, dev_id, errno);
		hci_close_dev(dd);
		return 0;
	}

	if (hci_disconnect(dd, handle, reason, 1000) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't write stored link key on hci%d: %s (%d)\n",
						dev_id, strerror(errno), errno);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_DC, dev_id, errno);
		hci_close_dev(dd);
		return 0;
	}

	hci_close_dev(dd);

	BLUEZ_DEBUG_INFO("[HCI] command_dc() End");

	return 1;
}

static int command_vs_codec(int ctl, uint8_t *status, uint8_t opt)
{
	int dd;

	*status = 0x90;

	if (ioctl(ctl, HCIGETDEVINFO, (void *) &di)) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't get device info");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_VSCODEC, 0, 0);
		return 0;
	}
	
	dd = hci_open_dev(dev_id);
	if (dd < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open device hci%d: %s (%d)", dev_id, strerror(errno), errno);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_VSCODEC, dev_id, errno);
		return 0;
	}
	
	if (hci_vs_write_codec_config(dd, status, opt, 2000) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't set Vs Write Codec Config on hci%d: %s (%d)",
							dev_id, strerror(errno), errno);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_VSCODEC, dev_id, errno);
		hci_close_dev(dd);
		return 0;
	}
	hci_close_dev(dd);
	return 1;
}

static int command_vs_wbs_disassociate(int ctl, uint8_t *status)
{
	int dd;

	*status = 0x90;

	if (ioctl(ctl, HCIGETDEVINFO, (void *) &di)) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't get device info");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_VSCODEC, 0, 0);
		return 0;
	}
	
	dd = hci_open_dev(dev_id);
	if (dd < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open device hci%d: %s (%d)", dev_id, strerror(errno), errno);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_VSCODEC, dev_id, errno);
		return 0;
	}
	
	if (hci_vs_wbs_disassociate(dd, status, 2000) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't set Vs Wbs Disassociate on hci%d: %s (%d)",
							dev_id, strerror(errno), errno);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_VSCODEC, dev_id, errno);
		hci_close_dev(dd);
		return 0;
	}
	hci_close_dev(dd);
	return 1;
}

static int command_send(int ctl, uint8_t *status, char *opt, uint32_t len)
{
	*status = 0x00;
	int s = 0;

	BLUEZ_DEBUG_INFO("[HCI] command_send() Start");

	if (ioctl(ctl, HCIGETDEVINFO, (void *) &di)) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't get device info");
		BLUEZ_DEBUG_LOG("[HCI_CMD_006_001] command_send ioctl Err return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_EVTMASK, 0, 0);
		return 0;
	}

	s = hci_open_dev(dev_id);
	BLUEZ_DEBUG_LOG("[HCI_CMD_006_002] command_send hci_open_dev s = %d", s);

	if (s < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open device hci%d: %s (%d)\n",
						dev_id, strerror(errno), errno);
		BLUEZ_DEBUG_LOG("[HCI_CMD_006_003] command_send return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_EVTMASK, dev_id, errno);
		return 0;
	}
	if (opt) {
		if (hci_send_packet(s, status, opt, len, 1000) < 0) {
			BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't Command Send on hci%d: %s (%d)\n",
								dev_id, strerror(errno), errno);
			BLUEZ_DEBUG_LOG("[HCI_CMD_006_004] command_send hci_send_packet return 0");
			ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_EVTMASK, dev_id, errno);
			hci_close_dev(s);
			return 0;
		}
	} else {
		BLUEZ_DEBUG_LOG("[HCI_CMD_006_005] command_send  return 0");
		hci_close_dev(s);
		return 0;
	}
	hci_close_dev(s);
	BLUEZ_DEBUG_LOG("[HCI_CMD_006_006] command_send  return 1");
	BLUEZ_DEBUG_INFO("[HCI] command_send() End");
	return 1;
}

/** class command 実行**/
static DBusMessage *class_comm(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	int ctl;
	char err;
	DBusMessage *reply;
	uint32_t classVal;
	uint8_t status;

	BLUEZ_DEBUG_INFO("[HCI] class_comm() Start.");
	BLUEZ_DEBUG_LOG("[HCI_CMD_002_001] class_comm Call");
	EVENT_DRC_BLUEZ_HCI_METHOD(FORMAT_ID_HCI_COMMAND, METHOD_HCI_CLASSCOMM);

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_UINT32, &classVal,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_LOG("[HCI_CMD_003_001] class_comm dbus_message_get_args Err");
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_CLASS, 0, 0);
		return NULL;
	}
		
	BLUEZ_DEBUG_LOG("[HCI] class_comm() classVal = %x.", classVal);

	/* Open HCI socket  */
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open HCI socket");
		BLUEZ_DEBUG_LOG("[HCI_CMD_003_002] class_comm socket return NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_CLASS, 0, 0);
		return NULL;
	}

	/*class command処理実行 */
	err = command_class(ctl, &status, classVal);
	BLUEZ_DEBUG_LOG("[HCI_CMD_003_003] class_comm command_class err = %d", err);
	/* Close HCI socket	*/
	close(ctl);
	BLUEZ_DEBUG_INFO("[HCI_CMD_003_004] class_comm close");

	g_dbus_emit_signal(g_hci_connection, "/",
						SRC_HCICOMMAND_INTERFACE,
						"RET_CLASS",
						DBUS_TYPE_BYTE, &status,
						DBUS_TYPE_INVALID);

	BLUEZ_DEBUG_INFO("[HCI] class_comm() End. status = %d", status);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HCI_CLASSCOMM, status);
	return dbus_message_new_method_return(msg);
}

/** evtmask command 実行**/
static DBusMessage *evtmask_comm(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	int ctl;
	char err;
	DBusMessage *reply;
	const char *mask;
	uint8_t status;
	uint32_t len;

	BLUEZ_DEBUG_INFO("[HCI] evtmask_comm() Start.");
	BLUEZ_DEBUG_LOG("[HCI_CMD_002_002] evtmask_comm Call");
	EVENT_DRC_BLUEZ_HCI_METHOD(FORMAT_ID_HCI_COMMAND, METHOD_HCI_EVTMASKCOMM);

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
						&mask, &len,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] comm dbus_message_get_args() Error return NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_EVTMASK, 0, 0);
		return NULL;
	}
	
	if ( mask )
	{
		BLUEZ_DEBUG_LOG("[HCI] evtmask_comm() mask = %s.len = %d", mask,len);
		BLUEZ_DEBUG_LOG("[HCI_CMD_005_002] evtmask_comm mask = %s",  mask);
	}

	/* Open HCI socket  */
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open HCI socket. ");
		BLUEZ_DEBUG_LOG("[HCI_CMD_005_003] evtmask_comm socket return NULL; ");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_EVTMASK, 0, 0);
		return NULL;
	}

	/*evtmask command処理実行 */
	err = command_evtmask(ctl, &status, (char *)mask);
	BLUEZ_DEBUG_LOG("[HCI_CMD_005_004] evtmask_comm command_evtmask err = %d", err);
	/* Close HCI socket	*/
	close(ctl);
	BLUEZ_DEBUG_INFO("[HCI_CMD_005_005] evtmask_comm close");

	g_dbus_emit_signal(g_hci_connection, "/",
						SRC_HCICOMMAND_INTERFACE,
						"RET_MASK",
						DBUS_TYPE_BYTE, &status,
						DBUS_TYPE_INVALID);

	BLUEZ_DEBUG_INFO("[HCI] evtmask_comm() End. status = %d", status);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HCI_EVTMASKCOMM, status);
	return dbus_message_new_method_return(msg);
}

/** name command 実行**/
static DBusMessage *name_comm(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	int ctl;
	char err;
	DBusMessage *reply;
	const char *name;
	uint8_t status;

	BLUEZ_DEBUG_INFO("[HCI] name_comm() Start.");
	EVENT_DRC_BLUEZ_HCI_METHOD(FORMAT_ID_HCI_COMMAND, METHOD_HCI_NAMECOMM);

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_STRING, &name,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] dbus_message_get_args() Error return NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_NAME, 0, 0);
		return NULL;
	}
	
	if ( name )
	{
		BLUEZ_DEBUG_LOG("[HCI] name_comm() name = %s.",name);
		BLUEZ_DEBUG_LOG("[HCI_CMD_007_002] name_comm name = %s", name);
	}

	/* Open HCI socket  */
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open HCI socket.");
		BLUEZ_DEBUG_LOG("[HCI_CMD_007_003] name_comm socket return NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_NAME, 0, 0);
		return NULL;
	}

	/*name command処理実行 */
	err = command_name(ctl, &status, (char *)name);
	BLUEZ_DEBUG_LOG("[HCI_CMD_007_004] name_comm command_name err = %d", err);
	
	/* Close HCI socket	*/
	close(ctl);
	BLUEZ_DEBUG_LOG("[HCI_CMD_007_005] name_comm close");

	g_dbus_emit_signal(g_hci_connection, "/",
						SRC_HCICOMMAND_INTERFACE,
						"RET_NAME",
						DBUS_TYPE_BYTE, &status,
						DBUS_TYPE_INVALID);

	BLUEZ_DEBUG_INFO("[HCI] name_comm() End. status = %d", status);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HCI_NAMECOMM, status);
	return dbus_message_new_method_return(msg);
}

/** version command 実行**/
static DBusMessage *version_comm(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	int ctl;
	char err;
	DBusMessage *reply;
	uint16_t hci_rev;
	uint16_t manufacturer;
	uint16_t lmp_subver;
	uint8_t  status;
	uint8_t  hci_ver;
	uint8_t  lmp_ver;

	BLUEZ_DEBUG_INFO("[HCI] version_comm() Start.");
	BLUEZ_DEBUG_LOG("[HCI_CMD_002_004] version_comm Call");
	EVENT_DRC_BLUEZ_HCI_METHOD(FORMAT_ID_HCI_COMMAND, METHOD_HCI_VERSIONCOMM);

	/* Open HCI socket  */
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open HCI socket.");
		BLUEZ_DEBUG_LOG("[HCI_CMD_009_001] version_comm socket return 0");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_VERSION, 0, 0);
		return NULL;
	}

	/*version command処理実行 */
	err = command_version(ctl, &status, &manufacturer, &hci_ver, &hci_rev, &lmp_ver, &lmp_subver);
	BLUEZ_DEBUG_LOG("[HCI_CMD_009_002] version_comm command_version err = %d", err);

	/* Close HCI socket	*/
	close(ctl);
	BLUEZ_DEBUG_LOG("[HCI_CMD_009_003] version_comm close");

	g_dbus_emit_signal(g_hci_connection, "/",
						SRC_HCICOMMAND_INTERFACE,
						"RET_VERSION",
						DBUS_TYPE_UINT16, &hci_rev,
						DBUS_TYPE_UINT16, &manufacturer,
						DBUS_TYPE_UINT16, &lmp_subver,
						DBUS_TYPE_BYTE, &status,
						DBUS_TYPE_BYTE, &hci_ver,
						DBUS_TYPE_BYTE, &lmp_ver,
						DBUS_TYPE_INVALID);

	BLUEZ_DEBUG_INFO("[HCI] version_comm() End. status = %d", status);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HCI_VERSIONCOMM, status);
	return dbus_message_new_method_return(msg);
}

/** sspmode command 実行**/
static DBusMessage *sspmode_comm(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	int ctl;
	char err;
	DBusMessage *reply;
	uint8_t mode;
	uint8_t status;

	BLUEZ_DEBUG_INFO("[HCI] sspmode_comm() Start.");
	BLUEZ_DEBUG_LOG("[HCI_CMD_002_005] sspmode_comm Call");
	EVENT_DRC_BLUEZ_HCI_METHOD(FORMAT_ID_HCI_COMMAND, METHOD_HCI_SSPMODECOMM);

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_BYTE, &mode,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] dbus_message_get_args() return NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_SSPMODE, 0, 0);
		return NULL;
	}

	BLUEZ_DEBUG_LOG("[HCI] sspmode_comm() mode = %x.", mode);

	/* Open HCI socket  */
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open HCI socket.");
		BLUEZ_DEBUG_LOG("[HCI_CMD_011_002] sspmode_comm socket return NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_SSPMODE, 0, 0);
		return NULL;
	}

	/*sspmode command処理実行 */
	err = command_sspmode(ctl, &status, mode);
	BLUEZ_DEBUG_LOG("[HCI_CMD_011_003] sspmode_comm command_sspmode err = %d", err);

	/* Close HCI socket	*/
	close(ctl);
	BLUEZ_DEBUG_LOG("[HCI_CMD_011_004] sspmode_comm close");

	g_dbus_emit_signal(g_hci_connection, "/",
						SRC_HCICOMMAND_INTERFACE,
						"RET_SSPMODE",
						DBUS_TYPE_BYTE, &status,
						DBUS_TYPE_INVALID);

	BLUEZ_DEBUG_INFO("[HCI] sspmode_comm() End. status = %d", status);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HCI_SSPMODECOMM, status);
	return dbus_message_new_method_return(msg);
}

/** lp command 実行**/
static DBusMessage *lp_comm(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	int ctl;
	char err;
	DBusMessage *reply;
	const short link_policy;
	uint8_t status;

	BLUEZ_DEBUG_INFO("[HCI] lp_comm() Start.");
	BLUEZ_DEBUG_LOG("[HCI_CMD_002_006] lp_comm Call");
	EVENT_DRC_BLUEZ_HCI_METHOD(FORMAT_ID_HCI_COMMAND, METHOD_HCI_LPCOMM);

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_INT16, &link_policy,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] dbus_message_get_args() return NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_LP, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[HCI] lp_comm() link_policy = %d.", link_policy);

	/* Open HCI socket  */
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open HCI socket.");
		BLUEZ_DEBUG_LOG("[HCI_CMD_013_002] lp_comm socket return NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_LP, 0, 0);
		return NULL;
	}

	/*lp command処理実行 */
	err = command_lp(ctl, &status, link_policy);
	BLUEZ_DEBUG_LOG("[HCI_CMD_013_003] lp_comm command_lp err = %d", err);

	/* Close HCI socket	*/
	close(ctl);
	BLUEZ_DEBUG_LOG("[HCI_CMD_013_004] lp_comm close");

	g_dbus_emit_signal(g_hci_connection, "/",
						SRC_HCICOMMAND_INTERFACE,
						"RET_LP",
						DBUS_TYPE_BYTE, &status,
						DBUS_TYPE_INVALID);

	BLUEZ_DEBUG_INFO("[HCI] lp_comm() End. status = %d", status);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HCI_LPCOMM, status);
	return dbus_message_new_method_return(msg);
}

/** addr command 実行**/
static DBusMessage *addr_comm(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	int ctl;
	char err;
	DBusMessage *reply;
	bdaddr_t bdaddr;
	uint8_t  status;
	char  BdAddr[BDADDR_SIZE];
	char *pBdAddr;

	BLUEZ_DEBUG_INFO("[HCI] addr_comm() Start.");
	BLUEZ_DEBUG_LOG("[HCI_CMD_002_007] addr_comm Call");
	EVENT_DRC_BLUEZ_HCI_METHOD(FORMAT_ID_HCI_COMMAND, METHOD_HCI_ADDRCOMM);

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] dbus_message_get_args() return NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_ADDR, 0, 0);
		return NULL;
	}

	/* Open HCI socket  */
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open HCI socket.");
		BLUEZ_DEBUG_LOG("[HCI_CMD_015_002] addr_comm socket return NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_ADDR, 0, 0);
		return NULL;
	}

	/*sspmode command処理実行 */
	err = command_addr(ctl, &status, &bdaddr);
	BLUEZ_DEBUG_LOG("[HCI_CMD_015_003] addr_comm command_addr err = %d", err);

	close(ctl);
	BLUEZ_DEBUG_LOG("[HCI_CMD_015_004] addr_comm close");

	memcpy( BdAddr, &bdaddr, sizeof( BdAddr ) );
	pBdAddr = &BdAddr[0];
	g_dbus_emit_signal(g_hci_connection, "/",
						SRC_HCICOMMAND_INTERFACE,
						"RET_ADDR",
						DBUS_TYPE_BYTE, &status,
						DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
						&pBdAddr, BDADDR_SIZE,
						DBUS_TYPE_INVALID);

	memcpy( &gBdAddr, &bdaddr, sizeof( bdaddr_t ) );

	BLUEZ_DEBUG_INFO("[HCI] addr_comm() End. status = %d", status);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HCI_ADDRCOMM, status);
	return dbus_message_new_method_return(msg);
;
}

/** dc command 実行**/
static DBusMessage *dc_comm(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	int ctl;
	char err;
	DBusMessage *reply;
	uint16_t handle;
	uint8_t reason;
	uint8_t status = 0x90;

	BLUEZ_DEBUG_INFO("[HCI] dc_comm() Start.");
	BLUEZ_DEBUG_LOG("[HCI_CMD_002_008] dc_comm Call");
	EVENT_DRC_BLUEZ_HCI_METHOD(FORMAT_ID_HCI_COMMAND, METHOD_HCI_DCCOMM);

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_UINT16, &handle,
						DBUS_TYPE_BYTE, &reason,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] dbus_message_get_args() return NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_DC, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG("[HCI] handle = 0x%02x.", handle);
	BLUEZ_DEBUG_LOG("[HCI] reason = 0x%04x.", reason);

	/* Open HCI socket  */
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open HCI socket.");
		BLUEZ_DEBUG_LOG("[HCI_CMD_017_002] dc_comm socket return NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_DC, 0, 0);
		return NULL;
	}

	/* dc command処理実行 */
	err = command_dc(ctl, handle, reason);
	BLUEZ_DEBUG_LOG("[HCI] command_dc err = %d", err);

	/* Close HCI socket	*/
	close(ctl);
	BLUEZ_DEBUG_LOG("[HCI_CMD_017_003] dc_comm close");

	if( err != 0 ) {
		status = HCI_NO_CONNECTION;
	}
	g_dbus_emit_signal(g_hci_connection, "/",
						SRC_HCICOMMAND_INTERFACE,
						"RET_DISC",
						DBUS_TYPE_BYTE, &status,
						DBUS_TYPE_INVALID);

	BLUEZ_DEBUG_INFO("[HCI] dc_comm() End. status = %d", status);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HCI_DCCOMM, status);
	BLUEZ_DEBUG_WARN("[HCI] HCI_Disconnect() handle = 0x%02x reason = 0x%04x status = %d", handle, reason, status);
	return dbus_message_new_method_return(msg);
}

/** vs codec config command 実行**/
static DBusMessage *vs_codec_comm(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	BLUEZ_DEBUG_INFO("[HCI] vs_codec_comm() Start.");
	EVENT_DRC_BLUEZ_HCI_METHOD(FORMAT_ID_HCI_COMMAND, METHOD_HCI_VSCODECCOMM);

	int ctl;
	char err;
	DBusMessage *reply;
	uint8_t mode;
	uint8_t status;

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_BYTE, &mode,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] dbus_message_get_args() ERROR");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_VSCODEC, 0, 0);
		return NULL;
	}
	BLUEZ_DEBUG_LOG(" [HCI] vs_codec_comm() mode = %x.", mode);

	/* Open HCI socket  */
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		BLUEZ_DEBUG_ERROR(" [Bluez - HCI] Can't open HCI socket.");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_VSCODEC, 0, 0);
		return NULL;
	}

	/*vs codec config command処理実行 */
	err = command_vs_codec(ctl, &status, mode);

	/* Close HCI socket	*/
	close(ctl);

	g_dbus_emit_signal(g_hci_connection, "/",
						SRC_HCICOMMAND_INTERFACE,
						"RET_VS_CODEC",
						DBUS_TYPE_BYTE, &status,
						DBUS_TYPE_INVALID);

	BLUEZ_DEBUG_INFO("[HCI] vs_codec_comm() End. status = %d", status);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HCI_VSCODECCOMM, status);
	return dbus_message_new_method_return(msg);
}

/** vs wbs disassociate command 実行**/
static DBusMessage *vs_wbs_disassociate_comm(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	int ctl;
	int err;
	DBusMessage *reply;
	uint8_t  status;

	BLUEZ_DEBUG_INFO("[HCI] vs_wbs_disassociate_comm() Start.");
	BLUEZ_DEBUG_LOG("[HCI_CMD_002_007] vs_wbs_disassociate_comm Call");
	EVENT_DRC_BLUEZ_HCI_METHOD(FORMAT_ID_HCI_COMMAND, METHOD_HCI_VSWBSDISASSOCIATE);

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] dbus_message_get_args() return NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_VSWBSDISASSOCIATE, 0, 0);
		return NULL;
	}

	/* Open HCI socket  */
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open HCI socket.");
		BLUEZ_DEBUG_LOG("[HCI_CMD_015_002] vs_wbs_disassociate_comm socket return NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_VSWBSDISASSOCIATE, 0, 0);
		return NULL;
	}

	/*sspmode command処理実行 */
	err = command_vs_wbs_disassociate(ctl, &status);
	BLUEZ_DEBUG_LOG("[HCI_CMD_015_003] vs_wbs_disassociate_comm err = %d", err);

	close(ctl);
	BLUEZ_DEBUG_LOG("[HCI_CMD_015_004] vs_wbs_disassociate_comm close");

	g_dbus_emit_signal(g_hci_connection, "/",
						SRC_HCICOMMAND_INTERFACE,
						"RET_VS_WBS_DISASSOCIATE",
						DBUS_TYPE_BYTE, &status,
						DBUS_TYPE_INVALID);

	BLUEZ_DEBUG_INFO("[HCI] vs_wbs_disassociate_comm() End. status = %d", status);
	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HCI_VSWBSDISASSOCIATE, status);
	return dbus_message_new_method_return(msg);
}

/** send command 実行**/
static DBusMessage *snd_cmd(DBusConnection *conn,
					DBusMessage *msg, void *data)
{
	int ctl;
	char err;
	DBusMessage *reply;
	char *packet;
	uint8_t status;
	uint32_t len;
	unsigned short i;

	BLUEZ_DEBUG_INFO("[HCI] snd_cmd() Start.");
	BLUEZ_DEBUG_LOG("[HCI_CMD_002_002] snd_cmd Call");

	if (!dbus_message_get_args(msg, NULL,
						DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
						&packet, &len,
						DBUS_TYPE_INVALID)){
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] dbus_message_get_args() return NULL");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_EVTMASK, 0, 0);
		return dbus_message_new_method_return(msg);;
	}
	
	if ( packet )
	{
		for( i=0; i<len; i++ ) {
			BLUEZ_DEBUG_LOG("[HCI] snd_cmd() packet[%02d] = 0x%02x", i, *(packet + i));
		}
	}

	/* Open HCI socket  */
	if ((ctl = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open HCI socket. ");
		BLUEZ_DEBUG_LOG("[HCI_CMD_005_003] snd_cmd socket return NULL; ");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_EVTMASK, 0, 0);
		return dbus_message_new_method_return(msg);;
	}

	/*send_cmd処理実行 */
	err = command_send(ctl, &status, packet, len);
	BLUEZ_DEBUG_LOG("[HCI_CMD_005_004] snd_cmd command_send err = %d", err);
	/* Close HCI socket	*/
	close(ctl);
	BLUEZ_DEBUG_INFO("[HCI_CMD_005_005] snd_cmd close");


/* domy: */
	BLUEZ_DEBUG_INFO("[HCI] snd_cmd() End. status = %d", status);
	
//	EVENT_DRC_BLUEZ_SIGNAL(FORMAT_ID_DBUS_SIGNAL, SIGNAL_HCI_EVTMASKCOMM, status);
	
	return dbus_message_new_method_return(msg);
}

static void print_dev_hdr(struct hci_dev_info *di)
{
	BLUEZ_DEBUG_INFO("[HCI] print_dev_hdr() Start.");
	static int hdr = -1;
	char addr[18];

	if (hdr == di->dev_id)
	{
		BLUEZ_DEBUG_LOG("[HCI_CMD_018_001] print_dev_hdr return");
		return;
	}
	hdr = di->dev_id;

	ba2str(&di->bdaddr, addr);
	BLUEZ_DEBUG_LOG("[HCI_CMD_018_002] print_dev_hdr ba2str");

	printf("%s:\tType: %s  Bus: %s", di->name,
					hci_typetostr(di->type >> 4),
					hci_bustostr(di->type & 0x0f));
	printf("\tBD Address: %s  ACL MTU: %d:%d  SCO MTU: %d:%d",
					addr, di->acl_mtu, di->acl_pkts,
						di->sco_mtu, di->sco_pkts);
	BLUEZ_DEBUG_INFO("[HCI] print_dev_hdr() End.");
}

static GDBusMethodTable hciCommand_methods[] = {
	{ "class",					"u",	"",		class_comm,					0,	0	},
	{ "mask",					"au",	"",		evtmask_comm,				0,	0	},
	{ "name",					"s",	"",		name_comm,					0,	0	},
	{ "version",				"",		"",		version_comm,				0,	0	},
	{ "sspmode",				"y",	"",		sspmode_comm,				0,	0	},
	{ "lp", 					"n",	"",		lp_comm,					0,	0	},
	{ "addr",					"",		"",		addr_comm,					0,	0	},
	{ "disc", 					"qy",	"",		dc_comm,					0,	0	},
	{ "vs_codec",				"y",	"",		vs_codec_comm,				0,	0	},
	{ "vs_wbs_disassociate",	"",		"",		vs_wbs_disassociate_comm,	0,	0	},
	{ "sndcmd",					"au",	"",		snd_cmd,					0,	0	},
	{ NULL,						NULL,	NULL,	NULL,						0,	0	}
};

static GDBusSignalTable hciCommand_signals[] = {
	{ "CONNECT_CFM",				"bs"		,G_DBUS_SIGNAL_FLAG_DEPRECATED	},
	{ "RET_CLASS",					"y"			,G_DBUS_SIGNAL_FLAG_DEPRECATED	},
	{ "RET_MASK",					"y"			,G_DBUS_SIGNAL_FLAG_DEPRECATED	},
	{ "RET_NAME",					"y"			,G_DBUS_SIGNAL_FLAG_DEPRECATED	},
	{ "RET_VERSION",				"qqqyyy"	,G_DBUS_SIGNAL_FLAG_DEPRECATED	},
	{ "RET_SSPMODE",				"y"			,G_DBUS_SIGNAL_FLAG_DEPRECATED	},
	{ "RET_LP",						"y"			,G_DBUS_SIGNAL_FLAG_DEPRECATED	},
	{ "RET_ADDR",					"yay"		,G_DBUS_SIGNAL_FLAG_DEPRECATED	},
	{ "RET_DISC",					"y"			,G_DBUS_SIGNAL_FLAG_DEPRECATED	},
	{ "RET_VS_CODEC",				"y"			,G_DBUS_SIGNAL_FLAG_DEPRECATED	},
	{ "RET_VS_WBS_DISASSOCIATE",	"y"			,G_DBUS_SIGNAL_FLAG_DEPRECATED	},
	{ NULL,							NULL		,G_DBUS_SIGNAL_FLAG_DEPRECATED	}
};


int hcicommand_init(void)
{
	BLUEZ_DEBUG_INFO("[HCI] hcicommand_init() Start");

	if (g_hci_connection != NULL  ) {
		g_dbus_unregister_interface(g_hci_connection, "/", SRC_HCICOMMAND_INTERFACE);
		g_hci_connection = NULL;
	}

	g_hci_connection = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);

	EVENT_DRC_BLUEZ_INIT(FORMAT_ID_BT_INIT, INIT_DBUS_HCICMD);
	BLUEZ_DEBUG_LOG("[HCI_CMD_001_001] hcicommand_init g_hci_connection = %p", g_hci_connection);
	if (g_dbus_register_interface(g_hci_connection, "/",
					SRC_HCICOMMAND_INTERFACE,
					hciCommand_methods, hciCommand_signals,
					NULL, NULL, NULL) == FALSE) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] g_dbus_register_interface() ERROR ");
		BLUEZ_DEBUG_LOG("[HCI_CMD_001_002] hcicommand_init g_dbus_register_interface Err");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_DBUS_INIT, 0, 0);
		return -1;
	}
	BLUEZ_DEBUG_LOG("[HCI] hcicommand_init()->g_dbus_register_interface().SUCCESS ");
	BLUEZ_DEBUG_LOG("[HCI_CMD_001_003] hcicommand_init return 0");

	dev_id = HCI_get_device_id();
	BLUEZ_DEBUG_LOG("[HCI]  dev_id = %d", dev_id);
	
	BLUEZ_DEBUG_INFO("[HCI] hcicommand_init() End");
	return 0;
}

void bluezutil_get_bdaddr( bdaddr_t *pbdaddr )
{
	if( NULL != pbdaddr )
	{
		memcpy( pbdaddr, &gBdAddr, sizeof( bdaddr_t ) );
	}
	return;
}

int bluezutil_get_role( bdaddr_t *pbdaddr, uint8_t type )
{
	struct hci_role_info_req *rr;
	int ctl;
	int role = 0;
	int dd;
	int err;

	BLUEZ_DEBUG_INFO("[HCI] bluezutil_get_role() Start.");

	dd = hci_open_dev(dev_id);
	if (dd < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open device hci%d: %s (%d)\n",	dev_id, strerror(errno), errno);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_GETROLE, 0, 0);
		return 0;
	}

	rr = g_malloc0(sizeof(*rr) + sizeof(struct hci_role_info_req));
	bacpy(&rr->bdaddr, pbdaddr);
	rr->type = type;

	err = ioctl(dd, HCIGETROLE, rr);
	if (err < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't get Role info");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_GETROLE, 0, 0);
	}
	role = rr->role;
	g_free(rr);

	hci_close_dev(dd);
							 
	BLUEZ_DEBUG_INFO("[HCI] bluezutil_get_role() End. Role = %d", role);

	return role;
}

int bluezutil_accept_conn_req(bdaddr_t *pbdaddr, uint8_t *dev_class, uint8_t link_type )

{
	int dd;
	struct hci_ac_conn_req req;

	BLUEZ_DEBUG_INFO("[HCI] bluezutil_accept_conn_req() Start.");

	dd = hci_open_dev(dev_id);
	if (dd < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open device hci%d: %s (%d)\n",	dev_id, strerror(errno), errno);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_ACCON, 0, 0);
		return 0;
	}

	memcpy( &req.bdaddr, pbdaddr, sizeof( bdaddr_t ) );
	memcpy( &req.dev_class, dev_class, sizeof( req.dev_class ) );
	req.link_type = link_type;

	if (ioctl(dd, HCISETACCON, (void *) &req)) {
		BLUEZ_DEBUG_ERROR(" [Bluez - HCI] Can't get device info");
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_ACCON, 0, 0);
	}

	hci_close_dev(dd);

	BLUEZ_DEBUG_LOG("[HCI] command_ac_con_req End");

	return 1;
}

int bluezutil_reject_conn_req(bdaddr_t *pbdaddr, uint8_t reason)
{
	int dd;

	BLUEZ_DEBUG_INFO("[HCI] bluezutil_reject_conn_req() Start.");

	dd = hci_open_dev(dev_id);
	if (dd < 0) {
		BLUEZ_DEBUG_ERROR("[Bluez - HCI] Can't open device hci%d: %s (%d)\n", dev_id, strerror(errno), errno);
		ERR_DRC_BLUEZ(ERROR_BLUEZ_HCI_CMD_RJCON, 0, 0);
		return 0;
	}

	hci_reject_conn_req(dd, pbdaddr, reason, 1000);

	hci_close_dev(dd);

	BLUEZ_DEBUG_LOG("[HCI] command_rj_con_req End");

	return 1;
}
