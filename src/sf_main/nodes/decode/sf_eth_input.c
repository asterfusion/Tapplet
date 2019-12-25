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

#include "sf_main.h"
#include "common_header.h"

#include "sf_stat_tools.h"

#include "sf_main_shm.h"

#include "sf_eth_tool.h"

// #define SF_DEBUG
#include "sf_debug.h"

typedef enum
{
    SF_NEXT_NODE_SF_ACL_ENTRY,
    SF_NEXT_NODE_SF_IP4_INPUT , 
    SF_NEXT_NODE_SF_IP6_INPUT ,     
    SF_NEXT_NODE_SF_PKT_DROP,
    SF_NEXT_NODE_N_COUNT,
} sf_index_t;

typedef struct
{
    u32 next_index;
    u32 interface_id;
    u8 src_mac[6];
    u8 dst_mac[6];
    u16 eth_type;
} sf_trace_t;

static u8 *
sf_format_mac_address(u8 *s, va_list *args)
{
    u8 *a = va_arg(*args, u8 *);
    return format(s, "%02x:%02x:%02x:%02x:%02x:%02x",
                  a[0], a[1], a[2], a[3], a[4], a[5]);
}

/* packet trace format function */
static u8 *format_sf_trace(u8 *s, va_list *args)
{
    CLIB_UNUSED(vlib_main_t * vm) = va_arg(*args, vlib_main_t *);
    CLIB_UNUSED(vlib_node_t * node) = va_arg(*args, vlib_node_t *);
    sf_trace_t *t = va_arg(*args, sf_trace_t *);

    s = format(s, "sf_eth: interface_id %d, next index %d\n",
               t->interface_id, t->next_index);

    s = format(s, "  src %U -> dst %U  type: 0x%04x",
               sf_format_mac_address, t->src_mac,
               sf_format_mac_address, t->dst_mac,
               t->eth_type);

    return s;
}

/***************************/


VLIB_REGISTER_NODE(sf_eth_input_node) = {
    .name = "sf_eth_input",
    .vector_size = sizeof(u32),
    .format_trace = format_sf_trace,
    .type = VLIB_NODE_TYPE_INTERNAL,

    // .n_errors = ARRAY_LEN(test_error_strings),
    // .error_strings = test_error_strings,

    .n_next_nodes = SF_NEXT_NODE_N_COUNT,

    /* edit / add dispositions here */
    .next_nodes = {
        [SF_NEXT_NODE_SF_ACL_ENTRY] = "sf_acl_entry",
        [SF_NEXT_NODE_SF_IP4_INPUT] = "sf_ip4_input",
        [SF_NEXT_NODE_SF_IP6_INPUT] = "sf_ip6_input",
        [SF_NEXT_NODE_SF_PKT_DROP] = "sf_pkt_drop",
    },
};


/**
 * eth_type : in network order
 *  @return : ether type . in network order ; 0 mean error
*/
static uint16_t parse_eth_hdr(vlib_buffer_t *b0 , sf_wdata_t * pkt_wdata , uint16_t eth_type)
{
    // int32_t left_len = sf_wqe_get_current_len(b0);
    if(eth_type == ETHER_QINQ_NET_ORDER || eth_type == ETHER_VLAN_NET_ORDER)
    {
        eth_type = parse_vlan_hdr_only(b0 , pkt_wdata);
    }

    return eth_type;
}

/***
 * eth_type : in network order
 * @return : 0 means didn't find , so keep next_index(0) as a default next node
 */
always_inline uint32_t sf_eth_get_next_index(uint16_t  eth_type  )
{
    switch(eth_type)
    {
        case ETHER_IPV4_NET_ORDER:
            return SF_NEXT_NODE_SF_IP4_INPUT;
        case ETHER_IPV6_NET_ORDER:
            return SF_NEXT_NODE_SF_IP6_INPUT;
        default:
            return SF_NEXT_NODE_SF_ACL_ENTRY;
    }
}



