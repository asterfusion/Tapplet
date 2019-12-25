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


#include "sf_debug.h"

sf_timer_main_t sf_timer_main;

/************************************************/
/************ next node register function *******/
/************************************************/
/***
 * first, add GROUP id in sf_tw_timer_group_recode(sf_timer.h); 
 * Notice : Initialization orde issues
 */
extern vlib_node_registration_t sf_timer_input_node;

void sf_timer_input_regist_next_node(vlib_main_t *vm, u32 timer_group_id, u32 node_index)
{
    sf_timer_main_t *sf_timer_main_ptr = &sf_timer_main;
    u32 next_index;

    //in case other module init first
    {
        clib_error_t *error = vlib_call_init_function(vm, sf_timer_global_init);
        if (error)
            clib_error_report(error);
    }

    ASSERT(timer_group_id < SF_TW_TIMER_GROUP_CNT);

    next_index =
        vlib_node_add_next(vm, sf_timer_input_node.index, node_index);

    sf_timer_main_ptr->next_index_vec[timer_group_id] = next_index;
}

/************************************************/
/************ check timer group cnt       *******/
/************************************************/
int check_timer_group_cnt()
{
    uint8_t test[SF_TW_TIMER_GROUP_MAX - SF_TW_TIMER_GROUP_CNT] __attribute__((unused));
    return 0;
}

/************************************************/
/************ init timer main             *******/
/************************************************/

static clib_error_t *sf_timer_global_init(vlib_main_t *vm)
{
    sf_timer_main_t *sf_timer_main_ptr = &sf_timer_main;
    int i;

    sf_memset_zero(sf_timer_main_ptr, sizeof(sf_timer_main_t));

    sf_timer_main_ptr->next_index_vec =
        vec_new_ha(u32, SF_TW_TIMER_GROUP_CNT, 0, CLIB_CACHE_LINE_BYTES);

    sf_timer_main_ptr->tw_timer_array = vec_new(sf_tw_timer_t , sf_vlib_num_workers());

    if(sf_timer_main_ptr->next_index_vec == NULL || sf_timer_main_ptr->tw_timer_array == NULL)
    {
        clib_error("sf_timer_global_init failed ; memory not enough");
    }

    for (i = 0; i < SF_TW_TIMER_GROUP_CNT; i++)
    {
        //reset next index
        sf_timer_main_ptr->next_index_vec[i] = 0;
    }

    for (i = 0; i < sf_vlib_num_workers(); i++)
    {
        //reset tw timer
        sf_memset_zero(sf_timer_main_ptr->tw_timer_array + i , sizeof(sf_tw_timer_t));
    }

    /***** pool *****/
    sf_pool_init( 
        &(sf_timer_main_ptr->timer_info_pool) , 
        "timer_info" ,
        sizeof(sf_timer_info_t) , 
        SF_TIMER_INFO_POOL_INIT_SIZE , 
        SF_TIMER_INFO_POOL_INIT_SIZE , 
        SF_TIMER_INFO_POOL_INC_SIZE  , 
        10);

    sf_pool_init( 
        &(sf_timer_main_ptr->timer_chunk_pool) , 
        "timer_chunk" , 
        sizeof(sf_timer_chunk_t) , 
        SF_TIMER_CHUNK_POOL_INIT_SIZE , 
        SF_TIMER_CHUNK_POOL_INIT_SIZE , 
        SF_TIMER_CHUNK_POOL_INC_SIZE ,
        10 );



    return 0;
}

VLIB_INIT_FUNCTION(sf_timer_global_init);


/************************************************/
/************  debug cli                  *******/
/************************************************/

static clib_error_t *sf_show_timer_fn(vlib_main_t *vm,
                                unformat_input_t *input,
                                vlib_cli_command_t *cmd)
{
    // sf_timer_main_t *sf_timer_main_ptr = &sf_timer_main;

    uint32_t worker_cnt  = sf_vlib_num_workers();

    uint32_t i;    

    vlib_cli_output(vm , "---  expired timer ---\n");
    for(i=0 ; i< worker_cnt ; i++ )
    {
        vlib_cli_output(vm , "%2d -- %lu\n" , i , get_sf_timer_input_expire_cnt(i));
    }
    
    
    return 0;
}

/**
 * @brief CLI command to enable/disable the sf plugin.
 */
VLIB_CLI_COMMAND(sf_show_timer_command, static) = {
    .path = "sf show timer",
    .short_help =
        "sf show timer",
    .function = sf_show_timer_fn,
    .is_mp_safe = 1,
};
