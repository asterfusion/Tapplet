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
/** 1 **/
#ifndef __include_sf_ip_reass_shm_h__
#define __include_sf_ip_reass_shm_h__


#include <stdint.h>

#include "sf_shm_aligned.h"
#include "sf_api_macro.h"

#include "sf_thread.h"
#include "sf_interface.h"


#undef MOD_NAME

/** 2 **/
#define MOD_NAME ip_reass

#define IP_REASS_NODE_INIT_SIZE 10000
#define IP_REASS_MAX_CACHE_PKTS 10
#define IP_REASS_DEFAULT_HASH_NODE_NUM     0x7fffffff
#define IP_REASS_DEFAULT_HASH_SIZE         (1024*1024)
#define IP_REASS_DEFAULT_MAX_LAYERS         1
#define IP_REASS_DEFAULT_OUTPUT_DISABLE     0
#define IP_REASS_DEFAULT_TIMEOUT_SEC        3
/** 3 **/
typedef struct
{
    uint32_t hashsize_ip_reass;
    uint32_t maxnum_ip_reass;
    uint32_t pool_node_init_size;
    uint32_t timeout_sec;
    // uint8_t ip_reass_outer_enable[MAX_INTERFACE_CNT];
    // uint8_t ip_reass_inner_enable[MAX_INTERFACE_CNT];

    // num of ip layer which should be handled
    uint8_t ip_reass_layers[MAX_INTERFACE_CNT];
    uint8_t ip_reass_output_enable[MAX_INTERFACE_CNT];

}SF_ALIGNED SF_CONF_T ;

typedef struct{
    // how many pkts success to reass
    uint64_t ip_reass_pkt_success;
    // how many pkts fail to reass
    uint64_t ip_reass_pkt_fail;

    uint64_t ip_reass_success;
    uint64_t ip_reass_fail;
    uint64_t ip_reass_fail_too_large;
    uint64_t ip_reass_fail_too_many;
    uint64_t ip_reass_timer_submit;
    uint64_t ip_reass_timer_delete;
    uint64_t ip_reass_timer_timeout;

}ip_reass_local_stat_t;

typedef struct
{
    ip_reass_local_stat_t local_stat[MAX_WORKER_THREADS][MAX_INTERFACE_CNT];
} SF_STAT_T ;

#ifndef IN_NSHELL_LIB
SF_CONF_PTR_DECLARE_VPP
SF_STAT_PTR_DECLARE_VPP
#endif

INIT_CONFIG_FUNC_DECLARE


#endif
