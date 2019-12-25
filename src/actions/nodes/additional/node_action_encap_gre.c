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

typedef struct{
    eth_header_t eth_hdr;
    ipv4_header_t ipv4_hdr;
    sf_gre_header_t gre_hdr;
}__attribute__((packed))  gre_encap_hdr_t;

typedef struct{
    eth_header_t eth_hdr;
    ipv6_header_t ipv6_hdr;
    sf_gre_header_t gre_hdr;
}__attribute__((packed))  gre_encap_hdr_v6_t;

#ifndef CLIB_MARCH_VARIANT
gre_encap_hdr_t gre_encap_hdr_template;
gre_encap_hdr_v6_t gre_encap_hdr_template_ipv6;
#else
extern gre_encap_hdr_t gre_encap_hdr_template;
extern gre_encap_hdr_v6_t gre_encap_hdr_template_ipv6;
#endif

VLIB_REGISTER_NODE(sf_encap_gre_node) = {
    .name = "node_action_encap_gre",
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


VLIB_NODE_FN(sf_encap_gre_node)
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
            
            if(action_0->additional_actions.gre_dip_type == 4)
            {
                gre_encap_hdr_t *gre_encap_ptr_0 = (gre_encap_hdr_t *)(sf_wqe_get_current(b0) - sizeof(gre_encap_hdr_t));


                clib_memcpy(gre_encap_ptr_0 , &gre_encap_hdr_template , sizeof(gre_encap_hdr_t));

                //mac
                clib_memcpy(gre_encap_ptr_0->eth_hdr.src_mac , interfaces_config->interfaces[pkt_wdata_0->interface_id-1].mac , 6);
                clib_memcpy(gre_encap_ptr_0->eth_hdr.dst_mac , action_0->additional_actions.gre_dmac , 6);
                //dscp
                gre_encap_ptr_0->ipv4_hdr.ip_tos = action_0->additional_actions.gre_dscp;
                //sip
    	    if(interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv4_addr[0] || 
    	       interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv4_addr[1] || 
    	       interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv4_addr[2] || 
    	       interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv4_addr[3])
                {
                   clib_memcpy(&(gre_encap_ptr_0->ipv4_hdr.src_ip), (uint32_t *)interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv4_addr, 4);
                }
                //dip
                gre_encap_ptr_0->ipv4_hdr.dst_ip = action_0->additional_actions.gre_dip.addr_v4;
                //total_len
                gre_encap_ptr_0->ipv4_hdr.totalLen = clib_host_to_net_u16(
                     IPV4_HEADER_LEN + GRE_HEADER_LEN  + sf_wqe_get_current_len(b0));
    
                //checksum 
                gre_encap_ptr_0->ipv4_hdr.checksum = 
                    sf_ip4_chechsum( &(gre_encap_ptr_0->ipv4_hdr));
    
                sf_wqe_advance(b0 , - (sizeof(gre_encap_hdr_t)));
            }
            else if(action_0->additional_actions.gre_dip_type == 6)
            {
                gre_encap_hdr_v6_t *gre_encap_ptr_0_ipv6 = (gre_encap_hdr_v6_t *)(sf_wqe_get_current(b0) - sizeof(gre_encap_hdr_v6_t));

                clib_memcpy(gre_encap_ptr_0_ipv6 , &gre_encap_hdr_template_ipv6 , sizeof(gre_encap_hdr_v6_t));

                //mac
                clib_memcpy(gre_encap_ptr_0_ipv6->eth_hdr.src_mac , interfaces_config->interfaces[pkt_wdata_0->interface_id-1].mac , 6);
                clib_memcpy(gre_encap_ptr_0_ipv6->eth_hdr.dst_mac , action_0->additional_actions.gre_dmac , 6);
                //dscp
                // gre_encap_ptr_0_ipv6->ipv6_hdr.traffic_class = action_0->additional_actions.gre_dscp;
                ip6_hdr_set_traffic_class( &(gre_encap_ptr_0_ipv6->ipv6_hdr) , 
                    action_0->additional_actions.gre_dscp);
                //sip
    	    if( interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv6_addr.addr_v6_upper ||
                interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv6_addr.addr_v6_lower) 
                {
                   clib_memcpy(&(gre_encap_ptr_0_ipv6->ipv6_hdr.src_ip), &(interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv6_addr), sizeof(ip_addr_t));
                }
                //dip
                clib_memcpy(&(gre_encap_ptr_0_ipv6->ipv6_hdr.dst_ip), &(action_0->additional_actions.gre_dip) , sizeof(ip_addr_t));
                //total_len
                gre_encap_ptr_0_ipv6->ipv6_hdr.payload_len = clib_host_to_net_u16(
                     GRE_HEADER_LEN  + sf_wqe_get_current_len(b0));
                
                //flow_label + traffic_class + version = 32bits ->change order
                // *(uint32_t *)&gre_encap_ptr_0_ipv6->ipv6_hdr = clib_host_to_net_u32(*(uint32_t *)&gre_encap_ptr_0_ipv6->ipv6_hdr);
                
                sf_wqe_advance(b0 , - (sizeof(gre_encap_hdr_v6_t)));
            }
            if(action_1->additional_actions.gre_dip_type == 4)
            {
                gre_encap_hdr_t *gre_encap_ptr_1 = (gre_encap_hdr_t *)(sf_wqe_get_current(b1) - sizeof(gre_encap_hdr_t));


                clib_memcpy(gre_encap_ptr_1 , &gre_encap_hdr_template , sizeof(gre_encap_hdr_t));

                //mac
                clib_memcpy(gre_encap_ptr_1->eth_hdr.src_mac , interfaces_config->interfaces[pkt_wdata_1->interface_id-1].mac , 6);
                clib_memcpy(gre_encap_ptr_1->eth_hdr.dst_mac , action_1->additional_actions.gre_dmac , 6);
                //dscp
                gre_encap_ptr_1->ipv4_hdr.ip_tos = action_1->additional_actions.gre_dscp;
                //sip
    	    if(interfaces_config->interfaces[pkt_wdata_1->interface_id-1].ipv4_addr[0] || 
    	       interfaces_config->interfaces[pkt_wdata_1->interface_id-1].ipv4_addr[1] || 
    	       interfaces_config->interfaces[pkt_wdata_1->interface_id-1].ipv4_addr[2] || 
    	       interfaces_config->interfaces[pkt_wdata_1->interface_id-1].ipv4_addr[3])
                {
                   clib_memcpy(&(gre_encap_ptr_1->ipv4_hdr.src_ip), (uint32_t *)interfaces_config->interfaces[pkt_wdata_1->interface_id-1].ipv4_addr, 4);
                }
                //dip
                gre_encap_ptr_1->ipv4_hdr.dst_ip = action_1->additional_actions.gre_dip.addr_v4;
                //total_len
                gre_encap_ptr_1->ipv4_hdr.totalLen = clib_host_to_net_u16(
                     IPV4_HEADER_LEN + GRE_HEADER_LEN  + sf_wqe_get_current_len(b1));
    
                //checksum  
                gre_encap_ptr_1->ipv4_hdr.checksum = 
                    sf_ip4_chechsum( &(gre_encap_ptr_1->ipv4_hdr) );
    
                sf_wqe_advance(b1 , - (sizeof(gre_encap_hdr_t)));
            }
            else if(action_1->additional_actions.gre_dip_type == 6)
            {
                gre_encap_hdr_v6_t *gre_encap_ptr_1_ipv6 = (gre_encap_hdr_v6_t *)(sf_wqe_get_current(b1) - sizeof(gre_encap_hdr_v6_t));

                clib_memcpy(gre_encap_ptr_1_ipv6 , &gre_encap_hdr_template_ipv6 , sizeof(gre_encap_hdr_v6_t));

                //mac
                clib_memcpy(gre_encap_ptr_1_ipv6->eth_hdr.src_mac , interfaces_config->interfaces[pkt_wdata_1->interface_id-1].mac , 6);
                clib_memcpy(gre_encap_ptr_1_ipv6->eth_hdr.dst_mac , action_1->additional_actions.gre_dmac , 6);
                //dscp
                // gre_encap_ptr_1_ipv6->ipv6_hdr.traffic_class = action_1->additional_actions.gre_dscp;
                ip6_hdr_set_traffic_class( &(gre_encap_ptr_1_ipv6->ipv6_hdr) , 
                    action_1->additional_actions.gre_dscp);                
                //sip
    	    if( interfaces_config->interfaces[pkt_wdata_1->interface_id-1].ipv6_addr.addr_v6_upper ||
                interfaces_config->interfaces[pkt_wdata_1->interface_id-1].ipv6_addr.addr_v6_lower) 
                {
                   clib_memcpy(&(gre_encap_ptr_1_ipv6->ipv6_hdr.src_ip), &(interfaces_config->interfaces[pkt_wdata_1->interface_id-1].ipv6_addr), sizeof(ip_addr_t));
                }
                //dip
                clib_memcpy(&(gre_encap_ptr_1_ipv6->ipv6_hdr.dst_ip), &(action_1->additional_actions.gre_dip) , sizeof(ip_addr_t));
                //total_len
                gre_encap_ptr_1_ipv6->ipv6_hdr.payload_len = clib_host_to_net_u16(
                     GRE_HEADER_LEN  + sf_wqe_get_current_len(b1));
                
                //flow_label + traffic_class + version = 32bits ->change order
                // *(uint32_t *)&gre_encap_ptr_1_ipv6->ipv6_hdr = clib_host_to_net_u32(*(uint32_t *)&gre_encap_ptr_1_ipv6->ipv6_hdr);
                
                sf_wqe_advance(b1 , - (sizeof(gre_encap_hdr_v6_t)));
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

            if(action_0->additional_actions.gre_dip_type == 4)
            {
                gre_encap_hdr_t *gre_encap_ptr_0 = (gre_encap_hdr_t *)(sf_wqe_get_current(b0) - sizeof(gre_encap_hdr_t));


                clib_memcpy(gre_encap_ptr_0 , &gre_encap_hdr_template , sizeof(gre_encap_hdr_t));

                //mac
                clib_memcpy(gre_encap_ptr_0->eth_hdr.src_mac , interfaces_config->interfaces[pkt_wdata_0->interface_id-1].mac , 6);
                clib_memcpy(gre_encap_ptr_0->eth_hdr.dst_mac , action_0->additional_actions.gre_dmac , 6);
                //dscp
                gre_encap_ptr_0->ipv4_hdr.ip_tos = action_0->additional_actions.gre_dscp;
                //sip
    	    if(interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv4_addr[0] || 
    	       interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv4_addr[1] || 
    	       interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv4_addr[2] || 
    	       interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv4_addr[3])
                {
                   clib_memcpy(&(gre_encap_ptr_0->ipv4_hdr.src_ip), (uint32_t *)interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv4_addr, 4);
                }
                //dip
                gre_encap_ptr_0->ipv4_hdr.dst_ip = action_0->additional_actions.gre_dip.addr_v4;
                //total_len
                gre_encap_ptr_0->ipv4_hdr.totalLen = clib_host_to_net_u16(
                     IPV4_HEADER_LEN + GRE_HEADER_LEN  + sf_wqe_get_current_len(b0));
    
                //checksum 
                gre_encap_ptr_0->ipv4_hdr.checksum = 
                    sf_ip4_chechsum( &(gre_encap_ptr_0->ipv4_hdr) );
    
                sf_wqe_advance(b0 , - (sizeof(gre_encap_hdr_t)));
            }
            else if(action_0->additional_actions.gre_dip_type == 6)
            {
                gre_encap_hdr_v6_t *gre_encap_ptr_0_ipv6 = (gre_encap_hdr_v6_t *)(sf_wqe_get_current(b0) - sizeof(gre_encap_hdr_v6_t));

                clib_memcpy(gre_encap_ptr_0_ipv6 , &gre_encap_hdr_template_ipv6 , sizeof(gre_encap_hdr_v6_t));

                //mac
                clib_memcpy(gre_encap_ptr_0_ipv6->eth_hdr.src_mac , interfaces_config->interfaces[pkt_wdata_0->interface_id-1].mac , 6);
                clib_memcpy(gre_encap_ptr_0_ipv6->eth_hdr.dst_mac , action_0->additional_actions.gre_dmac , 6);
                //dscp
                // gre_encap_ptr_0_ipv6->ipv6_hdr.traffic_class = action_0->additional_actions.gre_dscp;
                ip6_hdr_set_traffic_class( &(gre_encap_ptr_0_ipv6->ipv6_hdr) , 
                    action_0->additional_actions.gre_dscp);
                //sip
    	    if( interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv6_addr.addr_v6_upper ||
                interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv6_addr.addr_v6_lower) 
                {
                   clib_memcpy(&(gre_encap_ptr_0_ipv6->ipv6_hdr.src_ip), &(interfaces_config->interfaces[pkt_wdata_0->interface_id-1].ipv6_addr), sizeof(ip_addr_t));
                }
                //dip
                clib_memcpy(&(gre_encap_ptr_0_ipv6->ipv6_hdr.dst_ip), &(action_0->additional_actions.gre_dip) , sizeof(ip_addr_t));
                //total_len
                gre_encap_ptr_0_ipv6->ipv6_hdr.payload_len = clib_host_to_net_u16(
                     GRE_HEADER_LEN  + sf_wqe_get_current_len(b0));
                
                //flow_label + traffic_class + version = 32bits ->change order
                // *(uint32_t *)&gre_encap_ptr_0_ipv6->ipv6_hdr = clib_host_to_net_u32(*(uint32_t *)&gre_encap_ptr_0_ipv6->ipv6_hdr);
                
                sf_wqe_advance(b0 , - (sizeof(gre_encap_hdr_v6_t)));
            }

            next0 = pop_addtional_list(addi_list_ptr_0);

            /*****************************************************/

            /* verify speculative enqueue, maybe switch current next frame */
            vlib_validate_buffer_enqueue_x1(vm, node, next_index,
                                            to_next, n_left_to_next,
                                            bi0, next0);
        }

        vlib_put_next_frame(vm, node, next_index, n_left_to_next);
    }
    return frame->n_vectors;
}


