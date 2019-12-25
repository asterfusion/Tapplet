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
#ifndef __include_sf_acl_shm_h__
#define __include_sf_acl_shm_h__


#include <stdint.h>

#include "sf_shm_aligned.h"
#include "sf_api_macro.h"

#undef MOD_NAME

/** 2 **/
#define MOD_NAME acl

#include "common_header.h"
#include "acl_defines.h"
#include "sf_bitmap.h"

enum{
    ACL_RULE_TYPE_NONE = 0,
    ACL_RULE_TYPE_TUPLE4,
    ACL_RULE_TYPE_TUPLE6,
}sf_rule_type_t;

typedef struct acl_hash_bucket{
    uint32_t node_cnt : 8 ;
    uint32_t hash_next_rule : 24 ;
}sf_acl_hash_bucket_t;

typedef struct acl_hash_node{
    uint32_t hash_prev;
    uint32_t hash_next;
}sf_acl_hash_node_t;

typedef struct rule_map{
    uint32_t rule_type : 8;
    uint32_t actual_rule_id : 24;
}sf_rule_map_t;


typedef struct map_rule{
    uint32_t outer_id_valid:1; 
    uint32_t outer_group_id:7;
    uint32_t outer_rule_id:24;
}sf_map_rule_t;


/****************/
typedef struct{

    uint8_t is_ipv6;

    uint16_t sport_min;
    uint16_t sport_max;
    uint16_t dport_min;
    uint16_t dport_max;
    uint8_t proto_min;
    uint8_t proto_max;

    char sip[IP_STR_MAX_LEN];
    uint8_t sip_mask;
    char dip[IP_STR_MAX_LEN];
    uint8_t dip_mask;
}sf_tuple_rule_cfg_t;

typedef struct{

    uint16_t sport_min;
    uint16_t sport_max;
    uint16_t dport_min;
    uint16_t dport_max;
    uint8_t proto_min;
    uint8_t proto_max;
    uint8_t sip_mask;
    uint8_t dip_mask;

    uint8_t sip[4];
    uint8_t dip[4];
}sf_tuple4_rule_cfg_t;


typedef struct{

    uint16_t sport_min;
    uint16_t sport_max;
    uint16_t dport_min;
    uint16_t dport_max;
    uint8_t proto_min;
    uint8_t proto_max;
    uint8_t sip_mask;
    uint8_t dip_mask;

    uint8_t sip[16];
    uint8_t dip[16];
}sf_tuple6_rule_cfg_t;


typedef struct{
    sf_map_rule_t map_rule;
    sf_tuple4_rule_cfg_t rule_cfg;
}sf_tuple4_rule_t;

typedef struct{
    sf_map_rule_t map_rule;
    sf_tuple6_rule_cfg_t rule_cfg;
}sf_tuple6_rule_t;


/** 3 **/
typedef struct
{
    uint8_t cache_line_0[0] SF_ALIGNED_CACHE;

#define ACL_STILL_SYNC 3
#define ACL_NEED_SYNC 2
#define ACL_PREPARE_SYNC 1
#define ACL_SYNC_OK 0 
// -1 means ptr null
#define ACL_SYNC_FAILED -2

    int acl_sync_flag;
    int acl_sync_times;

    sf_rule_map_t rule_map[MAX_RULE_GROUP_CNT][MAX_RULE_PER_GROUP];
    
    uint8_t cache_line_2[0] SF_ALIGNED_CACHE;


    /*** tuple ***/
    // bitmap
    uint64_t tuple_rule_bitmap_v4[MAX_RULE_GROUP_CNT][TUPLE4_RULE_BITMAP_CNT];
    uint64_t tuple_rule_bitmap_v6[MAX_RULE_GROUP_CNT][TUPLE6_RULE_BITMAP_CNT];
   
    //hash
    sf_acl_hash_bucket_t tuple_hash_bucket_v4[MAX_RULE_GROUP_CNT][TUPLE4_RULE_HASH_BUCKET_CNT];
    sf_acl_hash_bucket_t tuple_hash_bucket_v6[MAX_RULE_GROUP_CNT][TUPLE6_RULE_HASH_BUCKET_CNT];

    sf_acl_hash_node_t tuple_hash_node_v4[MAX_RULE_GROUP_CNT][MAX_TUPLE4_RULE_PER_GROUP];
    sf_acl_hash_node_t tuple_hash_node_v6[MAX_RULE_GROUP_CNT][MAX_TUPLE6_RULE_PER_GROUP];

    //rule
    sf_tuple4_rule_t tuple_rule_v4[MAX_RULE_GROUP_CNT][MAX_TUPLE4_RULE_PER_GROUP];
    sf_tuple6_rule_t tuple_rule_v6[MAX_RULE_GROUP_CNT][MAX_TUPLE6_RULE_PER_GROUP];

}SF_ALIGNED SF_CONF_T ;


typedef struct
{
    uint64_t acl_rule_hit[MAX_RULE_GROUP_CNT][MAX_RULE_PER_GROUP];
} SF_STAT_T ;

#ifndef IN_NSHELL_LIB
SF_CONF_PTR_DECLARE_VPP
SF_STAT_PTR_DECLARE_VPP
#endif

INIT_CONFIG_FUNC_DECLARE


#define FORMAT_ACL_RULE_CODE(group_id , rule_id) \
((uint64_t)((group_id -1) * MAX_RULE_PER_GROUP + rule_id))


#endif
