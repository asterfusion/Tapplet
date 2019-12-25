
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

#include "sf_timer.h"
#include "generic_hash.h"
#include "sf_main.h"


#include "sf_tools.h"
#include "sf_pkt_tools.h"
#include "sf_thread_tools.h"

#include "ip_reass_timer.h"
#include "sf_ip_reass.h"
#include "sf_ip_reass_shm.h"

#include "ip_reass_tools.h"

// #define SF_DEBUG
#include "sf_debug.h"

typedef enum
{
  SF_NEXT_NODE_SF_ACL_ENTRY,
  SF_NEXT_NODE_SF_PKT_DROP,
  SF_NEXT_NODE_N_COUNT,
} sf_index_t;

/* *INDENT-OFF* */
VLIB_REGISTER_NODE(sf_ip_reass_timer_node) = {
    .name = "ip_reass_on_timer",
    .vector_size = sizeof(u32),
    .type = VLIB_NODE_TYPE_INTERNAL,
    .n_next_nodes = SF_NEXT_NODE_N_COUNT,

    /* edit / add dispositions here */
    .next_nodes = {
        [SF_NEXT_NODE_SF_ACL_ENTRY] = "sf_acl_entry",
        [SF_NEXT_NODE_SF_PKT_DROP] = "sf_pkt_drop",
    },
};


static __thread uint32_t *vlib_buffer_array = NULL;
static __thread uint16_t *nexts = NULL;


VLIB_NODE_FN(sf_ip_reass_timer_node)
(vlib_main_t *vm,
 vlib_node_runtime_t *node,
 vlib_frame_t *frame)
{
    
    u32 n_left_from;
    ip_reass_timer_data_t **from;

    from = vlib_frame_vector_args(frame);
    n_left_from = frame->n_vectors;

    sf_debug("recv %d pkts\n", n_left_from);

    generic_hash_t *table_ip_reass = &(sf_ip_reass_main.table_ip_reass);

    generic_pnode_t pnode;



    vec_validate(vlib_buffer_array, VLIB_FRAME_SIZE);
    vec_validate(nexts, VLIB_FRAME_SIZE);
    uint32_t count = 0;

    uint32_t worker_index = sf_get_current_worker_index();


    while (n_left_from > 0)
    {
        ip_reass_timer_data_t *timer_data;
        vlib_buffer_t *b0;
        node_ip_reass_t *node_ip_reass;

        timer_data =  from[0];
        from += 1;
        n_left_from -= 1;

        hashLock(table_ip_reass , timer_data->hash_index);

        pnode = timer_data->pnode;
        if(pnode == NULL)
        {
            hashUnlock(table_ip_reass, timer_data->hash_index);
            free_timer_info(timer_data);
            continue;
        }

        node_ip_reass = (node_ip_reass_t *)node_data(pnode);


        //TODO
        // if(node_ip_reass->recv_new_pkt_in_interval == 1)
        // {
        //     node_ip_reass->recv_new_pkt_in_interva = 0;
        //     if(ip_reass_submit_timer(pnode , timer_data->hash_index , ))
        // }
        
        hashFreeNode(table_ip_reass , pnode , timer_data->hash_index);
        hashUnlock(table_ip_reass, timer_data->hash_index);
        

        b0 = node_ip_reass->head;

        count = split_frag_buffer_on_fail(vm , b0 , 
            vlib_buffer_array , 
            nexts , 
            worker_index , 
            node_ip_reass->interface_id ,
            SF_NEXT_NODE_SF_ACL_ENTRY ,
            SF_NEXT_NODE_SF_PKT_DROP );

        vlib_buffer_enqueue_to_next(vm , node , vlib_buffer_array , nexts , count);

        // if(PREDICT_FALSE(sf_opaque(b0)->unused8 & UNUSED8_FLAG_IP_REASS_PKT))
        // {
        //     count = split_frag_buffer(vm , b0 , vlib_buffer_array , nexts);

        //     vlib_buffer_enqueue_to_next(vm , node , vlib_buffer_array , nexts , count);
        // }
        // else
        // {
        //     count = push_normal_pkts(vm , b0 , vlib_buffer_array , nexts);

        //     sf_push_pkt_to_next(vm , node , SF_NEXT_NODE_SF_ACL_ENTRY , vlib_buffer_array , count);
        // }    

        free_ip_reass_hash_node(pnode);

        update_ip_reass_stat(worker_index , node_ip_reass->interface_id , ip_reass_timer_timeout);
        update_ip_reass_stat(worker_index , node_ip_reass->interface_id , ip_reass_fail);
    }


        return frame->n_vectors;
}


