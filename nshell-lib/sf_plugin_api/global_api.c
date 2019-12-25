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
#include <stdlib.h>
#include <string.h>

#include "sf_shm_main.h"


int reset_all_global_config()
{
    sf_shm_map_registration_t *head = &sf_shm_map_reg_main_in_lib;
    sf_shm_map_registration_t *reg_ptr;
    shm_config_init_t init_function;
    void *ptr;

    int ret ;
    reg_ptr = head->next;

    while(reg_ptr != NULL)
    {
        if(reg_ptr->callback_ptr == NULL)
        {
            reg_ptr = reg_ptr->next;
            continue;
        }
        if( *(reg_ptr->callback_ptr) == NULL)
        {
            reg_ptr = reg_ptr->next;
            continue;
        }
        if( reg_ptr->init_func == NULL)
        {
            reg_ptr = reg_ptr->next;
            continue;
        }

        init_function = reg_ptr->init_func;
        ptr = *(reg_ptr->callback_ptr);

        ret = init_function(ptr , 1 /* is reset */);

        if(ret != 0)
        {
            return ret;
        }

        reg_ptr = reg_ptr->next;
    }


   
    return 0;
}

/* lock shm when save or load or reset shm  */
int try_lock_shm_mem()
{
    sf_shm_main_t *shm_main = get_sf_shm_main_ptr();

    if(shm_main == NULL)
    {
        return -1;
    }

    if(shm_main->sync_lock != 0)
    {
        return -2;
    }
    else
    {
        shm_main->sync_lock = 1;
        return 0;
    }
    

}


int unlock_shm_mem()
{
    sf_shm_main_t *shm_main = get_sf_shm_main_ptr();

    if(shm_main == NULL)
    {
        return -1;
    }

    shm_main->sync_lock = 0;
    return 0;
    
}

int shm_main_get_system_version(char *buffer , int max_len)
{
    sf_shm_main_t *shm_main = get_sf_shm_main_ptr();

    if(shm_main == NULL)
    {
        return -1;
    }

    strncpy(buffer , shm_main->version , max_len);
    return 0;
}


int shm_main_get_device_type_name(char *buffer , int max_len)
{
    sf_shm_main_t *shm_main = get_sf_shm_main_ptr();

    if(shm_main == NULL)
    {
        return -1;
    }

    buffer[0] = '\0';

    strncpy(buffer , SF_TARGET_DEVICE_STR , max_len);
    return 0;
}