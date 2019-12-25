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

#include "sf_main.h"
#include "common_header.h"
#include "pkt_wdata.h"


#include "sf_pkt_trace.h"

//#define SF_DEBUG
#include "sf_debug.h"


typedef enum
{
    SF_NEXT_NODE_SF_ACL_ENTRY,
    SF_NEXT_NODE_N_COUNT,
} sf_index_t;

typedef struct
{
    u32 next_index;
    u32 interface_id;
    u16 src_port;
    u16 dst_port;
} sf_trace_t;

/* packet trace format function */
static u8 *format_sf_trace(u8 *s, va_list *args)
{
    CLIB_UNUSED(vlib_main_t * vm) = va_arg(*args, vlib_main_t *);
    CLIB_UNUSED(vlib_node_t * node) = va_arg(*args, vlib_node_t *);
    sf_trace_t *t = va_arg(*args, sf_trace_t *);

    s = format(s, "tcp: interface_id %d, next index %d\n",
               t->interface_id, t->next_index);

    s = format(s, "  src %d -> dst %d ",
               t->src_port, t->dst_port);

    return s;
}


/****************************/
VLIB_REGISTER_NODE(sf_tcp_input_node) = {
    .name = "sf_tcp_input",
    .vector_size = sizeof(u32),
    .format_trace = format_sf_trace,
    .type = VLIB_NODE_TYPE_INTERNAL,

    .n_next_nodes = SF_NEXT_NODE_N_COUNT,

    /* edit / add dispositions here */
    .next_nodes = {
        [SF_NEXT_NODE_SF_ACL_ENTRY] = "sf_acl_entry",
    },
};



// 0x17 : 010111
//flag : URG    ACK  PUSH   RST   SYN   FIN
//first : [0|1] 0    [0|1]  0     1     0
#define is_tcp_handshake_first(flag)  (((flag) & 0x17) == 0x02) 
// 0x12 : 010010
//flag   : URG    ACK  PUSH    RST     SYN   FIN
//second : [0|1]  1    [0|1]   [0|1]   1     [0|1]
#define is_tcp_handshake_second(flag) (((flag) & 0x12) == 0x12) 


always_inline uint32_t get_tcp_default_next_index()
{

    return SF_NEXT_NODE_SF_ACL_ENTRY;

}


static_always_inline void sf_tcp_decode(sf_wdata_t *pkt_wdata0 , uint32_t *next)
{
    sf_tcp_header_t *tcp_hdr_0;
    sf_wqe_len_t tcp_hdr_len0;

    tcp_hdr_0 = (sf_tcp_header_t *)sf_wqe_get_current_wdata(pkt_wdata0);
    
    tcp_hdr_len0 = tcp_hdr_0->hdr_len << 2;

    if(PREDICT_FALSE(
            sf_wqe_get_cur_len_wdata(pkt_wdata0) < tcp_hdr_len0
        )
    )
    {
        set_packet_type( pkt_wdata0 , PACKET_TYPE_ERROR);
        *next = SF_NEXT_NODE_SF_ACL_ENTRY;
        return ;
    }

    pkt_wdata0->arg1_to_next = tcp_hdr_0->src_port;
    pkt_wdata0->arg2_to_next = tcp_hdr_0->dst_port;

    update_l4_wdata(pkt_wdata0 , 
        sf_wqe_get_cur_offset_wdata(pkt_wdata0) , 
        tcp_hdr_0->src_port , tcp_hdr_0->dst_port , 
        0 , tcp_hdr_len0);

    sf_wqe_advance_wdata(pkt_wdata0, tcp_hdr_len0);
}


VLIB_NODE_FN(sf_tcp_input_node)
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

            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);
            sf_wdata_t *pkt_wdata1 = sf_wdata(b1);

            sf_tcp_decode(pkt_wdata0 , &next0);
            sf_tcp_decode(pkt_wdata1 , &next1);


            /****************************/

            if (PREDICT_FALSE((node->flags & VLIB_NODE_FLAG_TRACE)))
            {
                if (pkt_wdata0->unused8 & UNUSED8_FLAG_PKT_TRACED)
                {
                    sf_trace_t *t =
                        vlib_add_trace(vm, node, b0, sizeof(*t));
                    t->interface_id = pkt_wdata0->interface_id;
                    t->next_index = next0;
                     t->src_port = clib_net_to_host_u16(pkt_wdata0->arg1_to_next);
                    t->dst_port = clib_net_to_host_u16(pkt_wdata0->arg2_to_next);
                }
                if (pkt_wdata1->unused8 & UNUSED8_FLAG_PKT_TRACED)
                {
                    sf_trace_t *t =
                        vlib_add_trace(vm, node, b1, sizeof(*t));
                    t->interface_id = pkt_wdata1->interface_id;
                    t->next_index = next1;
                     t->src_port = clib_net_to_host_u16(pkt_wdata1->arg1_to_next);
                    t->dst_port = clib_net_to_host_u16(pkt_wdata1->arg2_to_next);
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
            u32 next0 = SF_NEXT_NODE_SF_ACL_ENTRY;

            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);

            sf_tcp_decode(pkt_wdata0 , &next0);

            /****************************/

            if (PREDICT_FALSE((node->flags & VLIB_NODE_FLAG_TRACE)))
            {
                if (pkt_wdata0->unused8 & UNUSED8_FLAG_PKT_TRACED)
                {
                    sf_trace_t *t =
                        vlib_add_trace(vm, node, b0, sizeof(*t));
                    t->interface_id = pkt_wdata0->interface_id;
                    t->next_index = next0;
                    t->src_port = clib_net_to_host_u16(pkt_wdata0->arg1_to_next);
                    t->dst_port = clib_net_to_host_u16(pkt_wdata0->arg2_to_next);
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
