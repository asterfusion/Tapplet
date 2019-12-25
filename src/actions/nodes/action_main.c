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

#include "sf_main.h"
#include "sf_string.h"
#include "pkt_wdata.h"

#include "action.h"
#include "action_shm.h"
#include "action_tools.h"

#include "sf_tools.h"
#include "sf_stat_tools.h"

#include "sf_pkt_tools.h"


#include "sf_interface_tools.h"

#include "action_path_tools.h"


#include "sf_pkt_trace.h"

// typedef enum{
//     SF_NEXT_NODE_ACTION_ADDITIONAL,

//     SF_NEXT_NODE_SF_PKT_DROP,
//     SF_NEXT_NODE_N_COUNT,
// } sf_next_index_t;


VLIB_REGISTER_NODE(action_main_node) = {
    __sf_node_reg_basic_trace__


    .name = "sf_action_main",
    .vector_size = sizeof(u32),
    //.format_trace = format_sf_trace,
    .type = VLIB_NODE_TYPE_INTERNAL,

    .n_next_nodes = SF_ACTION_NEXT_NODE_N_COUNT,

    /* edit / add dispositions here */
    .next_nodes = {
#define FUNC(a, b) [a] = b,
    foreach_additional_next_node
#undef FUNC

#define FUNC(a, b, c) [b] = c,
    foreach_forward_next_node
#undef FUNC         
    },
};




// uint32_t action_type_to_next_index_start_global;
// static clib_error_t *action_type_to_next_index_init(vlib_main_t *vm)
// {
//     action_type_to_next_index_start_global = 
    
//     return 0;
// }
// VLIB_INIT_FUNCTION(action_type_to_next_index_init);

#define action_type_to_next_index(type) ((type) + ADDITIONAL_TYPE_COUNT - 1)


static_always_inline uint32_t get_action_forward_path (
        sf_wdata_t *wdata , sf_additional_list_t *addi_list_ptr)
{

    uint32_t forward_next_index = SF_NEXT_NODE_SF_PKT_DROP;
    additional_action_switch_t addi_switch;
    sf_action_t *action = get_action(wdata->arg_to_next);
    wdata->arg1_to_next = wdata->arg_to_next & 0xFFFF;
    wdata->arg2_to_next = 0;
    
    if(PREDICT_FALSE(!action))
        return SF_NEXT_NODE_SF_PKT_DROP;

    // if action type == drop , we  won't do any addtional action to it
    if(action->type <= ACTION_TYPE_DROP || action->type >= ACTION_TYPE_COUNT)
    {
        return SF_NEXT_NODE_SF_PKT_DROP;
    }

    forward_next_index = action_type_to_next_index(action->type);
    // set_tx_intf_index(b0, action->forward_interface_id);
    set_tx_intf_index_wdata(wdata , action->forward_interface_id);

    // /*Save the action_id for loadbalance*/
    // sf_opaque->arg3_to_next = sf_opaque->arg1_to_next;
    // /*Save the pointer to the action for the next node to use*/
    // sf_opaque->arg_ptr_to_next = action;

    addi_switch.switch_flags =   action->additional_actions.additional_switch.switch_flags;
    if (!addi_switch.switch_flags)
    {
        return forward_next_index;
    }
    else
    {
        // sf_additional_list_t *addi_list_ptr = get_additional_list(b0);

        reset_addtional_list(addi_list_ptr);

        addi_list_ptr->delta = -sf_wqe_get_cur_offset_wdata(wdata);
        

        if(addi_switch.flag_erspan_encapsulation)
        {
            append_addtional_list(addi_list_ptr,SF_NEXT_NODE_ACTION_ERSPAN_ENCAPSULATION);
        }
        if (addi_switch.flag_vxlan_encapsulation)
        {
            append_addtional_list(addi_list_ptr, SF_NEXT_NODE_ACTION_VXLAN_ENCAPSULATION);
        }
        if (addi_switch.flag_gre_encapsulation)
        {
            append_addtional_list(addi_list_ptr, SF_NEXT_NODE_ACTION_GRE_ENCAPSULATION);
        }

        append_addtional_list(addi_list_ptr, forward_next_index);
        reset_addtional_list_index(addi_list_ptr);

        return pop_addtional_list(addi_list_ptr);
    }
    

    return SF_NEXT_NODE_SF_PKT_DROP;
}





