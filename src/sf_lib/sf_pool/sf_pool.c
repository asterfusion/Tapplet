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
#include <vlib/vlib.h>

#include <vppinfra/cache.h>

#include "sf_thread_tools.h"
#include "sf_pool.h"

// #define SF_DEBUG
#include "sf_debug.h"

#define SF_POOL_ALIGNED_LEN (64)

sf_pool_head_t *pool_record_header = NULL;

static uint32_t
align_data_len(uint32_t data_len)
{
    uint32_t aligned_len ;
    uint32_t align_page_cnt =  data_len / CLIB_CACHE_LINE_BYTES ;
    align_page_cnt += (data_len % CLIB_CACHE_LINE_BYTES) > 0 ? 1 : 0;

    if(data_len <= SF_POOL_ALIGNED_LEN)
    {
        aligned_len = SF_POOL_ALIGNED_LEN;
    }
    else
    {
        aligned_len = align_page_cnt * CLIB_CACHE_LINE_BYTES;
    }
    
    return aligned_len;
}



sf_pool_per_thread_t *get_local_pool_ptr(sf_pool_head_t *sf_pool_hdr)
{
    uint32_t worker_index = sf_get_current_worker_index();

    return sf_pool_hdr->per_worker_pool + worker_index;
}

/*******************************************/
/*** fill / free local cache  **************/
/*******************************************/

static inline int 
sf_pool_increase_nodes(sf_pool_head_t *sf_pool_hdr , sf_pool_node_t **target_array  ,  int needed_size)
{
    uint8_t *new_mem;
    int i;

    if(sf_pool_hdr->pool_mem_inc_time < sf_pool_hdr->pool_mem_inc_max )
    {
        new_mem = clib_mem_alloc_aligned_at_offset( sf_pool_hdr->data_align_len * needed_size
            , CLIB_CACHE_LINE_BYTES , 0 , 0 /* don't call os_out_of_memory */);

        if(new_mem)
        {
            for(i=0 ; i<needed_size ; i++)
            {
                target_array[i] = (sf_pool_node_t *)(new_mem + sf_pool_hdr->data_align_len * i);
            }

            sf_pool_hdr->pool_mem_inc_time ++;

            return needed_size;
        }
    }

    return 0;
}

int sf_pool_fill_cache( sf_pool_head_t *sf_pool_hdr , sf_pool_node_t **target_array , int is_runtime)
{
    int index_or_ret = 0 ;
    int needed_size = sf_pool_hdr->local_cache_size;
    if(is_runtime)
    {
        clib_spinlock_lock( &(sf_pool_hdr->spin_lock) );
        needed_size = sf_pool_hdr->fill_free_size;

        sf_debug("lock ok, need_size %d\n" , needed_size);
    }

    sf_debug("total_cnt : %d free %d\n" , sf_pool_hdr->total_cnt , sf_pool_hdr->free_cnt);
    
    if( PREDICT_FALSE(sf_pool_hdr->free_cnt == 0 ) )
    {
        //alloc new and fill target_array
        index_or_ret =  sf_pool_increase_nodes(sf_pool_hdr , target_array , needed_size);

        sf_pool_hdr->total_cnt += index_or_ret;

        sf_debug("alloc new : index_or_ret %d  \n" , index_or_ret );
    }
    else
    {
        while(sf_pool_hdr->freelist != NULL && index_or_ret < needed_size)
        {
            target_array[index_or_ret] = sf_pool_hdr->freelist;
            index_or_ret ++;

            sf_pool_hdr->freelist = sf_pool_hdr->freelist->next;
        }

        sf_pool_hdr->free_cnt -= index_or_ret;

        sf_debug("use old  : index_or_ret %d  \n" , index_or_ret );
    }

    sf_debug("total_cnt : %d free %d\n" , sf_pool_hdr->total_cnt , sf_pool_hdr->free_cnt);

    if(is_runtime)
    {
        clib_spinlock_unlock( &(sf_pool_hdr->spin_lock) );
        sf_debug("unlock ok, index_or_ret %d\n" , index_or_ret);
    }

    return index_or_ret;
}




/*************************/
/*** init  **************/
/*************************/

