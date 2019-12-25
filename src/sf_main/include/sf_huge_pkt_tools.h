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
#ifndef __include_sf_huge_pkt_tools_h__
#define __include_sf_huge_pkt_tools_h__

#include <vlib/vlib.h>

#include "sf_main.h"

#include "sf_huge_pkt.h"
#include "sf_pkt_tools.h"
#include "sf_thread_tools.h"



#define HUGEPKT_REASS_FAIL_TOO_LONG -1
#define HUGEPKT_REASS_FAIL_NO_MEMORY -2

#define get_huge_pkt_16k_pool(i) \
    (&(sf_main.huge_pkt_16k_pool.per_worker_pool[i]))


static_always_inline uint8_t *
alloc_huge_pkt_16k(sf_pool_per_thread_t *local_pool)
{
    uint8_t *data_ptr;
    data_ptr = alloc_node_from_pool(local_pool);
    data_ptr += SF_HUGE_PKT_PRE_DATA_SIZE;
    return data_ptr;
}
// Notice : pointer to where it is alloced
static_always_inline void
free_huge_pkt_16k(sf_pool_per_thread_t *local_pool , uint8_t * data_ptr )
{
    data_ptr -= SF_HUGE_PKT_PRE_DATA_SIZE;
    free_node_to_pool(local_pool , data_ptr);
}




#endif