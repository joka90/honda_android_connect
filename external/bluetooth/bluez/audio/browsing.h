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


#define AUDIO_BROWSING_INTERFACE "org.bluez.Browsing"

typedef enum {
	BROWSING_STATE_DISCONNECTED = 0,
	BROWSING_STATE_CONNECTING,
	BROWSING_STATE_CONNECTED
} browsing_state_t;

struct browsing  *browsing_init(struct audio_device *dev);
void browsing_unregister(struct audio_device *dev);
int browsing_dbus_init(void);
browsing_state_t browsing_get_state(struct audio_device *dev);
uint16_t browsing_get_cid(void);
int browsing_get_disconnect_initiator(void);
void browsing_set_disconnect_initiator(int flg);
gboolean register_l2cap_browsing( bdaddr_t *bdaddr, uint16_t browsingPsm );
gboolean deregister_l2cap_browsing( bdaddr_t *bdaddr, uint16_t browsingPsm );
