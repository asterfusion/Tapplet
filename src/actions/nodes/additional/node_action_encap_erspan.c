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
#include <vlib/vlib.h>

#include <vnet/vnet.h>
#include <vnet/pg/pg.h>
#include <vppinfra/error.h>
#include <stdio.h>

#include <vnet/ip/ip4_packet.h>


#include "sf_main.h"
#include "sf_string.h"
#include "pkt_wdata.h"

#include "sf_interface_shm.h"

#include "action.h"
#include "action_shm.h"
#include "action_tools.h"

#include "sf_tools.h"
#include "sf_stat_tools.h"

#include "sf_checksum_tools.h"

#include "action_path_tools.h"

#define NSEC_PER_SECOND     (1000000000ull)
#define NSEC_PER_100USEC    (100000ull)
/*
***initial the same header when node start
***common header are MAC & IPV4 & GRE
erspan_i_encap_hdr   = erspan_encap_common_hdr_t
erspan_ii_encap_hdr  = erspan_encap_common_hdr_t + sf_gre_option_t + sf_erspan_ii_header_t
erspan_iii_encap_hdr = erspan_encap_common_hdr_t + sf_gre_option_t + sf_erspan_iii_header_t
*/
typedef struct{
    eth_header_t eth_hdr;
    ipv4_header_t ipv4_hdr;
    sf_gre_header_t gre_hdr;
}__attribute__((packed))  erspan_encap_common_hdr_t;

typedef struct{
    eth_header_t eth_hdr;
    ipv6_header_t ipv6_hdr;
    sf_gre_header_t gre_hdr;
}__attribute__((packed))  erspan_encap_common_hdr_v6_t;

typedef uint32_t sf_gre_option_t;

typedef struct _timestamp_time
{
    f64 vlib_time;
    u64 milisecond_time; 
}timestamp_time_t;

#ifndef CLIB_MARCH_VARIANT
timestamp_time_t  timestamp_time;

erspan_encap_common_hdr_v6_t erspan_encap_common_hdr_template_ipv6;
erspan_encap_common_hdr_t erspan_encap_common_hdr_template;
sf_erspan_ii_header_t sf_erspan_ii_header_template;
sf_gre_option_t sf_gre_option_template;
sf_erspan_iii_header_t sf_erspan_iii_header_template;
#else
extern timestamp_time_t  timestamp_time;

extern erspan_encap_common_hdr_v6_t erspan_encap_common_hdr_template_ipv6;
extern erspan_encap_common_hdr_t erspan_encap_common_hdr_template;
extern sf_erspan_ii_header_t sf_erspan_ii_header_template;
extern sf_gre_option_t sf_gre_option_template;
extern sf_erspan_iii_header_t sf_erspan_iii_header_template;
#endif


VLIB_REGISTER_NODE(sf_encap_erspan_node) = {
    .name = "node_action_encap_erspan",
    .vector_size = sizeof(u32),
    //.format_trace = format_sf_trace,
    .type = VLIB_NODE_TYPE_INTERNAL,

    .n_next_nodes = SF_ACTION_NEXT_NODE_N_COUNT,

    /* edit / add dispositions here */
    .next_nodes = {
#define FUNC(a, b) [a] = b,
    foreach_additional_next_node
#undef FUNC

#define FUNC(a, b, c) [b] = c,
    foreach_forward_next_node
#undef FUNC
    },
};

