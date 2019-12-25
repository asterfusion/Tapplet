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

#include "sf_interface_tools.h"


typedef enum{
    SF_NEXT_NODE_SF_PKT_OUTPUT,

    SF_NEXT_NODE_SF_PKT_DROP,
    SF_NEXT_NODE_N_COUNT,
} sf_next_index_t;




VLIB_REGISTER_NODE(balance_intf_node) = {
    .name = "node_load_balance_interfaces",
    .vector_size = sizeof(u32),
    //.format_trace = format_sf_trace,
    .type = VLIB_NODE_TYPE_INTERNAL,

    .n_next_nodes = SF_NEXT_NODE_N_COUNT,

    /* edit / add dispositions here */
    .next_nodes = {
        [SF_NEXT_NODE_SF_PKT_OUTPUT] = "sf_pkt_output",
        [SF_NEXT_NODE_SF_PKT_DROP] = "sf_pkt_drop",
    },
};

#if 1
//Record interface real interface id
static uint16_t interface_array_real_load_balance[MAX_ACTION_COUNT][MAX_INTERFACE_CNT];
//Record interface real link num
static uint16_t interface_num_real_load_balance[MAX_ACTION_COUNT];
//Record interface link state transitions
static uint16_t interface_update_load_balance[MAX_ACTION_COUNT];
#endif

static_always_inline uint32_t load_balance_interfaces (
        sf_wdata_t  *wdata,
        sf_action_t *action,
        uint32_t action_id)
{
    int interface_id = INVALID_INTERFACE_ID;
    int interface_num;
    int index;

    load_balance_data_t  data;

    fill_load_balance_data(wdata, &data);

    interface_num = action->load_balance_interfaces_num;

    index = action_get_load_balance_index(action, &data, interface_num);

    interface_id = action->load_balance_interface_array[index];

#if 1
    if(!get_interface_admin_status(interface_id) || !get_interface_link(interface_id) )
    {
        int i = 0;
        if(action->load_balance_mode == LOAD_BALANCE_MODE_ROUND_ROBIN ||
           action->load_balance_mode == LOAD_BALANCE_MODE_WRR)
        {
            while(i < MAX_LOAD_BALANCE_NODE)
            {
                index = action_get_load_balance_index(action, &data, interface_num);
                interface_id = action->load_balance_interface_array[index];
                if(get_interface_admin_status(interface_id) && get_interface_link(interface_id))
                    break;
                i++;
            }
            interface_id = i < MAX_LOAD_BALANCE_NODE ? interface_id : INVALID_INTERFACE_ID;
        }
        else
        {
            uint16_t *interface_update = &interface_update_load_balance[action_id-1];
            uint16_t *interface_real_num = &interface_num_real_load_balance[action_id-1];
            uint16_t *interface_real_array = interface_array_real_load_balance[action_id-1];
            if(*interface_update  !=  0xFF)
            {
                *interface_real_num = 0;
                for(i=0; i<interface_num; ++i)
                {
                    if( get_interface_admin_status( action->load_balance_interface_array[i] ) &&  
                        get_interface_link( action->load_balance_interface_array[i] )  )
                    {
                        interface_real_array[(*interface_real_num)++] = action->load_balance_interface_array[i];
                    }
                }
                *interface_update = 1;
            }

            if(!*interface_real_num) return INVALID_INTERFACE_ID;
            index = action_get_load_balance_index(action, &data, *interface_real_num);
            interface_id = interface_real_array[index];
        }
    }
#endif
    return interface_id;
}



VLIB_NODE_FN(balance_intf_node)
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

    while(n_left_from > 0)
    {
        vlib_get_next_frame(vm, node, next_index, to_next, n_left_to_next);
        while(n_left_from >=4  && n_left_to_next >=2)
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

                sf_wdata_prefetch_full(p2);
                sf_wdata_prefetch_full(p3);
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
            u32 next0 = SF_NEXT_NODE_SF_PKT_OUTPUT;
            u32 next1 = SF_NEXT_NODE_SF_PKT_OUTPUT;
            uint8_t interface_id_0 = 0;
            uint8_t interface_id_1 = 0;

            sf_action_t *action_0 = get_action_ptr_nocheck(b0);
            sf_action_t *action_1 = get_action_ptr_nocheck(b1);
            sf_wdata_t * pkt_wdata_0 = sf_wdata(b0);
            sf_wdata_t * pkt_wdata_1 = sf_wdata(b1);

            interface_id_0 = load_balance_interfaces(pkt_wdata_0, action_0, pkt_wdata_0->arg1_to_next);
            interface_id_1 = load_balance_interfaces(pkt_wdata_1, action_1, pkt_wdata_1->arg1_to_next);

            if(interface_id_0 == INVALID_INTERFACE_ID)
                next0 = SF_NEXT_NODE_SF_PKT_DROP;

            if(interface_id_1 == INVALID_INTERFACE_ID)
                next1 = SF_NEXT_NODE_SF_PKT_DROP;

            set_tx_intf_index(b0 , interface_id_0);
            set_tx_intf_index(b1 , interface_id_1);
            ///////////////////////////////////////////////////////////
            /*sf_interface_t  egerss_config_t output_vlan_id is not 0*/
            /*TODO*/
            //   now is none 
            /*******************/

            //port_id = interface_id;
            //vnet_buffer(b0)->sw_if_index[VLIB_TX] = port_id;
             
            //////////////////////////////////////////////////////////

            vlib_validate_buffer_enqueue_x2(vm, node, next_index,
                                            to_next, n_left_to_next,
                                            bi0, bi1,next0, next1);
        }
        while(n_left_from >0  && n_left_to_next >0)
        {
            u32 bi0;
            vlib_buffer_t *b0;

            /* speculatively enqueue b0 to the current next frame */
            to_next[0] = bi0 = from[0];
            from += 1;
            to_next += 1;
            n_left_from -= 1;
            n_left_to_next -= 1;

            b0 = vlib_get_buffer(vm, bi0);

            /*******************  start handle pkt  ***************/
            u32 next0 = SF_NEXT_NODE_SF_PKT_OUTPUT;
            uint8_t interface_id_0 = 0;

            sf_action_t *action_0 = get_action_ptr_nocheck(b0);
            sf_wdata_t * pkt_wdata_0 = sf_wdata(b0);

            interface_id_0 = load_balance_interfaces(pkt_wdata_0, action_0, pkt_wdata_0->arg1_to_next);

            if(interface_id_0 == INVALID_INTERFACE_ID)
                next0 = SF_NEXT_NODE_SF_PKT_DROP;
                
            set_tx_intf_index(b0 , interface_id_0);
            ///////////////////////////////////////////////////////////
            /*sf_interface_t  egerss_config_t output_vlan_id is not 0*/
            /*TODO*/
            //   now is none 
            /*******************/

            //port_id = interface_id;
            //vnet_buffer(b0)->sw_if_index[VLIB_TX] = port_id;
             
            //////////////////////////////////////////////////////////

            vlib_validate_buffer_enqueue_x1(vm, node, next_index,
                                            to_next, n_left_to_next,
                                            bi0, next0);
        }
        vlib_put_next_frame(vm, node, next_index, n_left_to_next);
    }
    return frame->n_vectors;
}
