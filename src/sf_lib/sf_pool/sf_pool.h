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
#ifndef __include_sf_pool_h__
#define __include_sf_pool_h__

#include <stdint.h>
#include <vppinfra/clib.h>
#include <vppinfra/vec.h>

#include "sf_debug.h"


#define SF_POOL_DEFAULT_CACHE_SIZE 10000

typedef struct sf_pool_node{
    struct sf_pool_node *next;
    uint8_t data[0];
}sf_pool_node_t;

typedef sf_pool_node_t *sf_pool_pnode_t; // carefully use , don't mix up it with sf_pool_node_t

#define pnode_of_data(data) (data)

struct sf_pool_head;

typedef union{
    uint8_t cache_line[64];
    struct{
        uint32_t current_stack_len;
        uint32_t stack_max_len;

        uint32_t free_list_len;
        uint32_t free_list_max_len;

        sf_pool_node_t **local_pool_stack; // contains normal stack and freelist stack

        sf_pool_node_t *local_free_list;
        sf_pool_node_t *local_last_node;

        struct sf_pool_head *callback_head;
    };
}sf_pool_per_thread_t;

typedef struct sf_pool_head{
    struct sf_pool_head *next;
    char *name;

    //memory config    
    uint32_t init_size;
    uint32_t increase_size; // keep increase_size = fill_free_size
    uint32_t fill_free_size;
    uint32_t pool_mem_inc_max;
    uint32_t local_cache_size;

    // data
    uint32_t data_len;  // data len ; true len
    uint32_t data_align_len;

    // runtime 
    uint32_t pool_mem_inc_time;

    clib_spinlock_t spin_lock;

    uint32_t free_cnt;
    uint32_t total_cnt;

    sf_pool_node_t *freelist;

    sf_pool_per_thread_t *per_worker_pool;

}sf_pool_head_t;


extern sf_pool_head_t *pool_record_header;


int sf_pool_init(
    sf_pool_head_t *sf_pool_hdr,
    char *pool_name, 
    uint32_t data_len, 
    uint32_t init_size_per_worker, 
    uint32_t local_cache_size,
    uint32_t fill_free_size,
    uint32_t increment_max_times
);


sf_pool_per_thread_t * get_local_pool_ptr(sf_pool_head_t *sf_pool_hdr);

int sf_pool_fill_cache( sf_pool_head_t *sf_pool_hdr , sf_pool_node_t **target_array , int is_runtime);



static_always_inline
void sf_pool_recycle_cache(sf_pool_head_t *sf_pool_hdr , sf_pool_node_t *local_free_list , 
        sf_pool_node_t *local_last_node )
{
    clib_spinlock_lock( &(sf_pool_hdr->spin_lock) );

    local_last_node->next = sf_pool_hdr->freelist;
    sf_pool_hdr->freelist = local_free_list;
    sf_pool_hdr->free_cnt += sf_pool_hdr->fill_free_size;

    clib_spinlock_unlock( &(sf_pool_hdr->spin_lock) );
}


static_always_inline void *
alloc_node_from_pool(sf_pool_per_thread_t *local_pool )
{
    int ret;
    sf_pool_node_t *pnode;
    SF_ASSERT(local_pool != NULL);

    if(PREDICT_FALSE(local_pool->current_stack_len == 0))
    {
        ret = sf_pool_fill_cache(local_pool->callback_head , local_pool->local_pool_stack , 1);
        if(ret == 0)
        {
            return NULL;
        }
        
        local_pool->current_stack_len = ret;
    }

    /* get pnode */
    local_pool->current_stack_len --;
    pnode = local_pool->local_pool_stack[local_pool->current_stack_len];

    // judge if it is alloced from freelist
    if(local_pool->current_stack_len > local_pool->stack_max_len)
    {
        local_pool->local_free_list = local_pool->local_pool_stack[local_pool->current_stack_len - 1];
        local_pool->free_list_len -= 1;
    }
    else if(local_pool->current_stack_len == local_pool->stack_max_len)
    {
        local_pool->local_free_list = NULL;
        local_pool->local_last_node = NULL;
        local_pool->free_list_len -= 1;
    }
    

    return pnode;

}

static_always_inline void 
free_node_to_pool(sf_pool_per_thread_t *local_pool , void *data_ptr)
{
    SF_ASSERT(local_pool != NULL);
    SF_ASSERT(data_ptr != NULL);

    local_pool->local_pool_stack[local_pool->current_stack_len] = (sf_pool_node_t *)data_ptr;
    local_pool->current_stack_len ++;

    if( local_pool->current_stack_len > local_pool->stack_max_len )
    {
        ((sf_pool_node_t *)data_ptr)->next =  local_pool->local_free_list ;
        local_pool->local_free_list = (sf_pool_node_t *)data_ptr;
        local_pool->free_list_len ++;

        if(local_pool->local_last_node == NULL)
        {
            SF_ASSERT(local_pool->free_list_len == 1);

            //first node
            local_pool->local_last_node = local_pool->local_free_list;
        }

        if(local_pool->free_list_len >= local_pool->free_list_max_len )
        {
            sf_pool_recycle_cache(local_pool->callback_head , local_pool->local_free_list , local_pool->local_last_node);
            local_pool->local_free_list = NULL;
            local_pool->local_last_node = NULL;
            local_pool->free_list_len  = 0 ;
            local_pool->current_stack_len -= local_pool->free_list_max_len;

        }

    }
    
}

uint8_t *format_sf_pool_local(uint8_t * s, va_list * args);
uint8_t * format_sf_pool(uint8_t * s, va_list * args);
uint8_t *format_sf_pool_mem(uint8_t *s, va_list *args);



#endif