static_always_inline void encap_erspan_proc
(vlib_main_t * vm,
vlib_buffer_t *vbuffer, 
sf_wdata_t *wdata, 
additional_action_t *adtn_ptr, 
int16_t *delta)
{
    switch(adtn_ptr->erspan_type)
    {
        case 1:
        {
            erspan_encap_common_hdr_t *erspan_encap_common_ptr =
            (erspan_encap_common_hdr_t *)(sf_wqe_get_current(vbuffer) - sizeof(erspan_encap_common_hdr_t));
            clib_memcpy(erspan_encap_common_ptr ,
                        &erspan_encap_common_hdr_template ,
                        sizeof(erspan_encap_common_hdr_t));

            //mac
            clib_memcpy(erspan_encap_common_ptr->eth_hdr.src_mac , interfaces_config->interfaces[wdata->interface_id -1].mac , 6);
            clib_memcpy(erspan_encap_common_ptr->eth_hdr.dst_mac , adtn_ptr->erspan_dmac , 6);
            //sip
	    if(interfaces_config->interfaces[wdata->interface_id-1].ipv4_addr[0] || 
	       interfaces_config->interfaces[wdata->interface_id-1].ipv4_addr[1] || 
	       interfaces_config->interfaces[wdata->interface_id-1].ipv4_addr[2] || 
	       interfaces_config->interfaces[wdata->interface_id-1].ipv4_addr[3])
            {
               clib_memcpy(&(erspan_encap_common_ptr->ipv4_hdr.src_ip), (uint32_t *)interfaces_config->interfaces[wdata->interface_id-1].ipv4_addr, 4);
            }
            //dscp
            erspan_encap_common_ptr->ipv4_hdr.ip_tos = adtn_ptr->erspan_dscp;
            //dip
            erspan_encap_common_ptr->ipv4_hdr.dst_ip = adtn_ptr->erspan_dip.addr_v4;
            //total_len
            erspan_encap_common_ptr->ipv4_hdr.totalLen = clib_host_to_net_u16(
                 IPV4_HEADER_LEN + GRE_HEADER_LEN  + sf_wqe_get_current_len(vbuffer));
            //checksum 
            erspan_encap_common_ptr->ipv4_hdr.checksum = 
                sf_ip4_chechsum( &(erspan_encap_common_ptr->ipv4_hdr) );
            //no need to change gre ,see node initial
            //change current data
            sf_wqe_advance(vbuffer , - (sizeof(erspan_encap_common_hdr_t)));
            *delta += sizeof(erspan_encap_common_hdr_t);
            break;
        }
        case 2:
        {
            uint8_t *packet = (uint8_t *)(sf_wqe_get_data_ptr(vbuffer));
            int16_t l2_hdr_offset = wdata->l2_hdr_offset;

            erspan_encap_common_hdr_t *erspan_encap_common_ptr = 
            (erspan_encap_common_hdr_t *)(sf_wqe_get_current(vbuffer) -
                                            sizeof(erspan_encap_common_hdr_t) -
                                            sizeof(sf_gre_option_t) -
                                            sizeof(sf_erspan_ii_header_t));

            sf_gre_option_t *sf_gre_option_ptr = 
            (sf_gre_option_t *)(sf_wqe_get_current(vbuffer) - 
                                        sizeof(sf_gre_option_t) -
                                        sizeof(sf_erspan_ii_header_t));

            sf_erspan_ii_header_t *sf_erspan_ii_header_ptr =
            (sf_erspan_ii_header_t *)(sf_wqe_get_current(vbuffer) - sizeof(sf_erspan_ii_header_t));

            clib_memcpy(erspan_encap_common_ptr ,
                        &erspan_encap_common_hdr_template ,
                        sizeof(erspan_encap_common_hdr_t));
            clib_memcpy(sf_gre_option_ptr ,
                        &sf_gre_option_template ,
                        sizeof(sf_gre_option_t));
            clib_memcpy(sf_erspan_ii_header_ptr ,
                        &sf_erspan_ii_header_template ,
                        sizeof(sf_erspan_ii_header_t));
            //mac
            clib_memcpy(erspan_encap_common_ptr->eth_hdr.src_mac , interfaces_config->interfaces[wdata->interface_id -1].mac , 6);
            clib_memcpy(erspan_encap_common_ptr->eth_hdr.dst_mac , adtn_ptr->erspan_dmac , 6);
            //dscp
            erspan_encap_common_ptr->ipv4_hdr.ip_tos = adtn_ptr->erspan_dscp;
            //sip
	    if(interfaces_config->interfaces[wdata->interface_id-1].ipv4_addr[0] || 
	       interfaces_config->interfaces[wdata->interface_id-1].ipv4_addr[1] || 
	       interfaces_config->interfaces[wdata->interface_id-1].ipv4_addr[2] || 
	       interfaces_config->interfaces[wdata->interface_id-1].ipv4_addr[3])
            {
               clib_memcpy(&(erspan_encap_common_ptr->ipv4_hdr.src_ip), (uint32_t *)interfaces_config->interfaces[wdata->interface_id-1].ipv4_addr, 4);
            }
            //dip
            erspan_encap_common_ptr->ipv4_hdr.dst_ip = adtn_ptr->erspan_dip.addr_v4;
            //total_len
            erspan_encap_common_ptr->ipv4_hdr.totalLen = clib_host_to_net_u16(
                 IPV4_HEADER_LEN + 
                 GRE_HEADER_LEN  + 
                 sizeof(sf_gre_option_t) + 
                 sizeof(sf_erspan_ii_header_t) + 
                 sf_wqe_get_current_len(vbuffer));
            //checksum 
            erspan_encap_common_ptr->ipv4_hdr.checksum = 
                sf_ip4_chechsum( &(erspan_encap_common_ptr->ipv4_hdr));
            
            /*gre*/
            //Gre flag_S
            erspan_encap_common_ptr->gre_hdr.flag_S = 0b1;
            //make Sequence Number unique
            *sf_gre_option_ptr = clib_host_to_net_u32(sf_gre_option_template++);

            /*erspan_ii*/
            // COS
            if(clib_net_to_host_u16(*(uint16_t *)(packet + l2_hdr_offset + 12)) == 0x8100)
            {
                sf_erspan_ii_header_ptr->COS = *(packet + l2_hdr_offset + 14) >> 5;
            }
            //session id in erspan hdr
            sf_erspan_ii_header_ptr->Session_ID = adtn_ptr->erspan_session_id;

            //uint16_t Ver+Vlan change byte order
            *(uint16_t *)sf_erspan_ii_header_ptr = clib_host_to_net_u16(*(uint16_t *)sf_erspan_ii_header_ptr);
            //uint16_t COS+EN+T+Session_ID change byte order
            *((uint16_t *)(sf_erspan_ii_header_ptr) + 1) = clib_host_to_net_u16(*((uint16_t *)(sf_erspan_ii_header_ptr) + 1));
            //uint32_t Reserved+Index change byte order
            *((uint32_t *)(sf_erspan_ii_header_ptr) + 1) = clib_host_to_net_u32(*((uint32_t *)(sf_erspan_ii_header_ptr) + 1));
            //change current data
            sf_wqe_advance(vbuffer , - (sizeof(erspan_encap_common_hdr_t)) - sizeof(sf_gre_option_t) - sizeof(sf_erspan_ii_header_t));
            *delta += sizeof(erspan_encap_common_hdr_t) + sizeof(sf_gre_option_t) + sizeof(sf_erspan_ii_header_t);
            break;
        }
        case 3:
        {
            uint8_t *packet = (uint8_t *)(sf_wqe_get_data_ptr(vbuffer));
            int16_t l2_hdr_offset = wdata->l2_hdr_offset;
            
            erspan_encap_common_hdr_t *erspan_encap_common_ptr = 
            (erspan_encap_common_hdr_t *)(sf_wqe_get_current(vbuffer) - 
                                            sizeof(erspan_encap_common_hdr_t) - 
                                            sizeof(sf_gre_option_t) - 
                                            sizeof(sf_erspan_iii_header_t));

            sf_gre_option_t *sf_gre_option_ptr = 
            (sf_gre_option_t *)(sf_wqe_get_current(vbuffer) - 
                                        sizeof(sf_gre_option_t) -
                                        sizeof(sf_erspan_iii_header_t));

            sf_erspan_iii_header_t *sf_erspan_iii_header_ptr =
            (sf_erspan_iii_header_t *)(sf_wqe_get_current(vbuffer) - sizeof(sf_erspan_iii_header_t));

            clib_memcpy(erspan_encap_common_ptr ,
                        &erspan_encap_common_hdr_template ,
                        sizeof(erspan_encap_common_hdr_t));
            clib_memcpy(sf_gre_option_ptr ,
                        &sf_gre_option_template ,
                        sizeof(sf_gre_option_t));
            clib_memcpy(sf_erspan_iii_header_ptr ,
                        &sf_erspan_iii_header_template ,
                        sizeof(sf_erspan_iii_header_t));
             //mac
            clib_memcpy(erspan_encap_common_ptr->eth_hdr.src_mac , interfaces_config->interfaces[wdata->interface_id -1].mac , 6);
            clib_memcpy(erspan_encap_common_ptr->eth_hdr.dst_mac , adtn_ptr->erspan_dmac , 6);
            //sip
	    if(interfaces_config->interfaces[wdata->interface_id-1].ipv4_addr[0] || 
	       interfaces_config->interfaces[wdata->interface_id-1].ipv4_addr[1] || 
	       interfaces_config->interfaces[wdata->interface_id-1].ipv4_addr[2] || 
	       interfaces_config->interfaces[wdata->interface_id-1].ipv4_addr[3])
            {
               clib_memcpy(&(erspan_encap_common_ptr->ipv4_hdr.src_ip), (uint32_t *)interfaces_config->interfaces[wdata->interface_id-1].ipv4_addr, 4);
            }
            //dscp
            erspan_encap_common_ptr->ipv4_hdr.ip_tos = adtn_ptr->erspan_dscp;
            //dip
            erspan_encap_common_ptr->ipv4_hdr.dst_ip = adtn_ptr->erspan_dip.addr_v4;
            //total_len
            erspan_encap_common_ptr->ipv4_hdr.totalLen = clib_host_to_net_u16(
                 IPV4_HEADER_LEN + 
                 GRE_HEADER_LEN  + 
                 sizeof(sf_gre_option_t) + 
                 sizeof(sf_erspan_iii_header_t) + 
                 sf_wqe_get_current_len(vbuffer));
            //checksum 
            erspan_encap_common_ptr->ipv4_hdr.checksum = 
                sf_ip4_chechsum( &(erspan_encap_common_ptr->ipv4_hdr) );

            /*gre*/
            //Gre flag_S
            erspan_encap_common_ptr->gre_hdr.flag_S = 0b1;
            //Gre proto type
            erspan_encap_common_ptr->gre_hdr.protocol_type = clib_net_to_host_u16(0x22EB);
            //make Sequence Number unique
            *sf_gre_option_ptr = clib_host_to_net_u32(sf_gre_option_template++);

            /*erspan_iii*/
            // COS & Vlan
            if(clib_net_to_host_u16(*(uint16_t *)(packet + l2_hdr_offset + 12)) == 0x8100)
            {
                sf_erspan_iii_header_ptr->COS = *(packet + l2_hdr_offset + 14) >> 5;
                sf_erspan_iii_header_ptr->Vlan = clib_net_to_host_u16(*(uint16_t *)(packet + l2_hdr_offset + 14)) & 0b0000111111111111;
            }
            //session id in erspan hdr
            sf_erspan_iii_header_ptr->Session_ID = adtn_ptr->erspan_session_id;
            //timestamp
            uint64_t ts = (u64) (((vlib_time_now (vm) - timestamp_time.vlib_time)* NSEC_PER_SECOND) + 
                                timestamp_time.milisecond_time  +
                                (wdata->timestamp - clib_cpu_time_now()) /
                                sf_action_main.cpu_clock_pre_second/ NSEC_PER_SECOND) /
                                NSEC_PER_100USEC;
            sf_erspan_iii_header_ptr->Timestamp = clib_host_to_net_u32((uint32_t)ts);
            //uint16_t Ver+Vlan change byte order
            *(uint16_t *)sf_erspan_iii_header_ptr = clib_host_to_net_u16(*(uint16_t *)sf_erspan_iii_header_ptr);
            //uint16_t COS+BSO+T+Session_ID change byte order
            *((uint16_t *)(sf_erspan_iii_header_ptr) + 1) = clib_host_to_net_u16(*((uint16_t *)(sf_erspan_iii_header_ptr) + 1));
            //uint16_t P+FT+HW_ID+D+Gra+O change byte order
            *((uint16_t *)(sf_erspan_iii_header_ptr) + 5) = clib_host_to_net_u16(*((uint16_t *)(sf_erspan_iii_header_ptr) + 5));
            
            //change current data
            sf_wqe_advance(vbuffer , - (sizeof(erspan_encap_common_hdr_t)) - sizeof(sf_gre_option_t) - sizeof(sf_erspan_iii_header_t));
            *delta += sizeof(erspan_encap_common_hdr_t) + sizeof(sf_gre_option_t) + sizeof(sf_erspan_iii_header_t);
            break;
        }
        default:
        break;
    }
    return;
}
static_always_inline void encap_erspan_proc_ipv6
(vlib_main_t * vm,
vlib_buffer_t *vbuffer, 
sf_wdata_t *wdata, 
additional_action_t *adtn_ptr, 
int16_t *delta)
{
    switch(adtn_ptr->erspan_type)
    {
        case 1:
        {
            erspan_encap_common_hdr_v6_t *erspan_encap_common_ptr =
            (erspan_encap_common_hdr_v6_t *)(sf_wqe_get_current(vbuffer) - sizeof(erspan_encap_common_hdr_v6_t));
            clib_memcpy(erspan_encap_common_ptr ,
                        &erspan_encap_common_hdr_template_ipv6 ,
                        sizeof(erspan_encap_common_hdr_v6_t));

            //mac
            clib_memcpy(erspan_encap_common_ptr->eth_hdr.src_mac , interfaces_config->interfaces[wdata->interface_id -1].mac , 6);
            clib_memcpy(erspan_encap_common_ptr->eth_hdr.dst_mac , adtn_ptr->erspan_dmac , 6);
            //dscp
            // erspan_encap_common_ptr->ipv6_hdr.traffic_class = adtn_ptr->erspan_dscp;
            ip6_hdr_set_traffic_class( &(erspan_encap_common_ptr->ipv6_hdr) , adtn_ptr->erspan_dscp );
            //sip
	    if(interfaces_config->interfaces[wdata->interface_id-1].ipv6_addr.addr_v6_upper || 
	       interfaces_config->interfaces[wdata->interface_id-1].ipv6_addr.addr_v6_lower)
            {
               clib_memcpy(&(erspan_encap_common_ptr->ipv6_hdr.src_ip), &(interfaces_config->interfaces[wdata->interface_id-1].ipv6_addr), sizeof(ip_addr_t));
            }
            //dip
            clib_memcpy(&(erspan_encap_common_ptr->ipv6_hdr.dst_ip), &(adtn_ptr->erspan_dip), sizeof(ip_addr_t));
            //total_len
            erspan_encap_common_ptr->ipv6_hdr.payload_len = clib_host_to_net_u16(
                 GRE_HEADER_LEN  + sf_wqe_get_current_len(vbuffer));
            //no need to change gre ,see node initial
            //flow_label + traffic_class + version = 32bits ->change order
            // *(uint32_t *)&erspan_encap_common_ptr->ipv6_hdr = clib_host_to_net_u32(*(uint32_t *)&erspan_encap_common_ptr->ipv6_hdr);

            //change current data
            sf_wqe_advance(vbuffer , - (sizeof(erspan_encap_common_hdr_v6_t)));
            *delta += sizeof(erspan_encap_common_hdr_v6_t);
            break;
        }
        case 2:
        {
            uint8_t *packet = (uint8_t *)(sf_wqe_get_data_ptr(vbuffer));
            int16_t l2_hdr_offset = wdata->l2_hdr_offset;

            erspan_encap_common_hdr_v6_t *erspan_encap_common_ptr = 
            (erspan_encap_common_hdr_v6_t *)(sf_wqe_get_current(vbuffer) -
                                            sizeof(erspan_encap_common_hdr_v6_t) -
                                            sizeof(sf_gre_option_t) -
                                            sizeof(sf_erspan_ii_header_t));

            sf_gre_option_t *sf_gre_option_ptr = 
            (sf_gre_option_t *)(sf_wqe_get_current(vbuffer) - 
                                        sizeof(sf_gre_option_t) -
                                        sizeof(sf_erspan_ii_header_t));

            sf_erspan_ii_header_t *sf_erspan_ii_header_ptr =
            (sf_erspan_ii_header_t *)(sf_wqe_get_current(vbuffer) - sizeof(sf_erspan_ii_header_t));

            clib_memcpy(erspan_encap_common_ptr ,
                        &erspan_encap_common_hdr_template_ipv6 ,
                        sizeof(erspan_encap_common_hdr_v6_t));
            clib_memcpy(sf_gre_option_ptr ,
                        &sf_gre_option_template ,
                        sizeof(sf_gre_option_t));
            clib_memcpy(sf_erspan_ii_header_ptr ,
                        &sf_erspan_ii_header_template ,
                        sizeof(sf_erspan_ii_header_t));
            //mac
            clib_memcpy(erspan_encap_common_ptr->eth_hdr.src_mac , interfaces_config->interfaces[wdata->interface_id -1].mac , 6);
            clib_memcpy(erspan_encap_common_ptr->eth_hdr.dst_mac , adtn_ptr->erspan_dmac , 6);
            //dscp
            // erspan_encap_common_ptr->ipv6_hdr.traffic_class = adtn_ptr->erspan_dscp;
            ip6_hdr_set_traffic_class( &(erspan_encap_common_ptr->ipv6_hdr) , adtn_ptr->erspan_dscp );

            //sip
	    if(interfaces_config->interfaces[wdata->interface_id-1].ipv6_addr.addr_v6_upper || 
	       interfaces_config->interfaces[wdata->interface_id-1].ipv6_addr.addr_v6_lower)
            {
               clib_memcpy(&(erspan_encap_common_ptr->ipv6_hdr.src_ip), &(interfaces_config->interfaces[wdata->interface_id-1].ipv6_addr), sizeof(ip_addr_t));
            }
            //dip
            clib_memcpy(&(erspan_encap_common_ptr->ipv6_hdr.dst_ip), &(adtn_ptr->erspan_dip), sizeof(ip_addr_t));
            //total_len
            erspan_encap_common_ptr->ipv6_hdr.payload_len = clib_host_to_net_u16(
                 GRE_HEADER_LEN  + 
                 sizeof(sf_gre_option_t) + 
                 sizeof(sf_erspan_ii_header_t) + 
                 sf_wqe_get_current_len(vbuffer));
            
            /*gre*/
            //Gre flag_S
            erspan_encap_common_ptr->gre_hdr.flag_S = 0b1;
            //make Sequence Number unique
            *sf_gre_option_ptr = clib_host_to_net_u32(sf_gre_option_template++);

            /*erspan_ii*/
            // COS
            if(clib_net_to_host_u16(*(uint16_t *)(packet + l2_hdr_offset + 12)) == 0x8100)
            {
                sf_erspan_ii_header_ptr->COS = *(packet + l2_hdr_offset + 14) >> 5;
            }
            //session id in erspan hdr
            sf_erspan_ii_header_ptr->Session_ID = adtn_ptr->erspan_session_id;

            //flow_label + traffic_class + version = 32bits ->change order
            // *(uint32_t *)&erspan_encap_common_ptr->ipv6_hdr = clib_host_to_net_u32(*(uint32_t *)&erspan_encap_common_ptr->ipv6_hdr);
            
            //uint16_t Ver+Vlan change byte order
            *(uint16_t *)sf_erspan_ii_header_ptr = clib_host_to_net_u16(*(uint16_t *)sf_erspan_ii_header_ptr);
            //uint16_t COS+EN+T+Session_ID change byte order
            *((uint16_t *)(sf_erspan_ii_header_ptr) + 1) = clib_host_to_net_u16(*((uint16_t *)(sf_erspan_ii_header_ptr) + 1));
            //uint32_t Reserved+Index change byte order
            *((uint32_t *)(sf_erspan_ii_header_ptr) + 1) = clib_host_to_net_u32(*((uint32_t *)(sf_erspan_ii_header_ptr) + 1));
            //change current data
            sf_wqe_advance(vbuffer , - (sizeof(erspan_encap_common_hdr_v6_t)) - sizeof(sf_gre_option_t) - sizeof(sf_erspan_ii_header_t));
            *delta += sizeof(erspan_encap_common_hdr_v6_t) + sizeof(sf_gre_option_t) + sizeof(sf_erspan_ii_header_t);
            break;
        }
        case 3:
        {
            uint8_t *packet = (uint8_t *)(sf_wqe_get_data_ptr(vbuffer));
            int16_t l2_hdr_offset = wdata->l2_hdr_offset;
            
            erspan_encap_common_hdr_v6_t *erspan_encap_common_ptr = 
            (erspan_encap_common_hdr_v6_t *)(sf_wqe_get_current(vbuffer) - 
                                            sizeof(erspan_encap_common_hdr_v6_t) - 
                                            sizeof(sf_gre_option_t) - 
                                            sizeof(sf_erspan_iii_header_t));

            sf_gre_option_t *sf_gre_option_ptr = 
            (sf_gre_option_t *)(sf_wqe_get_current(vbuffer) - 
                                        sizeof(sf_gre_option_t) -
                                        sizeof(sf_erspan_iii_header_t));

            sf_erspan_iii_header_t *sf_erspan_iii_header_ptr =
            (sf_erspan_iii_header_t *)(sf_wqe_get_current(vbuffer) - sizeof(sf_erspan_iii_header_t));

            clib_memcpy(erspan_encap_common_ptr ,
                        &erspan_encap_common_hdr_template_ipv6 ,
                        sizeof(erspan_encap_common_hdr_v6_t));
            clib_memcpy(sf_gre_option_ptr ,
                        &sf_gre_option_template ,
                        sizeof(sf_gre_option_t));
            clib_memcpy(sf_erspan_iii_header_ptr ,
                        &sf_erspan_iii_header_template ,
                        sizeof(sf_erspan_iii_header_t));
             //mac
            clib_memcpy(erspan_encap_common_ptr->eth_hdr.src_mac , interfaces_config->interfaces[wdata->interface_id -1].mac , 6);
            clib_memcpy(erspan_encap_common_ptr->eth_hdr.dst_mac , adtn_ptr->erspan_dmac , 6);
            //dscp
            // erspan_encap_common_ptr->ipv6_hdr.traffic_class = adtn_ptr->erspan_dscp;
            ip6_hdr_set_traffic_class( &(erspan_encap_common_ptr->ipv6_hdr) , adtn_ptr->erspan_dscp );
            
            //sip
	    if(interfaces_config->interfaces[wdata->interface_id-1].ipv6_addr.addr_v6_upper || 
	       interfaces_config->interfaces[wdata->interface_id-1].ipv6_addr.addr_v6_lower)
            {
               clib_memcpy(&(erspan_encap_common_ptr->ipv6_hdr.src_ip), &(interfaces_config->interfaces[wdata->interface_id-1].ipv6_addr), sizeof(ip_addr_t));
            }
            //dip
            clib_memcpy(&(erspan_encap_common_ptr->ipv6_hdr.dst_ip), &(adtn_ptr->erspan_dip), sizeof(ip_addr_t));
            //total_len
            erspan_encap_common_ptr->ipv6_hdr.payload_len = clib_host_to_net_u16(
                 GRE_HEADER_LEN  + 
                 sizeof(sf_gre_option_t) + 
                 sizeof(sf_erspan_iii_header_t) + 
                 sf_wqe_get_current_len(vbuffer));

            /*gre*/
            //Gre flag_S
            erspan_encap_common_ptr->gre_hdr.flag_S = 0b1;
            //Gre proto type
            erspan_encap_common_ptr->gre_hdr.protocol_type = clib_net_to_host_u16(0x22EB);
            //make Sequence Number unique
            *sf_gre_option_ptr = clib_host_to_net_u32(sf_gre_option_template++);

            /*erspan_iii*/
            // COS & Vlan
            if(clib_net_to_host_u16(*(uint16_t *)(packet + l2_hdr_offset + 12)) == 0x8100)
            {
                sf_erspan_iii_header_ptr->COS = *(packet + l2_hdr_offset + 14) >> 5;
                sf_erspan_iii_header_ptr->Vlan = clib_net_to_host_u16(*(uint16_t *)(packet + l2_hdr_offset + 14)) & 0b0000111111111111;
            }
            //session id in erspan hdr
            sf_erspan_iii_header_ptr->Session_ID = adtn_ptr->erspan_session_id;
            //timestamp
            uint64_t ts = (u64) (((vlib_time_now (vm) - timestamp_time.vlib_time)* NSEC_PER_SECOND) + 
                                timestamp_time.milisecond_time  +
                                (wdata->timestamp - clib_cpu_time_now()) /
                                sf_action_main.cpu_clock_pre_second/ NSEC_PER_SECOND) /
                                NSEC_PER_100USEC;
            sf_erspan_iii_header_ptr->Timestamp = clib_host_to_net_u32((uint32_t)ts);

            //flow_label + traffic_class + version = 32bits ->change order
            // *(uint32_t *)&erspan_encap_common_ptr->ipv6_hdr = clib_host_to_net_u32(*(uint32_t *)&erspan_encap_common_ptr->ipv6_hdr);
            
            //uint16_t Ver+Vlan change byte order
            *(uint16_t *)sf_erspan_iii_header_ptr = clib_host_to_net_u16(*(uint16_t *)sf_erspan_iii_header_ptr);
            //uint16_t COS+BSO+T+Session_ID change byte order
            *((uint16_t *)(sf_erspan_iii_header_ptr) + 1) = clib_host_to_net_u16(*((uint16_t *)(sf_erspan_iii_header_ptr) + 1));
            //uint16_t P+FT+HW_ID+D+Gra+O change byte order
            *((uint16_t *)(sf_erspan_iii_header_ptr) + 5) = clib_host_to_net_u16(*((uint16_t *)(sf_erspan_iii_header_ptr) + 5));
            
            //change current data
            sf_wqe_advance(vbuffer , - (sizeof(erspan_encap_common_hdr_v6_t)) - sizeof(sf_gre_option_t) - sizeof(sf_erspan_iii_header_t));
            *delta += sizeof(erspan_encap_common_hdr_v6_t) + sizeof(sf_gre_option_t) + sizeof(sf_erspan_iii_header_t);
            break;
        }
        default:
        break;
    }
    return;
}



