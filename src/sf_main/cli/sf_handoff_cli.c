
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
#include <vlib/vlib.h>


#include "sf_feature.h"
#include "sf_interface_shm.h"

#include "sf_thread_tools.h"
#include "sf_string.h"

static clib_error_t *
sf_show_handoff_statics_fn (vlib_main_t * vm,
		      unformat_input_t * input, vlib_cli_command_t * cmd)
{
    sf_intf_handoff_main_t *sm = &sf_intf_handoff_main;

    int intf_num = interfaces_stat->interfaces_cnt;
    int worker_num =  sf_vlib_num_workers();
    int intf_index;
    int worker_index;

    int need_show;

    uint64_t *cnt_per_intf;

    u8 *s = NULL;

    for(intf_index=0 ; intf_index < intf_num ; intf_index ++)
    {
        s = format(s , "%-3d", intf_index + 1);
    }
    vlib_cli_output(vm , "%v\n" , s);

    vec_free(s);
    s = NULL;

    for(intf_index=0 ; intf_index < intf_num ; intf_index ++)
    {
        if(sf_intf_get_handoff_status(intf_index + 1))
        {
            s = format(s , "%-3s", "*");
        }
        else
        {
            s = format(s , "%-3s", " ");
        }   
    }
    vlib_cli_output(vm , "%v\n" , s);
    vec_free(s);
    s = NULL;

    for(worker_index = 0 ; worker_index <worker_num ; worker_index ++ )
    {
        need_show = 0;
        cnt_per_intf = sm->handoff_pkt_cnt[worker_index];

        for(intf_index=0 ; intf_index < intf_num ; intf_index ++)
        {
            if(cnt_per_intf[intf_index] 
                && sf_intf_get_handoff_status(intf_index + 1))
            {
                need_show = 1;
                break;
            }
        }

        if(!need_show)
        {
            continue;
        }

        vlib_cli_output(vm , "-- worker %d --\n" , worker_index);

        for(intf_index=0 ; intf_index < intf_num ; intf_index ++)
        {

            if(cnt_per_intf[intf_index] 
                && sf_intf_get_handoff_status(intf_index + 1))
            {
                vlib_cli_output(vm , "%3d : %lu\n" , 
                    intf_index + 1  , cnt_per_intf[intf_index]);
            }
        }
    }


    return 0;
}

VLIB_CLI_COMMAND (sf_show_handoff_statics_command, static) = {
    .path = "sf show handoff stat",
    .short_help = "sf show handoff stat",
    .function = sf_show_handoff_statics_fn,
    .is_mp_safe = 1,
};




static clib_error_t *
sf_clear_handoff_statics_fn (vlib_main_t * vm,
		      unformat_input_t * input, vlib_cli_command_t * cmd)
{
    int worker_num =  sf_vlib_num_workers();
    int intf_num = interfaces_stat->interfaces_cnt;
    sf_intf_handoff_main_t *sm = &sf_intf_handoff_main;

    int worker_index;
    uint64_t *cnt_per_intf;


    for(worker_index = 0 ; worker_index <worker_num ; worker_index ++ )
    {
        cnt_per_intf = sm->handoff_pkt_cnt[worker_index];

        sf_memset_zero(cnt_per_intf , sizeof(uint64_t) * intf_num) ;
    }


    return 0;
}

VLIB_CLI_COMMAND (sf_clear_handoff_statics_command, static) = {
    .path = "sf clear handoff stat",
    .short_help = "sf clear handoff stat",
    .function = sf_clear_handoff_statics_fn,
    .is_mp_safe = 1,
};



static clib_error_t *
sf_set_intf_handoff_fn (vlib_main_t * vm,
		      unformat_input_t * input, vlib_cli_command_t * cmd)
{
    int intf_num = interfaces_stat->interfaces_cnt;

    int intf_id = -1;
    int is_enable = 1;

    while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT) {
        if (unformat (input, "%d" , &intf_id))
            ;
        if (unformat (input, "disable" ))
            is_enable = 0;
        else
            break;
    }

    if(intf_id <= 0  || intf_id > intf_num)
    {
        return clib_error_return (0, "Please specify intf_id (1-%d)..." , intf_num);
    }

    sf_intf_set_handoff_status(intf_id , is_enable);

    if(is_enable)
    {
        vlib_cli_output(vm , "enable %d handoff\n" , intf_id);
    }
    else
    {
        vlib_cli_output(vm , "disable %d handoff\n" , intf_id);
    }


    return 0;
}

VLIB_CLI_COMMAND (sf_set_intf_handoff_command, static) = {
    .path = "sf set handoff",
    .short_help = "sf set handoff [intf_id] [disable]",
    .function = sf_set_intf_handoff_fn,
    .is_mp_safe = 1,
};