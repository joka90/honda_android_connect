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


#define SRC_HCICOMMAND_INTERFACE "org.bluez.HciCommand"

int hcicommand_init(void);

typedef enum {
	HCI_CMD_NONE = 0,
	HCI_CMD_CLASS,
	A2DP_CMD_CONFIGURE,
	A2DP_CMD_START,
	A2DP_CMD_STOP,
	A2DP_CMD_QUIT,
} hci_command_t;

void bluezutil_get_bdaddr( bdaddr_t *pbdaddr );
int bluezutil_get_role( bdaddr_t *pbdaddr, uint8_t type );
int bluezutil_accept_conn_req(bdaddr_t *pbdaddr, uint8_t *dev_class, uint8_t link_type );
int bluezutil_reject_conn_req(bdaddr_t *pbdaddr, uint8_t reason);
