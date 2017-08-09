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


#define AUDIO_AVRCPAD_INTERFACE "org.bluez.AvrcpAD"

typedef enum {
	AVRCP_STATE_DISCONNECTED = 0,
	AVRCP_STATE_CONNECTING,
	AVRCP_STATE_CONNECTED
} avrcpad_state_t;

struct avrcpad  *avrcpad_init(struct audio_device *dev);
void avrcpad_unregister(struct audio_device *dev);
int avrcpad_dbus_init(void);
avrcpad_state_t avrcpad_get_state(struct audio_device *dev);
uint16_t avrcpad_get_cid(void);
int avrcpad_get_disconnect_initiator(void);
void avrcpad_set_disconnect_initiator(int flg);
