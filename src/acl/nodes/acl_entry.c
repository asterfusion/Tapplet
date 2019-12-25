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
#include <rte_acl.h>


#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vnet/pg/pg.h>
#include <vppinfra/error.h>




#include "pkt_wdata.h"
#include "sf_main.h"

#include "acl_main.h"
#include "sf_acl_lock.h"

#include "sf_pkt_tools.h"
#include "sf_interface_tools.h"


#include "acl_stat_tools.h"


#define SF_DEBUG
#include "sf_debug.h"

typedef enum
{
    SF_NEXT_NODE_SF_ACTION_MAIN,
    SF_ACL_NEXT_NODE_IP_REASS_FORWARD,
    SF_NEXT_NODE_SF_PKT_DROP,
    SF_NEXT_NODE_N_COUNT,
} sf_index_t;


/* *INDENT-OFF* */
VLIB_REGISTER_NODE(sf_acl_entry_node) = {

    .name = "sf_acl_entry",
    .vector_size = sizeof(u32),
    .n_next_nodes = SF_NEXT_NODE_N_COUNT,

    .next_nodes = {
        [SF_NEXT_NODE_SF_ACTION_MAIN] = "sf_action_main",
        [SF_ACL_NEXT_NODE_IP_REASS_FORWARD] = "ip_reass_forward",
        [SF_NEXT_NODE_SF_PKT_DROP] = "sf_pkt_drop",
    },
};
/* *INDENT-ON* */

#if MAX_RULE_GROUP_CNT == 1

