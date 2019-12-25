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


#include "sf_string.h"

#include "sf_acl_api.h"


SF_CONF_PTR_DEFINE
SHM_MAP_REGISTION_IN_LIB_CONF

SF_STAT_PTR_DEFINE
SHM_MAP_REGISTION_IN_LIB_STAT

RESET_CONFIG_FUNC_DEFINITION

INIT_CONFIG_FUNC_DEFINITION()
{
    int i;
    int j;
    int acl_sync_flag;
    int acl_sync_times;

    if (SF_CONF_PTR_VPP == NULL)
    {
        return -1;
    }

    if(is_reset)
    {
        if(SF_CONF_PTR_VPP->acl_sync_flag > 0)
        {
            return -1;
        }
    }

    if(is_reset)
    {
        acl_sync_flag = SF_CONF_PTR_VPP->acl_sync_flag;
        acl_sync_times = SF_CONF_PTR_VPP->acl_sync_times;
    }

    sf_memset_zero(SF_CONF_PTR_VPP, sizeof(SF_CONF_T));

    if(is_reset)
    {
        SF_CONF_PTR_VPP->acl_sync_flag = acl_sync_flag ;
        SF_CONF_PTR_VPP->acl_sync_times = acl_sync_times ;
    }

    return 0;
}


/****************/
/*** sync flag **/
/****************/

int acl_get_sync_times()
{
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }

    return SF_CONF_PTR->acl_sync_times;
}


int acl_get_sync_flag()
{
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    return SF_CONF_PTR->acl_sync_flag;
}

int acl_set_sync_flag_need()
{
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (SF_CONF_PTR->acl_sync_flag != ACL_STILL_SYNC)
    {
        SF_CONF_PTR->acl_sync_flag = ACL_NEED_SYNC;
        return 0;
    }

    return -1;
}

int acl_set_sync_flag_prepare()
{
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if (SF_CONF_PTR->acl_sync_flag <= 0 )
    {
        SF_CONF_PTR->acl_sync_flag = ACL_PREPARE_SYNC;
        return 0;
    }

    return -1;
}

int acl_set_sync_flag_fail()
{
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    
    SF_CONF_PTR->acl_sync_flag = ACL_SYNC_FAILED;
    return 0;
}


/***********************************************/
int acl_group_cnt()
{   
    return MAX_RULE_GROUP_CNT;
}
int acl_rule_cnt_per_group()
{   
    return MAX_RULE_PER_GROUP;
}
/********************/
/***** rule map *****/
/********************/
int acl_get_rule_map_type(int group_id , int outer_rule_id )
{
    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if(group_id <= 0 || group_id > MAX_RULE_GROUP_CNT)
    {
        return -1;
    }
    if(outer_rule_id <= 0 || outer_rule_id > MAX_RULE_PER_GROUP)
    {
        return -1;
    }

    return SF_CONF_PTR->rule_map[ group_id -1 ][ outer_rule_id - 1 ].rule_type;
}


int acl_delete_old_map(int group_id , int outer_rule_id , int no_check)
{
    sf_rule_map_t *rule_map;
    int ret = 0;

    if (SF_CONF_PTR == NULL)
    {
        return -1;
    }
    if(group_id <= 0 || group_id > MAX_RULE_GROUP_CNT)
    {
        return -1;
    }
    if(outer_rule_id <= 0 || outer_rule_id > MAX_RULE_PER_GROUP)
    {
        return -1;
    }

    rule_map = &( SF_CONF_PTR->rule_map[group_id - 1][outer_rule_id - 1] );

    switch(rule_map->rule_type)
    {
        case ACL_RULE_TYPE_TUPLE4:
            ret = tuple_delete_old_map(group_id , rule_map->actual_rule_id , 0  );
            break;
        case ACL_RULE_TYPE_TUPLE6:
            ret = tuple_delete_old_map(group_id , rule_map->actual_rule_id , 1 );
            break;
        case ACL_RULE_TYPE_NONE:
            if(!no_check)
            {
                return -2;
            }

            break;

        default :
            return -1;
    }

    memset(rule_map , 0 , sizeof(sf_rule_map_t));


    return ret;
}
/********************/
/***** rule stat ****/
/********************/

