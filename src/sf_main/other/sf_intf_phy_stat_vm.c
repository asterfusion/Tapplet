
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

/************** statics *****************/

int update_single_intf_phy_statics(int interface_id ,  intf_old_stat_t *new_stat)
{
    sf_port_statics_t port_statics;

    int ret ;

    if(interface_id <=0 || interface_id > MAX_INTERFACE_CNT)
    {
        return -1;
    }

    ret = dpdk_get_sw_if_statics(interface_id , &port_statics);

    if(ret != 0)
    {
        return -1;
    }

    trans_port_statics_to_old_stat( &port_statics , new_stat);

    return 0;
}

/************** phy stat *****************/

int update_singel_intf_phy_info(int interface_id , intf_phy_status_t * phy_stat)
{
    int ret ;

    if(interface_id <=0 || interface_id > interfaces_stat->interfaces_cnt || phy_stat == NULL)
    {
        return -1;
    }

    ret = dpdk_get_intf_phy_stat(interface_id , phy_stat);
      
    if(ret != 0)
    {
        return -1;
    }

    return 0;
}


