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
#include <vnet/ip/ip_packet.h>	/* for ip_csum_t */
#include <vnet/pg/pg.h>
#include <vppinfra/error.h>

#include <stdio.h>

#include "sf_main.h"
#include "common_header.h"
#include "pkt_wdata.h"

// #define SF_DEBUG
#include "sf_debug.h"

typedef enum
{
    SF_NEXT_NODE_SF_ACL_ENTRY,
    SF_NEXT_NODE_UDP_INPUT , 
    SF_NEXT_NODE_TCP_INPUT,
    SF_NEXT_NODE_IP4_REASS,
    SF_NEXT_NODE_N_COUNT,
} sf_index_t;

typedef struct
{
    u32 next_index;
    u32 interface_id;
    u32 src_ip4;
    u32 dst_ip4;
} sf_trace_t;

static u8 *
format_ip_address(u8 *s, va_list *args)
{
    u8 *a = va_arg(*args, u8 *);
    return format(s, "%d.%d.%d.%d",
                  a[0], a[1], a[2], a[3]);
}

/* packet trace format function */
static u8 *format_sf_trace(u8 *s, va_list *args)
{
    CLIB_UNUSED(vlib_main_t * vm) = va_arg(*args, vlib_main_t *);
    CLIB_UNUSED(vlib_node_t * node) = va_arg(*args, vlib_node_t *);
    sf_trace_t *t = va_arg(*args, sf_trace_t *);

    s = format(s, "ip4: interface_id %d, next index %d\n",
               t->interface_id, t->next_index);

    s = format(s, "  src %U -> dst %U ",
               format_ip_address, &(t->src_ip4),
               format_ip_address, &(t->dst_ip4));

    return s;
}

/***************************/

VLIB_REGISTER_NODE(sf_ip4_input_node) = {
    .name = "sf_ip4_input",
    .vector_size = sizeof(u32),
    .format_trace = format_sf_trace,
    .type = VLIB_NODE_TYPE_INTERNAL,
    .n_next_nodes = SF_NEXT_NODE_N_COUNT,

    /* edit / add dispositions here */
    .next_nodes = {
        [SF_NEXT_NODE_SF_ACL_ENTRY] = "sf_acl_entry",
        [SF_NEXT_NODE_UDP_INPUT] = "sf_udp_input",
        [SF_NEXT_NODE_TCP_INPUT] = "sf_tcp_input",
        [SF_NEXT_NODE_IP4_REASS] = "sf_ip4_reass",
    },
};



always_inline  void 
ip4_decode(sf_wdata_t *pkt_wdata0 , uint32_t *next0 , uint32_t ip_frag_next , uint8_t *need_lookup)
{

    ipv4_header_t *ipv4_hdr_0;
    sf_wqe_len_t ip4_hdr_len0;
    ip_wdata_t *ip_wdata0;

    ipv4_hdr_0 = (ipv4_header_t *)sf_wqe_get_current_wdata(pkt_wdata0);

    ip4_hdr_len0 = (ipv4_hdr_0->ipHeaderLen << 2);

    *need_lookup = 0;

    if(PREDICT_FALSE(pkt_wdata0->ip_wdata_cnt >= MAX_IP_WDATA))
    {
        *next0 = SF_NEXT_NODE_SF_ACL_ENTRY;
    }
    else if(PREDICT_FALSE(sf_wqe_get_cur_len_wdata(pkt_wdata0) < ip4_hdr_len0))
    {
        set_packet_type( pkt_wdata0 , PACKET_TYPE_ERROR);
        *next0 = SF_NEXT_NODE_SF_ACL_ENTRY;
    }
    else
    {
        
        ip_wdata0 = &(pkt_wdata0->l3l4_wdatas.ip_wdata);
        pkt_wdata0->ip_wdata_cnt ++;

        update_ipv4_wdata(
            ip_wdata0 ,
            sf_wqe_get_cur_offset_wdata(pkt_wdata0),
            ipv4_hdr_0->src_ip , 
            ipv4_hdr_0->dst_ip , 
            ipv4_hdr_0->protocol ,
            ip4_hdr_len0 ,
            clib_net_to_host_u16(ipv4_hdr_0->totalLen) - ip4_hdr_len0 ,
            ip4_is_frag(ipv4_hdr_0)
        );

        if( !(ip_wdata0->is_frag) )
        {
            sf_wqe_advance_wdata(pkt_wdata0 , ip4_hdr_len0);
            *need_lookup = ipv4_hdr_0->protocol;
        }
        else
        {
            *next0 = ip_frag_next;
        }
        
    }
}

always_inline uint32_t sf_ip4_get_next_index(uint8_t  protocol )
{
    if(protocol == IP_PRO_UDP)
    {
        return SF_NEXT_NODE_UDP_INPUT;
    }
    else if(protocol == IP_PRO_TCP)
    {
        return SF_NEXT_NODE_TCP_INPUT;
    }
    return SF_NEXT_NODE_SF_ACL_ENTRY;

}


