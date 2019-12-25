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
#ifndef __include_sf_timer_h__
#define __include_sf_timer_h__

#include <vlib/vlib.h>
#include <vppinfra/smp.h>

#include "sf_string.h"

#define SF_TIMER_SECOND_PER_TICK 0.1  // 64bit float 
#define SF_TIMER_MAX_TICKS       (1 << 14) // 27min


#include "sf_tw_timer.h"
#include "sf_timer_input.h"


/*******
 *  for sf ; the pool will expand if more mem is needed , but will not free it by itself
 *  pool mem  = pool size * sizeof( TWT (tw_timer) ) 
 *  sizeof( TWT (tw_timer) )  = 12 byte
 */

#define SF_TIMER_INFO_POOL_INIT_SIZE 400000
#define SF_TIMER_INFO_POOL_INC_SIZE 32

#define SF_TIMER_CHUNK_POOL_INIT_SIZE ( (SF_TIMER_INFO_POOL_INIT_SIZE / SF_TW_TIMER_SLOT_CNT) + 2 )
#define SF_TIMER_CHUNK_POOL_INC_SIZE (2)

typedef enum{
    SF_TW_TIMER_GROUP_0, // use to identify stopped timer
    SF_TW_TIMER_GROUP_TEST,
    SF_TW_TIMER_GROUP_FLOW_TABLE,
    SF_TW_TIMER_GROUP_NETFLOW,
    SF_TW_TIMER_GROUP_IP_REASS,
    SF_TW_TIMER_GROUP_DEDUPLICATION,
    SF_TW_TIMER_GROUP_TCP_REASS,
    SF_TW_TIMER_GROUP_SSL_SESSION,


    SF_TW_TIMER_GROUP_CNT,
    SF_TW_TIMER_GROUP_MAX = 255,
}sf_tw_timer_group_record;



typedef struct sf_timer_main{
    sf_tw_timer_t *tw_timer_array;
    uint32_t *next_index_vec; // next index (not node index) of each timer_group id
    sf_pool_head_t timer_info_pool;
    sf_pool_head_t timer_chunk_pool;
}sf_timer_main_t;

extern sf_timer_main_t sf_timer_main;


/***
 * first, add GROUP index in sf_tw_timer_group_record;
 */
void sf_timer_input_regist_next_node(vlib_main_t *vm , u32 timer_group_id , u32 node_index );


#include "sf_thread_tools.h"
#define sf_local_worker_index sf_get_current_worker_index()
static_always_inline sf_tw_timer_t *get_local_tw_timer()
{
    return sf_timer_main.tw_timer_array + sf_get_current_worker_index();
}

static_always_inline sf_timer_info_t * alloc_timer_info()
{
    sf_tw_timer_t *tw_timer = get_local_tw_timer();
    return ( sf_timer_info_t *)alloc_node_from_pool(tw_timer->timer_info_pool);
}



static_always_inline void free_timer_info(void *sf_timer_info)
{
    sf_tw_timer_t *tw_timer = get_local_tw_timer();
    free_node_to_pool(tw_timer->timer_info_pool , sf_timer_info);
}



static_always_inline void *get_timer_info_by_index(uint64_t index)
{
    sf_tw_timer_t *tw_timer = get_local_tw_timer();
    return (void *)(tw_timer->timer_info_start + index );
}

#define alloc_timer_info_tw(tw_timer) (( sf_timer_info_t *)alloc_node_from_pool((tw_timer)->timer_info_pool))
#define free_timer_info_tw(tw_timer , timer_info ) free_node_to_pool((tw_timer)->timer_info_pool , timer_info)
#define get_timer_info_by_index_tw(tw_timer , index ) ((void *)((tw_timer)->timer_info_start + (index) ))


#define GET_PNODE_TIMER_RECORD_PTR(type,var)  (&(((type *)var)->timer_record))

