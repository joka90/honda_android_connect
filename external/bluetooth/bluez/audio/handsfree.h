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


#define AUDIO_HANDSFREE_INTERFACE "org.bluez.HandsFree"

typedef enum {
	HANDSFREE_STATE_DISCONNECTED,
	HANDSFREE_STATE_CONNECTING,
	HANDSFREE_STATE_CONNECT_AFTER_SDP,
	HANDSFREE_STATE_CONNECTED,
	HANDSFREE_STATE_PLAY_IN_PROGRESS,
	HANDSFREE_STATE_PLAYING
} handsfree_state_t;

typedef void (*handsfree_state_cb) (struct audio_device *dev,
					handsfree_state_t old_state,
					handsfree_state_t new_state,
					void *user_data);
typedef void (*handsfree_stream_cb_t) (struct audio_device *dev, void *user_data);

void handsfree_connect_cb(GIOChannel *chan, GError *err, gpointer user_data);
void handsfree_set_state(struct audio_device *dev, handsfree_state_t state);

struct handsfree *handsfree_init(struct audio_device *dev);
void handsfree_unregister(struct audio_device *dev);
void handsfree_shutdown(struct audio_device *dev);
int handsfree_dbus_init(void);
handsfree_state_t handsfree_get_state(struct audio_device *dev);
int handsfree_connect_sco(struct audio_device *dev, GIOChannel *io);
gboolean get_handsfree_active(struct audio_device *dev);
void set_handsfree_active(struct audio_device *dev, gboolean active);
