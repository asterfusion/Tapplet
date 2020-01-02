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

#include "sf_interface_shm.h"
#include "sf_interface_tools.h"

#include "sf_thread_tools.h"

#include "sf_intf_phy_stat.h"

#include "sf_feature.h"

#include "sf_string.h"

#define SF_DEBUG
#include "sf_debug.h"

#define TOTAL_THREAD_CNT ((sf_vlib_num_workers() + 1))

#define INTF_STATICS_LOOP_TIME (1.0)
#define INTE_PHY_STATIC_LOOP_TIME (2.0)
// typedef struct
// {
//     uint64_t in_packets;
//     uint64_t in_octets;
//     uint64_t out_packets;
//     uint64_t out_octets;
//     uint64_t packet_type_packets[PACKET_TYPE_CNT];
//     uint64_t packet_type_octets[PACKET_TYPE_CNT];
// } single_stat_t;

intf_old_stat_t *intf_old_stat_array;
sf_intf_stat_pre_thread_t *sf_intf_old_stat_record;

intf_old_stat_t intf_new_stat;
sf_intf_stat_pre_thread_t sf_intf_new_stat_record;

static int calculate_intf_statics_in_process = 0;

#define EXTRA_OCTETS_PER_PKT (12 + 8 + 4)

static_always_inline void calculate_pps_and_bps_internal(uint64_t old_packets, uint64_t new_packets,
                                                         uint64_t old_octets, uint64_t new_octets, uint64_t *pps, uint64_t *bps, f64 due_time, uint64_t mask, uint64_t extra_bytes)
{
    uint64_t delta_packets = (new_packets - old_packets) & mask;
    uint64_t delta_octets = (new_octets - old_octets) & mask;
    uint64_t pps_tmp = delta_packets / due_time;
    uint64_t bps_tmp = delta_packets * EXTRA_OCTETS_PER_PKT + delta_octets;

    if (extra_bytes)
    {
        bps_tmp -= delta_packets * extra_bytes;
    }

    bps_tmp = bps_tmp * 8 / due_time;

    if (pps != NULL)
    {
        *pps = pps_tmp;
    }
    if (bps != NULL)
    {
        *bps = bps_tmp;
    }
}

static_always_inline void calculate_pps_and_bps(uint64_t old_packets, uint64_t new_packets,
                                                uint64_t old_octets, uint64_t new_octets, uint64_t *pps, uint64_t *bps, f64 due_time, uint64_t mask)
{
    calculate_pps_and_bps_internal(old_packets, new_packets,
                                   old_octets, new_octets,
                                   pps, bps, due_time, mask, 0);
}

static_always_inline void calculate_pps_and_bps_minus_extra_bytes(uint64_t old_packets, uint64_t new_packets,
                                                                 uint64_t old_octets, uint64_t new_octets, uint64_t *pps, uint64_t *bps, f64 due_time, uint64_t mask, uint64_t extra_bytes)
{
    calculate_pps_and_bps_internal(old_packets, new_packets,
                                   old_octets, new_octets,
                                   pps, bps, due_time, mask, extra_bytes);
}

static void init_intf_phy_record(int intf_index_in_array)
{
    int ret;
    intf_old_stat_t *last_stat;

    last_stat = intf_old_stat_array + intf_index_in_array;
    ret = update_single_intf_phy_statics(intf_index_in_array + 1, last_stat);

    if (ret != 0)
    {
        clib_warning("intf %d init phy statics failed\n", intf_index_in_array + 1);
    }
}