static_always_inline 
int 
sf_submit_timer_tw(sf_tw_timer_t *tw , sf_timer_record_t *timer_record , void *sf_timer_info  ,
            uint8_t group_id , uint32_t ticks )
{
    SF_ASSERT(tw != NULL);
    SF_ASSERT(sf_timer_info != NULL);
    SF_ASSERT(group_id != NULL);

    sf_timer_bucket_t *target_bucket;
    sf_timer_chunk_t *current_chunk;

    uint32_t timer_bucket_index;
    sf_timer_handle_t timer_info_handler;
    timer_info_handler.timer_info_index  = (uint64_t)(sf_timer_info) - tw->timer_info_start;
    timer_info_handler.group_id = group_id;


    if(PREDICT_FALSE(ticks > SF_TIMER_MAX_TICKS))
    {
        return -1;
    }

    timer_bucket_index = tw->pre_bucket_index + ticks;
    timer_bucket_index &= SF_TW_TIMER_BUCKET_MASK;


    target_bucket = tw->sf_timer_buckets + timer_bucket_index;
    current_chunk = target_bucket->current_chunk;

    if(target_bucket->slot_left_cnt > 0 )
    {
        current_chunk->sf_timer_slots[current_chunk->current_slot_cnt] = timer_info_handler.data;

        current_chunk->current_slot_cnt ++;
        target_bucket->slot_left_cnt --;
        target_bucket->slot_used_cnt ++;
    }
    else
    {
        current_chunk = (sf_timer_chunk_t *)alloc_node_from_pool(tw->chunk_pool);

        if(current_chunk == NULL)
        {
            return -1;
        }

        current_chunk->next_chunk = NULL;
        current_chunk->sf_timer_slots[0] = timer_info_handler.data;
        current_chunk->current_slot_cnt = 1;

        if(target_bucket->current_chunk == NULL)
        {
            target_bucket->first_chunk = current_chunk;
            target_bucket->current_chunk = current_chunk;
        }
        else
        {
            target_bucket->current_chunk->next_chunk = current_chunk;
            target_bucket->current_chunk = current_chunk;
        }
        
        // target_bucket->slot_left_cnt += SF_TW_TIMER_SLOT_CNT;
        // target_bucket->slot_left_cnt --;
        target_bucket->slot_left_cnt += SF_TW_TIMER_SLOT_CNT - 1;
        target_bucket->slot_used_cnt ++;
    }

    if(timer_record)
    {
        timer_record->safe_clib_time =  tw->last_run_time + (ticks - 2) * tw->timer_interval;
        timer_record->slot_handler = current_chunk->sf_timer_slots + current_chunk->current_slot_cnt - 1;
        timer_record->timer_info = (sf_timer_info_t *)sf_timer_info;
    }

    return 0;
}



static_always_inline 
int 
sf_submit_timer(sf_timer_record_t *timer_record , void *sf_timer_info  ,
            uint8_t group_id , uint32_t ticks )
{
    SF_ASSERT(tw != NULL);
    SF_ASSERT(sf_timer_info != NULL);
    SF_ASSERT(group_id != NULL);
    sf_tw_timer_t *tw = get_local_tw_timer();

    return sf_submit_timer_tw(tw , timer_record , sf_timer_info , group_id , ticks);
}


/***
 *  Notice : judge if the timer is busy first !!!!
 *  Stop a timer 
 */
#define SF_TIMER_IS_BUSY 1
#define SF_TIMER_IS_SAFE_TO_STOP 0

#define SF_TIMER_STATUS_BUSY SF_TIMER_IS_BUSY
#define SF_TIMER_STATUS_SUCCESS SF_TIMER_IS_SAFE_TO_STOP

static_always_inline 
int
sf_stop_timer(sf_timer_record_t *timer_record  )
{
    sf_tw_timer_t *tw = get_local_tw_timer();
    f64 time_now = vlib_time_now(tw->vm);

    if(time_now < timer_record->safe_clib_time)
    {
        *(timer_record->slot_handler) = 0;

        return SF_TIMER_STATUS_SUCCESS;
    }
    else
    {
        return SF_TIMER_STATUS_BUSY;
    }
}

static_always_inline 
void
sf_stop_timer_void_tw(sf_tw_timer_t *tw , sf_timer_record_t *timer_record  )
{
    f64 time_now = vlib_time_now(tw->vm);

    if(time_now < timer_record->safe_clib_time)
    {
        *(timer_record->slot_handler) = 0;
        free_node_to_pool(tw->timer_info_pool , timer_record->timer_info);
    }
    else
    {
        // sf_memset_zero(timer_record->timer_info  , memset_size);
        sf_memset_zero(timer_record->timer_info  , sizeof(sf_timer_info_t));
    }
}

static_always_inline 
void
sf_stop_timer_void(sf_timer_record_t *timer_record  )
{
    sf_tw_timer_t *tw = get_local_tw_timer();
    sf_stop_timer_void_tw(tw , timer_record);
}

#endif
