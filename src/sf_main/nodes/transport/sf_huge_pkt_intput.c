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

#include "pkt_wdata.h"
#include "sf_ports.h"

#include "sf_stat_tools.h"

#include "sf_huge_pkt_tools.h"

#include "sf_interface_tools.h"

#include "sf_string.h"

#include "sf_thread_tools.h"
#include "sf_interface_tools.h"
#include "sf_pkt_trace.h"

#include "sf_feature.h"

// #define SF_DEBUG
#include "sf_debug.h"


/********************/
typedef struct
{
    uint16_t *interface_id_or_next_index;
    uint32_t *buffer_index;
    uint32_t *drop_buffer_index;
    uint16_t *drop_pkt_cnt_per_intf;
    uint64_t buffer_cnt;
} hugepkt_input_runtime_t;

/********************/

typedef enum
{
    SF_NEXT_NODE_ETH_INPUT,
    SF_NEXT_NODE_N_COUNT,
} sf_index_t;

VLIB_REGISTER_NODE(sf_hugepkt_input_node) = {
    __sf_node_reg_basic_trace__

    .name = "sf_hugepkt_input",
    .vector_size = sizeof(u32),
    .type = VLIB_NODE_TYPE_INTERNAL,
    .runtime_data_bytes = sizeof(hugepkt_input_runtime_t),

    .n_next_nodes = SF_NEXT_NODE_N_COUNT,

    /* edit / add dispositions here */
    .next_nodes = {
        [SF_NEXT_NODE_ETH_INPUT] = "sf_eth_input",
    },
};

/*********************************/
/*********************************/
#ifndef CLIB_MARCH_VARIANT
static clib_error_t *
hugepkt_input_runtime_init(vlib_main_t *vm)
{
    hugepkt_input_runtime_t *rt;

    rt = vlib_node_get_runtime_data(vm, sf_hugepkt_input_node.index);

    vec_validate (rt->interface_id_or_next_index, VLIB_FRAME_SIZE);
    vec_validate (rt->buffer_index, VLIB_FRAME_SIZE);
    vec_validate (rt->drop_buffer_index, VLIB_FRAME_SIZE);
    vec_validate (rt->drop_pkt_cnt_per_intf, MAX_INTERFACE_CNT);

    if( !(rt->interface_id_or_next_index) 
        || !(rt->buffer_index)    
        || !(rt->drop_buffer_index)
        || !(rt->drop_pkt_cnt_per_intf)  )
    {
        clib_error("hugepkt_input_runtime_init fail");
    }

    return 0;
}

VLIB_WORKER_INIT_FUNCTION(hugepkt_input_runtime_init);
#endif
/********************************/
/********************************/
always_inline uint32_t
sf_vlib_buffer_contents(vlib_main_t *vm , 
    sf_hugepkt_helper_t *hugepkt_helper , 
    uint8_t *data_ptr, 
    uint32_t *drop_buffer_index ) 
{
    int i;
    uint32_t copyed_bytes = 0;
    sf_hugepkt_sub_t *pre_sub_buffer =  hugepkt_helper->sub_buffer;
    vlib_buffer_t *b_sub;


    // first buffer
    b_sub = vlib_get_buffer(vm , pre_sub_buffer->buffer_index);
    clib_memcpy(data_ptr , b_sub->data + pre_sub_buffer->current_data , pre_sub_buffer->current_length );
    data_ptr += pre_sub_buffer->current_length;
    copyed_bytes += pre_sub_buffer->current_length;

sf_debug("0 : len %d\n",  pre_sub_buffer->current_length);

    pre_sub_buffer ++;

    //left buffer
    for(i=1; i<hugepkt_helper->sub_buffer_cnt ; i++ , pre_sub_buffer ++)
    {
        drop_buffer_index[ i-1 ] =  pre_sub_buffer->buffer_index;

        b_sub = vlib_get_buffer(vm , pre_sub_buffer->buffer_index);

        clib_memcpy(data_ptr , b_sub->data + pre_sub_buffer->current_data , pre_sub_buffer->current_length );

sf_debug("%d : len %d\n", i , pre_sub_buffer->current_length);
        data_ptr += pre_sub_buffer->current_length;
        copyed_bytes += pre_sub_buffer->current_length;
    }

    return copyed_bytes;
}

