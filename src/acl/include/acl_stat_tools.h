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
#ifndef __include_acl_rule_hit_tools_h__
#define __include_acl_rule_hit_tools_h__

#include "acl_defines.h"

extern uint32_t *acl_rule_hit_stat_global;
extern uint32_t *acl_rule_hit_stat_old;
extern uint32_t acl_rule_hit_stat_mem_size;

/****** stat ******/
#define update_acl_rule_hit_inline(acl_rule_hit_stat_inline , worker_index, group_index , rule_id) \
    acl_rule_hit_stat_inline[(worker_index) * MAX_RULE_GROUP_CNT  * MAX_RULE_PER_GROUP   \
        + (group_index) * MAX_RULE_PER_GROUP + (rule_id) - 1  ] += 1

// if increment is negative , then it means decrement
#define update_acl_rule_hit_multi_inline(acl_rule_hit_stat_inline , worker_index, group_index , rule_id, increment) \
    acl_rule_hit_stat_inline[(worker_index) * MAX_RULE_GROUP_CNT  * MAX_RULE_PER_GROUP   \
        + (group_index) * MAX_RULE_PER_GROUP + (rule_id) - 1  ] += increment

#define decrease_acl_rule_hit_inline(acl_rule_hit_stat_inline , worker_index, group_index , rule_id) \
    acl_rule_hit_stat_inline[(worker_index) * MAX_RULE_GROUP_CNT  * MAX_RULE_PER_GROUP   \
        + (group_index) * MAX_RULE_PER_GROUP + (rule_id) - 1  ] -= 1

#ifdef VALIDATE_STAT_NULL_POINTER

#define update_acl_rule_hit(worker_index, group_index , rule_id)         \
    if ((acl_rule_hit_stat_global) != NULL)                                 \
    {                                                              \
        update_acl_rule_hit_inline(acl_rule_hit_stat_global , worker_index, group_index , rule_id); \
    }

// if increment is negative , then it means decrement
#define update_acl_rule_hit_multi(worker_index, group_index , rule_id, increment)         \
    if ((acl_rule_hit_stat_global) != NULL)                                                  \
    {                                                                               \
        update_acl_rule_hit_multi_inline(acl_rule_hit_stat_global , worker_index, group_index , rule_id, increment); \
    }

#define decrease_acl_rule_hit(worker_index, group_index , rule_id)         \
    if ((acl_rule_hit_stat_global) != NULL)                                   \
    {                                                                \
        decrease_acl_rule_hit_inline(acl_rule_hit_stat_global , worker_index, group_index , rule_id); \
    }

#else // #if VALIDATE_STAT_NULL_POINTER

#define update_acl_rule_hit(worker_index, group_index , rule_id) \
    update_acl_rule_hit_inline(acl_rule_hit_stat_global , worker_index, group_index , rule_id)

// if increment is negative , then it means decrement
#define update_acl_rule_hit_multi(worker_index, group_index , rule_id, increment) \
    update_acl_rule_hit_multi_inline(acl_rule_hit_stat_global , worker_index, group_index , rule_id, increment)

#define decrease_acl_rule_hit(worker_index, group_index , rule_id) \
    decrease_acl_rule_hit_inline(acl_rule_hit_stat_global , worker_index, group_index , rule_id)

#endif // #if VALIDATE_STAT_NULL_POINTER



#define update_acl_rule_hit_quick(acl_rule_hit_stat , worker_index, group_index , rule_id) \
    update_acl_rule_hit_inline(acl_rule_hit_stat , worker_index, group_index , rule_id)

// if increment is negative , then it means decrement
#define update_acl_rule_hit_multi_quick(acl_rule_hit_stat , worker_index, group_index , rule_id, increment) \
    update_acl_rule_hit_multi_inline(acl_rule_hit_stat , worker_index, group_index , rule_id, increment)



#define get_acl_rule_hit_gloabl(worker_index , group_index , rule_id ) \
    ( acl_rule_hit_stat_global[(worker_index) * MAX_RULE_GROUP_CNT  * MAX_RULE_PER_GROUP   \
        + (group_index) * MAX_RULE_PER_GROUP + (rule_id) - 1  ] )


#define get_acl_rule_hit_old(worker_index , group_index , rule_id ) \
    ( acl_rule_hit_stat_old[(worker_index) * MAX_RULE_GROUP_CNT  * MAX_RULE_PER_GROUP   \
        + (group_index) * MAX_RULE_PER_GROUP + (rule_id) - 1  ] )



#endif