VLIB_NODE_FN(sf_acl_entry_node)
(vlib_main_t *vm,
 vlib_node_runtime_t *node,
 vlib_frame_t *frame)
{
    uint8_t* data[VLIB_FRAME_SIZE];
    u32 results[VLIB_FRAME_SIZE];

    u32 n_left_from, *from, *to_next;
    u32 next_index;
    u32 pkt_index;
    uint32_t worker_index = sf_get_current_worker_index();
    uint32_t *acl_rule_hit_g = acl_rule_hit_stat_global;

    from = vlib_frame_vector_args(frame);
    n_left_from = frame->n_vectors;
    next_index = SF_NEXT_NODE_SF_ACTION_MAIN;

        sf_debug("recv %d pkts\n", n_left_from);

    for(pkt_index = 0 ; pkt_index < n_left_from ; pkt_index++)
    {
        data[pkt_index] = (uint8_t *)sf_wdata(vlib_get_buffer(vm , from[pkt_index]));
        results[pkt_index] = 0;
    }


    sf_acl_sync_lock(worker_index);

    if(sf_acl_main.v46_context[0] != NULL)
        rte_acl_classify(sf_acl_main.v46_context[0] , (const uint8_t **)data , results , n_left_from , 1);


    pkt_index = 0;
    while (n_left_from > 0)
    {
        u32 n_left_to_next;

        vlib_get_next_frame(vm, node, next_index,
                            to_next, n_left_to_next);

        while (n_left_from >= 4 && n_left_to_next >= 2)
        {

            u32 bi0;
            u32 bi1;
 

            /* Prefetch next iteration. */
            {
                vlib_buffer_t *p2, *p3;

                p2 = vlib_get_buffer(vm, from[2]);
                p3 = vlib_get_buffer(vm, from[3]);

                sf_wdata_prefetch_cache(p2);
                sf_wdata_prefetch_cache(p3);

            }

            /* speculatively enqueue b0 to the current next frame */
            to_next[0] = bi0 = from[0];
            to_next[1] = bi1 = from[1];
            from += 2;
            to_next += 2;
            n_left_from -= 2;
            n_left_to_next -= 2;

            /*******************  start handle pkt  ***************/

            u32 next0 = SF_NEXT_NODE_SF_ACTION_MAIN;
            u32 next1 = SF_NEXT_NODE_SF_ACTION_MAIN;

            sf_wdata_t *pkt_wdata0 = (sf_wdata_t *)data[pkt_index];
            sf_wdata_t *pkt_wdata1 = (sf_wdata_t *)data[pkt_index+1];
            uint32_t result0 = results[pkt_index];
            uint32_t result1 = results[pkt_index + 1];

            pkt_index += 2;

            sf_wqe_reset_wdata(pkt_wdata0);
            sf_wqe_reset_wdata(pkt_wdata1);

            if(pkt_wdata0->unused8 & UNUSED8_FLAG_IP_REASS_PKT)
            {
                next0 = SF_ACL_NEXT_NODE_IP_REASS_FORWARD;
            }

            if(pkt_wdata1->unused8 & UNUSED8_FLAG_IP_REASS_PKT)
            {
                next1 = SF_ACL_NEXT_NODE_IP_REASS_FORWARD;
            }

            pkt_wdata0->arg_to_next = 0;
            pkt_wdata1->arg_to_next = 0;

            if(result0 != 0)
            {
                update_acl_rule_hit_quick(acl_rule_hit_g , worker_index , 0 , result0);
                pkt_wdata0->arg_to_next = get_interface_rule_to_action(pkt_wdata0->interface_id, result0);
            }

            if(result1 != 0)
            {
                update_acl_rule_hit_quick(acl_rule_hit_g , worker_index , 0 , result1);
                pkt_wdata1->arg_to_next = get_interface_rule_to_action(pkt_wdata1->interface_id, result1);
            }

            if(pkt_wdata0->arg_to_next == 0)
            {
                pkt_wdata0->arg_to_next = get_interface_default_action(pkt_wdata0->interface_id);
            }
            
            if(pkt_wdata1->arg_to_next == 0)
            {
                pkt_wdata1->arg_to_next = get_interface_default_action(pkt_wdata1->interface_id);
            }

            /*****************************************************/

            /* verify speculative enqueues, maybe switch current next frame */
            vlib_validate_buffer_enqueue_x2(vm, node, next_index,
                                            to_next, n_left_to_next,
                                            bi0, bi1, next0, next1);
        }

        while (n_left_from > 0 && n_left_to_next > 0)
        {
            u32 bi0;

            /* speculatively enqueue b0 to the current next frame */
            to_next[0] = bi0 = from[0];
            from += 1;
            to_next += 1;
            n_left_from -= 1;
            n_left_to_next -= 1;

            /*******************  start handle pkt  ***************/
            u32 next0 = SF_NEXT_NODE_SF_ACTION_MAIN;

            sf_wdata_t *pkt_wdata0 = (sf_wdata_t *)data[pkt_index];
            uint32_t result0 = results[pkt_index];

sf_debug("result0 : %d\n" , result0);

            pkt_index ++;

            sf_wqe_reset_wdata(pkt_wdata0);

            if(pkt_wdata0->unused8 & UNUSED8_FLAG_IP_REASS_PKT)
            {
                next0 = SF_ACL_NEXT_NODE_IP_REASS_FORWARD;
            }


            pkt_wdata0->arg_to_next = 0;

            if(result0 != 0)
            {
                update_acl_rule_hit_quick(acl_rule_hit_g , worker_index , 0 , result0);
                pkt_wdata0->arg_to_next = get_interface_rule_to_action(pkt_wdata0->interface_id, result0);
            }

            if(pkt_wdata0->arg_to_next == 0)
            {
                pkt_wdata0->arg_to_next = get_interface_default_action(pkt_wdata0->interface_id);
            }
            
            /*****************************************************/
            /* verify speculative enqueue, maybe switch current next frame */
            vlib_validate_buffer_enqueue_x1(vm, node, next_index,
                                            to_next, n_left_to_next,
                                            bi0, next0);
        }

        vlib_put_next_frame(vm, node, next_index, n_left_to_next);
    }

    sf_acl_sync_unlock(worker_index);


    return frame->n_vectors;
}


#else

#error Not surpported yet

#endif