VLIB_NODE_FN(action_main_node)
(
    vlib_main_t *vm,
    vlib_node_runtime_t *node,
    vlib_frame_t *frame)
{
    u32 n_left_from, n_left_to_next;
    u32 *from, *to_next;
    u32 next_index;

    from  =  vlib_frame_vector_args(frame);

    n_left_from = frame->n_vectors;

    next_index = node->cached_next_index;

    if(PREDICT_FALSE( !action_config ))
    {
        clib_error("action_config is NULL");
    }

    while(n_left_from > 0)
    {
        vlib_get_next_frame(vm, node, next_index, to_next, n_left_to_next);

        while (n_left_from >= 4 && n_left_to_next >= 2)
        {

            u32 bi0;
            u32 bi1;
            vlib_buffer_t *b0;
            vlib_buffer_t *b1;


            /* Prefetch next iteration. */
            {
                vlib_buffer_t *p2, *p3;

                p2 = vlib_get_buffer(vm, from[2]);
                p3 = vlib_get_buffer(vm, from[3]);

                sf_wdata_prefetch_cache(p2);
                sf_wdata_prefetch_cache(p3);

                sf_prefetch_additional_list(p2);
                sf_prefetch_additional_list(p3);
            }

            /* speculatively enqueue b0 to the current next frame */
            to_next[0] = bi0 = from[0];
            to_next[1] = bi1 = from[1];
            from += 2;
            to_next += 2;
            n_left_from -= 2;
            n_left_to_next -= 2;

            b0 = vlib_get_buffer(vm, bi0);
            b1 = vlib_get_buffer(vm, bi1);

            /*******************  start handle pkt  ***************/

            u32 next0 = SF_NEXT_NODE_SF_PKT_DROP;
            u32 next1 = SF_NEXT_NODE_SF_PKT_DROP;

            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);
            sf_wdata_t *pkt_wdata1 = sf_wdata(b1);

            

            next0 = get_action_forward_path(pkt_wdata0 , get_additional_list(b0));
            next1 = get_action_forward_path(pkt_wdata1 , get_additional_list(b1));

            /*******************************************************/
            sf_add_basic_trace_x2(vm, node, b0, b1,
                                  pkt_wdata0, pkt_wdata1, next0, next1);

            /* verify speculative enqueues, maybe switch current next frame */
            vlib_validate_buffer_enqueue_x2(vm, node, next_index,
                                            to_next, n_left_to_next,
                                            bi0, bi1, next0, next1);
        }

        while(n_left_from >0  && n_left_to_next >0)
        {
            u32 bi0;
            vlib_buffer_t *b0;

            to_next[0] = bi0 = from[0];
            
            from += 1;
            to_next += 1;
            n_left_from -= 1;
            n_left_to_next -= 1;

            b0 = vlib_get_buffer(vm ,bi0);

            /*******************  start handle pkt  ***************/
            u32 next0 = SF_NEXT_NODE_SF_PKT_DROP;

            sf_wdata_t *pkt_wdata0 = sf_wdata(b0);


            next0 = get_action_forward_path(pkt_wdata0 , get_additional_list(b0));

            /*******************************************************/
            sf_add_basic_trace_x1(vm, node, b0, pkt_wdata0, next0);

            vlib_validate_buffer_enqueue_x1(vm, node, next_index,
                                            to_next, n_left_to_next,
                                            bi0, next0);
        }
        vlib_put_next_frame(vm, node, next_index, n_left_to_next);
    }
    return frame->n_vectors;
}