VLIB_NODE_FN(sf_eth_input_node)
(vlib_main_t *vm,
           vlib_node_runtime_t *node,
           vlib_frame_t *frame)
{
    u32 n_left_from, *from, *to_next;
    u32 next_index;

    from = vlib_frame_vector_args(frame);
    n_left_from = frame->n_vectors;
    next_index = node->cached_next_index;

        sf_debug("recv %d pkts\n", n_left_from);

    while (n_left_from > 0)
    {
        u32 n_left_to_next;

        vlib_get_next_frame(vm, node, next_index,
                            to_next, n_left_to_next);

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

                sf_prefetch_data(p2);
                sf_prefetch_data(p3);
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

            eth_header_t *eth_hdr0;
            eth_header_t *eth_hdr1;
            uint16_t eth_type0;
            uint16_t eth_type1;
            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);
            sf_wdata_t *pkt_wdata1 = sf_wdata(b1);

            eth_hdr0 = (eth_header_t *)sf_wqe_get_current_wdata(pkt_wdata0);
            eth_hdr1 = (eth_header_t *)sf_wqe_get_current_wdata(pkt_wdata1);

            sf_wqe_advance_wdata(pkt_wdata0 , ETH_HEADER_LEN);
            sf_wqe_advance_wdata(pkt_wdata1 , ETH_HEADER_LEN);

            eth_type0 = parse_eth_hdr(b0 , pkt_wdata0 , eth_hdr0->eth_type );
            eth_type1 = parse_eth_hdr(b1 , pkt_wdata1 , eth_hdr1->eth_type );
    

            next0 = sf_eth_get_next_index(eth_type0);
            next1 = sf_eth_get_next_index(eth_type1);


            /****************************/
            if (PREDICT_FALSE ((node->flags & VLIB_NODE_FLAG_TRACE)))
            {
                if (pkt_wdata0->unused8 & UNUSED8_FLAG_PKT_TRACED)
                {
                    sf_trace_t *t =
                        vlib_add_trace (vm, node, b0, sizeof (*t));
                    t->interface_id = pkt_wdata0->interface_id;
                    clib_memcpy (&t->dst_mac, eth_hdr0->dst_mac, 6);
                    clib_memcpy (&t->src_mac, eth_hdr0->src_mac, 6);
                    t->eth_type = eth_type0;
                }
                if (pkt_wdata1->unused8 & UNUSED8_FLAG_PKT_TRACED)
                {
                    sf_trace_t *t =
                        vlib_add_trace (vm, node, b1, sizeof (*t));
                    t->interface_id = pkt_wdata1->interface_id;
                    clib_memcpy (&t->dst_mac, eth_hdr1->dst_mac, 6);
                    clib_memcpy (&t->src_mac, eth_hdr1->src_mac, 6);
                    t->eth_type = eth_type1;
                }
            }

            /*****************************************************/
            /* verify speculative enqueues, maybe switch current next frame */
            vlib_validate_buffer_enqueue_x2(vm, node, next_index,
                                            to_next, n_left_to_next,
                                            bi0, bi1, next0, next1);
        }

        while (n_left_from > 0 && n_left_to_next > 0)
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

            eth_header_t *eth_hdr0;
            uint16_t eth_type0;
            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);

            eth_hdr0 = (eth_header_t *)sf_wqe_get_current_wdata(pkt_wdata0);

            sf_wqe_advance_wdata(pkt_wdata0 , ETH_HEADER_LEN);

            eth_type0 = parse_eth_hdr(b0 , pkt_wdata0 , eth_hdr0->eth_type );
    
            next0 = sf_eth_get_next_index(eth_type0);

            /****************************/

            if (PREDICT_FALSE ((node->flags & VLIB_NODE_FLAG_TRACE)))
            {
                if (pkt_wdata0->unused8 & UNUSED8_FLAG_PKT_TRACED)
                {
                    sf_trace_t *t =
                        vlib_add_trace (vm, node, b0, sizeof (*t));
                    t->interface_id = pkt_wdata0->interface_id;
                    clib_memcpy (&t->dst_mac, eth_hdr0->dst_mac, 6);
                    clib_memcpy (&t->src_mac, eth_hdr0->src_mac, 6);
                    t->eth_type = eth_type0;
                }
            }

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

