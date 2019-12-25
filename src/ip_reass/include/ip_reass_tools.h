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
#ifndef __include_ip_reass_tools_h__
#define __include_ip_reass_tools_h__

#include "ip_reass_stat_tools.h"

// #define SF_DEBUG
#include "sf_debug.h"

static_always_inline void update_frag_wdata(sf_wdata_t *frag_wdata, sf_wdata_t *reass_wdata)
{
    frag_wdata->ip_wdata_cnt = reass_wdata->ip_wdata_cnt;
    frag_wdata->l4_wdata_cnt = reass_wdata->l4_wdata_cnt;
    frag_wdata->vlan_wdata_cnt = reass_wdata->vlan_wdata_cnt;

    memcpy( &(frag_wdata->vlan_wdatas), &(reass_wdata->vlan_wdatas), sizeof(vlan_wdata_t));

    memcpy( &(frag_wdata->l3l4_wdatas), &(reass_wdata->l3l4_wdatas) , sizeof(l3l4_decode_t));
}

static_always_inline uint32_t split_frag_buffer_on_fail(
    vlib_main_t *vm, vlib_buffer_t *b0, uint32_t *vlib_buffer_array,
    uint16_t *nexts, uint32_t worker_index, uint32_t interface_id,
    uint32_t frag_next_index, uint32_t next_drop)
{
    uint32_t count = 0;
    uint32_t pre_index = 0;
    vlib_buffer_array[count] = vlib_get_buffer_index(vm, b0);
    count++;
    vlib_buffer_t *tmp;
    vlib_buffer_t *child_brother_tmp;

    sf_debug("before while :pre_index %d  count %d\n", pre_index, count);

    while (pre_index != count)
    {
        sf_debug("in while :pre_index %d  count %d\n", pre_index, count);

        tmp = vlib_get_buffer(vm, vlib_buffer_array[pre_index]);

        //brother
        child_brother_tmp = ip_reass_record(tmp)->next_frag;
        if (child_brother_tmp != NULL)
        {
            vlib_buffer_array[pre_index + ip_reass_record(tmp)->followed_nodes_cnt + 1] = vlib_get_buffer_index(vm, child_brother_tmp);
            count++;

            sf_debug("brothrer %p (%d)\n", child_brother_tmp, pre_index + ip_reass_record(tmp)->followed_nodes_cnt + 1);
        }

        //child
        if (sf_wdata(tmp)->unused8 & UNUSED8_FLAG_IP_REASS_PKT)
        {
            child_brother_tmp = ip_reass_record(tmp)->child_frag;
            sf_debug("child %p (%d)\n", child_brother_tmp, pre_index + 1);

            vlib_buffer_array[pre_index + 1] = vlib_get_buffer_index(vm, child_brother_tmp);
            count++;
            nexts[pre_index] = next_drop;
        }
        //leaf
        else
        {
            sf_debug("leaf \n");
            //update wdata using b0
            if (pre_index != 0)
            {
                update_frag_wdata(sf_wdata(tmp), sf_wdata(b0));
            }

            sf_wqe_reset(tmp);

            nexts[pre_index] = frag_next_index;

            sf_debug("worker_index %d interface_id %d\n", worker_index, interface_id);

            update_ip_reass_stat(worker_index, interface_id, ip_reass_pkt_fail);
        }
        pre_index++;
    }

    sf_debug("count : %d\n", count);

    return count;
}

void update_ip_reass_pkt(vlib_buffer_t *b0);

#endif