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
#ifndef _include_sf_checksum_tools_h__
#define _include_sf_checksum_tools_h__

#include <vnet/ip/ip4_packet.h> /* for ip4_header_checksum and ip_csum_t */
#include "pkt_wdata.h"

// #define SF_DEBUG
// #include "sf_debug.h"

#define sf_ip4_chechsum(ip4_hdr) ip4_header_checksum((ip4_header_t *)(ip4_hdr))


static_always_inline void sf_tcp_checksum(ip_wdata_t *ip_wdata , sf_tcp_header_t *tcp_hdr , uint16_t tcp_total_len)
{
    uint8_t protocol = ip_wdata->protocol;
    uint16_t tcp_len = ip_wdata->payload_len;
    ip_csum_t sum0;

    tcp_hdr->checksum = 0;

    if(ip_wdata->version == IP_VERSION_4)
    {
        uint32_t sip = ip_wdata->sip.addr_v4;
        uint32_t dip = ip_wdata->dip.addr_v4;


        sum0 = sip;
        sum0 = ip_csum_with_carry (sum0 , dip);
        sum0 = ip_csum_with_carry (sum0 , clib_host_to_net_u32 (tcp_len + (protocol << 16)));

        sum0 = ip_incremental_checksum(sum0 , tcp_hdr , tcp_total_len);
        tcp_hdr->checksum = ~ip_csum_fold (sum0);

        // sf_debug("new checksum : %04x\n" , tcp_hdr->checksum);
    }
    else
    {

        sum0 = ip_wdata->sip.addr_v6_upper;
        sum0 = ip_csum_with_carry (sum0 , ip_wdata->sip.addr_v6_lower);
        sum0 = ip_csum_with_carry (sum0 , ip_wdata->dip.addr_v6_upper);
        sum0 = ip_csum_with_carry (sum0 , ip_wdata->dip.addr_v6_lower);
        sum0 = ip_csum_with_carry (sum0 , clib_host_to_net_u32 (tcp_len + (protocol << 16)));

        sum0 = ip_incremental_checksum(sum0 , tcp_hdr , tcp_total_len);
        tcp_hdr->checksum = ~ip_csum_fold (sum0);

        // sf_debug("new checksum : %04x\n" , tcp_hdr->checksum);
    }
}


#endif