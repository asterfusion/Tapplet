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
#include "sf_timer.h"
#include "sf_ip_reass.h"
#include "ip_reass_stat_tools.h"

typedef struct{
    uint32_t recv;
    uint32_t hash_index;
    generic_pnode_t pnode;
}ip_reass_timer_data_t;




static_always_inline int 
ip_reass_submit_timer(generic_pnode_t pnode, uint32_t hash_index , uint32_t timeout_sec)
{
    ip_reass_timer_data_t *timer_data;
    sf_timer_record_t *timer_record;


    timer_data = (ip_reass_timer_data_t *)alloc_timer_info();

    if(timer_data == NULL)
    {
        return -1;
    }

    timer_data->pnode = pnode;
    timer_data->hash_index = hash_index;


    timer_record = GET_PNODE_TIMER_RECORD_PTR(ip_reass_hash_node_t , pnode);



    if(sf_submit_timer(timer_record , timer_data , SF_TW_TIMER_GROUP_IP_REASS , timeout_sec * 10) != 0)
    {
        free_timer_info(timer_data);
        return -1;
    }

    //sf_debug("pnode  : %p timer_record : %d\n" , pnode , timer_record->timer_index);

    return 0;

}



static_always_inline void 
ip_reass_delete_timer(generic_pnode_t pnode)
{
    sf_timer_record_t *timer_record;
    
    timer_record = GET_PNODE_TIMER_RECORD_PTR(ip_reass_hash_node_t , pnode);


	sf_stop_timer_void(timer_record);

    
	return;
}
