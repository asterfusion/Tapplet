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
#include <vppinfra/error.h>


#include "pkt_wdata.h"
#include "sf_main.h"

#include "sf_interface_tools.h"

#include "sf_thread_tools.h"

#include "sf_huge_pkt_tools.h"

#include "sf_pkt_trace.h"

// #define SF_DEBUG
#include "sf_debug.h"


typedef enum
{
    SF_NEXT_NODE_HUGE_PKT_OUTPUT,
    SF_NEXT_NODE_SF_PKT_DROP,
    SF_NEXT_NODE_N_COUNT,
} sf_index_t;


/* *INDENT-OFF* */
VLIB_REGISTER_NODE(sf_pkt_output_node) = {
    __sf_node_reg_basic_trace__

    .name = "sf_pkt_output",
    .vector_size = sizeof(u32),
    .n_next_nodes = SF_NEXT_NODE_N_COUNT,

    /* edit / add dispositions here */
    .next_nodes = {
        [SF_NEXT_NODE_HUGE_PKT_OUTPUT] = "sf_huge_pkt_output",
        [SF_NEXT_NODE_SF_PKT_DROP] = "sf_pkt_drop",
    },
};
/* *INDENT-ON* */


VLIB_NODE_FN(sf_pkt_output_node)
(vlib_main_t *vm,
           vlib_node_runtime_t *node,
           vlib_frame_t *frame)
{



    u32 n_left_from, *from;
    u32 *to_next;
    u32 next_index;

    from = vlib_frame_vector_args(frame);
    n_left_from = frame->n_vectors;

    next_index = node->cached_next_index;

        sf_debug("recv %d pkts\n", n_left_from);
    uint32_t thread_index = sf_get_current_thread_index();

    uint64_t *intf_out_pkts = sf_intf_stat_main[thread_index].out_packets;
    uint64_t *intf_out_bytes = sf_intf_stat_main[thread_index].out_octets;


    uint8_t *intf_admin_status = interfaces_config->admin_status - 1; //!!! Notice !!!

    uint8_t *intf_next_index = sf_main.intf_output_next_index;


    sf_pool_per_thread_t *huge_16k_pool = get_huge_pkt_16k_pool(sf_get_current_worker_index());


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

            uint32_t intf_out_id_0 = pkt_wdata0->interface_out;
            uint32_t intf_out_id_1 = pkt_wdata1->interface_out;


            if(PREDICT_TRUE(intf_out_id_0 <= MAX_INTERFACE_CNT 
                && intf_out_id_0 > 0 
                && intf_admin_status[intf_out_id_0]))
            {                
                
                next0 = intf_next_index[ intf_out_id_0 ];

                if(PREDICT_FALSE(pkt_wdata0->unused8 & UNUSED8_FLAG_HUGE_PKT))
                {
                    next0 = SF_NEXT_NODE_HUGE_PKT_OUTPUT;
                }

                intf_out_pkts[intf_out_id_0 - 1] ++;
                intf_out_bytes[intf_out_id_0 - 1] += pkt_wdata0->current_length;
            }
            else
            {
                if(PREDICT_FALSE(pkt_wdata0->unused8 & UNUSED8_FLAG_HUGE_PKT))
                {
                    free_huge_pkt_16k( huge_16k_pool , pkt_wdata0->data_ptr );
                }
            }


            if(PREDICT_TRUE(intf_out_id_1 <= MAX_INTERFACE_CNT 
                && intf_out_id_1 > 0 
                && intf_admin_status[intf_out_id_1]))
            {                
                
                next1 = intf_next_index[ intf_out_id_1 ];

                if(PREDICT_FALSE(pkt_wdata1->unused8 & UNUSED8_FLAG_HUGE_PKT))
                {
                    next1 = SF_NEXT_NODE_HUGE_PKT_OUTPUT;
                }

                intf_out_pkts[intf_out_id_1 - 1] ++;
                intf_out_bytes[intf_out_id_1 - 1] += pkt_wdata1->current_length;
            }
            else
            {
                if(PREDICT_FALSE(pkt_wdata1->unused8 & UNUSED8_FLAG_HUGE_PKT))
                {
                    free_huge_pkt_16k( huge_16k_pool , pkt_wdata1->data_ptr );
                }
            }

            /******************************************************/
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
            bi0 = from[0];
            to_next[0] = bi0;
            from += 1;
            to_next += 1;
            n_left_from -= 1;
            n_left_to_next -= 1;

            b0 = vlib_get_buffer(vm, bi0);
            /*******************  start handle pkt  ***************/
            u32 next0 = SF_NEXT_NODE_SF_PKT_DROP;

            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);

            uint32_t intf_out_id_0 = pkt_wdata0->interface_out;


            if(PREDICT_TRUE(intf_out_id_0 <= MAX_INTERFACE_CNT 
                && intf_out_id_0 > 0 
                && intf_admin_status[intf_out_id_0]))
            {                
                
                next0 = intf_next_index[ intf_out_id_0 ];

                if(PREDICT_FALSE(pkt_wdata0->unused8 & UNUSED8_FLAG_HUGE_PKT))
                {
                    next0 = SF_NEXT_NODE_HUGE_PKT_OUTPUT;
                }           

                intf_out_pkts[intf_out_id_0 - 1] ++;
                intf_out_bytes[intf_out_id_0 - 1] += pkt_wdata0->current_length;
            }
            else
            {
                if(PREDICT_FALSE(pkt_wdata0->unused8 & UNUSED8_FLAG_HUGE_PKT))
                {
                    free_huge_pkt_16k( huge_16k_pool , pkt_wdata0->data_ptr );
                }
            }


            /******************************************************/
            sf_add_basic_trace_x1(vm , node , b0 ,pkt_wdata0 , next0 );   

            /* verify speculative enqueue, maybe switch current next frame */
            vlib_validate_buffer_enqueue_x1(vm, node, next_index,
                                            to_next, n_left_to_next,
                                            bi0, next0);
        }

        vlib_put_next_frame(vm, node, next_index, n_left_to_next);
    }
    
    return frame->n_vectors;
}
