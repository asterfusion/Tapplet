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
#ifndef __include_sf_lookup_port_h__

#define __include_sf_lookup_port_h__

#include <vlib/vlib.h>


#include <stdint.h>


static_always_inline void sf_sparse_init(uint32_t **sparse_array)
{
    vec_validate_aligned( (*sparse_array) , 1  , CLIB_CACHE_LINE_BYTES);

    if(*sparse_array == NULL)
    {
        clib_error("sf_sparse_init fail.");
    }

    (*sparse_array)[0] = 0;
}


static_always_inline void sf_sparse_append_item(uint32_t **sparse_array , uint16_t port_key , uint16_t next_index)
{
    uint32_t old_cnt = 0;
    uint32_t *new_item;

    if( *sparse_array == NULL)
    {
        sf_sparse_init(sparse_array);
    }

    old_cnt = (*sparse_array)[0];

    vec_validate_aligned( (*sparse_array) , old_cnt + 1 , CLIB_CACHE_LINE_BYTES);
    
    if(*sparse_array == NULL)
    {
        clib_error("sf_sparse_append_item fail , no memory.");
    }

    new_item = ( (*sparse_array) + old_cnt + 1 )  ;  // the first one is cnt


    *new_item = 0;
    *new_item = next_index;
    *new_item <<= 16;
    *new_item |= port_key;

    (*sparse_array)[0] = old_cnt+1;
}

#define sf_sparse_get_key(u32_value) ()

static_always_inline uint32_t sf_sparse_search(uint32_t *sparse_array , uint16_t src_port , uint16_t dst_port)
{
    uint16_t i;
    uint16_t port_key;

    i = sparse_array[0];

    for( ;  i > 0 ; i--)
    {
        port_key = sparse_array[i] & 0xFFFF;
        if(dst_port == port_key  || src_port == port_key  )
        {
            return sparse_array[i] >> 16 ;
        }
    }

    return 0;
}


#endif // __include_sf_lookup_port_h__