

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

#include <vppinfra/error_bootstrap.h>

#include <stdio.h>
#include <string.h>
#include "time.h"

#include "pkt_wdata.h"
#include "sf_main.h"

#include "sf_shm_main.h"
#include "sf_shm_vpp.h"

#include "sf_interface.h"
#include "sf_interface_shm.h"
#include "sf_main_shm.h"
#include "sf_interface_tools.h"


#include "sf_debug.h"


#include "syslog.h"
sf_main_t sf_main;

sf_intf_stat_pre_thread_t *sf_intf_stat_main;

STATIC_ASSERT(VLIB_SF_WDATA_SIZE == sizeof(sf_wdata_t)  , "SF wdata size is not right. check vpp source code");

//check wdata size when compiling
int check_sf_wdata_size()
{
    uint8_t test[VLIB_SF_WDATA_SIZE - sizeof(sf_wdata_t)] __attribute__((unused));
    uint8_t test2[sizeof(sf_wdata_t) - VLIB_SF_WDATA_SIZE] __attribute__((unused));

    return 0;
}


/**
 * @brief Initialize the sf plugin.
 */
static clib_error_t *sf_main_init(vlib_main_t *vm)
{
    sf_main_t *sm = &sf_main;
    clib_error_t *error = 0;

    // int i;

    clib_memset_u8(sm, 0, sizeof(sf_main_t));

    sm->vnet_main = vnet_get_main();
    sm->vlib_main = vlib_get_main();

    sm->cpu_clock_pre_second = vm->clib_time.clocks_per_second;


        sf_pool_init(
            &(sm->huge_pkt_16k_pool),
            "huge_pkt",
            SF_HUGE_PKT_16KB_MAX_SIZE,
            SF_HUGE_PKT_16KB_POOL_INIT_SIZE,
            SF_HUGE_PKT_16KB_POOL_INIT_SIZE,
            4,
            10);

    sf_intf_stat_main = clib_mem_alloc_aligned_at_offset(
            sizeof(sf_intf_stat_pre_thread_t) * ( sf_vlib_num_workers() + 1 ) , 
            CLIB_CACHE_LINE_BYTES , 0 , 0 );

    if(sf_intf_stat_main == NULL)
    {
        clib_error("alloc sf_intf_stat_main failed\n");
    }
    memset(sf_intf_stat_main , 0 , sizeof(sf_intf_stat_pre_thread_t) * ( sf_vlib_num_workers() + 1 )  );

    return error;
}

VLIB_INIT_FUNCTION(sf_main_init);


/************** shm init **********/
/**
 * @brief Initialize the sf plugin.
 */
static clib_error_t *sf_shm_init(vlib_main_t *vm)
{
    clib_error_t *error = 0;
    int ret;
    sf_debug("init\n");
    // judge if shm already exists
    if (get_and_link_all_shm() != 0)
    {
        sf_debug("create\n");

        ret = create_and_link_all_shm();

        if ( ret != 0 )
        {
            clib_error("create_and link shm error(ret : %d)." , ret);
        }

        ret = init_shm_main();

        if ( ret != 0)
        {
            clib_error("init shm main error(ret : %d)." , ret);
        }

        ret = upload_all_shm_reg();

        if ( ret != 0)
        {
            clib_error("init shm map error or memory is not enough(ret : %d)." , ret);
        }

        sf_debug("create ok\n");

        clib_warning("Try to load shm_config.....");
        if (load_shm_config_from_default_file() != 0)
        {
            clib_warning("Fail to load shm_config");
        }
        else
        {
            clib_warning("Finish loading shm_config");
        }
    }
    else
    {
        ret = check_shm_main();
        if (ret != 0)
        {
            clib_error("!!!! [ Fatal ] old shared memory needs to be deleted(minus %d).!!!", ret);
        }

        ret = update_all_callback_ptr();

        if ( ret != 0)
        {
            clib_error("init callback ptr error.(ret %d)" , ret);
        }
    }

    sf_main_stat->sf_vpp_status++;

    return error;
}

VLIB_INIT_FUNCTION(sf_shm_init);



static clib_error_t *sf_intf_admin_up_init(vlib_main_t *vm)
{
    clib_error_t *error = 0;

    CALL_SHM_INIT_FUNC(vm, error);

    interfaces_stat->dpdk_init_status = 0;

    return error;
}

VLIB_INIT_FUNCTION(sf_intf_admin_up_init);


/***************** each interface output index reg  ************************/
extern vlib_node_registration_t sf_pkt_output_node;
extern vlib_node_registration_t sf_huge_pkt_output_node;
clib_error_t *
sf_output_hw_interface_add_del(vnet_main_t *vnm,
                               u32 hw_if_index,
                               u32 is_create)
{
    vnet_hw_interface_t *hi = vnet_get_hw_interface(vnm, hw_if_index);
    vnet_sw_interface_t *sw = vnet_get_sw_interface(vnm, hi->sw_if_index);

    char *tx_node_name = (char *)format(0, "%v-tx", hi->name);

    u32 intf_next_index;
    u32 hugepkt_next_index;
    u32 interface_id;
    u32 node_index;

    vlib_node_t *output_node;


    if(sw == NULL || sw->sw_if_index == 0) // local0
    {
        return 0;
    }

    output_node = vlib_get_node_by_name(vnm->vlib_main, (u8 *)tx_node_name);

    if (output_node == NULL)
    {
        clib_error("node %s not exist\n", tx_node_name);
    }
    node_index = output_node->index;

    intf_next_index = vlib_node_add_next(vnm->vlib_main, sf_pkt_output_node.index, node_index);
    hugepkt_next_index = vlib_node_add_next(vnm->vlib_main, sf_huge_pkt_output_node.index, node_index);

    interface_id = sf_sw_if_to_interface(sw->sw_if_index);

    if(interface_id == (u32)-1 || interface_id > MAX_INTERFACE_CNT)
    {
        return 0;
    }

    vec_validate(sf_main.intf_output_next_index, interface_id);
    vec_validate(sf_main.hugepkt_output_next_index, interface_id);

    sf_main.intf_output_next_index[interface_id] = intf_next_index;
    sf_main.hugepkt_output_next_index[interface_id] = hugepkt_next_index;

    return 0;
}

VNET_HW_INTERFACE_ADD_DEL_FUNCTION(sf_output_hw_interface_add_del);
