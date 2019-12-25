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
#include "pkt_wdata.h"

// #define SF_DEBUG
#include "sf_debug.h"

typedef enum
{
    SF_NEXT_NODE_SF_ACL_ENTRY,
    
    SF_NEXT_NODE_UDP_INPUT , 
    SF_NEXT_NODE_TCP_INPUT,

    SF_NEXT_NODE_IP6_REASS,

    SF_NEXT_NODE_N_COUNT,
} sf_index_t;

typedef struct
{
    u32 next_index;
    u32 interface_id;
    uint8_t src_ip6[16];
    uint8_t dst_ip6[16];
    //   u16 ip6_type;
} sf_trace_t;

static u8 *
sf_format_ip6_address(u8 *s, va_list *args)
{
    u8 *a = va_arg(*args, u8 *);
    return format(s, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
                  a[0], a[1], a[2], a[3], a[6], a[5], a[6], a[7]);
}

/* packet trace format function */
static u8 *format_sf_trace(u8 *s, va_list *args)
{
    CLIB_UNUSED(vlib_main_t * vm) = va_arg(*args, vlib_main_t *);
    CLIB_UNUSED(vlib_node_t * node) = va_arg(*args, vlib_node_t *);
    sf_trace_t *t = va_arg(*args, sf_trace_t *);

    s = format(s, "ip6: interface_id %d, next index %d\n",
               t->interface_id, t->next_index);

    s = format(s, "  src %U -> dst %U ",
               sf_format_ip6_address, &(t->src_ip6),
               sf_format_ip6_address, &(t->dst_ip6));

    return s;
}

/***************************/

VLIB_REGISTER_NODE(sf_ip6_input_node) = {
    .name = "sf_ip6_input",
    .vector_size = sizeof(u32),
    .format_trace = format_sf_trace,
    .type = VLIB_NODE_TYPE_INTERNAL,
    .n_next_nodes = SF_NEXT_NODE_N_COUNT,

    /* edit / add dispositions here */
    .next_nodes = {
        [SF_NEXT_NODE_SF_ACL_ENTRY] = "sf_acl_entry",

        [SF_NEXT_NODE_UDP_INPUT] = "sf_udp_input",
        [SF_NEXT_NODE_TCP_INPUT] = "sf_tcp_input",
        [SF_NEXT_NODE_IP6_REASS] = "sf_ip6_reass",
    },
};

static_always_inline void
parse_ipv6_hdr(sf_wdata_t *pkt_wdata, ipv6_header_t *ipv6_hdr,
               sf_wqe_len_t *hdr_len, uint8_t *is_frag, uint8_t *protocol)
{
    ipv6_ext_t *ipv6_ext_hdr = NULL;
    ipv6_frag_t *ipv6_frag_hdr = NULL;
    uint8_t *pre_data = (uint8_t *)ipv6_hdr;

    sf_wqe_len_t left_len = sf_wqe_get_cur_len_wdata(pkt_wdata);

    uint8_t stop_flag = 0;
    sf_wqe_len_t ext_hdr_len;

    if(PREDICT_FALSE(left_len < IPV6_HEADER_LEN))
    {
        *protocol = IP_PRO_INVALID;
        set_packet_type( pkt_wdata , PACKET_TYPE_ERROR);

        return;
    }

    left_len -= IPV6_HEADER_LEN;
    *hdr_len = IPV6_HEADER_LEN;
    pre_data += IPV6_HEADER_LEN;

    *protocol = ipv6_hdr->next_header;
    while (!stop_flag)
    {
        switch (*protocol)
        {
        case IP_PRO_IPV6_FRAG:
            if(PREDICT_FALSE(left_len <= IPV6_FRAG_HEADER_LEN))
            {
                stop_flag = 1;
                *protocol = IP_PRO_INVALID;
            }
            else
            {
                ipv6_frag_hdr = (ipv6_frag_t *)pre_data;
                *protocol = ipv6_frag_hdr->next_header;
                // *is_frag = 1;
                left_len -= IPV6_FRAG_HEADER_LEN;
                *hdr_len += IPV6_FRAG_HEADER_LEN;
                pre_data += IPV6_FRAG_HEADER_LEN;
                //TODO !!!
                if(ipv6_frag_hdr->ip6f_flags || get_ip6_frag_offset(ipv6_frag_hdr) )
                {
                    sf_debug("ipv6 is frag: stop\n");
                    sf_debug("flags : %d\n" , ipv6_frag_hdr->ip6f_flags);
                    sf_debug("offset : %d\n" , get_ip6_frag_offset(ipv6_frag_hdr));
                    sf_debug("%p \n" , ipv6_frag_hdr);
                    stop_flag = 1;
                    *is_frag = 1;
                }
            }
            
            break;

        case IP_PRO_AH:

            ipv6_ext_hdr = (ipv6_ext_t *)pre_data;
            *protocol = ipv6_ext_hdr->next_header;
            ext_hdr_len = (ipv6_ext_hdr->hdr_len + 2) << 2;
            if(PREDICT_FALSE(left_len <= ext_hdr_len))
            {
                stop_flag = 1;
                *protocol = IP_PRO_INVALID;
            }
            else
            {
                left_len -= ext_hdr_len;
                *hdr_len += ext_hdr_len;
                pre_data += ext_hdr_len;
            }
            break;

        case IP_PRO_HOPOPT:
        case IP_PRO_ROUTING:
        case IP_PRO_DSTOPTS:
            ipv6_ext_hdr = (ipv6_ext_t *)pre_data;
            *protocol = ipv6_ext_hdr->next_header;
            ext_hdr_len = (ipv6_ext_hdr->hdr_len + 1) << 3;
            if(PREDICT_FALSE(left_len <= ext_hdr_len))
            {
                stop_flag = 1;
                *protocol = IP_PRO_INVALID;
            }
            else
            {
                left_len -= ext_hdr_len;
                *hdr_len += ext_hdr_len;
                pre_data += ext_hdr_len;
            }
            break;

        default:
            stop_flag = 1;
            break;
        }
    }

    if(*protocol == IP_PRO_INVALID)
    {
        set_packet_type( pkt_wdata , PACKET_TYPE_ERROR);
    }
}


