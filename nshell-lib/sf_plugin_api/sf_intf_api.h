/*
 *------------------------------------------------------------------
 * Copyright (c) 2019 Asterfusion.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *------------------------------------------------------------------
 */
/* 1 */
#ifndef __include_sf_intf_api_h__
#define __include_sf_intf_api_h__

#include <stdint.h>

#undef IN_NSHELL_LIB
#define IN_NSHELL_LIB

/* 2 */
#include "sf_interface.h"
#include "sf_interface_shm.h"
#include "acl_defines.h"

#undef foreach_member
#define foreach_member                                               \
    FUNC(uint8_t, interface_type, 0, 0, 2)                           \
    FUNC(uint8_t, ipaddr_mask, 0, 0, 32)                            
#undef FUNC

#undef foreach_member_ingress
#define foreach_member_ingress                                       \
    FUNC(uint8_t, acl_rule_group, 1, 1, MAX_RULE_GROUP_CNT)          \
    FUNC(uint8_t, default_action_id, 0, 0, MAX_ACTION_COUNT)         
#undef FUNC

#undef foreach_phy_info_member
#define foreach_phy_info_member                                       \
    FUNC(link) \
    FUNC(speed) \
    FUNC(duplex) \
    FUNC(mtu) 
#undef FUNC



#define FUNC(a, b, c, d, e) int get_interface_##b(int index);
foreach_member
#undef FUNC

#define FUNC(a, b, c, d, e) int set_interface_##b(int index , a value);
foreach_member
#undef FUNC

#define FUNC(a, b, c, d, e) int get_ingress_##b(int index);
foreach_member_ingress
#undef FUNC

#define FUNC(a, b, c, d, e) int set_ingress_##b(int index , a value);
foreach_member_ingress
#undef FUNC


RESET_CONFIG_FUNC_DECLARE
// GET_STAT_PTR_FUNC_DECLARE
// CLEAN_STAT_FUNC_DECLARE

/* 4 */
/** alias name **/
int  get_interface_alias_name(int index, char *alias , int max_str_len);
int set_interface_alias_name(int index, char *alias );
/** mac addr **/
int  get_interface_mac_addr(int index, char *mac_str , int max_str_len);
int set_interface_mac_addr(int index, char *mac_array );
/** ip **/
int  get_interface_ipv4(int index, char *ip_str , int str_max_len);
int  set_interface_ipv4(int index, char *ip_str);
int  get_interface_ipv6(int index, char *ip_str , int str_max_len);
int  set_interface_ipv6(int index, char *ip_str);
/** gateway **/
int  get_interface_ipv4_gateway(int index, char *ip_str , int str_max_len);
int  set_interface_ipv4_gateway(int index, char *ip_str);
int  get_interface_ipv6_gateway(int index, char *ip_str , int str_max_len);
int  set_interface_ipv6_gateway(int index, char *ip_str);
/*** rule to action ***/
int get_ingress_rule_to_action(int index , int rule_id);
int set_ingress_rule_to_action(int index , int rule_id , sf_action_index_t action_id);
/*** vlan access list ***/
int get_ingress_vlan_access_list(int index , int vlan_id);
int set_ingress_vlan_access_list(int index , int vlan_id , int enable);
/*** decode_vxlan_port***/
int get_ingress_decode_vxlan_port(int index, uint16_t *array , int len);
uint32_t set_ingress_decode_vxlan_port(int index, uint16_t *value_array, int len);

/**** init & reset ****/
int reset_single_interface_config(int index);
/*****  stat ******/
/* get */
single_interface_stat_t *get_interface_stat_ptr(int index);
/* clean */
int clean_interface_stat(int index);

/*****  stat ******/
int get_interfaces_status_link(int index);
int get_interfaces_status_service_link(int index);
#endif
