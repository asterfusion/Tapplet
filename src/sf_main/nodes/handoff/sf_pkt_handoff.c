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
#include <vlib/node.h>
#include <vnet/vnet.h>
#include <vnet/pg/pg.h>
#include <vnet/feature/feature.h>
#include <vnet/ethernet/ethernet.h>
#include <vppinfra/error.h>
#include <vppinfra/xxhash.h>
#include <stdio.h>

#include "pkt_wdata.h"
#include "sf_thread_tools.h"

#include "sf_pkt_tools.h"
#include "sf_feature.h"

#define SF_DEBUG
#include "sf_debug.h"

typedef enum
{
    SF_NEXT_NODE_ETH_INPUT,
    SF_NEXT_NODE_HUGE_PKT_INPUT,
    SF_NEXT_NODE_SF_PKT_DROP,
    SF_NEXT_NODE_SF_PKT_OUTPUT,
    SF_NEXT_NODE_N_COUNT,
} sf_index_t;

typedef struct _sf_handoff_trace_t
{
    uint8_t interface_id;
    uint8_t next_worker_index;
} sf_handoff_trace_t;

/* packet trace format function */
static u8 *
sf_pkt_handoff_trace(u8 *s, va_list *args)
{
    CLIB_UNUSED(vlib_main_t * vm) = va_arg(*args, vlib_main_t *);
    CLIB_UNUSED(vlib_node_t * node) = va_arg(*args, vlib_node_t *);
    sf_handoff_trace_t *t = va_arg(*args, sf_handoff_trace_t *);

    s = format(s, "pkt_handoff: intf_id %d, next_worker_index %d\n",
               t->interface_id, t->next_worker_index);

    return s;
}

/********************/
typedef struct
{
    uint32_t *handoff_buffer_index;
    uint16_t *handoff_worker_index;
    uint32_t *self_buffer_index;
    uint16_t *self_next_index;
    uint32_t frame_next_index;
} pkt_handoff_runtime_t;

/********************/


VLIB_REGISTER_NODE(sf_pkt_handoff_node) = {
    .name = "sf_pkt_handoff",
    .vector_size = sizeof(u32),
    .format_trace = sf_pkt_handoff_trace,
    .runtime_data_bytes = sizeof(pkt_handoff_runtime_t),

    .n_next_nodes = SF_NEXT_NODE_N_COUNT,
    .next_nodes = {
        [SF_NEXT_NODE_ETH_INPUT] = "sf_eth_input",
        [SF_NEXT_NODE_HUGE_PKT_INPUT] = "sf_hugepkt_input",
        [SF_NEXT_NODE_SF_PKT_DROP] = "sf_pkt_drop",
        [SF_NEXT_NODE_SF_PKT_OUTPUT] = "sf_pkt_output"
    },
};

/*********************************/
#ifndef CLIB_MARCH_VARIANT
static clib_error_t *
pkt_handoff_runtime_init(vlib_main_t *vm)
{
    pkt_handoff_runtime_t *rt;

    rt = vlib_node_get_runtime_data(vm, sf_pkt_handoff_node.index);

    rt->frame_next_index = sf_intf_handoff_main.frame_next_index;

    vec_validate_aligned (rt->self_buffer_index, VLIB_FRAME_SIZE , CLIB_CACHE_LINE_BYTES);
    vec_validate_aligned (rt->self_next_index, VLIB_FRAME_SIZE , CLIB_CACHE_LINE_BYTES);
    vec_validate_aligned (rt->handoff_buffer_index, VLIB_FRAME_SIZE , CLIB_CACHE_LINE_BYTES);
    vec_validate_aligned (rt->handoff_worker_index, VLIB_FRAME_SIZE , CLIB_CACHE_LINE_BYTES);

    if( !(rt->self_buffer_index) 
        || !(rt->self_next_index) 
        || !(rt->handoff_worker_index)
        || !(rt->handoff_buffer_index) 
    )
    {
        clib_error(" pkt_handoff_runtime_init fail");
    }

    return 0;
}

VLIB_WORKER_INIT_FUNCTION( pkt_handoff_runtime_init);
#endif
/********************************/