uint64_t acl_get_rule_hit(int group_id , int outer_rule_id )
{
    if (SF_STAT_PTR == NULL)
    {
        return -1;
    }
    if(group_id <= 0 || group_id > MAX_RULE_GROUP_CNT)
    {
        return -1;
    }
    if(outer_rule_id <= 0 || outer_rule_id > MAX_RULE_PER_GROUP)
    {
        return -1;
    }

    return SF_STAT_PTR->acl_rule_hit[group_id - 1][outer_rule_id - 1];;
}



int acl_clear_rule_hit_single(int group_id , int outer_rule_id )
{
    if (SF_STAT_PTR == NULL)
    {
        return -1;
    }
    if(group_id <= 0 || group_id > MAX_RULE_GROUP_CNT)
    {
        return -1;
    }
    if(outer_rule_id <= 0 || outer_rule_id > MAX_RULE_PER_GROUP)
    {
        return -1;
    }

    SF_STAT_PTR->acl_rule_hit[group_id - 1][outer_rule_id - 1] = 0;

    return 0;
}

int acl_clear_rule_hit_all()
{
    if (SF_STAT_PTR == NULL)
    {
        return -1;
    }
    sf_memset_zero(SF_STAT_PTR , sizeof(SF_STAT_T));
    return 0;
}
/********************************************/
/***************  error    ******************/
/********************************************/
int acl_new_rule_error_to_str(int result , char *error_str , int error_str_len)
{
    if(result > 0)
    {
        result = FAIL_REASON_DUPLICATE_RULE;
    }
    switch(result)
    {
        case FAIL_REASON_INTERNAL_ERROR:
            strncpy(error_str , "server error" , error_str_len - 1 );
            break;
        case FAIL_REASON_OLD_RULE_EXIST:
            strncpy(error_str , "old rule exist" , error_str_len - 1 );
            break;
        case FAIL_REASON_INVALID_ID:
            strncpy(error_str , "invalid id" , error_str_len - 1 );
            break;
        case FAIL_REASON_INVALID_CFG:
            strncpy(error_str , "invalid rule cfg" , error_str_len - 1 );
            break;
        case FAIL_REASON_RULE_FULL:
            strncpy(error_str , "rule full" , error_str_len - 1 );
            break;
        case FAIL_REASON_MEMBER_RULE_FULL:
            strncpy(error_str , "member rule full" , error_str_len - 1 );
            break;
        case FAIL_REASON_DUPLICATE_RULE:
            strncpy(error_str , "duplicate rule" , error_str_len - 1 );
            break;
        default:
            snprintf(error_str , error_str_len - 1 , "(%d) ???" , result);
            break;
    }

    return 0;
}




/************************/
/*   sys_conf apis      */
/************************/
int acl_sys_conf_find_next_rule(int group_id , int start_rule_id )
{
    int find_flag = 0;
    sf_rule_map_t *rule_map = NULL;

    if(SF_CONF_PTR == NULL)
    {
        return -1;
    }

    if (group_id <= 0 || group_id > MAX_RULE_GROUP_CNT)
    {
        return -1;
    }

    start_rule_id ++;

    for( ; start_rule_id <= MAX_RULE_PER_GROUP ; start_rule_id ++ )
    {
        rule_map = &(SF_CONF_PTR->rule_map[group_id - 1][start_rule_id - 1]);
        if(rule_map->rule_type != 0)
        {
            find_flag = 1;
            break;
        }
    }  

    if(find_flag)
    {
        return start_rule_id;
    } 
    else
    {
        return 0;
    }
    
}
