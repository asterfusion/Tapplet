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
#include <stdio.h>
#include <stdlib.h>

#include "sf_shm_main.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#include <syslog.h>
#include <errno.h>
#include <time.h>

#include "sf_string.h"
#include "sf_bitmap.h"

#include "sf_acl_api.h"

#define SF_DEBUG
#include "nlib_debug.h"

#define CHECK_CONF_PTR       \
    if (SF_CONF_PTR == NULL) \
    {                        \
        return -1;           \
    }

#define CHECK_OUTER_RULE_ID                                               \
    if (outer_rule_id <= 0 || outer_rule_id > MAX_RULE_PER_GROUP) \
    {                                                                     \
        return -1;                                                        \
    }

/**********************  hash *************************/
int tuple_rule_cfg_cmp_v4(int group_id, int actual_rule_id, uint8_t *key)
{
    sf_tuple4_rule_cfg_t *tuple_cfg_src = &(SF_CONF_PTR->tuple_rule_v4[group_id - 1][actual_rule_id - 1].rule_cfg);
    return memcmp(key, tuple_cfg_src, sizeof(sf_tuple4_rule_cfg_t));
}
int tuple_rule_cfg_cmp_v6(int group_id, int actual_rule_id, uint8_t *key)
{
    sf_tuple6_rule_cfg_t *tuple_cfg_src = &(SF_CONF_PTR->tuple_rule_v6[group_id - 1][actual_rule_id - 1].rule_cfg);
    return memcmp(key, tuple_cfg_src, sizeof(sf_tuple6_rule_cfg_t));
}

int copy_tuple_cfg_to_tuple4(sf_tuple4_rule_cfg_t *tuple_rule_cfg, sf_tuple_rule_cfg_t *rule_cfg);
int copy_tuple_cfg_to_tuple6(sf_tuple6_rule_cfg_t *tuple6_rule_cfg, sf_tuple_rule_cfg_t *rule_cfg);

int tuple_rule_find_v4(int group_id, sf_tuple_rule_cfg_t *rule_cfg)
{
    int ret;
    sf_tuple4_rule_cfg_t tmp_cfg;

    ret = copy_tuple_cfg_to_tuple4(&tmp_cfg, rule_cfg);
    if (ret != 0)
    {
        return -1;
    }

    ret = acl_hash_find(
        SF_CONF_PTR->tuple_hash_bucket_v4[group_id -1] ,
        TUPLE4_RULE_HASH_BUCKET_CNT,
        SF_CONF_PTR->tuple_hash_node_v4[group_id - 1],
        (uint8_t *)&tmp_cfg,
        sizeof(sf_tuple4_rule_cfg_t),
        tuple_rule_cfg_cmp_v4,
        group_id
    );

    return ret;
}
int tuple_rule_find_v6(int group_id, sf_tuple_rule_cfg_t *rule_cfg)
{
    int ret;
    sf_tuple6_rule_cfg_t tmp_cfg;

    ret = copy_tuple_cfg_to_tuple6(&tmp_cfg, rule_cfg);
    if (ret != 0)
    {
        return -1;
    }

    ret = acl_hash_find(
        SF_CONF_PTR->tuple_hash_bucket_v6[group_id -1] ,
        TUPLE6_RULE_HASH_BUCKET_CNT,
        SF_CONF_PTR->tuple_hash_node_v6[group_id - 1],
        (uint8_t *)&tmp_cfg,
        sizeof(sf_tuple6_rule_cfg_t),
        tuple_rule_cfg_cmp_v6,
        group_id
    );

    return ret;
}

int tuple_hash_find_actual(int group_id, sf_tuple_rule_cfg_t *rule_cfg)
{
    if (rule_cfg->is_ipv6)
    {
        return tuple_rule_find_v6(group_id, rule_cfg);
    }
    else
    {
        return tuple_rule_find_v4(group_id, rule_cfg);
    }
}

int tuple_hash_find_outer(int group_id, int actual_rule_id , int is_ipv6)
{
    int outer_rule_id = 0;
    sf_map_rule_t *map_rule = NULL;

    if(actual_rule_id != 0)
    {
        if (is_ipv6)
        {
            map_rule = &(SF_CONF_PTR->tuple_rule_v6[group_id -1][actual_rule_id - 1].map_rule);
        }
        else
        {
            map_rule = &(SF_CONF_PTR->tuple_rule_v4[group_id -1][actual_rule_id - 1].map_rule);
        }

        if(map_rule->outer_id_valid)
        {
            outer_rule_id = map_rule->outer_rule_id;
        }
    }

    return outer_rule_id;

}

