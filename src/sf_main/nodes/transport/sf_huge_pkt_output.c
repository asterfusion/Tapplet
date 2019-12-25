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

#include "sf_interface_tools.h"
#include "sf_huge_pkt_tools.h"

#include "sf_pkt_trace.h"

// #define SF_DEBUG
#include "sf_debug.h"

/********************/
typedef struct
{
    uint32_t *buffer_index;
} hugepkt_output_runtime_t;

/********************/

typedef enum
{
    SF_NEXT_NODE_SF_PKT_DROP,
    SF_NEXT_NODE_N_COUNT,
} sf_index_t;


/* *INDENT-OFF* */
VLIB_REGISTER_NODE(sf_huge_pkt_output_node) = {
    __sf_node_reg_basic_trace__

    .name = "sf_huge_pkt_output",
    .vector_size = sizeof(u32),
    .n_next_nodes = SF_NEXT_NODE_N_COUNT,
    .runtime_data_bytes = sizeof(hugepkt_output_runtime_t),

    /* edit / add dispositions here */
    .next_nodes = {
        [SF_NEXT_NODE_SF_PKT_DROP] = "sf_pkt_drop",
    },
};
/* *INDENT-ON* */
/*********************************/
#ifndef CLIB_MARCH_VARIANT
static clib_error_t *
hugepkt_output_runtime_init(vlib_main_t *vm)
{
    hugepkt_output_runtime_t *rt;

    rt = vlib_node_get_runtime_data(vm, sf_huge_pkt_output_node.index);

    vec_validate_aligned (rt->buffer_index, SF_HUFEPKT_MAX_SUB_BUFFER_CNT , CLIB_CACHE_LINE_BYTES);

    if( !(rt->buffer_index) ) 
    {
        clib_error("hugepkt_output_runtime_init fail");
    }

    return 0;
}

VLIB_WORKER_INIT_FUNCTION(hugepkt_output_runtime_init);
#endif
/********************************/

//VLIB_BUFFER_DATA_SIZE

// 2048 = 2^10
#define SF_VLIB_BUFFER_DATA_SIZE_SHIT 11
#define SF_VLIB_BUFFER_DATA_SIZE_MASK ((uint32_t)( (1 << SF_VLIB_BUFFER_DATA_SIZE_SHIT)  - 1))


#define min(a,b) ((a) > (b) ? (b) : (a))

#define OUTPUT_TAG_PRE_SIZE 8

always_inline 
uint32_t sf_vlib_buffer_split(
    vlib_main_t *vm , vlib_buffer_t *b0 , 
    sf_wdata_t *wdata0 , uint32_t *buffer_index
    )
{
    int i;
    uint32_t total_len = wdata0->current_length;
    uint32_t copyed_len ;
    uint8_t *src_data_ptr = sf_wqe_get_current_wdata(wdata0);
    sf_hugepkt_helper_t *hugepkt_helper = sf_hugepkt_helper(wdata0);
    sf_hugepkt_sub_t *pre_sub_buffer = hugepkt_helper->sub_buffer;
    uint32_t sub_buffer_cnt = ((total_len + OUTPUT_TAG_PRE_SIZE) >> SF_VLIB_BUFFER_DATA_SIZE_SHIT);

    sub_buffer_cnt += ((total_len + OUTPUT_TAG_PRE_SIZE)  & SF_VLIB_BUFFER_DATA_SIZE_MASK) ? 1 : 0;

    hugepkt_helper->sub_buffer_cnt = sub_buffer_cnt;


    sf_debug("total_len : %d data %d\n" , wdata0->current_length , wdata0->current_data);

    // too long
    if(PREDICT_FALSE(sub_buffer_cnt > SF_HUFEPKT_MAX_SUB_BUFFER_CNT))
    {
        return 0;
    }

    //first buffer
    sub_buffer_cnt -= 1;

    if(sub_buffer_cnt > 0)
    {
        sub_buffer_cnt = sf_vlib_alloc_no_fail(vm , buffer_index , sub_buffer_cnt);

        //no vlib buffer
        if(sub_buffer_cnt == 0 )
        {
            return 0;
        }

        wdata0->unused8 |= UNUSED8_FLAG_HUGE_PKT; // !! Notice !!!
    }

    //copy first buffer
    copyed_len = 0;
    wdata0->current_data = OUTPUT_TAG_PRE_SIZE ; // !!! Notice !!!!

    pre_sub_buffer->buffer_index = vlib_get_buffer_index(vm , b0);
    pre_sub_buffer->current_data = OUTPUT_TAG_PRE_SIZE;
    pre_sub_buffer->current_length = min(total_len , VLIB_BUFFER_DATA_SIZE - OUTPUT_TAG_PRE_SIZE); //we may add some tag

    clib_memcpy(b0->data + OUTPUT_TAG_PRE_SIZE , src_data_ptr ,  pre_sub_buffer->current_length);
        

    src_data_ptr += pre_sub_buffer->current_length;
    total_len -= pre_sub_buffer->current_length;
    copyed_len += pre_sub_buffer->current_length;

    pre_sub_buffer ++;

    //left buffer
    for(i=0; i<sub_buffer_cnt ; i++ )
    {
        pre_sub_buffer->buffer_index = buffer_index[i];

        b0 = vlib_get_buffer(vm , buffer_index[i]);

        pre_sub_buffer->current_data = 0;
        pre_sub_buffer->current_length = min(total_len , VLIB_BUFFER_DATA_SIZE);

        clib_memcpy(b0->data , src_data_ptr ,  pre_sub_buffer->current_length);

        sf_debug("data :%d len %d\n" , pre_sub_buffer->current_data , pre_sub_buffer->current_length);

        src_data_ptr += pre_sub_buffer->current_length;
        total_len -= pre_sub_buffer->current_length;
        copyed_len += pre_sub_buffer->current_length;
        
        pre_sub_buffer++;
    }

    sf_debug("total len : %d copy len %d\n" , total_len , copyed_len);

    SF_ASSERT(total_len == 0);

    return copyed_len;
}

