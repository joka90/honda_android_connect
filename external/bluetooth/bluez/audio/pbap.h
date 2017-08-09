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


#define OBEX_PBAP_INTERFACE "org.bluez.Pbap"

typedef enum {
	PBAP_STATE_DISCONNECTED,
	PBAP_STATE_CONNECTING,
	PBAP_STATE_CONNECTED
} pbap_state_t;

void pbap_set_state(struct audio_device *dev, pbap_state_t state);
struct pbap *pbap_init(struct audio_device *dev);
void pbap_unregister(struct audio_device *dev);
void pbap_shutdown(struct audio_device *dev);
int pbap_dbus_init(void);
pbap_state_t pbap_get_state(struct audio_device *dev);
int pbap_get_channel(struct audio_device *dev);
int pbap_get_disconnect_initiator(void);
void pbap_set_disconnect_initiator(int flg);
