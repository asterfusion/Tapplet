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
/** 1 **/
#ifndef __include_sf_interface_shm_h__
#define __include_sf_interface_shm_h__


#include <stdint.h>

#include "sf_shm_aligned.h"
#include "sf_api_macro.h"
#include "common_header.h"

#include "sf_interface.h"

#include "sf_thread.h"

#undef MOD_NAME

/** 2 **/
#define MOD_NAME interfaces


/** 3 **/
typedef struct
{
    uint8_t admin_status[MAX_INTERFACE_CNT];
    sf_inferface_t interfaces[MAX_INTERFACE_CNT];
}SF_ALIGNED_CACHE SF_CONF_T ;


typedef struct
{
    uint64_t in_packets;
    uint64_t in_octets;
    uint64_t in_bps;
    uint64_t in_pps;

    uint64_t out_packets;
    uint64_t out_octets;
    uint64_t out_bps;
    uint64_t out_pps;
    
    uint64_t drop_packets;

    uint64_t tx_error_packets;
    uint64_t tx_ok_packets;

    uint64_t rx_miss_packets;

    uint64_t packet_type_packets[PACKET_TYPE_MAX];
    uint64_t packet_type_octets[PACKET_TYPE_MAX];
    uint64_t packet_type_bps[PACKET_TYPE_MAX];
    uint64_t packet_type_pps[PACKET_TYPE_MAX];
}SF_ALIGNED_CACHE single_interface_stat_t;

// typedef struct{
//     uint64_t in_packets;
//     uint64_t in_octets;
//     uint64_t out_packets;
//     uint64_t out_octets;
//     uint64_t drop_packets;
//     uint64_t packet_type_packets[PACKET_TYPE_MAX];
//     uint64_t packet_type_octets[PACKET_TYPE_MAX];
// }SF_ALIGNED_CACHE single_interface_stat_pre_thread_t;

typedef struct
{
    uint64_t in_packets;
    uint64_t in_octets;
    uint64_t out_packets;
    uint64_t out_octets;
    uint64_t drop_packets;

    uint64_t phy_rx_packets; 
    uint64_t phy_rx_octets; 
    uint64_t phy_rx_miss;
    uint64_t phy_rx_error; 

    uint64_t phy_tx_packets; 
    uint64_t phy_tx_octets; 
    uint64_t phy_tx_error; 

    //phy counters maybe 32-bit
    uint64_t phy_mask; 
    uint64_t extra_byte_pre_pkt;

    uint64_t packet_type_packets[PACKET_TYPE_CNT];
    uint64_t packet_type_octets[PACKET_TYPE_CNT];
} intf_old_stat_t;


#define SF_INTF_LINK_UNKNOWN_DUPLEX 0
#define SF_INTF_LINK_HALF_DUPLEX 1
#define SF_INTF_LINK_FULL_DUPLEX 2


typedef struct{
    int link;
    int speed;
    int duplex;
    int mtu;
}intf_phy_status_t;

typedef struct{
    uint64_t dpdk_init_status;
    uint8_t intf_link[MAX_INTERFACE_CNT]; // final link status = service_link + phy link

    int dpdk_port_cnt;
    int interfaces_cnt;
    /* TODO *//* update it when user changes queues*/
    uint8_t dpdk_max_queue[MAX_INTERFACE_CNT];
    uint8_t dpdk_used_queue[MAX_INTERFACE_CNT];

    intf_phy_status_t intf_phy_status[MAX_INTERFACE_CNT];
    single_interface_stat_t interfaces[MAX_INTERFACE_CNT];
}SF_STAT_T;


#undef for_each_member_in_out
#define for_each_member_in_out \
    FUNC(in_packets , in_pps , 1)    \
    FUNC(in_octets , in_bps  , 8 )    \
    FUNC(out_packets , out_pps , 1)   \
    FUNC(out_octets , out_bps , 8)

#undef for_each_member_packet_type 
#define for_each_member_packet_type\
    FUNC(packet_type_packets , packet_type_pps , 1)   \
    FUNC(packet_type_octets , packet_type_bps  , 8 )    


#ifndef IN_NSHELL_LIB
SF_CONF_PTR_DECLARE_VPP
SF_STAT_PTR_DECLARE_VPP
#endif

INIT_CONFIG_FUNC_DECLARE
int init_interfaces_global_status();





#endif