int tuple_hash_insert(int group_id,  int actual_rule_id, int is_ipv6)
{
    uint8_t *key;
    int key_len = is_ipv6 ? sizeof(sf_tuple6_rule_cfg_t) : sizeof(sf_tuple4_rule_cfg_t);
    sf_acl_hash_bucket_t *hash_buckets;
    uint32_t hash_bucket_cnt;
    sf_acl_hash_node_t *hash_nodes;
    if (is_ipv6)
    {
        key = (uint8_t *)&(SF_CONF_PTR->tuple_rule_v6[group_id - 1][actual_rule_id - 1].rule_cfg);
        hash_buckets = SF_CONF_PTR->tuple_hash_bucket_v6[group_id - 1];
        hash_bucket_cnt = TUPLE6_RULE_HASH_BUCKET_CNT;
        hash_nodes = SF_CONF_PTR->tuple_hash_node_v6[group_id - 1];
    }
    else
    {
        key = (uint8_t *)&(SF_CONF_PTR->tuple_rule_v4[group_id - 1][actual_rule_id - 1].rule_cfg);
        hash_buckets = SF_CONF_PTR->tuple_hash_bucket_v4[group_id - 1];
        hash_bucket_cnt = TUPLE4_RULE_HASH_BUCKET_CNT;
        hash_nodes = SF_CONF_PTR->tuple_hash_node_v4[group_id - 1];
    }

    return acl_hash_insert(
        hash_buckets ,
        hash_bucket_cnt,
        hash_nodes,
        key,
        key_len,
        actual_rule_id
        );
}

int tuple_hash_free(int group_id, int actual_rule_id, int is_ipv6)
{
    uint8_t *key;
    int key_len = is_ipv6 ? sizeof(sf_tuple6_rule_cfg_t) : sizeof(sf_tuple4_rule_cfg_t);
    sf_acl_hash_bucket_t *hash_buckets;
    uint32_t hash_bucket_cnt;
    sf_acl_hash_node_t *hash_nodes;
    if (is_ipv6)
    {
        key = (uint8_t *)&(SF_CONF_PTR->tuple_rule_v6[group_id - 1][actual_rule_id - 1].rule_cfg);
        hash_buckets = SF_CONF_PTR->tuple_hash_bucket_v6[group_id - 1];
        hash_bucket_cnt = TUPLE6_RULE_HASH_BUCKET_CNT;
        hash_nodes = SF_CONF_PTR->tuple_hash_node_v6[group_id - 1];
    }
    else
    {
        key = (uint8_t *)&(SF_CONF_PTR->tuple_rule_v4[group_id - 1][actual_rule_id - 1].rule_cfg);
        hash_buckets = SF_CONF_PTR->tuple_hash_bucket_v4[group_id - 1];
        hash_bucket_cnt = TUPLE4_RULE_HASH_BUCKET_CNT;
        hash_nodes = SF_CONF_PTR->tuple_hash_node_v4[group_id - 1];
    }

    return acl_hash_free(
        hash_buckets ,
        hash_bucket_cnt,
        hash_nodes,
        key,
        key_len,
        actual_rule_id
    );
}

/**********************  bitmap *************************/

int calculate_used_cnt(uint64_t bitmap)
{
    int cnt = 0;
    int i;
    if (bitmap == 0)
    {
        return 0;
    }
    if (bitmap == (uint64_t)-1)
    {
        return 64;
    }

    for (i = 0; i < 64; i++)
    {
        if (bitmap & 1UL)
        {
            cnt++;
        }
        bitmap >>= 1;
        if (bitmap == 0)
        {
            break;
        }
    }
    return cnt;
}

