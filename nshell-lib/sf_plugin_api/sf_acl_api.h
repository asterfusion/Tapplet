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
/* 1 */
#ifndef __include_sf_acl_api_h__
#define __include_sf_acl_api_h__

#include <stdint.h>

#undef IN_NSHELL_LIB
#define IN_NSHELL_LIB

/* 2 */
#include "sf_acl_shm.h"

typedef enum{
    FAIL_REASON_INTERNAL_ERROR = -1,
    FAIL_REASON_OLD_RULE_EXIST = -2,
    FAIL_REASON_INVALID_ID = -3,
    FAIL_REASON_INVALID_CFG = -4,
    FAIL_REASON_RULE_FULL = -5,
    FAIL_REASON_MEMBER_RULE_FULL = -6 , 
    FAIL_REASON_DUPLICATE_RULE = -7,
}new_rule_fail_reason_t;


static inline char *trans_rule_fail_reason_to_str(int fail_reason)
{
    switch (fail_reason)
    {
        case FAIL_REASON_INTERNAL_ERROR: 
            return "internal error";
        case FAIL_REASON_OLD_RULE_EXIST: 
            return "old rule exist";
        case FAIL_REASON_INVALID_ID: 
            return "invalid id";
        case FAIL_REASON_INVALID_CFG:
            return "invalid rule config";
        case FAIL_REASON_RULE_FULL:
            return "rule full";
        case FAIL_REASON_MEMBER_RULE_FULL:
            return "member rule full";
        case FAIL_REASON_DUPLICATE_RULE: 
            return "duplicate rule";
    
        default:
            return "Unknown";
    }
}

SF_CONF_PTR_DECLARE
SF_STAT_PTR_DECLARE
RESET_CONFIG_FUNC_DECLARE

/****************/
/*** sync flag **/
/****************/
int acl_get_sync_times();
int acl_get_sync_flag();
int acl_set_sync_flag_need();
int acl_set_sync_flag_prepare();
int acl_set_sync_flag_fail();
/***********************************************/
int acl_group_cnt();
int acl_rule_cnt_per_group();
/********************/
/***** rule map *****/
/********************/
int acl_get_rule_map_type(int group_id , int outer_rule_id );
int acl_delete_old_map(int group_id , int outer_rule_id , int no_check);
/********************/
/***** rule stat ****/
/********************/
uint64_t acl_get_rule_hit(int group_id , int outer_rule_id );
int acl_clear_rule_hit_single(int group_id , int outer_rule_id );
int acl_clear_rule_hit_all();
/********************************************/
/***************  error    ******************/
/********************************************/
int acl_new_rule_error_to_str(int result , char *error_str , int error_str_len);
/********************/
/***** rule hash ****/
/********************/
typedef int (*hash_cmp_func_t)(int  group_id , int rule_id , uint8_t *key);

int acl_hash_find(sf_acl_hash_bucket_t *hash_buckets , uint32_t hash_bucket_cnt ,  sf_acl_hash_node_t *hash_nodes, 
    uint8_t *key , int key_len , hash_cmp_func_t rule_cfg_cmp , int group_id);
int acl_hash_insert(sf_acl_hash_bucket_t *hash_buckets , uint32_t hash_bucket_cnt ,  sf_acl_hash_node_t *hash_nodes, 
    uint8_t *key , int key_len , int acutal_rule_id);
int acl_hash_free(sf_acl_hash_bucket_t *hash_buckets , uint32_t hash_bucket_cnt ,  sf_acl_hash_node_t *hash_nodes, 
    uint8_t *key , int key_len , int acutal_rule_id);

/*************************************************/
/****** tuple                                *****/
/*************************************************/
/**********************  hash *************************/
/**********************  bitmap *************************/
int calculate_used_cnt(uint64_t bitmap);
/***********************************************/
/************************/
/****** rule config *****/
/************************/
void init_tuple_rule_cfg(sf_tuple_rule_cfg_t *rule_cfg);
int check_tuple_rule_cfg(sf_tuple_rule_cfg_t *rule_cfg);
/**********************  set *************************/
int set_tuple_rule_cfg(int group_id, int actual_rule_id, sf_tuple_rule_cfg_t *rule_cfg, int is_ipv6);
/**********************  get *************************/
int get_tuple_rule_cfg(int group_id, int outer_rule_id, sf_tuple_rule_cfg_t *rule_cfg);
/**********************  map *************************/
//export
int tuple_try_map_new_rule(int group_id, int outer_rule_id, sf_tuple_rule_cfg_t *rule_cfg  , int check_old);
/******************* delete ********************/
int tuple_delete_old_map(int group_id , int actual_rule_id, int is_ipv6 );



#endif