static void calculate_single_interface(int intf_index_in_array, f64 due_time)
{
    int j;
    int k;
    single_interface_stat_t *intf_stat_target;
    sf_intf_stat_pre_thread_t *intf_new_stat_record = &(sf_intf_new_stat_record);
    sf_intf_stat_pre_thread_t *intf_old_stat_record;

    intf_old_stat_t *last_stat;
    intf_old_stat_t *new_stat = &(intf_new_stat);

    sf_memset_zero(new_stat, sizeof(intf_old_stat_t));

    sf_memset_zero(intf_new_stat_record, sizeof(sf_intf_stat_pre_thread_t));

    //collect data from dpdk or dsa port
    k = update_single_intf_phy_statics(intf_index_in_array + 1, new_stat);

    if (k != 0)
    {
        return;
    }

    //collect data from other threads
    for (j = 0; j < TOTAL_THREAD_CNT; j++)
    {
        clib_memcpy(intf_new_stat_record, &(sf_intf_stat_main[j]), sizeof(sf_intf_stat_pre_thread_t));
        intf_old_stat_record = &(sf_intf_old_stat_record[j]);

#undef for_earch_in_out_drop
#define for_earch_in_out_drop \
    FUNC(in_packets)          \
    FUNC(in_octets)           \
    FUNC(out_packets)         \
    FUNC(out_octets)          \
    FUNC(drop_packets)

#undef FUNC
#define FUNC(name)                                                                                                       \
    new_stat->name += intf_new_stat_record->name[intf_index_in_array] - intf_old_stat_record->name[intf_index_in_array]; \
    intf_old_stat_record->name[intf_index_in_array] = intf_new_stat_record->name[intf_index_in_array];

        for_earch_in_out_drop

#undef FUNC

            for (k = 0; k < PACKET_TYPE_CNT; k++)
        {
            new_stat->packet_type_packets[k] +=
                intf_new_stat_record->packet_type_stat[k][intf_index_in_array].packets -
                intf_old_stat_record->packet_type_stat[k][intf_index_in_array].packets;

            new_stat->packet_type_octets[k] +=
                intf_new_stat_record->packet_type_stat[k][intf_index_in_array].octets -
                intf_old_stat_record->packet_type_stat[k][intf_index_in_array].octets;

            intf_old_stat_record->packet_type_stat[k][intf_index_in_array].packets =
                intf_new_stat_record->packet_type_stat[k][intf_index_in_array].packets;

            intf_old_stat_record->packet_type_stat[k][intf_index_in_array].octets =
                intf_new_stat_record->packet_type_stat[k][intf_index_in_array].octets;
        }
    }

    //calculate pps , bps
    last_stat = intf_old_stat_array + intf_index_in_array;
    intf_stat_target = interfaces_stat->interfaces + intf_index_in_array;

    calculate_pps_and_bps(
        0, new_stat->in_packets,
        0, new_stat->in_octets,
        &(intf_stat_target->in_pps), &(intf_stat_target->in_bps),
        due_time,
        (uint64_t)(-1));

    if (new_stat->extra_byte_pre_pkt > 0)
    {
        calculate_pps_and_bps_minus_extra_bytes(
            last_stat->phy_tx_packets,
            new_stat->phy_tx_packets,
            last_stat->phy_tx_octets,
            new_stat->phy_tx_octets,
            &(intf_stat_target->out_pps), &(intf_stat_target->out_bps),
            due_time,
            new_stat->phy_mask,
            new_stat->extra_byte_pre_pkt);
    }
    else
    {

        calculate_pps_and_bps(
            last_stat->phy_tx_packets,
            new_stat->phy_tx_packets,
            last_stat->phy_tx_octets,
            new_stat->phy_tx_octets,
            &(intf_stat_target->out_pps), &(intf_stat_target->out_bps),
            due_time,
            new_stat->phy_mask);
    }

    for (j = 0; j < PACKET_TYPE_CNT; j++)
    {
        calculate_pps_and_bps(
            0,
            new_stat->packet_type_packets[j],
            0,
            new_stat->packet_type_octets[j],
            &(intf_stat_target->packet_type_pps[j]),
            &(intf_stat_target->packet_type_bps[j]),
            due_time,
            (uint64_t)(-1));
    }

    intf_stat_target->in_packets += new_stat->in_packets;
    intf_stat_target->in_octets += new_stat->in_octets;
    intf_stat_target->out_packets += new_stat->out_packets;
    intf_stat_target->out_octets += new_stat->out_octets;
    intf_stat_target->drop_packets += new_stat->drop_packets;

    intf_stat_target->rx_miss_packets +=
        (new_stat->phy_rx_miss - last_stat->phy_rx_miss) & (new_stat->phy_mask);

    intf_stat_target->tx_ok_packets +=
        (new_stat->phy_tx_packets - last_stat->phy_tx_packets) & (new_stat->phy_mask);

    if (intf_stat_target->tx_ok_packets <= intf_stat_target->out_packets)
    {
        intf_stat_target->tx_error_packets =
            intf_stat_target->out_packets -
            intf_stat_target->tx_ok_packets;
    }

    // update old data

    clib_memcpy(last_stat, new_stat, sizeof(intf_old_stat_t));
}

/********************* init ******************/