int tuple_bitmap_get_total_cnt(int group_id, int is_ipv6)
{
    if(is_ipv6)
    {
        return MAX_TUPLE6_RULE_PER_GROUP;
    }
    else
    {
        return MAX_TUPLE4_RULE_PER_GROUP;
    }
    
}
/*** check config_ptr/group_id/actual_rule_id outside ***/
int tuple_bitmap_get_used_cnt(int group_id, int is_ipv6)
{
    uint64_t *ptr = NULL;
    int bitmap_cnt;
    int i;
    int total_cnt = 0;

    if (is_ipv6)
    {
        ptr = SF_CONF_PTR->tuple_rule_bitmap_v6[group_id - 1];
        bitmap_cnt = TUPLE6_RULE_BITMAP_CNT;
    }
    else
    {
        ptr = SF_CONF_PTR->tuple_rule_bitmap_v4[group_id - 1];
        bitmap_cnt = TUPLE4_RULE_BITMAP_CNT;
    }

    for (i = 0; i < bitmap_cnt; i++)
    {
        total_cnt += calculate_used_cnt(ptr[i]);
    }

    return total_cnt;
}
/*** check config_ptr/group_id/actual_rule_id outside ***/
/* return : actual_rule_id */
int tuple_bitmap_get_first_unused(int group_id, int is_ipv6)
{
    uint64_t *ptr = NULL;
    int bitmap_cnt;
    int inner_index;
    int find_flag = 0;
    int bitmap_index;
    uint64_t temp;

    if (is_ipv6)
    {
        ptr = SF_CONF_PTR->tuple_rule_bitmap_v6[group_id - 1];
        bitmap_cnt = TUPLE6_RULE_BITMAP_CNT;
    }
    else
    {
        ptr = SF_CONF_PTR->tuple_rule_bitmap_v4[group_id - 1];
        bitmap_cnt = TUPLE4_RULE_BITMAP_CNT ;
    }

    for (bitmap_index = 0; bitmap_index < bitmap_cnt; bitmap_index++)
    {
        if (ptr[bitmap_index] != (uint64_t)(-1))
        {
            break;
        }
    }

    if (bitmap_index == bitmap_cnt)
    {
        return 0;
    }

    temp = ptr[bitmap_index];

    for (inner_index = 0; inner_index < 64; inner_index++)
    {
        if ((temp & 1UL) == 0)
        {
            find_flag = 1;
            break;
        }
        temp >>= 1;
    }

    if (!find_flag)
    {
        return 0;
    }

    return bitmap_get_id(bitmap_index, inner_index);
}
/*** check config_ptr/group_id/actual_rule_id outside ***/
void tuple_bitmap_set_used_flag(int group_id, int actual_rule_id, int is_ipv6)
{
    uint64_t *ptr = NULL;

    int index = bitmap_get_index(actual_rule_id - 1);
    uint64_t mask = bitmap_get_mask(actual_rule_id - 1);

    if (is_ipv6)
    {
        ptr = &(SF_CONF_PTR->tuple_rule_bitmap_v6[group_id - 1][index]);
    }
    else
    {
        ptr = &(SF_CONF_PTR->tuple_rule_bitmap_v4[group_id - 1][index]);
    }

    *ptr |= mask;
}
/*** check config_ptr/group_id/actual_rule_id outside ***/
void tuple_bitmap_unset_used_flag(int group_id, int actual_rule_id, int is_ipv6)
{
    uint64_t *ptr = NULL;
    int index = bitmap_get_index(actual_rule_id - 1);
    uint64_t mask = bitmap_get_mask(actual_rule_id - 1);

    if (is_ipv6)
    {
        ptr = &(SF_CONF_PTR->tuple_rule_bitmap_v6[group_id - 1][index]);
    }
    else
    {
        ptr = &(SF_CONF_PTR->tuple_rule_bitmap_v4[group_id - 1][index]);
    }

    *ptr &= ~mask;
}

/***********************************************/

/************************/
/****** rule config *****/
/************************/

void init_tuple_rule_cfg(sf_tuple_rule_cfg_t *rule_cfg)
{
    if (rule_cfg == NULL)
        return;

    sf_memset_zero(rule_cfg, sizeof(sf_tuple_rule_cfg_t));
}