int sf_pool_init_local(sf_pool_per_thread_t *local_pool, sf_pool_head_t *sf_pool_hdr)
{

    memset(local_pool, 0, sizeof(sf_pool_per_thread_t));

    local_pool->callback_head = sf_pool_hdr;
    local_pool->stack_max_len = sf_pool_hdr->local_cache_size;
    local_pool->free_list_max_len = sf_pool_hdr->fill_free_size;

    //stack init
    local_pool->local_pool_stack = vec_new_ha(void * , local_pool->stack_max_len + local_pool->free_list_max_len , 
        0 , CLIB_CACHE_LINE_BYTES);
    
    if(local_pool->local_pool_stack == NULL)
    {
        clib_error("init %s local pool failed : memory not enough" , sf_pool_hdr->name);
    }

    //pool_init
    local_pool->current_stack_len = 
        sf_pool_fill_cache( sf_pool_hdr ,  local_pool->local_pool_stack ,    0 );

    return 0;
}

int sf_pool_init(
    sf_pool_head_t *sf_pool_hdr,
    char *pool_name, 
    uint32_t data_len, 
    uint32_t init_size_per_worker, 
    uint32_t local_cache_size,
    uint32_t fill_free_size,
    uint32_t increment_max_times
)
{
    SF_ASSERT(sf_pool_hdr != NULL);

    uint8_t *memory_start;
    sf_pool_node_t *pool_node;
    int i;
    int ret;
    uint32_t worker_thread_cnt = sf_vlib_num_workers();

    memset(sf_pool_hdr, 0, sizeof(sf_pool_head_t));

    //append record
    sf_pool_hdr->next = pool_record_header;
    pool_record_header = sf_pool_hdr;

    //config
    sf_pool_hdr->name = pool_name;

    sf_pool_hdr->data_len = data_len;
    sf_pool_hdr->data_align_len = align_data_len(data_len);

    sf_pool_hdr->init_size = init_size_per_worker  * sf_vlib_num_workers() ;
    sf_pool_hdr->increase_size = fill_free_size; // not used 
    sf_pool_hdr->fill_free_size = fill_free_size;
    sf_pool_hdr->pool_mem_inc_max = increment_max_times;
    sf_pool_hdr->local_cache_size = local_cache_size;
    //lock
    clib_spinlock_init( &(sf_pool_hdr->spin_lock) );


    if(fill_free_size > local_cache_size)
    {
        clib_error("init %s fail , fill_free_size > local_cache_size" , pool_name);
    }

    //init first memory
    memory_start = clib_mem_alloc_aligned_at_offset(
            sf_pool_hdr->data_align_len * sf_pool_hdr->init_size
            , CLIB_CACHE_LINE_BYTES , 0 ,0);
    if(memory_start == NULL)
    {
        clib_error("init %s fail , memory not enough" , pool_name);
    }

    //split memory
    for(i=0;  i<sf_pool_hdr->init_size ; i ++)
    {
        pool_node = (sf_pool_node_t *)(memory_start + i * sf_pool_hdr->data_align_len);

        pool_node->next = sf_pool_hdr->freelist;
        sf_pool_hdr->freelist = pool_node;
    }

    sf_pool_hdr->total_cnt = sf_pool_hdr->init_size;
    sf_pool_hdr->free_cnt = sf_pool_hdr->init_size;

    // init pre thread pool : Notice : keep sizeof(sf_pool_per_thread_t) aligned
    sf_pool_hdr->per_worker_pool = clib_mem_alloc_aligned(sizeof(sf_pool_per_thread_t) * worker_thread_cnt , sizeof(sf_pool_per_thread_t));
    for (i = 0; i < worker_thread_cnt; i++)
    {
        ret = sf_pool_init_local(sf_pool_hdr->per_worker_pool + i , sf_pool_hdr);
        if (ret != 0)
        {   
            return ret;
        }
    }

    return 0;
}

/*************************/
/*** format **************/
/*************************/
uint8_t *format_sf_pool_local(uint8_t *s, va_list *args)
{
    sf_pool_per_thread_t *local_pool = va_arg(*args, sf_pool_per_thread_t *);
    uint32_t list_cnt;
    s = format(s, "SF pool local:\n");
    s = format(s, "%30s : %d\n", "current_stack_len", local_pool->current_stack_len);
    s = format(s, "%30s : %d\n", "stack_max_len", local_pool->stack_max_len);
    s = format(s, "%30s : %d\n", "free_list_len", local_pool->free_list_len);
    s = format(s, "%30s : %d\n", "free_list_max_len", local_pool->free_list_max_len);

    s = format(s, "%s\n", "freelist:");
    sf_pool_node_t *ptr;
    ptr = local_pool->local_free_list;
    list_cnt = 0;
    while (ptr != NULL)
    {
        list_cnt++;
        // s = format(s , "%p\n" , ptr);
        ptr = ptr->next;
    }
    s = format(s, "total : %d\n", list_cnt);

    return s;
}

