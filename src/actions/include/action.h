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
#ifndef  __ACTIONS_H__          
#define  __ACTIONS_H__

#include <vlib/vlib.h>
#include "stdint.h"                 
#include "common_header.h"

#include "sf_intf_define.h"


#define MAX_LOAD_BALANCE_NODE    64

#define INVALID_ACTION_ID        0
#define INVALID_INTERFACE_ID     0
#define INVALID_ELAG_ID          0
#define INVALID_VLAN_ID          0

typedef  uint32_t  action_type_t;


typedef enum
{
    NONE_TYPE,
    INTERFACE = 1,
} output_type_t;

typedef struct _load_balance_data_t
{
    ip_addr_t src_ip;
    ip_addr_t dst_ip;

    ip_addr_t inner_sip;
    ip_addr_t inner_dip;

    uint64_t  packet_type;
    uint8_t   inner_exist;
} load_balance_data_t;

typedef enum
{
    LOAD_BALANCE_MODE_NONE,
    LOAD_BALANCE_MODE_ROUND_ROBIN,
    LOAD_BALANCE_MODE_WRR,
    LOAD_BALANCE_MODE_OUTER_SRCDSTIP,
    LOAD_BALANCE_MODE_OUTER_SRCIP,
    LOAD_BALANCE_MODE_OUTER_DSTIP,

    LOAD_BALANCE_MODE_COUNT
} load_balance_mode_t;

typedef struct _load_balance_weigth_t
{
    int cw; // current weight
    int gcd; // greatest common divisor
    int mw; // max weight
} load_balance_weight_t;

/*additional action switch flag*/
typedef union
{
    struct
    {
        uint32_t flag_gre_encapsulation:1;
        uint32_t flag_vxlan_encapsulation:1;
        uint32_t flag_erspan_encapsulation:1;
    };
    uint32_t switch_flags;
} additional_action_switch_t;

/*      !!!!!!!!! notice!!!!!!!!!!!                               */
/* When modifying the struct additional_action_t,                 */
/* the tmp_action_switch_flags  struct not needs to be synchronized   */
typedef struct _additional_action_t
{
    /*additional switch cfg_*/
    additional_action_switch_t  additional_switch;

    /*gre */
    uint8_t gre_dmac[6];
    uint8_t gre_dip_type;                   //only internal use
    ip_addr_t gre_dip;
    uint8_t gre_dscp;

    /*vxlan */
    uint8_t vxlan_dmac[6];
    uint16_t vxlan_dport;
    uint8_t vxlan_dip_type;                 //only internal use
    ip_addr_t vxlan_dip;
    uint32_t vxlan_vni;
    uint8_t vxlan_dscp;

    /*erspan*/
    uint8_t erspan_dmac[6];
    uint8_t erspan_dip_type;                //only internal use
    ip_addr_t erspan_dip;
    uint8_t erspan_type;
    uint16_t erspan_session_id;
    uint8_t erspan_dscp;

} additional_action_t;

typedef struct _action_t
{
    uint32_t               ref_counter; // reference counter
    action_type_t          type;

    uint8_t               forward_interface_id;
    
    uint8_t               load_balance_interfaces_num;
    uint8_t               load_balance_interface_array[MAX_INTERFACE_CNT];

    additional_action_t     additional_actions;

    /*load balance cfg*/
    load_balance_mode_t    load_balance_mode;
    load_balance_weight_t  weight_data;  

    uint32_t               load_balance_index;
    int                    load_balance_weight[MAX_LOAD_BALANCE_NODE];

} sf_action_t;


#endif
