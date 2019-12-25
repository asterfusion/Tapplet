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

#include "sf_main.h"

#include "sf_ip_reass.h"
#include "sf_ip_reass_shm.h"

#include "sf_timer.h"

sf_ip_reass_main_t sf_ip_reass_main;


extern vlib_node_registration_t sf_ip4_reass_node;
extern vlib_node_registration_t sf_ip6_reass_node;


static clib_error_t *sf_ip_reass_main_init(vlib_main_t *vm)
{
    sf_ip_reass_main_t  *irm = &sf_ip_reass_main;
    clib_error_t *error = 0;
    int ret;

    clib_memset_u8(irm , 0 , sizeof(sf_ip_reass_main_t));


    CALL_SHM_INIT_FUNC(vm , error);
    // fix_sf_main_link_error();



        ret = sf_pool_init(
            &(irm->hash_ip_reass_pool) , 
            "ip_reass_hash_node" ,
            sizeof(ip_reass_pool_node_t) , 
            ip_reass_config->pool_node_init_size,
            SF_POOL_DEFAULT_CACHE_SIZE , 
            128 , 10);

    if(ret != 0)
    {
        clib_error("hash_ip_reass_pool init failed!!!");
    }

    // ret = hashInit(&(irm->table_ip_reass) , 
    //               ip_reass_config->hashsize_ip_reass ,
    //               ip_reass_config->maxnum_ip_reass
    //           );
    

        ret = hashInit(&(irm->table_ip_reass) , 
            ip_reass_config->hashsize_ip_reass ,
            ip_reass_config->maxnum_ip_reass
            );

    if(ret != 0)
    {
        clib_error("ip reass hash table init failed!!!");
    }

    return error;
}

VLIB_INIT_FUNCTION(sf_ip_reass_main_init);



extern vlib_node_registration_t sf_ip_reass_timer_node;
static clib_error_t *ip_reass_timer_init(vlib_main_t *vm)
{
    sf_timer_input_regist_next_node(vm, SF_TW_TIMER_GROUP_IP_REASS,
            sf_ip_reass_timer_node.index);

    return 0;
}

VLIB_INIT_FUNCTION(ip_reass_timer_init);
