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
#include "acl_rte_def.h"

#include <vnet/vnet.h>
#include <vnet/plugin/plugin.h>
#include <vlib/cli.h>
#include <vlib/threads.h>
#include <vppinfra/os.h>

#include <stdio.h>

#include "pkt_wdata.h"
#include "sf_main.h"


#include "acl_main.h"
#include "sf_acl_shm.h"

#include "sf_debug.h"

sf_acl_main_t sf_acl_main;


uint32_t *acl_rule_hit_stat_global;
uint32_t *acl_rule_hit_stat_old;

uint32_t acl_rule_hit_stat_mem_size;

static clib_error_t * acl_main_init (vlib_main_t * vm)
{
    clib_error_t * error = 0;

    fix_sf_main_link_error();

    CALL_SHM_INIT_FUNC(vm, error);

    int worker_num  = sf_vlib_num_workers();


#define CHECK_and_CLEAR_MEMORY(ptr , size) if(!(ptr)) \
{ clib_error("acl_main_init alloc memory failed."); } \
else  \
{ sf_memset_zero((ptr) , (size)); }

    acl_rule_hit_stat_mem_size = sizeof(uint32_t) * (worker_num * MAX_RULE_GROUP_CNT  * MAX_RULE_PER_GROUP);

    acl_rule_hit_stat_global = clib_mem_alloc_aligned_at_offset( 
        acl_rule_hit_stat_mem_size , 
        CLIB_CACHE_LINE_BYTES , 0 , 0 );

    acl_rule_hit_stat_old = clib_mem_alloc_aligned_at_offset( 
        acl_rule_hit_stat_mem_size , 
        CLIB_CACHE_LINE_BYTES , 0 , 0 );

    CHECK_and_CLEAR_MEMORY(acl_rule_hit_stat_global , acl_rule_hit_stat_mem_size );

    CHECK_and_CLEAR_MEMORY(acl_rule_hit_stat_old , acl_rule_hit_stat_mem_size );

    return error;
}

VLIB_INIT_FUNCTION (acl_main_init);


/*****************/
/** acl sync lock **/
/*****************/
clib_spinlock_t *acl_sync_locks;

static clib_error_t * acl_sync_lock_init (vlib_main_t * vm)
{
    clib_error_t * error = 0;

    int worker_num = sf_vlib_num_workers();
    int i;

    acl_sync_locks = vec_new( clib_spinlock_t  , worker_num);
    if(acl_sync_locks == NULL)
    {
        clib_error( "alloc acl sync lock failed.\n");
    }

    for(i=0 ; i<worker_num ; i++)
    {
        clib_spinlock_init( acl_sync_locks + i  );
    }
   
    return error;
}

VLIB_INIT_FUNCTION (acl_sync_lock_init);
