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


#include "sf_acl_shm.h"


#define SF_DEBUG
#include "sf_debug.h"

int sync_all_rules(int check_sync);

static uword
acl_sync_process_fn(vlib_main_t *vm,
                                 vlib_node_runtime_t *rt,
                                 vlib_frame_t *f0)
{
    uword *event_data = 0;
    f64 sleep_time = 2;
    int ret;
    
    while (1)
    {
        vlib_process_wait_for_event_or_clock(vm, sleep_time);
        vlib_process_get_events(vm, &event_data);
        vec_reset_length(event_data);

        if (acl_config == NULL)
        {
            continue;
        }
// sf_debug("acl_sync flag : %d\n" , acl_config->acl_sync_flag);
        if(acl_config->acl_sync_flag == ACL_STILL_SYNC )
        {
            acl_config->acl_sync_flag = ACL_SYNC_FAILED;
        }
        
        if(acl_config->acl_sync_flag != ACL_NEED_SYNC )
        {
            continue;
        }

        acl_config->acl_sync_flag =  ACL_STILL_SYNC;

        ret = sync_all_rules(1);

        if(ret != 0)
        {
            acl_config->acl_sync_flag = ACL_SYNC_FAILED;
        }
        else
        {
            acl_config->acl_sync_flag = ACL_SYNC_OK;
            acl_config->acl_sync_times ++ ;
            acl_config->acl_sync_times &= (uint32_t)(1 << 31) - 1;
        }        
    }

    return 0;
}

/* *INDENT-OFF* */
VLIB_REGISTER_NODE(acl_sync_process_node, static) = {
    .function = acl_sync_process_fn,
    .type = VLIB_NODE_TYPE_PROCESS,
    .name = "sf_acl_sync_process",
    .process_log2_n_stack_bytes = 20,
};

/*****************************************/

int sync_rte_tuple_rules(int check_sync);

int sync_all_rules(int check_sync)
{
    int ret = 0;

    ret = sync_rte_tuple_rules(check_sync);

    return ret;
}

int sync_all_rules_at_start()
{
    return sync_all_rules(0);
}