VLIB_NODE_FN(sf_ip4_input_node)
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

    uint32_t ip_frag_next = SF_NEXT_NODE_IP4_REASS;

    while (n_left_from > 0)
    {
        u32 n_left_to_next;

        vlib_get_next_frame(vm, node, next_index,
                            to_next, n_left_to_next);

        while (n_left_from >= 4 && n_left_to_next >= 2)
        {
            u32 bi0, bi1;
            vlib_buffer_t *b0, *b1;

            /* Prefetch next iteration. */
            {
                vlib_buffer_t *p2, *p3;

                p2 = vlib_get_buffer(vm, from[2]);
                p3 = vlib_get_buffer(vm, from[3]);

                sf_wdata_prefetch(p2);
                sf_wdata_prefetch(p3);

                sf_prefetch_data(p2);
                sf_prefetch_data(p3);
            }

            /* speculatively enqueue b0 and b1 to the current next frame */
            to_next[0] = bi0 = from[0];
            to_next[1] = bi1 = from[1];
            from += 2;
            to_next += 2;
            n_left_from -= 2;
            n_left_to_next -= 2;

            b0 = vlib_get_buffer(vm, bi0);
            b1 = vlib_get_buffer(vm, bi1);

            /************** start handle pkt *********/
            u32 next0 = 0;
            u32 next1 = 0;

            uint8_t need_lookup_0 = 0;
            uint8_t need_lookup_1 = 0;

            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);
            sf_wdata_t *pkt_wdata1 = sf_wdata(b1);

            ip4_decode(pkt_wdata0 , &next0 , ip_frag_next , &need_lookup_0);
            ip4_decode(pkt_wdata1 , &next1 , ip_frag_next , &need_lookup_1);


            if(need_lookup_0)
            {
                next0 = sf_ip4_get_next_index(need_lookup_0);
            }
            if(need_lookup_1)
            {
                next1 = sf_ip4_get_next_index(need_lookup_1);
            }

            /****************************/

            if (PREDICT_FALSE((node->flags & VLIB_NODE_FLAG_TRACE)))
            {
                if (pkt_wdata0->unused8 & UNUSED8_FLAG_PKT_TRACED)
                {
                    sf_trace_t *t =
                        vlib_add_trace(vm, node, b0, sizeof(*t));
                    ip_wdata_t *ip_wdata0 = get_current_ip_wdata(pkt_wdata0);

                    t->interface_id = pkt_wdata0->interface_id;
                    t->next_index = next0;
                    t->src_ip4 = ip_wdata0->sip.addr_v4;
                    t->dst_ip4 = ip_wdata0->dip.addr_v4;
                }
                if (pkt_wdata1->unused8 & UNUSED8_FLAG_PKT_TRACED)
                {
                    sf_trace_t *t =
                        vlib_add_trace(vm, node, b1, sizeof(*t));
                    ip_wdata_t *ip_wdata1 = get_current_ip_wdata(pkt_wdata1);
                    t->interface_id = pkt_wdata1->interface_id;
                    t->next_index = next1;
                    t->src_ip4 = ip_wdata1->sip.addr_v4;
                    t->dst_ip4 = ip_wdata1->dip.addr_v4;
                }
            }

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
            bi0 = from[0];
            to_next[0] = bi0;
            from += 1;
            to_next += 1;
            n_left_from -= 1;
            n_left_to_next -= 1;

            b0 = vlib_get_buffer(vm, bi0);
           /************** start handle pkt *********/
            u32 next0 = 0;

            uint8_t need_lookup_0 = 0;

            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);

            ip4_decode(pkt_wdata0 , &next0 , ip_frag_next , &need_lookup_0);


            if(need_lookup_0)
            {
                next0 = sf_ip4_get_next_index(need_lookup_0);
            }

            /****************************/

            if (PREDICT_FALSE((node->flags & VLIB_NODE_FLAG_TRACE)))
            {
                if (pkt_wdata0->unused8 & UNUSED8_FLAG_PKT_TRACED)
                {
                    sf_trace_t *t =
                        vlib_add_trace(vm, node, b0, sizeof(*t));
                    ip_wdata_t *ip_wdata0 = get_current_ip_wdata(pkt_wdata0);

                    t->interface_id = pkt_wdata0->interface_id;
                    t->next_index = next0;
                    t->src_ip4 = ip_wdata0->sip.addr_v4;
                    t->dst_ip4 = ip_wdata0->dip.addr_v4;
                }
            }


            /* verify speculative enqueue, maybe switch current next frame */
            vlib_validate_buffer_enqueue_x1(vm, node, next_index,
                                            to_next, n_left_to_next,
                                            bi0, next0);
        }

        vlib_put_next_frame(vm, node, next_index, n_left_to_next);
    }

    return frame->n_vectors;
}

