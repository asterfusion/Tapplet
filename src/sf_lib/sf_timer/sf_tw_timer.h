
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
#ifndef __included_sf_tw_timer_h__
#define __included_sf_tw_timer_h__

#include <vlib/vlib.h>

#include "sf_pool.h"

#include "sf_debug.h"

typedef uint64_t sf_timer_slot_t;

typedef union sf_timer_handle{
    struct{
        uint64_t group_id : 8;
        uint64_t timer_info_index : 56;
    };
    uint64_t data;
}sf_timer_handle_t;

#define SF_TIMER_DATA_LEN 64

typedef struct{
    u8 timer_data[SF_TIMER_DATA_LEN];
}sf_timer_info_t;

typedef struct{
    sf_timer_slot_t *slot_handler;
    sf_timer_info_t *timer_info;
    f64 safe_clib_time;
}sf_timer_record_t;


#define SF_TW_TIMER_BUCKET_MASK (SF_TIMER_MAX_TICKS - 1 )
#define SF_TW_TIMER_BUCKET_CNT SF_TIMER_MAX_TICKS


#define SF_TW_TIMER_CHUNK_128B_MULTI 32

// 8: next ptr or this chunk cnt  ; 1 : current_slot_cnt
#define SF_TW_TIMER_SLOT_CNT ( (SF_TW_TIMER_CHUNK_128B_MULTI * 128 - 8 ) / sizeof(sf_timer_slot_t)  - 1)


typedef struct sf_timer_chunk{
    struct sf_timer_chunk *next_chunk;
    sf_timer_slot_t current_slot_cnt;
    sf_timer_slot_t sf_timer_slots[SF_TW_TIMER_SLOT_CNT];
} sf_timer_chunk_t;


typedef struct{
    uint32_t slot_used_cnt;
    uint32_t slot_left_cnt;
    sf_timer_chunk_t *first_chunk;
    sf_timer_chunk_t *current_chunk;
    uint64_t pad;
}sf_timer_bucket_t;



typedef struct{    
    //record 
    vlib_main_t *vm;
    uint64_t  timer_info_start;
    sf_pool_per_thread_t *timer_info_pool;
    sf_pool_per_thread_t *chunk_pool;
    sf_timer_bucket_t *sf_timer_buckets;

    //runtime 
    sf_timer_chunk_t *expire_chunk_list;
    uint32_t pre_bucket_index;
    uint32_t pad;
    
    // record
    f64 ticks_per_second;
    f64 timer_interval;

    //runtime 
    f64 last_run_time;
    f64 next_run_time;
}sf_tw_timer_t;

int  sf_tw_timer_init(vlib_main_t *vm ,  sf_tw_timer_t *tw , 
    sf_pool_per_thread_t *timer_info_pool ,  sf_pool_per_thread_t *chunk_pool );

static_always_inline 
int 
sf_tw_timer_expire_internal(sf_tw_timer_t *tw, f64 now)
{
    uint32_t nticks, i;
    int ret_cnt = 0;
    sf_timer_chunk_t *first_expire_chunk = NULL;
    sf_timer_chunk_t *last_expire_chunk = NULL;
    sf_timer_bucket_t *current_bucket;

    /* Shouldn't happen */
    if (PREDICT_FALSE(now < tw->next_run_time))
        return 0;

    /* Shouldn't happen */
    if (PREDICT_FALSE(tw->expire_chunk_list != NULL))
        return 0;

    /* Number of ticks which have occurred */
    nticks = tw->ticks_per_second * (now - tw->last_run_time);

    /* Remember when we ran, compute next runtime */
    tw->next_run_time = (now + tw->timer_interval);


    for(i=0 ; i<nticks ; i++)
    {
        tw->pre_bucket_index ++;
        tw->pre_bucket_index &= SF_TW_TIMER_BUCKET_MASK;

        current_bucket = tw->sf_timer_buckets + tw->pre_bucket_index;

        if(current_bucket->slot_used_cnt == 0)
        {
            continue;
        } 

        ret_cnt += current_bucket->slot_used_cnt;
                
        //append to expire chunk list
        if(first_expire_chunk == NULL)
        {
            first_expire_chunk = current_bucket->first_chunk;
            last_expire_chunk = current_bucket->current_chunk;
        }
        else
        {
            last_expire_chunk->next_chunk = current_bucket->first_chunk;
            last_expire_chunk = current_bucket->current_chunk;
        }

        //reset current bucket
        sf_memset_zero( current_bucket , sizeof(sf_timer_bucket_t) );

    }

    tw->expire_chunk_list = first_expire_chunk;

    
    tw->last_run_time += (nticks ) * tw->timer_interval;

    return ret_cnt;
}



#endif /* __included_tw_timer_2t_1w_2048sl_h__ */
