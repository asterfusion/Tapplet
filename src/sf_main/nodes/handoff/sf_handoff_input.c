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
#include "sf_pkt_trace.h"

#define SF_DEBUG
#include "sf_debug.h"

typedef enum
{
    SF_NEXT_NODE_SF_ETH_INPUT,
    SF_NEXT_NODE_HUGE_PKT_INPUT,
    SF_NEXT_NODE_SF_PKT_DROP,
    SF_NEXT_NODE_SF_PKT_OUTPUT,
    SF_NEXT_NODE_N_COUNT,
} sf_index_t;


VLIB_REGISTER_NODE(sf_pkt_hf_input_node) = {
    __sf_node_reg_basic_trace__
    
    .name = "sf_pkt_hf_input",
    .vector_size = sizeof(u32),

    .n_next_nodes = SF_NEXT_NODE_N_COUNT,
    .next_nodes = {
        [SF_NEXT_NODE_SF_ETH_INPUT] = "sf_eth_input",
        [SF_NEXT_NODE_HUGE_PKT_INPUT] = "sf_hugepkt_input",
        [SF_NEXT_NODE_SF_PKT_DROP] = "sf_pkt_drop",
        [SF_NEXT_NODE_SF_PKT_OUTPUT] = "sf_pkt_output"
    },
};

VLIB_NODE_FN(sf_pkt_hf_input_node)
(vlib_main_t *vm,
           vlib_node_runtime_t *node,
           vlib_frame_t *frame)
{
    u32 n_left_from, *from, *to_next;
    u32 next_index;

    from = vlib_frame_vector_args(frame);
    n_left_from = frame->n_vectors;
    next_index = node->cached_next_index;

sf_debug("recv %d pkts\n" , n_left_from);

    u32 worker_index = sf_get_current_worker_index();

    uint64_t *handoff_cnt_array = sf_intf_get_handoff_cnt_array(worker_index);

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

            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);
            sf_wdata_t *pkt_wdata1 = sf_wdata(b1);

            
            next0 = SF_NEXT_NODE_SF_ETH_INPUT;
            next1 = SF_NEXT_NODE_SF_ETH_INPUT;

            handoff_cnt_array[pkt_wdata0->interface_id - 1] ++;
            handoff_cnt_array[pkt_wdata1->interface_id - 1] ++;


            if (pkt_wdata0->unused8 & UNUSED8_FLAG_HUGE_PKT)
            {
                next0 = SF_NEXT_NODE_HUGE_PKT_INPUT;
            }

            if (pkt_wdata1->unused8 & UNUSED8_FLAG_HUGE_PKT)
            {
                next1 = SF_NEXT_NODE_HUGE_PKT_INPUT;
            }


            /*****************************************************/
            sf_add_basic_trace_x2(vm , node , b0 , b1 , 
                    pkt_wdata0 , pkt_wdata1 , next0 ,next1);

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

            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);

            next0 = SF_NEXT_NODE_SF_ETH_INPUT;
            handoff_cnt_array[pkt_wdata0->interface_id - 1] ++;

            if (pkt_wdata0->unused8 & UNUSED8_FLAG_HUGE_PKT)
            {
                next0 = SF_NEXT_NODE_HUGE_PKT_INPUT;
            }

        
            /*****************************************************/
            sf_add_basic_trace_x1(vm , node , b0 ,  pkt_wdata0 , next0 );

            /* verify speculative enqueue, maybe switch current next frame */
            vlib_validate_buffer_enqueue_x1(vm, node, next_index,
                                            to_next, n_left_to_next,
                                            bi0, next0);
        }

        vlib_put_next_frame(vm, node, next_index, n_left_to_next);
    }

    return frame->n_vectors;
}
