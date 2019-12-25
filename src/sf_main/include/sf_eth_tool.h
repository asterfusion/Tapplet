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
#ifndef __include_sf_eth_tool_h__
#define __include_sf_eth_tool_h__


#include <vlib/vlib.h>
#include "common_header.h"
#include "pkt_wdata.h"

// #define SF_DEBUG
#include "sf_debug.h"


/**********************************************************/
/******************  parse  function **********************/
/**********************************************************/

static_always_inline uint16_t parse_vlan_hdr_only(vlib_buffer_t *b0 , sf_wdata_t * pkt_wdata)
{

    uint8_t layer_cnt = 0;
    vlan_head_t *vlan_hdr;
    uint16_t eth_type = 0;
    uint16_t offset = sf_wqe_get_cur_offset_wdata(pkt_wdata);
    while(1)
    {
        if(PREDICT_FALSE(pkt_wdata->current_length <= VLAN_HEADER_LEN))
        {
            return ETHER_INVALID_TYPE;
        }
        vlan_hdr = (vlan_head_t *)sf_wqe_get_current_wdata(pkt_wdata);
        layer_cnt ++;

        eth_type = vlan_hdr->eth_type;

        sf_wqe_advance_wdata(pkt_wdata , VLAN_HEADER_LEN);

        if(vlan_hdr->eth_type != ETHER_VLAN_NET_ORDER &&  vlan_hdr->eth_type != ETHER_QINQ_NET_ORDER)
        {
            break;
        }

    }

    if(PREDICT_TRUE(layer_cnt != 0))
    {
        update_vlan_wdata(pkt_wdata , offset , layer_cnt);
    }

    return eth_type;

}


typedef struct
{
    eth_header_t eth_hdr;
    vlan_head_t vlan_hdr;
} sf_eth_vlan_t;

static_always_inline void outer_vlan_quick_parse( sf_wdata_t * pkt_wdata , eth_header_t *eth_hdr)
{

    sf_eth_vlan_t *eth_vlan_hdr;
    
    eth_vlan_hdr = (sf_eth_vlan_t *)eth_hdr;

    if( (eth_vlan_hdr->eth_hdr.eth_type == ETHER_VLAN_NET_ORDER 
    ||  eth_vlan_hdr->eth_hdr.eth_type == ETHER_QINQ_NET_ORDER)
    && pkt_wdata->ip_wdata_cnt == 0
    )
    {
        pkt_wdata->outer_vlan_id = get_vlan_id_netorder( &(eth_vlan_hdr->vlan_hdr) );
    }

}

#endif