static_always_inline 
uint16_t
sf_pkt_get_worker_index(eth_header_t *en , uint32_t num_workers)
{
    u64 hash_key = en->eth_type;

    if (PREDICT_TRUE(en->eth_type == ETHER_IPV4_NET_ORDER))
    {
        ipv4_header_t *ip = (ipv4_header_t *)(en + 1);
        hash_key = (u64)(ip->src_ip ^ ip->dst_ip ^ ip->protocol);
    }
    else if (en->eth_type == ETHER_IPV6_NET_ORDER)
    {
        ipv6_header_t *ip = (ipv6_header_t *)(en + 1);
        hash_key = (u64)(ip->src_ip.value[0] ^ ip->src_ip.value[1] ^
                         ip->dst_ip.value[0] ^ ip->dst_ip.value[1] ^ ip->next_header);
    }
    else if (PREDICT_FALSE((en->eth_type == ETHER_VLAN_NET_ORDER) || en->eth_type == ETHER_QINQ_NET_ORDER))
    {
        eth_header_t *eth = (eth_header_t *)(en + 1);
        eth = (eth->eth_type == ETHER_VLAN_NET_ORDER) ? eth + 1 : eth; //only support two layer
        if (PREDICT_TRUE(eth->eth_type == ETHER_IPV4_NET_ORDER))
        {
            ipv4_header_t *ip = (ipv4_header_t *)(eth + 1);
            hash_key = (u64)(ip->src_ip ^ ip->dst_ip ^ ip->protocol);
        }
        else if (eth->eth_type == ETHER_IPV6_NET_ORDER)
        {
            ipv6_header_t *ip = (ipv6_header_t *)(eth + 1);
            hash_key = (u64)(ip->src_ip.value[0] ^ ip->src_ip.value[1] ^
                             ip->dst_ip.value[0] ^ ip->dst_ip.value[1] ^ ip->next_header);
        }
        else
        {
            hash_key = eth->eth_type;
        }
    }

    hash_key = clib_xxhash(hash_key);

    if (is_pow2(num_workers))
    {
        return hash_key & (num_workers - 1);
    }
    else
    {
        return hash_key % num_workers;
    }
}

