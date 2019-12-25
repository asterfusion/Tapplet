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
#ifndef __include_sf_ip_reass_h__
#define __include_sf_ip_reass_h__

#include <vlib/vlib.h>
#include <vnet/vnet.h>
#include <vppinfra/sparse_vec.h>


#include "common_header.h"
#include "sf_pool.h"
#include "generic_hash.h"
#include "sf_timer.h"


// #define SF_DEBUG
#include "sf_debug.h"

typedef struct {
    ip_addr_t sip;
    ip_addr_t dip;
    uint32_t ip_id;
    uint32_t ip_layer;
}__attribute__((packed)) key_ip_reass_t;


typedef struct {
    key_ip_reass_t key;
    vlib_buffer_t *head;
    vlib_buffer_t *tail;
    uint8_t recv_new_pkt_in_interval;
    uint8_t cache_pkt_num;
    uint16_t recv_last_frag;
    uint16_t total_len;
    uint16_t meet_len;
    uint32_t interface_id;
    uint32_t resv;
}node_ip_reass_t;

typedef struct{
    generic_node_t node_info;
    node_ip_reass_t node_data;
    sf_timer_record_t timer_record;
}ip_reass_hash_node_t;


// typedef struct{
//     ip_reass_hash_node_t hash_node;
// }ip_reass_pool_node_t;

typedef ip_reass_hash_node_t ip_reass_pool_node_t;

typedef union{
    // 48 B
    // uint32_t opaque2[12];
    struct{
        // 8B
        vlib_buffer_t *child_frag;
        // 8B
        vlib_buffer_t *next_frag;
        // 6B
        uint16_t frag_offset;
        uint16_t frag_size;
        uint16_t ip_hdr_len;
        // 3B
        uint8_t interface_id; // Notice : 255 max
        uint8_t ip_reass_output;
        uint8_t followed_nodes_cnt; //trans tree to  array
    };
    
}ip_reass_record_t;


#define ip_reass_record(b0) ((ip_reass_record_t *)(sf_wdata(b0)->recv_for_opaque))


typedef struct{
    generic_hash_t table_ip_reass;
    sf_pool_head_t hash_ip_reass_pool;
}sf_ip_reass_main_t;


extern sf_ip_reass_main_t sf_ip_reass_main;



#define get_ip_reass_pool(i) \
(&(sf_ip_reass_main.hash_ip_reass_pool.per_worker_pool[i]))


static_always_inline generic_pnode_t 
alloc_ip_reass_hash_node()
{
    // ip_reass_pool_node_t *tmp = alloc_node_from_pool(get_ip_reass_pool(sf_local_worker_index) );
    // return (generic_pnode_t)&(tmp->hash_node);

    // return (generic_pnode_t)alloc_node_from_pool(get_ip_reass_pool(sf_local_worker_index) );
#ifdef SF_DEBUG
    generic_pnode_t tmp = (generic_pnode_t)alloc_node_from_pool(get_ip_reass_pool(sf_local_worker_index) );
    sf_debug("alloc node %p\n" , tmp);
    return tmp;
#else
    return (generic_pnode_t)alloc_node_from_pool(get_ip_reass_pool(sf_local_worker_index) );
#endif
}

static_always_inline void 
free_ip_reass_hash_node(generic_pnode_t pnode )
{
    sf_debug("free node %p\n" , pnode);

    // pnode_of_data(pnode) means the start of ip_reass_pool_node_t
    free_node_to_pool( get_ip_reass_pool(sf_local_worker_index) , pnode );
}


#endif