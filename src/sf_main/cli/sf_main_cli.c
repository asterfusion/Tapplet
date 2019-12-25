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

#include "pkt_wdata.h"
#include "sf_main.h"

#include "sf_interface_shm.h"
#include "sf_interface_tools.h"

#include "sf_pool.h"

#include "sf_intf_phy_stat.h"

// in nshell-lib
single_interface_stat_t *get_interface_stat_ptr(int index);
int clean_interface_stat(int index);

static char *admin_status_to_str(int status)
{
    if(status)
    {
        return "up";
    }
    else
    {
        return "down";
    }
    
}

static clib_error_t *
sf_show_int_fn (vlib_main_t * vm,
		      unformat_input_t * input, vlib_cli_command_t * cmd)
{
    single_interface_stat_t *single_stat;
    
    int i;
    vlib_cli_output(vm , "===== id == status =====%9srx =====%9stx =====%9sdrop =====\n" , 
        "" , "" , "");

    for(i=1 ; i<= sf_get_intf_cnt() ; i++)
    {
        single_stat = get_interface_stat_ptr(i);
        if(single_stat == NULL)
        {
            continue;
        }
        vlib_cli_output(vm , "%8d %9s %16lu %16lu %18lu\n" , i , 
            admin_status_to_str(get_interface_admin_status(i)) , 
            single_stat->in_packets , 
            single_stat->out_packets , 
            single_stat->drop_packets);
        
        vlib_cli_output(vm , "%18s %16lu %16lu\n\n" ,  "" , single_stat->in_octets , single_stat->out_octets);
    }


#define SF_DEBUG

#if  defined(SF_DEBUG)  && !defined(BUILD_RELEASE)
    vlib_cli_output(vm , "\n\n" );

    for(i=1 ; i<= MAX_INTERFACE_CNT ; i++)
    {
        single_stat = get_interface_stat_ptr(i);
        if(single_stat == NULL)
        {
            continue;
        }
        vlib_cli_output(vm , "---- intf %d ----\n" , i );




#undef for_each_member_of_intf_stat
#define for_each_member_of_intf_stat \
FUNC(in_packets) \
FUNC(in_octets) \
FUNC(in_bps) \
FUNC(in_pps) \
FUNC(out_packets) \
FUNC(out_octets) \
FUNC(out_pps) \
FUNC(out_bps) \
FUNC(drop_packets) \
FUNC(tx_error_packets) \
FUNC(tx_ok_packets) \
FUNC(rx_miss_packets)

#undef FUNC
#define FUNC(name) vlib_cli_output(vm , "%20s : %lu\n" , #name , single_stat->name );
for_each_member_of_intf_stat

        int packet_type_index;
        for(packet_type_index=0 ; packet_type_index<PACKET_TYPE_CNT ; packet_type_index++)
        {
            vlib_cli_output(vm , "- %d -\n" , packet_type_index);
            vlib_cli_output(vm , "%s : %lu\n" , "pkts" ,  single_stat->packet_type_packets[packet_type_index] );
            vlib_cli_output(vm , "%s : %lu\n" , "bytes" , single_stat->packet_type_octets[packet_type_index] );
            vlib_cli_output(vm , "%s : %lu\n" , "bps",    single_stat->packet_type_bps[packet_type_index] );
            vlib_cli_output(vm , "%s : %lu\n" , "pps" ,   single_stat->packet_type_pps[packet_type_index] );
        }

    }

#endif

    return 0;
}

VLIB_CLI_COMMAND (sf_show_int_command, static) = {
    .path = "sf show int",
    .short_help = "sf show int",
    .function = sf_show_int_fn,
    .is_mp_safe = 1,
};


static clib_error_t *
sf_show_phy_int_fn (vlib_main_t * vm,
		      unformat_input_t * input, vlib_cli_command_t * cmd)
{

    int i;
    intf_old_stat_t intf_stat;
    int ret;
    for(i=1;i<sf_get_intf_cnt(); i++)
    {
        ret = update_single_intf_phy_statics(i , &intf_stat);

        if(ret != 0)
        {
            vlib_cli_output(vm , "-- %d -- no info\n" ,  i);
        }
        vlib_cli_output(vm , "-- %d --\n" ,  i);
#undef for_each_member
#define for_each_member \
    FUNC(phy_rx_packets) \
    FUNC(phy_rx_octets) \
    FUNC(phy_rx_error) \
    FUNC(phy_rx_miss) \
    FUNC(phy_tx_packets) \
    FUNC(phy_tx_octets) \
    FUNC(phy_tx_error) 


#undef FUNC
#define FUNC(name) vlib_cli_output(vm , "%20s : %lu\n" , #name , intf_stat.name );
for_each_member


    vlib_cli_output(vm , "%20s : 0x%016lx\n" , "phy_mask" , intf_stat.phy_mask );

    }

    return 0;
}

VLIB_CLI_COMMAND (sf_show_phy_int_command, static) = {
    .path = "sf show phy int",
    .short_help = "sf show phy int",
    .function = sf_show_phy_int_fn,
    .is_mp_safe = 1,
};

static clib_error_t *
sf_clear_int_fn (vlib_main_t * vm,
		      unformat_input_t * input, vlib_cli_command_t * cmd)
{

    int i;

    for(i=1 ; i<= sf_get_intf_cnt() ; i++)
    {
        clean_interface_stat(i);
    }
    
    return 0;
}

VLIB_CLI_COMMAND (sf_clear_int_command, static) = {
    .path = "sf clear int",
    .short_help = "sf clear int",
    .function = sf_clear_int_fn,
    .is_mp_safe = 1,
};



/*************** for performace test *****************/

static clib_error_t *
sf_show_int_rx_miss_fn (vlib_main_t * vm,
		      unformat_input_t * input, vlib_cli_command_t * cmd)
{
    single_interface_stat_t *single_stat;
    
    int intf_id = 0;

    while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT) {
        if (unformat (input, "%d" , &intf_id))
            ;
        else
            break;
    }

    if(intf_id == 0 )
    {
        return clib_error_return (0, "Please specify intf_id...");
    }

    if(intf_id <= 0 || intf_id >  sf_get_intf_cnt())
    {
        return clib_error_return (0, "intf_id not found...");
    }

    single_stat = get_interface_stat_ptr(intf_id);
    if(single_stat == NULL)
    {
        return 0;
    }
    
    vlib_cli_output(vm , "---- intf %d ----\n" , intf_id );

#undef for_each_member_of_intf_stat
#define for_each_member_of_intf_stat \
FUNC(rx_miss_packets)

#undef FUNC
#define FUNC(name) vlib_cli_output(vm , "%20s : %lu\n" , #name , single_stat->name );
for_each_member_of_intf_stat



    return 0;
}

VLIB_CLI_COMMAND (sf_show_int_rx_miss_command, static) = {
    .path = "sf show int rx-miss",
    .short_help = "sf show int rx-miss [intf-id]",
    .function = sf_show_int_rx_miss_fn,
    .is_mp_safe = 1,
};

