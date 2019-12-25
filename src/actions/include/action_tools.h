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
#ifndef __ACTION_TOOLS_H__
#define __ACTION_TOOLS_H__

#include "action.h"
#include "action_main.h"
#include "action_shm.h"
#include "pkt_wdata.h"

#define is_valid_action_id(id) (((id) >= 1) && ((id) <= MAX_ACTION_NUM)) 

#define u64_X_VALUE_1  0xFFFFFFFFFFFFFFFF


#define VXLAN      0x0
#define PPP        0x01
#define GRE        0x02
#define DCE        0x03                                                                                                                                                                                                                                           
#define GTP        0x04
#define TRILL      0x05
#define MACINMAC   0x06
#define GRE_IN_MAC 0x07
#define ERSPAN_I_II 0x08
#define ERSPAN_III 0x09


static_always_inline sf_action_t *get_action(uint32_t action_id)
{
    sf_action_t *action;


    if(PREDICT_FALSE(!is_valid_action_id(action_id)))
        return NULL;

    action = &action_config->actions[action_id -1];
    if(PREDICT_FALSE(!action->ref_counter))
        return NULL;
    return action;
}

#define get_action_nocheck(action_id) (action_config->actions + ((action_id) - 1))

#define get_action_ptr_nocheck(b0) get_action_nocheck(sf_wdata(b0)->arg1_to_next)

static_always_inline void fill_load_balance_data(sf_wdata_t *wdata, load_balance_data_t *data)
{
    /*fill load_balance_data*/
    data->src_ip = get_l3_src_ip(wdata , 0);
    data->dst_ip = get_l3_dst_ip(wdata , 0);

    data->inner_exist = wdata->ip_wdata_cnt > 1 ? 1:0;

    data->inner_sip = get_l3_src_ip(wdata , 1);
    data->inner_dip = get_l3_dst_ip(wdata , 1);
    data->packet_type = wdata->packet_type;

#if 1
    if(IP_VERSION_4 == get_l3_ip_version(wdata, 0) )
    {
        data->src_ip.value[0] = clib_net_to_host_u64(data->src_ip.value[0] << 32);
        data->src_ip.value[1] = clib_net_to_host_u64(data->src_ip.value[1] << 32);
        data->dst_ip.value[0] = clib_net_to_host_u64(data->dst_ip.value[0] << 32);
        data->dst_ip.value[1] = clib_net_to_host_u64(data->dst_ip.value[1] << 32);
    }
    else
    {
        data->src_ip.value[0] = clib_net_to_host_u64(data->src_ip.value[0]);
        data->src_ip.value[1] = clib_net_to_host_u64(data->src_ip.value[1]);
        data->dst_ip.value[0] = clib_net_to_host_u64(data->dst_ip.value[0]);
        data->dst_ip.value[1] = clib_net_to_host_u64(data->dst_ip.value[1]);

    }
    if(IP_VERSION_4 == get_l3_ip_version(wdata, 1) )
    {
        data->inner_sip.value[0] = clib_net_to_host_u64(data->inner_sip.value[0] << 32);
        data->inner_sip.value[1] = clib_net_to_host_u64(data->inner_sip.value[1] << 32);
        data->inner_dip.value[0] = clib_net_to_host_u64(data->inner_dip.value[0] << 32);
        data->inner_dip.value[1] = clib_net_to_host_u64(data->inner_dip.value[1] << 32);

    }
    else
    {
        data->inner_sip.value[0] = clib_net_to_host_u64(data->inner_sip.value[0]);
        data->inner_sip.value[1] = clib_net_to_host_u64(data->inner_sip.value[1]);
        data->inner_dip.value[0] = clib_net_to_host_u64(data->inner_dip.value[0]);
        data->inner_dip.value[1] = clib_net_to_host_u64(data->inner_dip.value[1]);
    }
#endif

    /************************/
}

static_always_inline int load_balance_get_weight_index(load_balance_weight_t *data, int *weight, int num, uint32_t *index)
{
    while(1)
    {
        *index = (*index +1 ) % num;
        if(!*index || !data->cw )
        {
            data->cw += data->gcd;
            if(data->cw > data->mw)
            {
                data->cw = data->gcd;
            }
        }
        if(weight[*index] >= data->cw)
        {
            return *index;
        }
    }
}

static_always_inline int action_get_load_balance_index(sf_action_t *action, load_balance_data_t *data, int num)
{
    load_balance_mode_t mode = action->load_balance_mode;
    int index;
    if(!num)
        return 0;
    switch(mode)
    {
    case LOAD_BALANCE_MODE_NONE:
    case LOAD_BALANCE_MODE_ROUND_ROBIN:
        action->load_balance_index = (action->load_balance_index + 1)% num;
        return action->load_balance_index;
        break;

    case LOAD_BALANCE_MODE_WRR:
        index = load_balance_get_weight_index(
                         &action->weight_data, 
                         action->load_balance_weight, 
                         num , 
                         &action->load_balance_index
                         );
        return index;
        break;

    case LOAD_BALANCE_MODE_OUTER_SRCDSTIP:
        return (data->src_ip.value[0] + data->src_ip.value[1] +
                data->dst_ip.value[0] + data->dst_ip.value[1]) % num;
        break;

    case LOAD_BALANCE_MODE_OUTER_SRCIP:
        return (data->src_ip.value[0] + data->src_ip.value[1]) % num;
        break;

    case LOAD_BALANCE_MODE_OUTER_DSTIP:
        return (data->dst_ip.value[0] + data->dst_ip.value[1]) % num;
        break;

    default:
        action->load_balance_index = (action->load_balance_index + 1)% num;
        return action->load_balance_index;
        break;
    }
    return 0;
}

#endif