static_always_inline
 int handle_huge_packet(vlib_main_t *vm , 
    sf_wdata_t *wdata0, 
    sf_hugepkt_helper_t *hugepkt_helper , 
    sf_pool_per_thread_t *local_pool , 
    uint32_t *drop_buffer_index , 
    uint32_t *drop_buffer_cnt)
{
    uint32_t total_len = 0;
    uint8_t *data_ptr;
    total_len = hugepkt_helper->sub_total_len_no_first + wdata0->current_length;
    int i;

    if (total_len >= SF_HUGE_PKT_MAX_DATA_SIZE)
    {
        return HUGEPKT_REASS_FAIL_TOO_LONG;
    }

    data_ptr = alloc_huge_pkt_16k( local_pool );
    if (data_ptr != NULL)
    {
        total_len = sf_vlib_buffer_contents(vm, hugepkt_helper , data_ptr , 
            drop_buffer_index + (*drop_buffer_cnt)
            );

        *drop_buffer_cnt += hugepkt_helper->sub_buffer_cnt -1;

        wdata0->current_length = total_len;
        wdata0->current_data = 0;

        wdata0->l2_hdr_offset = 0; //!!!!! Notice !!!!

        wdata0->data_ptr =  data_ptr;

        sf_debug("final :  data %d  len %d\n" , wdata0->current_data , wdata0->current_length);

        return 0;
    }

    drop_buffer_index += *drop_buffer_cnt;
    for(i=0 ; i <hugepkt_helper->sub_buffer_cnt ; i++ )
    {
        drop_buffer_index[i] = hugepkt_helper->sub_buffer[i].buffer_index;
    }

    *drop_buffer_cnt += hugepkt_helper->sub_buffer_cnt;


    return HUGEPKT_REASS_FAIL_NO_MEMORY;
}