VLIB_NODE_FN(sf_encap_erspan_node)
(
    vlib_main_t *vm,
    vlib_node_runtime_t *node,
    vlib_frame_t *frame)
{
    u32 n_left_from, n_left_to_next;
    u32 *from, *to_next;
    u32 next_index;

    from  =  vlib_frame_vector_args(frame);

    n_left_from = frame->n_vectors;

    next_index = node->cached_next_index;

    while(n_left_from > 0)
    {
        vlib_get_next_frame(vm, node, next_index, to_next, n_left_to_next);

        while (n_left_from >= 4 && n_left_to_next >= 2)
        {

            u32 bi0;
            u32 bi1;
            vlib_buffer_t *b0;
            vlib_buffer_t *b1;

            /* Prefetch next iteration. */
            {
                vlib_buffer_t *p2, *p3;

                p2 = vlib_get_buffer(vm, from[2]);
                p3 = vlib_get_buffer(vm, from[3]);

                sf_wdata_prefetch_cache(p2);
                sf_wdata_prefetch_cache(p3);

                sf_prefetch_additional_list(p2);
                sf_prefetch_additional_list(p3);

                sf_prefetch_data_store(p2);
                sf_prefetch_data_store(p3);
            }

            /* speculatively enqueue b0 to the current next frame */
            to_next[0] = bi0 = from[0];
            to_next[1] = bi1 = from[1];
            from += 2;
            to_next += 2;
            n_left_from -= 2;
            n_left_to_next -= 2;

            b0 = vlib_get_buffer(vm, bi0);
            b1 = vlib_get_buffer(vm, bi1);

            /*******************  start handle pkt  ***************/
            u32 next0 = SF_NEXT_NODE_SF_PKT_DROP;
            u32 next1 = SF_NEXT_NODE_SF_PKT_DROP;

            sf_additional_list_t * addi_list_ptr_0 = get_additional_list(b0);
            sf_additional_list_t * addi_list_ptr_1 = get_additional_list(b1);

            sf_action_t *action_0 = get_action_ptr_nocheck(b0);
            sf_action_t *action_1 = get_action_ptr_nocheck(b1);
            sf_wdata_t * pkt_wdata_0 = sf_wdata(b0);
            sf_wdata_t * pkt_wdata_1 = sf_wdata(b1);

            additional_action_t *adtn_action_ptr_0 = &action_0->additional_actions;
            additional_action_t *adtn_action_ptr_1 = &action_1->additional_actions;

            int16_t *delta_0 = &addi_list_ptr_0->delta;
            int16_t *delta_1 = &addi_list_ptr_1->delta;
            
            if(adtn_action_ptr_0->erspan_dip_type == 4)
            {
                encap_erspan_proc(vm,b0,pkt_wdata_0,adtn_action_ptr_0,delta_0);
            }
            else if(adtn_action_ptr_0->erspan_dip_type == 6)
            {
                encap_erspan_proc_ipv6(vm,b0,pkt_wdata_0,adtn_action_ptr_0,delta_0);
            }
  
            if(adtn_action_ptr_1->erspan_dip_type == 4)
            {
                encap_erspan_proc(vm,b1,pkt_wdata_1,adtn_action_ptr_1,delta_1);
            }
            else if(adtn_action_ptr_1->erspan_dip_type == 6)
            {
                encap_erspan_proc_ipv6(vm,b1,pkt_wdata_1,adtn_action_ptr_1,delta_1);
            }


            next0 = pop_addtional_list(addi_list_ptr_0);
            next1 = pop_addtional_list(addi_list_ptr_1);

            /* verify speculative enqueues, maybe switch current next frame */
            vlib_validate_buffer_enqueue_x2(vm, node, next_index,
                                            to_next, n_left_to_next,
                                            bi0, bi1, next0, next1);
        }
        while(n_left_from >0  && n_left_to_next >0)
        {
            u32 bi0;
            vlib_buffer_t *b0;


            /* speculatively enqueue b0 to the current next frame */
            to_next[0] = bi0 = from[0];
            from += 1;
            to_next += 1;
            n_left_from -= 1;
            n_left_to_next -= 1;

            b0 = vlib_get_buffer(vm, bi0);

            /*******************  start handle pkt  ***************/
            u32 next0 = SF_NEXT_NODE_SF_PKT_DROP;

            sf_additional_list_t * addi_list_ptr_0 = get_additional_list(b0);

            sf_action_t *action_0 = get_action_ptr_nocheck(b0);
            sf_wdata_t * pkt_wdata_0 = sf_wdata(b0);

            additional_action_t *adtn_action_ptr_0 = &action_0->additional_actions;

            int16_t *delta_0 = &addi_list_ptr_0->delta;
            
            if(adtn_action_ptr_0->erspan_dip_type == 4)
            {
                encap_erspan_proc(vm,b0,pkt_wdata_0,adtn_action_ptr_0,delta_0);
            }
            else if(adtn_action_ptr_0->erspan_dip_type == 6)
            {
                encap_erspan_proc_ipv6(vm,b0,pkt_wdata_0,adtn_action_ptr_0,delta_0);
            }

            next0 = pop_addtional_list(addi_list_ptr_0);


            /* verify speculative enqueue, maybe switch current next frame */
            vlib_validate_buffer_enqueue_x1(vm, node, next_index,
                                            to_next, n_left_to_next,
                                            bi0, next0);
        }

        vlib_put_next_frame(vm, node, next_index, n_left_to_next);
    }
    return frame->n_vectors;
}


