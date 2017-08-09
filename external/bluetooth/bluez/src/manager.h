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

#include <bluetooth/bluetooth.h>
#include <dbus/dbus.h>

#define MANAGER_INTERFACE "org.bluez.Manager"

#define MNG_CON_HANDSFREE	0x0001
#define MNG_CON_A2DP_CTRL	0x0002
#define MNG_CON_A2DP_STRM	0x0004
#define MNG_CON_AVRCP		0x0008
#define MNG_CON_BROWS		0x0010
#define MNG_CON_PBAP		0x0020

#define MNG_ACL_MAX			7

typedef void (*adapter_cb) (struct btd_adapter *adapter, gpointer user_data);

dbus_bool_t manager_init(DBusConnection *conn, const char *path);
void manager_cleanup(DBusConnection *conn, const char *path);

const char *manager_get_base_path(void);
struct btd_adapter *manager_find_adapter(const bdaddr_t *sba);
struct btd_adapter *manager_find_adapter_by_id(int id);
void manager_foreach_adapter(adapter_cb func, gpointer user_data);
GSList *manager_get_adapters(void);
struct btd_adapter *btd_manager_register_adapter(int id);
int btd_manager_unregister_adapter(int id);
void manager_add_adapter(const char *path);
void btd_manager_set_did(uint16_t vendor, uint16_t product, uint16_t version);

void btd_manager_receive_cmd_status( uint8_t status, uint8_t ncmd, uint16_t opcode );
void btd_manager_receive_conn_complete( uint8_t status, uint16_t handle, bdaddr_t *bdaddr,
										uint8_t link_type, uint8_t encr_mode );
void btd_manager_fake_signal_conn_complete( bdaddr_t *bdaddr );
void btd_manager_receive_disconn_complete( uint8_t status, uint16_t handle, uint8_t reason );
void btd_manager_receive_conn_request( bdaddr_t *bdaddr, uint8_t * cod, uint8_t linktype );
void btd_manager_receive_sync_conn_complete( uint8_t status,  uint16_t handle, bdaddr_t *bdaddr,
											 uint8_t link_type, uint8_t trans_interval, 
											 uint8_t retrans_window, uint16_t rx_pkt_len, 
											 uint16_t tx_pkt_len, uint8_t air_mode );
void btd_manager_receive_sync_conn_changed( uint8_t status, uint16_t handle, 
											uint8_t trans_interval, uint8_t retrans_window, 
											uint16_t rx_pkt_len, uint16_t tx_pkt_len );
void btd_manager_disconnected_profile( bdaddr_t *bdaddr, uint16_t profile, uint16_t value );
void btd_manager_connect_profile( bdaddr_t *bdaddr, uint16_t profile, uint8_t chan, uint16_t cid );
uint16_t btd_manager_judgment_rfcomm( bdaddr_t *bdaddr, uint8_t chan, uint8_t dir );
void btd_manager_disconnect_cfm_profile_rfcomm( bdaddr_t *bdaddr, uint16_t value, uint8_t chan, uint16_t profile );
void btd_manager_disconnected_profile_rfcomm( bdaddr_t *bdaddr, uint16_t value, uint8_t chan, uint16_t profile );
void btd_manager_disconnect_cfm_profile_l2cap( bdaddr_t *bdaddr, uint16_t value, uint16_t cid );
void btd_manager_disconnected_profile_l2cap( bdaddr_t *bdaddr, uint16_t value, uint16_t cid );
void btd_manager_connect_unknown_profile_rfcomm( bdaddr_t *bdaddr, uint16_t cid );
void btd_manager_disconnect_unknown_profile_rfcomm( bdaddr_t *bdaddr, uint16_t cid );
void btd_manager_connect_unknown_profile_l2cap( bdaddr_t *bdaddr, uint16_t psm );
void btd_manager_disconnect_unknown_profile_l2cap( bdaddr_t *bdaddr, uint16_t psm );
int btd_manager_find_connect_info_by_bdaddr( bdaddr_t *bdaddr );
void btd_manager_handsfree_signal_disconnect_cfm( uint16_t value );
void btd_manager_pbap_signal_disconnect_cfm( uint16_t value );
void btd_manager_a2dp_signal_disconnect_cfm( uint16_t value, uint16_t cid );
void btd_manager_avrcp_signal_disconnect_cfm( uint16_t value, uint16_t cid );
void btd_manager_browsing_signal_disconnect_cfm( uint16_t value, uint16_t cid );
int btd_manager_get_acl_num( void );
int btd_manager_get_acl_ope( void );
void btd_manager_set_acl_ope( int ope );
void btd_manager_set_acl_guard( int ope );
int btd_manager_get_acl_guard( void );
int btd_manager_check_acl( bdaddr_t *bdaddr );
void btd_manager_set_reject_info( bdaddr_t *bdaddr );
void btd_manager_receive_hci_event_packet( uint8_t * data, int len );
uint8_t btd_manager_find_reject_info( bdaddr_t *bdaddr );
uint8_t btd_manager_get_disconnect_info( bdaddr_t *bdaddr, uint16_t profile );
void btd_manager_set_state_info( bdaddr_t *bdaddr, uint16_t profile, int state );
