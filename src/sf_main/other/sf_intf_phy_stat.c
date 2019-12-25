
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

#include "sf_interface.h"
#include "sf_interface_shm.h"
#include "sf_interface_tools.h"

#include "sf_thread_tools.h"

#include "sf_intf_phy_stat.h"

dpdk_get_sw_if_statics_t dpdk_get_sw_if_statics_fn = NULL;
dpdk_get_phy_stat_t dpdk_get_phy_stat_fn = NULL;

void sf_intf_dpdk_reg_statics_fn(dpdk_get_sw_if_statics_t dpdk_get_sw_if_statics_tmp)
{
    dpdk_get_sw_if_statics_fn = dpdk_get_sw_if_statics_tmp;
}
void sf_intf_dpdk_reg_phy_stat_fn(dpdk_get_phy_stat_t dpdk_get_phy_stat)
{
    dpdk_get_phy_stat_fn = dpdk_get_phy_stat;
}


int dpdk_get_sw_if_statics(int interface_id , sf_port_statics_t *results)
{
    int sw_if_index = sf_interface_to_sw_if(interface_id);

    if(dpdk_get_sw_if_statics_fn == NULL)
    {
        clib_warning("dpdk_get_sw_if_statics_fn is not registed\n");
        return -1;
    }

    return dpdk_get_sw_if_statics_fn(sw_if_index , results);
}

int dpdk_get_intf_phy_stat(int interface_id , intf_phy_status_t * results)
{
    int sw_if_index = sf_interface_to_sw_if(interface_id);

    if(dpdk_get_phy_stat_fn == NULL)
    {
        clib_warning("dpdk_get_phy_stat_fn is not registed\n");
        return -1;
    }

    return dpdk_get_phy_stat_fn(sw_if_index , results);
}


void trans_port_statics_to_old_stat(sf_port_statics_t *port_statics , intf_old_stat_t *new_stat)
{
    #undef for_each_member_in_out
    #define for_each_member_in_out \
    FUNC(rx_packets) \
    FUNC(rx_octets) \
    FUNC(rx_error) \
    FUNC(rx_miss) \
    FUNC(tx_packets) \
    FUNC(tx_octets) \
    FUNC(tx_error) \
    FUNC(mask) 

    #undef FUNC

    #define FUNC(name) new_stat->phy_##name = port_statics->name;
    for_each_member_in_out
}



int update_all_intf_phy_stat(int is_first_time)
{
    int intf_id;
    if(interfaces_stat == NULL)
    {
        return -1;
    }
    for(intf_id = 1; intf_id <= MAX_INTERFACE_CNT ; intf_id ++)
    {

        if(intf_id <= interfaces_stat->interfaces_cnt)
        {
            update_singel_intf_phy_info(intf_id , interfaces_stat->intf_phy_status + (intf_id - 1));
        }
        else
        {
            memset(interfaces_stat->intf_phy_status + (intf_id - 1) , 0 , sizeof(intf_phy_status_t));
        }
        


        if(interfaces_stat->intf_phy_status[intf_id - 1].link )
        {
            interfaces_stat->intf_link[intf_id - 1] = 1;
        }
        else
        {
            interfaces_stat->intf_link[intf_id - 1] = 0;
        }


        if(interfaces_stat->intf_phy_status[intf_id - 1].link  == 0)
        {
            interfaces_stat->intf_phy_status[intf_id - 1].speed = 0;
            interfaces_stat->intf_phy_status[intf_id - 1].duplex = 0;
        }
    }


    return 0;
}
