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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sf_shm_main.h"

// only name and callback_ptr  ; for control plane
sf_shm_map_registration_t sf_shm_map_reg_main_in_lib;
// name , len , callback_ptr , init function ; for data plane
sf_shm_map_registration_t sf_shm_map_reg_main_outside;


static inline void
sf_memset_zero (void *p, uint64_t count)
{
    uint64_t *ptr = (uint64_t *)p;
    uint8_t *ptr_u8;
    while(count >= 8)
    {
        *ptr = 0;
        ptr ++;
        count -=8;
    }
    ptr_u8 = (uint8_t *)ptr;
    while(count > 0)
    {
        *ptr_u8 = 0;
        ptr_u8 ++;
        count --;
    }
}



//for control-plane
//Notice : if shm init fail , return -1
int get_shm_map_status()
{
    sf_shm_main_t *shm_main = get_sf_shm_main_ptr();
    //check if shm init over ?
    if(shm_main == NULL)
    {
        return -1;
    }
    return shm_main->status;
}

// //for data-plane
// int append_shm_map(sf_shm_map_registration_t *ptr )
// {
//     sf_shm_map_registration_t *head = &sf_shm_map_reg_main_outside;

//     ptr->next = head->next;
//     head->next = ptr;

//     return 0;
// }

// int init_shm_map_reg_outside(sf_shm_map_registration_t *head)
// {
//     sf_shm_map_registration_t *reg_ptr;
//     reg_ptr = head->next;
//     while(reg_ptr != NULL)
//     {
//         append_shm_map(reg_ptr);
//         reg_ptr = reg_ptr->next;
//     }

//     return 0;
// }

#define get_pointer_by_offset(start_ptr , offset) \
    (void *)(start_ptr + offset)


int update_shm_map_map(int in_lib)
{
    sf_shm_map_registration_t *head = &sf_shm_map_reg_main_in_lib;
    sf_shm_map_registration_t *reg_ptr;
    sf_shm_main_t *shm_main = get_sf_shm_main_ptr();
    sf_shm_map_t *shm_map;
    sf_shm_mgr_t *shm_mgr = &main_shm_mgr;
    uint8_t *start_ptr = shm_mgr->start_ptr;
    int i;
    // int flag;


    if(in_lib)
    {
        head = &sf_shm_map_reg_main_in_lib;
    }
    else
    {
        head = &sf_shm_map_reg_main_outside;
    }
    

    //check if shm init over ?
    if(shm_main == NULL)
    {
        return -1;
    }

    //check if map mem alloc is finished
    if (shm_main->status == MAP_NOT_READY)
    {
        return -1;
    }

    reg_ptr = head->next;

    while(reg_ptr != NULL)
    {
        *(reg_ptr->callback_ptr) = NULL;
        for(i=0 ; i<shm_main->map_count ; i++)
        {
            shm_map = &(shm_main->shm_maps[i]);
            if(strcmp(reg_ptr->name , shm_map->name ) == 0)
            {
                if(shm_map->offset != 0)
                {
                    *(reg_ptr->callback_ptr) = get_pointer_by_offset(start_ptr,  shm_map->offset);
                }
                break;
            }
        }

        reg_ptr = reg_ptr->next;
    }

    return 0;
}

//control-plane
int update_shm_map_map_inlib()
{
    return update_shm_map_map(1);
}

//for data-plane
int update_shm_map_map_outside()
{
    return update_shm_map_map(0);
}

int compare_shm_reg_name(sf_shm_map_registration_t * a , sf_shm_map_registration_t * b)
{
    return strcmp(a->name , b->name );
}