uint8_t *format_sf_pool(uint8_t *s, va_list *args)
{
    sf_pool_head_t *sf_pool = va_arg(*args, sf_pool_head_t *);
    sf_pool_per_thread_t *local_pool;
    uint32_t worker_cnt = sf_vlib_num_workers();
    uint32_t worker_index;

    s = format(s, "SF pool(%s) -- \ntotal workers : %d \n", sf_pool->name ? sf_pool->name : "", worker_cnt);

#define for_each_pool_mem \
FUNC(init_size)\
FUNC(increase_size)\
FUNC(fill_free_size)\
FUNC(local_cache_size)\
FUNC(data_len)\
FUNC(data_align_len)\
FUNC(free_cnt)\
FUNC(total_cnt)\
FUNC(pool_mem_inc_time)\
FUNC(pool_mem_inc_max)


#define FUNC(member) s=format(s, "%30s : %d\n", #member , sf_pool->member);
for_each_pool_mem
#undef FUNC

    s = format(s, "%s\n", "freelist:");
    sf_pool_node_t *ptr;
    ptr = sf_pool->freelist;
    int list_cnt = 0;
    while (ptr != NULL)
    {
        list_cnt++;
        // s = format(s , "%p\n" , ptr);
        ptr = ptr->next;
    }
    s = format(s, "total : %d\n", list_cnt);

    for (worker_index = 0; worker_index < worker_cnt; worker_index++)
    {
        local_pool = sf_pool->per_worker_pool + worker_index;
        s = format(s, "worker(%d) : %U", worker_index,
                   format_sf_pool_local, local_pool);
    }

    return s;
}


uint8_t *format_sf_pool_mem(uint8_t *s, va_list *args)
{
    sf_pool_head_t *sf_pool = va_arg(*args, sf_pool_head_t *);

    uint32_t total_mem = 0;
    uint32_t max_mem = 0;
    uint32_t local_total_mem = 0;

    total_mem = sf_pool->data_align_len * sf_pool->total_cnt;
    max_mem = sf_pool->data_align_len * (sf_pool->init_size  + sf_pool->increase_size * sf_pool->pool_mem_inc_max);

    local_total_mem = sf_vlib_num_workers() * (sf_pool->local_cache_size + sf_pool->fill_free_size ) * 8;
    total_mem += local_total_mem;
    max_mem += local_total_mem;

#define B_TO_MB(size) ((size/1024/1024))

    s = format(s, "-- %s --  \nprecent : %u(%uM) \nmax : %u(%uM)\n", sf_pool->name ? sf_pool->name : ""  , 
        total_mem , B_TO_MB(total_mem) , max_mem , B_TO_MB(max_mem));
   

    return s;
}


static clib_error_t *sf_stat_pool_fn(vlib_main_t *vm,
                                unformat_input_t *input,
                                vlib_cli_command_t *cmd)
{
    // sf_main_t * sm = &sf_main;

    sf_pool_head_t *pool_ptr = pool_record_header;

    uint8_t show_mem = 0;
    char *pool_name = NULL;

    while (unformat_check_input(input) != UNFORMAT_END_OF_INPUT)
    {
        if (unformat(input, "mem"))
            show_mem = 1;
        if (unformat(input, "%s" , &pool_name))
            ;
        else
            break;
    }

    if(pool_name != NULL)
    {
        vec_add1(pool_name , 0);
        vlib_cli_output(vm , "TARGET : %s\n" , pool_name);
    }

    if (!show_mem)
    {
        while (pool_ptr != NULL)
        {
            if( pool_name != NULL && pool_ptr->name != NULL 
                && strncmp(pool_ptr->name , pool_name , strlen(pool_name)) != 0 )
            {
                pool_ptr = pool_ptr->next;
                continue;
            }
            vlib_cli_output(vm, "%U", format_sf_pool, pool_ptr );
            pool_ptr = pool_ptr->next;
        }
    }
    else
    {
        while (pool_ptr != NULL)
        {
            if( pool_name != NULL && pool_ptr->name != NULL 
                && strncmp(pool_ptr->name , pool_name , strlen(pool_name)) != 0 )
            {
                pool_ptr = pool_ptr->next;
                continue;
            }
            vlib_cli_output(vm, "%U", format_sf_pool_mem, pool_ptr );
            pool_ptr = pool_ptr->next;
        }
    }

    if(pool_name != NULL)
    {
        vec_free(pool_name);
    }

    return 0;
}

/**
 * @brief CLI command to enable/disable the sf plugin.
 */
VLIB_CLI_COMMAND(sf_show_pool_command, static) = {
    .path = "sf show pool",
    .short_help =
        "sf show pool [mem] [<pool name>]",
    .function = sf_stat_pool_fn,
    .is_mp_safe = 1,
};