int check_tuple_rule_cfg(sf_tuple_rule_cfg_t *rule_cfg)
{
    uint8_t buf[20];
    uint8_t ip_mask_max;
    int af_type;

    if (rule_cfg == NULL)
    {
        return -1;
    }

#undef foreach_member
#define foreach_member        \
    FUNC(is_ipv6, 0, 1)       \
    FUNC(sport_min, 0, 65535) \
    FUNC(sport_max, 0, 65535) \
    FUNC(dport_min, 0, 65535) \
    FUNC(dport_max, 0, 65535) \
    FUNC(proto_min, 0, 255)   \
    FUNC(proto_max, 0, 255)

#define FUNC(member, min, max)                                 \
    if (!(rule_cfg->member >= min && rule_cfg->member <= max)) \
    {                                                          \
        return -1;                                             \
    }
    foreach_member
#undef FUNC

#undef foreach_member
#define foreach_member         \
    FUNC(sport_min, sport_max) \
    FUNC(dport_min, dport_max) \
    FUNC(proto_min, proto_max)

#define FUNC(min, max)                 \
    if (rule_cfg->min > rule_cfg->max) \
    {                                  \
        return -1;                     \
    }
        foreach_member
#undef FUNC

        if (rule_cfg->is_ipv6) // ip6
    {
        ip_mask_max = 128;
        af_type = AF_INET6;
    }
    else // ip4
    {
        ip_mask_max = 32;
        af_type = AF_INET;
    }

    if (rule_cfg->sip_mask > ip_mask_max || rule_cfg->dip_mask > ip_mask_max)
    {
        return -1;
    }

    if (inet_pton(af_type, rule_cfg->sip, buf) != 1)
    {
        return -1;
    }

    if (inet_pton(af_type, rule_cfg->dip, buf) != 1)
    {
        return -1;
    }

    return 0;
}

#define foreach_shared_member_rule_cfg \
    FUNC(sport_min)                    \
    FUNC(sport_max)                    \
    FUNC(dport_min)                    \
    FUNC(dport_max)                    \
    FUNC(proto_min)                    \
    FUNC(proto_max)                    \
    FUNC(sip_mask)                     \
    FUNC(dip_mask)

#define CHECK_IP46_MAX_RULE_ID                                                                        \
    if (actual_rule_id <= 0 || actual_rule_id >                                                       \
                                   (is_ipv6 ? MAX_TUPLE6_RULE_PER_GROUP : MAX_TUPLE4_RULE_PER_GROUP)) \
    {                                                                                                 \
        return -1;                                                                                    \
    }

int copy_tuple_cfg_to_tuple4(sf_tuple4_rule_cfg_t *tuple_rule_cfg, sf_tuple_rule_cfg_t *rule_cfg)
{

#define FUNC(member) tuple_rule_cfg->member = rule_cfg->member;
    foreach_shared_member_rule_cfg
#undef FUNC

     if (inet_pton(AF_INET, rule_cfg->sip, tuple_rule_cfg->sip) != 1)
    {
        return -1;
    }

    if (inet_pton(AF_INET, rule_cfg->dip, tuple_rule_cfg->dip) != 1)
    {
        return -1;
    }

    return 0;
}

int copy_tuple_cfg_to_tuple6(sf_tuple6_rule_cfg_t *tuple6_rule_cfg, sf_tuple_rule_cfg_t *rule_cfg)
{
#define FUNC(member) tuple6_rule_cfg->member = rule_cfg->member;
    foreach_shared_member_rule_cfg
#undef FUNC

        if (inet_pton(AF_INET6, rule_cfg->sip, tuple6_rule_cfg->sip) != 1)
    {
        return -1;
    }

    if (inet_pton(AF_INET6, rule_cfg->dip, tuple6_rule_cfg->dip) != 1)
    {
        return -1;
    }

    return 0;
}
/**********************  set *************************/

int set_tuple4_rule_cfg(int group_id, int actual_rule_id, sf_tuple_rule_cfg_t *rule_cfg)
{
    sf_tuple4_rule_t *tuple_rule = NULL;
    sf_tuple4_rule_cfg_t *tuple_rule_cfg = NULL;
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (group_id <= 0 || group_id > MAX_RULE_GROUP_CNT)
    {
        return -1;
    }

    if (actual_rule_id <= 0 || actual_rule_id > MAX_TUPLE4_RULE_PER_GROUP)
    {
        return -1;
    }

    if (rule_cfg->is_ipv6)
    {
        return -1;
    }

    tuple_rule = &(SF_CONF_PTR->tuple_rule_v4[group_id - 1][actual_rule_id - 1]);
    tuple_rule_cfg = &(tuple_rule->rule_cfg);

    copy_tuple_cfg_to_tuple4(tuple_rule_cfg, rule_cfg);

    return 0;
}

