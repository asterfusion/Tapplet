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


#include "pkt_wdata.h"
#include "sf_main.h"

#include "sf_pkt_tools.h"

#include "sf_interface_tools.h"

#include "sf_thread_tools.h"

#include "sf_huge_pkt_tools.h"

#include "sf_pkt_trace.h"


// #define SF_DEBUG
#include "sf_debug.h"


/* *INDENT-OFF* */
VLIB_REGISTER_NODE(sf_pkt_drop_node) = {
    __sf_node_reg_basic_trace__

    .name = "sf_pkt_drop",
    .vector_size = sizeof(u32),
    .flags = VLIB_NODE_FLAG_IS_DROP,
};
/* *INDENT-ON* */

VLIB_NODE_FN(sf_pkt_drop_node)
(vlib_main_t *vm,
           vlib_node_runtime_t *node,
           vlib_frame_t *frame)
{


    u32 n_left_from, *from;
    // u32 *first_buffer;

    // u32 *to_next;
    // u32 next_index;

    from = vlib_frame_vector_args(frame);
    // first_buffer = from;
    n_left_from = frame->n_vectors;

    uint32_t thread_index = sf_get_current_thread_index();

    uint64_t *intf_drop_pkts = sf_intf_stat_main[thread_index].drop_packets;

    sf_pool_per_thread_t *huge_16k_pool = get_huge_pkt_16k_pool(sf_get_current_worker_index());

    sf_debug("recv %d pkts\n", n_left_from);



    while (n_left_from >= 4)
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
        }

        /* speculatively enqueue b0 and b1 to the current next frame */
        bi0 = from[0];
        bi1 = from[1];
        from += 2;
        n_left_from -= 2;
        // n_left_to_next -= 2;

        b0 = vlib_get_buffer(vm, bi0);
        b1 = vlib_get_buffer(vm, bi1);

        /************** start handle pkt *********/
        sf_wdata_t *pkt_wdata0 = sf_wdata(b0);
        sf_wdata_t *pkt_wdata1 = sf_wdata(b1);


        if(pkt_wdata0->unused8 & UNUSED8_FLAG_HUGE_PKT )
        {
            free_huge_pkt_16k(huge_16k_pool , pkt_wdata0->data_ptr);
        }
        if(pkt_wdata1->unused8 & UNUSED8_FLAG_HUGE_PKT )
        {
            free_huge_pkt_16k(huge_16k_pool , pkt_wdata1->data_ptr);
        }

        if(pkt_wdata0->unused8 & UNUSED8_FLAG_PKT_RECORD_DROP 
            && pkt_wdata0->interface_id > 0 && pkt_wdata0->interface_id <= MAX_INTERFACE_CNT)
        {
            intf_drop_pkts[pkt_wdata0->interface_id - 1] += 1;
        }
        if(pkt_wdata1->unused8 & UNUSED8_FLAG_PKT_RECORD_DROP 
            && pkt_wdata1->interface_id > 0 && pkt_wdata1->interface_id <= MAX_INTERFACE_CNT)
        {
            intf_drop_pkts[pkt_wdata1->interface_id - 1] += 1;
        }

        /****************************/
        sf_add_basic_trace_x2(vm , node , b0 , b1 , 
                    pkt_wdata0 , pkt_wdata1 , 0xFFFF , 0xFFFF);
    }

    while (n_left_from > 0)
    {
        u32 bi0;
        vlib_buffer_t *b0;

        /* speculatively enqueue b0 to the current next frame */
        bi0 = from[0];
        from += 1;
        n_left_from -= 1;

        b0 = vlib_get_buffer(vm, bi0);
        /*******************  start handle pkt  ***************/
        sf_wdata_t *pkt_wdata0 = sf_wdata(b0);

        if(pkt_wdata0->unused8 & UNUSED8_FLAG_HUGE_PKT )
        {
            free_huge_pkt_16k(huge_16k_pool , pkt_wdata0->data_ptr);
        }

        if(pkt_wdata0->unused8 & UNUSED8_FLAG_PKT_RECORD_DROP 
            && pkt_wdata0->interface_id > 0 && pkt_wdata0->interface_id <= MAX_INTERFACE_CNT)
        {
            intf_drop_pkts[pkt_wdata0->interface_id - 1] += 1;
        }

        /****************************/
        sf_add_basic_trace_x1(vm , node , b0 ,pkt_wdata0 , 0xFFFF );   
       
    }

    
    // from = vlib_frame_vector_args(frame);
    // n_left_from = frame->n_vectors;
    // sf_push_pkt_to_next(vm , node , SF_NEXT_NODE_ERROR_DROP , from , n_left_from);
    from = vlib_frame_vector_args(frame);
    n_left_from = frame->n_vectors;
    sf_vlib_buffer_free_no_next(vm , from , n_left_from);
    
    return frame->n_vectors;
}


// static_always_inline void
// show_frame_stat(vlib_main_t *vm)
// {
//     vlib_node_main_t *nm = &vm->node_main;
//     vlib_frame_size_t *fs;
//     int i;

//     sf_debug("%=6s%=12s%=12s\n", "Size", "# Alloc", "# Free");
//     vec_foreach(fs, nm->frame_sizes)
//     {
//         u32 n_alloc = fs->n_alloc_frames;
//         u32 n_free = vec_len(fs->free_frame_indices);

//         if (n_alloc + n_free > 0)
//             sf_debug("%=6d%=12d%=12d\n",
//                      fs - nm->frame_sizes, n_alloc, n_free);
//         // sf_debug("free indices : %d first %d sec %d\n" ,  vec_len (fs->free_frame_indices) ,
//         //   fs->free_frame_indices[0] , fs->free_frame_indices[1]);

//         for (i = 0; i < vec_len(fs->free_frame_indices); i++)
//         {
//             fformat(stdout, "%d : %d\n", i, fs->free_frame_indices[i]);
//         }
//     }
// }
