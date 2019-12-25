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
#include "common_header.h"
#include "pkt_wdata.h"

#include "sf_huge_pkt_tools.h"
#include "sf_thread_tools.h"

#include "sf_ip_reass.h"

#include "ip_reass_tools.h"
#include "ip_reass_stat_tools.h"

// #define SF_DEBUG
#include "sf_debug.h"

typedef enum
{
    SF_NEXT_NODE_ACTION_ENTRY,
    SF_NEXT_NODE_SF_PKT_DROP,
    SF_NEXT_NODE_N_COUNT,
} sf_index_t;

VLIB_REGISTER_NODE(sf_ip_reass_forward_node) = {
    .name = "ip_reass_forward",
    .vector_size = sizeof(u32),
    .type = VLIB_NODE_TYPE_INTERNAL,
    .n_next_nodes = SF_NEXT_NODE_N_COUNT,

    /* edit / add dispositions here */
    .next_nodes = {
        [SF_NEXT_NODE_ACTION_ENTRY] = "sf_action_main",
        [SF_NEXT_NODE_SF_PKT_DROP] = "sf_pkt_drop",
    },
};

static_always_inline int split_frag_buffer(vlib_main_t *vm, vlib_buffer_t *b0, uint32_t *vlib_buffer_array,
                                           uint16_t *nexts, uint32_t worker_index)
{
    uint32_t count = 0;
    uint32_t pre_index = 0;
    vlib_buffer_t *tmp;
    vlib_buffer_t *child_brother_tmp;
    uint32_t interface_id;

    uint16_t frag_next_index = SF_NEXT_NODE_ACTION_ENTRY;

    vlib_buffer_array[pre_index] = vlib_get_buffer_index(vm, b0);

    if (sf_wdata(b0)->arg_to_next == 0)
    {
        frag_next_index = SF_NEXT_NODE_SF_PKT_DROP;
    }

    count = ip_reass_record(b0)->followed_nodes_cnt + 1;
    interface_id = ip_reass_record(b0)->interface_id;

    // sf_debug("before while :pre_index %d  count %d\n", pre_index, count);

    while (pre_index != count)
    {
        // sf_debug("in while :pre_index %d  count %d\n", pre_index, count);

        tmp = vlib_get_buffer(vm, vlib_buffer_array[pre_index]);

        //brother
        child_brother_tmp = ip_reass_record(tmp)->next_frag;
        if (child_brother_tmp != NULL)
        {
            vlib_buffer_array[pre_index + ip_reass_record(tmp)->followed_nodes_cnt + 1] = vlib_get_buffer_index(vm, child_brother_tmp);
            // sf_debug("brothrer %p (%d)\n", child_brother_tmp, pre_index + ip_reass_record(tmp)->followed_nodes_cnt + 1);
        }

        //child
        if (sf_wdata(tmp)->unused8 & UNUSED8_FLAG_IP_REASS_PKT)
        {
            child_brother_tmp = ip_reass_record(tmp)->child_frag;
            // sf_debug("child %p (%d)\n", child_brother_tmp, pre_index + 1);

            vlib_buffer_array[pre_index + 1] = vlib_get_buffer_index(vm, child_brother_tmp);

            nexts[pre_index] = SF_NEXT_NODE_SF_PKT_DROP;
        }
        //leaf
        else
        {
            // sf_debug("leaf \n");
            //update wdata using b0
            update_frag_wdata(sf_wdata(tmp), sf_wdata(b0));
            sf_wdata(tmp)->arg_to_next = sf_wdata(b0)->arg_to_next;

            sf_wqe_reset(tmp);
            // sf_debug("worker_index %d , interface_id %d\n", worker_index, interface_id);
            update_ip_reass_stat(worker_index, interface_id, ip_reass_pkt_success);

            nexts[pre_index] = frag_next_index;
        }
        pre_index++;
    }

    update_ip_reass_stat(worker_index, interface_id, ip_reass_success);

    // sf_debug("output enable : %d\n", ip_reass_record(b0)->ip_reass_output);

    if (ip_reass_record(b0)->ip_reass_output)
    {
        nexts[0] = frag_next_index;
    }

    // sf_debug("count : %d\n", count);

    return count;
}

static __thread uint32_t *vlib_buffer_array = NULL;
static __thread uint16_t *nexts = NULL;

VLIB_NODE_FN(sf_ip_reass_forward_node)
(vlib_main_t *vm,
 vlib_node_runtime_t *node,
 vlib_frame_t *frame)
{
    u32 n_left_from, *from;

    from = vlib_frame_vector_args(frame);
    n_left_from = frame->n_vectors;

    sf_debug("recv %d pkts\n", n_left_from);

    vec_validate(vlib_buffer_array, VLIB_FRAME_SIZE);
    vec_validate(nexts, VLIB_FRAME_SIZE);
    uint32_t count = 0;

    uint32_t worker_index = sf_get_current_worker_index();

    while (n_left_from > 0)
    {
        u32 bi0;
        vlib_buffer_t *b0;

        bi0 = from[0];
        from += 1;
        n_left_from -= 1;

        b0 = vlib_get_buffer(vm, bi0);
        /*******************  start handle pkt  ***************/

        count = split_frag_buffer(vm, b0, vlib_buffer_array, nexts, worker_index);
        sf_debug("count : %d %p\n", count, &count);

        vlib_buffer_enqueue_to_next(vm, node, vlib_buffer_array, nexts, (uint64_t)count);
    }

    return frame->n_vectors;
}