int set_tuple6_rule_cfg(int group_id, int actual_rule_id, sf_tuple_rule_cfg_t *rule_cfg)
{
    sf_tuple6_rule_t *tuple_rule = NULL;
    sf_tuple6_rule_cfg_t *tuple_rule_cfg = NULL;
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (group_id <= 0 || group_id > MAX_RULE_GROUP_CNT)
    {
        return -1;
    }

    if (actual_rule_id <= 0 || actual_rule_id > MAX_TUPLE6_RULE_PER_GROUP)
    {
        return -1;
    }

    if (!(rule_cfg->is_ipv6))
    {
        return -1;
    }

    tuple_rule = &(SF_CONF_PTR->tuple_rule_v6[group_id - 1][actual_rule_id - 1]);
    tuple_rule_cfg = &(tuple_rule->rule_cfg);

    copy_tuple_cfg_to_tuple6(tuple_rule_cfg, rule_cfg);

    return 0;
}

int set_tuple_rule_cfg(int group_id, int actual_rule_id, sf_tuple_rule_cfg_t *rule_cfg, int is_ipv6)
{
    if (is_ipv6)
    {
        return set_tuple6_rule_cfg(group_id, actual_rule_id, rule_cfg);
    }
    else
    {
        return set_tuple4_rule_cfg(group_id, actual_rule_id, rule_cfg);
    }
}

/**********************  get *************************/
int get_tuple_rule_cfg_inline(int group_id, int actual_rule_id, sf_tuple_rule_cfg_t *rule_cfg, int is_ipv6)
{
    sf_tuple4_rule_t *tuple4_rule = NULL;
    sf_tuple4_rule_cfg_t *tuple4_rule_cfg = NULL;

    sf_tuple6_rule_t *tuple6_rule = NULL;
    sf_tuple6_rule_cfg_t *tuple6_rule_cfg = NULL;

    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (group_id <= 0 || group_id > MAX_RULE_GROUP_CNT)
    {
        return -1;
    }

    CHECK_IP46_MAX_RULE_ID

    if (rule_cfg == NULL)
    {
        return -1;
    }

    rule_cfg->is_ipv6 = is_ipv6 ? 1 : 0;

    if (is_ipv6)
    {
        tuple6_rule = &(SF_CONF_PTR->tuple_rule_v6[group_id - 1][actual_rule_id - 1]);
        tuple6_rule_cfg = &(tuple6_rule->rule_cfg);

#define FUNC(member) rule_cfg->member = tuple6_rule_cfg->member;
        foreach_shared_member_rule_cfg
#undef FUNC

            inet_ntop(AF_INET6, tuple6_rule_cfg->sip, rule_cfg->sip, IP_STR_MAX_LEN);
        inet_ntop(AF_INET6, tuple6_rule_cfg->dip, rule_cfg->dip, IP_STR_MAX_LEN);
    }
    else
    {
        tuple4_rule = &(SF_CONF_PTR->tuple_rule_v4[group_id - 1][actual_rule_id - 1]);
        tuple4_rule_cfg = &(tuple4_rule->rule_cfg);

#define FUNC(member) rule_cfg->member = tuple4_rule_cfg->member;
        foreach_shared_member_rule_cfg
#undef FUNC

            inet_ntop(AF_INET, tuple4_rule_cfg->sip, rule_cfg->sip, IP_STR_MAX_LEN);
        inet_ntop(AF_INET, tuple4_rule_cfg->dip, rule_cfg->dip, IP_STR_MAX_LEN);
    }

    return 0;
}

int get_tuple_rule_cfg(int group_id, int outer_rule_id, sf_tuple_rule_cfg_t *rule_cfg)
{
    int is_ipv6;
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (group_id <= 0 || group_id > MAX_RULE_GROUP_CNT)
    {
        return -1;
    }

    CHECK_OUTER_RULE_ID

    sf_rule_map_t *rule_map = NULL;

    rule_map = &(SF_CONF_PTR->rule_map[group_id - 1][outer_rule_id - 1]);

    switch (rule_map->rule_type)
    {
    case ACL_RULE_TYPE_TUPLE4:
        is_ipv6 = 0;
        break;
    case ACL_RULE_TYPE_TUPLE6:
        is_ipv6 = 1;
        break;
    default:
        return -1;
    }
    return get_tuple_rule_cfg_inline(group_id , rule_map->actual_rule_id, rule_cfg, is_ipv6);
}

