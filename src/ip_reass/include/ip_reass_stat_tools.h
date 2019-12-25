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
#ifndef __include_ip_reass_stat_tools_h__
#define __include_ip_reass_stat_tools_h__

#include "sf_ip_reass_shm.h"


/****** stat ******/
#define update_ip_reass_stat_inline(worker_index, interface_id, member) \
    ip_reass_stat->local_stat[worker_index][(interface_id)-1].member += 1

// if increment is negative , then it means decrement
#define update_ip_reass_stat_multi_inline(worker_index, interface_id, member, increment) \
    ip_reass_stat->local_stat[worker_index][(interface_id)-1].member += increment

#define decrease_ip_reass_stat_inline(worker_index, interface_id, member) \
    ip_reass_stat->local_stat[worker_index][(interface_id)-1].member -= 1




#ifdef VALIDATE_STAT_NULL_POINTER

#define update_ip_reass_stat(worker_index, interface_id, member)         \
    if ((ip_reass_stat) != NULL)                                 \
    {                                                              \
        update_ip_reass_stat_inline(worker_index, interface_id, member); \
    }

// if increment is negative , then it means decrement
#define update_ip_reass_stat_multi(worker_index, interface_id, member, increment)         \
    if ((ip_reass_stat) != NULL)                                                  \
    {                                                                               \
        update_ip_reass_stat_multi_inline(worker_index, interface_id, member, increment); \
    }

#define decrease_ip_reass_stat(worker_index, interface_id, member)         \
    if ((ip_reass_stat) != NULL)                                   \
    {                                                                \
        decrease_ip_reass_stat_inline(worker_index, interface_id, member); \
    }

#else // #if VALIDATE_STAT_NULL_POINTER

#define update_ip_reass_stat(worker_index, interface_id, member) \
    update_ip_reass_stat_inline(worker_index, interface_id, member)

// if increment is negative , then it means decrement
#define update_ip_reass_stat_multi(worker_index, interface_id, member, increment) \
    update_ip_reass_stat_multi_inline(worker_index, interface_id, member, increment)

#define decrease_ip_reass_stat(worker_index, interface_id, member) \
    decrease_ip_reass_stat_inline(worker_index, interface_id, member)

#endif // #if VALIDATE_STAT_NULL_POINTER




#endif