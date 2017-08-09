/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2006-2010  Nokia Corporation
 *  Copyright (C) 2004-2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
/*----------------------------------------------------------------------------*/
// COPYRIGHT(C) FUJITSU TEN LIMITED 2012
/*----------------------------------------------------------------------------*/

struct enabled_interfaces {
	gboolean hfp;
	gboolean headset;
	gboolean gateway;
	gboolean sink;
	gboolean source;
	gboolean control;
	gboolean socket;
	gboolean media;
};

int audio_manager_init(DBusConnection *conn, GKeyFile *config,
							gboolean *enable_sco);
void audio_manager_exit(void);

gboolean server_is_enabled(bdaddr_t *src, uint16_t svc);

struct audio_device *manager_find_device(const char *path,
					const bdaddr_t *src,
					const bdaddr_t *dst,
					const char *interface,
					gboolean connected);

struct audio_device *manager_get_device(const bdaddr_t *src,
					const bdaddr_t *dst,
					gboolean create);

gboolean manager_allow_headset_connection(struct audio_device *device);
gboolean manager_allow_handsfree_connection(struct audio_device *device);
gboolean manager_allow_pbap_connection(struct audio_device *device);
gboolean manager_allow_a2dpad_connection(struct audio_device *device, const bdaddr_t *dst);
gboolean manager_allow_avrcpad_connection(struct audio_device *device, const bdaddr_t *dst);
gboolean manager_allow_browsing_connection(struct audio_device *device, const bdaddr_t *dst);
gboolean manager_register_browsing( bdaddr_t *bdaddr, uint16_t browsingPsm );
gboolean manager_deregister_browsing( bdaddr_t *bdaddr, uint16_t browsingPsm );

/* TRUE to enable fast connectable and FALSE to disable fast connectable for all
 * audio adapters. */
void manager_set_fast_connectable(gboolean enable);

void btd_manager_set_connection_hfp( DBusConnection * con );
void btd_manager_set_connection_pbap( DBusConnection * con );
void btd_manager_set_connection_a2dp( DBusConnection * con );
void btd_manager_set_connection_avrcp( DBusConnection * con );
void btd_manager_set_connection_browsing( DBusConnection * con );
int btd_manager_isconnect( uint16_t profile );
void btd_manager_get_pbap_rfcommCh( bdaddr_t *bdaddr, int *chan );
void btd_manager_set_pbap_rfcommCh( bdaddr_t *bdaddr, int chan );

#define MNG_RFCOMM_PSM		0x003
#define MNG_AVCTP_PSM		0x017
#define MNG_AVDTP_PSM		0x019
#define MNG_BROWSING_PSM	0x01b

#define MNG_CONNECT_CFM		0
#define MNG_CONNECTED		1
#define MNG_DISCONNECT_CFM	2
#define MNG_DISCONNECTED	3