/**********************  map *************************/
static void tuple_map_rule_map(int group_id , int outer_rule_id , int actual_rule_id , int is_ipv6)
{
    sf_rule_map_t *rule_map;
    sf_map_rule_t *map_rule;

    rule_map = &(SF_CONF_PTR->rule_map[group_id - 1][outer_rule_id - 1]);

    if(is_ipv6)
    {
        rule_map->rule_type = ACL_RULE_TYPE_TUPLE6;
    }
    else
    {
        rule_map->rule_type = ACL_RULE_TYPE_TUPLE4;
    }

    rule_map->actual_rule_id = actual_rule_id;

    if(is_ipv6)
    {
        map_rule = &(SF_CONF_PTR->tuple_rule_v6[group_id - 1][actual_rule_id - 1].map_rule);
    }
    else
    {
        map_rule = &(SF_CONF_PTR->tuple_rule_v4[group_id - 1][actual_rule_id - 1].map_rule);
    }
    
    map_rule->outer_id_valid = 1;
    map_rule->outer_group_id = group_id;
    map_rule->outer_rule_id = outer_rule_id;
}




//export
int tuple_try_map_new_rule(int group_id, int outer_rule_id, sf_tuple_rule_cfg_t *rule_cfg  , int check_old)
{
    if (SF_CONF_PTR == NULL)
    {
        return FAIL_REASON_INTERNAL_ERROR;
    }

    if (group_id <= 0 || group_id > MAX_RULE_GROUP_CNT)
    {
        return FAIL_REASON_INVALID_ID;
    }

    if(outer_rule_id <= 0 || outer_rule_id > MAX_RULE_PER_GROUP)
    {
        return FAIL_REASON_INVALID_ID;
    }

    if (rule_cfg == NULL)
    {
        return FAIL_REASON_INTERNAL_ERROR;
    }

    //check old rule
    if(check_old)
    {
        if(acl_get_rule_map_type(group_id , outer_rule_id) > 0)
        {
            return FAIL_REASON_OLD_RULE_EXIST;
        }
    }
    

    //check config
    if (check_tuple_rule_cfg(rule_cfg) != 0)
    {
        return FAIL_REASON_INVALID_CFG;
    }

    uint32_t actual_rule_id = 0;
    int outer_rule_id_duplicate = 0;
    int is_ipv6;
    
    is_ipv6 = rule_cfg->is_ipv6;

    actual_rule_id = tuple_hash_find_actual(group_id , rule_cfg);

    if(actual_rule_id == 0)
    {
        //create new one
        actual_rule_id = tuple_bitmap_get_first_unused(group_id, is_ipv6);

        if (actual_rule_id == 0)
        {
            return FAIL_REASON_RULE_FULL;
        }

        if (set_tuple_rule_cfg(group_id, actual_rule_id, rule_cfg, is_ipv6) != 0)
        {
            return FAIL_REASON_INTERNAL_ERROR;
        }
    }
    else
    {
        //check duplicate
        outer_rule_id_duplicate = tuple_hash_find_outer(group_id , actual_rule_id , is_ipv6);
        if( outer_rule_id_duplicate > 0)
        {
            //is just the rule asked
            if(outer_rule_id_duplicate == outer_rule_id)
            {
                // do nothing
                return 0;
            }
            // return FAIL_REASON_DUPLICATE_RULE;
            return outer_rule_id_duplicate;
        }
    }

    if(!check_old)
    {
        //PUT : delete old rule
        if(acl_delete_old_map(group_id  , outer_rule_id , 1) < 0)
        {
            return FAIL_REASON_INTERNAL_ERROR;
        }
    }

    tuple_map_rule_map( group_id,  outer_rule_id,  actual_rule_id,  is_ipv6);
    tuple_bitmap_set_used_flag(group_id, actual_rule_id, is_ipv6);
    tuple_hash_insert(group_id, actual_rule_id, is_ipv6);

    return 0;
}

/******************* delete ********************/
int tuple_delete_old_map(int group_id , int actual_rule_id, int is_ipv6 )
{
    sf_map_rule_t *map_rule = NULL;
    uint32_t *ref_cnt_array = NULL;

    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (group_id <= 0 || group_id > MAX_RULE_GROUP_CNT)
    {
        return -1;
    }

    CHECK_IP46_MAX_RULE_ID

    if (is_ipv6)
    {
        map_rule = &(SF_CONF_PTR->tuple_rule_v6[group_id - 1][actual_rule_id - 1].map_rule);
    }
    else
    {
        map_rule = &(SF_CONF_PTR->tuple_rule_v4[group_id - 1][actual_rule_id - 1].map_rule);
    }

    memset(map_rule, 0, sizeof(sf_map_rule_t));

    tuple_hash_free(group_id, actual_rule_id, is_ipv6);
    tuple_bitmap_unset_used_flag(group_id, actual_rule_id, is_ipv6);
    
    return 0;
}
