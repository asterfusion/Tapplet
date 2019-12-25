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
#ifndef __include_addtional_tools_h__
#define __include_addtional_tools_h__

#include <vlib/vlib.h>

#include "sf_pool.h"
#include "action_path_defines.h"
#include "pkt_wdata.h"


typedef union {
    uint32_t opaque2[16];
    struct
    {
        int16_t delta;
        uint8_t pre_index;
        uint8_t forward_id; // not used
        uint8_t addtional_list[60];
    };
} sf_additional_list_t;

#define sf_prefetch_additional_list(b0) CLIB_PREFETCH( sf_wdata(b0)->recv_for_opaque , 64 , STORE)

#define get_additional_list(b0) ((sf_additional_list_t *)(sf_wdata(b0)->recv_for_opaque))

#define reset_addtional_list(addi_list_ptr) sf_memset_zero(addi_list_ptr , 64)

/// !!! Notice : this op will move pre_index to next
#define pop_addtional_list(addi_list_ptr) (addi_list_ptr->addtional_list[addi_list_ptr->pre_index++])


#define append_addtional_list(addi_list_ptr, value)                      \
    {                                                                    \
        addi_list_ptr->addtional_list[addi_list_ptr->pre_index] = value; \
        addi_list_ptr->pre_index++;                                      \
    }

#define reset_addtional_list_index(addi_list_ptr) \
    {                                           \
        addi_list_ptr->pre_index = 0;           \
    }


#endif