void sort_shm_config_reg_by_name(sf_shm_map_registration_t **head_ptr , sf_shm_map_registration_t **tail_ptr)
{
    // sf_shm_map_registration_t *head = *head_ptr;
    sf_shm_map_registration_t *result_head = NULL;
    sf_shm_map_registration_t *prev_ptr;
    sf_shm_map_registration_t *next_ptr;
    sf_shm_map_registration_t *pre_ptr;
    while(*head_ptr)
    {
        pre_ptr = *head_ptr;
        *head_ptr = pre_ptr->next;
        pre_ptr->next = NULL;
        if(result_head == NULL)
        {
            result_head = pre_ptr;
            *tail_ptr = result_head;
            continue;
        }
        
        if(compare_shm_reg_name(pre_ptr , result_head) < 0)
        {
            pre_ptr->next = result_head;
            result_head = pre_ptr;
            continue;
        }

        prev_ptr = result_head;
        next_ptr = result_head->next;

        while(prev_ptr != NULL && next_ptr != NULL)
        {
            if(compare_shm_reg_name(prev_ptr , pre_ptr) < 0)
            {
                if(compare_shm_reg_name(next_ptr , pre_ptr) > 0)
                {
                    pre_ptr->next = next_ptr;
                    prev_ptr->next = pre_ptr;
                    break;
                }
            }

            prev_ptr = next_ptr;
            next_ptr = prev_ptr->next;
        }

        if(next_ptr == NULL)
        {
            prev_ptr->next = pre_ptr;
            pre_ptr->next = NULL;
            *tail_ptr = pre_ptr;
        }
    }

    *head_ptr = result_head;

}
void sort_shm_map_reg_outside()
{
    sf_shm_map_registration_t *temp_head_ptr[SF_SHM_MEM_TYPE_COUNT];
    sf_shm_map_registration_t *temp_tail_ptr[SF_SHM_MEM_TYPE_COUNT];
    sf_shm_map_registration_t *head = &sf_shm_map_reg_main_outside;
    sf_shm_map_registration_t *reg_ptr;
    
    int index = 0;
    reg_ptr = head->next;

    for(index ; index<SF_SHM_MEM_TYPE_COUNT ; index++)
    {
        temp_head_ptr[index] = NULL;
        temp_tail_ptr[index] = NULL;
    }

    while(reg_ptr != NULL)
    {
        index = reg_ptr->mem_type;

        if(temp_head_ptr[index] == NULL)
        {
            temp_head_ptr[index] = reg_ptr;
            temp_tail_ptr[index] = reg_ptr;
        }
        else
        {
            temp_tail_ptr[index]->next = reg_ptr;
            temp_tail_ptr[index] = reg_ptr;
        }

        reg_ptr = reg_ptr->next;
        temp_tail_ptr[index]->next = NULL;
    }

    sort_shm_config_reg_by_name( &( temp_head_ptr[SF_SHM_MEM_TYPE_CONFIG] ) , 
        &( temp_tail_ptr[SF_SHM_MEM_TYPE_CONFIG] ));

    head->next = NULL;
    for(index = 0 ; index < SF_SHM_MEM_TYPE_COUNT; index++)
    {
        if(temp_head_ptr[index] == NULL)
        {
            continue;
        }

        if(head->next == NULL) // temp_head_ptr[index] != NULL
        {
            head->next = temp_head_ptr[index];
            reg_ptr = temp_tail_ptr[index];
            continue;
        }


        reg_ptr->next =  temp_head_ptr[index];
        reg_ptr = temp_tail_ptr[index];
    }
}
//for data plane ; usually , update_shm_map_map = 1
int start_shm_map_malloc(int update_map_in_lib)
{
    sf_shm_map_registration_t *head = &sf_shm_map_reg_main_outside;
    sf_shm_map_registration_t *reg_ptr;
    sf_shm_main_t *shm_main = get_sf_shm_main_ptr();
    sf_shm_map_t *shm_map;
    sf_shm_mgr_t *shm_mgr = &main_shm_mgr;
    uint8_t *start_ptr = shm_mgr->start_ptr;

    uint32_t pre_offset;
    int i;



    //check if shm init over ?
    if(shm_main == NULL)
    {
        return -1;
    }

    // pre_offset =  sizeof(sf_shm_main_t) 
    //         + shm_main->map_count * sizeof(sf_shm_map_t) 
    //         + PADDING_AFTER_SHM_MAPS;

    strncpy(shm_main->version , SF_BUILD_VERSION_STR , SF_SHM_MEM_VERSION_LEN);
    strncpy(shm_main->device_type_name ,  SF_TARGET_DEVICE_STR , SF_SHM_MEM_DEVICE_TYPE_LEN);
    
    //make sure the version is right generally
    if(strlen(shm_main->version) < SF_SHM_MEM_VERSION_MIN_LEN)
    {
        return -3;
    }
    
    shm_main->sync_lock = 0;
    /**** 0. arrage all config before stat ****/
    sort_shm_map_reg_outside();

    /**** 1. init shm_maps using sf_shm_map_reg_main_outside ****/
    reg_ptr = head->next;
    while(reg_ptr !=NULL)
    {
        shm_main->map_count++;
        shm_map = shm_main->shm_maps + shm_main->map_count - 1 ;

        strcpy(shm_map->name , reg_ptr->name);
        shm_map->len = reg_ptr->len;

        reg_ptr = reg_ptr->next;
    }

    /**** 2. init pre offset****/
    pre_offset =  sizeof(sf_shm_main_t) 
           + shm_main->map_count * sizeof(sf_shm_map_t) 
           + PADDING_AFTER_SHM_MAPS;
    
    //record config mem start offset
    shm_main->config_mem_start = pre_offset;
    shm_main->config_mem_end = shm_main->config_mem_start;
// printf("entry : pre_offset : %d\n"  ,pre_offset);
    /**** 3. init memory offset using shm_maps****/
    reg_ptr = head->next;
    for(i=0 ; i<shm_main->map_count ; i++)
    {
#if 1 
        if(strcmp(reg_ptr->name ,shm_main->shm_maps[i].name ) != 0)
        {
            return -1;
        }
        if(reg_ptr->len != shm_main->shm_maps[i].len )
        {
            return -1;
        }
#endif
        if((pre_offset + shm_main->shm_maps[i].len)  
                    > (MAIN_SHM_LEN))
        {
            return -2;
        }
        // record config mem end offset
        if(reg_ptr->mem_type != SF_SHM_MEM_TYPE_CONFIG)
        {
            if(shm_main->config_mem_end == shm_main->config_mem_start)
            {
                shm_main->config_mem_end = pre_offset - 1;
            }
        }

        //offset
        shm_main->shm_maps[i].offset = pre_offset;

        //callback_ptr
        *(reg_ptr->callback_ptr) = get_pointer_by_offset(start_ptr , pre_offset);

        //call init function
        if(reg_ptr->init_func != NULL)
        {
            reg_ptr->init_func( get_pointer_by_offset(start_ptr , pre_offset)  , 0 /* is init */);
        }
        else
        {
            sf_memset_zero( get_pointer_by_offset(start_ptr , pre_offset) , reg_ptr->len);
        }
        

        //pre_offset ++;
        pre_offset += reg_ptr->len;

        //reg_ptr ++;
        reg_ptr = reg_ptr->next;

// printf("pre_offset : %d\n"  ,pre_offset);


    }

// printf("exit pre_offset : %d\n"  ,pre_offset);


    /**** 4. update used_mem and free_mem****/
    shm_main->used_mem = pre_offset;
    shm_main->free_mem = MAIN_SHM_LEN - shm_main->used_mem;

// printf("total : %d\n"  ,MAIN_SHM_LEN);

    /**** 5. update callback_ptrs in this lib****/
    shm_main->status = MAP_MEM_FINISH;
    if(update_map_in_lib)
    {
        update_shm_map_map_inlib();
    }


    return 0;
}