always_inline uint32_t sf_ip6_get_next_index(
    uint32_t frag_next_index , 
    uint8_t  ip_protocol  , uint8_t is_frag  )
{

    if(is_frag)
    {
        return frag_next_index;
    }

    if(ip_protocol == IP_PRO_TCP)
    {
        return SF_NEXT_NODE_TCP_INPUT;
    }
    else if(ip_protocol == IP_PRO_UDP)
    {
        return SF_NEXT_NODE_UDP_INPUT;
    }

    return SF_NEXT_NODE_SF_ACL_ENTRY;
    

}



VLIB_NODE_FN(sf_ip6_input_node)
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

    uint32_t ip_frag_next = SF_NEXT_NODE_IP6_REASS;
    

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
            u32 next0 = SF_NEXT_NODE_SF_ACL_ENTRY;
            u32 next1 = SF_NEXT_NODE_SF_ACL_ENTRY;

            ipv6_header_t *ipv6_hdr_0;
            ipv6_header_t *ipv6_hdr_1;
            sf_wqe_len_t ip6_hdr_len0;
            sf_wqe_len_t ip6_hdr_len1;
            sf_wqe_len_t ip6_payload_len0;
            sf_wqe_len_t ip6_payload_len1;
            uint8_t inside_protocol0 = IP_PRO_INVALID;
            uint8_t inside_protocol1 = IP_PRO_INVALID;
            uint8_t is_frag0 = 0;
            uint8_t is_frag1 = 0;


            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);
            sf_wdata_t *pkt_wdata1 = sf_wdata(b1);
            ip_wdata_t *ip_wdata0;
            ip_wdata_t *ip_wdata1;

            ipv6_hdr_0 = (ipv6_header_t *)sf_wqe_get_current_wdata(pkt_wdata0);
            ipv6_hdr_1 = (ipv6_header_t *)sf_wqe_get_current_wdata(pkt_wdata1);

            parse_ipv6_hdr(pkt_wdata0, ipv6_hdr_0, &ip6_hdr_len0, &is_frag0, &inside_protocol0);
            parse_ipv6_hdr(pkt_wdata1, ipv6_hdr_1, &ip6_hdr_len1, &is_frag1, &inside_protocol1);

            ip6_payload_len0 = clib_net_to_host_u16(ipv6_hdr_0->payload_len) + IPV6_HEADER_LEN - ip6_hdr_len0;
            ip6_payload_len1 = clib_net_to_host_u16(ipv6_hdr_1->payload_len) + IPV6_HEADER_LEN - ip6_hdr_len1;


            if(inside_protocol0 != IP_PRO_INVALID)
            {
                ip_wdata0 = &(pkt_wdata0->l3l4_wdatas.ip_wdata);
                pkt_wdata0->ip_wdata_cnt ++;

                update_ipv6_wdata(
                    ip_wdata0, 
                    sf_wqe_get_cur_offset_wdata(pkt_wdata0),
                    &(ipv6_hdr_0->src_ip), 
                    &(ipv6_hdr_0->dst_ip), 
                    inside_protocol0 ,
                    ip6_hdr_len0 , ip6_payload_len0 , is_frag0);


                next0 = sf_ip6_get_next_index(ip_frag_next , inside_protocol0, is_frag0 );

                if ( !is_frag0 )
                {
                    sf_wqe_advance_wdata(pkt_wdata0, ip6_hdr_len0);
                }
            }

            if ( inside_protocol1 != IP_PRO_INVALID )
            {
                ip_wdata1 = &(pkt_wdata1->l3l4_wdatas.ip_wdata);
                pkt_wdata1->ip_wdata_cnt ++;

                update_ipv6_wdata(
                    ip_wdata1, 
                    sf_wqe_get_cur_offset_wdata(pkt_wdata1),
                    &(ipv6_hdr_1->src_ip), 
                    &(ipv6_hdr_1->dst_ip), 
                    inside_protocol1 ,
                    ip6_hdr_len1 , ip6_payload_len1 , is_frag1);


                next1 = sf_ip6_get_next_index(ip_frag_next , inside_protocol1, is_frag1 );

                if ( !is_frag1 )
                {
                    sf_wqe_advance_wdata(pkt_wdata1, ip6_hdr_len1);
                }
            }


            /****************************/

            if (PREDICT_FALSE((node->flags & VLIB_NODE_FLAG_TRACE)))
            {
                if (pkt_wdata0->unused8 & UNUSED8_FLAG_PKT_TRACED)
                {
                    sf_trace_t *t =
                        vlib_add_trace(vm, node, b0, sizeof(*t));
                    t->interface_id = pkt_wdata0->interface_id;
                    t->next_index = next0;
                
                    memcpy(t->src_ip6 , ipv6_hdr_0->src_ip.ipv6_byte , 16);
                    memcpy(t->dst_ip6 , ipv6_hdr_0->dst_ip.ipv6_byte , 16);
                }
                if (pkt_wdata1->unused8 & UNUSED8_FLAG_PKT_TRACED)
                {
                    sf_trace_t *t =
                        vlib_add_trace(vm, node, b1, sizeof(*t));
                    t->interface_id = pkt_wdata1->interface_id;
                    t->next_index = next1;
                     
                    memcpy(t->src_ip6 , ipv6_hdr_1->src_ip.ipv6_byte , 16);
                    memcpy(t->dst_ip6 , ipv6_hdr_1->dst_ip.ipv6_byte , 16);
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
            /*******************  start handle pkt  ***************/
            u32 next0 = SF_NEXT_NODE_SF_ACL_ENTRY;

            ipv6_header_t *ipv6_hdr_0;
            sf_wqe_len_t ip6_hdr_len0;
            sf_wqe_len_t ip6_payload_len0;
            uint8_t inside_protocol0 = IP_PRO_INVALID;
            uint8_t is_frag0 = 0;

            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);
            ip_wdata_t *ip_wdata0;

            ipv6_hdr_0 = (ipv6_header_t *)sf_wqe_get_current_wdata(pkt_wdata0);

            parse_ipv6_hdr(pkt_wdata0, ipv6_hdr_0, &ip6_hdr_len0, &is_frag0, &inside_protocol0);

            ip6_payload_len0 = clib_net_to_host_u16(ipv6_hdr_0->payload_len) + IPV6_HEADER_LEN - ip6_hdr_len0;

            if(PREDICT_FALSE(pkt_wdata0->ip_wdata_cnt >= MAX_IP_WDATA))
            {
                next0 = SF_NEXT_NODE_SF_ACL_ENTRY;
            }
            else if (PREDICT_FALSE(inside_protocol0 == IP_PRO_INVALID))
            {
                set_packet_type( pkt_wdata0 , PACKET_TYPE_ERROR);
                next0 = SF_NEXT_NODE_SF_ACL_ENTRY;
            }
            else
            {
                ip_wdata0 = &(pkt_wdata0->l3l4_wdatas.ip_wdata);
                pkt_wdata0->ip_wdata_cnt ++;

                update_ipv6_wdata(
                    ip_wdata0, 
                    sf_wqe_get_cur_offset_wdata(pkt_wdata0),
                    &(ipv6_hdr_0->src_ip), 
                    &(ipv6_hdr_0->dst_ip), 
                    inside_protocol0 ,
                    ip6_hdr_len0 , ip6_payload_len0 , is_frag0);


                next0 = sf_ip6_get_next_index(ip_frag_next  , inside_protocol0, is_frag0 );

                if ( !is_frag0 )
                {
                    sf_wqe_advance_wdata(pkt_wdata0, ip6_hdr_len0);
                }
            }
            /****************************/

            if (PREDICT_FALSE((node->flags & VLIB_NODE_FLAG_TRACE)))
            {
                if (pkt_wdata0->unused8 & UNUSED8_FLAG_PKT_TRACED)
                {
                    sf_trace_t *t =
                        vlib_add_trace(vm, node, b0, sizeof(*t));
                    t->interface_id = pkt_wdata0->interface_id;
                    t->next_index = next0;
                
                    memcpy(t->src_ip6 , ipv6_hdr_0->src_ip.ipv6_byte , 16);
                    memcpy(t->dst_ip6 , ipv6_hdr_0->dst_ip.ipv6_byte , 16);
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
