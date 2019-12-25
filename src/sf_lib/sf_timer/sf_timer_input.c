
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

#include "sf_timer_input.h"
#include "sf_timer.h"

#include "sf_timer_enqueue.h"

#include "sf_main.h"

#include "sf_thread_tools.h"

#include "sf_tools.h"


#include "sf_debug.h"

typedef enum
{
    SF_NEXT_NODE_TIMER_ERROR_DROP,
    SF_NEXT_NODE_N_COUNT,
} sf_index_t;

typedef struct
{
    sf_tw_timer_t *tw_timer;
    uint64_t expired_timer_cnt;
} timer_input_runtime_t;

/* *INDENT-OFF* */
VLIB_REGISTER_NODE(sf_timer_input_node) = {
    .name = "sf_timer_input",
    .type = VLIB_NODE_TYPE_INPUT,
    .state = VLIB_NODE_STATE_DISABLED,
    .runtime_data_bytes = sizeof(timer_input_runtime_t),

    .n_next_nodes = SF_NEXT_NODE_N_COUNT,

    .next_nodes = {
        [SF_NEXT_NODE_TIMER_ERROR_DROP] = "sf_timer_error_drop",
    },
};

/*********************************/
#ifndef CLIB_MARCH_VARIANT
static clib_error_t *
sf_timer_local_init(vlib_main_t *vm)
{
    timer_input_runtime_t *rt = vlib_node_get_runtime_data(vm, sf_timer_input_node.index);

    sf_timer_main_t *sf_timer_main_ptr = &sf_timer_main;
    uint32_t worker_index = sf_get_current_worker_index();
    sf_tw_timer_t *tw_timer;

    sf_pool_per_thread_t *timer_info_pool;
    sf_pool_per_thread_t *chunk_pool;

    vlib_node_set_state(vm, sf_timer_input_node.index, VLIB_NODE_STATE_POLLING);
    
    tw_timer = sf_timer_main_ptr->tw_timer_array + worker_index;

    timer_info_pool = get_local_pool_ptr(&(sf_timer_main_ptr->timer_info_pool));
    chunk_pool = get_local_pool_ptr(&(sf_timer_main_ptr->timer_chunk_pool));

    sf_tw_timer_init(vm, tw_timer, timer_info_pool, chunk_pool);

    rt->tw_timer = tw_timer;
    rt->expired_timer_cnt = 0;
    return 0;
}

VLIB_WORKER_INIT_FUNCTION(sf_timer_local_init);


uint64_t get_sf_timer_input_expire_cnt(uint32_t worker_index)
{
    vlib_main_t *vm;

    if( worker_index < sf_vlib_num_workers()  &&  vlib_mains[worker_index + 1] != NULL)
    { 
        vm = vlib_mains[worker_index + 1];

        timer_input_runtime_t *rt = vlib_node_get_runtime_data( vm , sf_timer_input_node.index);

        return rt->expired_timer_cnt;
    }
    return 0;
}

#endif
/********************************/