static clib_error_t *sf_statics_global_init(vlib_main_t *vm)
{

    intf_old_stat_array = clib_mem_alloc_aligned_at_offset(sizeof(intf_old_stat_t) * MAX_INTERFACE_CNT, 1, 0, 0);
    sf_intf_old_stat_record = clib_mem_alloc_aligned_at_offset(
        sizeof(sf_intf_stat_pre_thread_t) * TOTAL_THREAD_CNT,
        1, 0, 0);
    if (intf_old_stat_array == NULL || sf_intf_old_stat_record == NULL)
    {
        clib_error("alloc intf_old_stat_t fail.");
    }

    memset(intf_old_stat_array, 0, sizeof(intf_old_stat_t) * MAX_INTERFACE_CNT);
    memset(sf_intf_old_stat_record, 0, sizeof(sf_intf_stat_pre_thread_t) * TOTAL_THREAD_CNT);
    return 0;
}
VLIB_INIT_FUNCTION(sf_statics_global_init);

/***************************************/

static f64 last_time = 0;

static void calculate_all_intf_statics(vlib_main_t *vm)
{
    f64 current_time = 0;
    f64 due_time = 0;
    int i;

    current_time = vlib_time_now(vm);
    due_time = current_time - last_time;

    for (i = 0; i < sf_get_intf_cnt() ; i++)
    {
        calculate_single_interface(i, due_time);
        current_time = vlib_time_now(vm);
        due_time = current_time - last_time;
    }
    last_time = vlib_time_now(vm);
    
}

static uword
calculate_interface_stat_process(vlib_main_t *vm,
                                 vlib_node_runtime_t *rt,
                                 vlib_frame_t *f0)
{

    uword *event_data = 0;
    f64 sleep_time = INTF_STATICS_LOOP_TIME;

    int is_first_time = 1;
    int i;
    last_time = vlib_time_now(vm);

    while (1)
    {
        vlib_process_wait_for_event_or_clock(vm, sleep_time);
        vlib_process_get_events(vm, &event_data);
        vec_reset_length(event_data);

        if (interfaces_stat == NULL)
        {
            continue;
        }
        
        calculate_intf_statics_in_process = 1;

        if (is_first_time)
        {
            for (i = 0; i < sf_get_intf_cnt(); i++)
            {
                init_intf_phy_record(i);
            }
            is_first_time = 0;
            continue;
        }

        calculate_all_intf_statics(vm);

        calculate_intf_statics_in_process = 0;

    }

    return 0;
}

/* *INDENT-OFF* */
VLIB_REGISTER_NODE(calculate_interface_stat_node, static) = {
    .function = calculate_interface_stat_process,
    .type = VLIB_NODE_TYPE_PROCESS,
    .name = "calculate_interface_stat",
};

/***************************************/

static uword
update_intf_phy_stat_process(vlib_main_t *vm,
                             vlib_node_runtime_t *rt,
                             vlib_frame_t *f0)
{

    uword *event_data = 0;
    f64 sleep_time = INTE_PHY_STATIC_LOOP_TIME;
    int dpdk_port_cnt ;
    int i;
    vnet_hw_interface_t *sf_hw;
    vnet_main_t *vnm = vnet_get_main();
    int intf_id;
    clib_error_t *error = 0;
    /******* 1 : wait for dpdk signal ************/

    vlib_process_wait_for_event(vm);
    vlib_process_get_events(vm, &event_data);
    vec_reset_length(event_data);

    /* init mac address & phy status */
    
    if (interfaces_config == NULL || interfaces_stat == NULL)
    {
        clib_error("interfaces_config or interfaces_stat is NULL");
    }

    dpdk_port_cnt = interfaces_stat->dpdk_port_cnt;

    if(dpdk_port_cnt <= 0)
    {
        clib_warning(" ------- No interface detected ------- \n");
        return 0 ;
    }


/**************************/
#if defined(RUN_ON_VM)
interfaces_stat->interfaces_cnt = interfaces_stat->dpdk_port_cnt;

#ifdef DEVICE_HAS_HANDOFF_FEATURE
    //turn off interface handoff if queue is enough
    for(i=0 ; i<interfaces_stat->interfaces_cnt  ; i++)
    {
        if( interfaces_stat->dpdk_max_queue[ sf_interface_to_sw_if(i+1) - 1 ]  > 1 )
        {
            sf_intf_set_handoff_status(i+1 , 0);
        }
    }
#endif // DEVICE_HAS_HANDOFF_FEATURE

#endif // defined(RUN_ON_VM)
/**************************/

    for(i=0 ; i<dpdk_port_cnt ; i++ )
    {
        sf_hw = sf_vnet_get_sw_hw_interface(vnm , i + 1);
        if(sf_hw == NULL)
        {
            clib_warning("can not find sw_if_index %d 's hw_interface\n");
            continue;
        }

        intf_id = sf_sw_if_to_interface(sf_hw->sw_if_index);

        //mac
        if (sf_hw->hw_address != NULL && vec_len(sf_hw->hw_address) != 0 && intf_id != 0)
        {
            clib_memcpy(interfaces_config->interfaces[intf_id - 1].mac, sf_hw->hw_address, 6);
        }
    }

    /* update all intf phy stat */
    update_all_intf_phy_stat(1 /* is fisrt time */);

    /* auto up all dpdk ports */
    for(i=0 ; i<dpdk_port_cnt ; i++ )
    {
        sf_hw = sf_vnet_get_sw_hw_interface(vnm , i + 1);
        if(sf_hw == NULL)
        {
            continue;
        }

        error = vnet_sw_interface_set_flags(vnm, sf_hw->sw_if_index, VNET_SW_INTERFACE_FLAG_ADMIN_UP);
        if (error)
        {
            clib_error_report(error);
        }
    }

    /* now start rx */
    interfaces_stat->dpdk_init_status = 1;

    while (1)
    {
        vlib_process_wait_for_event_or_clock(vm, sleep_time);
        vlib_process_get_events(vm, &event_data);
        vec_reset_length(event_data);

        update_all_intf_phy_stat(0);
    }

    return 0;
}

