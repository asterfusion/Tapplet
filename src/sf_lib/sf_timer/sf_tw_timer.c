

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
#include <vppinfra/error.h>
#include "sf_timer.h"
#include "sf_tw_timer.h"

int 
sf_tw_timer_init(vlib_main_t *vm ,  sf_tw_timer_t *tw , 
    sf_pool_per_thread_t *timer_info_pool , 
    sf_pool_per_thread_t *chunk_pool 
)
{
    sf_memset_zero(tw , sizeof(sf_tw_timer_t));

    tw->sf_timer_buckets = vec_new_ha(sf_timer_bucket_t , SF_TW_TIMER_BUCKET_CNT , 0 , sizeof(sf_timer_bucket_t));

    if(  !(tw->sf_timer_buckets)  )
    {
        clib_error("sf_tw_timer_init fail, memory not enough");
        return -1;
    }

    tw->vm = vm;
    tw->timer_info_start = (uint64_t)(sf_tw_timer_init); // use function address as the start address
    tw->timer_info_pool = timer_info_pool;
    tw->chunk_pool = chunk_pool;

    //time init
    tw->ticks_per_second = 1.0 / SF_TIMER_SECOND_PER_TICK;
    tw->timer_interval = SF_TIMER_SECOND_PER_TICK;


    return 0;
}