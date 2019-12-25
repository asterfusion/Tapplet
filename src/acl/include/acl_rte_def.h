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
#ifndef __include_acl_rte_def_h__
#define __include_acl_rte_def_h__


#include <rte_acl.h>

#include "pkt_wdata.h"

#define sf_rte_rule_priority(rule_id) (0xFFFFFF - (rule_id))


/*************************************************/
/************  IPv46 *****************************/
/*************************************************/
enum {
    PROTO_FIELD_IPV46,
	SIP1_FIELD_IPV46,
	SIP2_FIELD_IPV46,
	SIP3_FIELD_IPV46,
	SIP4_FIELD_IPV46,
	DIP1_FIELD_IPV46,
	DIP2_FIELD_IPV46,
	DIP3_FIELD_IPV46,
	DIP4_FIELD_IPV46,
    SPORT_FIELD_IPV46,
    DPORT_FIELD_IPV46,

	NUM_FIELDS_IPV46
};


#define BASIC_L2_OFFSET_IPv46 ( offsetof(sf_wdata_t , outer_vlan_id) )
#define BASIC_L3L4_OFFSET_IPv46 ( offsetof(sf_wdata_t , l3l4_wdatas) )
#define BASIC_L3_OFFSET_IPv46 ( offsetof(l3l4_decode_t , ip_wdata) )
#define BASIC_L4_OFFSET_IPv46 ( offsetof(l3l4_decode_t , l4_wdata) )

#define PROTO_FIELD_IPV46_offset ( BASIC_L3L4_OFFSET_IPv46 + BASIC_L3_OFFSET_IPv46  + offsetof(ip_wdata_t , protocol))

#define SIP1_FIELD_IPV46_offset  ( BASIC_L3L4_OFFSET_IPv46 + BASIC_L3_OFFSET_IPv46  + offsetof(ip_wdata_t , sip)     )
#define SIP2_FIELD_IPV46_offset  ( BASIC_L3L4_OFFSET_IPv46 + BASIC_L3_OFFSET_IPv46  + offsetof(ip_wdata_t , sip) + 4 )
#define SIP3_FIELD_IPV46_offset  ( BASIC_L3L4_OFFSET_IPv46 + BASIC_L3_OFFSET_IPv46  + offsetof(ip_wdata_t , sip) + 8 )
#define SIP4_FIELD_IPV46_offset  ( BASIC_L3L4_OFFSET_IPv46 + BASIC_L3_OFFSET_IPv46  + offsetof(ip_wdata_t , sip) + 12)

#define DIP1_FIELD_IPV46_offset  ( BASIC_L3L4_OFFSET_IPv46 + BASIC_L3_OFFSET_IPv46  + offsetof(ip_wdata_t , dip)     )
#define DIP2_FIELD_IPV46_offset  ( BASIC_L3L4_OFFSET_IPv46 + BASIC_L3_OFFSET_IPv46  + offsetof(ip_wdata_t , dip) + 4 )
#define DIP3_FIELD_IPV46_offset  ( BASIC_L3L4_OFFSET_IPv46 + BASIC_L3_OFFSET_IPv46  + offsetof(ip_wdata_t , dip) + 8 )
#define DIP4_FIELD_IPV46_offset  ( BASIC_L3L4_OFFSET_IPv46 + BASIC_L3_OFFSET_IPv46  + offsetof(ip_wdata_t , dip) + 12)


#define SPORT_FIELD_IPV46_offset ( BASIC_L3L4_OFFSET_IPv46 + BASIC_L4_OFFSET_IPv46  + offsetof(l4_wdata_t , sport) )
#define DPORT_FIELD_IPV46_offset ( BASIC_L3L4_OFFSET_IPv46 + BASIC_L4_OFFSET_IPv46  + offsetof(l4_wdata_t , dport) )


#endif