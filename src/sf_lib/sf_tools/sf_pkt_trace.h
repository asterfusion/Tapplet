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
#ifndef __include_sf_pkt_trace_h__
#define __include_sf_pkt_trace_h__

#include <stdint.h>
#include <vlib/vlib.h>
#include <vnet/vnet.h>

#include "pkt_wdata.h"

#ifndef SF_PKT_TRACE_OFF
    #define SF_PKT_TRACE_ON
#endif


#ifdef SF_PKT_TRACE_ON


//basic trace struct
typedef struct
{
    uint32_t next_index;
    uint32_t interface_in;
} sf_pkt_trace_t;

uint8_t * sf_format_basic_trace(uint8_t *s, va_list *args);


#define __sf_node_reg_basic_trace__ \
    .format_trace = sf_format_basic_trace,

#define sf_add_basic_trace_x1(vm_i, node_i, b0_i, pkt_wdata_i, next_i)                                                   \
    if (PREDICT_FALSE((node_i->flags & VLIB_NODE_FLAG_TRACE) && (pkt_wdata_i->unused8 & UNUSED8_FLAG_PKT_TRACED))) \
    {                                                                                                              \
        sf_pkt_trace_t *t = vlib_add_trace(vm_i, node_i, b0_i, sizeof(*t));                                        \
        t->interface_in = pkt_wdata_i->interface_id;                                                               \
        t->next_index = next_i;                                                                                    \
    }

#define sf_add_basic_trace_x2(vm_i, node_i, b0_i, b0_j, pkt_wdata_i, pkt_wdata_j, next_i, next_j) \
    if (PREDICT_FALSE((node_i->flags & VLIB_NODE_FLAG_TRACE)))                              \
    {                                                                                       \
        if (pkt_wdata_i->unused8 & UNUSED8_FLAG_PKT_TRACED)                                 \
        {                                                                                   \
            sf_pkt_trace_t *t = vlib_add_trace(vm_i, node_i, b0_i, sizeof(*t));             \
            t->interface_in = pkt_wdata_i->interface_id;                                    \
            t->next_index = next_i;                                                         \
        }                                                                                   \
        if (pkt_wdata_j->unused8 & UNUSED8_FLAG_PKT_TRACED)                                 \
        {                                                                                   \
            sf_pkt_trace_t *t = vlib_add_trace(vm_i, node_i, b0_j, sizeof(*t));             \
            t->interface_in = pkt_wdata_j->interface_id;                                    \
            t->next_index = next_j;                                                         \
        }                                                                                   \
    }

#else // SF_PKT_TRACE_OFF

#define __sf_node_reg_basic_trace__ 
#define sf_add_basic_trace_x1(vm_i, node_i, b0_i, pkt_wdata_i, next_i)   ;
#define sf_add_basic_trace_x2(vm_i, node_i, b0_i, b0_j, pkt_wdata_i, pkt_wdata_j, next_i, next_j)  ;
    
#endif // SF_PKT_TRACE_OFF




#endif // __include_sf_pkt_trace_h__