static clib_error_t *sf_erspan_encap_common_hdr_t_init(vlib_main_t *vm)
{
    /*ipv4*/
    erspan_encap_common_hdr_t *erspan_encap_common_hdr = &erspan_encap_common_hdr_template;
    eth_header_t *eth_hdr = &(erspan_encap_common_hdr->eth_hdr);
    ipv4_header_t *ip4_hdr = &(erspan_encap_common_hdr->ipv4_hdr);
    sf_gre_header_t*gre_hdr = &(erspan_encap_common_hdr->gre_hdr);
    
    sf_erspan_ii_header_t *sf_erspan_ii_header = &sf_erspan_ii_header_template;
    sf_gre_option_t *sf_gre_option = &sf_gre_option_template;
    sf_erspan_iii_header_t *sf_erspan_iii_header = &sf_erspan_iii_header_template;

    
    sf_memset_zero(erspan_encap_common_hdr , sizeof(erspan_encap_common_hdr_t));
    sf_memset_zero(sf_erspan_ii_header , sizeof(sf_erspan_ii_header_t));
    sf_memset_zero(sf_gre_option , sizeof(sf_gre_option_t));
    sf_memset_zero(sf_erspan_iii_header , sizeof(sf_erspan_iii_header_t));
    
    /*initial erspan_encap_common_hdr*/
    eth_hdr->src_mac[0] = 0x11;
    eth_hdr->src_mac[1] = 0x22;
    eth_hdr->src_mac[2] = 0x33;
    eth_hdr->src_mac[3] = 0x44;
    eth_hdr->src_mac[4] = 0x55;
    eth_hdr->src_mac[5] = 0x66;
    eth_hdr->eth_type = ETHER_IPV4_NET_ORDER;

    ip4_hdr->version = 0x4;
    ip4_hdr->ipHeaderLen = 0x5;
    ip4_hdr->ip_tos = 0;
    ip4_hdr->id = 0;
    ip4_hdr->flags = 0;
#ifdef CPU_BIG_ENDIAN
    ip4_hdr->offset = 0;
#else
    ip4_hdr->offset_5bit = 0;
    ip4_hdr->offset_8bit = 0;
#endif
    ip4_hdr->TTL = 0x40;
    ip4_hdr->protocol = IP_PRO_GRE;//gre
    ip4_hdr->checksum = 0;
    ip4_hdr->src_ip = clib_net_to_host_u32(0xc0a8fffe);


    gre_hdr->flag_version = 0;
    gre_hdr->protocol_type = clib_net_to_host_u16(0x88BE);

    /*initial sf_erspan_ii_header*/
    sf_erspan_ii_header->Ver = 0x1;
    sf_erspan_ii_header->Vlan = 0;
    sf_erspan_ii_header->COS = 0;
    sf_erspan_ii_header->En = 0b11;
    sf_erspan_ii_header->T = 0;
    sf_erspan_ii_header->Session_ID = 0;
    sf_erspan_ii_header->Reserved = 0;
    sf_erspan_ii_header->Index = 0;

    /*initial sf_gre_option_template*/
    *sf_gre_option = 0x1;

    /*initial sf_erspan_iii_header_template*/
    sf_erspan_iii_header->Ver = 0x2;
    sf_erspan_iii_header->Vlan = 0;
    sf_erspan_iii_header->COS = 0;
    sf_erspan_iii_header->T = 0;
    sf_erspan_iii_header->Session_ID = 0;
    sf_erspan_iii_header->BSO = 0;
    sf_erspan_iii_header->Timestamp = 0;
    sf_erspan_iii_header->SGT = 0;      //clib_host_to_net_u16(0x);
    sf_erspan_iii_header->P = 0b1;
    sf_erspan_iii_header->FT = 0;
    sf_erspan_iii_header->Hw_ID = 0;
    sf_erspan_iii_header->D = 0;
    sf_erspan_iii_header->Gra = 0;
    sf_erspan_iii_header->O = 0;

    /*ipv6*/
    erspan_encap_common_hdr_v6_t *erspan_encap_common_hdr_ipv6 = &erspan_encap_common_hdr_template_ipv6;
    eth_header_t *eth_hdr_ipv6 = &(erspan_encap_common_hdr_ipv6->eth_hdr);
    ipv6_header_t *ip6_hdr = &(erspan_encap_common_hdr_ipv6->ipv6_hdr);
    sf_gre_header_t*gre_hdr_ipv6 = &(erspan_encap_common_hdr_ipv6->gre_hdr);
    

    sf_memset_zero(erspan_encap_common_hdr_ipv6 , sizeof(erspan_encap_common_hdr_v6_t));
    
    /*initial erspan_encap_common_hdr*/
    eth_hdr_ipv6->src_mac[0] = 0x11;
    eth_hdr_ipv6->src_mac[1] = 0x22;
    eth_hdr_ipv6->src_mac[2] = 0x33;
    eth_hdr_ipv6->src_mac[3] = 0x44;
    eth_hdr_ipv6->src_mac[4] = 0x55;
    eth_hdr_ipv6->src_mac[5] = 0x66;
    eth_hdr_ipv6->eth_type = ETHER_IPV6_NET_ORDER;

    *((uint32_t *)ip6_hdr) = 0;
    ip6_hdr->version = 0x06;

    ip6_hdr->payload_len = 0;
    ip6_hdr->next_header = IP_PRO_GRE;
    ip6_hdr->hop_limit = 0x40;
    ip6_hdr->src_ip.addr_v6_upper = clib_host_to_net_u64(0xabcdabcdabcdabcd);
    ip6_hdr->src_ip.addr_v6_lower = clib_host_to_net_u64(0x1234123412341234);


    gre_hdr_ipv6->flag_version = 0;
    gre_hdr_ipv6->protocol_type = clib_net_to_host_u16(0x88BE);


    return 0;
}
static clib_error_t *sf_time_stamp_erspan_init(vlib_main_t *vm)
{
    timestamp_time_t *time = &timestamp_time;
    sf_memset_zero(time , sizeof(timestamp_time));
    time->vlib_time = vlib_time_now(vm);
    time->milisecond_time  = unix_time_now_nsec();
    return 0;
}
VLIB_INIT_FUNCTION(sf_erspan_encap_common_hdr_t_init);
VLIB_INIT_FUNCTION(sf_time_stamp_erspan_init);
