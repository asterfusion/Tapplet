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
#ifndef __include_sf_intf_phy_stat_h__
#define __include_sf_intf_phy_stat_h__

#include <stdint.h>
#include "sf_interface_shm.h"

typedef struct{
    uint64_t rx_packets;
    uint64_t rx_octets;
    uint64_t rx_error;
    uint64_t rx_miss;

    uint64_t tx_packets;
    uint64_t tx_octets;
    uint64_t tx_error;

    uint64_t mask;
}sf_port_statics_t;

typedef int (*dpdk_get_sw_if_statics_t)(int sw_if_index , sf_port_statics_t *results);
typedef int (*dpdk_get_phy_stat_t)(int sw_if_index , intf_phy_status_t *results);


extern dpdk_get_sw_if_statics_t dpdk_get_sw_if_statics_fn;
extern dpdk_get_phy_stat_t dpdk_get_phy_stat_fn;
// common
void sf_intf_dpdk_reg_statics_fn(dpdk_get_sw_if_statics_t dpdk_get_sw_if_statics_fn);
void sf_intf_dpdk_reg_phy_stat_fn(dpdk_get_phy_stat_t dpdk_get_phy_stat_fn);


int dpdk_get_sw_if_statics(int interface_id , sf_port_statics_t *results);
int dpdk_get_intf_phy_stat(int interface_id , intf_phy_status_t * results);

void trans_port_statics_to_old_stat(sf_port_statics_t *port_statics , intf_old_stat_t *new_stat);

int update_all_intf_phy_stat();


// specified
int update_single_intf_phy_statics(int interface_id ,   intf_old_stat_t *new_stat);
int update_singel_intf_phy_info(int interface_id , intf_phy_status_t * phy_stat);



#endif