static clib_error_t *sf_gre_hdr_template_init(vlib_main_t *vm)
{
    /*ipv4*/
    gre_encap_hdr_t *gre_encap_hdr = &gre_encap_hdr_template;
    eth_header_t *eth_hdr = &(gre_encap_hdr->eth_hdr);
    ipv4_header_t *ip4_hdr = &(gre_encap_hdr->ipv4_hdr);
    sf_gre_header_t *gre_hdr = &(gre_encap_hdr->gre_hdr);

    sf_memset_zero(gre_encap_hdr , sizeof(gre_encap_hdr_t));
    eth_hdr->src_mac[0] = 0x11;
    eth_hdr->src_mac[1] = 0x22;
    eth_hdr->src_mac[2] = 0x33;
    eth_hdr->src_mac[3] = 0x44;
    eth_hdr->src_mac[4] = 0x55;
    eth_hdr->src_mac[5] = 0x66;
    eth_hdr->eth_type = ETHER_IPV4_NET_ORDER;

    ip4_hdr->version = 0x04;
    ip4_hdr->ipHeaderLen = 0x05;
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
    gre_hdr->protocol_type = clib_net_to_host_u16(0x6558);



    /*ipv6*/
    gre_encap_hdr_v6_t *gre_encap_hdr_ipv6 = &gre_encap_hdr_template_ipv6;
    eth_header_t *eth_hdr_ipv6 = &(gre_encap_hdr_ipv6->eth_hdr);
    ipv6_header_t *ip6_hdr = &(gre_encap_hdr_ipv6->ipv6_hdr);
    sf_gre_header_t *gre_hdr_ipv6 = &(gre_encap_hdr_ipv6->gre_hdr);

    sf_memset_zero(gre_encap_hdr_ipv6 , sizeof(gre_encap_hdr_v6_t));
    
    eth_hdr_ipv6->src_mac[0] = 0x11;
    eth_hdr_ipv6->src_mac[1] = 0x22;
    eth_hdr_ipv6->src_mac[2] = 0x33;
    eth_hdr_ipv6->src_mac[3] = 0x44;
    eth_hdr_ipv6->src_mac[4] = 0x55;
    eth_hdr_ipv6->src_mac[5] = 0x66;
    eth_hdr_ipv6->eth_type = ETHER_IPV6_NET_ORDER;

    *(uint32_t *)ip6_hdr = 0;
    ip6_hdr->version = 0x06;
    // ip6_hdr->traffic_class = 0;
    // ip6_hdr->flow_label = 0;
    ip6_hdr->payload_len = 0;
    ip6_hdr->next_header = IP_PRO_GRE;
    ip6_hdr->hop_limit = 0x40;
    ip6_hdr->src_ip.addr_v6_upper = clib_host_to_net_u64(0xabcdabcdabcdabcd);
    ip6_hdr->src_ip.addr_v6_lower = clib_host_to_net_u64(0x1234123412341234);

    gre_hdr_ipv6->flag_version = 0;
    gre_hdr_ipv6->protocol_type = clib_net_to_host_u16(0x6558);
    
    return 0;
}
VLIB_INIT_FUNCTION(sf_gre_hdr_template_init);
