
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
#include <vnet/vnet.h>
#include <vnet/plugin/plugin.h>
#include <vlib/cli.h>
#include <vlib/threads.h>
#include <vppinfra/os.h>


#include "sf_feature.h"
#include "sf_interface_shm.h"
#include "sf_thread_tools.h"
#include "sf_string.h"


sf_intf_handoff_main_t sf_intf_handoff_main;


clib_error_t *sf_intf_handoff_main_init(vlib_main_t *vm)
{
    sf_intf_handoff_main_t *sm = &sf_intf_handoff_main;
    clib_error_t *error = 0;

    sf_memset_zero(sm , sizeof(sf_intf_handoff_main_t));

    uint32_t worker_num = sf_vlib_num_workers();
    uint32_t intf_cnt = MAX_INTERFACE_CNT;

    uint32_t i;

#define CHECK_and_CLEAR_MEMORY(ptr , size) if(!(ptr)) \
{ clib_error("sf_intf_handoff_main_init alloc memory failed."); } \
else  \
{ sf_memset_zero((ptr) , (size)); }

    sm->handoff_switch = clib_mem_alloc_aligned_at_offset( 
        sizeof(uint64_t) * (intf_cnt / 64 + 1) , 
        0 , 0 , 0 );
    
    sm->handoff_pkt_cnt = clib_mem_alloc_aligned_at_offset( 
        sizeof(uint64_t *) * (worker_num ) , 
        0 , 0 , 0 );

    CHECK_and_CLEAR_MEMORY(sm->handoff_switch , sizeof(uint64_t) * (intf_cnt / 64 + 1));
    CHECK_and_CLEAR_MEMORY(sm->handoff_pkt_cnt , sizeof(uint64_t *) * (worker_num ));


    for(i=0; i<worker_num ; i++)
    {
        sm->handoff_pkt_cnt[i] = clib_mem_alloc_aligned_at_offset( 
        sizeof(uint64_t) * (intf_cnt ) , 
        CLIB_CACHE_LINE_BYTES , 0 , 0 );

        CHECK_and_CLEAR_MEMORY(sm->handoff_pkt_cnt[i] , sizeof(uint64_t) * (intf_cnt ) );

    }

#ifdef RUN_ON_VM
    if(worker_num > 1)
    {
        //turn on all interface handoff
        for(i=0 ; i<intf_cnt  ; i++)
        {
            sf_intf_set_handoff_status(i+1 , 1);
        }
    }
#endif

    return error;
}

VLIB_INIT_FUNCTION(sf_intf_handoff_main_init);


#ifdef DEVICE_HAS_HANDOFF_FEATURE

clib_error_t *sf_intf_handoff_nodes_init(vlib_main_t *vm)
{
    sf_intf_handoff_main_t *sm = &sf_intf_handoff_main;

    vlib_node_t *dpdk_input;
    vlib_node_t *handoff_node;

    uint32_t next_index;
    
    clib_error_t *error;
    error = vlib_call_init_function(vm, sf_intf_handoff_main_init); 
    if (error)                                        
    {                                                 
        clib_error_report(error);                     
        return error;                                 
    }

    //  reg first node to dpdk-input
    dpdk_input = vlib_get_node_by_name(vm, (u8 *)"sf-dpdk-input");

    if(dpdk_input == NULL)
    {
        clib_error("no node named sf-dpdk-input");
    }

    {
        vlib_node_t *n = vlib_get_node_by_name (vm, (u8 *)"sf_pkt_hf_input");
        if (n) {
            sm->frame_next_index = vlib_frame_queue_main_init (n->index, 0);
        }
        else {
            clib_error("sf_pkt_hf_input not found\n");
        }

        handoff_node = vlib_get_node_by_name (vm, (u8 *)"sf_pkt_handoff");
    }
    

    if(handoff_node == NULL)
    {
        clib_error("sf_pkt_handoff not found\n");
    }

    next_index =
        vlib_node_add_next(vm, dpdk_input->index, handoff_node->index);

    sm->dpdk_next_index_for_handoff = next_index;

    return 0;
}

VLIB_INIT_FUNCTION(sf_intf_handoff_nodes_init);



#endif //#ifdef DEVICE_HAS_HANDOFF_FEATURE
