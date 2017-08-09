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


#define AUDIO_A2DPAD_INTERFACE "org.bluez.A2dpAD"

typedef enum {
	A2DPAD_STATE_DISCONNECTED = 0,
	A2DPAD_STATE_CONNECTING_1,
	A2DPAD_STATE_CONNECTING_2,
	A2DPAD_STATE_CONNECTED
} a2dpad_state_t;

typedef struct wL2CAP_CONFIG_PARAM{
		uint16_t	wFlags;	
		uint16_t	wMtu;
		uint16_t	wFlushTo;
		uint16_t	wLinkTo;
		uint8_t		bServiceType;
		uint32_t	dwTokenRate;
		uint32_t	dwTokenBucketSize;
		uint32_t	dwPeakBandwidth;
		uint32_t	dwLatency;
		uint32_t	dwDelayVariation;
		uint8_t		bMode;
		uint8_t		bTxWindowSize;
		uint8_t		bMaxTransmit;
		uint16_t	wRetTimeout;
		uint16_t	wMonTimeout;
		uint16_t	wMps;
		uint8_t		bFcsType;
} L2CAP_CONFIG_PARAM;

/* Default Value */
#define	L2CAP_CONFIG_NO_SERVICE_TYPE		0xFF
#define	L2CAP_FLOW_MODE_BASIC				0x00
#define L2CAP_FCS_TYPE_16BIT_FCS			0x01
#define L2CAP_CONFIG_DEF_FLAGS				0x0000
#define	L2CAP_CONFIG_DEF_MTU				0x02A0
#define	L2CAP_CONFIG_DEF_FLUSH_TO			0xFFFF
#define	L2CAP_CONFIG_DEF_LINK_TO			0x4E20
#define	L2CAP_CONFIG_DEF_SERVICE_TYPE		(L2CAP_CONFIG_NO_SERVICE_TYPE)
#define	L2CAP_CONFIG_DEF_TOKEN_RATE			0x00000000
#define	L2CAP_CONFIG_DEF_TOKEN_BUCKET_SIZE	0x00000000
#define	L2CAP_CONFIG_DEF_PEAK_BANDWIDTH		0x00000000
#define	L2CAP_CONFIG_DEF_LATENCY			0xFFFFFFFF
#define	L2CAP_CONFIG_DEF_DELAY_VARIATION	0xFFFFFFFF
#define L2CAP_CONFIG_DEF_MODE_FLAGS			L2CAP_FLOW_MODE_BASIC
#define L2CAP_CONFIG_DEF_TX_WINDOW_SIZE		1
#define L2CAP_CONFIG_DEF_MAX_TRANSMIT		2
#define L2CAP_CONFIG_DEF_RET_TIMEOUT		2000
#define L2CAP_CONFIG_DEF_MON_TIMEOUT		12000
#define L2CAP_CONFIG_DEF_MPS				L2CAP_CONFIG_DEF_MTU
#define L2CAP_CONFIG_DEF_FCS				L2CAP_FCS_TYPE_16BIT_FCS

struct a2dpad *a2dpad_init(struct audio_device *dev);
void a2dpad_unregister(struct audio_device *dev);
int a2dpad_dbus_init(void);
void l2capSetCfgDefParam( L2CAP_CONFIG_PARAM *cfgParam );
a2dpad_state_t a2dpad_get_state(struct audio_device *dev);

