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
#include <vnet/vnet.h>
#include <vnet/pg/pg.h>
#include <vppinfra/error.h>

#include <stdio.h>

#include "sf_main.h"
#include "common_header.h"
#include "pkt_wdata.h"

#include "sf_checksum_tools.h"

#include "sf_huge_pkt_tools.h"
#include "sf_thread_tools.h"

#include "sf_ip_reass.h"
#include "ip_reass_timer.h"
#include "sf_ip_reass_shm.h"

#include "ip_reass_stat_tools.h"

#include "ip_reass_tools.h"

// #define SF_DEBUG
#include "sf_debug.h"

#define get_ip_wdata(wdata) (&((wdata)->l3l4_wdatas.ip_wdata))
#define get_l4_wdata(wdata) (&((wdata)->l3l4_wdatas.l4_wdata))

void update_ip_reass_pkt(vlib_buffer_t *b0)
{

    sf_wdata_t *pkt_wdata = sf_wdata(b0);

    ipv6_header_t *ip6_hdr;
    ipv4_header_t *ip4_hdr;

    ip_wdata_t *ip_wdata;

    ip_wdata = get_ip_wdata(pkt_wdata);
    if(ip_wdata->version == IP_VERSION_4)
    {
        ip4_hdr = (ipv4_header_t *)sf_wqe_get_current(b0);
        ip4_hdr->totalLen = clib_host_to_net_u16( ip_wdata->header_len + ip_wdata->payload_len );
        ip4_hdr->checksum = sf_ip4_chechsum(ip4_hdr);
    }
    else
    {
        ip6_hdr = (ipv6_header_t *)sf_wqe_get_current(b0);
        ip6_hdr->payload_len = clib_host_to_net_u16( ip_wdata->header_len + ip_wdata->payload_len  - IPV6_HEADER_LEN);
        //TODO : update ipv6 fragement header 
    }

}