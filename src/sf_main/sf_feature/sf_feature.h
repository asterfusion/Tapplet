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
#ifndef __include_sf_feature_h__
#define __include_sf_feature_h__

#include <stdint.h>
#include <vlib/vlib.h>


#include "sf_thread_tools.h"
#include "sf_intf_define.h"

#include "sf_bitmap.h"

typedef uint8_t sf_feature_next_index_t;
/********************************************/
/********************************************/
/************ handoff feature ***************/
/********************************************/
/********************************************/
#if defined(RUN_ON_VM) && !defined(DEVICE_HAS_HANDOFF_FEATURE)

#define DEVICE_HAS_HANDOFF_FEATURE

#endif


typedef struct{
    // uint32_t *next_worker_array;
    // uint32_t next_worker_array_mask; // is pow2 - 1
    uint32_t dpdk_next_index_for_handoff;
    uint32_t frame_next_index;
    uint64_t *handoff_switch;
    uint64_t **handoff_pkt_cnt; //per thread ; per interfaces
}sf_intf_handoff_main_t;

extern sf_intf_handoff_main_t sf_intf_handoff_main;

static_always_inline int sf_intf_get_handoff_status_inline(uint64_t *handoff_switch , int intf_id)
{
    return handoff_switch[bitmap_get_index(intf_id)] & bitmap_get_mask(intf_id);
}

static_always_inline int sf_intf_get_handoff_status(int intf_id)
{
    return sf_intf_get_handoff_status_inline(sf_intf_handoff_main.handoff_switch , intf_id);
}

static_always_inline void sf_intf_set_handoff_status(int intf_id , int on_or_off)
{
    uint64_t mask = bitmap_get_mask(intf_id);
    if(on_or_off)
    {
        sf_intf_handoff_main.handoff_switch[bitmap_get_index(intf_id)] |= mask;
    }
    else
    {
        mask = ~mask;
        sf_intf_handoff_main.handoff_switch[bitmap_get_index(intf_id)] &= mask;
    }
}



static_always_inline uint64_t * sf_intf_get_handoff_cnt_array(uint32_t worker_index)
{
    return sf_intf_handoff_main.handoff_pkt_cnt[worker_index];
}

#endif //__include_sf_feature_h__