static_always_inline uint32_t
sf_timer_input_internal(vlib_main_t *vm,
                        vlib_node_runtime_t *node,
                        sf_tw_timer_t *tw_timer,
                        sf_timer_chunk_t *timer_chunk)
{
    uint32_t *next_index_vec = sf_timer_main.next_index_vec;
    uint64_t timer_info_start = tw_timer->timer_info_start;

    u32 n_left_from;
    uint64_t *from;
    uint64_t *to_next;
    u32 next_index;

    from = timer_chunk->sf_timer_slots;
    n_left_from = timer_chunk->current_slot_cnt;
    next_index = SF_NEXT_NODE_TIMER_ERROR_DROP;

    uint32_t not_zero_cnt = 0;

    while (n_left_from > 0)
    {
        u32 n_left_to_next;

        vlib_get_next_frame(vm, node, next_index,
                            to_next, n_left_to_next);

        while (n_left_from >= 2 && n_left_to_next >= 2)
        {
            u32 next0 = SF_NEXT_NODE_TIMER_ERROR_DROP;
            u32 next1 = SF_NEXT_NODE_TIMER_ERROR_DROP;

            sf_timer_handle_t bi0;
            sf_timer_handle_t bi1;

            u32 timer_grp_id0;
            u32 timer_grp_id1;

            /* speculatively enqueue b0 and b1 to the current next frame */
            bi0.data = from[0];
            bi1.data = from[1];

            uint64_t ptr0 = bi0.timer_info_index + timer_info_start;
            uint64_t ptr1 = bi1.timer_info_index + timer_info_start;
            
            to_next[0] = ptr0;
            to_next[1] = ptr1;

            from += 2;
            to_next += 2;
            n_left_from -= 2;
            n_left_to_next -= 2;

            timer_grp_id0 = bi0.group_id;
            timer_grp_id1 = bi1.group_id;

            if ( timer_grp_id0 != 0)
            {
                next0 =  next_index_vec[timer_grp_id0];
                not_zero_cnt++;
            }
            if ( timer_grp_id1 != 0 )
            {
                next1 =  next_index_vec[timer_grp_id1];
                not_zero_cnt++;
            }

            /* verify speculative enqueues, maybe switch current next frame */
            sf_u64_validate_buffer_enqueue_x2(vm, node, next_index,
                                            to_next, n_left_to_next,
                                            ptr0, ptr1, next0, next1);
        }

        while (n_left_from > 0 && n_left_to_next > 0)
        {
            u32 next0 = SF_NEXT_NODE_TIMER_ERROR_DROP;
            sf_timer_handle_t bi0;
            u32 timer_grp_id0;

            /* speculatively enqueue b0 and b1 to the current next frame */
            bi0.data = from[0];
            
            uint64_t ptr0 = bi0.timer_info_index + timer_info_start;

            to_next[0] = ptr0;

            from += 1;
            to_next += 1;
            n_left_from -= 1;
            n_left_to_next -= 1;

            timer_grp_id0 = bi0.group_id;

            if ( timer_grp_id0 != 0)
            {
                next0 =  next_index_vec[timer_grp_id0];
                not_zero_cnt++;
            }
 
            /* verify speculative enqueue, maybe switch current next frame */
            vlib_validate_buffer_enqueue_x1(vm, node, next_index,
                                            to_next, n_left_to_next,
                                            ptr0, next0);
        }

        vlib_put_next_frame(vm, node, next_index, n_left_to_next);
    }

    return not_zero_cnt;
}

VLIB_NODE_FN(sf_timer_input_node)
(vlib_main_t *vm,
 vlib_node_runtime_t *node,
 vlib_frame_t *frame)
{
    timer_input_runtime_t *rt = (void *)node->runtime_data;
    sf_tw_timer_t *tw_timer = rt->tw_timer;
    sf_timer_chunk_t *current_chunk;


    f64 time_now;

    int ret_cnt ;


    if(tw_timer->expire_chunk_list == NULL)
    {
        time_now = vlib_time_now(vm);
        ret_cnt = sf_tw_timer_expire_internal(tw_timer , time_now);

        // if(ret_cnt == 0 )
        // {
        //     return 0;
        // }
    }

    ret_cnt = 0;
    while(tw_timer->expire_chunk_list != NULL)
    {
        current_chunk = tw_timer->expire_chunk_list;
        tw_timer->expire_chunk_list = current_chunk->next_chunk;


        ret_cnt +=  sf_timer_input_internal(vm, node , tw_timer ,  current_chunk);
        
        free_node_to_pool(tw_timer->chunk_pool , current_chunk);
        if(ret_cnt >= SF_TW_TIMER_SLOT_CNT)
        {
            break;
        }
    }
    
    rt->expired_timer_cnt += ret_cnt;

    return 0;
}

/* *INDENT-ON* */

VLIB_NODE_FN(sf_timer_error_node)
(vlib_main_t *vm,
 vlib_node_runtime_t *node,
 vlib_frame_t *frame)
{
    return frame->n_vectors;
}

/* *INDENT-OFF* */
VLIB_REGISTER_NODE(sf_timer_error_node) = {
    .name = "sf_timer_error_drop",
    .vector_size = sizeof(uint64_t),
    .type = VLIB_NODE_TYPE_INTERNAL,
    .flags = VLIB_NODE_FLAG_IS_DROP,
    .n_next_nodes = 0,
};
/* *INDENT-ON* */
