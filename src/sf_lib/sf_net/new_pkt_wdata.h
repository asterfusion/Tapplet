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
#ifndef __include_new_pkt_wdata_h__
#define __include_new_pkt_wdata_h__

/***** !!!!! include "pkt_wdata.h" !!!! *****/

#include <vppinfra/clib.h>

#include "sf_shm_aligned.h"
#include "sf_string.h"


typedef uint16_t sf_hdr_offset_t;
typedef uint16_t sf_wqe_len_t;


#define MAX_IP_WDATA 1

#define IP_VERSION_4 4
#define IP_VERSION_6 6
typedef struct
{
    //1 * 8 B
    sf_hdr_offset_t offset;
    uint8_t version;
    uint8_t protocol;
    sf_wqe_len_t header_len;
    sf_wqe_len_t payload_len;
    //4 * 8 B
    ip_addr_t sip;
    ip_addr_t dip;
    // 2 B
    uint8_t is_frag;
    uint8_t reass_over;
} __attribute__((packed)) ip_wdata_t;

typedef struct
{
    sf_hdr_offset_t offset;
    uint8_t inner_layer_count; // how many vlans in this layer
}__attribute__((packed)) vlan_wdata_t;

typedef struct
{
    // 1 *8B
    sf_hdr_offset_t offset;
    uint16_t sport;
    uint16_t dport;
    uint16_t header_len;
}__attribute__((packed)) l4_wdata_t;


typedef union{
    uint8_t cacheline_bytes[64];
    struct{
        ip_wdata_t ip_wdata;
        l4_wdata_t l4_wdata;
    };

}__attribute__((packed)) l3l4_decode_t;


/****************************************************************/
typedef struct
{
    union{
        uint8_t cacheline_bytes[64];
        struct{
            //8 B
            i16 current_data;  
            u16 current_length;  
            uint8_t interface_id;
            uint8_t interface_out; 
            uint8_t l2_hdr_offset;
            uint8_t unused8; // not unused now

            //8 B
            uint8_t *data_ptr;

            //8 B
            uint64_t timestamp;

            //8 B
            uint64_t packet_type;

            //8 B
            uint16_t outer_vlan_id;
            uint16_t outer_ether_type;
            uint8_t ip_wdata_cnt;
            uint8_t l4_wdata_cnt;
            uint8_t vlan_wdata_cnt;
            uint8_t recv8;

            //3B
            vlan_wdata_t vlan_wdatas;
            //1B
            uint8_t recv8_2;

            //4 B 
            union{
                uint32_t arg_to_next;
                struct{
                    uint16_t arg1_to_next;
                    uint16_t arg2_to_next;
                };
            };
        };
    };

    //64 bytes
    l3l4_decode_t l3l4_wdatas;

    //for action and ip_reass
    uint32_t recv_for_opaque[CLIB_CACHE_LINE_BYTES/4]; // uint8_t will cause [-Wstrict-aliasing]

    //using pre data area in vlib_buffer_t
    uint32_t recv_pre_data[0];
}SF_ALIGNED_CACHE sf_wdata_t;

//main wdata
#define SF_WDATA_MAIN_LEN (64 + 64)

#define reset_pkt_wdata(wdata) memset(wdata , 0 , SF_WDATA_MAIN_LEN)


static_always_inline void update_vlan_wdata(sf_wdata_t *wdata, sf_hdr_offset_t offset, uint8_t layer_cnt)
{
    wdata->vlan_wdatas.offset = offset;
    wdata->vlan_wdatas.inner_layer_count = layer_cnt;
    wdata->vlan_wdata_cnt = 1;
}

static_always_inline void update_ipv4_wdata(ip_wdata_t *ip_wdata, sf_hdr_offset_t offset,
                                            uint32_t sip, uint32_t dip, uint8_t protocol,
                                            sf_wqe_len_t header_len, sf_wqe_len_t payload_len , 
                                            uint8_t is_frag)
{
    ip_wdata->offset = offset;
    ip_wdata->version = IP_VERSION_4;
    ip_wdata->sip.addr_v4 = sip;
    ip_wdata->dip.addr_v4 = dip;
    ip_wdata->protocol = protocol;
    ip_wdata->header_len = header_len;
    ip_wdata->payload_len = payload_len;
    ip_wdata->is_frag = is_frag;
}
static_always_inline void update_ipv6_wdata(ip_wdata_t *ip_wdata, sf_hdr_offset_t offset,
                                            ip_addr_t *sip, ip_addr_t *dip, uint8_t protocol,
                                            sf_wqe_len_t header_len, sf_wqe_len_t payload_len ,
                                            uint8_t is_frag)
{

    ip_wdata->offset = offset;
    ip_wdata->version = IP_VERSION_6;
    ip_wdata->sip.addr_v6_upper = sip->addr_v6_upper;
    ip_wdata->sip.addr_v6_lower = sip->addr_v6_lower;
    ip_wdata->dip.addr_v6_upper = dip->addr_v6_upper;
    ip_wdata->dip.addr_v6_lower = dip->addr_v6_lower;
    ip_wdata->protocol = protocol;
    ip_wdata->header_len = header_len;
    ip_wdata->payload_len = payload_len;
    ip_wdata->is_frag = is_frag;
}

static_always_inline void update_l4_wdata(sf_wdata_t *wdata, sf_hdr_offset_t offset,
                                          uint16_t sport, uint16_t dport, uint8_t protocol, uint16_t header_len)
{
    wdata->l3l4_wdatas.l4_wdata.offset = offset;
    wdata->l3l4_wdatas.l4_wdata.sport = sport;
    wdata->l3l4_wdatas.l4_wdata.dport = dport;
    wdata->l3l4_wdatas.l4_wdata.header_len = header_len;
    wdata->l4_wdata_cnt = 1;

}


#define get_pre_l4_src_port(wdata)  (wdata->l3l4_wdatas.l4_wdata.sport)
#define get_pre_l4_dst_port(wdata)  (wdata->l3l4_wdatas.l4_wdata.dport)

#define get_l3_src_ip(wdata , index) (wdata->l3l4_wdatas.ip_wdata.sip)
#define get_l3_dst_ip(wdata , index) (wdata->l3l4_wdatas.ip_wdata.dip)

#define get_l3_ip_version(wdata , index) (wdata->l3l4_wdatas.ip_wdata.version)

#define get_current_ip_wdata(wdata) (&((wdata)->l3l4_wdatas.ip_wdata))

#endif /* include new pkt wdata   */