VLIB_NODE_FN(sf_pkt_handoff_node)
(vlib_main_t *vm,
 vlib_node_runtime_t *node,
 vlib_frame_t *frame)
{
    u32 n_left_from, *from;
    from = vlib_frame_vector_args(frame);
    n_left_from = frame->n_vectors;

    u32 worker_index = sf_get_current_worker_index();

    pkt_handoff_runtime_t *rt = (void *) node->runtime_data;

    uint32_t *self_buffer = rt->self_buffer_index;
    uint16_t *self_next = rt->self_next_index;

    uint32_t *handoff_buffer = rt->handoff_buffer_index;
    uint16_t *handoff_thread = rt->handoff_worker_index;

    uint32_t self_buffer_cnt = 0;
    uint32_t handoff_buffer_cnt = 0;

    uint32_t num_workers = sf_vlib_num_workers();

    uint64_t *handoff_cnt_array = sf_intf_get_handoff_cnt_array(worker_index);

    while (n_left_from > 0)
    {
        while (n_left_from >= 4  )
        {
            u32 bi0, bi1;
            vlib_buffer_t *b0, *b1;


            /* Prefetch next iteration. */
            {
                vlib_buffer_t *p2, *p3;

                p2 = vlib_get_buffer(vm, from[2]);
                p3 = vlib_get_buffer(vm, from[3]);

                sf_wdata_prefetch_cache(p2);
                sf_wdata_prefetch_cache(p3);

                CLIB_PREFETCH(p2->data, CLIB_CACHE_LINE_BYTES, LOAD);
                CLIB_PREFETCH(p3->data, CLIB_CACHE_LINE_BYTES, LOAD);
            }

            /* speculatively enqueue b0 and b1 to the current next frame */
            bi0 = from[0];
            bi1 = from[1];
            from += 2;
            n_left_from -= 2;

            b0 = vlib_get_buffer(vm, bi0);
            b1 = vlib_get_buffer(vm, bi1);

            /************** start handle pkt *********/
            eth_header_t *en0, *en1;
            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);
            sf_wdata_t *pkt_wdata1 = sf_wdata(b1);

            uint16_t next_worker0;
            uint16_t next_worker1;

            en0 = (eth_header_t *)sf_wqe_get_current_wdata(pkt_wdata0);
            en1 = (eth_header_t *)sf_wqe_get_current_wdata(pkt_wdata1);

            next_worker0 = sf_pkt_get_worker_index(en0 , num_workers);
            next_worker1 = sf_pkt_get_worker_index(en1 , num_workers);

            if(next_worker0 == worker_index)
            {
                self_buffer[0] = bi0;
                self_next[0] = SF_NEXT_NODE_ETH_INPUT;
                handoff_cnt_array[pkt_wdata0->interface_id - 1] ++;

                if (pkt_wdata0->unused8 & UNUSED8_FLAG_HUGE_PKT)
                {
                    self_next[0] = SF_NEXT_NODE_HUGE_PKT_INPUT;
                }

                self_buffer ++;
                self_next ++;
                self_buffer_cnt ++;
            }
            else
            {
                handoff_buffer[0] = bi0;
                handoff_thread[0] = next_worker0 + 1;

                handoff_buffer ++;
                handoff_thread ++;

                handoff_buffer_cnt ++;
            }

            if(next_worker1 == worker_index)
            {

                self_buffer[0] = bi1;
                self_next[0] = SF_NEXT_NODE_ETH_INPUT;
                handoff_cnt_array[pkt_wdata1->interface_id - 1] ++;

                if (pkt_wdata1->unused8 & UNUSED8_FLAG_HUGE_PKT)
                {
                    self_next[0] = SF_NEXT_NODE_HUGE_PKT_INPUT;
                }
                self_buffer ++;
                self_next ++;
                self_buffer_cnt ++;
            }
            else
            {
                handoff_buffer[0] = bi1;
                handoff_thread[0] = next_worker1 + 1;

                handoff_buffer ++;
                handoff_thread ++;

                handoff_buffer_cnt ++;
            }
            

            /********************************************/
            if (PREDICT_FALSE((node->flags & VLIB_NODE_FLAG_TRACE)))
            {
                if (is_vlib_b_traced(b0))
                {
                    sf_handoff_trace_t *t =
                        vlib_add_trace(vm, node, b0, sizeof(*t));
                    t->interface_id = pkt_wdata0->interface_id;
                    t->next_worker_index = next_worker0;
                }
                if (is_vlib_b_traced(b1))
                {
                    sf_handoff_trace_t *t =
                        vlib_add_trace(vm, node, b1, sizeof(*t));
                    t->interface_id = pkt_wdata1->interface_id;
                    t->next_worker_index = next_worker1;
                }
            }
        }

        while (n_left_from > 0 )
        {
            u32 bi0;
            vlib_buffer_t *b0;

            /* speculatively enqueue b0 to the current next frame */
            bi0 = from[0];
            from += 1;
            n_left_from -= 1;

            b0 = vlib_get_buffer(vm, bi0);

           /************** start handle pkt *********/
            eth_header_t *en0;

            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);

            uint16_t next_worker0;

            en0 = (eth_header_t *)sf_wqe_get_current_wdata(pkt_wdata0);

            next_worker0 = sf_pkt_get_worker_index(en0 , num_workers);

            if(next_worker0 == worker_index)
            {
                self_buffer[0] = bi0;
                self_next[0] = SF_NEXT_NODE_ETH_INPUT;
                handoff_cnt_array[pkt_wdata0->interface_id - 1] ++;

                if (pkt_wdata0->unused8 & UNUSED8_FLAG_HUGE_PKT)
                {
                    self_next[0] = SF_NEXT_NODE_HUGE_PKT_INPUT;
                }

                self_buffer ++;
                self_next ++;
                self_buffer_cnt ++;
            }
            else
            {
                handoff_buffer[0] = bi0;
                handoff_thread[0] = next_worker0 + 1;

                handoff_buffer ++;
                handoff_thread ++;

                handoff_buffer_cnt ++;
            }
            /********************************************/
            if (PREDICT_FALSE((node->flags & VLIB_NODE_FLAG_TRACE)))
            {
                if (is_vlib_b_traced(b0))
                {
                    sf_handoff_trace_t *t =
                        vlib_add_trace(vm, node, b0, sizeof(*t));
                    t->interface_id = pkt_wdata0->interface_id;
                    t->next_worker_index = next_worker0;
                }
            }
        }
    }

    if(handoff_buffer_cnt > 0)
        vlib_buffer_enqueue_to_thread(vm, 
            rt->frame_next_index , 
            rt->handoff_buffer_index, 
            rt->handoff_worker_index, 
            handoff_buffer_cnt, 0);

    if(self_buffer_cnt > 0)
        vlib_buffer_enqueue_to_next(vm , node , 
            rt->self_buffer_index, 
            rt->self_next_index, 
            self_buffer_cnt);

    return frame->n_vectors;
}