extern int sw_if_next_indexs_self[10];



VLIB_NODE_FN(sf_huge_pkt_output_node)
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

    int thread_index = sf_get_current_thread_index();

    sf_pool_per_thread_t *huge_16k_pool = get_huge_pkt_16k_pool(sf_get_current_worker_index());
    hugepkt_output_runtime_t *rt = (void *) node->runtime_data;

    uint32_t *temp_buffer_index = rt->buffer_index;


    uint8_t *intf_next_index = sf_main.hugepkt_output_next_index;

    uint8_t unset_huge_mask = UNUSED8_FLAG_HUGE_PKT;
    unset_huge_mask = ~unset_huge_mask;

sf_debug("recv %d pkts\n" , n_left_from);

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

                sf_wdata_prefetch(p2);
                sf_wdata_prefetch(p3);
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
            uint32_t ret0;
            uint32_t ret1;
            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);
            sf_wdata_t *pkt_wdata1 = sf_wdata(b1);


            ret0 = sf_vlib_buffer_split(vm , b0 , pkt_wdata0 , temp_buffer_index);
            ret1 = sf_vlib_buffer_split(vm , b1 , pkt_wdata1 , temp_buffer_index);

            free_huge_pkt_16k(huge_16k_pool , pkt_wdata0->data_ptr);
            free_huge_pkt_16k(huge_16k_pool , pkt_wdata1->data_ptr);

            next0 = intf_next_index[pkt_wdata0->interface_out];
            next1 = intf_next_index[pkt_wdata1->interface_out];

            if(PREDICT_FALSE(ret0 == 0  ))
            {
                pkt_wdata0->unused8 &= unset_huge_mask; // so drop node don't have to consider hugepkt

                next0 = SF_NEXT_NODE_SF_PKT_DROP;
                //we have added intf stat in sf_pkt_output, so decrease now
                decrease_interface_stat(thread_index , pkt_wdata0->interface_out , out_packets);
                update_interface_stat_multi(thread_index , pkt_wdata0->interface_out , out_octets , -(pkt_wdata0->current_length) );

            }

            if(PREDICT_FALSE(ret1 == 0))
            {
                pkt_wdata1->unused8 &= unset_huge_mask;

                next1 = SF_NEXT_NODE_SF_PKT_DROP;
                //we have added intf stat in sf_pkt_output, so decrease now
                decrease_interface_stat(thread_index , pkt_wdata1->interface_out , out_packets);
                update_interface_stat_multi(thread_index , pkt_wdata1->interface_out , out_octets , -(pkt_wdata1->current_length) );

            }

            /****************************/
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
            uint32_t ret0;
            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);

            pkt_wdata0->unused8 &= unset_huge_mask;

            ret0 = sf_vlib_buffer_split(vm , b0 , pkt_wdata0 , temp_buffer_index);
            next0 = intf_next_index[pkt_wdata0->interface_out];

            free_huge_pkt_16k(huge_16k_pool , pkt_wdata0->data_ptr);

            if(PREDICT_FALSE(ret0 == 0  ))
            {
                pkt_wdata0->unused8 &= unset_huge_mask; // so drop node don't have to consider hugepkt

                next0 = SF_NEXT_NODE_SF_PKT_DROP;
                //we have added intf stat in sf_pkt_output, so decrease now
                decrease_interface_stat(thread_index , pkt_wdata0->interface_out , out_packets);
                update_interface_stat_multi(thread_index , pkt_wdata0->interface_out , out_octets , -(pkt_wdata0->current_length) );

            }

            /****************************/
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