VLIB_NODE_FN(sf_hugepkt_input_node)
(vlib_main_t *vm,
 vlib_node_runtime_t *node,
 vlib_frame_t *frame)
{
    u32 n_left_from, *from;

    from = vlib_frame_vector_args(frame);
    n_left_from = frame->n_vectors;

    sf_pool_per_thread_t *hugepkt_pool =  get_huge_pkt_16k_pool(sf_get_current_worker_index());
    hugepkt_input_runtime_t *rt = (void *) node->runtime_data;

    uint16_t *drop_pkt_cnt_per_intf = rt->drop_pkt_cnt_per_intf;

    uint16_t *interface_or_next = rt->interface_id_or_next_index;
    uint32_t *buffer_index = rt->buffer_index;
    rt->buffer_cnt = 0;

    uint32_t *drop_buffer_index = rt->drop_buffer_index;
    uint32_t drop_buffer_cnt = 0;

    uint8_t need_update_drop_flag = 0;
    memset(drop_pkt_cnt_per_intf , 0 , sizeof(drop_pkt_cnt_per_intf[0]) * MAX_INTERFACE_CNT );

    sf_debug("recv %d pkts\n" , n_left_from);

    // step 1.try reass pkt
    while (n_left_from >= 4  )
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
        }

        /* speculatively enqueue b0 and b1 to the current next frame */
        bi0 = from[0];
        bi1 = from[1];
        from += 2;
        n_left_from -= 2;

        b0 = vlib_get_buffer(vm, bi0);
        b1 = vlib_get_buffer(vm, bi1);

        /************** start handle pkt *********/
        int ret0;
        int ret1;

        sf_wdata_t *pkt_wdata0 = sf_wdata(b0);
        sf_wdata_t *pkt_wdata1 = sf_wdata(b1);

        sf_hugepkt_helper_t *hugepkt_helper0 = sf_hugepkt_helper(pkt_wdata0);
        sf_hugepkt_helper_t *hugepkt_helper1 = sf_hugepkt_helper(pkt_wdata1);

        uint16_t interface_0 = pkt_wdata0->interface_id ;
        uint16_t interface_1 = pkt_wdata1->interface_id ;

        if(PREDICT_FALSE( 
            (drop_buffer_cnt + hugepkt_helper0->sub_buffer_cnt + hugepkt_helper1->sub_buffer_cnt)
            > VLIB_FRAME_SIZE ))
        {
            sf_vlib_buffer_free_no_next(vm , drop_buffer_index , drop_buffer_cnt);
            drop_buffer_cnt = 0;
        }

        ret0 = handle_huge_packet(vm , pkt_wdata0 , hugepkt_helper0 , 
            hugepkt_pool , drop_buffer_index , &drop_buffer_cnt);
        ret1 = handle_huge_packet(vm , pkt_wdata1 , hugepkt_helper1 , 
            hugepkt_pool , drop_buffer_index , &drop_buffer_cnt );


        if(ret0 == 0 && ret1 == 0)
        {
            interface_or_next[0] = interface_0;
            interface_or_next[1] = interface_1;

            buffer_index[0] = bi0;
            buffer_index[1] = bi1;
            
            interface_or_next += 2;
            buffer_index += 2;

            rt->buffer_cnt += 2;

            continue;
        }

        if(ret0 == 0 )
        {
            interface_or_next[0] = interface_0;
            buffer_index[0] = bi0;

            interface_or_next += 1;
            buffer_index += 1;
            rt->buffer_cnt += 1;
        }
        else
        {
            drop_pkt_cnt_per_intf[pkt_wdata0->interface_id - 1] ++;
            need_update_drop_flag = 1;
        }
        
        

        if(ret1 == 0 )
        {
            interface_or_next[0] = interface_1;
            buffer_index[0] = bi1;

            interface_or_next += 1;
            buffer_index += 1;
            rt->buffer_cnt += 1;
        }
        else
        {
            drop_pkt_cnt_per_intf[pkt_wdata1->interface_id - 1] ++;
            need_update_drop_flag = 1;
        }

    }

    while (n_left_from > 0  )
    {
        u32 bi0;
        vlib_buffer_t *b0;


        /* speculatively enqueue b0 and b1 to the current next frame */
        bi0 = from[0];
        from += 1;
        n_left_from -= 1;

        b0 = vlib_get_buffer(vm, bi0);

        /************** start handle pkt *********/
        int ret0;

        sf_wdata_t *pkt_wdata0 = sf_wdata(b0);

        sf_hugepkt_helper_t *hugepkt_helper0 = sf_hugepkt_helper(pkt_wdata0);

        uint16_t interface_0 = pkt_wdata0->interface_id ;

        if(PREDICT_FALSE( 
            (drop_buffer_cnt + hugepkt_helper0->sub_buffer_cnt)
            > VLIB_FRAME_SIZE ))
        {
            sf_vlib_buffer_free_no_next(vm , drop_buffer_index , drop_buffer_cnt);
            drop_buffer_cnt = 0;
        }

        ret0 = handle_huge_packet(vm , pkt_wdata0 , hugepkt_helper0 , 
            hugepkt_pool , drop_buffer_index , &drop_buffer_cnt);

        if(ret0 == 0 )
        {
            interface_or_next[0] = interface_0;
            buffer_index[0] = bi0;

            interface_or_next += 1;
            buffer_index += 1;
            rt->buffer_cnt += 1;
        }
        else
        {
            drop_pkt_cnt_per_intf[pkt_wdata0->interface_id - 1] ++;
            need_update_drop_flag = 1;
        }
    }

    //step 2 : free not need && record drop cnt
    if(drop_buffer_cnt > 0)
    {
        sf_vlib_buffer_free_no_next(vm , drop_buffer_index , drop_buffer_cnt);
    }
    sf_debug("free %d sub buffer\n" , drop_buffer_cnt);

    if(need_update_drop_flag)
    {
        int i;
        uint32_t thread_index = sf_get_current_thread_index();
        for(i= 0 ; i<MAX_INTERFACE_CNT ; i++)
        {
            if(drop_pkt_cnt_per_intf[i] != 0)
            {
                update_interface_stat_multi( thread_index, i+1 , drop_packets  , drop_pkt_cnt_per_intf[i]);
            }
        }
    }

    // step 2. determine next  && push to next
    interface_or_next = rt->interface_id_or_next_index;
    buffer_index = rt->buffer_index;
    n_left_from = rt->buffer_cnt;

    while(n_left_from > 0)
    {

        interface_or_next[0] = SF_NEXT_NODE_ETH_INPUT;

        interface_or_next++;
        n_left_from--;
    }

    vlib_buffer_enqueue_to_next(vm , node , 
        rt->buffer_index , rt->interface_id_or_next_index , rt->buffer_cnt);



    return frame->n_vectors;
}

