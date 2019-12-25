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
#include <vnet/vnet.h>
#include <vnet/plugin/plugin.h>
#include <vlib/cli.h>
#include <vlib/threads.h>
#include <vppinfra/os.h>

#include <stdio.h>

#include "sf_string.h"

#include "sf_thread_tools.h"


#include "sf_acl_shm.h"
#include "acl_stat_tools.h"


#define get_acl_rule_hit_minus(worker_index , grp_index , rule_id_index) \
(get_acl_rule_hit_gloabl(worker_index , grp_index , rule_id_index + 1) \
- \
get_acl_rule_hit_old(worker_index , grp_index , rule_id_index + 1)) \



#define update_acl_rule_hit_old(worker_index , grp_index , rule_id_index) \
get_acl_rule_hit_old(worker_index , grp_index , rule_id_index + 1) \
= \
get_acl_rule_hit_gloabl(worker_index , grp_index , rule_id_index + 1); \



void sf_acl_stat_sync_all()
{
    int worker_index ;
    int grp_index = -1;
    int rule_id_index = -1;

    for(worker_index = 0 ; worker_index < sf_vlib_num_workers() ; worker_index ++)
    {
        for(grp_index = 0 ; grp_index < MAX_RULE_GROUP_CNT ; grp_index ++)
        {
            for(rule_id_index = 0 ; rule_id_index < MAX_RULE_PER_GROUP ; rule_id_index ++)
            {
                acl_stat->acl_rule_hit[grp_index][rule_id_index] += 
                    get_acl_rule_hit_minus(worker_index , grp_index , rule_id_index);

                update_acl_rule_hit_old(worker_index , grp_index , rule_id_index);
            }
        }
    }

}

static clib_error_t * sf_acl_stat_sync_fn (vlib_main_t * vm,
                                   unformat_input_t * input,
                                   vlib_cli_command_t * cmd)
{
    
    int worker_index ;
    int grp_index = -1;
    int rule_id_index = -1;
    int all_flag = 0;

    while (unformat_check_input(input) != UNFORMAT_END_OF_INPUT)
    {
        if (unformat(input, "group %d" , &grp_index))
            ;
        else if (unformat(input, "rule %d" , &rule_id_index))
            ;
        else if (unformat(input, "all"))
            all_flag = 1;
        else
            break;
    }

    if(all_flag == 0 && rule_id_index == -1 && grp_index == -1)
    {
        return clib_error_return(0, "Please specify a rule or all rule ...");
    }


    if(all_flag )
    {
        sf_acl_stat_sync_all();
    }
    else
    {
        if(grp_index <=0 || grp_index > MAX_RULE_GROUP_CNT)
        {
            return clib_error_return(0, "group invalid ...");
        }

        if(rule_id_index <=0 || rule_id_index > MAX_RULE_PER_GROUP)
        {
            return clib_error_return(0, "rule id invalid ...");
        }

        grp_index --;
        rule_id_index --;



        for(worker_index = 0 ; worker_index < sf_vlib_num_workers() ; worker_index ++)
        {
            acl_stat->acl_rule_hit[grp_index][rule_id_index] += 
                get_acl_rule_hit_minus(worker_index , grp_index , rule_id_index);;

            update_acl_rule_hit_old(worker_index , grp_index , rule_id_index);
        }

        

    }
    

    return 0;
}

/**
 * @brief CLI command to enable/disable the sf plugin.
 */
VLIB_CLI_COMMAND (sf_acl_stat_sync_cmd, static) = {
    .path = "sf acl sync stat",
    .short_help = 
    "sf acl sync stat [group <id> rule <id> | all ]",
    .function = sf_acl_stat_sync_fn,
    .is_mp_safe = 1,
};

static clib_error_t * sf_acl_stat_clear_fn (vlib_main_t * vm,
                                   unformat_input_t * input,
                                   vlib_cli_command_t * cmd)
{
    int worker_index ;
    int grp_index = -1;
    int rule_id_index = -1;
    int all_flag = 0;

    while (unformat_check_input(input) != UNFORMAT_END_OF_INPUT)
    {
        if (unformat(input, "group %d" , &grp_index))
            ;
        else if (unformat(input, "rule %d" , &rule_id_index))
            ;
        else if (unformat(input, "all"))
            all_flag = 1;
        else
            break;
    }

    if(all_flag == 0 && rule_id_index == -1 && grp_index == -1)
    {
        return clib_error_return(0, "Please specify a rule or all rule ...");
    }


    if(all_flag )
    {

        memcpy(acl_rule_hit_stat_old , acl_rule_hit_stat_global , acl_rule_hit_stat_mem_size);

        sf_memset_zero(acl_stat , sizeof(acl_global_stat_t));

    }
    else
    {
        if(grp_index <=0 || grp_index > MAX_RULE_GROUP_CNT)
        {
            return clib_error_return(0, "group invalid ...");
        }

        if(rule_id_index <=0 || rule_id_index > MAX_RULE_PER_GROUP)
        {
            return clib_error_return(0, "rule id invalid ...");
        }

        grp_index --;
        rule_id_index --;
        

        acl_stat->acl_rule_hit[grp_index][rule_id_index] = 0;
        for(worker_index = 0 ; worker_index < sf_vlib_num_workers() ; worker_index ++)
        {
            update_acl_rule_hit_old(worker_index , grp_index , rule_id_index);
        }

    }



    return 0;
}

/**
 * @brief CLI command to enable/disable the sf plugin.
 */
VLIB_CLI_COMMAND (sf_acl_stat_clear_cmd, static) = {
    .path = "sf acl clear stat",
    .short_help = 
    "sf acl clear stat [group <id> rule <id> | all ]",
    .function = sf_acl_stat_clear_fn,
    .is_mp_safe = 1,
};




/****** only for debug ********/
#define cli_printf(fmt , ...)  vlib_cli_output(vm , fmt ,## __VA_ARGS__)

static clib_error_t *sf_show_acl_hit_func(vlib_main_t *vm,
                                    unformat_input_t *input,
                                    vlib_cli_command_t *cmd)
{
    int group_index;
    int rule_index;
    int thread_index;
    uint64_t result;


    for(group_index=0 ; group_index<MAX_RULE_GROUP_CNT;group_index++)
    {
        cli_printf("-- group %d --\n" , group_index + 1 ); 

        for(rule_index=0 ; rule_index< MAX_RULE_PER_GROUP ; rule_index++)
        {
            result = 0 ;
            for(thread_index=0 ; thread_index<sf_vlib_num_workers() ; thread_index++)
            {
                result += get_acl_rule_hit_gloabl(thread_index , group_index , rule_index + 1);
            }
            

            if(result != 0)
            {
                cli_printf("%6d -- %lu\n" , rule_index + 1 , result); 
            }
        }
    }
   
    return 0;
}


VLIB_CLI_COMMAND(sf_show_acl_hit_cmd, static) = {
    .path = "sf show acl hit",
    .short_help =
        "sf show acl hit",
    .function = sf_show_acl_hit_func,
    .is_mp_safe = 1,

};