/* *INDENT-OFF* */
VLIB_REGISTER_NODE(update_intf_phy_stat_node, static) = {
    .function = update_intf_phy_stat_process,
    .type = VLIB_NODE_TYPE_PROCESS,
    .name = "update_intf_phy_stat",
};


/**************************************************/

static clib_error_t *
sf_show_phy_int_stat_fn (vlib_main_t * vm,
		      unformat_input_t * input, vlib_cli_command_t * cmd)
{
    intf_phy_status_t *intf_status;
    int i;

    vlib_cli_output(vm , "Total interfaces : %d (dpdk ports: %d)\n" , 
        interfaces_stat->interfaces_cnt , interfaces_stat->dpdk_port_cnt );

    for(i=0 ; i<interfaces_stat->interfaces_cnt ; i++)
    {
        intf_status = &(interfaces_stat->intf_phy_status[i]);
        vlib_cli_output(vm , "%10s-- %d --  \n" , " " ,  i+1);

        vlib_cli_output(vm , "%20s : %s\n" , "Link" , intf_status->link ? "Up" : "Down");
        vlib_cli_output(vm , "%20s : %d\n" , "MTU" , intf_status->mtu);

        if(intf_status->speed)
        {
            vlib_cli_output(vm , "%20s : %dM\n" , "Speed" , intf_status->speed);
        }
        else
        {
            vlib_cli_output(vm , "%20s : Unknown\n", "Speed" );
        }


        switch (intf_status->duplex)
        {
            case SF_INTF_LINK_UNKNOWN_DUPLEX:
                vlib_cli_output(vm , "%20s : Unknown\n", "Duplex");
                break;
            case SF_INTF_LINK_HALF_DUPLEX:
                vlib_cli_output(vm , "%20s : Half\n", "Duplex");
                break;
            case SF_INTF_LINK_FULL_DUPLEX:
                vlib_cli_output(vm , "%20s : Full\n", "Duplex");
                break;                
            default:
                vlib_cli_output(vm , "%20s : Error\n", "Duplex");
                break;
        }
    }


    return 0;
}

VLIB_CLI_COMMAND (sf_show_phy_int_stat_command, static) = {
    .path = "sf show phy int info",
    .short_help = "sf show phy int info",
    .function = sf_show_phy_int_stat_fn,
    .is_mp_safe = 1,
};



/***      !!!! only for auto-test !!!!************************/
static clib_error_t *
sf_sync_intf_statics_fn (vlib_main_t * vm,
		      unformat_input_t * input, vlib_cli_command_t * cmd)
{
    if(calculate_intf_statics_in_process)
    {
        vlib_cli_output(vm , "calculate_intf_statics_in_process\n");
        return 0;
    }

    calculate_all_intf_statics(vm);

    vlib_cli_output(vm , "calculate intf statics finish\n");

    return 0;
}

/***      !!!! only for auto-test !!!!************************/
VLIB_CLI_COMMAND (sf_sync_intf_statics_fn_command, static) = {
    .path = "sf_auto_test sync intf statics",
    .short_help = "sf_auto_test sync intf statics",
    .function = sf_sync_intf_statics_fn,
    // need lock workers
    // .is_mp_safe = 1,
};
