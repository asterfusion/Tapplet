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
#ifndef __include_sf_interface_tools_h__
#define __include_sf_interface_tools_h__

#include <vnet/vnet.h>
#include <vlib/vlib.h>

#include "sf_interface_shm.h"
#include "sf_stat_tools.h"

typedef struct {
    uint64_t packets;
    uint64_t octets;
}sf_intf_pkt_type_stat_t;

typedef struct{
    uint64_t in_packets[MAX_INTERFACE_CNT]; // 
    uint64_t in_octets[MAX_INTERFACE_CNT];
    uint64_t out_packets[MAX_INTERFACE_CNT];
    uint64_t out_octets[MAX_INTERFACE_CNT];
    uint64_t drop_packets[MAX_INTERFACE_CNT];

    sf_intf_pkt_type_stat_t packet_type_stat[PACKET_TYPE_CNT][MAX_INTERFACE_CNT];
}sf_intf_stat_pre_thread_t;


extern sf_intf_stat_pre_thread_t *sf_intf_stat_main;

always_inline uint8_t get_interface_admin_status(uint32_t intf_id)
{
    return interfaces_config->admin_status[intf_id - 1];
}


always_inline sf_action_index_t get_interface_default_action(uint32_t intf_id)
{
    return interfaces_config->interfaces[intf_id - 1].ingress_config.default_action_id;
}

always_inline uint8_t get_interface_link( uint32_t intf_id)
{
    if(PREDICT_FALSE(intf_id > MAX_INTERFACE_CNT))
    {
        return 0;
    }
    return interfaces_stat->intf_link[intf_id -1];
}



always_inline uint8_t get_interface_acl_rule_group(uint32_t intf_id)
{
    return interfaces_config->interfaces[intf_id - 1].ingress_config.acl_rule_group;
} 


always_inline sf_action_index_t get_interface_rule_to_action(uint32_t intf_id , uint64_t rule_id)
{
    return interfaces_config->interfaces[intf_id - 1].ingress_config.rule_to_action[rule_id - 1];
}

#define sf_port_to_interface(port_id) \
    (port_id >= 1 && port_id <= MAX_PORT_NUM ? port_id : 0)

#define set_tx_intf_index(b0, index)                 \
    {                                                \
        sf_wdata(b0)->interface_out = index; \
    }

#define set_tx_intf_index_wdata(wdata , intf_id)      \
    {                                               \
        (wdata)->interface_out = intf_id;             \
    }


/****** stat ******/
#define update_interface_stat_inline(thread_index, intf_id, member) \
    sf_intf_stat_main[thread_index].member[(intf_id)-1] += 1

// if increment is negative , then it means decrement
#define update_interface_stat_multi_inline(thread_index, intf_id, member, increment) \
    sf_intf_stat_main[thread_index].member[(intf_id)-1] += increment

#define decrease_interface_stat_inline(thread_index, intf_id, member) \
    sf_intf_stat_main[thread_index].member[(intf_id)-1] -= 1

#ifdef VALIDATE_STAT_NULL_POINTER

#define update_interface_stat(thread_index, intf_id, member)         \
    if ((sf_intf_stat_main) != NULL)                                 \
    {                                                              \
        update_interface_stat_inline(thread_index, intf_id, member); \
    }

// if increment is negative , then it means decrement
#define update_interface_stat_multi(thread_index, intf_id, member, increment)         \
    if ((sf_intf_stat_main) != NULL)                                                  \
    {                                                                               \
        update_interface_stat_multi_inline(thread_index, intf_id, member, increment); \
    }

#define decrease_interface_stat(thread_index, intf_id, member)         \
    if ((sf_intf_stat_main) != NULL)                                   \
    {                                                                \
        decrease_interface_stat_inline(thread_index, intf_id, member); \
    }

#else // #if VALIDATE_STAT_NULL_POINTER

#define update_interface_stat(thread_index, intf_id, member) \
    update_interface_stat_inline(thread_index, intf_id, member)

// if increment is negative , then it means decrement
#define update_interface_stat_multi(thread_index, intf_id, member, increment) \
    update_interface_stat_multi_inline(thread_index, intf_id, member, increment)

#define decrease_interface_stat(thread_index, intf_id, member) \
    decrease_interface_stat_inline(thread_index, intf_id, member)

#endif // #if VALIDATE_STAT_NULL_POINTER

always_inline vnet_hw_interface_t *
sf_vnet_get_sw_hw_interface(vnet_main_t * vnm, u32 sw_if_index)
{
    vnet_sw_interface_t *sw = vnet_get_sw_interface_safe (vnm, sw_if_index);
    if(sw == NULL)
    {
        return NULL;
    }

    return vnet_get_hw_interface_safe(vnm , sw->hw_if_index);
}


#define sf_get_intf_cnt() (interfaces_stat->interfaces_cnt)


#define sf_interface_to_sw_if(intf_id) (intf_id)
#define sf_sw_if_to_interface(sw_if_index) (sw_if_index)


#endif
